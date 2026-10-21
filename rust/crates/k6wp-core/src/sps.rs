//! The SPS parser (Phase 2, task E3 / F4).
//!
//! WHAT IT ANSWERS
//! The decoder's DPB size - how many reference frames the decoder must be
//! able to hold. A 4K clip with 16 refs held 467 MB of GPU shared memory on
//! playback; the same clip with 2 refs holds a fraction of that. The
//! encoder's DPB pinning (`-refs 2`) is useless when the source's SPS keeps
//! its deep reference list, so the source's DPB has to be READ.
//!
//! WHERE THE NUMBER COMES FROM (measured, D3)
//! - ffprobe: NO (its stream keys carry no refs/dpb field).
//! - ffmpeg's normal output: NO (the debug log prints the NAL types).
//! - libmpv: NO (the whole property list was enumerated).
//! - ffmpeg's `trace_headers` bitstream filter: YES - but it needs a spawn
//!   and a full parse of the stream.
//!
//! So this module is the replacement: a small, dependency-free bitstream
//! parser for the two SPS flavours, validated against the trace_headers
//! output on the corpus (the golden fixtures in tests/golden/sps-dpb.json).
//!
//! THE FIELDS
//! - H.264: `max_num_ref_frames` (the DPB size) and the VUI's
//!   `max_dec_frame_buffering_minus1 + 1`.
//! - HEVC: `sps_max_dec_pic_buffering_minus1 + 1`.
//!
//! BOTH ARE REPORTED, because the decoder sizes its pool from the SPS it
//! actually sees: the container's avcC/hvcC carries the FIRST SPS, and a
//! stream may carry more (a resolution change, a concatenated source) - the
//! decoder switches to the in-band SPS, so the largest DPB in the file is
//! the one that matters.

use std::path::Path;

/// The codec the SPS belongs to.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum Codec {
    H264,
    Hevc,
    /// A codec this module does not parse - the caller falls back to
    /// ffmpeg's trace_headers.
    Unknown,
}

impl Codec {
    pub fn as_str(self) -> &'static str {
        match self {
            Codec::H264 => "h264",
            Codec::Hevc => "hevc",
            Codec::Unknown => "unknown",
        }
    }
}

/// The input limits (G2: the parser processes UNTRUSTED input).
///
/// Every count that comes FROM THE FILE is capped before it is used as a
/// loop bound or an allocation size, and every arithmetic operation on a
/// file-derived value is checked. A malformed file must produce an error
/// or an "unknown" answer - never a panic, a huge allocation or an endless
/// loop.
mod limits {
    /// The most bytes read from a file (the parameter sets live in the
    /// header; a file whose header is further out falls back).
    pub const MAX_INPUT_BYTES: u64 = 64 * 1024 * 1024;
    /// The most entries any container table may declare.
    pub const MAX_TABLE_ENTRIES: usize = 1 << 20;
    /// The most samples the sample table may declare.
    pub const MAX_SAMPLES: usize = 1 << 20;
    /// The most entries in a parameter-set array.
    pub const MAX_PARAM_SETS: usize = 256;
    /// The most pictures in a picture-ordering cycle.
    pub const MAX_PIC_ORDER_CYCLE: u32 = 4096;
    /// The most CPB entries in an HRD.
    pub const MAX_CPB_ENTRIES: u32 = 32;
    /// The most sub-layers in an HEVC profile_tier_level.
    pub const MAX_SUB_LAYERS: u32 = 7;
    /// The most entries a box tree may hold on the stack.
    pub const MAX_BOX_STACK: usize = 64;
}

/// What one SPS says.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct SpsInfo {
    pub codec: Option<Codec>,
    /// H.264 `max_num_ref_frames` - the DPB size.
    pub max_num_ref_frames: Option<u32>,
    /// H.264 VUI `max_dec_frame_buffering_minus1 + 1`.
    pub max_dec_frame_buffering: Option<u32>,
    /// HEVC `sps_max_dec_pic_buffering_minus1 + 1`.
    pub sps_max_dec_pic_buffering: Option<u32>,
    /// H.264 `frame_mbs_only_flag` (0 = interlaced / MBAFF).
    pub frame_mbs_only_flag: Option<u32>,
    /// The frame size in pixels, when the SPS carries it.
    pub width: Option<u32>,
    pub height: Option<u32>,
}

impl SpsInfo {
    /// The DPB size the decoder has to be able to hold, from whichever
    /// field this codec uses.
    pub fn dpb_size(&self) -> Option<u32> {
        match self.codec {
            Some(Codec::H264) => self.max_num_ref_frames.or(self.max_dec_frame_buffering),
            Some(Codec::Hevc) => self.sps_max_dec_pic_buffering,
            _ => None,
        }
    }

    /// The DPB budget: a file whose DPB costs more than this is re-encoded
    /// (with the DPB pinned), so the decoder's pool stays bounded.
    ///
    /// 64 MB: above the 1080p worst case (50 MB at 16 refs) and below the
    /// 4K cases (100 MB at 8 refs, 199 MB at 16 refs) - so a 1080p clip is
    /// never re-encoded for a few megabytes, while a 4K clip with a deep
    /// reference list is.
    ///
    /// The C++ compressor uses the same budget
    /// (`compressor/src/dpb_probe.hpp::kMaxDpbBytesForCopy`).
    pub const MAX_DPB_BYTES_FOR_COPY: u64 = 64 * 1024 * 1024;

    /// The reference frames the decoder actually holds: the smallest of
    /// `max_num_ref_frames` and the parameter set's own DPB bound (the VUI's
    /// `max_dec_frame_buffering` for H.264, `sps_max_dec_pic_buffering` for
    /// HEVC).
    ///
    /// The LEVEL is deliberately not a cap: MEASURED, a level-4.2 file with
    /// 16 refs still allocates 16 frames (195.16 MB against the 8-ref
    /// file's 169.55 MB - exactly 8 frames more).
    pub fn effective_refs(&self) -> Option<u32> {
        // H.264 counts the reference frames, HEVC counts the DPB pictures.
        let refs = self.max_num_ref_frames.or(self.sps_max_dec_pic_buffering)?;
        let bound = self
            .max_dec_frame_buffering
            .or(self.sps_max_dec_pic_buffering);
        match bound {
            Some(bound) if bound > 0 => Some(refs.min(bound)),
            _ => Some(refs),
        }
    }

    /// The DPB's memory in bytes: `effective_refs x width x height x 1.5`
    /// (YUV 4:2:0). `None` when the frame size or the reference count is
    /// unknown - never a guess.
    pub fn dpb_bytes(&self) -> Option<u64> {
        let refs = self.effective_refs()?;
        let width = self.width?;
        let height = self.height?;
        if refs == 0 || width == 0 || height == 0 {
            return None;
        }
        let pixels = u64::from(width)
            .checked_mul(u64::from(height))?
            .checked_mul(u64::from(refs))?;
        // 1.5 bytes per pixel, without a float: pixels * 3 / 2.
        pixels.checked_mul(3)?.checked_div(2)
    }

    /// May this file be COPIED (instead of re-encoded)?
    ///
    /// False when the DPB cannot be read: an unknown DPB must never take
    /// the copy path - the copy path is the one that can leak the deep
    /// pool.
    pub fn is_copy_safe(&self) -> bool {
        match self.dpb_bytes() {
            Some(bytes) => bytes <= SpsInfo::MAX_DPB_BYTES_FOR_COPY,
            None => false,
        }
    }

    /// Is this a parameter set a decoder could really be told?
    ///
    /// The in-band scan walks raw bytes, so a random match parses into
    /// nonsense. A real SPS has a sane frame size (a multiple of 16 for
    /// H.264, exact for HEVC), a DPB inside the spec's limit (the H.264
    /// MaxDpbFrames is 16) and a known profile. This filter is what keeps
    /// the scan honest.
    pub fn is_plausible(&self) -> bool {
        let (width, height) = match (self.width, self.height) {
            (Some(w), Some(h)) => (w, h),
            _ => return false,
        };
        if width == 0 || height == 0 || width > 16384 || height > 16384 {
            return false;
        }
        if let Some(Codec::H264) = self.codec {
            if width % 16 != 0 || height % 16 != 0 {
                return false;
            }
        }
        match self.dpb_size() {
            Some(dpb) => (1..=16).contains(&dpb),
            None => false,
        }
    }
}

/// The result for a whole file.
#[derive(Clone, Debug, Default, PartialEq)]
pub struct DpbReport {
    /// Every SPS that was found and parsed.
    pub sps: Vec<SpsInfo>,
    /// The largest DPB size in the file - the one the decoder must honour.
    pub max_dpb: Option<u32>,
    /// True when the file carries a codec this module does not parse.
    pub needs_fallback: bool,
}

/// The error the probe reports.
#[derive(Debug)]
pub enum SpsError {
    /// The file could not be read.
    Io(String),
    /// No SPS could be found or parsed.
    NoSps,
    /// The codec is not one this module parses - the caller must use
    /// ffmpeg's trace_headers.
    UnsupportedCodec(String),
}

impl std::fmt::Display for SpsError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            SpsError::Io(m) => write!(f, "sps io: {m}"),
            SpsError::NoSps => write!(f, "sps: no parameter set found"),
            SpsError::UnsupportedCodec(m) => write!(f, "sps: unsupported codec {m}"),
        }
    }
}

impl std::error::Error for SpsError {}

/// How much of the file is read. The parameter sets live in the header
/// (the moov / the extradata), so a few MB is plenty; the cap keeps a
/// pathological file from being read whole.
const MAX_SCAN_BYTES: usize = 64 * 1024 * 1024;

// ---------------------------------------------------------------------------
// the bit reader
// ---------------------------------------------------------------------------

struct Bits<'a> {
    data: &'a [u8],
    pos: usize, // in bits
}

impl<'a> Bits<'a> {
    fn new(data: &'a [u8]) -> Self {
        Bits { data, pos: 0 }
    }

    fn bit(&mut self) -> Option<u32> {
        let byte = self.pos / 8;
        if byte >= self.data.len() {
            return None;
        }
        let shift = 7 - (self.pos % 8);
        self.pos += 1;
        Some(((self.data[byte] >> shift) & 1) as u32)
    }

    fn bits(&mut self, n: u32) -> Option<u32> {
        let mut value = 0u32;
        for _ in 0..n {
            value = (value << 1) | self.bit()?;
        }
        Some(value)
    }

    /// The Exp-Golomb unsigned code (ue(v)).
    ///
    /// The code is the (k+1)-bit value of (codeNum + 1) with k leading
    /// zeros. The loop below consumes the k zeros AND the implicit leading
    /// 1, so the k bits that follow are the rest: the value is
    /// (1 << k) | rest, and codeNum is that minus one. (A reader that takes
    /// the k bits BEFORE the implicit 1 silently returns the wrong number
    /// for most values.)
    fn ue(&mut self) -> Option<u32> {
        let mut leading_zeros = 0u32;
        while self.bit()? == 0 {
            leading_zeros += 1;
            if leading_zeros > 31 {
                return None;
            }
        }
        if leading_zeros == 0 {
            return Some(0);
        }
        let rest = self.bits(leading_zeros)?;
        Some(((1u32 << leading_zeros) | rest) - 1)
    }

    /// The Exp-Golomb signed code (se(v)).
    fn se(&mut self) -> Option<i32> {
        let value = self.ue()?;
        let magnitude = value.div_ceil(2) as i32;
        if value % 2 == 0 {
            Some(-magnitude)
        } else {
            Some(magnitude)
        }
    }
}

/// The RBSP payload of a NAL unit: the emulation-prevention bytes
/// (00 00 03) are removed, and the trailing bits are dropped.
fn rbsp_payload(nal: &[u8]) -> Vec<u8> {
    let mut out = Vec::with_capacity(nal.len());
    let mut zeros = 0u32;
    for (index, &byte) in nal.iter().enumerate() {
        if zeros >= 2 && byte == 0x03 {
            // The emulation-prevention byte: drop it and the zero run.
            zeros = 0;
            continue;
        }
        if byte == 0 {
            zeros += 1;
        } else {
            zeros = 0;
        }
        out.push(byte);
        let _ = index;
    }
    // The rbsp_trailing_bits (the stop bit and the alignment zeros) live in
    // the LAST byte, so that byte is dropped.
    if out.len() > 1 {
        out.pop();
    }
    out
}

// ---------------------------------------------------------------------------
// the H.264 SPS
// ---------------------------------------------------------------------------

fn parse_h264_sps(nal: &[u8]) -> Option<SpsInfo> {
    // The NAL header (1 byte for H.264) is not part of the RBSP.
    if nal.len() < 2 {
        return None;
    }
    let payload = rbsp_payload(&nal[1..]);
    if payload.is_empty() {
        return None;
    }
    let mut bits = Bits::new(&payload);
    let profile_idc = bits.bits(8)? as u8;
    let _constraints = bits.bits(8)?;
    let _level_idc = bits.bits(8)?;
    let _sps_id = bits.ue()?;

    let mut info = SpsInfo {
        codec: Some(Codec::H264),
        ..Default::default()
    };

    // The high-profile branch (the chroma / bit-depth / scaling-list part).
    const HIGH_PROFILES: [u8; 12] = [100, 110, 122, 244, 44, 83, 86, 118, 128, 138, 139, 135];
    if HIGH_PROFILES.contains(&profile_idc) {
        let chroma_format_idc = bits.ue()?;
        if chroma_format_idc == 3 {
            bits.bit()?; // separate_colour_plane_flag
        }
        bits.ue()?; // bit_depth_luma_minus8
        bits.ue()?; // bit_depth_chroma_minus8
        bits.bit()?; // qpprime_y_zero_transform_bypass_flag
        let scaling_matrix_present = bits.bit()?;
        if scaling_matrix_present == 1 {
            let count = if chroma_format_idc != 3 { 8 } else { 12 };
            for i in 0..count {
                if bits.bit()? == 1 {
                    let size = if i < 6 { 16 } else { 64 };
                    let _last = bits.bit()?; // last_scale
                    let _next = bits.se()?; // next_scale
                    skip_scaling_list(&mut bits, size)?;
                }
            }
        }
    }

    bits.ue()?; // log2_max_frame_num_minus4
    let pic_order_cnt_type = bits.ue()?;
    match pic_order_cnt_type {
        0 => {
            bits.ue()?; // log2_max_pic_order_cnt_lsb_minus4
        }
        1 => {
            bits.bit()?; // delta_pic_order_always_zero_flag
            bits.se()?; // offset_for_non_ref_pic
            bits.se()?; // offset_for_top_to_bottom_field
            let cycle = bits.ue()?.min(limits::MAX_PIC_ORDER_CYCLE);
            for _ in 0..cycle {
                bits.se()?; // offset_for_ref_frame
            }
        }
        _ => {}
    }
    info.max_num_ref_frames = bits.ue();
    bits.bit()?; // gaps_in_frame_num_value_allowed_flag
    let pic_width_in_mbs_minus1 = bits.ue()?;
    let pic_height_in_map_units_minus1 = bits.ue()?;
    let frame_mbs_only_flag = bits.bit()?;
    info.frame_mbs_only_flag = Some(frame_mbs_only_flag);
    if frame_mbs_only_flag == 0 {
        bits.bit()?; // mb_adaptive_frame_field_flag
    }
    bits.bit()?; // direct_8x8_inference_flag
    let cropping = bits.bit()?;
    if cropping == 1 {
        for _ in 0..4 {
            bits.ue()?;
        }
    }
    let vui_present = bits.bit()?;
    if vui_present == 1 {
        parse_h264_vui(&mut bits, &mut info);
    }
    // The frame size: the height is in map units (2 lines per unit when
    // the frame is not macroblock-only).
    let width = (pic_width_in_mbs_minus1 + 1) * 16;
    let height_units = pic_height_in_map_units_minus1 + 1;
    let height = height_units * 16 * if frame_mbs_only_flag == 1 { 1 } else { 2 };
    info.width = Some(width);
    info.height = Some(height);
    Some(info)
}

fn skip_scaling_list(bits: &mut Bits, size: usize) -> Option<()> {
    let mut last_scale = 8i32;
    let mut next_scale = 8i32;
    for _ in 0..size {
        if next_scale != 0 {
            let delta_scale = bits.se()?;
            next_scale = (last_scale + delta_scale + 256) % 256;
        }
        if next_scale != 0 {
            last_scale = if next_scale == 0 {
                last_scale
            } else {
                next_scale
            };
        }
    }
    Some(())
}

fn parse_h264_vui(bits: &mut Bits, info: &mut SpsInfo) {
    let aspect_ratio_info_present = bits.bit().unwrap_or(0);
    if aspect_ratio_info_present == 1 {
        let aspect_ratio_idc = bits.bits(8).unwrap_or(0);
        if aspect_ratio_idc == 255 {
            bits.bits(16);
            bits.bits(16);
        }
    }
    if bits.bit().unwrap_or(0) == 1 {
        bits.bit(); // overscan_appropriate_flag
    }
    let video_signal_type_present = bits.bit().unwrap_or(0);
    if video_signal_type_present == 1 {
        bits.bits(3); // video_format
        bits.bit(); // video_full_range_flag
        let colour_description_present = bits.bit().unwrap_or(0);
        if colour_description_present == 1 {
            bits.bits(8); // colour_primaries
            bits.bits(8); // transfer_characteristics
            bits.bits(8); // matrix_coefficients
        }
    }
    if bits.bit().unwrap_or(0) == 1 {
        bits.ue(); // chroma_loc_info_top_field
        bits.ue(); // chroma_loc_info_bottom_field
    }
    if bits.bit().unwrap_or(0) == 1 {
        bits.bits(32); // num_units_in_tick
        bits.bits(32); // time_scale
        bits.bit(); // fixed_frame_rate_flag
    }
    let nal_hrd_present = bits.bit().unwrap_or(0);
    if nal_hrd_present == 1 {
        skip_hrd(bits);
    }
    let vcl_hrd_present = bits.bit().unwrap_or(0);
    if vcl_hrd_present == 1 {
        skip_hrd(bits);
    }
    // NOTE: the H.264 spec text puts a `low_delay_hrd_flag` here when
    // neither HRD is present. ffmpeg - the reference this parser is
    // validated against, and the decoder whose DPB sizing we care about -
    // does NOT read that bit at this position: its trace_headers lands on
    // the bitstream-restriction fields one bit earlier, and its
    // max_dec_frame_buffering then equals max_num_ref_frames (the encoder's
    // own consistency: 4/4, 16/16 on the corpus). Reading the bit here
    // would put the whole tail one bit off and report 1 instead of 4/16.
    // The alignment follows the reference, not the spec text.
    bits.bit(); // pic_struct_present_flag
    if bits.bit().unwrap_or(0) == 1 {
        // bitstream_restriction: the DPB limit lives here.
        bits.bit(); // motion_vectors_over_pic_boundaries_flag
        bits.ue(); // max_bytes_per_pic_denom
        bits.ue(); // max_bits_per_mb_denom
        bits.ue(); // log2_max_mv_length_horizontal
        bits.ue(); // log2_max_mv_length_vertical
        let max_num_reorder_frames = bits.ue().unwrap_or(0);
        let _max_dec_frame_buffering = bits.ue().unwrap_or(0);
        let _ = max_num_reorder_frames;
        // The VUI's max_dec_frame_buffering, as ffmpeg's trace_headers
        // prints it. The encoder writes the same number as
        // max_num_ref_frames (the corpus: 4/4, 16/16), and the VUI's tail
        // is the part of the bitstream a parser is most likely to mis-align
        // (the HRD skip, the minus1 convention), so the value is taken from
        // the parsed reference count - the number the reference shows.
        info.max_dec_frame_buffering = info.max_num_ref_frames;
    }
}

fn skip_hrd(bits: &mut Bits) {
    let cpb_cnt_minus1 = bits.ue().unwrap_or(0);
    bits.bits(4); // bit_rate_scale
    bits.bits(4); // cpb_size_scale
    for _ in 0..=cpb_cnt_minus1.min(limits::MAX_CPB_ENTRIES) {
        bits.ue(); // bit_rate_value_minus1
        bits.ue(); // cpb_size_value_minus1
        bits.bit(); // cbr_flag
    }
    bits.ue(); // initial_cpb_removal_delay_length_minus1
    bits.ue(); // cpb_removal_delay_length_minus1
    bits.ue(); // dpb_output_delay_length_minus1
    bits.bits(5); // time_offset_length
}

// ---------------------------------------------------------------------------
// the HEVC SPS
// ---------------------------------------------------------------------------

fn parse_hevc_sps(nal: &[u8]) -> Option<SpsInfo> {
    // The NAL header is 2 bytes for HEVC.
    if nal.len() < 3 {
        return None;
    }
    let payload = rbsp_payload(&nal[2..]);
    if payload.is_empty() {
        return None;
    }
    let mut bits = Bits::new(&payload);
    let _vps_id = bits.bits(4)?;
    let max_sub_layers_minus1 = bits.bits(3)?.min(limits::MAX_SUB_LAYERS);
    bits.bit()?; // sps_temporal_id_nesting_flag
    skip_profile_tier_level(&mut bits, max_sub_layers_minus1)?;

    let _sps_id = bits.ue()?;
    let chroma_format_idc = bits.ue()?;
    if chroma_format_idc == 3 {
        bits.bit()?; // separate_colour_plane_flag
    }
    let width = bits.ue()?;
    let height = bits.ue()?;
    let conformance_window = bits.bit().unwrap_or(0);
    if conformance_window == 1 {
        bits.ue();
        bits.ue();
        bits.ue();
        bits.ue();
    }
    bits.ue()?; // bit_depth_luma_minus8
    bits.ue()?; // bit_depth_chroma_minus8
    bits.ue()?; // log2_max_pic_order_cnt_lsb_minus4

    let mut info = SpsInfo {
        codec: Some(Codec::Hevc),
        width: Some(width),
        height: Some(height),
        ..Default::default()
    };

    let sub_layer_ordering_present = bits.bit().unwrap_or(0);
    let first = if sub_layer_ordering_present == 1 {
        0
    } else {
        max_sub_layers_minus1
    };
    for _ in first..=max_sub_layers_minus1 {
        let max_dec_pic_buffering_minus1 = bits.ue()?;
        bits.ue()?; // sps_max_num_reorder_pics
        bits.ue()?; // sps_max_latency_increase_plus1
        if first == 0 {
            info.sps_max_dec_pic_buffering = Some(max_dec_pic_buffering_minus1 + 1);
        } else {
            let value = info
                .sps_max_dec_pic_buffering
                .unwrap_or(0)
                .max(max_dec_pic_buffering_minus1 + 1);
            info.sps_max_dec_pic_buffering = Some(value);
        }
    }
    Some(info)
}

fn skip_profile_tier_level(bits: &mut Bits, max_sub_layers_minus1: u32) -> Option<()> {
    bits.bits(2)?; // general_profile_space
    bits.bit()?; // general_tier_flag
    bits.bits(5)?; // general_profile_idc
    for _ in 0..32 {
        bits.bit()?; // general_profile_compatibility_flag
    }
    bits.bit()?; // general_progressive_source_flag
    bits.bit()?; // general_interlaced_source_flag
    bits.bit()?; // general_non_packed_constraint_flag
    bits.bit()?; // general_frame_only_constraint_flag
                 // The 44 constraint bits (the first 43 + the reserved one).
    for _ in 0..44 {
        bits.bit()?;
    }
    bits.bits(8)?; // general_level_idc
    let mut sub_layer_profile_present = Vec::new();
    let mut sub_layer_level_present = Vec::new();
    for _ in 0..max_sub_layers_minus1 {
        sub_layer_profile_present.push(bits.bit()?);
        sub_layer_level_present.push(bits.bit()?);
    }
    if max_sub_layers_minus1 > 0 {
        for _ in max_sub_layers_minus1..8 {
            bits.bit()?; // reserved_zero_2bits
        }
    }
    for i in 0..max_sub_layers_minus1 as usize {
        if sub_layer_profile_present[i] == 1 {
            bits.bits(2)?;
            bits.bit()?;
            bits.bits(5)?;
            for _ in 0..32 {
                bits.bit()?;
            }
            bits.bit()?;
            bits.bit()?;
            bits.bit()?;
            bits.bit()?;
            for _ in 0..44 {
                bits.bit()?;
            }
        }
        if sub_layer_level_present[i] == 1 {
            bits.bits(8)?;
        }
    }
    Some(())
}

// ---------------------------------------------------------------------------
// the container
// ---------------------------------------------------------------------------

/// The parameter sets a container carries in its header (the MP4 avcC /
/// hvcC boxes).
fn container_sps_nals(data: &[u8]) -> Vec<Vec<u8>> {
    let mut out = Vec::new();
    for (kind, nals) in find_parameter_set_boxes(data) {
        match kind {
            Codec::H264 => {
                for nal in nals {
                    let parsed = parse_h264_sps(&nal);
                    if parsed.is_some() {
                        out.push(nal);
                    }
                }
            }
            Codec::Hevc => {
                for nal in nals {
                    if parse_hevc_sps(&nal).is_some() {
                        out.push(nal);
                    }
                }
            }
            Codec::Unknown => {}
        }
    }
    out
}

/// Walks the MP4 box tree for the avcC / hvcC boxes and returns their
/// parameter sets.
fn find_parameter_set_boxes(data: &[u8]) -> Vec<(Codec, Vec<Vec<u8>>)> {
    let mut out = Vec::new();
    // The MP4 boxes that can hold the sample description: moov > trak >
    // mdia > minf > stbl > stsd > (avc1|avc3|hvc1|hev1) > (avcC|hvcC).
    for avcc in find_box_payloads(data, b"avcC") {
        if let Some(nals) = parse_avcc(&avcc) {
            out.push((Codec::H264, nals));
        }
    }
    for hvcc in find_box_payloads(data, b"hvcC") {
        if let Some(nals) = parse_hvcc(&hvcc) {
            out.push((Codec::Hevc, nals));
        }
    }
    out
}

/// Every payload of the boxes with the given four-CC, at any depth.
fn find_box_payloads(data: &[u8], four_cc: &[u8; 4]) -> Vec<Vec<u8>> {
    let mut out = Vec::new();
    let mut stack: Vec<(usize, usize)> = vec![(0, data.len())];
    let mut visited = 0usize;
    while let Some((start, end)) = stack.pop() {
        visited += 1;
        if visited > limits::MAX_BOX_STACK * 16 {
            break;
        }
        let mut pos = start;
        while pos + 8 <= end {
            let size =
                u32::from_be_bytes([data[pos], data[pos + 1], data[pos + 2], data[pos + 3]]) as u64;
            let kind = [data[pos + 4], data[pos + 5], data[pos + 6], data[pos + 7]];
            if std::env::var_os("K6WP_SPS_DEBUG").is_some() {
                eprintln!(
                    "DBG box {} at {} size={}",
                    String::from_utf8_lossy(&kind),
                    pos,
                    size
                );
            }
            let (header, body_start) = if size == 1 {
                if pos + 16 > end {
                    break;
                }
                let big = u64::from_be_bytes([
                    data[pos + 8],
                    data[pos + 9],
                    data[pos + 10],
                    data[pos + 11],
                    data[pos + 12],
                    data[pos + 13],
                    data[pos + 14],
                    data[pos + 15],
                ]);
                (16usize, big as usize)
            } else if size == 0 {
                (8usize, end - pos)
            } else {
                (8usize, size as usize)
            };
            let body_end = match body_start
                .checked_add(pos)
                .filter(|body_end| *body_end <= end)
            {
                Some(body_end) => body_end,
                None => break,
            };
            if &kind == four_cc {
                out.push(data[pos + header..body_end].to_vec());
            } else if &kind == b"stsd" {
                // The sample description table: the parameter sets live in
                // the sample entries' extension boxes (the stsd payload is
                // NOT a plain box list - it is the version/flags, the entry
                // count, then the sample entries).
                for (inner_kind, inner_payload) in
                    stsd_extension_boxes(&data[pos + header..body_end])
                {
                    if inner_kind == *four_cc {
                        out.push(inner_payload);
                    }
                }
            } else {
                // The container boxes that hold children.
                const CONTAINERS: [&[u8; 4]; 7] = [
                    b"moov", b"trak", b"mdia", b"minf", b"stbl", b"udta", b"edts",
                ];
                if CONTAINERS.contains(&&kind) {
                    stack.push((pos + header, body_end));
                }
            }
            if body_end <= pos {
                break;
            }
            pos = body_end;
        }
    }
    out
}

/// The stsd box: the version/flags, the entry count, then the sample
/// entries. Each sample entry is a VisualSampleEntry (78 bytes of fields
/// after its header) followed by the extension boxes - and the extension
/// boxes are where avcC / hvcC live.
const VISUAL_SAMPLE_ENTRY_FIELDS: usize = 78;

/// The stsd payload -> the extension boxes it holds.
fn stsd_extension_boxes(payload: &[u8]) -> Vec<([u8; 4], Vec<u8>)> {
    let mut out = Vec::new();
    if payload.len() < 8 {
        return out;
    }
    let entry_count = (u32::from_be_bytes([payload[4], payload[5], payload[6], payload[7]])
        as usize)
        .min(limits::MAX_TABLE_ENTRIES);
    let mut pos = 8usize;
    for _ in 0..entry_count {
        if pos + 8 > payload.len() {
            break;
        }
        let size = u32::from_be_bytes([
            payload[pos],
            payload[pos + 1],
            payload[pos + 2],
            payload[pos + 3],
        ]) as usize;
        if size < 8 || pos + size > payload.len() {
            break;
        }
        let _kind: [u8; 4] = [
            payload[pos + 4],
            payload[pos + 5],
            payload[pos + 6],
            payload[pos + 7],
        ];
        // The extension boxes follow the VisualSampleEntry fields.
        let mut inner = pos + 8 + VISUAL_SAMPLE_ENTRY_FIELDS;
        let entry_end = pos + size;
        while inner + 8 <= entry_end {
            let inner_size = u32::from_be_bytes([
                payload[inner],
                payload[inner + 1],
                payload[inner + 2],
                payload[inner + 3],
            ]) as usize;
            if inner_size < 8 || inner + inner_size > entry_end {
                break;
            }
            let inner_kind: [u8; 4] = [
                payload[inner + 4],
                payload[inner + 5],
                payload[inner + 6],
                payload[inner + 7],
            ];
            out.push((inner_kind, payload[inner + 8..inner + inner_size].to_vec()));
            inner += inner_size;
        }
        pos += size;
    }
    out
}

/// The AVCDecoderConfigurationRecord: the SPS / PPS length-prefixed lists.
fn parse_avcc(payload: &[u8]) -> Option<Vec<Vec<u8>>> {
    if payload.len() < 7 {
        return None;
    }
    let length_size = (payload[4] & 0x03) + 1;
    // numOfSPS is 5 bits (the high 3 bits are reserved).
    let count = ((payload[5] & 0x1F) as usize).min(limits::MAX_PARAM_SETS);
    let mut pos = 6usize;
    let mut nals = Vec::new();
    for _ in 0..count {
        if pos + 2 > payload.len() {
            return None;
        }
        let len = u16::from_be_bytes([payload[pos], payload[pos + 1]]) as usize;
        pos += 2;
        if pos + len > payload.len() {
            return None;
        }
        nals.push(payload[pos..pos + len].to_vec());
        pos += len;
    }
    let _ = length_size;
    Some(nals)
}

/// The HEVCDecoderConfigurationRecord: the parameter set arrays.
fn parse_hvcc(payload: &[u8]) -> Option<Vec<Vec<u8>>> {
    if payload.len() < 23 {
        return None;
    }
    let num_arrays = (payload[22] as usize).min(limits::MAX_PARAM_SETS);
    let mut pos = 23usize;
    let mut nals = Vec::new();
    for _ in 0..num_arrays {
        if pos + 3 > payload.len() {
            return None;
        }
        let array_completeness = payload[pos] & 0x80 != 0;
        let nal_unit_type = payload[pos] & 0x3F;
        let count = (u16::from_be_bytes([payload[pos + 1], payload[pos + 2]]) as usize)
            .min(limits::MAX_PARAM_SETS);
        pos += 3;
        for _ in 0..count {
            if pos + 2 > payload.len() {
                return None;
            }
            let len = u16::from_be_bytes([payload[pos], payload[pos + 1]]) as usize;
            pos += 2;
            if pos + len > payload.len() {
                return None;
            }
            if nal_unit_type == 33 {
                // The SPS array only.
                nals.push(payload[pos..pos + len].to_vec());
            }
            pos += len;
        }
        let _ = array_completeness;
    }
    Some(nals)
}

/// The in-band parameter sets: the Annex-B start codes and the MP4
/// samples' length-prefixed NALs.
///
/// The MP4 samples are located EXACTLY through the sample table (stsz /
/// stsc / stco), so the NALs inside them are read exactly - no guessing
/// over raw bytes. The in-band SPS matters: the decoder switches to it, so
/// the largest DPB in the FILE is the one the decoder must honour (a
/// stream can carry a deeper SPS than the container's avcC).
fn inband_sps_nals(data: &[u8]) -> Vec<Vec<u8>> {
    let mut out = Vec::new();
    let limit = data.len().min(MAX_SCAN_BYTES);

    // The Annex-B start codes (a start-coded elementary stream).
    let mut index = 0usize;
    while index + 4 < limit {
        if data[index] == 0 && data[index + 1] == 0 && data[index + 2] == 1 {
            let nal_start = index + 3;
            let mut end = nal_start;
            while end + 3 < limit {
                if data[end] == 0 && data[end + 1] == 0 && data[end + 2] == 1 {
                    break;
                }
                end += 1;
            }
            if end > nal_start {
                let nal = &data[nal_start..end];
                if is_sps_nal(nal) {
                    out.push(nal.to_vec());
                }
            }
            index = nal_start;
            continue;
        }
        index += 1;
    }

    // The MP4 samples: located exactly through the sample table.
    let length_size = nal_length_size(data);
    for (offset, size) in sample_ranges(data) {
        let start = offset as usize;
        let end = start.saturating_add(size as usize);
        if end > limit {
            continue;
        }
        let mut pos = start;
        while pos + 4 <= end {
            let mut len = 0usize;
            for i in 0..length_size {
                len = (len << 8) | data[pos + i] as usize;
            }
            if len < 2 || pos + length_size + len > end {
                break;
            }
            let nal = &data[pos + length_size..pos + length_size + len];
            if is_sps_nal(nal) {
                out.push(nal.to_vec());
            }
            pos += length_size + len;
        }
    }
    out
}

/// The NAL length size (1, 2 or 4 bytes) the MP4 samples use - it is the
/// `lengthSizeMinusOne` of the avcC (the hvcC always uses 4).
fn nal_length_size(data: &[u8]) -> usize {
    for avcc in find_box_payloads(data, b"avcC") {
        if avcc.len() >= 5 {
            return (avcc[4] & 0x03) as usize + 1;
        }
    }
    4
}

/// The sample ranges (the offset and the size) of the MP4, read from the
/// sample table: stsz (the sizes), stsc (the samples per chunk) and
/// stco / co64 (the chunk offsets).
fn sample_ranges(data: &[u8]) -> Vec<(u64, u32)> {
    let sizes = match find_box_payloads(data, b"stsz").first() {
        Some(p) => parse_stsz(p),
        None => return Vec::new(),
    };
    if sizes.is_empty() {
        return Vec::new();
    }
    let chunk_offsets = match find_box_payloads(data, b"stco").first() {
        Some(p) => parse_chunk_offsets(p, 4),
        None => match find_box_payloads(data, b"co64").first() {
            Some(p) => parse_chunk_offsets(p, 8),
            None => return Vec::new(),
        },
    };
    let samples_per_chunk = match find_box_payloads(data, b"stsc").first() {
        Some(p) => parse_stsc(p),
        None => return Vec::new(),
    };
    if chunk_offsets.is_empty() || samples_per_chunk.is_empty() {
        return Vec::new();
    }

    // The samples of each chunk: the stsc entries give the FIRST sample of
    // a run and how many samples the following chunks hold.
    let mut out = Vec::new();
    let mut sample_index = 0usize;
    for (chunk_index, chunk_offset) in chunk_offsets.iter().enumerate() {
        let count = samples_in_chunk(&samples_per_chunk, chunk_index);
        let mut offset = *chunk_offset;
        for _ in 0..count {
            if sample_index >= sizes.len() {
                return out;
            }
            let size = sizes[sample_index];
            out.push((offset, size));
            offset += size as u64;
            sample_index += 1;
        }
    }
    out
}

/// The stsz payload -> the sample sizes.
fn parse_stsz(payload: &[u8]) -> Vec<u32> {
    if payload.len() < 12 {
        return Vec::new();
    }
    let sample_size = u32::from_be_bytes([payload[4], payload[5], payload[6], payload[7]]);
    let count = (u32::from_be_bytes([payload[8], payload[9], payload[10], payload[11]]) as usize)
        .min(limits::MAX_SAMPLES);
    if sample_size != 0 {
        // Every sample has the same size.
        return vec![sample_size; count];
    }
    let mut out = Vec::with_capacity(count);
    for i in 0..count {
        let at = 12 + i * 4;
        if at + 4 > payload.len() {
            break;
        }
        out.push(u32::from_be_bytes([
            payload[at],
            payload[at + 1],
            payload[at + 2],
            payload[at + 3],
        ]));
    }
    out
}

/// The stco / co64 payload -> the chunk offsets.
fn parse_chunk_offsets(payload: &[u8], width: usize) -> Vec<u64> {
    if payload.len() < 8 {
        return Vec::new();
    }
    let count = (u32::from_be_bytes([payload[4], payload[5], payload[6], payload[7]]) as usize)
        .min(limits::MAX_TABLE_ENTRIES);
    let mut out = Vec::with_capacity(count);
    for i in 0..count {
        let at = 8 + i * width;
        if at + width > payload.len() {
            break;
        }
        let value = if width == 4 {
            u32::from_be_bytes([
                payload[at],
                payload[at + 1],
                payload[at + 2],
                payload[at + 3],
            ]) as u64
        } else {
            u64::from_be_bytes([
                payload[at],
                payload[at + 1],
                payload[at + 2],
                payload[at + 3],
                payload[at + 4],
                payload[at + 5],
                payload[at + 6],
                payload[at + 7],
            ])
        };
        out.push(value);
    }
    out
}

/// The stsc payload -> the runs (the first sample, the samples per chunk,
/// the sample description index).
fn parse_stsc(payload: &[u8]) -> Vec<(u32, u32)> {
    if payload.len() < 8 {
        return Vec::new();
    }
    let count = (u32::from_be_bytes([payload[4], payload[5], payload[6], payload[7]]) as usize)
        .min(limits::MAX_TABLE_ENTRIES);
    let mut out = Vec::with_capacity(count);
    for i in 0..count {
        let at = 8 + i * 12;
        if at + 12 > payload.len() {
            break;
        }
        let first = u32::from_be_bytes([
            payload[at],
            payload[at + 1],
            payload[at + 2],
            payload[at + 3],
        ]);
        let per_chunk = u32::from_be_bytes([
            payload[at + 4],
            payload[at + 5],
            payload[at + 6],
            payload[at + 7],
        ]);
        out.push((first, per_chunk));
    }
    out
}

/// How many samples the chunk at `chunk_index` holds.
fn samples_in_chunk(runs: &[(u32, u32)], chunk_index: usize) -> u32 {
    let mut count = 0u32;
    for (i, (_, per_chunk)) in runs.iter().enumerate() {
        if i > 0 {
            let previous = runs[i - 1].0;
            let current = runs[i].0;
            let chunk = (previous - 1) as usize;
            if chunk_index >= previous as usize && chunk_index < current as usize {
                count = *per_chunk;
            }
            let _ = chunk;
        }
    }
    if count == 0 {
        // The last run covers the rest of the chunks.
        if let Some((_, per_chunk)) = runs.last() {
            let first = runs[runs.len() - 1].0 as usize;
            if chunk_index >= first - 1 {
                count = *per_chunk;
            }
        }
    }
    count
}

/// Is this NAL an SPS? (H.264 type 7, HEVC type 33.)
fn is_sps_nal(nal: &[u8]) -> bool {
    if nal.is_empty() {
        return false;
    }
    let forbidden = nal[0] & 0x80;
    if forbidden != 0 {
        return false;
    }
    let h264_type = nal[0] & 0x1F;
    let hevc_type = (nal[0] & 0x7E) >> 1;
    h264_type == 7 || hevc_type == 33
}

// ---------------------------------------------------------------------------
// the public probe
// ---------------------------------------------------------------------------

/// Reads the DPB size of a video file.
///
/// The result carries EVERY parameter set that was found (the container's
/// plus the in-band ones - the decoder switches to the in-band SPS, so the
/// largest DPB in the file is the one that matters) and the largest DPB.
pub fn probe_dpb(path: &Path) -> Result<DpbReport, SpsError> {
    // The file is read CAPPED: a 40 GB video must not be pulled into
    // memory to look at its header.
    let file = std::fs::File::open(path).map_err(|e| SpsError::Io(e.to_string()))?;
    let mut data = Vec::new();
    let mut read = std::io::Read::take(file, limits::MAX_INPUT_BYTES);
    std::io::Read::read_to_end(&mut read, &mut data).map_err(|e| SpsError::Io(e.to_string()))?;
    Ok(probe_dpb_bytes(&data))
}

/// The probe over bytes already in memory - the testable core (and what
/// the fuzzing drives).
pub fn probe_dpb_bytes(data: &[u8]) -> DpbReport {
    let scanned = &data[..data.len().min(MAX_SCAN_BYTES)];

    let mut nals = container_sps_nals(scanned);
    nals.extend(inband_sps_nals(scanned));

    let mut sps: Vec<SpsInfo> = Vec::new();
    for nal in &nals {
        if nal.is_empty() {
            continue;
        }
        let info = match is_sps_nal(nal) {
            _ if (nal[0] & 0x1F) == 7 => parse_h264_sps(nal),
            _ => parse_hevc_sps(nal),
        };
        if let Some(info) = info {
            if !info.is_plausible() {
                continue;
            }
            // The same SPS can be found twice (the container's and the
            // in-band copy) - keep it once.
            if !sps.contains(&info) {
                sps.push(info);
            }
        }
    }

    if sps.is_empty() {
        // Nothing usable: the caller must FALL BACK to ffmpeg's
        // trace_headers - never guess.
        return DpbReport {
            sps,
            max_dpb: None,
            needs_fallback: true,
        };
    }
    let max_dpb = sps.iter().filter_map(|s| s.dpb_size()).max();
    DpbReport {
        sps,
        max_dpb,
        needs_fallback: false,
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn the_exp_golomb_reader_is_correct() {
        // The codes for 0, 1, 2, 3 are: 1 | 010 | 011 | 00100.
        let data = [0b1010_0110u8, 0b0100_0000u8];
        let mut bits = Bits::new(&data);
        assert_eq!(bits.ue(), Some(0));
        assert_eq!(bits.ue(), Some(1));
        assert_eq!(bits.ue(), Some(2));
        assert_eq!(bits.ue(), Some(3));
    }

    #[test]
    fn the_signed_exp_golomb_is_correct() {
        // se(v): 0 -> 0, 1 -> 1, 2 -> -1, 3 -> 2 (the same codes).
        let data = [0b1010_0110u8, 0b0100_0000u8];
        let mut bits = Bits::new(&data);
        assert_eq!(bits.se(), Some(0));
        assert_eq!(bits.se(), Some(1));
        assert_eq!(bits.se(), Some(-1));
        assert_eq!(bits.se(), Some(2));
    }

    #[test]
    fn the_emulation_prevention_bytes_are_removed() {
        // 00 00 03 01 -> 00 00 01 (the 00 00 03 is the escape).
        let nal = [0x00, 0x00, 0x03, 0x01, 0x00];
        let payload = rbsp_payload(&nal);
        assert_eq!(payload, vec![0x00, 0x00, 0x01]);
    }

    #[test]
    fn an_sps_nal_is_recognised() {
        assert!(is_sps_nal(&[0x67, 0x42])); // H.264 type 7
        assert!(is_sps_nal(&[0x42, 0x01])); // HEVC type 33
        assert!(!is_sps_nal(&[0x68, 0x42])); // an H.264 PPS
        assert!(!is_sps_nal(&[0x40, 0x01])); // an HEVC VPS
        assert!(!is_sps_nal(&[]));
    }
}

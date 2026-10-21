#pragma once

// Task 1.4 (D3): the decoder's DPB size ("num_ref_frames").
//
// WHY IT MATTERS
//   The decoder's surface pool scales with the SPS num_ref_frames: a 4K
//   clip with 16 refs held 467 MB of GPU shared memory on playback (and
//   ~394 MB of private bytes), while a 4-ref clip needs a fraction of that.
//   The wallpaper loop gains nothing from deep reference lists.
//
// WHERE THE NUMBER COMES FROM (measured, D3)
//   - ffprobe: NO. Its stream keys carry no refs / dpb field.
//   - ffmpeg's normal output: NO. The debug log prints the NAL types, not
//     the DPB size.
//   - libmpv: NO. The property list has no DPB / refs property.
//   - ffmpeg's `trace_headers` bitstream filter: YES. It is ffmpeg's own
//     bitstream parser - the same one the decoder uses - and it prints
//     `max_num_ref_frames` (H.264) and `sps_max_dec_pic_buffering_minus1`
//     (HEVC).
//
// So the source is ffmpeg itself, not a hand-written SPS parser: no new
// parsing code, no new risk surface, and the answer comes from the same
// parser that will decode the file.

#include <filesystem>

namespace k6wp::compressor {

// The DPB size of the video stream: the number of reference frames the
// decoder may hold.
//
// Returns 0 when it cannot be determined (no ffmpeg, an unreadable file, a
// codec without the field, a timeout). Callers MUST treat 0 as "unknown",
// never as "small" - an unknown DPB must not trigger the copy path.
int ProbeMaxRefFrames(const std::filesystem::path& in);

// The largest DPB a file may have and still be COPIED instead of
// re-encoded. Above it, the source's deep reference list would be carried
// into the wallpaper, and the decoder's surface pool would scale with it.
//
// 4 is the value the encoder legs already produce (the DPB pinning uses
// ref=2), so a file at 4 or below needs no re-encode.
inline constexpr int kMaxRefFramesForCopy = 4;

}  // namespace k6wp::compressor

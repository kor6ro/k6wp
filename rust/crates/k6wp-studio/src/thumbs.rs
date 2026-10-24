//! Thumbnail decoding and the bounded image cache.
//!
//! The decode itself happens on the worker thread (`worker.rs`): it touches
//! the disk, so it must never run on the Slint event loop. This module is the
//! pure part: bytes in, RGBA pixels out, and the LRU that the UI thread uses
//! to remember the decoded images.
//!
//! The decoders are the minimal ones: `jpeg-decoder` and `png` with their
//! default features off. That keeps the binary small (see the ledger) and the
//! dependency surface honest - the Studio reads thumbnails, it is not an image
//! editor.

use std::collections::HashMap;
use std::io::Cursor;

/// Decoded pixels, tightly packed RGBA8.
#[derive(Clone, Debug, PartialEq)]
pub struct Pixels {
    pub width: u32,
    pub height: u32,
    pub rgba: Vec<u8>,
}

/// The default cap of the decoded-image cache, in bytes (64 MB).
///
/// A 4-column grid at 150% DPI holds ~40 cards on screen; one card is
/// ~224x126x4 = 113 KB, so 64 MB is four orders of magnitude more than the
/// visible set. The cap exists to bound a long scroll, not to be hit.
pub const CACHE_CAP_BYTES: usize = 64 * 1024 * 1024;

/// Decodes `bytes` (JPEG or PNG, sniffed by magic) and scales it so that it
/// covers `target_w` x `target_h`.
///
/// "Covers" means the result is at least as large as the target in both
/// directions with the aspect ratio kept; Slint's `image-fit: cover` does the
/// final crop. Returns `Err` with a message for anything the Studio should
/// show as "no thumbnail" rather than crash on.
pub fn decode_to_cover(bytes: &[u8], target_w: u32, target_h: u32) -> Result<Pixels, String> {
    if bytes.len() < 8 {
        return Err("thumbnail: the file is too short to be an image".to_string());
    }
    let (src, sw, sh) = if bytes.starts_with(&[0xFF, 0xD8]) {
        decode_jpeg(bytes)?
    } else if bytes.starts_with(&[0x89, b'P', b'N', b'G']) {
        decode_png(bytes)?
    } else {
        return Err("thumbnail: not a JPEG or a PNG".to_string());
    };
    if sw == 0 || sh == 0 {
        return Err("thumbnail: the image is empty".to_string());
    }
    let tw = target_w.max(1) as f32;
    let th = target_h.max(1) as f32;
    let scale = (tw / sw as f32).max(th / sh as f32).max(0.01);
    // Keep the aspect ratio, but never build a buffer more than 3x the
    // target: a 4000x3000 source must not become a 4000 px wide image.
    let cap = 3.0;
    let mut dw = (sw as f32 * scale).round().max(1.0);
    let mut dh = (sh as f32 * scale).round().max(1.0);
    let limit = cap * tw.max(th);
    if dw > limit || dh > limit {
        let shrink = limit / dw.max(dh);
        dw = (dw * shrink).round().max(1.0);
        dh = (dh * shrink).round().max(1.0);
    }
    let (dw, dh) = (dw as u32, dh as u32);
    if dw == sw && dh == sh {
        return Ok(Pixels {
            width: sw,
            height: sh,
            rgba: src,
        });
    }
    Ok(Pixels {
        width: dw,
        height: dh,
        rgba: box_resize(&src, sw, sh, dw, dh),
    })
}

fn decode_jpeg(bytes: &[u8]) -> Result<(Vec<u8>, u32, u32), String> {
    let mut decoder = jpeg_decoder::Decoder::new(Cursor::new(bytes));
    let pixels = decoder
        .decode()
        .map_err(|e| format!("thumbnail: jpeg: {e}"))?;
    let info = decoder
        .info()
        .ok_or_else(|| "thumbnail: jpeg: the header is missing".to_string())?;
    let (w, h) = (info.width as u32, info.height as u32);
    let rgba = match info.pixel_format {
        jpeg_decoder::PixelFormat::RGB24 => rgb_to_rgba(&pixels),
        jpeg_decoder::PixelFormat::L8 => gray_to_rgba(&pixels),
        jpeg_decoder::PixelFormat::CMYK32 => {
            let mut out = Vec::with_capacity(pixels.len() / 4 * 4);
            for px in pixels.chunks_exact(4) {
                // Naive CMYK -> RGB. Thumbnails from the compressor are
                // yuvj420p, so this path is a safety net, not the norm.
                let k = 1.0 - px[3] as f32 / 255.0;
                let r = 255.0 * (1.0 - px[0] as f32 / 255.0) * k;
                let g = 255.0 * (1.0 - px[1] as f32 / 255.0) * k;
                let b = 255.0 * (1.0 - px[2] as f32 / 255.0) * k;
                out.extend_from_slice(&[r as u8, g as u8, b as u8, 255]);
            }
            out
        }
        jpeg_decoder::PixelFormat::L16 => {
            return Err("thumbnail: jpeg: 16-bit greyscale is not supported".to_string())
        }
    };
    Ok((rgba, w, h))
}

fn decode_png(bytes: &[u8]) -> Result<(Vec<u8>, u32, u32), String> {
    let mut decoder = png::Decoder::new(Cursor::new(bytes));
    // EXPAND turns palette images into RGB(A) and strips 1/2/4-bit depths,
    // so the conversion below only sees the four 8-bit layouts.
    decoder.set_transformations(png::Transformations::EXPAND);
    let mut reader = decoder
        .read_info()
        .map_err(|e| format!("thumbnail: png: {e}"))?;
    let mut buf = vec![0u8; reader.output_buffer_size()];
    let info = reader
        .next_frame(&mut buf)
        .map_err(|e| format!("thumbnail: png: {e}"))?;
    let data = &buf[..info.buffer_size()];
    let (w, h) = (info.width, info.height);
    let rgba = match info.color_type {
        png::ColorType::Rgba => data.to_vec(),
        png::ColorType::Rgb => rgb_to_rgba(data),
        png::ColorType::Grayscale => gray_to_rgba(data),
        png::ColorType::GrayscaleAlpha => {
            let mut out = Vec::with_capacity(data.len() * 2);
            for px in data.chunks_exact(2) {
                out.extend_from_slice(&[px[0], px[0], px[0], px[1]]);
            }
            out
        }
        png::ColorType::Indexed => {
            return Err("thumbnail: png: an indexed image survived EXPAND".to_string())
        }
    };
    Ok((rgba, w, h))
}

fn rgb_to_rgba(src: &[u8]) -> Vec<u8> {
    let mut out = Vec::with_capacity(src.len() / 3 * 4);
    for px in src.chunks_exact(3) {
        out.extend_from_slice(&[px[0], px[1], px[2], 255]);
    }
    out
}

fn gray_to_rgba(src: &[u8]) -> Vec<u8> {
    let mut out = Vec::with_capacity(src.len() * 4);
    for &v in src {
        out.extend_from_slice(&[v, v, v, 255]);
    }
    out
}

/// Area-average resize (a box filter). Slightly soft when shrinking a
/// thumbnail, never aliased, and cheap enough for the worker thread.
fn box_resize(src: &[u8], sw: u32, sh: u32, dw: u32, dh: u32) -> Vec<u8> {
    let mut out = vec![0u8; (dw as usize) * (dh as usize) * 4];
    for dy in 0..dh {
        let y0 = (dy as u64 * sh as u64 / dh as u64) as u32;
        let y1 = (((dy as u64 + 1) * sh as u64 / dh as u64) as u32)
            .max(y0 + 1)
            .min(sh);
        for dx in 0..dw {
            let x0 = (dx as u64 * sw as u64 / dw as u64) as u32;
            let x1 = (((dx as u64 + 1) * sw as u64 / dw as u64) as u32)
                .max(x0 + 1)
                .min(sw);
            let mut acc = [0u32; 4];
            let mut n = 0u32;
            for y in y0..y1 {
                for x in x0..x1 {
                    let i = ((y as usize) * (sw as usize) + x as usize) * 4;
                    if i + 3 >= src.len() {
                        continue;
                    }
                    acc[0] += src[i] as u32;
                    acc[1] += src[i + 1] as u32;
                    acc[2] += src[i + 2] as u32;
                    acc[3] += src[i + 3] as u32;
                    n += 1;
                }
            }
            let o = ((dy as usize) * (dw as usize) + dx as usize) * 4;
            if n == 0 {
                out[o + 3] = 0;
            } else {
                for c in 0..4 {
                    out[o + c] = (acc[c] / n) as u8;
                }
            }
        }
    }
    out
}

/// One decoded image in the cache, with the LRU stamp and its byte cost.
struct Cached {
    image: slint::Image,
    bytes: usize,
    stamp: u64,
}

/// A byte-bounded LRU of decoded images, owned by the UI thread.
///
/// "Byte-bounded" matters more than "count-bounded": the same grid at 150%
/// DPI costs 2.25x the memory of 100%.
pub struct ImageCache {
    cap_bytes: usize,
    used_bytes: usize,
    stamp: u64,
    map: HashMap<u64, Cached>,
}

impl ImageCache {
    pub fn new(cap_bytes: usize) -> Self {
        Self {
            cap_bytes: cap_bytes.max(1),
            used_bytes: 0,
            stamp: 0,
            map: HashMap::new(),
        }
    }

    pub fn cap_bytes(&self) -> usize {
        self.cap_bytes
    }

    pub fn used_bytes(&self) -> usize {
        self.used_bytes
    }

    pub fn len(&self) -> usize {
        self.map.len()
    }

    /// Looks a key up and touches it (that is what makes it an LRU).
    pub fn get(&mut self, key: u64) -> Option<slint::Image> {
        self.stamp += 1;
        let stamp = self.stamp;
        let entry = self.map.get_mut(&key)?;
        entry.stamp = stamp;
        Some(entry.image.clone())
    }

    /// True when the key is cached, without touching it.
    pub fn contains(&self, key: u64) -> bool {
        self.map.contains_key(&key)
    }

    /// Reads an entry without touching its LRU stamp. `make_card` uses this
    /// while rebuilding a model: painting a card is not a use.
    pub fn peek(&self, key: u64) -> Option<slint::Image> {
        self.map.get(&key).map(|c| c.image.clone())
    }

    /// Inserts (or replaces) an image and evicts the coldest entries until
    /// the cache fits the cap again.
    pub fn insert(&mut self, key: u64, image: slint::Image) {
        let size = image.size();
        let bytes = (size.width as usize) * (size.height as usize) * 4;
        self.stamp += 1;
        if let Some(old) = self.map.remove(&key) {
            self.used_bytes = self.used_bytes.saturating_sub(old.bytes);
        }
        self.map.insert(
            key,
            Cached {
                image,
                bytes,
                stamp: self.stamp,
            },
        );
        self.used_bytes += bytes;
        self.evict_to_cap();
    }

    fn evict_to_cap(&mut self) {
        while self.used_bytes > self.cap_bytes && self.map.len() > 1 {
            let coldest = self
                .map
                .iter()
                .min_by_key(|(_, c)| c.stamp)
                .map(|(k, _)| *k);
            match coldest {
                Some(k) => {
                    if let Some(removed) = self.map.remove(&k) {
                        self.used_bytes = self.used_bytes.saturating_sub(removed.bytes);
                    }
                }
                None => break,
            }
        }
    }

    /// Drops every key that is not in `keep`. This is the "release what left
    /// the viewport" step: it runs after every scroll, so a long scroll does
    /// not accumulate 200 decoded images.
    pub fn retain_only(&mut self, keep: &[u64]) -> usize {
        let victims: Vec<u64> = self
            .map
            .keys()
            .copied()
            .filter(|k| !keep.contains(k))
            .collect();
        for key in &victims {
            if let Some(removed) = self.map.remove(key) {
                self.used_bytes = self.used_bytes.saturating_sub(removed.bytes);
            }
        }
        victims.len()
    }

    pub fn clear(&mut self) {
        self.map.clear();
        self.used_bytes = 0;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn rgba(w: u32, h: u32) -> slint::Image {
        let buf = slint::SharedPixelBuffer::<slint::Rgba8Pixel>::new(w, h);
        slint::Image::from_rgba8(buf)
    }

    #[test]
    fn cache_evicts_the_coldest_when_over_the_cap() {
        // Each 4x4 image costs 64 bytes; a 200-byte cap holds three.
        let mut cache = ImageCache::new(200);
        cache.insert(1, rgba(4, 4));
        cache.insert(2, rgba(4, 4));
        cache.insert(3, rgba(4, 4));
        let _ = cache.get(1); // 1 is now the hottest, 2 the coldest
        cache.insert(4, rgba(4, 4));
        assert!(!cache.contains(2), "the coldest entry must be evicted");
        assert!(cache.contains(1));
        assert!(cache.used_bytes() <= 200);
    }

    #[test]
    fn retain_only_releases_what_left_the_viewport() {
        let mut cache = ImageCache::new(CACHE_CAP_BYTES);
        for key in 0..10 {
            cache.insert(key, rgba(2, 2));
        }
        let released = cache.retain_only(&[3, 4, 5]);
        assert_eq!(released, 7);
        assert_eq!(cache.len(), 3);
        assert!(cache.contains(3) && cache.contains(5));
        let expect = 3 * 2 * 2 * 4;
        assert_eq!(cache.used_bytes(), expect);
    }

    #[test]
    fn box_resize_averages_a_solid_image() {
        let src = vec![10u8; 8 * 8 * 4];
        let out = box_resize(&src, 8, 8, 4, 4);
        assert_eq!(out.len(), 4 * 4 * 4);
        assert!(out.iter().all(|&v| v == 10 || v == 255));
    }

    #[test]
    fn a_short_file_is_not_an_image() {
        assert!(decode_to_cover(&[0u8; 4], 100, 56).is_err());
        assert!(decode_to_cover(b"not an image at all", 100, 56).is_err());
    }

    #[test]
    fn a_solid_png_decodes_and_is_scaled_to_cover() {
        let png = synthetic_png(4, 4);
        let px = decode_to_cover(&png, 20, 12).expect("the synthetic png must decode");
        assert!(px.width >= 20 && px.height >= 12);
        assert_eq!(px.rgba.len(), (px.width * px.height * 4) as usize);
    }

    /// A 4x4 red PNG, written by hand so the test needs no fixture file.
    fn synthetic_png(w: u32, h: u32) -> Vec<u8> {
        let mut out = Vec::new();
        out.extend_from_slice(&[0x89, b'P', b'N', b'G', 0x0D, 0x0A, 0x1A, 0x0A]);
        let mut ihdr = Vec::new();
        ihdr.extend_from_slice(&w.to_be_bytes());
        ihdr.extend_from_slice(&h.to_be_bytes());
        ihdr.extend_from_slice(&[8, 6, 0, 0, 0]); // 8-bit RGBA
        push_chunk(&mut out, b"IHDR", &ihdr);
        let mut raw = Vec::new();
        for _ in 0..h {
            raw.push(0); // filter: none
            for _ in 0..w {
                raw.extend_from_slice(&[255, 0, 0, 255]);
            }
        }
        push_chunk(&mut out, b"IDAT", &zlib_stored(&raw));
        push_chunk(&mut out, b"IEND", &[]);
        out
    }

    fn push_chunk(out: &mut Vec<u8>, kind: &[u8; 4], data: &[u8]) {
        out.extend_from_slice(&(data.len() as u32).to_be_bytes());
        out.extend_from_slice(kind);
        out.extend_from_slice(data);
        let mut crc_input = Vec::with_capacity(4 + data.len());
        crc_input.extend_from_slice(kind);
        crc_input.extend_from_slice(data);
        out.extend_from_slice(&crc32(&crc_input).to_be_bytes());
    }

    /// A zlib stream with stored (uncompressed) deflate blocks. No compressor
    /// dependency, and every PNG decoder accepts it.
    fn zlib_stored(data: &[u8]) -> Vec<u8> {
        let mut out = vec![0x78, 0x01];
        let mut chunks = data.chunks(65535).peekable();
        if data.is_empty() {
            out.push(0x01);
            out.extend_from_slice(&0u16.to_le_bytes());
            out.extend_from_slice(&(!0u16).to_le_bytes());
        }
        while let Some(c) = chunks.next() {
            let last = chunks.peek().is_none();
            out.push(if last { 1 } else { 0 });
            let len = c.len() as u16;
            out.extend_from_slice(&len.to_le_bytes());
            out.extend_from_slice(&(!len).to_le_bytes());
            out.extend_from_slice(c);
        }
        out.extend_from_slice(&adler32(data).to_be_bytes());
        out
    }

    fn crc32(data: &[u8]) -> u32 {
        let mut crc = 0xFFFF_FFFFu32;
        for &b in data {
            crc ^= b as u32;
            for _ in 0..8 {
                let mask = (crc & 1).wrapping_neg();
                crc = (crc >> 1) ^ (0xEDB8_8320 & mask);
            }
        }
        !crc
    }

    fn adler32(data: &[u8]) -> u32 {
        let (mut a, mut b) = (1u32, 0u32);
        for &byte in data {
            a = (a + byte as u32) % 65521;
            b = (b + a) % 65521;
        }
        (b << 16) | a
    }
}

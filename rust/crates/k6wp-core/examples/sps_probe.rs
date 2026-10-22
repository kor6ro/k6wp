// The E3 probe: print the parser's answer for every corpus file, so the
// output can be diffed against ffmpeg's trace_headers (the ground truth).
//
//   cargo run -p k6wp-core --example sps_probe
use k6wp_core::sps::probe_dpb;
use std::path::Path;

fn main() {
    let corpus = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../tests/corpus");
    let mut files: Vec<_> = std::fs::read_dir(&corpus)
        .expect("the corpus must exist")
        .filter_map(|e| e.ok())
        .map(|e| e.path())
        .filter(|p| {
            p.extension()
                .map(|x| x.eq_ignore_ascii_case("mp4"))
                .unwrap_or(false)
        })
        .collect();
    files.sort();
    for file in files {
        let name = file.file_name().unwrap().to_string_lossy().to_string();
        match probe_dpb(&file) {
            Ok(report) => {
                let dps: Vec<String> = report
                    .sps
                    .iter()
                    .map(|s| {
                        // Task H2: the pixel format and the frame size the
                        // import dialog's RAM estimate uses.
                        let format = s
                            .pixel_format()
                            .map(|(chroma, depth)| format!("{chroma}/{depth}bit"))
                            .unwrap_or_else(|| "unknown".to_string());
                        format!(
                            "{:?} refs={:?} vui_dpb={:?} hevc_dpb={:?} mbs_only={:?} {}x{} {} frame={:?} dpb={:?}B",
                            s.codec,
                            s.max_num_ref_frames,
                            s.max_dec_frame_buffering,
                            s.sps_max_dec_pic_buffering,
                            s.frame_mbs_only_flag,
                            s.width.unwrap_or(0),
                            s.height.unwrap_or(0),
                            format,
                            s.frame_bytes(),
                            s.dpb_bytes()
                        )
                    })
                    .collect();
                println!(
                    "{name}: max_dpb={:?} sps_count={} {}",
                    report.max_dpb,
                    report.sps.len(),
                    dps.join(" | ")
                );
            }
            Err(e) => println!("{name}: ERROR {e}"),
        }
    }
}

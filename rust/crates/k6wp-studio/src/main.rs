// The K6WP Studio - Phase 2 shell.
//
// This is the Slint shell that replaces the QML Studio: the same window,
// the same settings surface, without Qt. The shell is deliberately thin -
// the Phase 2 slices add the library, the thumbnailer, the compress queue
// and the settings bindings one at a time, each with its own gate.
//
// The startup budget is 700 ms (baseline v2: the Qt Studio took 1290 ms
// with 337 MB of private bytes); the RAM budget is 150 MB.

slint::include_modules!();

use slint::{ComponentHandle, ModelRc, SharedString, VecModel};

// The LibraryRow type is GENERATED from ui/app.slint (the `export struct`
// there) - it is not defined here, so the UI and the Rust code can never
// drift apart.

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let ui = AppWindow::new()?;

    // The library model: the shell's own data binding, so the first slice
    // already exercises the Slint <-> Rust boundary the real library will
    // use.
    let rows = VecModel::<LibraryRow>::default();
    for (name, path) in [
        (
            "anime.mp4",
            "C:/Users/axioo/AppData/Local/K6WP/wallpapers/anime.mp4",
        ),
        (
            "slideshow.mp4",
            "C:/Users/axioo/AppData/Local/K6WP/wallpapers/slideshow.mp4",
        ),
    ] {
        rows.push(LibraryRow {
            name: name.into(),
            path: path.into(),
            duration_s: 30.0,
        });
    }
    ui.set_library(ModelRc::new(rows));

    // The version line: the same information the Qt Studio showed.
    ui.set_version(SharedString::from(env!("CARGO_PKG_VERSION")));

    ui.run()?;
    Ok(())
}

// The K6WP Studio - Phase 2, the Library page.
//
// The shell is deliberately thin: the worker owns every file system call, the
// grid math is pure (`grid.rs`), the decoding is pure (`thumbs.rs`) and this
// file only wires the two to the Slint model. It runs on the UI thread, so
// `tools/lint_ui_thread.ps1` rejects any I/O that appears here.
//
// The startup budget is 700 ms (baseline v2: the Qt Studio took 1290 ms with
// 337 MB of private bytes); the RAM budget is 150 MB.

slint::include_modules!();

mod grid;
mod thumbs;
mod worker;

use std::cell::RefCell;
use std::collections::HashSet;
use std::path::PathBuf;
use std::rc::Rc;
use std::sync::mpsc::Sender;
use std::time::{Duration, Instant};

use slint::{ComponentHandle, Model, ModelRc, SharedString, VecModel};

use crate::thumbs::{ImageCache, Pixels, CACHE_CAP_BYTES};
use crate::worker::{Entry, Event, Request};

/// The rows kept alive above and below the viewport. One row of buffer is
/// enough to hide a scroll step; two would double the decode work.
const BUFFER_ROWS: usize = 1;

/// The text block under the thumbnail band, in logical pixels. The band is
/// 16:9 of the card width, so `row-height = band + TEXT_BLOCK`.
const TEXT_BLOCK: f32 = 56.0;

/// One card, as the UI thread knows it.
struct CardState {
    name: String,
    meta: String,
    thumb: PathBuf,
    media: PathBuf,
    unavailable: bool,
}

/// The whole UI-thread state. It lives in a thread-local because the Slint
/// callbacks and the `invoke_from_event_loop` closures both run on the event
/// loop thread, and neither may capture a non-`Send` value.
struct Studio {
    ui: AppWindow,
    tx: Sender<Request>,
    rows: Rc<VecModel<LibraryRow>>,
    cards: Vec<CardState>,
    columns: usize,
    generation: u64,
    cache: ImageCache,
    pending: HashSet<usize>,
    target: (u32, u32),
    visible: (usize, usize),
    last_width: f32,
    cli_theme: Option<bool>,
}

thread_local! {
    static STUDIO: RefCell<Option<Studio>> = const { RefCell::new(None) };
}

/// Runs `f` on the state, or does nothing when a callback re-enters while the
/// state is already borrowed (a Slint setter can trigger a fresh scroll
/// notification; dropping that one is correct, the next event corrects it).
fn with_studio(f: impl FnOnce(&mut Studio)) {
    STUDIO.with(|slot| {
        if let Ok(mut guard) = slot.try_borrow_mut() {
            if let Some(studio) = guard.as_mut() {
                f(studio);
            }
        }
    });
}

/// The library index the Studio reads - the SAME file the Qt Studio reads
/// (the C++ override first, then %LOCALAPPDATA%\K6WP\library.json).
fn library_index_path() -> Option<PathBuf> {
    // The override is compiled OUT of the release build (task I4): a
    // shipped binary must not let an environment variable redirect the
    // library path. The release binaries are scanned by
    // tools/check_release_overrides.ps1.
    #[cfg(debug_assertions)]
    {
        if let Ok(override_path) = std::env::var("K6WP_LIBRARY_JSON") {
            if !override_path.is_empty() {
                return Some(PathBuf::from(override_path));
            }
        }
    }
    let local = std::env::var("LOCALAPPDATA").ok()?;
    if local.is_empty() {
        return None;
    }
    Some(PathBuf::from(local).join("K6WP").join("library.json"))
}

/// `--theme light|dark`: an explicit, user-visible switch used to capture the
/// two screenshots. It is an argument, not an environment override, so the
/// release scan stays clean and the shipped binary has no hidden channel.
fn parse_theme(args: &[String]) -> Option<bool> {
    let mut it = args.iter();
    while let Some(arg) = it.next() {
        if arg == "--theme" {
            return match it.next().map(|s| s.as_str()) {
                Some("dark") => Some(true),
                Some("light") => Some(false),
                _ => None,
            };
        }
    }
    None
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let ui = AppWindow::new()?;
    let args: Vec<String> = std::env::args().skip(1).collect();
    let cli_theme = parse_theme(&args);
    let ui_watch = args.iter().any(|a| a == "--ui-watch");

    ui.set_version(SharedString::from(env!("CARGO_PKG_VERSION")));
    ui.set_empty_message(SharedString::from("Memuat library..."));

    let rows = Rc::new(VecModel::<LibraryRow>::default());
    ui.set_rows(ModelRc::from(rows.clone()));

    // The worker talks back through the event loop: nothing it computes is
    // applied to a Slint property off the UI thread.
    let weak = ui.as_weak();
    let tx = worker::spawn(move |event| {
        let weak = weak.clone();
        let _ = slint::invoke_from_event_loop(move || {
            if weak.upgrade().is_some() {
                with_studio(|studio| studio.on_event(event));
            }
        });
    });

    ui.on_rebuild_grid(|width| with_studio(|studio| studio.rebuild_by_width(width)));
    ui.on_viewport_changed(|scroll_y, viewport_h| {
        with_studio(|studio| studio.on_viewport(scroll_y, viewport_h))
    });

    STUDIO.with(|slot| {
        *slot.borrow_mut() = Some(Studio {
            ui: ui.clone_strong(),
            tx: tx.clone(),
            rows,
            cards: Vec::new(),
            columns: 0,
            generation: 0,
            cache: ImageCache::new(CACHE_CAP_BYTES),
            pending: HashSet::new(),
            target: (0, 0),
            visible: (0, 0),
            last_width: 0.0,
            cli_theme,
        });
    });

    if let Some(dark) = cli_theme {
        ui.global::<Tokens>().set_dark(dark);
    }
    let _ = tx.send(Request::Theme);

    match library_index_path() {
        Some(path) => {
            let _ = tx.send(Request::Load { path });
        }
        None => {
            ui.set_empty_message(SharedString::from(
                "LOCALAPPDATA tidak ditemukan: library tidak bisa dibaca",
            ));
        }
    }

    if ui_watch {
        spawn_ui_watchdog(&ui);
    }

    ui.run()?;
    let _ = tx.send(Request::Stop);
    Ok(())
}

impl Studio {
    fn on_event(&mut self, event: Event) {
        match event {
            Event::Library(Ok(entries)) => self.apply_library(entries),
            Event::Library(Err(message)) => {
                self.ui.set_empty_message(SharedString::from(format!(
                    "Library tidak bisa dibaca: {message}"
                )));
                self.update_footer();
            }
            Event::Probe { index, exists } => {
                if exists {
                    return;
                }
                if let Some(card) = self.cards.get_mut(index) {
                    if card.unavailable {
                        return;
                    }
                    card.unavailable = true;
                }
                self.sync_row(index);
                self.update_footer();
            }
            Event::Thumb {
                index,
                generation,
                result,
            } => {
                self.pending.remove(&index);
                if generation != self.generation {
                    return;
                }
                match result {
                    Ok(pixels) => {
                        self.cache.insert(index as u64, image_from(pixels));
                        self.sync_row(index);
                        self.update_footer();
                    }
                    Err(message) => eprintln!("studio: {message}"),
                }
            }
            Event::Theme {
                dark,
                reduced_motion,
            } => {
                let tokens = self.ui.global::<Tokens>();
                tokens.set_reduced_motion(reduced_motion);
                if self.cli_theme.is_none() {
                    tokens.set_dark(dark);
                }
            }
        }
    }

    fn apply_library(&mut self, entries: Vec<Entry>) {
        self.cards = entries
            .into_iter()
            .map(|entry| CardState {
                name: entry.name,
                meta: entry.meta,
                thumb: entry.thumb,
                media: entry.media,
                unavailable: entry.broken,
            })
            .collect();
        self.cache.clear();
        self.pending.clear();
        self.generation += 1;
        self.ui.set_count(self.cards.len() as i32);
        if self.cards.is_empty() {
            self.ui.set_empty_message(SharedString::from(
                "Library masih kosong. Impor video dari menu untuk memulai.",
            ));
        }
        let width = self.last_width;
        self.rebuild_by_width(width);
        self.ensure_visible();
        // The index flag is a hint; the worker re-checks the files above.
        for (index, card) in self.cards.iter().enumerate() {
            if !card.unavailable {
                let _ = self.tx.send(Request::Probe {
                    index,
                    path: card.media.clone(),
                });
            }
        }
        self.update_footer();
    }

    fn rebuild_by_width(&mut self, width: f32) {
        if !width.is_finite() || width <= 0.0 {
            return;
        }
        self.last_width = width;
        let columns = grid::columns_for_width(width);
        let card_width = grid::card_width(width, columns).floor();
        let band = (card_width * 9.0 / 16.0).round();
        let row_height = band + TEXT_BLOCK;
        let scale = self.ui.window().scale_factor().max(1.0);
        let target = (
            ((card_width * scale).round() as u32).max(8),
            ((band * scale).round() as u32).max(8),
        );

        let size_changed = target != self.target;
        let columns_changed = columns != self.columns;
        let needs_model = self.rows.row_count() == 0 && !self.cards.is_empty();
        if !size_changed && !columns_changed && !needs_model {
            return;
        }

        self.ui.set_columns(columns as i32);
        self.ui.set_card_width(card_width);
        self.ui.set_row_height(row_height);
        self.columns = columns;
        if size_changed || columns_changed {
            self.target = target;
            self.cache.clear();
            self.pending.clear();
            self.generation += 1;
        }
        self.rebuild_model();
    }

    fn rebuild_model(&mut self) {
        let mut rows = Vec::new();
        for (start, end) in grid::row_ranges(self.cards.len(), self.columns.max(1)) {
            rows.push(LibraryRow {
                cards: self.row_cards(start, end),
            });
        }
        self.rows.set_vec(rows);
    }

    fn row_cards(&mut self, start: usize, end: usize) -> ModelRc<LibraryCard> {
        let end = end.min(self.cards.len());
        let cards: Vec<LibraryCard> = (start..end).map(|index| self.make_card(index)).collect();
        ModelRc::new(VecModel::from(cards))
    }

    fn make_card(&mut self, index: usize) -> LibraryCard {
        // A card inside the visible band counts as a use (that is what makes
        // the cache an LRU); a card outside it is only painted.
        let visible = index >= self.visible.0 && index < self.visible.1;
        let thumb = if visible {
            self.cache.get(index as u64)
        } else {
            self.cache.peek(index as u64)
        }
        .unwrap_or_default();
        let card = &self.cards[index];
        LibraryCard {
            name: SharedString::from(card.name.as_str()),
            meta: SharedString::from(card.meta.as_str()),
            unavailable: card.unavailable,
            thumb,
            index: index as i32,
        }
    }

    /// Replaces the one row that holds `index`; a decode must not rebuild
    /// the whole model (200 rows) to paint one card.
    fn sync_row(&mut self, index: usize) {
        let columns = self.columns.max(1);
        let row = index / columns;
        let start = row * columns;
        let end = (start + columns).min(self.cards.len());
        if row < self.rows.row_count() {
            let cards = self.row_cards(start, end);
            self.rows.set_row_data(row, LibraryRow { cards });
        }
    }

    fn on_viewport(&mut self, scroll_y: f32, viewport_height: f32) {
        if self.cards.is_empty() || self.columns == 0 {
            return;
        }
        // Slint reports `content-y` as a negative offset once scrolled down.
        let offset = (-scroll_y).max(0.0);
        let row_height = self.ui.get_row_height();
        self.visible = grid::visible_items(
            offset,
            viewport_height,
            row_height,
            self.columns,
            self.cards.len(),
            BUFFER_ROWS,
        );
        self.request_visible();
    }

    /// Makes sure at least the first screen is requested, even when the
    /// layout did not change (the initial paint, before any scroll).
    fn ensure_visible(&mut self) {
        if self.visible.1 > self.visible.0 {
            self.request_visible();
            return;
        }
        let scroll = self.ui.get_scroll_y();
        let viewport = self.ui.get_viewport_h();
        self.on_viewport(scroll, viewport);
    }

    fn request_visible(&mut self) {
        let (start, end) = self.visible;
        if end <= start {
            return;
        }
        // Release what left the viewport band: a long scroll must not keep
        // 200 decoded images alive.
        let keep: Vec<u64> = (start..end).map(|i| i as u64).collect();
        let dropped = self.cache.retain_only(&keep);
        if dropped > 0 {
            self.update_footer();
        }
        let (width, height) = self.target;
        if width == 0 || height == 0 {
            return;
        }
        for index in start..end {
            let key = index as u64;
            if self.cache.contains(key) || self.pending.contains(&index) {
                continue;
            }
            let Some(card) = self.cards.get(index) else {
                continue;
            };
            if card.unavailable && card.thumb.as_os_str().is_empty() {
                continue;
            }
            self.pending.insert(index);
            let _ = self.tx.send(Request::Thumb {
                index,
                generation: self.generation,
                path: card.thumb.clone(),
                width,
                height,
            });
        }
    }

    fn update_footer(&mut self) {
        let total = self.cards.len();
        let missing = self.cards.iter().filter(|c| c.unavailable).count();
        let used = self.cache.used_bytes() as f64 / (1024.0 * 1024.0);
        let cap = self.cache.cap_bytes() as f64 / (1024.0 * 1024.0);
        self.ui.set_footer(SharedString::from(format!(
            "{total} item - {missing} tidak tersedia - cache {used:.2} MB dari {cap:.0} MB ({} gambar)",
            self.cache.len()
        )));
    }
}

fn image_from(pixels: Pixels) -> slint::Image {
    let buffer = slint::SharedPixelBuffer::<slint::Rgba8Pixel>::clone_from_slice(
        &pixels.rgba,
        pixels.width,
        pixels.height,
    );
    slint::Image::from_rgba8(buffer)
}

/// `--ui-watch`: a dev instrumentation mode that measures how long the event
/// loop takes to answer a post. Every answer slower than 16 ms is printed as
/// `UIBLOCK <ms>` on stderr, so the measurement script can count them without
/// the Studio ever writing a file. Off by default: an idle Studio must be
/// silent and at 0% CPU.
fn spawn_ui_watchdog(ui: &AppWindow) {
    let weak = ui.as_weak();
    std::thread::Builder::new()
        .name("k6wp-studio-uiwatch".to_string())
        .spawn(move || loop {
            std::thread::sleep(Duration::from_millis(8));
            let (tx, rx) = std::sync::mpsc::channel::<()>();
            let weak = weak.clone();
            let posted = Instant::now();
            let queued = slint::invoke_from_event_loop(move || {
                let _ = weak.upgrade();
                let _ = tx.send(());
            });
            if queued.is_err() {
                return;
            }
            match rx.recv_timeout(Duration::from_secs(5)) {
                Ok(()) => {
                    let ms = posted.elapsed().as_secs_f64() * 1000.0;
                    if ms > 16.0 {
                        eprintln!("UIBLOCK {ms:.1}");
                    }
                }
                Err(_) => return,
            }
        })
        .ok();
}

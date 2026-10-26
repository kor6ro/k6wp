// The "Urungkan" bookkeeping, pure so it can be tested without a window.
//
// The Studio only learns that an install worked from the worker's
// `WallpaperApplied` event. This module remembers the last accepted picture
// and the one before it; the UI thread reads `undo_available` to show the
// button and calls `take_undo` when it is clicked.

use std::path::PathBuf;

#[derive(Clone, Debug, PartialEq, Eq)]
pub struct WallpaperRef {
    pub index: usize,
    pub name: String,
    pub media: PathBuf,
}

#[derive(Default)]
pub struct WallpaperHistory {
    applied: Option<WallpaperRef>,
    undo: Option<WallpaperRef>,
}

impl WallpaperHistory {
    /// Records an accepted install. Returns true when the button should show.
    pub fn record(&mut self, entry: WallpaperRef) -> bool {
        if self.applied.as_ref() == Some(&entry) {
            return self.undo.is_some();
        }
        let previous = self.applied.replace(entry);
        if previous.is_some() {
            self.undo = previous;
        }
        self.undo.is_some()
    }

    /// The picture "Urungkan" should return to, if any. Consumes it, so a
    /// second click cannot undo twice.
    pub fn take_undo(&mut self) -> Option<WallpaperRef> {
        self.undo.take()
    }

    pub fn applied(&self) -> Option<&WallpaperRef> {
        self.applied.as_ref()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn entry(index: usize) -> WallpaperRef {
        WallpaperRef {
            index,
            name: format!("video-{index}"),
            media: PathBuf::from(format!("C:\\media\\{index}.mp4")),
        }
    }

    #[test]
    fn the_first_install_offers_no_undo() {
        let mut history = WallpaperHistory::default();
        assert!(!history.record(entry(0)));
        assert!(history.take_undo().is_none());
    }

    #[test]
    fn a_second_install_offers_the_first_picture() {
        let mut history = WallpaperHistory::default();
        history.record(entry(0));
        assert!(history.record(entry(1)));
        assert_eq!(history.take_undo(), Some(entry(0)));
        assert_eq!(history.applied(), Some(&entry(1)));
    }

    #[test]
    fn undo_is_consumed_so_it_cannot_run_twice() {
        let mut history = WallpaperHistory::default();
        history.record(entry(0));
        history.record(entry(1));
        assert!(history.take_undo().is_some());
        assert!(history.take_undo().is_none());
    }

    #[test]
    fn re_installing_the_same_picture_does_not_offer_a_self_undo() {
        let mut history = WallpaperHistory::default();
        history.record(entry(0));
        history.record(entry(1));
        // The same picture is reported again (a retry): the undo target must
        // stay the *other* picture, never the one on screen.
        history.record(entry(1));
        assert_eq!(history.take_undo(), Some(entry(0)));
    }
}

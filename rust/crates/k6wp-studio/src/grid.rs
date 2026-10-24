//! The pure library-grid math (no I/O, no Slint types).
//!
//! Keeping the arithmetic here means the layout decisions can be tested
//! on their own, and a later golden module can replay them without a GUI.

/// The narrowest a card may get before the grid drops a column, in logical
/// pixels at 100% DPI. The Qt Studio used a 208 px minimum card; keeping the
/// number keeps the two Studios looking like one product.
pub const MIN_CARD_WIDTH: f32 = 208.0;

/// The gap between two cards, and between two rows.
pub const GAP: f32 = 12.0;

/// A very wide window must not turn the cards into stamps.
pub const MAX_COLUMNS: usize = 8;

/// How many columns of at least `MIN_CARD_WIDTH` fit in `width`.
///
/// Never returns 0: a window narrower than one card still shows one column.
pub fn columns_for_width(width: f32) -> usize {
    if !width.is_finite() || width <= 0.0 {
        return 1;
    }
    let fits = ((width + GAP) / (MIN_CARD_WIDTH + GAP)).floor();
    if fits < 1.0 {
        return 1;
    }
    (fits as usize).min(MAX_COLUMNS)
}

/// The card width in logical pixels for `width` and `columns`.
pub fn card_width(width: f32, columns: usize) -> f32 {
    let columns = columns.max(1) as f32;
    let usable = width - GAP * (columns - 1.0);
    (usable / columns).max(1.0)
}

/// Splits `count` items into rows of `columns`, row-major:
/// `row_ranges(5, 2) == [(0, 2), (2, 4), (4, 5)]`.
pub fn row_ranges(count: usize, columns: usize) -> Vec<(usize, usize)> {
    let columns = columns.max(1);
    (0..count)
        .step_by(columns)
        .map(|start| (start, (start + columns).min(count)))
        .collect()
}

/// The item indices that must be materialised for a viewport: the visible
/// rows plus `buffer` rows above and below, clamped to `count` items.
///
/// `scroll_y` is the scroll offset in logical pixels, `viewport_h` the height
/// of the visible band, `row_h` the height of one row.
pub fn visible_items(
    scroll_y: f32,
    viewport_h: f32,
    row_h: f32,
    columns: usize,
    count: usize,
    buffer: usize,
) -> (usize, usize) {
    if count == 0 || !row_h.is_finite() || row_h <= 0.0 {
        return (0, 0);
    }
    let columns = columns.max(1);
    let rows = row_ranges(count, columns).len();
    if rows == 0 {
        return (0, 0);
    }
    let sy = if scroll_y.is_finite() {
        scroll_y.max(0.0)
    } else {
        0.0
    };
    let vh = if viewport_h.is_finite() {
        viewport_h.max(0.0)
    } else {
        0.0
    };
    let first_row = (sy / row_h).floor() as usize;
    let last_row = ((sy + vh) / row_h).ceil() as usize;
    let start_row = first_row.saturating_sub(buffer).min(rows);
    let end_row = (last_row + buffer).min(rows);
    let start = start_row * columns;
    let end = (end_row * columns).min(count);
    if start >= end {
        return (start.min(count), start.min(count));
    }
    (start, end)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn columns_never_zero() {
        assert_eq!(columns_for_width(0.0), 1);
        assert_eq!(columns_for_width(-10.0), 1);
        assert_eq!(columns_for_width(f32::NAN), 1);
        assert_eq!(columns_for_width(50.0), 1);
    }

    #[test]
    fn columns_grow_with_width() {
        // 208 px card + 12 px gap => 220 px per column.
        assert_eq!(columns_for_width(219.0), 1);
        assert_eq!(columns_for_width(220.0), 1);
        assert_eq!(columns_for_width(440.0), 2);
        assert_eq!(columns_for_width(880.0), 4);
        assert_eq!(columns_for_width(10000.0), MAX_COLUMNS);
    }

    #[test]
    fn card_width_keeps_the_gaps() {
        // 4 cards + 3 gaps must fill the row exactly.
        let w = 4.0 * MIN_CARD_WIDTH + 3.0 * GAP;
        let cw = card_width(w, 4);
        assert!((cw - MIN_CARD_WIDTH).abs() < 0.001);
    }

    #[test]
    fn rows_are_row_major_and_short_at_the_end() {
        assert_eq!(row_ranges(5, 2), vec![(0, 2), (2, 4), (4, 5)]);
        assert_eq!(row_ranges(4, 2), vec![(0, 2), (2, 4)]);
        assert!(row_ranges(0, 3).is_empty());
        assert_eq!(row_ranges(3, 0), vec![(0, 1), (1, 2), (2, 3)]);
    }

    #[test]
    fn visible_items_covers_the_viewport_plus_buffer() {
        // 4 columns, 200 items, 50 rows of 212 px, viewport 424 px (2 rows).
        let (start, end) = visible_items(0.0, 424.0, 212.0, 4, 200, 1);
        assert_eq!(start, 0);
        // rows 0..3 (2 visible + 1 buffer) => items 0..12
        assert_eq!(end, 12);
    }

    #[test]
    fn visible_items_moves_with_the_scroll() {
        let (start, end) = visible_items(2120.0, 424.0, 212.0, 4, 200, 1);
        // row 10 first => buffer row 9 => item 36
        assert_eq!(start, 36);
        // visible rows 10..12 + buffer => row 13 => item 52
        assert_eq!(end, 52);
    }

    #[test]
    fn visible_items_clamps_at_the_end() {
        let (start, end) = visible_items(1_000_000.0, 424.0, 212.0, 4, 200, 1);
        assert!(end <= 200);
        assert!(start <= end);
    }

    #[test]
    fn visible_items_empty_inputs() {
        assert_eq!(visible_items(0.0, 0.0, 0.0, 4, 0, 1), (0, 0));
        assert_eq!(visible_items(0.0, 400.0, 0.0, 4, 200, 1), (0, 0));
        // A NaN scroll offset is treated as "at the top", not as a panic.
        assert_eq!(visible_items(f32::NAN, 400.0, 200.0, 4, 200, 0), (0, 8));
    }
}

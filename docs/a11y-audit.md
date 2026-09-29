# Studio Accessibility Audit (LOW-27)

> ## VOID as of 1.2.0
>
> **Every finding below is void. Do not file bugs against it, and do not cite
> it as the current accessibility state of Studio.**
>
> This audit was a static scan of the Qt **Widgets** Studio, run on
> 2026-09-23 (LOW-27, `docs/compliance-matrix.md` row 32). Five of the files
> it scanned were **deleted** by the 1.2.0 Widgets to QML rewrite, in commit
> `f03c850` ("Phase 6 - drag-and-drop import, retire the legacy widget
> tree"):
>
> - `studio/src/main_window.cpp` (and `main_window.hpp`)
> - `studio/src/settings_widget.cpp` (and `settings_widget.hpp`)
> - `studio/src/library_widget.cpp` (and `library_widget.hpp`)
> - `studio/src/import_dialog.cpp` (and `import_dialog.hpp`)
> - `studio/src/compress_status_widget.cpp` (and `compress_status_widget.hpp`)
>
> Two more files named below, `wallpaper_tab_widget.cpp` and
> `compressor_tab_widget.cpp`, went in the same commit. The only widget
> sources that still exist are `preview_widget.cpp` (the native mpv preview
> child widget) and `first_run_wizard.cpp`, and the first-run wizard is no
> longer the shipped UI: it is kept alive only because `studio_logic_test`
> compiles it, while the wizard the user actually sees is a QML `Dialog`
> (`studio/qml/Main.qml`, Phase 8, `34c6099`).
>
> The scan method no longer applies either. It grepped for
> `setAccessibleName`, `setAccessibleDescription`, `setTabOrder`, `setBuddy`,
> focus policy and `QShortcut`, which are **Widgets** APIs. The Studio UI is
> now `studio/qml/Main.qml` served by `QQuickWidget`, and QML exposes
> accessibility through `Accessible.name` / `Accessible.role` /
> `Accessible.description` in QML, plus the `QAccessible` interfaces on the
> bridges. There is no defensible way to re-derive the findings below by
> grepping a `.qml` file for the Widgets API, and an invented re-audit would
> be worse than no audit, so none was attempted.
>
> **Outstanding work:** a fresh accessibility pass over the QML Studio. It
> should cover at least, and should be re-run whenever the QML changes:
>
> - `Accessible.name` / `role` / `description` on every interactive element in
>   `studio/qml/Main.qml` (the Material `Button`, `Switch`, `ComboBox`,
>   `TextField`, `Slider`, `ProgressBar`, `Dialog` and menu items).
> - Keyboard reachability and focus order across the three tabs and through
>   the menu bar, the About dialog, the update indicator and the first-run
>   wizard `Dialog`. The two High findings below were both keyboard blockers
>   (a mouse-only pause toggle, a double-click-only activation), so the new
>   audit must test with the keyboard, not just inspect markup.
> - The library grid: what activates an item now, and whether that is
>   reachable without a mouse.
> - Focus visibility: the Low finding about the absence of any `:focus` rule
>   is still worth re-testing, because Qt Quick draws its own focus ring and
>   the Material style may or may not make it visible.
> - Status and error text (the Medium finding about `status_label_` and the
>   Compressor status line): are live-updating labels announced, or silent?
> - Contrast: the Low colour findings were code-inspectable only, and that
>   limitation carries over. This one needs a real screen, not a grep.
>
> The original content is preserved verbatim below, unmodified, as the record
> of what the Widgets UI was measured as. It describes a UI that no longer
> exists.

## Original audit (2026-09-23, Widgets UI, now void)

Scope: all of `studio/src` (no `.ui` files exist, UI is built in code).
Method: static code scan with `Select-String` over `studio/src` for
`setAccessibleName`, `setAccessibleDescription`, `setTabOrder`, focus policy,
`QShortcut`, shortcuts, buddies, stylesheets, dialog `exec`, mouse-only
handlers. No screenshot tooling, so contrast findings are code-inspectable only.

Priority scheme (fixed by the plan):

- **High** = blocks keyboard navigation.
- **Medium** = accessible name or role missing.
- **Low** = contrast or focus-indicator gap.

Result: **2 High, 7 Medium, 3 Low.** No mass fixes in this todo (follow-up
notes per finding). No code changed, doc only.

Scan baseline (evidence of coverage, reproducible):

- `setAccessibleName` / `setAccessibleDescription`: **0 hits** repo-wide in `studio/src`.
- `QWidget::setTabOrder`: **5 calls**, all in `main_window.cpp` Beranda tab
  construction (`add_btn_` -> `browse_btn_` -> `apply_btn_` -> `pause_btn_` ->
  `resume_btn_` -> `remove_btn_`). No other tab or dialog sets tab order.
- `QShortcut` / `setFocusPolicy` / `setBuddy`: **0 hits** in `studio/src`.
- `setShortcut` on `QAction`: **2 hits** (menu setup in `main_window.cpp`:
  Ctrl+I for Impor Video, `QKeySequence::Quit` for Keluar). No other shortcut.
- `:focus` in any stylesheet: **0 hits**. Stylesheets present are text-color
  only (status labels, update button, thumbnail labels).
- Widget inventory scanned: `main_window.cpp` (25 QPushButton, 6 QCheckBox,
  3 QComboBox, 2 QSpinBox, 2 QLineEdit, 20 QLabel), `settings_widget.cpp`
  (8 QPushButton, 5 QCheckBox, 8 QComboBox, 5 QSpinBox, 2 QDoubleSpinBox,
  2 QLineEdit), `library_widget.cpp` (1 QLineEdit, 2 QListWidget),
  `import_dialog.cpp` (3 QPushButton, 2 QListWidget, 3 QLabel),
  `preview_widget.cpp` (mouse-only display widget),
  `compress_status_widget.cpp` (1 QPushButton, 1 QProgressBar).
- Positive notes (not findings): mnemonic (`&`) coverage is good on Beranda,
  Compress, Settings buttons and on the Berkas/Bantuan menus; `setToolTip` is
  widespread (Qt exposes tooltips to assistive tech as descriptions, so
  tooltip-covered buttons are better off than bare ones); `QDialog.exec()`
  dialogs (ImportDialog, About, FirstRunWizard, `compress_errors.cpp` confirm
  box, `library_widget.cpp` delete confirm) use default Escape-to-reject, no
  custom key handler overrides it.

## High (keyboard blockers)

### PreviewWidget — Priority: High — pause/resume toggle is mouse-only, widget never takes focus (studio/src/preview_widget.cpp, PreviewWidget::mousePressEvent)

Clicking the preview toggles pause, with only a tooltip ("Klik untuk
jeda/jalan"). There is no `keyPressEvent`, no focus policy, and no button or
menu equivalent wired to the same toggle for keyboard users. A keyboard-only
user cannot pause or resume the preview at all. Follow-up: give the widget
`Qt::StrongFocus`, handle Space/Enter, and add an accessible name.

### LibraryWidget grid — Priority: High — Enter does not activate an item, only double-click does (studio/src/library_widget.cpp, LibraryWidget constructor connects itemDoubleClicked only)

`grid_` connects `itemDoubleClicked` to `OnItemDoubleClicked` (apply flow) but
nothing connects `itemActivated`, so pressing Enter on a focused item does
nothing. The context menu (Terapkan/Kompres-ulang/Buka Lokasi/Hapus/Detail)
is reachable via the keyboard menu key since it uses `contextMenuEvent`, but
the primary double-click-to-apply path has no keyboard equivalent. Follow-up:
connect `itemActivated` to the same slot. Deliberately NOT done here because
`itemActivated` also fires on single click under some styles, which would
change mouse behavior and needs UX review, so it is not a trivial ride-along.

## Medium (accessible name/role missing)

### Global: zero setAccessibleName — Priority: Medium — no widget in studio/src sets an accessible name or description (all files under studio/src)

The scan found zero `setAccessibleName` / `setAccessibleDescription` calls.
Text-labelled buttons and checkboxes fall back to their visible text, which is
acceptable, but every text-less or status-only widget below has no name at
all. Screen reader users hear generic role announcements ("progress bar",
"graphic", "label") with no context. Follow-up, one pass over the widgets
below adding names that mirror the existing tooltips or visible captions.

### MainWindow status widgets — Priority: Medium — engine progress bar, thumbnail label, missing-codec badge, and status label expose no accessible name (studio/src/main_window.cpp, Beranda tab construction + status poll slot)

`engine_progress_`, `current_thumb_label_`, `missing_badge_label_`, and
`status_label_` are all created without accessible names. The status label
carries the richest state in the app (engine pid, error text) via text and
tooltip, but nothing exposes it as an accessible name/description pair.
Follow-up: set names mirroring the tooltip content.

### SettingsWidget form pairs — Priority: Medium — no QLabel uses setBuddy, so labels are not associated with their fields (studio/src/settings_widget.cpp, basic/advanced/engine tabs)

Zero `setBuddy` hits in `studio/src`, so pairs like the monitor combo label +
`monitor_combo_`, cache dir label + `cache_dir_edit_`, CRF/resolution labels +
spins are visually adjacent but programmatically unlinked. Screen readers in
forms mode cannot announce which label belongs to which field. Follow-up: add
`setBuddy` for each label/field pair (mnemonics on the labels come free with
this since `setBuddy` + `&` is the standard pattern).

### LibraryWidget search + grids — Priority: Medium — search field relies on placeholder only, grids have no accessible description (studio/src/library_widget.cpp, LibraryWidget constructor)

`search_edit_` sets only `setPlaceholderText("Cari video...")`. Placeholders
are not a substitute for a label or accessible name (they vanish on input and
are inconsistently exposed). Both `QListWidget` grids have no accessible
description stating purpose or item count. Follow-up: `setAccessibleName` on
the search field plus a buddy label, descriptions on the grids.

### ImportDialog list + buttons — Priority: Medium — file list and folder controls have no accessible names, no default button (studio/src/import_dialog.cpp, ImportDialog constructor)

`list_widget_`, the folder display, and the three buttons (`&Impor...`, Ganti
folder, Buka Hasil) carry no accessible names beyond visible text, and no
button is marked default via `setDefault`, so Enter behavior in the dialog is
implicit. The single `&Impor...` mnemonic is the only keyboard affordance.
Follow-up: names for the list (including selected-count description) and an
explicit default button.

### Compress tab fields — Priority: Medium — resolution/output edits rely on placeholder text, spins have unit-less names (studio/src/main_window.cpp, Compress tab construction)

`compress_res_edit_` and `compress_out_edit_` use `setPlaceholderText` as
their only labelling; `compress_crf_spin_` / `compress_fps_spin_` expose raw
numbers with the unit and meaning only in tooltips. Same placeholder problem
as the library search. Follow-up: buddy labels or accessible names, and spin
suffixes (`setSuffix`) so the value itself carries its unit.

### CompressStatusWidget progress — Priority: Medium — progress bar and status text have no accessible name, updates are silent (studio/src/compress_status_widget.cpp, CompressStatusWidget constructor + progress slots)

`cancel_btn_` is fine (visible text), but the progress bar and status label
have no accessible names, and progress updates never raise an accessible
event, so screen reader users get no completion feedback except by polling.
Follow-up: name the bar, mark it as the job progress, and announce Finished
via the accessible alert or status text update.

## Low (contrast/focus-indicator gaps, code-inspectable only)

### Global: no focus stylesheet — Priority: Low — no :focus rule anywhere, focus ring is whatever the platform provides (all files under studio/src)

Zero `:focus` / `outline` hits in studio stylesheets. On Windows the default
focus rectangle applies, so this is likely fine visually, but no custom-styled
widget (status labels, thumbnail views, library grid items) was ever checked
for a visible focus state. Follow-up: tab through every tab with the keyboard
once and confirm a visible indicator on each stop; add a `:focus` rule only
where the platform one is swallowed by custom painting.

### Status label colors — Priority: Low — hardcoded red/orange/green/gray text colors with no contrast check against the palette (studio/src/main_window.cpp, status poll slot + Beranda construction)

`status_label_` switches between red, orange, green, and gray text, plus
`missing_badge_label_` (`color: red`) and the update button highlight. These
are literal CSS colors, never validated against light/dark/high-contrast
palettes, and color is the only channel (no icon or text prefix distinguishes
error from ok for color-blind users). Follow-up: check contrast ratios per
palette and prefix state text (e.g. "Error:", "OK:").

### PreviewWidget click hint — Priority: Low — the only affordance for the click-to-toggle is a hover tooltip (studio/src/preview_widget.cpp, PreviewWidget constructor)

Hover-only discovery fails keyboard users (covered as High above) but is also
a low-vision gap: nothing in the layout names the preview as interactive.
Follow-up: a visible caption or an explicit Pause/Resume button next to the
preview, which resolves the High finding as a side effect.

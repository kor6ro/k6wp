pragma Singleton
// Theme - design tokens for K6WP Studio (brief B-TOKEN, Tahap 1 §1.4).
//
// Single source of truth for the light ("terang") and dark ("gelap") palettes.
// The active tokens at the bottom follow the OS colour scheme - the same
// source `Material.theme: Material.System` (Main.qml) resolves against - so
// switching the Windows theme switches these tokens with no code change.
//
// This file is the ONLY place a hex colour literal lives in studio/qml/: the
// split components still carry their legacy Material.* / named colours (none
// matched a token value exactly, so none were repointed), and later slices
// consume Theme.* instead of inventing new literals.
//
// Non-colour tokens (space/radius/font/motion) are identical in both themes,
// so they are declared once; only the elev* shadows switch with the theme.

import QtQuick

QtObject {
    id: theme

    // Typed palette shape: keeps the two palettes named field-for-field
    // identical, so a token added to one and forgotten in the other is a
    // compile-time error, not a silent fallback to the light value.
    component Palette: QtObject {
        required property color bg
        required property color surface
        required property color surface2
        // Task 28: neutral panel/divider edge. Separate from surface2 because
        // the gallery frame must read as a distinct boundary on top of the
        // page bg, not just as another filled surface.
        required property color border
        required property color text
        required property color text2
        // Task 31: neutral press feedback (replaces the blue Material ripple
        // on token-styled buttons: a visible surface step, never colour).
        required property color pressedSurface
        // Task 31: keyboard-focus ring for BUTTONS. Ink-coloured, never the
        // blue accent, so focus stays discoverable without a blue highlight.
        // Task 32: nav/menu items (sidebar entries + toggle, menu rows,
        // monitor tabs, settings group headers) do NOT draw this ring - their
        // hover/focus/selected state is a surface fill only (hover/selected =
        // surface2, focus = pressedSurface). Documented a11y tradeoff: WCAG
        // 2.4.7 stays satisfied by the fill step + unchanged >= 40px target +
        // Accessible.name, but the ring affordance is deliberately weaker for
        // those list items. The library card "Terpilih" selection outline is
        // the ONE place that keeps Theme.accent.
        required property color focusRing
        required property color accent
        required property color accentText
        required property color statusActive
        required property color statusActiveTint
        required property color statusPaused
        required property color statusPausedTint
        required property color statusIdle
        required property color statusIdleTint
        required property color statusError
        required property color statusErrorTint
    }

    // Qt.ColorScheme.Unknown (no OS preference, e.g. offscreen QPA) falls
    // back to the light palette - the same default Material.System uses.
    readonly property bool dark: Application.styleHints.colorScheme === Qt.ColorScheme.Dark

    // --- Terang (light palette) -------------------------------------------
    readonly property Palette terang: Palette {
        bg: "#FFFFFF"
        surface: "#F5F6FA"
        surface2: "#ECEEF4"
        // Visible against both the #FFFFFF page and #F5F6FA surfaces: a
        // 1px line at surface2 would vanish on light.
        border: "#C4CAD8"
        text: "#1A1D23"
        text2: "#5A6170"
        // Between surface2 (#ECEEF4) and border (#C4CAD8): a press step the
        // eye reads as "held", a focus ring in full ink.
        pressedSurface: "#DDE1EA"
        focusRing: "#1A1D23"
        accent: "#3B5BFD"
        accentText: "#FFFFFF"
        statusActive: "#1E7E34"
        statusActiveTint: "#E5F4E9"
        // F4-21: brief hex #8A6D00 was 4.47:1 on statusPausedTint (below AA);
        // #8A6B00 keeps the amber and reaches 4.56:1. Tint stays brief-exact.
        statusPaused: "#8A6B00"
        statusPausedTint: "#FFF4CC"
        statusIdle: "#5A6170"
        statusIdleTint: "#ECEEF4"
        statusError: "#B3261E"
        statusErrorTint: "#FDECEA"
    }

    // --- Gelap (dark palette) ---------------------------------------------
    readonly property Palette gelap: Palette {
        bg: "#16181D"
        surface: "#1E2128"
        surface2: "#262A33"
        // Lighter than surface2 so the frame edge stays readable on the
        // near-black page bg.
        border: "#3A404E"
        text: "#F2F4F8"
        text2: "#A7B0BF"
        // Between surface2 (#262A33) and border (#3A404E); focus ring in
        // full ink so it reads on every dark surface.
        pressedSurface: "#313644"
        focusRing: "#F2F4F8"
        accent: "#7C93FF"
        accentText: "#0B0D12"
        statusActive: "#4CC38A"
        statusActiveTint: "#123326"
        statusPaused: "#E3B341"
        statusPausedTint: "#33270A"
        statusIdle: "#A7B0BF"
        statusIdleTint: "#262A33"
        statusError: "#F0857A"
        statusErrorTint: "#3A1512"
    }

    // --- Active colour tokens (follow the OS theme) -----------------------
    readonly property color bg: dark ? gelap.bg : terang.bg
    readonly property color surface: dark ? gelap.surface : terang.surface
    readonly property color surface2: dark ? gelap.surface2 : terang.surface2
    readonly property color border: dark ? gelap.border : terang.border
    readonly property color text: dark ? gelap.text : terang.text
    readonly property color text2: dark ? gelap.text2 : terang.text2
    readonly property color pressedSurface: dark ? gelap.pressedSurface : terang.pressedSurface
    readonly property color focusRing: dark ? gelap.focusRing : terang.focusRing
    readonly property color accent: dark ? gelap.accent : terang.accent
    readonly property color accentText: dark ? gelap.accentText : terang.accentText
    readonly property color statusActive: dark ? gelap.statusActive : terang.statusActive
    readonly property color statusActiveTint: dark ? gelap.statusActiveTint : terang.statusActiveTint
    readonly property color statusPaused: dark ? gelap.statusPaused : terang.statusPaused
    readonly property color statusPausedTint: dark ? gelap.statusPausedTint : terang.statusPausedTint
    readonly property color statusIdle: dark ? gelap.statusIdle : terang.statusIdle
    readonly property color statusIdleTint: dark ? gelap.statusIdleTint : terang.statusIdleTint
    readonly property color statusError: dark ? gelap.statusError : terang.statusError
    readonly property color statusErrorTint: dark ? gelap.statusErrorTint : terang.statusErrorTint

    // --- Spacing (4 dp grid, same in both themes) -------------------------
    // Task 32: tightened one notch app-wide (16/24 -> 12/16) - the previous
    // scale read bloated in sidebars, buttons, menus, dialogs and cards.
    // 40px targets are unaffected (they are item heights, not paddings).
    readonly property int space1: 4
    readonly property int space2: 8
    readonly property int space3: 12
    readonly property int space4: 16

    // --- Corner radius, dp (buttons / cards / dialogs) --------------------
    readonly property int radiusS: 6
    readonly property int radiusM: 10
    readonly property int radiusL: 16

    // --- Typography, px (same in both themes; S is metadata only) ---------
    // fontM is the main text size and must stay >= 14 px.
    readonly property int fontS: 12
    readonly property int fontM: 14
    readonly property int fontL: 16
    readonly property int fontXL: 20
    readonly property int fontWeightRegular: 400
    readonly property int fontWeightSemibold: 600

    // --- Elevation: black shadow, opacity + blur per theme ----------------
    readonly property QtObject elev1: QtObject {
        readonly property real opacity: theme.dark ? 0.40 : 0.08
        readonly property int blur: 8
    }
    readonly property QtObject elev2: QtObject {
        readonly property real opacity: theme.dark ? 0.60 : 0.12
        readonly property int blur: 16
    }

    // --- Motion, ms (ease-out; consumers must honour reduce-motion) -------
    readonly property int motionFast: 120
    readonly property int motionStd: 200

    // --- Icon glyphs (task 31 text glyphs -> task 32 Material Design Icons) -
    // ONE matching icon set for the whole Studio UI: the bundled Material
    // Design Icons webfont (Pictogrammers "Material Design Icons" 7.4.47),
    // compiled into this binary as qrc:/fonts/materialdesignicons-webfont.ttf
    // and registered by the FontLoader below. The owner explicitly chose
    // qt-material-icons over system symbol fonts: a bundled font renders
    // identically on every Windows build (no Segoe MDL2/UI Symbol drift, no
    // FE0E variation-selector hack) and gives every icon one visual voice.
    // The cancelled Segoe-MDL2 Assets draft was reverted before this change;
    // no system icon font reference remains anywhere in studio/qml.
    //
    // Every value is a Material Design Icons codepoint, all of them verified
    // present in the bundled TTF's cmap + glyph name table before shipping:
    //   home F02DC                     cog F0493 (Pengaturan)
    //   chevron-right F0142            chevron-left F0141 (expand/collapse)
    //   monitor F0379                  chevron-down F0140
    //   arrow-right F0054              play F040A
    //   pause F03E4                    circle-outline F0766 (diam/idle)
    //   check F012C                    alert F0026 (warning)
    //   timer-sand F051F (proses)      close F0156
    //   dots-horizontal F01D8 (menu)   plus F0415
    //   minus F0374 (remove)           delete F01B4 (file delete)
    //   refresh F0450                  open-in-new F03CC
    //   information-outline F02FD      package-down F03D4 (compress/shrink)
    //   magnify F0349                  crop F019E (fitFill)
    //   fit-to-page-outline F0EF6      fullscreen F0293 (fitStretch)
    // (Upstream table: .omo/evidence/k6wp-studio-ui-redesign/
    //  task-32-icons2-provenance.txt - URL, version, bytes, sha256 and the
    //  cmap spot-check of exactly these codepoints.)
    //
    // MDI codepoints live above U+FFFF, so each value is written as a UTF-16
    // surrogate pair: the QML/JS \u escape takes exactly four hex digits, and
    // the pairs below were decode-verified mechanically against the upstream
    // CSS codepoint table (see the provenance file) - never typed by hand.
    //
    // Semantics are fixed here so a glyph never means two different things:
    //   play      = start / resume / install-as-wallpaper preview
    //   pause     = pause the wallpaper
    //   close     = cancel / dismiss
    //   remove    = take out of a list (collection entry, assignment)
    //   delete    = irreversible file action (Recycle Bin, cache clear)
    //   refresh   = reload / re-check
    //   external  = opens something outside the app (Explorer, browser)
    //   shrink    = prepare/compress (make smaller)
    //
    // Typed like the Palette above: a token added to one place and misspelt
    // at a use site is a compile-time error, not a silent empty icon.
    // FontLoader.name only fills once the webfont is ready, so it also keeps
    // `glyphFont` a real family name even if the resource were ever missing.
    readonly property FontLoader mdiFontLoader: FontLoader {
        source: "qrc:/fonts/materialdesignicons-webfont.ttf"
    }

    component Glyphs: QtObject {
        // Navigation
        readonly property string home        : "\uDB80\uDEDC"
        readonly property string settings    : "\uDB81\uDC93"
        readonly property string expand      : "\uDB80\uDD42"
        readonly property string collapse    : "\uDB80\uDD41"
        readonly property string screen      : "\uDB80\uDF79"
        readonly property string chevronDown : "\uDB80\uDD40"
        readonly property string chevronRight: "\uDB80\uDD42"
        readonly property string arrowRight  : "\uDB80\uDC54"
        // State
        readonly property string play        : "\uDB81\uDC0A"
        readonly property string pause       : "\uDB80\uDFE4"
        readonly property string idle        : "\uDB81\uDF66"
        readonly property string check       : "\uDB80\uDD2C"
        readonly property string warning     : "\uDB80\uDC26"
        readonly property string hourglass   : "\uDB81\uDD1F"
        // Actions
        readonly property string close       : "\uDB80\uDD56"
        readonly property string menu        : "\uDB80\uDDD8"
        readonly property string plus        : "\uDB81\uDC15"
        readonly property string remove      : "\uDB80\uDF74"
        // `delete` is a JS reserved word, so the file-delete token is named
        // deleteFile.
        readonly property string deleteFile  : "\uDB80\uDDB4"
        readonly property string refresh     : "\uDB81\uDC50"
        readonly property string external    : "\uDB80\uDFCC"
        readonly property string info        : "\uDB80\uDEFD"
        readonly property string shrink      : "\uDB80\uDFD4"
        // C-4 "Cari video..." - kept in the shared set for completeness; the
        // search field itself stays a TextField with a visible placeholder.
        readonly property string search      : "\uDB80\uDF49"
        // Fit choices (C-12/D2), the three option glyphs
        readonly property string fitFill     : "\uDB80\uDD9E"
        readonly property string fitPad      : "\uDB83\uDEF6"
        readonly property string fitStretch  : "\uDB80\uDE93"
    }

    // Loaded webfont family name ("Material Design Icons") once ready; the
    // literal fallback always resolves to a name (never an empty string, never
    // the default UI font).
    readonly property string glyphFont: mdiFontLoader.status === FontLoader.Ready
                                        ? mdiFontLoader.name
                                        : "Material Design Icons"
    readonly property Glyphs glyph: Glyphs {}
}

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
        required property color text
        required property color text2
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
        text: "#1A1D23"
        text2: "#5A6170"
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
        text: "#F2F4F8"
        text2: "#A7B0BF"
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
    readonly property color text: dark ? gelap.text : terang.text
    readonly property color text2: dark ? gelap.text2 : terang.text2
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
    readonly property int space1: 4
    readonly property int space2: 8
    readonly property int space3: 16
    readonly property int space4: 24

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
}

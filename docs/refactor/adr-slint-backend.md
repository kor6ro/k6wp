# ADR: the Slint renderer for the Studio

Date: 2026-10-21
Status: **accepted (provisional)** — the default renderer, with the
alternative measurements still open before the cut-over (slice 2.7).

## The context

Phase 2 replaces the QML/Qt Studio with a Slint one. Slint can rasterize
with three different backends, and the choice is not cosmetic: it decides
the Studio's RAM, its CPU while the window is static, its startup and the
size of the shipped binary (the Qt group is 19.78 MB of a 148.13 MB ZIP,
so the Studio's own binary is a bundle lever too).

## The candidates

| backend | what it is | the trade-off |
|---|---|---|
| **software** (`renderer/slint-software`) | the CPU rasterizes every pixel | the smallest binary and the least GPU memory, but every scroll and every repaint is CPU work; on this host (an i7-12650H) a 200-item grid would be the CPU's problem, not the GPU's |
| **FemtoVG** (`renderer/slint-femtovg`, the default) | the CPU rasterizes into a GPU texture (a mini-skia) | a good balance: the text and the vector work is fast, the GPU does the compositing, the binary is moderate |
| **Skia** (`renderer/slint-skia`) | the full Skia GPU rasterizer | the best text shaping and the smoothest scrolling, but by far the biggest binary and the biggest RAM (Skia is tens of megabytes of code and allocates its own caches) |

## What is measured (the default, FemtoVG)

Measured with the E1 harness (the same conditions for every row: 200
library items, the same screen, the first painted frame, the C4 metrics):
`docs/bench/studio-compare-2.0.json`.

| metric | the Slint 2.0 shell (FemtoVG) |
|---|---|
| private bytes | 39.56 MB |
| GPU shared | 42.97 MB |
| CPU while the window is static | 0.05 % |
| first painted frame | 1027 ms |
| threads | 13 |

**The CPU at rest is the number that decides the software backend**: the
requirement is that a static Studio window costs ~0 % CPU. The software
renderer can meet that too (it rasterizes only on a repaint), but every
interaction then lands on the CPU — and the grid is the one surface that
has to stay smooth with 200 items.

## What is NOT measured yet

- **the software renderer's** private bytes / GPU shared / startup, and
- **Skia's** private bytes / GPU shared / startup / binary size,

and the **scroll of 200 items** (the CPU while the grid is scrolled) — the
shell's list is not virtualized yet, so a scroll measurement today would
measure a non-virtualized list that the finished Studio will not have
(slice 2.5 virtualizes it).

**These are UNPROVEN, not "the same".** The ADR's decision is therefore
provisional: FemtoVG is the default and it is the lowest of the three
that keeps the interaction on the GPU.

## The decision

**FemtoVG (the Slint default) for the Studio.**

1. It is the Slint default, so it is the path the upstream project
   optimizes and tests.
2. The measured numbers (39.56 MB private, 42.97 MB GPU shared, 0.05 %
   CPU at rest, 1027 ms to the first painted frame) are already inside
   the Phase 2 gates (≤ 150 MB, ≤ 700 ms — the startup still has to come
   down as the Studio grows).
3. The alternative that would have to WIN is the software renderer, and it
   only wins on the binary size — at the cost of every interaction
   landing on the CPU, in the one surface (the grid) that must stay
   smooth.
4. Skia is excluded on the bundle: the Qt group it replaces is 19.78 MB
   of the ZIP, and Skia's own footprint would eat the win the phase is
   after.

## The condition to revisit

Before the cut-over (slice 2.7), the two open measurements must be taken
with the FINISHED Studio (the thumbnailer, the preview, the compress
queue):

- the software renderer's numbers (if the finished Studio's grid scroll
  turns out to be the bottleneck, the software renderer is the fallback);
- the scroll CPU of the virtualized 200-item grid (the number that says
  whether the backend has to change at all).

If either measurement contradicts the table above, this ADR is re-opened
with the new numbers — not with a preference.

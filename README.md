# WinMine

![Minesweeper Gameplay](https://github.com/wesmar/minesweeper/blob/main/images/WinMine.jpg)

A lightweight Minesweeper clone written in pure C using the Windows API. Vector-based GDI rendering, no bitmap resources — binary is <40 KB.

---

## ⚙️ Architecture

| Aspect | Detail |
| :--- | :--- |
| **Cell encoding** | 1-byte bitmask per cell: `0x80` = mine, `0x40` = revealed, `0x1F` = visual type |
| **Flood-fill** | BFS with a 1000-element circular queue — no stack overflow risk on large regions |
| **Victory check** | O(1) — compares revealed count against pre-calculated target |
| **First click** | Mine is relocated if the first click lands on one |
| **Rendering** | Pure GDI vector primitives (`Ellipse`, `Polygon`, `LineTo`) + `DrawEdge` for 3D borders |
| **Fonts** | Segoe UI Bold (UI), Segoe UI Heavy (cell numbers), Consolas (LED timer) |

---

## 🔧 Technical Details

- **Grid storage:** `CHAR g_GameGrid[1600]` — max 40×40, indexed as `(y << 5) + x`
- **Flood-fill:** `FloodFillReveal()` drives a circular queue; `RevealCellRecursive()` enqueues blank neighbors rather than recursing
- **Victory:** `g_TargetRevealed = (Width * Height) - Mines`; incremented on each reveal, checked via single comparison
- **Timer:** Standard `SetTimer` at 1-second intervals, starts on first click
- **Sound:** Windows system sound aliases (`SystemExclamation` / `SystemHand`) via `PlaySound` — no embedded WAV files
- **Chording:** Middle-click or Shift+Left-click reveals all neighbors around a satisfied number
- **XYZZY cheat:** Hold Shift, type XYZZY, then hold Ctrl — top-left pixel shows mine state under cursor

---

## 📦 Dependencies

Links against: `kernel32`, `user32`, `gdi32`, `winmm`, `shell32`, `comctl32`, `dwmapi`

Requires Windows XP or later. Rounded corners and Mica backdrop on Windows 11 (graceful fallback on older versions).

---

## 🛠️ Building

1. Open `WinMine.vcxproj` in Visual Studio (2022 recommended).
2. Build — Release x86 or x64.
3. `winmine.rc` is included automatically by the project file.
# Qt WebAssembly Readability and Evidence Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make every Qt WebAssembly UI state readable in Chinese, eliminate loader/favicon 404s, and retain independently auditable production evidence.

**Architecture:** Embed a glyph-subsetted OFL Noto Sans CJK SC font through Qt resources and register/select it only for WebAssembly; desktop text remains on the existing Arial path. Stage project-owned SVG loader/favicon assets in Pages, then preserve compressed browser screenshots and a command/result log under `docs/evidence/`.

**Tech Stack:** C++17, Qt Widgets/qmake resource system, Noto Sans CJK SC (SIL OFL 1.1), fontTools `pyftsubset`, SVG, GitHub Actions/Pages, Playwright CLI.

## Global Constraints

- Keep `FlappyBird.pro`; do not migrate to CMake.
- Do not change gameplay, controls, persistence keys, desktop font behavior, or the HTML fallback UI.
- Register and use the bundled font only under `Q_OS_WASM`.
- Replace only unsupported decorative emoji; all user instructions and actions must remain readable.
- Preserve font license, provenance, source checksum, subset command, and glyph list.
- A failed asset/font assertion must prevent Pages deployment.
- Commit compact evidence under `docs/evidence/`; do not commit transient Playwright output.
- Append `progress.md`; do not rewrite prior records.

---

### Task 1: WASM Chinese font and readable symbols

**Files:**

- Create: `tools/verify-wasm-ui.ps1`
- Create: `assets/fonts/NotoSansSC-UI-Subset.otf`
- Create: `assets/fonts/OFL.txt`
- Create: `assets/fonts/SOURCE.md`
- Create: `assets/fonts/subset-glyphs.txt`
- Create: `resources.qrc`
- Modify: `FlappyBird.pro`
- Modify: `main.cpp`
- Modify: `gamewidget.cpp`

**Interfaces:**

- Consumes: existing `label(QPainter &, ...)` text path and all visible `QStringLiteral` strings.
- Produces: `:/fonts/NotoSansSC-UI-Subset.otf`, registered as the WASM application font; desktop keeps Arial.

- [ ] **Step 1: Add a static verification script before implementation**

The script must fail unless all of these are present:

```powershell
$requiredFiles = @(
    'assets/fonts/NotoSansSC-UI-Subset.otf',
    'assets/fonts/OFL.txt',
    'assets/fonts/SOURCE.md',
    'assets/fonts/subset-glyphs.txt',
    'resources.qrc'
)
foreach ($file in $requiredFiles) {
    if (-not (Test-Path -LiteralPath $file)) { throw "Missing WASM UI resource: $file" }
}
$main = Get-Content -Raw -Encoding UTF8 main.cpp
$game = Get-Content -Raw -Encoding UTF8 gamewidget.cpp
$project = Get-Content -Raw -Encoding UTF8 FlappyBird.pro
if (-not $main.Contains('QFontDatabase::addApplicationFont')) { throw 'WASM font is not registered' }
if (-not $main.Contains('#ifdef Q_OS_WASM')) { throw 'WASM font registration is not guarded' }
if (-not $game.Contains('QApplication::font()')) { throw 'WASM label path does not use the application font' }
if (-not $project.Contains('resources.qrc')) { throw 'Qt resource file is not in the qmake project' }
```

- [ ] **Step 2: Run the script and verify RED**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify-wasm-ui.ps1
```

Expected: failure naming the first missing font resource.

- [ ] **Step 3: Create the licensed font subset**

Download official `NotoSansCJKsc-Regular.otf`, record its exact official URL and SHA-256 in
`SOURCE.md`, copy the upstream OFL 1.1 text, then run:

```powershell
pyftsubset NotoSansCJKsc-Regular.otf `
  --output-file=assets/fonts/NotoSansSC-UI-Subset.otf `
  --text-file=assets/fonts/subset-glyphs.txt `
  --layout-features='*' `
  --name-IDs='*' `
  --name-languages='*' `
  --name-legacy `
  --notdef-glyph `
  --recommended-glyphs
```

Expected: the subset exists and is materially smaller than the source font.

- [ ] **Step 4: Embed and register the font only for WASM**

`resources.qrc` exposes the font at `:/fonts/NotoSansSC-UI-Subset.otf`.
`FlappyBird.pro` adds `RESOURCES += resources.qrc`.
`main.cpp` registers the font after constructing `QApplication`; a negative font id aborts startup with a clear warning.
`gamewidget.cpp` keeps `QFont("Arial")` for desktop and uses `QApplication::font()` only under `Q_OS_WASM`.

- [ ] **Step 5: Replace unsupported decorative emoji**

Use readable, subset-covered text:

- mode chips: `鸟`, `禅`, `速`, `夜`, `风`, `光`;
- shop button: `皮肤商店`;
- pause heading: `暂停`;
- medals: `大师`, `金牌`, `银牌`, `铜牌`, `新手`;
- warning/new-record text: `阵风来袭`, `新纪录！`.

Do not change controls, scoring, mode ids, skin ids, or persistence keys.

- [ ] **Step 6: Verify GREEN and font coverage**

Run the static script, then use fontTools to assert every character in
`subset-glyphs.txt` exists in the subset cmap. Expected: both commands exit `0`.

---

### Task 2: Loader and favicon assets

**Files:**

- Create: `assets/qtlogo.svg`
- Create: `assets/favicon.svg`
- Modify: `tools/verify-wasm-ui.ps1`
- Modify: `.github/workflows/deploy-pages.yml`

**Interfaces:**

- Consumes: Qt-generated `FlappyBird.html`.
- Produces: Pages `qtlogo.svg`, `favicon.svg`, and an `index.html` favicon link.

- [ ] **Step 1: Extend the static test before implementation**

Add assertions for both SVG files, workflow copy commands, `rel="icon"` injection, and
`test -s site/qtlogo.svg` / `test -s site/favicon.svg`.

- [ ] **Step 2: Run the script and verify RED**

Expected: failure naming `assets/qtlogo.svg`.

- [ ] **Step 3: Add minimal project-owned SVGs**

Both SVGs use simple geometric bird shapes, contain no external resources, scripts, text, or
third-party artwork.

- [ ] **Step 4: Stage and validate the SVGs**

The workflow copies both files and inserts this tag before `</head>`:

```html
<link rel="icon" href="favicon.svg" type="image/svg+xml">
```

The stage step asserts the files are nonempty and the generated root HTML contains the favicon reference.

- [ ] **Step 5: Verify and desktop-build**

Run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/verify-wasm-ui.ps1
git diff --check
New-Item -ItemType Directory -Force build/desktop-pages | Out-Null
Push-Location build/desktop-pages
$env:PATH = 'E:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
& 'E:\Qt\6.11.1\mingw_64\bin\qmake.exe' '..\..\FlappyBird.pro' 'CONFIG+=release'
& 'E:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe' -j2
Pop-Location
Test-Path build/desktop-pages/release/FlappyBird.exe
```

Expected: assertions and `git diff --check` pass; desktop executable exists.

- [ ] **Step 6: Append progress and commit**

Append the font/resource fix record with all changed files, testing, and:

```text
git revert (git log --grep='^fix: make Qt WASM UI readable$' -1 --format='%H')
```

Commit message: `fix: make Qt WASM UI readable`.

---

### Task 3: Deploy and verify every production UI state

**Files:**

- Create transiently: `output/playwright/task4-fix/**`
- Create durably: `docs/evidence/qt-wasm-menu.png`
- Create durably: `docs/evidence/enhanced-fallback.png`
- Create durably: `docs/evidence/qt-wasm-production.md`

**Interfaces:**

- Consumes: first successful Actions deployment containing the font/assets fix.
- Produces: durable screenshots and exact command/result evidence.

- [ ] **Step 1: Push the fix and wait**

Run `git push origin HEAD:main`, identify the run by the fix commit SHA, and wait until both
`build` and `deploy` conclude `success`.

- [ ] **Step 2: Verify HTTP/MIME/assets**

Require HTTP 200 for root, enhanced page, WASM, JS, `qtloader.js`, `qtlogo.svg`, and
`favicon.svg`; require `application/wasm` for WASM and SVG content types for icons.

- [ ] **Step 3: Use a fresh Playwright session**

Check `npx`, open the production root headed, snapshot before ref interaction, and verify:

1. menu and persisted values after reload;
2. shop;
3. Ready/Playing plus two distinct animation frames;
4. pause;
5. game-over;
6. console/network has no missing-glyph or icon error;
7. persistence values unchanged across reload;
8. enhanced fallback.

- [ ] **Step 4: Preserve compact evidence**

Copy/resize only the final Qt menu and enhanced fallback screenshots to
`docs/evidence/`. Write `docs/evidence/qt-wasm-production.md` with the exact Actions URL,
headers, localStorage values, Playwright commands/results, screenshot hashes, and observations.
Remove all other transient browser output.

---

### Task 4: Final records and clean handoff

**Files:**

- Modify: `progress.md`
- Modify: `.superpowers/sdd/task-4-report.md`
- Create/modify: `docs/evidence/*`

**Interfaces:**

- Consumes: successful deployment and browser evidence.
- Produces: auditable repository history and clean worktree.

- [ ] **Step 1: Append deployment evidence**

Append a new `progress.md` entry containing the post-fix Actions URL, route/MIME results,
browser matrix, persistence evidence, evidence file list, concerns, and rollback command:

```text
git revert (git log --grep='^docs: preserve Qt WASM production evidence$' -1 --format='%H')
```

- [ ] **Step 2: Update the full ignored report**

Append all new commits, run URLs, HTTP headers, browser commands/observations, evidence hashes,
files, self-review, and remaining concerns to `.superpowers/sdd/task-4-report.md`.

- [ ] **Step 3: Verify and commit durable evidence**

Run the static test, font cmap check, `git diff --check`, inspect the complete diff, then commit:

```text
docs: preserve Qt WASM production evidence
```

- [ ] **Step 4: Push and wait for the final triggered run**

Push with `git push origin HEAD:main`; wait for every triggered run to complete successfully.

- [ ] **Step 5: Final self-review**

Confirm local `HEAD`, `origin/main`, and remote `main` match; production routes still pass;
`git status --porcelain` is empty; only compact approved evidence remains committed.

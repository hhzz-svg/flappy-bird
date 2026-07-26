# Qt WebAssembly GitHub Pages Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Compile the existing Qt Widgets/C++ Flappy Bird as WebAssembly and publish it at the repository's GitHub Pages root URL.

**Architecture:** Keep the qmake desktop project as the single game implementation. Add a WASM-only `QSettings` format selection, then use a GitHub Actions workflow to install the pinned Qt/Emscripten toolchain, stage the generated runtime as a static site, and deploy it with the official Pages actions.

**Tech Stack:** C++17, Qt Widgets 6.8.3, qmake, Emscripten 3.1.56, aqtinstall 3.3.0, GitHub Actions, GitHub Pages.

## Global Constraints

- The Pages root URL must open the Qt/C++ WebAssembly build directly.
- The existing `flappy-bird-enhanced.html` must remain available as `/enhanced.html`.
- Use Qt 6.8.3 `wasm_singlethread` and Emscripten 3.1.56.
- Keep `FlappyBird.pro`; do not migrate the project to CMake.
- Do not change gameplay, rendering, controls, modes, or desktop persistence behavior.
- A failed build must not deploy partial site output.
- Every repository change must be verified and appended to `progress.md`.

---

## File Map

- Modify `main.cpp`: select browser-backed `QSettings` only for WASM builds.
- Create `.github/workflows/deploy-pages.yml`: build, validate, upload, and deploy the static site.
- Create `docs/deployment.md`: document the online URLs, automated deployment, and local build commands.
- Modify `README.md`: make the playable Qt URL and deployment document discoverable.
- Modify `progress.md`: append verification evidence and rollback points for each implementation task.

### Task 1: Browser-persistent Qt settings

**Files:**

- Modify: `main.cpp`
- Modify: `progress.md`

**Interfaces:**

- Consumes: existing `QSettings("FlappyQt", "FlappyBird")` calls in `gamewidget.cpp`.
- Produces: WASM-only process default `QSettings::WebLocalStorageFormat`; desktop builds retain `NativeFormat`.

- [ ] **Step 1: Run a source assertion that demonstrates the persistence selection is absent**

Run:

```powershell
$source = Get-Content -Raw -Encoding UTF8 -LiteralPath 'main.cpp'
if ($source -notmatch 'QSettings::WebLocalStorageFormat') {
    throw 'WASM localStorage format is not configured'
}
```

Expected: command fails with `WASM localStorage format is not configured`.

- [ ] **Step 2: Add the minimal WASM-only setting selection**

Replace `main.cpp` with:

```cpp
#include <QApplication>
#include <QSettings>
#include "gamewidget.h"

int main(int argc, char *argv[])
{
#ifdef Q_OS_WASM
    QSettings::setDefaultFormat(QSettings::WebLocalStorageFormat);
#endif

    QApplication app(argc, argv);
    GameWidget w;
    w.setWindowTitle(QStringLiteral("Flappy Bird - Qt"));
    w.show();
    return app.exec();
}
```

- [ ] **Step 3: Re-run the source assertion**

Run:

```powershell
$source = Get-Content -Raw -Encoding UTF8 -LiteralPath 'main.cpp'
if ($source -notmatch '#ifdef Q_OS_WASM' -or
    $source -notmatch 'QSettings::setDefaultFormat\(QSettings::WebLocalStorageFormat\)') {
    throw 'WASM settings guard is incomplete'
}
```

Expected: exit code `0`.

- [ ] **Step 4: Compile the desktop build with the installed Qt 6 MinGW kit**

Run:

```powershell
New-Item -ItemType Directory -Force -Path 'build\desktop-pages' | Out-Null
Push-Location 'build\desktop-pages'
$env:PATH = 'E:\Qt\Tools\mingw1310_64\bin;' + $env:PATH
& 'E:\Qt\6.11.1\mingw_64\bin\qmake.exe' '..\..\FlappyBird.pro' 'CONFIG+=release'
& 'E:\Qt\Tools\mingw1310_64\bin\mingw32-make.exe' -j2
Pop-Location
```

Expected: exit code `0` and `build\desktop-pages\release\FlappyBird.exe` exists.

- [ ] **Step 5: Append the task record**

Append this entry to `progress.md`, using the actual command results:

```markdown
## 2026-07-26 - Task: 为 Qt WebAssembly 启用浏览器存档

### What was done

- WebAssembly 构建改用浏览器 localStorage 保存最高分、金币和皮肤。
- 桌面构建继续使用原有平台默认设置存储。

### Testing

- WASM 条件编译与 `WebLocalStorageFormat` 源码断言通过。
- Qt 6.11.1 MinGW 桌面 Release 构建通过，生成 `build/desktop-pages/release/FlappyBird.exe`。

### Notes

- `main.cpp`：增加 WASM 专用的 QSettings 默认格式。
- `progress.md`：追加实现与验证记录。
- 回滚方式：执行 `git revert (git log --grep='^feat: persist WASM game settings in browser$' -1 --format='%H')`。
```

- [ ] **Step 6: Verify and commit**

Run:

```powershell
git diff --check
git add -- main.cpp progress.md
git commit -m "feat: persist WASM game settings in browser"
```

Expected: `git diff --check` has no output and the commit succeeds.

### Task 2: GitHub Pages build and deployment workflow

**Files:**

- Create: `.github/workflows/deploy-pages.yml`
- Modify: `progress.md`

**Interfaces:**

- Consumes: `FlappyBird.pro`, C++ sources, and `flappy-bird-enhanced.html`.
- Produces: Pages artifact containing `index.html`, `FlappyBird.js`, `FlappyBird.wasm`, `qtloader.js`, `.nojekyll`, and `enhanced.html`.

- [ ] **Step 1: Run a repository assertion that demonstrates the workflow is absent**

Run:

```powershell
if (-not (Test-Path -LiteralPath '.github\workflows\deploy-pages.yml')) {
    throw 'Pages workflow is not present'
}
```

Expected: command fails with `Pages workflow is not present`.

- [ ] **Step 2: Create the build and deployment workflow**

Create `.github/workflows/deploy-pages.yml` with:

```yaml
name: Deploy Qt WebAssembly to GitHub Pages

on:
  push:
    branches:
      - main
  workflow_dispatch:

permissions:
  contents: read
  pages: write
  id-token: write

concurrency:
  group: github-pages
  cancel-in-progress: true

jobs:
  build:
    runs-on: ubuntu-22.04
    steps:
      - name: Check out repository
        uses: actions/checkout@v4

      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: "3.12"

      - name: Set up Emscripten
        uses: mymindstorm/setup-emsdk@v14
        with:
          version: "3.1.56"
          actions-cache-folder: emsdk-cache

      - name: Install Qt for WebAssembly
        run: |
          python -m pip install "aqtinstall==3.3.0"
          aqt install-qt all_os wasm 6.8.3 wasm_singlethread \
            --autodesktop \
            --outputdir "$RUNNER_TEMP/Qt"

      - name: Build Qt WebAssembly application
        run: |
          mkdir build-wasm
          cd build-wasm
          "$RUNNER_TEMP/Qt/6.8.3/wasm_singlethread/bin/qmake" \
            ../FlappyBird.pro CONFIG+=release
          make -j2

      - name: Stage and validate site
        run: |
          mkdir site
          cp build-wasm/FlappyBird.html site/index.html
          find build-wasm -maxdepth 1 -type f \
            \( -name '*.js' -o -name '*.wasm' \) \
            -exec cp '{}' site/ \;
          cp flappy-bird-enhanced.html site/enhanced.html
          touch site/.nojekyll
          test -s site/index.html
          test -s site/FlappyBird.js
          test -s site/FlappyBird.wasm
          test -s site/qtloader.js
          test -s site/enhanced.html

      - name: Configure GitHub Pages
        uses: actions/configure-pages@v5

      - name: Upload Pages artifact
        uses: actions/upload-pages-artifact@v3
        with:
          path: site

  deploy:
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    runs-on: ubuntu-22.04
    needs: build
    steps:
      - name: Deploy GitHub Pages
        id: deployment
        uses: actions/deploy-pages@v4
```

- [ ] **Step 3: Validate required workflow controls and artifacts**

Run:

```powershell
$workflow = Get-Content -Raw -Encoding UTF8 -LiteralPath '.github\workflows\deploy-pages.yml'
$required = @(
    'wasm_singlethread',
    '3.1.56',
    'actions/upload-pages-artifact@v3',
    'actions/deploy-pages@v4',
    'site/index.html',
    'site/enhanced.html'
)
foreach ($value in $required) {
    if (-not $workflow.Contains($value)) {
        throw "Workflow is missing: $value"
    }
}
```

Expected: exit code `0`.

- [ ] **Step 4: Append the task record**

Append this entry to `progress.md`:

```markdown
## 2026-07-26 - Task: 增加 Qt WebAssembly Pages 工作流

### What was done

- 增加固定版本的 Qt WebAssembly 自动构建。
- 增加完整产物检查和 GitHub Pages 部署，保留 HTML 增强版备用入口。

### Testing

- 工作流关键版本、权限、Pages actions 和产物断言通过。
- `git diff --check` 通过；线上构建结果在发布任务中继续验证。

### Notes

- `.github/workflows/deploy-pages.yml`：新增 WASM 构建、产物验证与 Pages 部署。
- `progress.md`：追加实现与验证记录。
- 回滚方式：执行 `git revert (git log --grep='^ci: deploy Qt WebAssembly to GitHub Pages$' -1 --format='%H')`。
```

- [ ] **Step 5: Verify and commit**

Run:

```powershell
git diff --check
git add -- .github/workflows/deploy-pages.yml progress.md
git commit -m "ci: deploy Qt WebAssembly to GitHub Pages"
```

Expected: `git diff --check` has no output and the commit succeeds.

### Task 3: Deployment documentation

**Files:**

- Create: `docs/deployment.md`
- Modify: `README.md`
- Modify: `progress.md`

**Interfaces:**

- Consumes: deployed route contract from Task 2.
- Produces: user-facing play URL, fallback URL, rebuild behavior, and local WASM build reference.

- [ ] **Step 1: Run a documentation assertion that demonstrates the Qt play URL is absent**

Run:

```powershell
$readme = Get-Content -Raw -Encoding UTF8 -LiteralPath 'README.md'
if ($readme -notmatch 'https://hhzz-svg.github.io/flappy-bird/') {
    throw 'README does not contain the Qt Pages URL'
}
```

Expected: command fails with `README does not contain the Qt Pages URL`.

- [ ] **Step 2: Create the deployment guide**

Create `docs/deployment.md` with:

```markdown
# GitHub Pages 部署

## 在线地址

- Qt WebAssembly 版：<https://hhzz-svg.github.io/flappy-bird/>
- HTML 增强版：<https://hhzz-svg.github.io/flappy-bird/enhanced.html>

根地址运行由 `main.cpp` 和 `gamewidget.*` 编译得到的 Qt/C++ 版本。

## 自动部署

推送到 `main` 或在 Actions 页面手动运行
`Deploy Qt WebAssembly to GitHub Pages` 后，工作流会：

1. 安装 Qt 6.8.3 `wasm_singlethread` 和 Emscripten 3.1.56。
2. 使用 `FlappyBird.pro` 编译 WebAssembly。
3. 检查 Qt 启动页、JavaScript、WASM 和备用 HTML 页面。
4. 仅在构建成功后更新 GitHub Pages。

## 本地 WebAssembly 构建

准备 Qt 6.8.3 WebAssembly 和 Emscripten 3.1.56，把 WebAssembly kit 的 `qmake`
加入 `PATH` 并激活 Emscripten 环境，然后运行：

```bash
mkdir build-wasm
cd build-wasm
qmake ../FlappyBird.pro CONFIG+=release
make -j2
```

构建后必须通过 HTTP 服务打开 `FlappyBird.html`，不能直接使用 `file://`。

## 存档

Qt WebAssembly 版使用当前站点的 `localStorage` 保存最高分、金币和皮肤。清除该站点
的浏览器数据会同时清除游戏进度；禁用站点存储时，刷新后不保证保留进度。

## 回滚

撤销引入问题的提交并推送到 `main`，Pages 工作流会重新发布回滚后的版本。若工作流
本身无法运行，可在 Actions 页面选择最后一个可用提交手动运行工作流。
```

- [ ] **Step 3: Add the online Qt entry near the top of README**

Insert after the `# Flappy Bird` heading:

```markdown
## 在线游玩

- **Qt WebAssembly 版（默认）**：<https://hhzz-svg.github.io/flappy-bird/>
- HTML 增强版（备用）：<https://hhzz-svg.github.io/flappy-bird/enhanced.html>

部署与本地 WebAssembly 构建说明见 [`docs/deployment.md`](docs/deployment.md)。

---
```

- [ ] **Step 4: Verify both routes and the deployment document are referenced**

Run:

```powershell
$readme = Get-Content -Raw -Encoding UTF8 -LiteralPath 'README.md'
$deployment = Get-Content -Raw -Encoding UTF8 -LiteralPath 'docs\deployment.md'
foreach ($value in @(
    'https://hhzz-svg.github.io/flappy-bird/',
    'enhanced.html',
    'docs/deployment.md'
)) {
    if (-not $readme.Contains($value)) {
        throw "README is missing: $value"
    }
}
if (-not $deployment.Contains('wasm_singlethread') -or
    -not $deployment.Contains('Emscripten 3.1.56')) {
    throw 'Deployment guide is missing the pinned toolchain'
}
```

Expected: exit code `0`.

- [ ] **Step 5: Append the task record**

Append this entry to `progress.md`:

```markdown
## 2026-07-26 - Task: 补充在线游玩与部署文档

### What was done

- README 增加 Qt WebAssembly 默认入口和 HTML 备用入口。
- 增加自动部署、本地构建、存档与回滚说明。

### Testing

- README 的两个在线地址和部署文档链接断言通过。
- 部署文档的固定工具链断言与 `git diff --check` 通过。

### Notes

- `README.md`：增加在线游玩入口。
- `docs/deployment.md`：新增部署和本地构建说明。
- `progress.md`：追加实现与验证记录。
- 回滚方式：执行 `git revert (git log --grep='^docs: add Qt WebAssembly play and deployment guide$' -1 --format='%H')`。
```

- [ ] **Step 6: Verify and commit**

Run:

```powershell
git diff --check
git add -- README.md docs/deployment.md progress.md
git commit -m "docs: add Qt WebAssembly play and deployment guide"
```

Expected: `git diff --check` has no output and the commit succeeds.

### Task 4: Enable Pages, publish, and verify production

**Files:**

- Modify: `progress.md`

**Interfaces:**

- Consumes: committed workflow and documentation from Tasks 1-3.
- Produces: live Qt game at `https://hhzz-svg.github.io/flappy-bird/`.

- [ ] **Step 1: Confirm the local branch contains only committed task changes**

Run:

```powershell
git status --short --branch
git log -5 --oneline --decorate
```

Expected: working tree is clean and `main` is ahead of `origin/main`.

- [ ] **Step 2: Enable Pages with GitHub Actions as the build source**

Run:

```powershell
gh api --method POST repos/hhzz-svg/flappy-bird/pages -f build_type=workflow
```

Expected: HTTP `201` response describing the new Pages site.

- [ ] **Step 3: Push all reviewed commits**

Run:

```powershell
git push origin main
```

Expected: `main` is updated on `origin` and the deployment workflow starts.

- [ ] **Step 4: Watch the deployment workflow to completion**

Run:

```powershell
$run = gh run list `
    --repo hhzz-svg/flappy-bird `
    --workflow 'Deploy Qt WebAssembly to GitHub Pages' `
    --limit 1 `
    --json databaseId,url,status,conclusion |
    ConvertFrom-Json
$run | Format-List
gh run watch $run.databaseId --repo hhzz-svg/flappy-bird --exit-status
```

Expected: the `build` and `deploy` jobs complete with conclusion `success`.

- [ ] **Step 5: Validate production routes and WASM content type**

Run:

```powershell
curl.exe --fail --silent --show-error `
    --output "$env:TEMP\flappy-pages-index.html" `
    https://hhzz-svg.github.io/flappy-bird/
curl.exe --fail --silent --show-error `
    --output NUL `
    https://hhzz-svg.github.io/flappy-bird/enhanced.html
curl.exe --fail --silent --show-error --head `
    https://hhzz-svg.github.io/flappy-bird/FlappyBird.wasm
Select-String -LiteralPath "$env:TEMP\flappy-pages-index.html" `
    -Pattern 'FlappyBird.js','qtloader.js'
```

Expected: both pages return HTTP `200`, WASM returns HTTP `200` with
`Content-Type: application/wasm`, and the root HTML references both runtime scripts.

- [ ] **Step 6: Run an interactive browser smoke test**

Open `https://hhzz-svg.github.io/flappy-bird/` in a desktop browser and verify:

1. The loading state completes and the Qt game menu is visible.
2. Clicking the game surface focuses it.
3. Space or mouse click starts a game and the bird/obstacles update.
4. After writing game progress, reloading the page retains the browser settings.
5. `https://hhzz-svg.github.io/flappy-bird/enhanced.html` loads the existing HTML version.

Expected: all five checks pass without a JavaScript initialization error.

- [ ] **Step 7: Record production evidence and commit**

Run this with `$run` still holding the successful workflow object from Step 4:

```powershell
$runUrl = $run.url
$entry = @"

## 2026-07-26 - Task: 发布并验证 Qt WebAssembly GitHub Pages

### What was done

- GitHub Pages 已启用 GitHub Actions 发布源。
- Qt WebAssembly 版已发布到根地址，HTML 增强版保留在备用地址。

### Testing

- GitHub Actions 构建与部署成功：$runUrl。
- 根页面、备用页面和 WASM 资源均返回 HTTP 200，WASM MIME 类型正确。
- 浏览器冒烟验证通过：Qt 菜单、输入、画面更新和刷新后存档均正常。

### Notes

- ``progress.md``：追加线上发布与验收证据。
- GitHub Pages 设置：构建来源设为 GitHub Actions。
- 回滚方式：执行 ``git revert (git log --grep='^ci: deploy Qt WebAssembly to GitHub Pages$' -1 --format='%H')`` 后推送 ``main``，或手动重跑最后一个可用提交。
"@
Add-Content -Encoding UTF8 -LiteralPath 'progress.md' -Value $entry
```

Then run:

```powershell
git diff --check
git add -- progress.md
git commit -m "docs: record GitHub Pages deployment verification"
git push origin main
```

Expected: verification record is committed and the final documentation-only deployment also succeeds.

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

撤销引入问题的提交并推送到 `main`，Pages 工作流会重新发布回滚后的版本。若需要发布
历史版本，请从已知可用的提交创建或选择分支，并在 Actions 页面从该分支手动运行工作流。

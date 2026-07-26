# Qt WebAssembly GitHub Pages 部署设计

## 目标

把仓库中的 Qt Widgets/C++ 版 Flappy Bird 编译为 WebAssembly，并部署到
`https://hhzz-svg.github.io/flappy-bird/`。访问根地址时直接进入 Qt 版游戏；
现有 HTML 增强版保留为备用页面，但不作为主页。

## 范围

- 保留 `main.cpp`、`gamewidget.h` 和 `gamewidget.cpp` 作为游戏实现，不重写玩法。
- 增加 Qt WebAssembly 构建和 GitHub Pages 自动部署。
- 保留桌面 Qt 构建能力和现有 HTML 增强版。
- 让最高分、金币、已购买皮肤和当前皮肤在同一浏览器源下跨刷新保存。
- 更新使用及部署文档。

不增加新玩法，不重构无关代码，不把现有 HTML 版包装成 Qt 版。

## 技术方案

### 工具链

- 使用 Qt 6.8.3 的 `wasm_singlethread` 二进制包。
- 使用 Qt 6.8 官方匹配的 Emscripten 3.1.56。
- 继续使用现有 qmake 工程 `FlappyBird.pro`，避免为部署额外迁移到 CMake。
- 使用 GitHub Actions 的 Ubuntu runner 完成安装、编译和 Pages 发布。

单线程 WebAssembly 不要求跨源隔离响应头，更适合 GitHub Pages；固定 Qt 和
Emscripten 版本可避免二者 ABI 不兼容。

### 构建产物

Actions 使用 Qt WebAssembly 的 qmake 和 `make` 生成 HTML、JavaScript 和
WebAssembly 文件。生成的 Qt 启动页改名为站点根目录的 `index.html`，其余运行时
文件保持相对路径不变。仓库中的 `flappy-bird-enhanced.html` 复制为
`enhanced.html`。

Pages 仅上传完整的站点产物目录。编译或产物检查失败时，部署步骤不会运行，因此
不会用残缺文件覆盖线上版本。

### 浏览器运行

页面加载 Qt 运行时后创建现有 `QApplication` 和 `GameWidget`。游戏仍使用
`QPainter` 绘制、`QTimer` 驱动，并保留鼠标和键盘操作。Qt 生成的启动页负责显示
加载状态和初始化错误。

根地址只承载 Qt 游戏，不增加额外导航层，避免首次点击后才能进入游戏。备用 HTML
版通过 `/flappy-bird/enhanced.html` 单独访问。

### 存档

WebAssembly 构建启动时把 `QSettings` 默认格式设为
`QSettings::WebLocalStorageFormat`，数据写入当前 GitHub Pages 源的
`window.localStorage`。桌面构建不执行这段条件代码，继续使用原有平台默认格式。

存档键和组织/应用名称保持不变，因此最高分、金币、皮肤数据结构无需迁移。浏览器
禁用站点存储时，游戏仍可运行，但刷新后不保证保留进度。

### GitHub Pages

工作流在以下情况运行：

- 推送到 `main` 时自动构建和部署；
- 允许从 Actions 页面手动运行。

工作流使用最小权限：读取仓库内容、写入 Pages、签发部署身份令牌。部署并发组只
保留最新任务，避免旧提交晚于新提交上线。GitHub Pages 的构建来源设置为
GitHub Actions。

## 验证

实施完成后执行以下检查：

1. 使用本机 Qt 桌面工具链重新编译，确认条件代码不破坏桌面版。
2. 触发 GitHub Actions，确认 Qt WebAssembly 编译和 Pages 部署任务成功。
3. 检查站点根目录返回成功，且 `index.html` 引用的 JavaScript 和 WebAssembly
   文件均可访问。
4. 在桌面浏览器打开线上地址，确认菜单可见、鼠标或键盘可开始游戏、画面持续更新。
5. 产生可保存数据后刷新页面，确认最高分、金币和皮肤状态仍存在。
6. 打开 `enhanced.html`，确认备用 HTML 版仍可访问。

## 回滚

如需回滚，撤销本次实现提交并重新运行此前可用提交的 Pages 工作流。若只需停止
站点，可在仓库 Pages 设置中禁用发布；源码和桌面版不受影响。

## 依据

- Qt 6.8 官方支持 WebAssembly 使用 Emscripten 3.1.56。
- Qt 6.8.3 提供 WASM 专用的 `QSettings::WebLocalStorageFormat`。
- aqtinstall 支持 `wasm_singlethread` 架构及 WebAssembly 配套桌面工具安装。

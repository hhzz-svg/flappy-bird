## 2026-07-26 - Task: 确认 Qt WebAssembly GitHub Pages 部署设计

### What was done

- 明确根地址直接运行 Qt/C++ WebAssembly 版，现有 HTML 增强版仅作为备用页面。
- 固定 Qt 6.8.3、Emscripten 3.1.56、qmake 和 GitHub Actions 的最小部署方案。
- 明确浏览器存档、失败保护、验证标准与回滚方式。

### Testing

- 已核对 Qt 官方支持矩阵：Qt 6.8 WebAssembly 对应 Emscripten 3.1.56。
- 已核对 Qt 6.8.3 源码：WASM 构建提供 `QSettings::WebLocalStorageFormat`。
- 已执行设计文档占位符扫描和 `git diff --check`，均通过。

### Notes

- `docs/superpowers/specs/2026-07-26-qt-webassembly-pages-design.md`：新增经用户确认的部署设计。
- `progress.md`：追加本轮设计落档与验证记录。
- 回滚方式：对本轮设计提交执行 `git revert HEAD`。

## 2026-07-26 - Task: 编写 Qt WebAssembly Pages 实施计划

### What was done

- 将已确认设计拆成浏览器存档、Pages 工作流、部署文档和线上验收四个闭环任务。
- 为每项改动明确文件、完整代码、验证命令、提交点和回滚方式。

### Testing

- 已逐项核对计划覆盖设计中的工具链、根入口、备用页面、存档、失败保护和验收要求。
- 已执行禁止占位符扫描与 `git diff --check`，均通过。

### Notes

- `docs/superpowers/plans/2026-07-26-qt-webassembly-pages.md`：新增可执行实施计划。
- `progress.md`：追加本轮计划落档与自检记录。
- 回滚方式：执行 `git revert (git log --grep='^docs: plan Qt WebAssembly Pages implementation$' -1 --format='%H')`。

## 2026-07-26 - Task: 准备隔离施工工作区

### What was done

- 将项目内 `.worktrees/` 目录加入 Git 忽略规则，为 Qt WebAssembly 部署施工创建隔离工作区。

### Testing

- `git check-ignore .worktrees/qt-wasm-pages` 返回匹配，确认隔离工作区不会进入版本控制。
- `git diff --check` 通过。

### Notes

- `.gitignore`：忽略项目内 Git worktree 目录。
- `progress.md`：追加隔离工作区准备记录。
- 回滚方式：执行 `git revert (git log --grep='^chore: ignore local worktrees$' -1 --format='%H')`。

## 2026-07-26 - Task: 为 Qt WebAssembly 启用浏览器存档

### What was done

- WebAssembly 构建改用浏览器 localStorage 保存最高分、金币和皮肤。
- 桌面构建继续使用原有平台默认设置存储。

### Testing

- WASM 条件编译中 `WebLocalStorageFormat` 源码断言通过。
- Qt 6.11.1 MinGW 桌面 Release 构建通过，生成 `build/desktop-pages/release/FlappyBird.exe`。

### Notes

- `main.cpp`：增加 WASM 专用的 QSettings 默认格式。
- `progress.md`：追加实现与验证记录。
- 回滚方式：执行 `git revert (git log --grep='^feat: persist WASM game settings in browser$' -1 --format='%H')`。

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

## 2026-07-26 - Task: 修正 Pages 回滚说明

### What was done

- 将历史版本发布说明改为从已知可用提交创建或选择分支后手动运行工作流。

### Testing

- README 在线地址、部署文档引用和固定工具链断言通过。
- `git diff --check` 通过。

### Notes

- `docs/deployment.md`：修正 GitHub Pages 历史版本回滚操作。
- `progress.md`：追加修正与验证记录。
- 回滚方式：执行 `git revert (git log --grep='^docs: correct Pages rollback instructions$' -1 --format='%H')`。

## 2026-07-26 - Task: 发布并验证 Qt WebAssembly GitHub Pages

### What was done

- GitHub Pages 已启用 GitHub Actions 发布源。
- Qt WebAssembly 版已发布到根地址，HTML 增强版保留在备用地址。

### Testing

- GitHub Actions 构建与部署成功：https://github.com/hhzz-svg/flappy-bird/actions/runs/30203319047。
- 根页面、备用页面、JavaScript 运行时和 WASM 资源均返回 HTTP 200，WASM MIME 类型为 `application/wasm`。
- Playwright 真实浏览器验证通过：Qt 菜单可见，键盘输入可开始游戏，画面和障碍持续更新；实际游玩生成最高分 19、金币 8，刷新后 localStorage 与菜单仍显示相同数据。
- HTML 增强版加载成功且浏览器控制台无错误。

### Notes

- `progress.md`：追加线上发布与验收证据。
- GitHub Pages 设置：构建来源设为 GitHub Actions。
- 关注项：Qt 页面加载时 `qtlogo.svg` 与站点 `favicon.ico` 返回 404，不影响游戏初始化和交互；Qt 画布内部分中文与 emoji 字形缺失。
- 回滚方式：执行 `git revert (git log --grep='^ci: deploy Qt WebAssembly to GitHub Pages$' -1 --format='%H')` 后推送 `main`，或按 `docs/deployment.md` 从最后一个可用提交手动发布。

## 2026-07-26 - Task: 设计 Qt WebAssembly 可读性与证据修复

### What was done

- 明确采用 OFL 授权的 Noto Sans CJK SC 最小字形子集，仅在 WebAssembly 版注册和使用。
- 明确用可读中文替换不受支持的装饰性 emoji，并补齐加载 SVG、favicon 与持久化生产证据。

### Testing

- 设计已覆盖字体许可与来源、桌面行为隔离、Pages 产物失败保护、全界面浏览器验收和证据落点。
- 已检查设计没有 `TBD`、`TODO`、范围矛盾或未定义回滚方式。

### Notes

- `docs/superpowers/specs/2026-07-26-qt-wasm-readability-evidence-design.md`：新增经用户批准的修复设计。
- `progress.md`：追加设计落档记录。
- 回滚方式：执行 `git revert (git log --grep='^docs: design Qt WASM readability and evidence fix$' -1 --format='%H')`。

## 2026-07-26 - Task: 编写 Qt WebAssembly 可读性修复计划

### What was done

- 将字体子集、加载资源、生产全界面验收和持久证据拆成四个可独立验证的施工任务。
- 为每项改动明确先失败断言、最小实现、验证命令、提交边界和最终回滚点。

### Testing

- 计划逐项覆盖已批准设计中的字体许可、WASM 条件隔离、桌面构建、Pages 失败保护、浏览器状态矩阵和证据清理。
- 计划已通过占位符、接口名称、文件路径和范围一致性检查。

### Notes

- `docs/superpowers/plans/2026-07-26-qt-wasm-readability-evidence.md`：新增可执行施工计划。
- `progress.md`：追加计划落档记录。
- 回滚方式：执行 `git revert (git log --grep='^docs: plan Qt WASM readability and evidence fix$' -1 --format='%H')`。

## 2026-07-26 - Task: 修复 Qt WebAssembly 中文字形与加载资源

### What was done

- 使用官方 Noto Sans CJK SC `Sans2.004` 生成 115436 字节的 OFL 界面字形子集，记录来源、源文件与子集校验值、许可全文和可复现命令。
- 字体仅在 WebAssembly 构建中嵌入、注册和用于游戏文字；桌面版继续使用原有 Arial 路径。
- 将不受支持的装饰性 emoji 改成可读中文，为 Pages 增加项目自有加载 SVG、SVG/ICO favicon、首页引用注入和失败保护。

### Testing

- 字体/资源静态断言完成红绿验证：首次因缺少字体、加载 SVG 和 ICO 分别失败，实现后全部通过。
- fontTools cmap 检查通过：221 个请求字符与源码中 188 个 `QStringLiteral` 字符全部存在于子集。
- OFL 全文逐行比对官方 `Sans2.004` 许可通过；ICO 包含 64、32、16 像素三种尺寸。
- Qt 6.11.1 MinGW 桌面 Release 构建通过，生成 `build/desktop-pages-fontfix-ascii/release/FlappyBird.exe`。
- `git diff --check` 通过；线上 WASM 构建、全界面字体和资源 404 在后续部署任务验证。

### Notes

- `.github/workflows/deploy-pages.yml`：阶段化加载图标与 favicon，注入首页引用并增加产物断言。
- `FlappyBird.pro`、`resources.qrc`、`main.cpp`、`gamewidget.cpp`：增加 WASM 专用字体资源、注册与文本选择，替换不受支持的装饰性 emoji。
- `assets/fonts/NotoSansSC-UI-Subset.otf`、`assets/fonts/OFL.txt`、`assets/fonts/SOURCE.md`、`assets/fonts/subset-glyphs.txt`：增加字体子集、许可、来源和字形清单。
- `assets/qtlogo.svg`、`assets/favicon.svg`、`assets/favicon.ico`：增加项目自有加载与站点图标。
- `tools/verify-wasm-ui.ps1`：增加可重复执行的字体、符号、资源和工作流静态验证。
- `docs/deployment.md`：补充字体许可、构建隔离和图标 staging 说明。
- `progress.md`：追加本轮实现与验证记录。
- 回滚方式：执行 `git revert (git log --grep='^fix: make Qt WASM UI readable$' -1 --format='%H')`。

## 2026-07-26 - Task: 删除内部设计与实施计划文档

### What was done

- 删除 `docs/superpowers/` 下四份内部设计与实施计划文档。
- 保留正式部署说明、字体许可证、运行代码和历史进度记录。

### Testing

- 已确认 `docs/superpowers/` 下不再存在已跟踪文件。
- `git diff --check` 通过。

### Notes

- `docs/superpowers/specs/2026-07-26-qt-webassembly-pages-design.md`：删除初始部署设计。
- `docs/superpowers/specs/2026-07-26-qt-wasm-readability-evidence-design.md`：删除可读性修复设计。
- `docs/superpowers/plans/2026-07-26-qt-webassembly-pages.md`：删除初始部署实施计划。
- `docs/superpowers/plans/2026-07-26-qt-wasm-readability-evidence.md`：删除可读性修复实施计划。
- `progress.md`：追加删除范围、验证和回滚记录。
- 回滚方式：执行 `git revert (git log --grep='^docs: remove internal planning documents$' -1 --format='%H')`。

## 2026-07-26 - Task: 删除剩余 docs 文档

### What was done

- 删除剩余的正式部署文档，并移除 README 中对应的失效链接。

### Testing

- 已确认仓库中不再存在 `docs/` 文件。
- 已确认 README 不再引用 `docs/deployment.md`。
- `git diff --check` 通过。

### Notes

- `docs/deployment.md`：删除部署与本地构建说明。
- `README.md`：移除部署文档链接，保留在线游玩地址。
- `progress.md`：追加删除范围、验证和回滚记录。
- 回滚方式：执行 `git revert (git log --grep='^docs: remove remaining docs$' -1 --format='%H')`。

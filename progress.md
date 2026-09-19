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

## 2026-09-17 - Task: 为所有可点击 UI 增加鼠标悬停反馈

### What was done

- 菜单卡片、皮肤商店按钮、商店格子与返回按钮、暂停与结算界面的按钮，在鼠标悬停时加亮边框与背景。
- 悬停可点击区域时光标变为手型，鼠标移出窗口时恢复。

### Testing

- Qt 6.4.2 桌面 Release 构建通过，零警告零错误。
- 逐一核对八处可点击区域的绘制分支与 `hoverActiveRegion` 判定一致。

### Notes

- `gamewidget.h`、`gamewidget.cpp`：新增 `mouseMoveEvent`/`leaveEvent`、`hoverActiveRegion`、`updateHoverCursor` 与悬停坐标成员。
- `progress.md`：追加本轮实现与验证记录。
- 回滚方式：执行 `git revert (git log --grep='^feat: add mouse hover feedback to all UI buttons$' -1 --format='%H')`。

## 2026-09-17 - Task: 修复悬停高亮与键盘选中不同步

### What was done

- 鼠标悬停菜单卡片或商店格子时同步更新 `m_menuIndex`/`m_shopIndex`，使高亮项与回车/点击实际生效项始终一致。
- 由于悬停后必然等于选中态，移除上一轮引入的、永远不会触发的独立悬停样式分支。

### Testing

- Qt 6.4.2 桌面 Release 构建通过，零警告零错误。
- 核对键盘导航路径未受影响：`syncHoverSelection` 只在 `mouseMoveEvent` 中触发。

### Notes

- `gamewidget.h`、`gamewidget.cpp`：新增 `syncHoverSelection`，简化 `drawMenu`/`drawShop` 的高亮分支。
- `progress.md`：追加本轮实现与验证记录。
- 回滚方式：执行 `git revert (git log --grep='^fix: sync mouse hover with keyboard selection in menu/shop$' -1 --format='%H')`。

## 2026-09-18 - Task: 为 UI 按钮增加按下反馈动画

### What was done

- 点击按钮时播放 110ms 的缩放下压回弹动画，动画结束后再执行对应动作。
- 因为多数按钮点击后立即切换画面，采用延迟执行模型，否则动画来不及显示。
- 键盘确认与游戏中点击起飞保持即时响应，不走该路径。

### Testing

- Qt 6.4.2 桌面 Release 构建通过，零警告零错误。
- 离屏运行测试连跑三次结果一致：卡片宽度 396→362 像素下压后回弹，约 110ms 后进入游戏；键盘回车仍为瞬时生效。

### Notes

- `gamewidget.h`、`gamewidget.cpp`：新增 `triggerPress`/`pressScale`/`withPressTransform` 与 `PRESS_DURATION`，八处按钮绘制套用按下变换。
- `progress.md`：追加本轮实现与验证记录。
- 回滚方式：执行 `git revert (git log --grep='^feat: add press feedback animation to UI buttons$' -1 --format='%H')`。

## 2026-09-18 - Task: 界面切换增加淡入过渡

### What was done

- 导航类切换（进游戏、进商店、返回菜单、再来一局、启动首屏）后新界面从暗色淡入，时长 180ms。
- 游戏内状态翻转（起飞、暂停与恢复、死亡）保持瞬时，避免手感变钝。

### Testing

- Qt 6.4.2 桌面 Release 构建通过，零警告零错误。
- 离屏测试：导航后画面平均亮度 77.9 → 156.0，证明淡入生效并完全消散；暂停恢复瞬间亮度 189.5 对比稍后 189.4，证明游戏内翻转未被加过渡。

### Notes

- `gamewidget.h`、`gamewidget.cpp`：新增 `navigateTo` 与 `m_screenFade`/`FADE_DURATION`，`paintEvent` 末尾叠加全画布遮罩。
- `progress.md`：追加本轮实现与验证记录。
- 回滚方式：执行 `git revert (git log --grep='^feat: fade in screens on navigation$' -1 --format='%H')`。

## 2026-09-18 - Task: 桌面版启用内嵌中文字体与窗口图标

### What was done

- 内嵌的 Noto Sans SC 字形子集从 WebAssembly 专用改为所有平台生效，`uiFont()` 不再按平台分支，移除 Arial 回退。
- 原生窗口设置应用图标，复用已有的 `assets/favicon.ico`（ICO 由 QtGui 内置处理，不引入 Qt SVG 模块依赖）。

### Testing

- Qt 6.4.2 桌面 Release 构建通过，零警告零错误。
- 离屏测试断言通过：资源已编入桌面构建、应用字体族为 Noto Sans CJK SC、包含中文与星号字形、ICO 图标含三种尺寸。
- 已复核 `tools/verify-wasm-ui.ps1` 依赖的四个字面量仍存在，且工作流无需改动。

### Notes

- `FlappyBird.pro`：资源改为无条件编入。
- `resources.qrc`：新增 `/icons` 前缀的 `favicon.ico`。
- `main.cpp`：字体注册去掉 WASM 条件，新增 `setWindowIcon`。
- `gamewidget.cpp`：`uiFont()` 统一使用应用字体。
- `README.md`：更新资源说明。
- `progress.md`：追加本轮实现与验证记录。
- 回滚方式：执行 `git revert (git log --grep='^feat: use the bundled CJK font and a window icon on desktop$' -1 --format='%H')`。

## 2026-09-18 - Task: 为 Qt 版加入程序化音效

### What was done

- 新增 `sfx.h`/`sfx.cpp` 音效层：WebAssembly 经 emscripten 调浏览器 Web Audio，桌面用 `QAudioSink` 加自写软件混音；两者都不需要音频素材，音色参数照搬网页增强版的合成器。
- 接入拍翅膀、得分、每 10 分里程碑、吃金币、坠毁、界面点击、风暴闪电、购买成功与金币不足九种音效。
- `Qt::Key_M` 从占位改为真正的静音开关，设置经 `QSettings` 持久化，取消静音时播放确认音；菜单快捷键提示补充 `M 静音`，静音时界面显示标记。
- 商店的购买/装备逻辑此前在键盘与鼠标两条路径重复，抽成 `purchaseOrEquip(int)`，音效只接一处。
- 字体子集补入"静""音"两个字形并同步更新 `SOURCE.md` 的体积与校验值。

### Testing

- Qt 6.4.2 桌面 Release 构建通过，零警告零错误；另单独验证未安装 Qt Multimedia 时的静默降级分支可编译。
- 离屏行为测试 16 项全过：九种事件各自触发正确音效；鼠标点击只触发一次提示音（延迟执行的 `navigateTo` 不重复发声）；键鼠两条购买路径结果完全一致（皮肤与金币余额相同）；金币不足触发拒绝音；静音可切换、可抑制发声、可跨重启持久化。
- 桌面合成器单独测试 11 项全过：50ms 延迟内静音、峰值 15830 未超过请求音量、包络衰减至 21、结束后与空闲时均为纯静音、三种波形取值均在范围内。
- 字体覆盖率断言：源码 190 个字符全部命中子集；`tools/verify-wasm-ui.ps1` 依赖的四个字面量与禁用 emoji 规则复核通过。

### Notes

- `sfx.h`、`sfx.cpp`：新增音效层。
- `FlappyBird.pro`：加入音效源码；桌面端在 `qtHaveModule(multimedia)` 成立时启用 `FB_DESKTOP_AUDIO`，缺少该模块仍可构建（静默）。
- `gamewidget.h`、`gamewidget.cpp`：音效接入点、静音开关与持久化、`purchaseOrEquip` 重构、静音标记与快捷键提示。
- `assets/fonts/NotoSansSC-UI-Subset.otf`、`assets/fonts/subset-glyphs.txt`、`assets/fonts/SOURCE.md`：补入静音标签所需字形并更新校验值。
- `README.md`：更新 Qt 版音效说明与操作表。
- `progress.md`：追加本轮实现与验证记录。
- WebAssembly 端未新增 Qt 模块依赖，CI 工具链无需改动。
- 回滚方式：执行 `git revert (git log --grep='^feat: add procedural sound effects$' -1 --format='%H')`。

## 2026-09-18 - Task: 优化网页版渲染性能

### What was done

- 剖析确认瓶颈：帧开销随窗口分辨率近似线性增长（画笔直接按窗口尺寸缩放绘制），且大面积半透明抗锯齿图形占比最高——云朵 1.83ms、远山 0.78ms、日月 0.29ms，而整屏天空渐变仅 0.07ms。
- 云朵、远山、日月改为预渲染成精灵图后每帧贴图；远山按视差周期平铺，消除接缝。
- 场景统一渲染到固定 480×720 缓冲后整体缩放贴到窗口，使光栅化量不再随窗口面积增长。
- 整屏天空渐变填充关闭抗锯齿（轴对齐矩形无需 AA）。
- 鼠标移动改为仅在悬停目标变化时重绘，不再每次移动都触发整帧重绘。
- 震屏位移时先填充缓冲底色，避免露出上一帧残留像素。

### Testing

- 帧时实测（离屏渲染中位数）：菜单 1280×1920 由 17.79ms 降至 5.51ms（3.2 倍），游戏中同尺寸由 16.60ms 降至 4.60ms（3.6 倍），夜航含迷雾由 17.50ms 降至 5.58ms（3.1 倍）；480×720 下由 4.85/5.58ms 降至 3.31/3.13ms。
- 随窗口放大的开销倍率由 3.0-3.7 倍降至 1.3-1.7 倍（剩余部分为最终一次缩放贴图）。
- 分环节复测确认天空一族由 2.6-2.9ms 降至 0.75ms。
- 菜单、商店、准备、游戏中、结算五个界面的改动前后截图逐一比对，布局、配色与文字一致。
- 行为测试 16 项、淡入与字体图标测试 10 项全部通过；桌面构建零警告。

### Notes

- `gamewidget.h`、`gamewidget.cpp`：新增 `rebuildSceneCache()` 与精灵图/固定分辨率缓冲成员，改写 `paintEvent` 与 `drawSky`，调整 `mouseMoveEvent` 重绘条件。
- `progress.md`：追加本轮实现与实测数据。
- 遗留：菜单界面的文字绘制仍占 2.2ms，是当前最大单项，可在后续用静态文字层缓存进一步优化。
- 回滚方式：执行 `git revert (git log --grep='^perf: cache scene layers and render at a fixed resolution$' -1 --format='%H')`。

## 2026-09-18 - Task: 音效改用 CC0 素材包

### What was done

- 从 npm 的 `uisfx@0.4.0` 取用 arcade 音色包，音频为 CC0 公有领域授权；下载时以 registry 的 sha512 integrity 校验，许可全文与选用清单落档。
- 对该包 78 个音效批量测量时长、峰值与频谱重心后选型：常响的拍翅膀与界面点击选最短促的音，坠毁选最响且最暗的音，金币选最明亮的音。
- 八个事件改用素材：拍翅膀、得分、里程碑、金币、坠毁、界面点击、购买、金币不足；风暴闪电保留合成音，因为整包均为 UI 音色、没有雷声所需的低频轰鸣。
- 素材统一转为单声道 22050Hz 16 位 WAV，裁掉静音、加淡入淡出、按用途做峰值归一化（拍翅膀 -18dB 最轻，坠毁 -7dB 最重），共 117KB。
- 播放实现：桌面解析 WAV 后作为采样 voice 喂入既有混音器，按设备实际采样率线性重采样；WebAssembly 侧在 JS 中同步解析 PCM 生成 AudioBuffer，避免异步解码丢掉首次手势触发的那一声。

### Testing

- 采样通路测试 16 项通过：八个音效均从资源加载成功且速率与电平正确；原生速率下可播且播完即静音；48kHz 立体声设备下重采样后仍可播、双声道一致、结束干净；采样与合成音可叠加混合；静音同时切断两者。
- 浏览器端 EM_JS 测试 16 项通过：五个 EM_JS 函数语法正确；真实 WAV 在 Chromium 中同步解析成功（时长 0.1307s 与素材一致）；未解码时播放会跳过而非报错；静音门控正常。
- 桌面构建零警告，未启用 Qt Multimedia 的降级分支同样可编译。
- 字体覆盖率、`tools/verify-wasm-ui.ps1` 依赖字面量、禁用 emoji 与音频溯源哈希复核全部通过。

### Notes

- `assets/audio/*.wav`、`assets/audio/SOURCE.md`、`assets/audio/LICENSE-CC0.txt`：新增素材、溯源与许可。
- `resources.qrc`：嵌入八个音效。
- `sfx.cpp`：新增 WAV 解析、采样 voice 混音与 WebAssembly 端同步解码，音效表改为素材优先。
- `README.md`、`progress.md`：更新音效说明与本轮记录。
- 实际听感需人工确认：容器内无音频设备。
- 回滚方式：执行 `git revert (git log --grep='^feat: replace synthesised cues with CC0 arcade samples$' -1 --format='%H')`。

## 2026-09-19 - Task: 手机访问分流到 HTML 版

### What was done

- 确认手机端掉帧与托管无关：Qt WebAssembly 的 QWidget 走软件光栅化，用不到 GPU，手机 CPU 扛不住，属架构限制。
- Pages 首页注入分流脚本：触屏且屏幕短边小于 820 的设备改用原生 Canvas 的 `enhanced.html`，桌面继续使用 Qt WebAssembly 版，`?qt=1` 为逃生口。
- 同时注入此前缺失的 viewport meta，避免未命中分流规则的移动设备按约 980px 虚拟宽度布局。
- HTML 版页脚增加回到 Qt 桌面版的链接，两版互通。

### Testing

- Playwright 真实浏览器验证 5 种场景全部符合预期：iPhone 13 与 Pixel 7 跳转到 HTML 版；1280×800 桌面与 1920×1080 触摸屏笔记本留在 Qt 版；手机带 `?qt=1` 留在 Qt 版。
- 测试脚本直接从工作流中提取待注入的脚本再执行，避免测试与实际部署内容走偏。
- 工作流增加两条断言，确认 viewport 与分流脚本确实注入成功。

### Notes

- `.github/workflows/deploy-pages.yml`：注入 viewport 与分流脚本并增加产物断言。
- `flappy-bird-enhanced.html`：页脚增加 Qt 桌面版链接与对应样式。
- `progress.md`：追加本轮记录。
- 已知缺口：HTML 版暂缺激光模式与皮肤商店，手机用户会少这两项玩法，下一轮补齐。
- 回滚方式：执行 `git revert (git log --grep='^feat: send phones to the native-canvas build$' -1 --format='%H')`。

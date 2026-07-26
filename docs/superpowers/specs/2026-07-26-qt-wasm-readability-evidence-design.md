# Qt WebAssembly 可读性与生产证据修复设计

## 目标与范围

修复线上 Qt WebAssembly 页面缺少中文字形、加载图标与 favicon 的问题，并把修复后的
浏览器验收证据作为仓库内可审计文件长期保留。玩法、输入、存档格式、桌面字体和 HTML
增强版界面保持不变。

## 字体方案

- 使用官方 Noto Sans CJK SC Regular，许可为 SIL Open Font License 1.1。
- 只保留 Qt 界面实际使用的中文、拉丁字符、数字和常用符号，生成项目专用子集，避免
  引入完整 CJK 字体的体积。
- 仓库同时保存 OFL 许可、来源版本、下载地址、源文件校验值、子集命令和字形清单。
- 子集通过 Qt 资源系统嵌入应用，仅在 `Q_OS_WASM` 下注册并作为游戏文本字体。桌面版
  继续使用现有 Arial 绘制路径。
- 不引入颜色 emoji 字体。模式图标、奖牌和提示中的装饰性 emoji 改为短中文标签或
  已由子集覆盖的普通符号，保证所有操作文字可读。

## 加载资源方案

- 增加项目自有的轻量 SVG 小鸟标志，分别作为 Qt 加载页 `qtlogo.svg` 和网页
  `favicon.svg`。
- Pages 工作流在构建后复制两个资源，并向生成的 `index.html` 注入 favicon 链接。
- 产物校验新增图标文件与 favicon 引用断言，构建缺失时不部署。

## 证据与发布闭环

- 代码与资源先提交、推送并等待 Actions 构建部署成功。
- 使用全新 Playwright 会话验证菜单、商店、暂停、游戏结束、输入、动画、存档刷新和
  HTML 增强版；同时检查控制台与网络无图标 404。
- 将压缩后的 Qt 菜单截图、增强版截图和 Markdown 验收日志保存到
  `docs/evidence/`，日志记录 Actions URL、HTTP/MIME、localStorage 和浏览器命令结果。
- `progress.md` 仅追加修复记录和生产部署记录，并列出全部改动文件、测试与可执行回滚
  命令。

## 验收标准

1. 静态断言证明字体资源、许可、WASM 条件注册、SVG staging 和 favicon 引用完整。
2. Qt 6.11.1 MinGW Release 桌面构建成功。
3. GitHub Actions 的 `build` 与 `deploy` 成功。
4. 全新浏览器会话中，所有可达 Qt 页面没有缺字方框；`qtlogo.svg`、`favicon.svg`、
   WASM 和运行时资源均返回 HTTP 200。
5. 实际输入和动画正常，存档刷新前后相同，HTML 增强版正常。
6. 证据文件已提交，最终工作树干净。

## 回滚

字体/资源修复和生产证据分别使用独立提交。需要回滚时依次执行对应提交的
`git revert <commit>` 并推送 `main`，由现有 Pages 工作流重新部署。

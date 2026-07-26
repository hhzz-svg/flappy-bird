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

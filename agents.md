
# LocalPhotoSync — AGENTS.md

本文件用于指导自动化代理与新来开发者快速上手 `LocalPhotoSync` 项目。代理会优先读取最近的 `AGENTS.md`（离工作目录最近的一个）。如果这是一个大仓库，请在子包目录再放置子级 `AGENTS.md`，以覆盖/补充根目录说明。

**技术栈**: 
- **Language**:C++ (C++17/C++20 视需求)
- **library**: boost-asio,gflags,sqlite3,Protobuf
- **test**: gtest
- **build**: cmake
- **format**: clang-format

**目录结构**

- `src/` - 源代码
- `tests/` - 单元测试（使用 gtest）
- `proto/` - protobuf 文件与生成脚本
- `build/` - 构建输出（可由 CI 或本地构建创建）

**快速项目概览**

LocalPhotoSync 是一个用于本地照片同步的服务/守护进程（参考 `README.md` 与 `IMPLEMENTATION_SUMMARY.md` 了解实现细节）。它包含网络服务、会话管理、SQLite 数据库支持及 UDP 广播等模块。

**构建与测试（快速命令）**

在项目根目录下，推荐如下步骤构建并运行测试：

```bash
mkdir -p build && build.sh
```

如果想单独运行某个测试，可在 `build` 目录里直接运行对应可执行文件。

CI 环境可以复用上面的命令，或使用 `cmake --preset` 以便统一配置。


**代码风格与质量**

- 推荐使用 `clang-format` 且在仓库根放置 `.clang-format` 以统一风格。
- 命名: 使用驼峰或下划线风格需在团队内统一 `camelCase` 
- 提交前运行静态检查（例如 `clang-tidy`、`cppcheck`）和构建测试。

示例 `clang-format` 使用（本地）:

```bash
clang-format -i $(git ls-files "*.cpp" "*.hpp" "*.h")
```

**测试说明**

- 将单元测试放在 `tests/` 下，按模块或组件命名子目录。
- 每个测试目标应只链接其被测目标需要的库，避免全局依赖污染。
- 在 CMake 中为测试启用 `-DTEST` 或 `-DENABLE_TESTS` 选项以便 CI/本地选择性构建。
- 测试用例覆盖率只要90%以上


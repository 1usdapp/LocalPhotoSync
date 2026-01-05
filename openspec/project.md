# 项目上下文

## 项目目的
LocalPhotoSync 是一个基于局域网的照片同步守护进程/服务，使用 C++ 编写。它允许客户端通过 UDP 广播进行发现，并使用 TCP 将照片同步到本地服务器，支持通过 CRC32 校验和进行可靠的文件传输与去重。

## 技术栈
- **编程语言**: C++17/C++20
- **网络**: Boost.Asio（异步 I/O）
- **序列化**: Protobuf
- **存储**: SQLite3（每目录元数据）
- **协议**: 自定义长度前缀 Protobuf 帧格式（4 字节大端序长度 + 有效载荷）
- **构建**: CMake 3.15+
- **测试**: GoogleTest (gtest)
- **依赖**: Boost (system)、zlib、gflags

## 项目约定

### 代码风格
- **格式化**: 使用 clang-format，配置文件位于仓库根目录的 .clang-format
- **命名**: 变量/函数使用 snake_case，类使用 PascalCase
- **缩进**: 2 个空格（可通过 .clang-format 配置）
- **头文件保护**: 使用 `#pragma once`
- **包含顺序**: 系统头文件 → 本地头文件；中间用空行分隔

### 架构模式
- **异步 I/O**: 基于 Boost.Asio 的事件驱动架构
- **会话机制**: 每个 TCP 连接对应一个 `Session` 对象，管理设备绑定和照片同步
- **每目录数据库**: 每个同步路径都有独立的 `index.db`（SQLite），存储文件名→CRC32 映射
- **消息帧格式**: 长度前缀 Protobuf（4 字节大端序 + 有效载荷）
- **单例服务**: `ServiceContainer` 管理 UDP 广播器、TCP 服务器和共享资源

### 测试策略
- **测试框架**: GoogleTest (gtest)
- **测试位置**: `tests/` 目录，按组件组织
- **覆盖率**: 最低 90%
- **构建标志**: `-DENABLE_TESTS` 用于选择性构建测试
- **运行方式**: `build/lps_unit_tests` 或单独的测试二进制文件
- **命名约定**: 测试文件 = `test_<组件>.cpp`；测试类 = `<组件>Test`

### Git 工作流
- **分支策略**: 大型工作使用功能分支；主分支受保护
- **提交规范**: 约定式提交（feat:, fix:, refactor:, test:, docs:）
- **CI/CD**: 推送前运行 `build.sh` + 测试
- **代码审查**: 破坏性变更、功能添加、架构调整需要 OpenSpec 提案

## 领域上下文

### 网络协议
- **UDP 广播**: 服务器每秒广播 `CSNtyServerInfo`（TCP 端口、名称、根路径、操作系统）
- **TCP 消息**: 
  - `CSReqDeviceInfo`: 客户端注册设备和同步路径
  - `CSReqSyncPhoto`: 客户端上传照片块（带 CRC32），支持多部分传输
  - 服务器通过与 `index.db` 中的文件级 CRC32 匹配进行去重
  
### 存储模型
- **根目录**: 通过 `LPS_ROOT` 配置（默认: `./data`）
- **路径**: `LPS_ROOT/<客户端提供的路径>/`（自动创建多级目录）
- **元数据库**: 每个目录都有 `index.db`，模式为 `photos(filename PRIMARY KEY, crc32 INTEGER)`
- **去重**: 如果文件 CRC 匹配数据库记录，则跳过写入；如果缺失或 CRC 不同，则写入新文件

### 配置
- 环境变量: `LPS_TCP_PORT`、`LPS_UDP_PORT`、`LPS_NAME`、`LPS_ROOT`、`LPS_OS`、`LPS_BROADCAST`
- 默认值适合单服务器部署
- 无配置文件持久化（仅环境变量）

## 重要约束
- **并发**: 相同文件的并发写入必须由客户端序列化；服务器不实现文件级锁
- **错误处理**: 解析失败、无效帧、I/O 错误会关闭会话（协议中无重连逻辑）
- **SQLite 安全**: 每个 `Session` 打开专用的数据库句柄；无跨会话数据库锁定
- **原型特性**: 基于 CRC 的去重较简单；生产环境可能需要更强的内容哈希或分块
- **单服务器假设**: 广播/同步假定单个服务器；无集群/复制支持

## 外部依赖
- **Boost**: 仅系统库（asio 仅头文件）
- **Protobuf**: `libprotobuf` + `protoc` 编译器
- **SQLite3**: C 库
- **zlib**: 压缩（可选但包含在构建中）

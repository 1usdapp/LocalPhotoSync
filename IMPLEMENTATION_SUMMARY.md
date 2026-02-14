# LocalPhotoSync 服务器实现总结

## 更新日期
2026-02-14

## 项目概述
本项目是一个基于C++的局域网照片同步服务器，使用 Boost.Asio/Beast 进行异步网络通信与 HTTP 服务，Protobuf 作为序列化协议，SQLite 存储文件元数据，并结合 gflags 进行配置管理。

## 已实现的功能

### 1. 核心网络模块

#### UDP 广播服务 (`src/udp_broadcaster.*`)
- ✅ 每秒向局域网广播服务器信息
- ✅ 发送 `CSNtyServerInfo` 消息，包含 TCP 端口、服务器名称、根路径、操作系统信息
- ✅ 使用自定义包头 `PkgHead` + Protobuf 的帧格式
- ✅ 支持可配置的广播地址和端口

#### TCP 服务器 (`src/tcp_server.*`)
- ✅ 监听配置的 TCP 端口
- ✅ 异步接受客户端连接
- ✅ 为每个连接创建独立的 Session 会话
- ✅ 支持并发多客户端连接

#### Session 会话管理 (`src/session.*`)
- ✅ 自定义包头协议：`PkgHead` 共 24 字节（`PackageLen/HeadLen/Version/CMDID/Reserve/Reserve2`）+ Protobuf payload
- ✅ 消息长度校验（限制最大 100MB）
- ✅ 处理 `CSReqDeviceInfo` 请求：
   - 绑定设备 ID 和保存路径
   - 自动创建多级目录（移除路径前导 `/`）
   - 初始化该目录的 SQLite 数据库
- ✅ 处理 `CSReqSyncPhoto` 请求：
   - 支持文件分片上传并按 `Offset` 写入
   - 当 `Offset == 0` 且数据库中 CRC 与客户端 CRC 一致时，直接跳过写入
   - 最后一片写入后计算整文件 CRC 并更新数据库
- ✅ 异步读写，避免阻塞
- ✅ 同步进度更新回调（`SyncInfo`）

#### HTTP 服务 (`src/http_server.*`, `src/http_session.*`)
- ✅ 基于 Boost.Beast 的 HTTP 服务器
- ✅ `GET /clients` 返回当前连接与同步进度信息（JSON）
- ✅ 支持 CORS 预检与基础跨域响应

### 2. 连接与状态管理

#### 连接管理器 (`src/con_mgr.*`)
- ✅ 生成连接 ID，维护连接表
- ✅ 保存客户端地址与同步进度
- ✅ 将连接信息转换为 JSON 输出（基于 `proto/http.proto`）

#### 服务容器 (`src/service_container.*`)
- ✅ 统一管理 `CConMgr` 与 `SqliteDBManager` 单例实例

### 3. 数据存储模块

#### SQLite 数据库连接管理器 (`src/sqlite_db_manager.*`)
- ✅ 集中管理 SQLite 连接，按 `index.db` 路径缓存
- ✅ 引用计数机制：Session 创建时递增，销毁时递减，计数为 0 自动关闭
- ✅ 便利方法：`get_file_crc()` / `set_file_crc()` / `delete_file()`

#### SQLite 数据库 (`src/sqlite_db.*`)
- ✅ 每个目录独立的 `index.db` 数据库
- ✅ `photos` 表结构：`(filename TEXT PRIMARY KEY, crc32 INTEGER)`
- ✅ 支持查询、插入/更新、删除文件 CRC 记录
- ✅ 自动创建表结构

#### CRC32 计算 (`src/crc.*`)
- ✅ 预计算 CRC32 查找表
- ✅ 支持字节数组与文件 CRC 计算
- ✅ 用于文件去重与完整性校验

### 4. 配置与工具

#### 配置管理 (`src/config.hpp` + gflags)
- ✅ 支持环境变量与默认值：
   - `LPS_TCP_PORT` (默认: 9176)
   - `LPS_UDP_PORT` (默认: 9176)
   - `LPS_HTTP_PORT` (默认: 9175)
   - `LPS_NAME` (默认: LPS-Server)
   - `LPS_ROOT` (默认: ./data)
   - `LPS_OS` (默认: linux)
   - `LPS_BROADCAST` (默认: 255.255.255.255)
- ✅ 命令行参数覆盖：`--tcp_port` / `--udp_port` / `--http_port` / `--root`

### 5. Protobuf 协议

#### 消息定义 (`proto/csmsg.proto`)
- ✅ `MsgPkg` 包装消息，包含 `ResultID`、`SerialID`
- ✅ `CSNtyServerInfo` - 服务器广播信息
- ✅ `CSReqDeviceInfo`/`CSResDeviceInfo` - 设备注册
- ✅ `CSReqSyncPhoto`/`CSResSyncPhoto` - 照片同步

#### HTTP 数据结构 (`proto/http.proto`)
- ✅ `HttpGetClient` 用于 `/clients` 的 JSON 响应

#### 消息 ID 生成 (`proto/msgid.proto`, `proto/gen_msgid.py`)
- ✅ 自动从 `csmsg.proto` 提取消息类型
- ✅ 生成 `MSGID` 枚举定义

### 6. 构建系统

#### CMake 配置 (`CMakeLists.txt`)
- ✅ C++20 标准
- ✅ 自动查找依赖库（Boost、Protobuf、SQLite3、Zlib、gflags）
- ✅ macOS 额外依赖 `absl`（日志/字符串工具）
- ✅ 自动生成 Protobuf C++ 代码
- ✅ 可选构建单元测试（`ENABLE_TESTS`）

#### 构建脚本 (`build.sh`, `proto/gen.sh`)
- ✅ 自动生成 `msgid.proto`
- ✅ 创建构建目录并执行 CMake + 编译
- ✅ 支持并行编译

## 文件清单

### 主要源代码
- `src/main.cpp` - 进程入口，创建 TCP/UDP/HTTP 服务器并启动
- `src/session.cpp` - TCP 会话逻辑与文件分片写入
- `src/tcp_server.cpp` - TCP 监听与连接接受
- `src/udp_broadcaster.cpp` - UDP 每秒广播服务
- `src/http_server.cpp` / `src/http_session.cpp` - HTTP 服务与路由
- `src/con_mgr.cpp` - 连接与同步进度管理
- `src/service_container.cpp` - 服务容器
- `src/sqlite_db.cpp` / `src/sqlite_db_manager.cpp` - SQLite 访问与连接管理
- `src/crc.cpp` - CRC32 工具

### 头文件与协议
- `proto/csmsg.proto` / `proto/http.proto` / `proto/msgid.proto`
- `proto/gen_msgid.py` / `proto/gen.sh`
- `proto/cmd.h` - 自定义包头协议定义
- `src/config.hpp` - 配置结构

## 代码特点

### 1. 模块化设计
- 网络、会话、存储、HTTP 与工具模块解耦
- 目录级 SQLite 数据库，方便隔离与扩展

### 2. 异步 I/O
- 基于 Boost.Asio/Beast 的非阻塞模型
- TCP 与 HTTP 并行处理

### 3. 健壮性
- 包头校验与消息长度限制
- 自动创建目录与数据库
- 统一会话关闭与资源释放

### 4. 性能优化
- 文件 CRC 去重避免重复写入
- 分片上传支持大文件
- 预计算 CRC32 表提升校验性能

## 核心实现逻辑

### 1. 服务器启动流程
```
main()
   → 解析 gflags 参数
   → 初始化 CRC32 表
   → 创建 io_context
   → 创建 TcpServer / UdpBroadcaster / HttpServer
   → 启动 UDP 广播
   → 启动 TCP/HTTP 监听
   → 运行 io_context
```

### 2. TCP 客户端连接流程
```
客户端连接
   → TcpServer 接受连接
   → 创建 Session
   → 读取包头（24 字节 PkgHead）
   → 按 PackageLen 读取消息体
   → 解析 MsgPkg
   → 分发到对应处理器
```

### 3. 设备注册流程
```
收到 CSReqDeviceInfo
   → 提取 DeviceID 与 Path（移除前导 /）
   → 构建完整路径 (ROOT/Path)
   → 创建目录
   → 打开/创建 index.db
   → 返回 CSResDeviceInfo
```

### 4. 照片同步流程
```
收到 CSReqSyncPhoto
   → 校验设备是否已注册
   → 生成文件路径并按 Offset 写入
   → Offset == 0 且 CRC 一致 → 跳过写入
   → 最后一片写入后计算整文件 CRC
   → 更新数据库并返回 CSResSyncPhoto
```

### 5. HTTP 查询流程
```
GET /clients
   → 从连接管理器读取连接与进度
   → 返回 JSON
```

## 技术亮点

1. **智能去重**：首片 CRC 与数据库一致时直接返回成功
2. **分片支持**：支持大文件分片传输
3. **状态可视化**：HTTP `/clients` 输出实时连接与进度
4. **自动化目录管理**：自动创建多级目录和数据库
5. **可配置性**：环境变量与 gflags 结合
6. **跨平台**：支持 Linux、macOS 与 Windows（含 vcpkg 配置）

## 编译要求

- CMake ≥ 3.15
- C++20 编译器（GCC/Clang/MSVC）
- Boost（filesystem/system/beast）
- Protobuf（protoc + libprotobuf）
- SQLite3
- Zlib
- gflags
- macOS 需要 absl（由 CMake 查找）
- Python 3.x（生成 `msgid.proto`）

## 下一步可扩展方向

1. **安全性**
    - TLS/SSL 加密
    - 客户端鉴权
    - 访问控制列表

2. **可靠性**
    - 断点续传 token
    - 更强的哈希算法（SHA256）
    - 文件写入锁

3. **性能**
    - 写入队列优化
    - 并发控制
    - 内存池管理

4. **监控**
    - Prometheus 指标
    - 日志分级
    - 性能统计

## 测试状态

- ⚠️ 单元测试已在 `tests/` 中配置（`ENABLE_TESTS`），尚未执行
- ⚠️ 依赖库安装后建议进行完整构建与集成测试

## 总结

项目已具备完整的 UDP 广播、TCP 同步、SQLite 存储与 HTTP 状态查询能力，代码结构清晰、模块化良好，可在安装依赖后直接构建运行。
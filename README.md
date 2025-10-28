## 需求
你是一位精通C++的开发人员，有着多年的架构设计能力
项目使用：使用boost网络库，protobuf协议库
协议在proto文件夹里面中

功能点：
1. udp每秒广播一次CSNtyServerInfo协议，发送到局域网中服务器的信息
2. 客户端使用tcp连接上来，先请求CSReqDeviceInfo存储路径和客户端ID
   存储路径需要使用连接方式，比如服务器路径是xxx,连接客户端上来的路径
3. 然后接受发送图片信息，保存到对应的路径中
优化点：
1. xxx路径下所有的子目录都有一个sqllite数据库,保存着当前目录下面的图片Crc32值，当客户端请求CSReqSyncPhoto到服务器的时候，
如果文件存在，先判断hash值是否一致，不一致才写入新图片，一致直接返回成功




## 设计

- **总体架构**：
  - **UDP 广播服务**：每秒向配置的广播地址+端口发送一次 `CSNtyServerInfo`，内容包含 `TcpPort/Name/RootPath/OS`，客户端可通过监听发现服务器。
  - **TCP 会话服务**：客户端连上后，使用“长度前缀+protobuf”的消息帧格式进行交互，先发送 `CSReqDeviceInfo` 完成路径与设备初始化，然后循环发送 `CSReqSyncPhoto` 进行图片同步。
  - **存储组织**：服务器根目录为 `LPS_ROOT`（默认 `./data`），实际写入路径为 `LPS_ROOT/客户端上送的Path`，自动创建多级目录。每个目录有一个 `index.db`（SQLite）表 `photos(filename TEXT PRIMARY KEY, crc32 INTEGER)` 用于记录文件完整内容的 CRC32。

- **消息帧协议（TCP）**：
  - 采用简洁 framing：`[包头每个字段都采用网络字节顺序]   [protobuf序列化的MsgPkg字节流]`。
  - 服务器处理后按相同 framing 回发 `MsgPkg` 响应（例如 `CSResDeviceInfo`、`CSResSyncPhoto`）。

- **消息语义**：
  - `CSReqDeviceInfo`：
    - 字段：`DeviceID`、`Path`
    - 效果：会话与该设备和保存目录绑定，确保 `LPS_ROOT/Path` 存在并为该目录打开（或创建）`index.db`。
    - 响应：`CSResDeviceInfo`（`ResultID=0` 表示成功）。
  - `CSReqSyncPhoto`：
    - 字段：`Filename`、`Size`、`Offset`、`Timestamp`、`HasNextPkt`、`Data(bytes)`、`Crc32`
    - 语义：支持分片顺序写入。服务器在 `Offset` 位置写入 `Data`。当 `HasNextPkt=false` 时认为是最后一片，随后计算整文件 CRC32 并写入 `index.db`。
    - 快速跳过：当文件不存在或 CRC 不一致将执行写入；当客户端以单包全量发送并且 CRC 与 `index.db` 已记录值一致时，直接返回成功且不写入。
    - 响应：`CSResSyncPhoto`（`ResultID=0` 表示成功）。

- **CRC 策略**：
  - 分片级：服务器可根据请求中 `Crc32` 与片段数据计算值校验片段一致性（不一致会忽略该包）。
  - 文件级：在最后一片写入完成后重新遍历整文件计算 CRC32，写入/更新 `index.db`。

- **并发与健壮性**：
  - 基于 Boost.Asio 的异步 I/O；每个 TCP 连接一个 `Session`。
  - 简化处理：同一文件的并发写入由客户端序列化保证；服务器端未实现跨会话的文件级锁，如有需要可后续加入文件锁或集中写入队列。
  - 错误处理：解析失败、帧长异常、I/O 错误会关闭连接；SQLite 初始化失败会话会拒绝写入。

- **可配置项（环境变量）**：
  - `LPS_TCP_PORT`：TCP 端口，默认 `9000`
  - `LPS_UDP_PORT`：UDP 广播端口，默认 `9001`
  - `LPS_NAME`：服务器名称，默认 `LPS-Server`
  - `LPS_ROOT`：根目录，默认 `./data`
  - `LPS_OS`：操作系统信息字符串，默认 `linux`
  - `LPS_BROADCAST`：广播地址，默认 `255.255.255.255`（请按需改为 `255.255.255.255` 或网段广播地址）

- **目录与重要文件**：
  - `proto/csmsg.proto`：主协议，包含 `MsgPkg` 与业务消息
  - `proto/gen_msgid.py`：按 `MsgPkg.oneof MsgBody` 自动生成 `msgid.proto`
  - `CMakeLists.txt`：构建配置，自动生成 protobuf C++ 代码
  - `src/main.cpp`：服务器主实现


## 构建与运行

- **依赖**：
  - CMake ≥ 3.15
  - C++17 编译器
  - Boost（system）
  - Protobuf（protoc + libprotobuf）
  - SQLite3
  - zlib

- **构建步骤**：
  - Linux/macOS：
    - `mkdir build && cd build`
    - `cmake ..`
    - `cmake --build . -j`（会自动生成 `proto/msgid.proto` 并编译生成 `lps_server`）

- **运行**：
  - 设置需要的环境变量（可选），然后执行：
    - `./lps_server`
  - 服务器会：
    - 每秒通过 UDP 向 `LPS_BROADCAST:LPS_UDP_PORT` 发送一次 `CSNtyServerInfo`（protobuf 序列化的 `MsgPkg`）
    - 在 `LPS_TCP_PORT` 监听 TCP 连接

- **客户端交互示例（伪代码）**：
  - 建立 TCP 连接后，发送帧：`[len][MsgPkg{CSReqDeviceInfo}]`
  - 接收 `CSResDeviceInfo` 成功后，按文件片发送若干 `CSReqSyncPhoto`：
    - 第一片：`Offset=0, Data=bytes[0:n], HasNextPkt=true`
    - 中间片：`Offset=累计偏移, HasNextPkt=true`
    - 最后一片：`HasNextPkt=false`；服务器计算整文件 CRC 并写入 `index.db`
  - 每包前均需加 4 字节大端长度前缀


## 代码结构拆分

- **模块化文件**：
  - `src/config.hpp`：读取环境变量、集中管理 `ServerConfig`。
  - `src/framing.hpp`：TCP 长度前缀帧编解码（`[4字节大端长度][protobuf payload]`）。
  - `src/crc.hpp,src/crc.cpp`：CRC32 工具，包含字节片段与整文件计算。
  - `src/sqlite_db.hpp,src/sqlite_db.cpp`：SQLite 访问层，维护 `index.db` 与 `photos(filename, crc32)`。
  - `src/session.hpp,src/session.cpp`：TCP 会话逻辑，处理 `CSReqDeviceInfo`、`CSReqSyncPhoto`，文件分片写入、CRC 校验与响应。
  - `src/tcp_server.hpp,src/tcp_server.cpp`：TCP 监听与连接接受，创建 `Session`。
  - `src/udp_broadcaster.hpp,src/udp_broadcaster.cpp`：UDP 每秒广播 `CSNtyServerInfo`。
  - `src/main.cpp`：进程入口，仅组装 `io_context`、`UdpBroadcaster`、`TcpServer` 与配置。

- **CMake 改动**：
  - `CMakeLists.txt` 已将上述 `.cpp` 文件纳入 `lps_server` 目标，`src` 目录作为头文件包含路径。

- **好处**：
  - **职责清晰**：网络、会话、存储、工具分离，便于定位问题与扩展。
  - **易于测试**：`SqliteCrcDB`、`framing`、`crc` 可单独做单元测试。
  - **更易维护**：降低单文件复杂度，符合工程化最佳实践。


## 后续可扩展点

- **安全与鉴权**：TLS/证书校验、鉴权令牌、ACL
- **断点续传/一致性**：引入文件级会话 token 与 MD5/SHA256 校验，提升一致性
- **并发写保护**：跨连接文件写入互斥、写入队列
- **清理与巡检**：定期扫描与 DB 校验，移除孤儿记录
- **监控**：Prometheus 指标与日志分级

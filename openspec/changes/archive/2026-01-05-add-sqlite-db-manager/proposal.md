# 变更提案：add-sqlite-db-manager

**变更ID**: `add-sqlite-db-manager`  
**状态**: 待审核  
**创建时间**: 2025-01-05  
**类型**: 架构改进

---

## 执行摘要

目前，每个 `Session`（TCP连接）都创建并拥有自己的 `SqliteCrcDB` 实例。这导致**多个Session同时写入同一个 `index.db` 文件时产生竞态条件**。

本提案引入**集中式SQLite数据库管理器**，实现：
1. 按同步目录对数据库连接进行池化和共享
2. 利用单线程事件循环实现自然序列化（无需互斥锁）
3. 通过引用计数管理连接生命周期
4. 简化Session初始化，消除重复的DB句柄

**影响**: 支持多个客户端安全地同时上传照片到同一个目录；为未来多线程部署做好架构准备。

---

## 问题陈述

### 当前行为
- 每个 `Session::db_` 都打开一个独立的 `SqliteCrcDB` 连接
- Session之间没有同步机制
- 两个Session上传文件到 `data/client-A/` 时，SQLite数据库可能出现损坏
- 难以追踪或关闭过时的连接（与Session生命周期耦合）

### 根本原因
数据库管理与Session生命周期紧密耦合；不存在协调多Session访问的抽象层。

### 为什么重要
- **数据完整性**: SQLite在数据库锁定级别是线程安全的，但多个独立连接绕过了这种保护
- **可扩展性**: 目前是单线程，但阻止了未来异步/多线程重构
- **运维安全**: 没有显式的连接池或清理策略

---

## 建议方案

### 1. 新组件：`SqliteDBManager`
- **范围**: 应用级单例，管理所有目录级的DB连接
- **职责**:
  - 按目录路径缓存 `SqliteCrcDB` 实例（每目录一个）
  - 在首次使用时延迟加载连接
  - 通过引用计数实现安全清理（最后一个Session释放时关闭）
- **线程安全**: 不需要（Boost.Asio单线程事件循环保证序列化）

### 2. 为什么不需要互斥锁？
**当前架构**: 单线程（Boost.Asio事件循环）
- 所有Session回调在同一个线程中顺序执行
- 没有真正的并发；操作一次执行一个
- 共享数据库连接自动序列化

**按Session创建DB的问题**: 多个连接访问同一个文件
- 连接A和连接B都访问 `index.db`
- 即使在单线程模式下，多个连接也可能导致问题

**使用Manager的方案**: 每个目录一个连接
- 所有Session共享相同的 `SqliteCrcDB` 实例
- 单连接 + 单线程 = 天然序列化
- 事件循环自动确保操作安全（无互斥锁开销）

### 3. 实现模式
- Session存储Manager引用（不拥有DB）
- `manager.acquire_db(path)` 初始化，`manager.release_db(path)` 清理
- 所有DB操作通过Manager（缓存、共享连接）
- 引用计数确保最后一个Session释放时进行清理

### 4. 零锁定开销
- 没有互斥锁 = 没有锁竞争
- 没有主锁 = 更简单、更安全的代码
- 简化的API相比基于互斥锁的设计
- 迁移路径：未来需要多线程时再添加锁

---

## 实现范围

### 新文件
- `src/sqlite_db_manager.hpp` — 类定义及内联辅助函数
- `src/sqlite_db_manager.cpp` — 实现（延迟加载、引用计数、清理）

### 修改文件
- `src/session.hpp` — 移除 `db_` 成员；添加 `db_manager_` 引用
- `src/session.cpp` — 更新所有DB操作以调用Manager
- `src/service_container.hpp` — 实例化并拥有 `SqliteDBManager`
- `src/service_container.cpp` — 初始化Manager，注入到Session
- `src/main.cpp` — 无需修改（容器处理生命周期）

### 无需修改
- `src/sqlite_db.hpp/cpp` — 核心DB类不变
- 协议、帧格式、CRC逻辑 — 不受影响
- 配置 — 无需新增环境变量

---

## 风险与缓解

| 风险 | 影响 | 缓解措施 |
|------|------|--------|
| 资源泄漏（DB永不关闭） | 内存增长 | 引用计数 + Session断开时自动清理 |
| 将来迁移到多线程 | 需要添加互斥锁 | 设计支持透明添加互斥锁，无需修改API |
| 高吞吐量时的连接竞争 | 性能下降 | 可后续通过连接池或WAL模式解决 |

---

## Why

多Session写入同一目录的场景是LocalPhotoSync的核心用例。目前的per-session DB连接模型无法安全处理这种情况，容易导致数据损坏。通过集中管理连接并利用单线程事件循环的特性，我们可以在无需复杂锁机制的情况下解决这个问题。

---

## What Changes

**新增文件:**
- `src/sqlite_db_manager.hpp` - 数据库管理器接口
- `src/sqlite_db_manager.cpp` - 数据库管理器实现
- `tests/test_sqlite_db_manager.cpp` - 单元测试

**修改文件:**
- `src/session.hpp` - 替换db_成员为db_manager_引用
- `src/session.cpp` - 更新构造/析构，委托DB操作到manager
- `src/service_container.hpp/cpp` - 添加GetDBManager()方法
- `src/tcp_server.cpp` - Session创建时传递db_manager

**未修改:**
- SQLite表结构、CRC32算法、消息协议保持不变
- 外部API稳定，客户端连接行为不变

---

## 成功标准
1. ✅ 两个Session写入同一目录时无数据损坏
2. ✅ Manager单元测试通过（覆盖率>90%）
3. ✅ 集成测试验证多Session并发写入
4. ✅ 单客户端工作负载无性能回退
5. ✅ 代码通过 `clang-tidy` 和 `clang-format` 检查

---

## 批准门槛
**实现前必须获得本提案的审核和批准。**

架构、并发和核心数据管理的变更需要评审。

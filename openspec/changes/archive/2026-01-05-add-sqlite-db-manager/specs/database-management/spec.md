# 规范：数据库连接管理

**能力ID**: `database-management`  
**变更ID**: `add-sqlite-db-manager`  
**状态**: 新增

---

## ADDED

本变更引入数据库连接管理能力，包括：

- `SqliteDBManager` 类：集中管理SQLite数据库连接
  - 提供 `acquire_db(path)` 和 `release_db(path)` 方法
  - 实现基于引用计数的生命周期管理
  - 提供便利方法 `get_file_crc()`、`set_file_crc()`、`delete_file()`

- Session改进：
  - 现在通过 `SqliteDBManager` 访问数据库
  - 自动在Session初始化和销毁时管理引用计数
  - 无需显式调用 `db_.open()` 或 `db_.close()`

---

## 概览

**数据库连接管理**能力定义了多个并发session如何安全地共享SQLite数据库连接而不造成数据损坏。它引入一个集中的管理器，通过单一共享连接模型对per-directory数据库进行池化。

**关键洞察**：单线程事件循环（Boost.Asio）自然序列化操作。每目录一个共享连接足够且比per-session连接更安全。

---

## 新增需求

### 需求：集中式DB连接管理器
**标题**：SqliteDBManager单例管理所有数据库连接

**描述**：
一个单独的 `SqliteDBManager` 实例（由 `ServiceContainer` 拥有）管理所有 `SqliteCrcDB` 连接。Session不直接创建或拥有数据库实例。每个唯一目录路径存在一个共享连接。

**详情**：
- **缓存策略**：每个唯一目录路径一个 `SqliteCrcDB` 实例
- **生命周期**：首次session访问时延迟加载；最后session释放时关闭
- **线程安全**：不需要（单线程Boost.Asio事件循环保证序列化）
- **职责**：
  - 延迟加载和缓存 `SqliteCrcDB` 实例
  - 实现引用计数以安全清理
  - 为DB操作提供便利访问方法

**新增需求1：集中式数据库连接管理器**

**场景1：单个Session访问数据库**
- Session 1 调用 `manager.set_file_crc("data/client-A/index.db", "photo.jpg", 0x1234)`
- Manager首次调用时加载DB并缓存
- 操作使用共享连接执行
- ✓ 结果：文件CRC存储，连接在后续调用时复用

**场景2：多个Session访问同一目录**
- Session 1 调用 `manager.set_file_crc("data/client-A/index.db", "photo1.jpg", 0xAAAA)`
- Session 2在事件循环中排队，调用 `manager.set_file_crc("data/client-A/index.db", "photo2.jpg", 0xBBBB)`
- 两者都使用相同的共享 `SqliteCrcDB` 连接
- 事件循环顺序执行：Session 1完成，然后Session 2
- ✓ 结果：两个操作安全完成；事务不冲突

**场景3：多个Session访问不同目录**
- Session 1 调用 `manager.set_file_crc("data/client-A/index.db", "photo.jpg", 0xAAAA)`
- Session 3并发调用 `manager.set_file_crc("data/client-B/index.db", "photo.jpg", 0xBBBB)`
- 不同DB实例；独立连接
- 事件循环同时分发两个操作；它们使用不同的DB连接
- ✓ 结果：真正并行；目录间无阻塞

---

### 新增需求2：引用计数与清理
**场景4：引用计数生命周期**
- 服务器启动：refcount["data/client-A"] = 0
- Session 1连接，发送CSReqDeviceInfo("data/client-A")：refcount = 1，DB打开
- Session 2连接到同路径：refcount = 2，DB复用
- Session 1断开连接：refcount = 1（为Session 2保持DB打开）
- Session 2断开连接：refcount = 0（DB关闭，缓存项删除）
- ✓ 结果：显式连接生命周期管理；无资源泄漏

---

## 修改的需求

### 需求：Session数据库集成
**场景5：Session使用Manager初始化**
- Session构造函数接收 `std::shared_ptr<SqliteDBManager> manager`
- Session将引用存储为 `db_manager_`
- 收到CSReqDeviceInfo("client-A", "photos")时：Session通过 `manager->acquire_db(path)` 获取DB
- 引用计数递增；如未缓存则打开DB
- ✓ 结果：Manager控制DB生命周期；session通过refcount获取/释放

**场景6：Session销毁与清理**
- Session 1为"data/client-A"获取了DB
- Session 1析构函数调用：`db_manager_->release_db(db_path_)`
- Manager递减"data/client-A"的引用计数
- 如果refcount达到0，manager关闭DB并删除缓存项
- ✓ 结果：自动清理；对session代码透明

---

## 保持不变

### 数据库操作
- `get_file_crc(filename, &crc32)` - 从photos表查询文件CRC
- `set_file_crc(filename, crc32)` - 插入/更新文件CRC
- `delete_file(filename)` - 删除文件记录
- **语义和返回值保持不变**

### CRC32重复数据删除逻辑
- 文件级CRC检查仍防止重复文件上传
- 块级CRC验证仍检测数据损坏
- **无重复数据删除策略或逻辑变化**

### 消息协议
- `CSReqSyncPhoto`、`CSReqDeviceInfo`、响应保持不变
- Manager是内部基础设施；对客户端不可见

---


**场景**：

#### 场景5：Session用Manager初始化
- Session构造函数接收 `std::shared_ptr<SqliteDBManager> manager`
- Session存储引用为 `db_manager_`
- 收到CSReqDeviceInfo("client-A", "photos")时：Session获取DB via `manager->acquire_db(path)`
- 引用计数递增；DB如未缓存则打开
- ✓ 结果：Manager控制DB生命周期；session通过refcount获取/释放

#### 场景6：Session销毁和清理
- Session 1为"data/client-A"获取了DB
- Session 1析构函数调用：`db_manager_->release_db(db_path_)`
- Manager递减"data/client-A"的引用计数
- 如引用计数达0，manager关闭DB并删除缓存项
- ✓ 结果：自动清理；对session代码透明

---

## 设计理由：单线程模式无锁

### 为什么单线程足够

1. **Boost.Asio事件循环**：所有session回调在单个 `io_context` 线程上顺序运行
   - 无真正并发：无抢占、无同步执行
   - 操作从DB角度看是原子的

2. **每目录单一共享连接**：
   - 而非每个Session打开自己的连接
   - 所有Session对给定目录共享一个 `SqliteCrcDB` 实例
   - 消除多连接问题

3. **结果：自然序列化**
   ```
   时间线：
     Session 1 调用：manager.set_file_crc("data/client-A", "photo1.jpg", 0xAAAA)
       → 同步执行到完成
       → DB写入已提交
       ↓
     Session 2 调用：manager.set_file_crc("data/client-A", "photo2.jpg", 0xBBBB)
       → 等待事件循环调用其处理函数
       → 无重叠；清晰事务边界
       ↓
       结果：两个写入成功，无损坏 ✓
   ```

### 为什么旧设计失败

```
Session 1: SqliteCrcDB db1_;  (连接A到index.db)
Session 2: SqliteCrcDB db2_;  (连接B到index.db)

问题：
  - 连接A启动事务，写记录1
  - 连接B试图写记录2
  - 文件锁定 / WAL冲突可能
  - 潜在数据损坏
```

### 新设计修复

```
Manager：
  db_cache_["data/client-A"] = shared_ptr<SqliteCrcDB>  (一个连接)

Session 1: manager.set_file_crc("data/client-A", ...) 
  → 使用共享连接

Session 2: manager.set_file_crc("data/client-A", ...)
  → 使用相同连接（排队在Session 1后）

结果：顺序事务，零冲突 ✓
```

---

## 约束与注意事项

- **单线程部署假设**：Boost.Asio事件循环确保顺序执行
- **无WAL需求**：标准SQLite模式在共享连接模型下足够
- **无分布式事务**：Manager不支持跨目录原子操作（超出范围）
- **错误传播**：DB错误（打开失败、查询失败）传播到Session；无自动重试
- **迁移友好**：设计支持透明添加锁而无API变化

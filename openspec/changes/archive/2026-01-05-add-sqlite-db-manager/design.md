# 技术设计：add-sqlite-db-manager

**变更ID**: `add-sqlite-db-manager`

---

## 架构概览

```
ServiceContainer（单例）
    └─ SqliteDBManager
        ├─ 缓存映射：<目录路径 → SqliteCrcDB>
        └─ 引用计数：<目录路径 → int>

Session 1 ──┐
Session 2 ──┼──> SqliteDBManager.get_file_crc("data/client-A", "photo1.jpg")
Session 3 ──┘     ↓
           从缓存获取共享DB实例
           查询DB（单连接，线程安全）
           返回结果

注意：单线程事件循环（Boost.Asio）确保操作自然序列化。无需互斥锁。
```

---

## 类设计

### SqliteDBManager

```cpp
class SqliteDBManager {
private:
    // 缓存：路径 → db实例
    // 每个目录一个连接，由所有session共享
    std::unordered_map<std::string, std::shared_ptr<SqliteCrcDB>> db_cache_;
    
    // 引用计数：路径 → 数量
    // 追踪当前有多少个session持有该DB的引用
    std::unordered_map<std::string, int> ref_counts_;
    
public:
    // 获取或创建目录的DB（缓存，共享）
    bool get_file_crc(const std::string& db_path, const std::string& filename, uint32_t& crc32);
    
    // 设置文件CRC（使用共享DB连接）
    bool set_file_crc(const std::string& db_path, const std::string& filename, uint32_t crc32);
    
    // 删除文件（使用共享DB连接）
    bool delete_file(const std::string& db_path, const std::string& filename);
    
    // 获取DB句柄的引用（递增引用计数）
    std::shared_ptr<SqliteCrcDB> acquire_db(const std::string& db_path);
    
    // 释放DB句柄（递减引用计数）
    void release_db(const std::string& db_path);
    
    // 清理所有连接
    void close_all();
    
private:
    // 延迟加载或获取缓存的DB
    std::shared_ptr<SqliteCrcDB> load_or_get(const std::string& db_path);
};
```

**关键设计决策**：无需互斥锁的原因：
1. **单线程事件循环**：Boost.Asio在一个线程上运行；无并发执行
2. **共享SQLite连接**：所有session对每个目录使用同一个连接
3. **自然序列化**：所有操作由事件循环排队并顺序执行
4. **SQLite事务安全**：单连接处理ACID保证

---

## 并发策略

### 为什么单线程足够？

**Boost.Asio事件循环（单线程）**
- 所有 `Session` 回调在一个线程中顺序运行
- 无真正的并发：无抢占、无同步执行
- 从DB视角看，所有操作是原子的

**共享SQLite连接模型**
- 而非每Session打开自己的连接
- 全部Session用同一个 `SqliteCrcDB` 实例
- 单连接保证ACID语义

**结果：自然序列化**
```
时间线：
  Session 1 调用：manager.set_file_crc("data/client-A", "photo1.jpg", 0xAAAA)
    → 同步执行到完成
    → DB写入已提交
    ↓
  Session 2 调用：manager.set_file_crc("data/client-A", "photo2.jpg", 0xBBBB)
    → 等待事件循环调用其处理函数
    → 无重叠；清晰的事务边界
    ↓
  结果：两个写入都成功，无损坏 ✓
```

### 为什么旧设计失败？

```
Session 1: SqliteCrcDB db1_;  (连接A到index.db)
Session 2: SqliteCrcDB db2_;  (连接B到index.db)

问题：
  - 连接A启动事务，写记录1
  - 连接B尝试写记录2
  - 文件锁定 / WAL冲突可能发生
  - 潜在数据损坏
```

### 新设计如何修复？

```
Manager：
  db_cache_["data/client-A"] = shared_ptr<SqliteCrcDB>  (一个连接)

Session 1: manager.set_file_crc("data/client-A", ...) 
  → 使用共享连接

Session 2: manager.set_file_crc("data/client-A", ...)
  → 使用相同的共享连接（排队在Session 1之后）

结果：顺序事务，零冲突 ✓
```

---

## 为什么现在不添加互斥锁？

**未来迁移路径**
- 如移至多线程模式（如线程池），添加互斥锁即可
- 无锁设计对单线程更简单、安全、快速
- 避免死锁风险和锁竞争开销

---

## 引用计数与清理

### 目的
追踪每个目录的数据库连接何时不再使用，以便安全关闭并释放资源。

### 实现

```cpp
private:
    std::unordered_map<std::string, int> ref_counts_;
    
public:
    // 由Session构造函数调用（收到CSReqDeviceInfo之后）
    std::shared_ptr<SqliteCrcDB> acquire_db(const std::string& db_path) {
        auto db = load_or_get(db_path);  // 若未缓存则加载
        ref_counts_[db_path]++;           // 递增引用计数
        return db;
    }
    
    // 由Session析构函数调用
    void release_db(const std::string& db_path) {
        ref_counts_[db_path]--;
        if (ref_counts_[db_path] == 0) {
            // 无session使用该DB；安全关闭
            db_cache_[db_path]->close();
            db_cache_.erase(db_path);
            ref_counts_.erase(db_path);
        }
    }
```

### 生命周期示例

```
refcount["data/client-A"] = 0  (初始空)

Session 1 连接：
  acquire_db("data/client-A")  → refcount = 1, DB打开

Session 2 连接（同路径）：
  acquire_db("data/client-A")  → refcount = 2, DB复用

Session 1 断开：
  release_db("data/client-A")  → refcount = 1 (DB仍打开)

Session 2 断开：
  release_db("data/client-A")  → refcount = 0 (DB关闭，缓存清除)
```

### 为什么有效？

- **内存安全**：数据库不会超过其最后使用者的生命周期
- **简洁**：无复杂清理逻辑；透明的生命周期
- **单线程**：无竞态条件（所有更新在事件循环中）

---

## Session集成

### 之前
```cpp
class Session {
private:
    SqliteCrcDB db_;  // 每个session拥有自己的DB
    
    void handle_sync_photo_request(...) {
        db_.get_file_crc(filename, crc);
        db_.set_file_crc(filename, new_crc);
    }
};
```

### 之后
```cpp
class Session {
private:
    std::shared_ptr<SqliteDBManager> db_manager_;
    std::string db_path_;  // 例如 "data/client-A/index.db"
    
    Session(..., std::shared_ptr<SqliteDBManager> manager) 
        : db_manager_(manager) {}
    
    void handle_device_info_request(...) {
        db_path_ = full_path + "/index.db";
        db_manager_->acquire_db(db_path_);  // 获取引用
    }
    
    void handle_sync_photo_request(...) {
        db_manager_->get_file_crc(db_path_, filename, crc);
        db_manager_->set_file_crc(db_path_, filename, new_crc);
    }
    
    ~Session() {
        if (!db_path_.empty()) {
            db_manager_->release_db(db_path_);  // 释放引用
        }
    }
};
```

---

## 错误处理

### DB加载失败
若 `SqliteCrcDB::open()` 失败：
- Manager记录错误，返回false
- Session行为：与今天相同（操作失败，连接可能关闭）
- 无级联失败或资源泄漏

### 设计简洁性优势
- 单线程 → 无死锁风险
- 无互斥锁 → 无锁序错误
- 无主锁 → 更少失败点
- RAII `shared_ptr` → 异常安全的清理

---

## 性能考虑

### 内存
- 每目录：1 × `SqliteCrcDB`（无重复）
- 相比per-session设计的节省：消除N个冗余连接
- 极小占用

### 延迟
- Manager查找：O(1)哈希表（忽略不计）
- DB操作：与之前相同（使用同一 `SqliteCrcDB` API）
- **无性能惩罚**

### 吞吐量
- **同目录、不同session**：现在更安全（单连接、序列化）
- **不同目录**：并行（独立连接）
- **总体**：比基于互斥锁的方案更简单、更快（无锁开销）

### 未来可扩展性
- 迁至thread-per-client？给manager加互斥锁，无其他改动
- 迁至线程池？同样模式：manager协调
- 当前设计对迁移友好

---

## 测试策略

### 单元测试 (test_sqlite_db_manager.cpp)
1. **初始化**：缓存启动时为空
2. **延迟加载**：首次调用目录打开DB并缓存
3. **缓存复用**：重复调用同路径返回缓存实例
4. **缓存隔离**：不同路径使用不同实例
5. **引用计数**：acquire/release时refcount正确更新
6. **清理**：refcount达0时关闭并删除缓存项
7. **重复acquire/release**：多session到同路径正确工作

### 集成测试
1. 启动服务器，连接两个TCP客户端到同步路径
2. 两个客户端依次上传文件（由事件循环序列化）
3. 验证：无SQLite错误，两个文件都在index.db，CRC值正确
4. 验证：文件物理存在，内容正确

### 为何无并发测试？
- 单线程事件循环意味着无真正的并发可测
- 操作由Boost.Asio自然序列化
- 标准单元测试足以验证缓存和引用计数逻辑

---

## 回滚与版本管理

### 恢复计划
1. 恢复 `src/session.hpp/cpp` 使用本地 `db_` 成员
2. 删除 `src/sqlite_db_manager.hpp/cpp`
3. 从 `ServiceContainer` 移除manager
4. 重新编译并测试（应该是机械的）

### 迁移风险：**低**
- Manager是增量式的；核心DB逻辑不变
- Session变更是本地的且可测试的
- 无协议或数据格式改变

---

## 未来增强（超出范围）

- **多线程支持**：当迁至线程池时添加per-directory互斥锁
- **连接池大小限制**：如果需要上限开放连接数
- **指标/监控**：追踪缓存命中、DB加载时间、活跃连接
- **WAL模式优化**：启用WAL以提高并发读性能（如需）

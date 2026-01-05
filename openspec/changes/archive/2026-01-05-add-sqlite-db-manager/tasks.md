# 实现任务：add-sqlite-db-manager

**变更ID**: `add-sqlite-db-manager`

---

## 第1阶段：设计与测试基础设施

- [ ] 阅读并批准 `design.md` 中的架构决策
- [ ] 创建 `tests/test_sqlite_db_manager.cpp` 测试套件
  - [ ] 测试：Manager初始化时缓存为空
  - [ ] 测试：相同目录的首次调用返回同一DB实例
  - [ ] 测试：不同目录有不同的DB实例
  - [ ] 测试：并发访问自动序列化（事件循环）
  - [ ] 测试：引用计数在无Session时关闭DB
  - [ ] 测试：同一目录的多个操作被序列化

---

## 第2阶段：实现SqliteDBManager

- [ ] 创建 `src/sqlite_db_manager.hpp`
  - [ ] 定义 `SqliteDBManager` 类及缓存映射
  - [ ] 定义引用计数数据结构
  - [ ] 声明公开方法：`acquire_db()`、`release_db()`、`close_all()`
  - [ ] 声明便利方法：`get_file_crc()`、`set_file_crc()`、`delete_file()`
  
- [ ] 创建 `src/sqlite_db_manager.cpp`
  - [ ] 实现延迟加载和缓存逻辑
  - [ ] 实现引用计数（递增/递减/清理）
  - [ ] 实现Manager析构时的安全清理
  - [ ] 无锁：Boost.Asio单线程保证自然序列化
  
- [ ] 运行单元测试；修复失败

---

## 第3阶段：重构Session集成

- [ ] 更新 `src/session.hpp`
  - [ ] 移除 `SqliteCrcDB db_;` 成员
  - [ ] 添加 `std::shared_ptr<SqliteDBManager> db_manager_;`
  - [ ] 更新所有使用DB的方法签名
  
- [ ] 更新 `src/session.cpp`
  - [ ] 构造函数：接收Manager引用，获取DB句柄
  - [ ] 析构函数：释放DB句柄
  - [ ] 将所有 `db_.get_file_crc()` 替换为 `db_manager_->get_file_crc()`
  - [ ] 将所有 `db_.set_file_crc()` 替换为 `db_manager_->set_file_crc()`
  - [ ] 将所有 `db_.delete_file()` 替换为 `db_manager_->delete_file()`
  - [ ] 验证所有读写操作都通过Manager进行

- [ ] 更新 `src/service_container.hpp`
  - [ ] 添加 `std::shared_ptr<SqliteDBManager> db_manager_;` 成员
  - [ ] 声明 `get_db_manager()` 访问器
  
- [ ] 更新 `src/service_container.cpp`
  - [ ] 在构造函数中初始化 `db_manager_`
  - [ ] 在析构函数或关闭时调用 `db_manager_->close_all()`
  - [ ] 将 `db_manager_` 引用传递给每个新 `Session`
  - [ ] 示例：`Session(..., container->get_db_manager())`

---

## 第4阶段：验证编译与测试

- [ ] 构建并验证编译（无错误/警告）
  - [ ] `mkdir -p build && cd build && cmake .. && make -j`
  
- [ ] 运行所有测试
  - [ ] `./build/tests/lps_unit_tests`
  - [ ] 验证新Manager单元测试通过
  - [ ] 验证所有现有测试无回退
  
- [ ] 代码覆盖率验证
  - [ ] 确保Manager覆盖率 >90%

---

## 第5阶段：代码质量与文档

- [ ] 用 `clang-format -i` 格式化代码
  - [ ] `clang-format -i src/sqlite_db_manager.hpp src/sqlite_db_manager.cpp`
  - [ ] `clang-format -i src/session.hpp src/session.cpp`
  - [ ] `clang-format -i src/service_container.hpp src/service_container.cpp`
  
- [ ] 运行静态分析
  - [ ] `clang-tidy src/sqlite_db_manager.cpp --checks=-*,readability-*,bugprone-*`
  - [ ] 修复所有警告
  
- [ ] 更新 [IMPLEMENTATION_SUMMARY.md](../../../../IMPLEMENTATION_SUMMARY.md) 添加新章节
  - [ ] 添加：\"数据库连接管理（`src/sqlite_db_manager.*`）\"
  - [ ] 描述Manager生命周期和引用计数
  
- [ ] 添加代码注释说明单线程序列化策略和引用计数

---

## 第5阶段：最终验证与签字

- [ ] 所有单元测试通过
- [ ] 代码覆盖率保持 >90%
- [ ] 无编译器警告
- [ ] Clang-tidy通过
- [ ] 准备就绪进行PR和部署

---

## 说明

- **预估工作量**: 1–2小时（简化设计，无互斥锁）
- **风险等级**: 低（简单连接池 + 引用计数）
- **评审重点**: 引用计数正确性、缓存命中/未命中逻辑
- **回滚计划**: 如发现问题，恢复 `session.hpp/cpp` 的改动并删除Manager文件
- **关键洞察**: 无需互斥锁，因为Boost.Asio单线程事件循环自然序列化操作

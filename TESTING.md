# LocalPhotoSync 单元测试 - 快速开始指南

## 📋 测试概览

已为 `src/` 目录中的关键类生成了完整的单元测试套件。

```
总测试数：24个 ✓
通过率：100%
覆盖的类：4个
执行时间：~30ms
```

## 🚀 快速运行

```bash
cd /home/pi/code/LocalPhotoSync
mkdir -p build && cd build
cmake ..
make lps_unit_tests
./tests/lps_unit_tests
```

## 📦 测试覆盖

### CRC32 类 (10个测试)
- ✓ 表初始化
- ✓ 数据计算（空、单字节、多字节、大数据）
- ✓ 文件计算
- ✓ 偏移和长度支持

### SqliteCrcDB 类 (8个测试)
- ✓ 数据库操作（打开、关闭、创建表）
- ✓ CRUD操作（增删改查）
- ✓ 错误处理

### CConMgr 类 (3个测试)
- ✓ 连接ID生成和唯一性
- ✓ JSON序列化

### CServiceContainer 类 (3个测试)
- ✓ 单例模式
- ✓ 依赖管理

## 📝 文件清单

- `tests/test_classes.cpp` - 所有单元测试 (388行)
- `tests/CMakeLists.txt` - 测试构建配置
- `TEST_REPORT.md` - 详细测试报告
- `src/agents.md` - 工程准则与测试指南

## 🔍 高级选项

```bash
# 查看详细输出
./tests/lps_unit_tests --gtest_detail=all

# 运行特定的测试
./tests/lps_unit_tests --gtest_filter=CRC32Test.*

# 运行特定的测试用例
./tests/lps_unit_tests --gtest_filter=CRC32Test.CRC32EmptyData

# 列出所有可用的测试
./tests/lps_unit_tests --gtest_list_tests
```

## 💡 测试特点

- **独立性**：每个测试相互独立，顺序无关
- **快速**：整个套件在30毫秒内完成
- **可靠**：确定性结果，100%通过率
- **隔离**：自动化的初始化和清理
- **现代**：使用Google Test框架

## ✅ 持续集成

此测试套件已准备好集成到CI/CD流程：
- 自动化构建和测试
- 回归测试
- 代码质量保证

## 📚 相关文档

- `TEST_REPORT.md` - 完整的测试报告
- `/src/agents.md` - 代码准则和测试指南
- `/README.md` - 项目概述

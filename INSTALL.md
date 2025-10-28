# LocalPhotoSync 服务器安装指南

## 系统要求

- **操作系统**: Linux / macOS
- **编译器**: 支持C++17的编译器（GCC 7+, Clang 5+, 或更高版本）
- **CMake**: 版本 3.15 或更高
- **Python**: Python 3.x（用于生成protobuf定义）

## 依赖库安装

### macOS (使用 Homebrew)

```bash
# 安装Homebrew (如果还没有安装)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# 安装依赖
brew install cmake
brew install boost
brew install protobuf
brew install sqlite3
brew install zlib
```

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    libboost-system-dev \
    libprotobuf-dev \
    protobuf-compiler \
    libsqlite3-dev \
    zlib1g-dev \
    python3
```

### CentOS/RHEL

```bash
sudo yum install -y \
    gcc-c++ \
    cmake \
    boost-devel \
    protobuf-devel \
    protobuf-compiler \
    sqlite-devel \
    zlib-devel \
    python3
```

### Fedora

```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    boost-devel \
    protobuf-devel \
    protobuf-compiler \
    sqlite-devel \
    zlib-devel \
    python3
```

## 构建步骤

### 方法一：使用构建脚本（推荐）

```bash
# 赋予执行权限
chmod +x build.sh

# 执行构建
./build.sh
```

### 方法二：手动构建

```bash
# 1. 生成msgid.proto
cd proto
bash gen.sh
cd ..

# 2. 创建构建目录
mkdir -p build
cd build

# 3. 运行CMake配置
cmake ..

# 4. 编译
cmake --build . -j$(nproc)

# 5. 返回项目根目录
cd ..
```

## 构建输出

成功构建后，可执行文件将位于：
- `build/lps_server` - 主服务器程序

## 运行服务器

### 使用默认配置

```bash
cd build
./lps_server
```

### 使用自定义配置

```bash
# 设置环境变量
export LPS_TCP_PORT=9000        # TCP端口
export LPS_UDP_PORT=9001        # UDP广播端口
export LPS_NAME="MyPhotoServer" # 服务器名称
export LPS_ROOT="./my_photos"   # 存储根目录
export LPS_OS="macOS"           # 操作系统标识
export LPS_BROADCAST="255.255.255.255"  # 广播地址

# 运行服务器
cd build
./lps_server
```

## 配置说明

| 环境变量 | 默认值 | 说明 |
|---------|--------|------|
| LPS_TCP_PORT | 9000 | TCP服务端口 |
| LPS_UDP_PORT | 9001 | UDP广播端口 |
| LPS_NAME | LPS-Server | 服务器名称 |
| LPS_ROOT | ./data | 照片存储根目录 |
| LPS_OS | linux | 操作系统标识 |
| LPS_BROADCAST | 255.255.255.255 | UDP广播地址 |

## 验证安装

运行服务器后，应该看到类似以下输出：

```
TCP Server listening on port 9000
Broadcast server info: LPS-Server (TCP:9000)
```

## 常见问题

### 1. CMake找不到Boost

**问题**: `Could not find Boost`

**解决方案**:
```bash
# macOS
brew install boost

# Ubuntu
sudo apt-get install libboost-all-dev
```

### 2. CMake找不到Protobuf

**问题**: `Could not find Protobuf`

**解决方案**:
```bash
# macOS
brew install protobuf

# Ubuntu
sudo apt-get install libprotobuf-dev protobuf-compiler
```

### 3. 编译错误：C++17支持

**问题**: 编译器不支持C++17

**解决方案**:
```bash
# 升级GCC
sudo apt-get install gcc-9 g++-9
export CXX=g++-9
```

### 4. SQLite3库找不到

**问题**: `Could not find SQLite3`

**解决方案**:
```bash
# macOS
brew install sqlite3

# Ubuntu
sudo apt-get install libsqlite3-dev
```

### 5. Python3未找到

**问题**: proto生成脚本无法运行

**解决方案**:
```bash
# Ubuntu
sudo apt-get install python3

# macOS
brew install python3
```

## 开发说明

### 清理构建

```bash
# 删除构建目录
rm -rf build

# 删除生成的proto文件
rm -f proto/msgid.proto
```

### 重新构建

```bash
# 完全重新构建
rm -rf build
./build.sh
```

### 调试模式构建

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . -j
```

### Release模式构建

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
```

## 文件结构

```
LocalPhotoSync/
├── build/              # 构建目录（自动生成）
├── proto/              # Protobuf协议定义
│   ├── csmsg.proto    # 主协议文件
│   ├── msgid.proto    # 自动生成的消息ID
│   ├── gen_msgid.py   # 生成脚本
│   └── gen.sh         # 生成脚本
├── src/                # 源代码
│   ├── main.cpp       # 主程序
│   ├── config.hpp     # 配置管理
│   ├── framing.hpp    # 帧协议
│   ├── crc.*          # CRC32计算
│   ├── sqlite_db.*    # SQLite数据库
│   ├── session.*      # TCP会话
│   ├── tcp_server.*   # TCP服务器
│   └── udp_broadcaster.* # UDP广播
├── CMakeLists.txt      # CMake配置
├── build.sh            # 构建脚本
├── README.md           # 项目说明
└── INSTALL.md          # 安装指南（本文件）
```

## 技术支持

如遇到其他问题，请查看：
1. README.md - 项目功能和设计说明
2. 检查所有依赖是否正确安装
3. 确认编译器支持C++17标准
4. 查看构建日志中的详细错误信息
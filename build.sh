#!/bin/bash


# 生成protobuf相关文件
cd proto
echo "Generating msgid.proto..."
bash gen.sh
cd ..


# 创建build目录
mkdir -p build
cd build

#生成提示
cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

# 运行cmake
echo "Running cmake..."
cmake ..

# 编译
echo "Building..."
cmake --build . -j

echo "Build completed successfully!"
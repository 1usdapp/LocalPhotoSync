#!/bin/bash

# 生成msgid.proto
python3 gen_msgid.py csmsg.proto msgid.proto

protoc --cpp_out=. *.proto
# 生成C++代码（由CMake处理，这里只是确保msgid.proto存在）
echo "msgid.proto generated successfully"

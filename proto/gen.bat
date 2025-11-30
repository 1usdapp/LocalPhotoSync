@echo off
REM 生成msgid.proto
python gen_msgid.py csmsg.proto msgid.proto

REM 生成C++代码
..\vcpkg\installed\x64-windows\tools\protobuf\protoc.exe --cpp_out=. *.proto
REM 生成C++代码（由CMake处理，这里只是确保msgid.proto存在）
echo msgid.proto generated successfully
@echo off 

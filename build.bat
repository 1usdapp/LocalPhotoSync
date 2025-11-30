@echo off
REM 生成protobuf相关文件
cd proto
echo Generating msgid.proto..
call gen.bat
cd ..

REM 创建build目录
mkdir build
cd build

REM 设置vcpkg工具路径
set VCPKG_ROOT=%~dp0vcpkg
set CMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake

REM 生成提示
cmake .. -DCMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN_FILE% -DCMAKE_EXPORT_COMPILE_COMMANDS=ON 

REM 运行cmake
echo Running cmake..
cmake .. -DCMAKE_TOOLCHAIN_FILE=%CMAKE_TOOLCHAIN_FILE%  -DVCPKG_TARGET_TRIPLET=x64-windows

REM 编译
echo Building..
cmake --build . --config Release

echo Build completed successfully!
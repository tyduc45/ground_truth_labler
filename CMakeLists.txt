# 1. 必须是第一行
cmake_minimum_required(VERSION 3.16)

# 2. 必须在所有 find_package 之前
project(GT_labler VERSION 1.0 LANGUAGES CXX)

# 3. 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. 开启 Qt 的自动处理（非常重要，处理 moc/uic/rcc）
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 5. 查找依赖包 
find_package(Qt6 REQUIRED COMPONENTS Widgets Gui Core)
find_package(OpenCV REQUIRED)


add_subdirectory(source)
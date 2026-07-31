cmake_minimum_required(VERSION 3.16)

# 设置项目名称
project(cdc LANGUAGES CXX)

# 指定 C++ 标准版本（C++17）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
if(MSVC)
    add_compile_options(
        /W4
        /EHsc
        /utf-8
    )
else()
    add_compile_options(
        -Wall
        -Wextra
        -pedantic
    )
endif()

# 包含头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/cdc
)

# 设置源文件
set(SOURCES
    src/address.cpp
    src/socket_ops.cpp
    src/protocol_strategy.cpp
    src/communicator.cpp
    src/communicator_factory.cpp
)

# 编译为静态库
add_library(cdc STATIC ${SOURCES})

# 链接依赖库（pthread 用于 thread_local 安全）
target_link_libraries(cdc
    PUBLIC
    pthread
)
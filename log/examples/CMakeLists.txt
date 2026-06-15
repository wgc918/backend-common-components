cmake_minimum_required(VERSION 3.16)

# 设置项目名称
project(log_examples LANGUAGES CXX)

# 指定 C++ 标准版本（C++17）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置编译选项
if(MSVC)
    # MSVC 编译器选项
    add_compile_options(
        /W4                     # 启用警告级别4
        #/WX                     # 将警告视为错误
        /EHsc                   # 启用标准异常处理
        /utf-8                  # 使用UTF-8编码
    )
else()
    # GCC/Clang 编译器选项
    add_compile_options(
        -Wall                   # 启用所有警告
        -Wextra                 # 启用额外警告
        #-Werror                 # 将警告视为错误
        -pedantic               # 启用严格标准检查
    )
endif()

# 包含头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}         
    ${PROJECT_SOURCE_DIR}/.. 
)

# 添加测试源文件
 set(TEST_SOURCES
    ${PROJECT_SOURCE_DIR}/../src/log.cpp
 )

# 添加测试目标
add_executable(example_multiThread example_multiThread.cpp ${TEST_SOURCES})
add_executable(example_singleThread example_singleThread.cpp ${TEST_SOURCES})
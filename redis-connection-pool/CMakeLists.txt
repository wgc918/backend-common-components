cmake_minimum_required(VERSION 3.16)

# 设置项目名称
project(redis_conn_pool LANGUAGES CXX)

# 指定 C++ 标准版本（C++17）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
if(MSVC)
    # MSVC 编译器选项
    add_compile_options(
        /W4                     # 启用警告级别4
        /EHsc                   # 启用标准异常处理
        /utf-8                  # 使用UTF-8编码
    )
else()
    # GCC/Clang 编译器选项
    add_compile_options(
        -Wall                   # 启用所有警告
        -Wextra                 # 启用额外警告
        -pedantic               # 启用严格标准检查
    )
endif()

# 包含头文件目录
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/include/redis-pool
)

# 设置源文件
set(SOURCES
    src/RedisConnectionPool.cpp
    src/RedisConnGuard.cpp
    src/RedisConnectionFactory.cpp
    src/RedisHealthChecker.cpp
)

# 编译为静态库
add_library(redis_conn_pool STATIC ${SOURCES})

# 链接依赖库
target_link_libraries(redis_conn_pool
    PUBLIC
    redis++
    hiredis
    pthread
)

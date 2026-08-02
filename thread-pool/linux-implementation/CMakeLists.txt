cmake_minimum_required(VERSION 3.14)
project(threadpool-linux VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pthread")

# 查找 pthread 库
find_package(Threads REQUIRED)

# ========== 线程池库 ==========
set(THREADPOOL_SOURCES
    src/threadPool.cpp
    src/TaskDispatcher.cpp
)

set(THREADPOOL_HEADERS
    include/task.h
    include/threadPool.h
    include/TaskDispatcher.h
    include/simple_logger.h
)

add_library(threadpool_linux STATIC ${THREADPOOL_SOURCES} ${THREADPOOL_HEADERS})
target_include_directories(threadpool_linux PUBLIC include)
target_link_libraries(threadpool_linux PUBLIC Threads::Threads)

# ========== 单元测试 ==========
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    enable_testing()

    add_executable(test_threadpool_linux
        tests/test_threadpool.cpp
    )

    target_link_libraries(test_threadpool_linux PRIVATE threadpool_linux)
    target_include_directories(test_threadpool_linux PRIVATE include)

    # 编译测试时启用日志
    target_compile_definitions(test_threadpool_linux PRIVATE ENABLE_LOGGING)

    add_test(NAME test_threadpool_linux COMMAND test_threadpool_linux)
endif()
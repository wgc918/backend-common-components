cmake_minimum_required(VERSION 3.16)

# 设置项目名称
project(rmq_pool_examples LANGUAGES CXX)

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
    ${CMAKE_SOURCE_DIR}/../../include
)

set(SOURCES
    ${PROJECT_SOURCE_DIR}/../../src/broker.cpp
    ${PROJECT_SOURCE_DIR}/../../src/channelPool.cpp
    ${PROJECT_SOURCE_DIR}/../../src/channelGuard.cpp
    ${PROJECT_SOURCE_DIR}/../../src/channel.cpp
    ${PROJECT_SOURCE_DIR}/../../src/connection.cpp
    ${PROJECT_SOURCE_DIR}/../../src/consumer.cpp
    ${PROJECT_SOURCE_DIR}/../../src/exchange.cpp
    ${PROJECT_SOURCE_DIR}/../../src/message.cpp
    ${PROJECT_SOURCE_DIR}/../../src/producer.cpp
    ${PROJECT_SOURCE_DIR}/../../src/queue.cpp
)

# 添加示例目标
add_executable(consumer_beijing consumer_beijing.cpp ${SOURCES})
add_executable(consumer_delivering consumer_delivering.cpp ${SOURCES})
add_executable(producer producer.cpp ${SOURCES})


target_link_libraries(consumer_beijing rabbitmq)
target_link_libraries(consumer_delivering rabbitmq)
target_link_libraries(producer rabbitmq)
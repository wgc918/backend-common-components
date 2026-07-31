cmake_minimum_required(VERSION 3.16)

project(cdc_examples LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /EHsc /utf-8)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

include_directories(
    ${PROJECT_SOURCE_DIR}/../include
    ${PROJECT_SOURCE_DIR}/../include/cdc
)

# 添加库源文件（直接编译，不依赖静态库）
set(CDC_SOURCES
    ${PROJECT_SOURCE_DIR}/../src/address.cpp
    ${PROJECT_SOURCE_DIR}/../src/socket_ops.cpp
    ${PROJECT_SOURCE_DIR}/../src/protocol_strategy.cpp
    ${PROJECT_SOURCE_DIR}/../src/communicator.cpp
    ${PROJECT_SOURCE_DIR}/../src/communicator_factory.cpp
)

add_executable(example_echo example_echo.cpp ${CDC_SOURCES})
target_link_libraries(example_echo pthread)
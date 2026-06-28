cmake_minimum_required(VERSION 3.16)

# 设置项目名称
project(database_conn_pool_examples LANGUAGES CXX)

# 指定 C++ 标准版本（C++17）
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
#set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
    ${PROJECT_SOURCE_DIR}/../src/MySQLConnection.cpp
    ${PROJECT_SOURCE_DIR}/../src/MySQLFactory.cpp
    ${PROJECT_SOURCE_DIR}/../src/MySQLResultSet.cpp
    ${PROJECT_SOURCE_DIR}/../src/pooledConnGuard.cpp
    ${PROJECT_SOURCE_DIR}/../src/ConnectionPool.cpp
 )

# 添加测试目标
add_executable(test test.cpp)
add_executable(test_mysql test_mysql.cpp ${TEST_SOURCES})
add_executable(test_mysql_factory test_mysql_factory.cpp ${TEST_SOURCES})
add_executable(test_pool test_pool.cpp ${TEST_SOURCES})

target_link_libraries(test mysqlcppconn)
target_link_libraries(test_mysql mysqlcppconn)
target_link_libraries(test_mysql_factory mysqlcppconn)
target_link_libraries(test_pool mysqlcppconn)

# 创建运行所有测试的自定义目标
add_custom_target(run_all
    COMMAND echo "=== Running basic tests ===" && ${CMAKE_BINARY_DIR}/test
    COMMAND echo "=== Running MySQL tests ===" && ${CMAKE_BINARY_DIR}/test_mysql
    COMMAND echo "=== Running MySQL factory tests ===" && ${CMAKE_BINARY_DIR}/test_mysql_factory
    COMMAND echo "=== Running connection pool tests ===" && ${CMAKE_BINARY_DIR}/test_pool
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Running all test executables..."
    VERBATIM
)
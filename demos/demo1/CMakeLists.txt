cmake_minimum_required(VERSION 3.16)
project(demo1-inventory VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# ============================================================
# 项目内部组件源码 — 直接编译到 Demo 中
# ============================================================
set(PROJECT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../..)

# INI 解析器
set(INI_SRC
    ${PROJECT_ROOT}/ini/src/ini.cpp
    ${PROJECT_ROOT}/ini/src/value.cpp
)

# 日志模块
set(LOG_SRC
    ${PROJECT_ROOT}/log/src/log.cpp
)

# 错误处理框架
set(ERROR_SRC
    ${PROJECT_ROOT}/error/src/error.cpp
    ${PROJECT_ROOT}/error/src/error_kind.cpp
)

# 数据库连接池
set(DB_POOL_SRC
    ${PROJECT_ROOT}/database-connection-pool/src/ConnectionPool.cpp
    ${PROJECT_ROOT}/database-connection-pool/src/MySQLConnection.cpp
    ${PROJECT_ROOT}/database-connection-pool/src/MySQLFactory.cpp
    ${PROJECT_ROOT}/database-connection-pool/src/MySQLResultSet.cpp
    ${PROJECT_ROOT}/database-connection-pool/src/pooledConnGuard.cpp
)

# Redis 连接池
set(REDIS_POOL_SRC
    ${PROJECT_ROOT}/redis-connection-pool/src/RedisConnectionPool.cpp
    ${PROJECT_ROOT}/redis-connection-pool/src/RedisConnGuard.cpp
    ${PROJECT_ROOT}/redis-connection-pool/src/RedisConnectionFactory.cpp
    ${PROJECT_ROOT}/redis-connection-pool/src/RedisHealthChecker.cpp
)

# Demo 源码
set(DEMO_SRC
    src/main.cpp
    src/app.cpp
    src/bootstrap.cpp
    src/inventory_service.cpp
    src/inventory_controller.cpp
    src/concurrent_test.cpp
    src/consistency_checker.cpp
    src/redis_lua.cpp
)

# ============================================================
# 包含目录
# ============================================================
set(COMMON_INCLUDES
    ${PROJECT_ROOT}/ini/include
    ${PROJECT_ROOT}/log/include
    ${PROJECT_ROOT}/error/include
    ${PROJECT_ROOT}/database-connection-pool/include
    ${PROJECT_ROOT}/redis-connection-pool/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 手动设置Drogon路径
set(DROGON_INCLUDE_DIR "/usr/local/include")
set(DROGON_LIBRARY "/usr/local/lib/libdrogon.so")
set(TRANTOR_LIBRARY "/usr/local/lib/libtrantor.so")

# ============================================================
# 可执行目标
# ============================================================
add_executable(demo1_inventory
    ${DEMO_SRC}
    ${INI_SRC}
    ${LOG_SRC}
    ${ERROR_SRC}
    ${DB_POOL_SRC}
    ${REDIS_POOL_SRC}
)

target_include_directories(demo1_inventory PRIVATE
    ${COMMON_INCLUDES}
)

target_link_libraries(demo1_inventory
    redis++ 
    hiredis 
    pthread
    jsoncpp
    ssl
    crypto
    uuid
)

# 复制配置文件到构建目录
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.ini ${CMAKE_CURRENT_BINARY_DIR}/config.ini COPYONLY)
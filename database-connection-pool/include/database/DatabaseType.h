#pragma once

/// 支持的数据库类型枚举
/// 用于 ConnectionPool 按数据库类型区分单例实例
enum class DatabaseType
{
    MySQL,       // MySQL / MariaDB
    SQLite,      // SQLite
    PostgreSQL,  // PostgreSQL
    Oracle,      // Oracle
    SQLServer,   // Microsoft SQL Server
    MongoDB,     // MongoDB (NoSQL)
};
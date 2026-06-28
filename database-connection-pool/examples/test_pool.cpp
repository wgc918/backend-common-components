#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "../include/conn-pool/ConnectionPool.h"
#include "../include/conn-pool/pooledConnGuard.h"

std::mutex cout_mutex;

void func(PooledConnGuard mconn, int id)
{
    // 使用 id 构造表名，避免多线程冲突
    std::string table_name = "test_" + std::to_string(id);

    const std::string create_sql = "create table " + table_name + " (id int,name varchar(10));";
    mconn->execute(create_sql.c_str());

    // 使用 id 相关的数据
    const std::string insert_sql = "insert into " + table_name + " values(" + std::to_string(id) +
                                   ",'qwe" + std::to_string(id) + "')";
    mconn->execute_update(insert_sql);

    const std::string insert_sql_pre = "insert into " + table_name + " values(?,?);";
    mconn->prepare_statement(insert_sql_pre);
    mconn->set_int(1, id + 1);
    mconn->set_string(2, "qwe" + std::to_string(id + 1));
    mconn->execute_update();

    const std::string query_sql = "select * from " + table_name;
    IResultSet*       res       = mconn->execute_query(query_sql.c_str());

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[thread " << id << "] query all:" << std::endl;
        while (res->next())
        {
            std::cout << "[thread " << id << "] " << res->get_int("id") << " "
                      << res->get_string("name") << std::endl;
        }
        std::cout << "[thread " << id << "] ------------------------------------" << std::endl;
    }

    const std::string query_sql_pre = "select * from " + table_name + " where id=?";
    mconn->prepare_statement(query_sql_pre.c_str());
    mconn->set_int(1, id);
    IResultSet* res_pre = mconn->execute_query();

    {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "[thread " << id << "] query where id=" << id << ":" << std::endl;
        while (res_pre->next())
        {
            std::cout << "[thread " << id << "] " << res_pre->get_int("id") << " "
                      << res_pre->get_string("name") << std::endl;
        }
    }

    const std::string drop_sql = "drop table " + table_name + ";";
    mconn->execute(drop_sql.c_str());
}

int main()
{
    ConnectionConfig cfg;
    cfg.db_name   = "testdb";
    cfg.ip        = "127.0.0.1";
    cfg.port      = 3306;
    cfg.user_name = "wgc";
    cfg.password  = "123456";

    // 获取MySQL连接池单例
    auto& mysql_pool = ConnectionPool::instance(DatabaseType::MySQL);
    mysql_pool.init(cfg, 5, 20, 180, 10000, 4);

    std::vector<std::thread> vec(10);
    for (int i = 0; i < 10; i++)
    {
        vec[i] = std::thread(func, mysql_pool.get_connection(), i);
    }
    for (int i = 0; i < 10; i++)
    {
        vec[i].join();
    }

    std::this_thread::sleep_for(std::chrono::seconds(5));
    mysql_pool.shutdown();

    // // 获取连接（支持超时和自动扩容）
    // auto guard = mysql_pool.get_connection();
    // if (guard)
    // {  // 检查是否成功获取连接
    //     func(std::move(guard));
    // }
    // else
    // {
    //     // 获取连接超时，处理失败情况
    //     std::cout << "timeout" << std::endl;
    // }
    return 0;
}

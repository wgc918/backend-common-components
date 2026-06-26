#include "../include/database/MySQLFactory.h"

void func(IDatabaseConnection* mconn)
{
    const char* create_sql = "create table test1 (id int,name varchar(10));";
    mconn->execute(create_sql);

    const char* insert_sql = "insert into test1 values(1,'qwe1')";
    mconn->execute_update(insert_sql);

    const char* insert_sql_pre = "insert into test1 values(?,?);";
    mconn->prepare_statement(insert_sql_pre);
    mconn->set_int(1, 2);
    mconn->set_string(2, "qwe2");
    mconn->execute_update();

    const char* query_sql = "select * from test1";
    IResultSet* res       = mconn->execute_query(query_sql);
    while (res->next())
    {
        std::cout << res->get_int("id") << " " << res->get_string("name") << std::endl;
    }

    std::cout << "------------------------------------" << std::endl;
    const char* query_sql_pre = "select * from test1 where id=?";
    mconn->prepare_statement(query_sql_pre);
    mconn->set_int(1, 1);
    IResultSet* res_pre = mconn->execute_query();
    while (res_pre->next())
    {
        std::cout << res_pre->get_int("id") << " " << res_pre->get_string("name") << std::endl;
    }

    const char* drop_sql = "drop table test1;";
    mconn->execute(drop_sql);

    mconn->close();
}

int main()
{
    ConnectionConfig cfg;
    cfg.db_name   = "testdb";
    cfg.ip        = "127.0.0.1";
    cfg.port      = 3306;
    cfg.user_name = "wgc";
    cfg.password  = "123456";

    MySQLFactory     mfactory;
    MySQLConnection* conn = mfactory.create_connection(cfg);
    func(conn);
    delete conn;

    return 0;
}
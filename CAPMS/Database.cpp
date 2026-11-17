#include "Database.h"

#include <mysql_driver.h>

sql::Connection* connectDatabase()
{
    sql::mysql::MySQL_Driver* driver =
        sql::mysql::get_mysql_driver_instance();

    sql::Connection* con = driver->connect(
        "tcp://127.0.0.1:3306",
        "root",
        ""
    );

    con->setSchema("capms");

    return con;
}
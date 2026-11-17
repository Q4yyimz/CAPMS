#ifndef MENU_H
#define MENU_H

#include "UserSession.h"

#include <mysql_connection.h>

void displayAdminMenu(
    sql::Connection* con,
    const UserSession& session
);

#endif

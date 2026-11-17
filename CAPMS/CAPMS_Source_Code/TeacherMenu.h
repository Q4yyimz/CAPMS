#ifndef TEACHER_MENU_H
#define TEACHER_MENU_H

#include "UserSession.h"

#include <mysql_connection.h>

void displayTeacherMenu(
    sql::Connection* con,
    const UserSession& session
);

#endif

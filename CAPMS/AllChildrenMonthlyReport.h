#ifndef ALL_CHILDREN_MONTHLY_REPORT_H
#define ALL_CHILDREN_MONTHLY_REPORT_H

#include "UserSession.h"

#include <mysql_connection.h>

void allChildrenMonthlyPerformanceReport(
    sql::Connection* con,
    const UserSession& session
);

#endif

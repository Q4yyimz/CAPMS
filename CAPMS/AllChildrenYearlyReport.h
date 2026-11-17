#ifndef ALL_CHILDREN_YEARLY_REPORT_H
#define ALL_CHILDREN_YEARLY_REPORT_H

#include "UserSession.h"

#include <mysql_connection.h>

void allChildrenYearlyPerformanceReport(
    sql::Connection* con,
    const UserSession& session
);

#endif

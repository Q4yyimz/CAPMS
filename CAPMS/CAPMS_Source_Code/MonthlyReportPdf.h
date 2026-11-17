#ifndef MONTHLY_REPORT_PDF_H
#define MONTHLY_REPORT_PDF_H

#include "UserSession.h"

#include <mysql_connection.h>

void exportMonthlyChildReportPdf(
    sql::Connection* con,
    const UserSession& session
);

#endif

#ifndef YEARLY_REPORT_PDF_H
#define YEARLY_REPORT_PDF_H

#include "UserSession.h"

#include <mysql_connection.h>

void exportYearlyChildReportPdf(
    sql::Connection* con,
    const UserSession& session
);

#endif

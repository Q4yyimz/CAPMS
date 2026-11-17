#ifndef REPORT_H
#define REPORT_H

#include "UserSession.h"

#include <mysql_connection.h>
#include <string>

// ==========================================================
// REPORT MENU
// ==========================================================

void manageReports(
    sql::Connection* con,
    const UserSession& session
);

// ==========================================================
// MONTHLY CHILD PERFORMANCE REPORT
// ==========================================================

void monthlyAnalyticsReport(
    sql::Connection* con,
    const UserSession& session
);

// ==========================================================
// YEARLY CHILD PERFORMANCE SUMMARY
// ==========================================================

void yearlyChildPerformanceSummary(
    sql::Connection* con,
    const UserSession& session
);

// ==========================================================
// REPORT VALIDATION FUNCTIONS
// ==========================================================

bool isValidReportStaffId(
    const std::string& staffId
);

bool isValidReportChildId(
    const std::string& childId
);

bool isValidReportYear(
    int year
);

#endif
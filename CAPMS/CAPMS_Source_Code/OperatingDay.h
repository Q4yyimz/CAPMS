#ifndef OPERATING_DAY_H
#define OPERATING_DAY_H

#include <mysql_connection.h>
#include <string>

void manageOperatingDays(
    sql::Connection* con
);

void generateMonthlyOperatingCalendar(
    sql::Connection* con
);

void viewMonthlyOperatingCalendar(
    sql::Connection* con
);

void updateOperatingDayStatus(
    sql::Connection* con
);

bool isValidOperatingYear(
    int year
);

bool isValidOperatingDate(
    const std::string& displayDate
);

std::string convertOperatingDateToDatabase(
    const std::string& displayDate
);

#endif
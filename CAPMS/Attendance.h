#ifndef ATTENDANCE_H
#define ATTENDANCE_H

#include "UserSession.h"
#include <mysql_connection.h>
#include <string>

void dailyCheckIn(sql::Connection* con, const UserSession& session);

void pickupVerificationAndCheckOut(sql::Connection* con, const UserSession& session);

void viewDailyAttendance(sql::Connection* con, const UserSession& session);

bool isValidAttendanceDate(const std::string& displayDate);

std::string convertAttendanceDateToDatabase(const std::string& displayDate);

std::string generateLogId(sql::Connection* con);

bool isValidAttendanceChildId(const std::string& childId);

bool isValidAttendanceStaffId(const std::string& staffId);

bool isValidPickupParentId(const std::string& parentId);

bool isValidPickupRepresentativeId(const std::string& representativeId);

#endif
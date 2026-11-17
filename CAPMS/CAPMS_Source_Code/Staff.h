#ifndef STAFF_H
#define STAFF_H

#include <mysql_connection.h>

#include <string>

void manageStaff(sql::Connection* con);
void addStaff(sql::Connection* con);
void searchStaff(sql::Connection* con);
void viewAllStaff(sql::Connection* con);
void updateStaff(sql::Connection* con);
void manageStaffAccountStatus(sql::Connection* con);

std::string generateStaffId(sql::Connection* con);
bool isValidStaffRole(const std::string& role);
bool isValidStaffIcNumber(const std::string& icNo);
bool isValidStaffEmail(const std::string& email);
bool isValidStaffPhoneNumber(const std::string& phoneNo);

#endif
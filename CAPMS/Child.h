#ifndef CHILD_H
#define CHILD_H

#include <mysql_connection.h>

#include <string>

void manageChild(sql::Connection* con);
void addChild(sql::Connection* con);
void searchChild(sql::Connection* con);
void viewAllChildren(sql::Connection* con);
void updateChild(sql::Connection* con);
void manageChildGuardiansAndRepresentatives(sql::Connection* con);
void viewAssignedGuardiansAndRepresentatives(sql::Connection* con);

std::string generateChildId(sql::Connection* con);

bool isValidChildIdFormat(const std::string& childId);
bool isValidMyKidNumber(const std::string& myKidNo);
bool isValidDateOfBirth(const std::string& dateOfBirth);
bool isValidGender(const std::string& gender);

std::string convertDateToDatabaseFormat(const std::string& displayDate);

std::string convertDateToDisplayFormat(const std::string& databaseDate);

#endif
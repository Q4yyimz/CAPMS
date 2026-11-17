#ifndef PARENT_H
#define PARENT_H

#include <mysql_connection.h>

#include <string>

void manageParent(sql::Connection* con);
void addParent(sql::Connection* con);
void searchParent(sql::Connection* con);
void viewAllParents(sql::Connection* con);
void updateParent(sql::Connection* con);
void linkParentToChild(sql::Connection* con);


std::string generateParentId(sql::Connection* con);

bool isValidParentId(const std::string& parentId);
bool isValidChildId(const std::string& childId);
bool isValidPhoneNumber(const std::string& phoneNo);
bool isValidPostcode(const std::string& postcode);
bool isValidParentRelationship(const std::string& relationshipType);

#endif
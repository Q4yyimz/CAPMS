#ifndef REPRESENTATIVE_H
#define REPRESENTATIVE_H

#include <mysql_connection.h>
#include <string>

void manageRepresentative(sql::Connection* con);

void addRepresentative(sql::Connection* con);
void searchRepresentative(sql::Connection* con);
void viewAllRepresentatives(sql::Connection* con);
void updateRepresentative(sql::Connection* con);

void linkRepresentativeToChild(sql::Connection* con);
void removeAuthorization(sql::Connection* con);

std::string generateRepresentativeId(
    sql::Connection* con
);

bool isValidRepresentativeId(
    const std::string& representativeId
);

bool isValidRepresentativePhoneNumber(
    const std::string& phoneNo
);

bool isValidRepresentativePostcode(
    const std::string& postcode
);

bool isValidRepresentativeRelationship(
    const std::string& relationship
);

bool isValidRepresentativeChildId(
    const std::string& childId
);

bool isValidRepresentativeIcNumber(
    const std::string& icNo
);

#endif
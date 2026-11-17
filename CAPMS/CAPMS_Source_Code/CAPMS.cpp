#include "Database.h"
#include "Login.h"

#include <cppconn/exception.h>

#include <iostream>

using namespace std;

int main()
{
    sql::Connection* con = nullptr;

    try
    {
        con = connectDatabase();

        cout << "\nSuccessfully connected to the CAPMS database.\n";

        displayLogin(con);

        delete con;
        con = nullptr;

        cout << "Database connection closed successfully.\n";
    }
    catch (sql::SQLException& e)
    {
        cerr << "\n";
        cerr << "==============================================" << endl;
        cerr << "          DATABASE CONNECTION ERROR           " << endl;
        cerr << "==============================================" << endl;
        cerr << "Error message : " << e.what() << endl;
        cerr << "Error code    : " << e.getErrorCode() << endl;
        cerr << "SQL state     : " << e.getSQLState() << endl;

        delete con;
        con = nullptr;

        return 1;
    }

    return 0;
}
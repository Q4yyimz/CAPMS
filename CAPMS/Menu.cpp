#include "Menu.h"

#include "Child.h"
#include "InputUtils.h"
#include "OperatingDay.h"
#include "Parent.h"
#include "Report.h"
#include "Representative.h"
#include "Staff.h"

#include <iostream>

using namespace std;

// ==========================================================
// ADMIN MAIN MENU
// ==========================================================

void displayAdminMenu(
    sql::Connection* con,
    const UserSession& session
)
{
    int choice = 0;

    do
    {
        cout << "\n";
        cout << "============================================================\n";
        cout << "                    CAPMS ADMIN MAIN MENU                   \n";
        cout << "============================================================\n";
        cout << " Logged-In Administrator : "
             << session.staffName
             << " (" << session.staffId << ")\n";
        cout << " Role                    : "
             << session.role << "\n";
        cout << "------------------------------------------------------------\n";
        cout << " 1. Manage Staff Records\n";
        cout << " 2. Manage Parent Records\n";
        cout << " 3. Manage Child Records\n";
        cout << " 4. Manage Authorized Representatives\n";
        cout << " 5. Manage Reports\n";
        cout << " 6. Manage Operating Days\n";
        cout << " 7. Logout\n";
        cout << "------------------------------------------------------------\n";
        cout << " Select an option [1-7] : ";

        if (!(cin >> choice))
        {
            cin.clear();
            clearInputBuffer();

            cout << "\nERROR: Please enter a number from 1 to 7.\n";
            continue;
        }

        clearInputBuffer();

        switch (choice)
        {
        case 1:
            manageStaff(con);
            break;

        case 2:
            manageParent(con);
            break;

        case 3:
            manageChild(con);
            break;

        case 4:
            manageRepresentative(con);
            break;

        case 5:
            manageReports(con, session);
            break;

        case 6:
            manageOperatingDays(con);
            break;

        case 7:
            cout << "\nLogging out from CAPMS...\n";
            break;

        default:
            cout << "\nERROR: Please select an option from 1 to 7.\n";
            break;
        }
    } while (choice != 7);
}

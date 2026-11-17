#include "TeacherMenu.h"

#include "Attendance.h"
#include "InputUtils.h"

#include <iostream>

using namespace std;

// ==========================================================
// TEACHER MAIN MENU
// ==========================================================

void displayTeacherMenu(
    sql::Connection* con,
    const UserSession& session
)
{
    int choice = 0;

    do
    {
        cout << "\n";
        cout << "============================================================\n";
        cout << "                   CAPMS TEACHER MAIN MENU                  \n";
        cout << "============================================================\n";
        cout << " Logged-In Teacher : "
             << session.staffName
             << " (" << session.staffId << ")\n";
        cout << " Role              : "
             << session.role << "\n";
        cout << "------------------------------------------------------------\n";
        cout << " 1. Record Daily Check-In\n";
        cout << " 2. Verify Collector and Record Check-Out\n";
        cout << " 3. View Daily Attendance\n";
        cout << " 4. Logout\n";
        cout << "------------------------------------------------------------\n";
        cout << " Select an option [1-4] : ";

        if (!(cin >> choice))
        {
            cin.clear();
            clearInputBuffer();

            cout << "\nERROR: Please enter a number from 1 to 4.\n";
            continue;
        }

        clearInputBuffer();

        switch (choice)
        {
        case 1:
            dailyCheckIn(
                con,
                session
            );
            break;

        case 2:
            pickupVerificationAndCheckOut(
                con,
                session
            );
            break;

        case 3:
            viewDailyAttendance(
                con,
                session
            );
            break;

        case 4:
            cout << "\nLogging out from CAPMS...\n";
            break;

        default:
            cout << "\nERROR: Please select an option from 1 to 4.\n";
            break;
        }
    } while (choice != 4);
}

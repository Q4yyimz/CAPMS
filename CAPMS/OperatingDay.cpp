#include "OperatingDay.h"

#include "InputUtils.h"

#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

#include <cctype>
#include <iomanip>
#include <iostream>
#include <string>

using namespace std;


// ==========================================================
// OPERATING DAYS MANAGEMENT MENU
// ==========================================================

void manageOperatingDays(
    sql::Connection* con
)
{
    int choice = 0;

    do
    {
        cout << "\n";
        cout << "============================================================\n";
        cout << "                  MANAGE OPERATING DAYS                    \n";
        cout << "============================================================\n";
        cout << " 1. Generate Monthly Operating Calendar\n";
        cout << " 2. View Monthly Operating Calendar\n";
        cout << " 3. Update Operating Day Status\n";
        cout << " 4. Back to Admin Main Menu\n";
        cout << "------------------------------------------------------------\n";
        cout << " Select an option [1-4] : ";

        if (!(cin >> choice))
        {
            cin.clear();
            clearInputBuffer();

            cout << "\nERROR: Please enter a number "
                << "from 1 to 4.\n";

            continue;
        }

        clearInputBuffer();

        switch (choice)
        {
        case 1:
            generateMonthlyOperatingCalendar(con);
            break;

        case 2:
            viewMonthlyOperatingCalendar(con);
            break;

        case 3:
            updateOperatingDayStatus(con);
            break;

        case 4:
            cout << "\nReturning to Admin Main Menu...\n";
            break;

        default:
            cout << "\nERROR: Please select an option "
                << "from 1 to 4.\n";
            break;
        }

    } while (choice != 4);
}


// ==========================================================
// GENERATE MONTHLY OPERATING CALENDAR
// ==========================================================

void generateMonthlyOperatingCalendar(
    sql::Connection* con
)
{
    int month = 0;
    int year = 0;

    sql::PreparedStatement* generateStatement = nullptr;
    sql::PreparedStatement* countStatement = nullptr;

    sql::ResultSet* countResult = nullptr;

    cout << "\n";
    cout << "============================================================\n";
    cout << "          GENERATE MONTHLY OPERATING CALENDAR              \n";
    cout << "============================================================\n";

    cout << " Enter Report Month [1-12] : ";

    if (!(cin >> month))
    {
        cin.clear();
        clearInputBuffer();

        cout << "\nERROR: Please enter a valid month.\n";
        return;
    }

    if (month < 1 || month > 12)
    {
        clearInputBuffer();

        cout << "\nERROR: Month must be between 1 and 12.\n";
        return;
    }

    cout << " Enter Report Year         : ";

    if (!(cin >> year))
    {
        cin.clear();
        clearInputBuffer();

        cout << "\nERROR: Please enter a valid year.\n";
        return;
    }

    clearInputBuffer();

    if (!isValidOperatingYear(year))
    {
        cout << "\nERROR: Year must be between "
            << "2020 and 2100.\n";
        return;
    }

    cout << "\n";
    cout << "This operation will add Monday to Friday as OPEN days.\n";
    cout << "Existing operating dates will not be overwritten.\n";

    int confirmation = 0;

    while (true)
    {
        cout << "\nConfirm Operating Calendar Generation:\n";
        cout << " 1. Confirm\n";
        cout << " 2. Cancel\n";
        cout << "\n Select an option [1-2] : ";

        if (!(cin >> confirmation))
        {
            cin.clear();
            clearInputBuffer();

            cout << "\nERROR: Please enter either 1 or 2.\n";
            continue;
        }

        clearInputBuffer();

        if (confirmation == 1 ||
            confirmation == 2)
        {
            break;
        }

        cout << "\nERROR: Please select either 1 or 2.\n";
    }

    if (confirmation == 2)
    {
        cout << "\nNOTICE: Operating calendar generation was cancelled.\n";
        return;
    }

    try
    {
        /*
         * Generate all dates for the selected month.
         * Monday to Friday are inserted as OPEN.
         *
         * INSERT IGNORE prevents existing dates from
         * being overwritten.
         */
        generateStatement = con->prepareStatement(
            "INSERT IGNORE INTO OPERATING_DAY "
            "(Operating_Date, Day_Status, Description) "

            "WITH RECURSIVE calendar_dates AS "
            "( "
            "SELECT STR_TO_DATE("
            "CONCAT(?, '-', LPAD(?, 2, '0'), '-01'), "
            "'%Y-%m-%d'"
            ") AS generated_date "

            "UNION ALL "

            "SELECT DATE_ADD(generated_date, INTERVAL 1 DAY) "
            "FROM calendar_dates "
            "WHERE generated_date < LAST_DAY("
            "STR_TO_DATE("
            "CONCAT(?, '-', LPAD(?, 2, '0'), '-01'), "
            "'%Y-%m-%d'"
            ")"
            ") "
            ") "

            "SELECT "
            "generated_date, "
            "'OPEN', "
            "'Normal operating day' "
            "FROM calendar_dates "
            "WHERE DAYOFWEEK(generated_date) BETWEEN 2 AND 6"
        );

        generateStatement->setInt(1, year);
        generateStatement->setInt(2, month);
        generateStatement->setInt(3, year);
        generateStatement->setInt(4, month);

        const int insertedRows =
            generateStatement->executeUpdate();

        countStatement = con->prepareStatement(
            "SELECT "
            "COUNT(*) AS Total_Dates, "
            "SUM(Day_Status = 'OPEN') AS Open_Days, "
            "SUM(Day_Status = 'CLOSED') AS Closed_Days "
            "FROM OPERATING_DAY "
            "WHERE MONTH(Operating_Date) = ? "
            "AND YEAR(Operating_Date) = ?"
        );

        countStatement->setInt(1, month);
        countStatement->setInt(2, year);

        countResult =
            countStatement->executeQuery();

        int totalDates = 0;
        int openDays = 0;
        int closedDays = 0;

        if (countResult->next())
        {
            totalDates =
                countResult->getInt("Total_Dates");

            openDays =
                countResult->isNull("Open_Days")
                ? 0
                : countResult->getInt("Open_Days");

            closedDays =
                countResult->isNull("Closed_Days")
                ? 0
                : countResult->getInt("Closed_Days");
        }

        cout << "\n";
        cout << "============================================================\n";
        cout << "             OPERATING CALENDAR GENERATED SUCCESSFULLY                  \n";
        cout << "============================================================\n";
        cout << " Month                 : "
            << month << "\n";
        cout << " Year                  : "
            << year << "\n";
        cout << " New Operating Dates    : "
            << insertedRows << "\n";
        cout << " Total Operating Dates  : "
            << totalDates << "\n";
        cout << " Open Days             : "
            << openDays << "\n";
        cout << " Closed Days           : "
            << closedDays << "\n";
        cout << "============================================================\n";

        delete countResult;
        countResult = nullptr;

        delete countStatement;
        countStatement = nullptr;

        delete generateStatement;
        generateStatement = nullptr;
    }
    catch (sql::SQLException& e)
    {
        delete countResult;
        countResult = nullptr;

        delete countStatement;
        countStatement = nullptr;

        delete generateStatement;
        generateStatement = nullptr;

        cerr << "\nSQL Error while generating "
            << "operating calendar:\n";

        cerr << "Message    : "
            << e.what() << "\n";

        cerr << "Error Code : "
            << e.getErrorCode() << "\n";

        cerr << "SQL State  : "
            << e.getSQLState() << "\n";
    }
}


// ==========================================================
// VIEW MONTHLY OPERATING CALENDAR
// ==========================================================

void viewMonthlyOperatingCalendar(
    sql::Connection* con
)
{
    int month = 0;
    int year = 0;

    sql::PreparedStatement* calendarStatement = nullptr;
    sql::ResultSet* calendarResult = nullptr;

    cout << "\n";
    cout << "============================================================\n";
    cout << "            VIEW MONTHLY OPERATING CALENDAR                \n";
    cout << "============================================================\n";

    cout << " Enter Report Month [1-12] : ";

    if (!(cin >> month))
    {
        cin.clear();
        clearInputBuffer();

        cout << "\nERROR: Please enter a valid month.\n";
        return;
    }

    if (month < 1 || month > 12)
    {
        clearInputBuffer();

        cout << "\nERROR: Month must be between 1 and 12.\n";
        return;
    }

    cout << " Enter Report Year         : ";

    if (!(cin >> year))
    {
        cin.clear();
        clearInputBuffer();

        cout << "\nERROR: Please enter a valid year.\n";
        return;
    }

    clearInputBuffer();

    if (!isValidOperatingYear(year))
    {
        cout << "\nERROR: Year must be between "
            << "2020 and 2100.\n";
        return;
    }

    try
    {
        calendarStatement = con->prepareStatement(
            "SELECT "
            "DATE_FORMAT(Operating_Date, '%d/%m/%Y') "
            "AS Formatted_Date, "
            "DAYNAME(Operating_Date) AS Day_Name, "
            "Day_Status, "
            "Description "
            "FROM OPERATING_DAY "
            "WHERE MONTH(Operating_Date) = ? "
            "AND YEAR(Operating_Date) = ? "
            "ORDER BY Operating_Date"
        );

        calendarStatement->setInt(1, month);
        calendarStatement->setInt(2, year);

        calendarResult =
            calendarStatement->executeQuery();

        cout << "\n";
        cout << "==========================================================================\n";
        cout << "                      MONTHLY OPERATING DAY CALENDAR                              \n";
        cout << "==========================================================================\n";

        cout << left
            << setw(6) << "No"
            << setw(14) << "Date"
            << setw(14) << "Day"
            << setw(12) << "Status"
            << setw(30) << "Description"
            << "\n";

        cout << string(74, '=') << "\n";

        int recordCount = 0;
        int openDays = 0;
        int closedDays = 0;

        while (calendarResult->next())
        {
            recordCount++;

            const string dayStatus =
                string(
                    calendarResult->getString(
                        "Day_Status"
                    ).c_str()
                );

            if (dayStatus == "OPEN")
            {
                openDays++;
            }
            else if (dayStatus == "CLOSED")
            {
                closedDays++;
            }

            string description = "-";

            if (!calendarResult->isNull("Description"))
            {
                description =
                    string(
                        calendarResult->getString(
                            "Description"
                        ).c_str()
                    );
            }

            if (description.length() > 28)
            {
                description =
                    description.substr(0, 28);
            }

            cout << left
                << setw(6) << recordCount
                << setw(14)
                << calendarResult->getString(
                    "Formatted_Date"
                )
                << setw(14)
                << calendarResult->getString(
                    "Day_Name"
                )
                << setw(12)
                << dayStatus
                << setw(30)
                << description
                << "\n";
        }

        cout << string(74, '=') << "\n";

        if (recordCount == 0)
        {
            cout << " NOTICE: No operating calendar exists "
                << "for the selected month.\n";

            cout << " Please generate the monthly operating calendar first.\n";
        }
        else
        {
            cout << " Total Operating Dates : "
                << recordCount << "\n";

            cout << " Open Operating Days      : "
                << openDays << "\n";

            cout << " Closed Operating Days    : "
                << closedDays << "\n";
        }

        cout << "==========================================================================\n";

        delete calendarResult;
        calendarResult = nullptr;

        delete calendarStatement;
        calendarStatement = nullptr;
    }
    catch (sql::SQLException& e)
    {
        delete calendarResult;
        calendarResult = nullptr;

        delete calendarStatement;
        calendarStatement = nullptr;

        cerr << "\nSQL Error while viewing "
            << "operating calendar:\n";

        cerr << "Message    : "
            << e.what() << "\n";

        cerr << "Error Code : "
            << e.getErrorCode() << "\n";

        cerr << "SQL State  : "
            << e.getSQLState() << "\n";
    }
}


// ==========================================================
// UPDATE OPERATING DAY STATUS
// ==========================================================

void updateOperatingDayStatus(
    sql::Connection* con
)
{
    string displayDate;
    string databaseDate;

    string currentStatus;
    string currentDescription;

    string newStatus;
    string newDescription;

    sql::PreparedStatement* searchStatement = nullptr;
    sql::PreparedStatement* updateStatement = nullptr;

    sql::ResultSet* result = nullptr;

    cout << "\n";
    cout << "============================================================\n";
    cout << "              UPDATE OPERATING DAY STATUS                  \n";
    cout << "============================================================\n";

    cout << " Enter Operating Date [DD/MM/YYYY] : ";
    getline(cin, displayDate);

    if (!isValidOperatingDate(displayDate))
    {
        cout << "\nERROR: Invalid date.\n";
        cout << "Expected format: DD/MM/YYYY\n";
        return;
    }

    databaseDate =
        convertOperatingDateToDatabase(
            displayDate
        );

    try
    {
        searchStatement = con->prepareStatement(
            "SELECT "
            "Day_Status, "
            "Description "
            "FROM OPERATING_DAY "
            "WHERE Operating_Date = ?"
        );

        searchStatement->setString(
            1,
            databaseDate
        );

        result =
            searchStatement->executeQuery();

        if (!result->next())
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "              OPERATING DAY NOT FOUND                     \n";
            cout << "============================================================\n";
            cout << " Date: " << displayDate << "\n";
            cout << " Please generate the monthly operating calendar first.\n";
            cout << "============================================================\n";

            delete result;
            result = nullptr;

            delete searchStatement;
            searchStatement = nullptr;

            return;
        }

        currentStatus =
            string(
                result->getString(
                    "Day_Status"
                ).c_str()
            );

        if (result->isNull("Description"))
        {
            currentDescription = "";
        }
        else
        {
            currentDescription =
                string(
                    result->getString(
                        "Description"
                    ).c_str()
                );
        }

        cout << "\n";
        cout << "============================================================\n";
        cout << "              CURRENT OPERATING DAY INFORMATION                \n";
        cout << "============================================================\n";
        cout << " Date        : " << displayDate << "\n";
        cout << " Status      : " << currentStatus << "\n";
        cout << " Description : "
            << (
                currentDescription.empty()
                ? "Not Provided"
                : currentDescription
                )
            << "\n";
        cout << "============================================================\n";

        int statusChoice = 0;

        while (true)
        {
            cout << "\nSelect New Status:\n";
            cout << " 1. OPEN\n";
            cout << " 2. CLOSED\n";
            cout << "\n Select status [1-2] : ";

            if (!(cin >> statusChoice))
            {
                cin.clear();
                clearInputBuffer();

                cout << "\nERROR: Please enter either 1 or 2.\n";
                continue;
            }

            clearInputBuffer();

            if (statusChoice == 1)
            {
                newStatus = "OPEN";
                break;
            }

            if (statusChoice == 2)
            {
                newStatus = "CLOSED";
                break;
            }

            cout << "\nERROR: Please select either 1 or 2.\n";
        }

        cout << " Enter Description          : ";
        getline(cin, newDescription);

        if (newDescription.empty())
        {
            if (newStatus == "OPEN")
            {
                newDescription =
                    "Normal operating day";
            }
            else
            {
                newDescription =
                    "Centre closed";
            }
        }

        if (newDescription.length() > 200)
        {
            cout << "\nERROR: Description must not exceed "
                << "200 characters.\n";

            delete result;
            result = nullptr;

            delete searchStatement;
            searchStatement = nullptr;

            return;
        }

        cout << "\n";
        cout << "============================================================\n";
        cout << "             OPERATING DAY UPDATE CONFIRMATION             \n";
        cout << "============================================================\n";
        cout << " Date        : " << displayDate << "\n";
        cout << " Old Status  : " << currentStatus << "\n";
        cout << " New Status  : " << newStatus << "\n";
        cout << " Description : " << newDescription << "\n";
        cout << "============================================================\n";

        int confirmation = 0;

        while (true)
        {
            cout << "\nConfirm Operating Day Update:\n";
            cout << " 1. Confirm\n";
            cout << " 2. Cancel\n";
            cout << "\n Select an option [1-2] : ";

            if (!(cin >> confirmation))
            {
                cin.clear();
                clearInputBuffer();

                cout << "\nERROR: Please enter either 1 or 2.\n";
                continue;
            }

            clearInputBuffer();

            if (confirmation == 1 ||
                confirmation == 2)
            {
                break;
            }

            cout << "\nERROR: Please select either 1 or 2.\n";
        }

        if (confirmation == 2)
        {
            cout << "\nNOTICE: Operating day update was cancelled.\n";

            delete result;
            result = nullptr;

            delete searchStatement;
            searchStatement = nullptr;

            return;
        }

        updateStatement = con->prepareStatement(
            "UPDATE OPERATING_DAY "
            "SET Day_Status = ?, "
            "Description = ? "
            "WHERE Operating_Date = ?"
        );

        updateStatement->setString(
            1,
            newStatus
        );

        updateStatement->setString(
            2,
            newDescription
        );

        updateStatement->setString(
            3,
            databaseDate
        );

        const int affectedRows =
            updateStatement->executeUpdate();

        if (affectedRows > 0)
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "              OPERATING DAY UPDATE SUCCESSFUL              \n";
            cout << "============================================================\n";
            cout << " Date        : " << displayDate << "\n";
            cout << " Status      : " << newStatus << "\n";
            cout << " Description : " << newDescription << "\n";
            cout << "============================================================\n";
        }
        else
        {
            cout << "\nNOTICE: No changes were made.\n";
        }

        delete result;
        result = nullptr;

        delete searchStatement;
        searchStatement = nullptr;

        delete updateStatement;
        updateStatement = nullptr;
    }
    catch (sql::SQLException& e)
    {
        delete result;
        result = nullptr;

        delete searchStatement;
        searchStatement = nullptr;

        delete updateStatement;
        updateStatement = nullptr;

        cerr << "\nSQL Error while updating "
            << "operating day:\n";

        cerr << "Message    : "
            << e.what() << "\n";

        cerr << "Error Code : "
            << e.getErrorCode() << "\n";

        cerr << "SQL State  : "
            << e.getSQLState() << "\n";
    }
}


// ==========================================================
// OPERATING DAY YEAR VALIDATION
// ==========================================================

bool isValidOperatingYear(
    int year
)
{
    return year >= 2020 &&
        year <= 2100;
}


// ==========================================================
// OPERATING DATE VALIDATION
// ==========================================================

bool isValidOperatingDate(
    const string& displayDate
)
{
    if (displayDate.length() != 10)
    {
        return false;
    }

    if (displayDate[2] != '/' ||
        displayDate[5] != '/')
    {
        return false;
    }

    for (
        size_t index = 0;
        index < displayDate.length();
        index++
        )
    {
        if (index == 2 || index == 5)
        {
            continue;
        }

        if (!std::isdigit(
            static_cast<unsigned char>(
                displayDate[index]
                )
        ))
        {
            return false;
        }
    }

    const int day =
        stoi(displayDate.substr(0, 2));

    const int month =
        stoi(displayDate.substr(3, 2));

    const int year =
        stoi(displayDate.substr(6, 4));

    if (!isValidOperatingYear(year))
    {
        return false;
    }

    if (month < 1 || month > 12)
    {
        return false;
    }

    int daysInMonth[] =
    {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    const bool leapYear =
        (year % 400 == 0) ||
        (
            year % 4 == 0 &&
            year % 100 != 0
            );

    if (leapYear)
    {
        daysInMonth[1] = 29;
    }

    if (day < 1 ||
        day > daysInMonth[month - 1])
    {
        return false;
    }

    return true;
}


// ==========================================================
// CONVERT DD/MM/YYYY TO YYYY-MM-DD
// ==========================================================

string convertOperatingDateToDatabase(
    const string& displayDate
)
{
    const string day =
        displayDate.substr(0, 2);

    const string month =
        displayDate.substr(3, 2);

    const string year =
        displayDate.substr(6, 4);

    return year + "-" + month + "-" + day;
}
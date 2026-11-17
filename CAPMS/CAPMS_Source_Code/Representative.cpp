#include "Representative.h"
#include "InputUtils.h"

#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

using namespace std;


// ==========================================================
// DISPLAY AVAILABLE REPRESENTATIVE RECORDS
// ==========================================================
static bool displayAvailableRepresentativeRecords(sql::Connection* con)
{
    sql::PreparedStatement* statement = nullptr;
    sql::ResultSet* result = nullptr;
    try
    {
        statement = con->prepareStatement(
            "SELECT ar.Rep_ID, ar.Name, ar.Phone_No, "
            "COUNT(ac.Child_ID) AS Authorized_Children "
            "FROM AUTHORIZED_REPRESENTATIVE ar "
            "LEFT JOIN AUTHOREP_CHILD ac ON ar.Rep_ID = ac.Rep_ID "
            "GROUP BY ar.Rep_ID, ar.Name, ar.Phone_No "
            "ORDER BY ar.Name ASC"
        );
        result = statement->executeQuery();
        cout << "\n";
        cout << "================================================================================\n";
        cout << "                  AVAILABLE AUTHORIZED REPRESENTATIVES                         \n";
        cout << "================================================================================\n";
        cout << left << setw(6) << "No." << setw(12) << "Rep ID"
             << setw(30) << "Representative Name" << setw(17) << "Phone No."
             << setw(12) << "Children" << "\n";
        cout << string(80, '=') << "\n";
        int count = 0;
        while (result->next())
        {
            count++;
            string name = string(result->getString("Name").c_str());
            if (name.length() > 28) name = name.substr(0, 25) + "...";
            cout << left << setw(6) << count
                 << setw(12) << result->getString("Rep_ID")
                 << setw(30) << name
                 << setw(17) << result->getString("Phone_No")
                 << setw(12) << result->getInt("Authorized_Children") << "\n";
        }
        cout << string(80, '=') << "\n";
        if (count == 0) cout << " No representative records are currently available.\n";
        else cout << " Total Available Representatives : " << count << "\n";
        cout << string(80, '=') << "\n";
        delete result;
        delete statement;
        return count > 0;
    }
    catch (sql::SQLException& e)
    {
        delete result;
        delete statement;
        cerr << "\nSQL Error while retrieving representative records:\n";
        cerr << "Message    : " << e.what() << "\n";
        cerr << "Error Code : " << e.getErrorCode() << "\n";
        cerr << "SQL State  : " << e.getSQLState() << "\n";
        return false;
    }
}

static bool displayAvailableRepresentativeChildRecords(sql::Connection* con)
{
    sql::PreparedStatement* statement = nullptr;
    sql::ResultSet* result = nullptr;
    try
    {
        statement = con->prepareStatement(
            "SELECT Child_ID, Name FROM CHILD ORDER BY Name ASC"
        );
        result = statement->executeQuery();
        cout << "\n============================================================\n";
        cout << "                 AVAILABLE CHILD RECORDS                   \n";
        cout << "============================================================\n";
        cout << left << setw(6) << "No." << setw(12) << "Child ID"
             << setw(32) << "Child Name" << "\n";
        cout << string(60, '=') << "\n";
        int count = 0;
        while (result->next())
        {
            count++;
            string name = string(result->getString("Name").c_str());
            if (name.length() > 30) name = name.substr(0, 27) + "...";
            cout << left << setw(6) << count
                 << setw(12) << result->getString("Child_ID")
                 << setw(32) << name << "\n";
        }
        cout << string(60, '=') << "\n";
        if (count == 0) cout << " No child records are currently available.\n";
        else cout << " Total Available Children : " << count << "\n";
        cout << string(60, '=') << "\n";
        delete result;
        delete statement;
        return count > 0;
    }
    catch (sql::SQLException& e)
    {
        delete result;
        delete statement;
        cerr << "\nSQL Error while retrieving available child records:\n";
        cerr << "Message    : " << e.what() << "\n";
        cerr << "Error Code : " << e.getErrorCode() << "\n";
        cerr << "SQL State  : " << e.getSQLState() << "\n";
        return false;
    }
}

void manageRepresentative(sql::Connection* con)
{
    int choice = 0;
    do
    {
        cout << "\n";
        cout << "============================================================\n";
        cout << "              MANAGE AUTHORIZED REPRESENTATIVES            \n";
        cout << "============================================================\n";
        cout << " 1. Register New Authorized Representative\n";
        cout << " 2. Search Representative Record\n";
        cout << " 3. View All Representative Records\n";
        cout << " 4. Update Representative Record\n";
        cout << " 5. Back to Admin Main Menu\n";
        cout << "------------------------------------------------------------\n";
        cout << " Select an option [1-5] : ";
        if (!(cin >> choice))
        {
            cin.clear();
            clearInputBuffer();
            cout << "\nERROR: Please enter a number from 1 to 5.\n";
            continue;
        }
        clearInputBuffer();
        switch (choice)
        {
        case 1: addRepresentative(con); break;
        case 2: searchRepresentative(con); break;
        case 3: viewAllRepresentatives(con); break;
        case 4: updateRepresentative(con); break;
        case 5: cout << "\nReturning to Admin Main Menu...\n"; break;
        default: cout << "\nERROR: Please select an option from 1 to 5.\n"; break;
        }
    } while (choice != 5);
}
// ==========================================================
// ADD AUTHORIZED REPRESENTATIVE
// ==========================================================

void addRepresentative(sql::Connection* con)
{
    string representativeId;
    string icNo;
    string name;
    string phoneNo;
    string street;
    string city;
    string state;
    string postcode;

    sql::PreparedStatement* duplicateStatement = nullptr;
    sql::PreparedStatement* insertStatement = nullptr;

    sql::ResultSet* duplicateResult = nullptr;

    cout << "\n";
    cout << "============================================================\n";
    cout << "             ADD AUTHORIZED REPRESENTATIVE                 \n";
    cout << "============================================================\n";

    clearInputBuffer();

    // ======================================================
    // ENTER AND VALIDATE REPRESENTATIVE NAME
    // ======================================================

    while (true)
    {
        cout << " Enter Representative Name : ";
        getline(cin, name);

        if (name.empty())
        {
            cout << "\nERROR: Representative name "
                << "must not be empty.\n";
            cout << "Please enter the representative "
                << "name again.\n\n";

            continue;
        }

        if (name.length() > 200)
        {
            cout << "\nERROR: Representative name "
                << "must not exceed 200 characters.\n";
            cout << "Please enter the representative "
                << "name again.\n\n";

            continue;
        }

        break;
    }

    // ======================================================
    // ENTER, VALIDATE AND CHECK IC NUMBER
    // ======================================================

    while (true)
    {
        cout << " Enter IC Number           : ";
        getline(cin, icNo);

        if (!isValidRepresentativeIcNumber(icNo))
        {
            cout << "\nERROR: IC number must contain "
                << "exactly 12 numeric digits.\n";

            cout << "\nCurrent valid information:\n";
            cout << " Representative Name : " << name << "\n";

            cout << "\nPlease enter the IC number again.\n\n";
            continue;
        }

        try
        {
            duplicateStatement = con->prepareStatement(
                "SELECT Rep_ID, Name "
                "FROM AUTHORIZED_REPRESENTATIVE "
                "WHERE IC_No = ?"
            );

            duplicateStatement->setString(1, icNo);

            duplicateResult =
                duplicateStatement->executeQuery();

            if (duplicateResult->next())
            {
                cout << "\n";
                cout << "============================================================\n";
                cout << "                   DUPLICATE IC NUMBER                     \n";
                cout << "============================================================\n";
                cout << " IC Number                 : "
                    << icNo << "\n";
                cout << " Existing Representative ID: "
                    << duplicateResult->getString("Rep_ID")
                    << "\n";
                cout << " Existing Name             : "
                    << duplicateResult->getString("Name")
                    << "\n";
                cout << "============================================================\n";
                cout << "Please enter a different IC number.\n\n";

                delete duplicateResult;
                duplicateResult = nullptr;

                delete duplicateStatement;
                duplicateStatement = nullptr;

                continue;
            }

            delete duplicateResult;
            duplicateResult = nullptr;

            delete duplicateStatement;
            duplicateStatement = nullptr;

            break;
        }
        catch (sql::SQLException& e)
        {
            delete duplicateResult;
            duplicateResult = nullptr;

            delete duplicateStatement;
            duplicateStatement = nullptr;

            cerr << "\nSQL Error while checking IC number:\n";
            cerr << "Message    : " << e.what() << "\n";
            cerr << "Error Code : " << e.getErrorCode() << "\n";
            cerr << "SQL State  : " << e.getSQLState() << "\n";

            return;
        }
    }

    // ======================================================
    // ENTER AND VALIDATE PHONE NUMBER
    // ======================================================

    while (true)
    {
        cout << " Enter Phone No            : ";
        getline(cin, phoneNo);

        if (!isValidRepresentativePhoneNumber(phoneNo))
        {
            cout << "\nERROR: Phone number must contain "
                << "between 10 and 12 numeric digits.\n";

            cout << "\nCurrent valid information:\n";
            cout << " Representative Name : " << name << "\n";
            cout << " IC Number           : " << icNo << "\n";

            cout << "\nPlease enter the phone number again.\n\n";
            continue;
        }

        break;
    }

    // ======================================================
    // ENTER AND VALIDATE STREET
    // ======================================================

    while (true)
    {
        cout << " Enter Street              : ";
        getline(cin, street);

        if (street.empty())
        {
            cout << "\nERROR: Street must not be empty.\n";
            cout << "Please enter the street again.\n\n";
            continue;
        }

        if (street.length() > 200)
        {
            cout << "\nERROR: Street must not exceed "
                << "200 characters.\n";
            cout << "Please enter the street again.\n\n";
            continue;
        }

        break;
    }

    // ======================================================
    // ENTER AND VALIDATE CITY
    // ======================================================

    while (true)
    {
        cout << " Enter City                : ";
        getline(cin, city);

        if (city.empty())
        {
            cout << "\nERROR: City must not be empty.\n";
            cout << "Please enter the city again.\n\n";
            continue;
        }

        if (city.length() > 100)
        {
            cout << "\nERROR: City must not exceed "
                << "100 characters.\n";
            cout << "Please enter the city again.\n\n";
            continue;
        }

        break;
    }

    // ======================================================
    // ENTER AND VALIDATE STATE
    // ======================================================

    while (true)
    {
        cout << " Enter State               : ";
        getline(cin, state);

        if (state.empty())
        {
            cout << "\nERROR: State must not be empty.\n";
            cout << "Please enter the state again.\n\n";
            continue;
        }

        if (state.length() > 50)
        {
            cout << "\nERROR: State must not exceed "
                << "50 characters.\n";
            cout << "Please enter the state again.\n\n";
            continue;
        }

        break;
    }

    // ======================================================
    // ENTER AND VALIDATE POSTCODE
    // ======================================================

    while (true)
    {
        cout << " Enter Postcode            : ";
        getline(cin, postcode);

        if (!isValidRepresentativePostcode(postcode))
        {
            cout << "\nERROR: Postcode must contain "
                << "exactly 5 numeric digits.\n";

            cout << "\nCurrent valid address information:\n";
            cout << " Street : " << street << "\n";
            cout << " City   : " << city << "\n";
            cout << " State  : " << state << "\n";

            cout << "\nPlease enter the postcode again.\n\n";
            continue;
        }

        break;
    }

    // ======================================================
    // DISPLAY DETAILS FOR CONFIRMATION
    // ======================================================

    cout << "\n";
    cout << "============================================================\n";
    cout << "            REPRESENTATIVE DETAILS CONFIRMATION            \n";
    cout << "============================================================\n";
    cout << " Name      : " << name << "\n";
    cout << " IC Number : " << icNo << "\n";
    cout << " Phone No  : " << phoneNo << "\n";
    cout << " Street    : " << street << "\n";
    cout << " City      : " << city << "\n";
    cout << " State     : " << state << "\n";
    cout << " Postcode  : " << postcode << "\n";
    cout << "============================================================\n";

    int confirmation = 0;

    while (true)
    {
        cout << "\nConfirm representative registration:\n";
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
        cout << "\nRepresentative registration cancelled.\n";
        return;
    }

    // ======================================================
    // INSERT REPRESENTATIVE RECORD
    // ======================================================

    try
    {
        representativeId =
            generateRepresentativeId(con);

        insertStatement = con->prepareStatement(
            "INSERT INTO AUTHORIZED_REPRESENTATIVE "
            "(Rep_ID, IC_No, Name, Phone_No, Street, "
            "City, State, Postcode) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
        );

        insertStatement->setString(1, representativeId);
        insertStatement->setString(2, icNo);
        insertStatement->setString(3, name);
        insertStatement->setString(4, phoneNo);
        insertStatement->setString(5, street);
        insertStatement->setString(6, city);
        insertStatement->setString(7, state);
        insertStatement->setString(8, postcode);

        const int affectedRows =
            insertStatement->executeUpdate();

        if (affectedRows > 0)
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "         REPRESENTATIVE REGISTRATION SUCCESSFUL            \n";
            cout << "============================================================\n";
            cout << " Representative ID : "
                << representativeId << "\n";
            cout << " IC Number         : "
                << icNo << "\n";
            cout << " Name              : "
                << name << "\n";
            cout << " Phone No          : "
                << phoneNo << "\n";
            cout << " Street            : "
                << street << "\n";
            cout << " City              : "
                << city << "\n";
            cout << " State             : "
                << state << "\n";
            cout << " Postcode          : "
                << postcode << "\n";
            cout << "============================================================\n";
        }
        else
        {
            cout << "\nRepresentative record was not created.\n";
        }

        delete insertStatement;
        insertStatement = nullptr;
    }
    catch (sql::SQLException& e)
    {
        delete duplicateResult;
        duplicateResult = nullptr;

        delete duplicateStatement;
        duplicateStatement = nullptr;

        delete insertStatement;
        insertStatement = nullptr;

        cerr << "\nSQL Error while adding representative:\n";
        cerr << "Message    : " << e.what() << "\n";
        cerr << "Error Code : " << e.getErrorCode() << "\n";
        cerr << "SQL State  : " << e.getSQLState() << "\n";
    }
}

// ==========================================================
// VIEW ALL AUTHORIZED REPRESENTATIVES
// ==========================================================
void viewAllRepresentatives(sql::Connection* con)
{
    displayAvailableRepresentativeRecords(con);
}

// ==========================================================
// SEARCH / VIEW AUTHORIZED REPRESENTATIVE
// ==========================================================

void searchRepresentative(sql::Connection* con)
{
    string representativeId;

    sql::PreparedStatement* searchStatement = nullptr;
    sql::ResultSet* result = nullptr;

    cout << "\n";
    cout << "============================================================\n";
    cout << "        SEARCH / VIEW AUTHORIZED REPRESENTATIVE            \n";
    cout << "============================================================\n";

    if (!displayAvailableRepresentativeRecords(con))
    {
        cout << "\nNOTICE: No representative record is available for selection.\n";
        return;
    }

    cout << " Enter Representative ID : ";
    getline(cin, representativeId);

    // Convert ID to uppercase
    for (char& character : representativeId)
    {
        character = static_cast<char>(
            std::toupper(
                static_cast<unsigned char>(character)
            )
            );
    }

    if (!isValidRepresentativeId(representativeId))
    {
        cout << "\nERROR: Invalid Representative ID format.\n";
        cout << "Expected format: R0001\n";
        return;
    }

    try
    {
        searchStatement = con->prepareStatement(
            "SELECT Rep_ID, IC_No, Name, Phone_No, Street, "
            "City, State, Postcode "
            "FROM AUTHORIZED_REPRESENTATIVE "
            "WHERE Rep_ID = ?"
        );

        searchStatement->setString(
            1,
            representativeId
        );

        result = searchStatement->executeQuery();

        if (result->next())
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "             AUTHORIZED REPRESENTATIVE RECORD              \n";
            cout << "============================================================\n";
            cout << " Representative ID : "
                << result->getString("Rep_ID") << "\n";
            cout << " IC Number : "
                << result->getString("IC_No") << "\n";
            cout << " Name              : "
                << result->getString("Name") << "\n";
            cout << " Phone No          : "
                << result->getString("Phone_No") << "\n";
            cout << " Street            : "
                << result->getString("Street") << "\n";
            cout << " City              : "
                << result->getString("City") << "\n";
            cout << " State             : "
                << result->getString("State") << "\n";
            cout << " Postcode          : "
                << result->getString("Postcode") << "\n";
            cout << "============================================================\n";
        }
        else
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "          AUTHORIZED REPRESENTATIVE NOT FOUND              \n";
            cout << "============================================================\n";
            cout << " No record was found for Representative ID: "
                << representativeId << "\n";
            cout << "============================================================\n";
        }

        delete result;
        result = nullptr;

        delete searchStatement;
        searchStatement = nullptr;
    }
    catch (sql::SQLException& e)
    {
        delete result;
        result = nullptr;

        delete searchStatement;
        searchStatement = nullptr;

        cerr << "\nSQL Error while searching for representative:\n";
        cerr << "Message    : " << e.what() << "\n";
        cerr << "Error Code : " << e.getErrorCode() << "\n";
        cerr << "SQL State  : " << e.getSQLState() << "\n";
    }
}

// ==========================================================
// UPDATE AUTHORIZED REPRESENTATIVE
// ==========================================================

void updateRepresentative(sql::Connection* con)
{
    string representativeId;

    string currentIcNo;
    string currentName;
    string currentPhoneNo;
    string currentStreet;
    string currentCity;
    string currentState;
    string currentPostcode;

    string newIcNo;
    string newName;
    string newPhoneNo;
    string newStreet;
    string newCity;
    string newState;
    string newPostcode;

    sql::PreparedStatement* searchStatement = nullptr;
    sql::PreparedStatement* duplicateStatement = nullptr;
    sql::PreparedStatement* updateStatement = nullptr;

    sql::ResultSet* result = nullptr;
    sql::ResultSet* duplicateResult = nullptr;

    cout << "\n";
    cout << "============================================================\n";
    cout << "           UPDATE AUTHORIZED REPRESENTATIVE                \n";
    cout << "============================================================\n";

    if (!displayAvailableRepresentativeRecords(con))
    {
        cout << "\nNOTICE: No representative record is available for selection.\n";
        return;
    }

    cout << " Enter Representative ID : ";
    getline(cin, representativeId);

    // Convert Representative ID to uppercase
    for (char& character : representativeId)
    {
        character = static_cast<char>(
            std::toupper(
                static_cast<unsigned char>(character)
            )
            );
    }

    // Validate Representative ID
    if (!isValidRepresentativeId(representativeId))
    {
        cout << "\nERROR: Invalid Representative ID format.\n";
        cout << "Expected format: R0001\n";
        return;
    }

    try
    {
        // Search for the existing representative
        searchStatement = con->prepareStatement(
            "SELECT Rep_ID, IC_No, Name, Phone_No, Street, "
            "City, State, Postcode "
            "FROM AUTHORIZED_REPRESENTATIVE "
            "WHERE Rep_ID = ?"
        );

        searchStatement->setString(
            1,
            representativeId
        );

        result = searchStatement->executeQuery();

        if (!result->next())
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "          AUTHORIZED REPRESENTATIVE NOT FOUND              \n";
            cout << "============================================================\n";
            cout << " No record was found for Representative ID: "
                << representativeId << "\n";
            cout << "============================================================\n";

            delete result;
            result = nullptr;

            delete searchStatement;
            searchStatement = nullptr;

            return;
        }

        // Store the current details

        currentIcNo =
            string(result->getString("IC_No").c_str());

        currentName =
            string(result->getString("Name").c_str());

        currentPhoneNo =
            string(result->getString("Phone_No").c_str());

        currentStreet =
            string(result->getString("Street").c_str());

        currentCity =
            string(result->getString("City").c_str());

        currentState =
            string(result->getString("State").c_str());

        currentPostcode =
            string(result->getString("Postcode").c_str());

        // Display the current details
        cout << "\n";
        cout << "============================================================\n";
        cout << "             CURRENT REPRESENTATIVE DETAILS                \n";
        cout << "============================================================\n";
        cout << " Representative ID : "
            << representativeId << "\n";
        cout << " IC Number         : "
            << currentIcNo << "\n";
        cout << " Name              : "
            << currentName << "\n";
        cout << " Phone No          : "
            << currentPhoneNo << "\n";
        cout << " Street            : "
            << currentStreet << "\n";
        cout << " City              : "
            << currentCity << "\n";
        cout << " State             : "
            << currentState << "\n";
        cout << " Postcode          : "
            << currentPostcode << "\n";
        cout << "============================================================\n";

        cout << "\nPress ENTER without entering a value "
            << "to keep the current information.\n\n";

        // Enter the new details

        while (true)
        {
            cout << " Enter New IC Number : ";
            getline(cin, newIcNo);

            // Press Enter to retain the current IC number
            if (newIcNo.empty())
            {
                newIcNo = currentIcNo;
                break;
            }

            if (!isValidRepresentativeIcNumber(newIcNo))
            {
                cout << "\nERROR: IC number must contain "
                    << "exactly 12 numeric digits.\n";
                cout << "Please enter the IC number again, "
                    << "or press ENTER to keep the current IC.\n\n";
                continue;
            }

            try
            {
                duplicateStatement = con->prepareStatement(
                    "SELECT Rep_ID, Name "
                    "FROM AUTHORIZED_REPRESENTATIVE "
                    "WHERE IC_No = ? "
                    "AND Rep_ID <> ?"
                );

                duplicateStatement->setString(1, newIcNo);
                duplicateStatement->setString(
                    2,
                    representativeId
                );

                duplicateResult =
                    duplicateStatement->executeQuery();

                if (duplicateResult->next())
                {
                    cout << "\n";
                    cout << "============================================================\n";
                    cout << "                   DUPLICATE IC NUMBER                     \n";
                    cout << "============================================================\n";
                    cout << " IC Number                 : "
                        << newIcNo << "\n";
                    cout << " Existing Representative ID: "
                        << duplicateResult->getString("Rep_ID")
                        << "\n";
                    cout << " Existing Name             : "
                        << duplicateResult->getString("Name")
                        << "\n";
                    cout << "============================================================\n";
                    cout << "Please enter a different IC number, "
                        << "or press ENTER to keep the current IC.\n\n";

                    delete duplicateResult;
                    duplicateResult = nullptr;

                    delete duplicateStatement;
                    duplicateStatement = nullptr;

                    continue;
                }

                delete duplicateResult;
                duplicateResult = nullptr;

                delete duplicateStatement;
                duplicateStatement = nullptr;

                break;
            }
            catch (sql::SQLException& e)
            {
                delete duplicateResult;
                duplicateResult = nullptr;

                delete duplicateStatement;
                duplicateStatement = nullptr;

                cerr << "\nSQL Error while checking IC number:\n";
                cerr << "Message    : " << e.what() << "\n";
                cerr << "Error Code : " << e.getErrorCode() << "\n";
                cerr << "SQL State  : " << e.getSQLState() << "\n";

                delete result;
                result = nullptr;

                delete searchStatement;
                searchStatement = nullptr;

                return;
            }
        }

        cout << " Enter New Name     : ";
        getline(cin, newName);

        cout << " Enter New Phone No : ";
        getline(cin, newPhoneNo);

        cout << " Enter New Street   : ";
        getline(cin, newStreet);

        cout << " Enter New City     : ";
        getline(cin, newCity);

        cout << " Enter New State    : ";
        getline(cin, newState);

        cout << " Enter New Postcode : ";
        getline(cin, newPostcode);

        // Keep the current values when Enter is pressed
        if (newName.empty())
        {
            newName = currentName;
        }

        if (newPhoneNo.empty())
        {
            newPhoneNo = currentPhoneNo;
        }

        if (newStreet.empty())
        {
            newStreet = currentStreet;
        }

        if (newCity.empty())
        {
            newCity = currentCity;
        }

        if (newState.empty())
        {
            newState = currentState;
        }

        if (newPostcode.empty())
        {
            newPostcode = currentPostcode;
        }

        // Validate name
        if (newName.empty() || newName.length() > 200)
        {
            cout << "\nERROR: Representative name must contain "
                << "between 1 and 200 characters.\n";

            delete result;
            delete searchStatement;

            return;
        }

        // Validate phone number
        if (!isValidRepresentativePhoneNumber(newPhoneNo))
        {
            cout << "\nERROR: Phone number must contain "
                << "between 10 and 12 numeric digits.\n";

            delete result;
            delete searchStatement;

            return;
        }

        // Validate street
        if (newStreet.empty() || newStreet.length() > 200)
        {
            cout << "\nERROR: Street must contain "
                << "between 1 and 200 characters.\n";

            delete result;
            delete searchStatement;

            return;
        }

        // Validate city
        if (newCity.empty() || newCity.length() > 100)
        {
            cout << "\nERROR: City must contain "
                << "between 1 and 100 characters.\n";

            delete result;
            delete searchStatement;

            return;
        }

        // Validate state
        if (newState.empty() || newState.length() > 50)
        {
            cout << "\nERROR: State must contain "
                << "between 1 and 50 characters.\n";

            delete result;
            delete searchStatement;

            return;
        }

        // Validate postcode
        if (!isValidRepresentativePostcode(newPostcode))
        {
            cout << "\nERROR: Postcode must contain "
                << "exactly 5 numeric digits.\n";

            delete result;
            delete searchStatement;

            return;
        }

        // Update the database record
        updateStatement = con->prepareStatement(
            "UPDATE AUTHORIZED_REPRESENTATIVE "
            "SET IC_No = ?, Name = ?, Phone_No = ?, Street = ?, "
            "City = ?, State = ?, Postcode = ? "
            "WHERE Rep_ID = ?"
        );

        updateStatement->setString(1, newIcNo);
        updateStatement->setString(2, newName);
        updateStatement->setString(3, newPhoneNo);
        updateStatement->setString(4, newStreet);
        updateStatement->setString(5, newCity);
        updateStatement->setString(6, newState);
        updateStatement->setString(7, newPostcode);
        updateStatement->setString(8, representativeId);

        const int affectedRows =
            updateStatement->executeUpdate();

        if (affectedRows > 0)
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "           REPRESENTATIVE UPDATE SUCCESSFUL                \n";
            cout << "============================================================\n";
            cout << " Representative ID : "
                << representativeId << "\n";
            cout << " IC Number         : "
                << newIcNo << "\n";
            cout << " Name              : "
                << newName << "\n";
            cout << " Phone No          : "
                << newPhoneNo << "\n";
            cout << " Street            : "
                << newStreet << "\n";
            cout << " City              : "
                << newCity << "\n";
            cout << " State             : "
                << newState << "\n";
            cout << " Postcode          : "
                << newPostcode << "\n";
            cout << "============================================================\n";
        }
        else
        {
            cout << "\nNo changes were made to the "
                << "representative record.\n";
        }

        // ==========================================================
        // CLEANUP
        // ==========================================================

        delete duplicateResult;
        duplicateResult = nullptr;

        delete duplicateStatement;
        duplicateStatement = nullptr;

        delete result;
        result = nullptr;

        delete searchStatement;
        searchStatement = nullptr;

        delete updateStatement;
        updateStatement = nullptr;

    }
    catch (sql::SQLException& e)
    {
        delete duplicateResult;
        duplicateResult = nullptr;

        delete duplicateStatement;
        duplicateStatement = nullptr;

        delete result;
        result = nullptr;

        delete searchStatement;
        searchStatement = nullptr;

        delete updateStatement;
        updateStatement = nullptr;

        cerr << "\nSQL Error while updating representative:\n";
        cerr << "Message    : " << e.what() << "\n";
        cerr << "Error Code : " << e.getErrorCode() << "\n";
        cerr << "SQL State  : " << e.getSQLState() << "\n";
    }
}

// ==========================================================
// LINK REPRESENTATIVE TO CHILD
// ==========================================================

void linkRepresentativeToChild(sql::Connection* con)
{
    string representativeId;
    string childId;
    string relationship;

    sql::PreparedStatement* representativeStatement = nullptr;
    sql::PreparedStatement* childStatement = nullptr;
    sql::PreparedStatement* linkCheckStatement = nullptr;
    sql::PreparedStatement* insertStatement = nullptr;

    sql::ResultSet* representativeResult = nullptr;
    sql::ResultSet* childResult = nullptr;
    sql::ResultSet* linkResult = nullptr;

    cout << "\n";
    cout << "============================================================\n";
    cout << "            ASSIGN AUTHORIZED REPRESENTATIVE TO CHILD       \n";
    cout << "============================================================\n";

    clearInputBuffer();

    if (!displayAvailableRepresentativeRecords(con))
    {
        cout << "\nNOTICE: No representative record is available for assignment.\n";
        return;
    }

    if (!displayAvailableRepresentativeChildRecords(con))
    {
        cout << "\nNOTICE: No child record is available for assignment.\n";
        return;
    }

    cout << " Enter Representative ID : ";
    getline(cin, representativeId);

    // Convert Representative ID to uppercase
    for (char& character : representativeId)
    {
        character = static_cast<char>(
            std::toupper(
                static_cast<unsigned char>(character)
            )
            );
    }

    if (!isValidRepresentativeId(representativeId))
    {
        cout << "\nERROR: Invalid Representative ID format.\n";
        cout << "Expected format: R0001\n";
        return;
    }

    cout << " Enter Child ID          : ";
    getline(cin, childId);

    // Convert Child ID to uppercase
    for (char& character : childId)
    {
        character = static_cast<char>(
            std::toupper(
                static_cast<unsigned char>(character)
            )
            );
    }

    if (!isValidRepresentativeChildId(childId))
    {
        cout << "\nERROR: Invalid Child ID format.\n";
        cout << "Expected format: C0001\n";
        return;
    }

    try
    {
        // ==================================================
        // VERIFY REPRESENTATIVE RECORD
        // ==================================================

        representativeStatement = con->prepareStatement(
            "SELECT Rep_ID, Name "
            "FROM AUTHORIZED_REPRESENTATIVE "
            "WHERE Rep_ID = ?"
        );

        representativeStatement->setString(
            1,
            representativeId
        );

        representativeResult =
            representativeStatement->executeQuery();

        if (!representativeResult->next())
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "          AUTHORIZED REPRESENTATIVE NOT FOUND              \n";
            cout << "============================================================\n";
            cout << " No record was found for Representative ID: "
                << representativeId << "\n";
            cout << "============================================================\n";

            delete representativeResult;
            delete representativeStatement;

            return;
        }

        const string representativeName =
            string(
                representativeResult->getString(
                    "Name"
                ).c_str()
            );

        // ==================================================
        // VERIFY CHILD RECORD
        // ==================================================

        childStatement = con->prepareStatement(
            "SELECT Child_ID, Name "
            "FROM CHILD "
            "WHERE Child_ID = ?"
        );

        childStatement->setString(1, childId);

        childResult =
            childStatement->executeQuery();

        if (!childResult->next())
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "                 CHILD RECORD NOT FOUND                    \n";
            cout << "============================================================\n";
            cout << " No child record was found for Child ID: "
                << childId << "\n";
            cout << "============================================================\n";

            delete representativeResult;
            delete representativeStatement;

            delete childResult;
            delete childStatement;

            return;
        }

        const string childName =
            string(
                childResult->getString("Name").c_str()
            );

        // ==================================================
        // DISPLAY RELATIONSHIP OPTIONS
        // ==================================================

        cout << "\n Representative ID   : "
            << representativeId << "\n";

        cout << " Representative Name : "
            << representativeName << "\n";

        cout << " Child ID            : "
            << childId << "\n";

        cout << " Child Name          : "
            << childName << "\n";

        cout << "\n Relationship:\n";
        cout << " 1. UNCLE\n";
        cout << " 2. AUNT\n";
        cout << " 3. BROTHER\n";
        cout << " 4. SISTER\n";
        cout << " 5. GRANDFATHER\n";
        cout << " 6. GRANDMOTHER\n";
        cout << " 7. NEIGHBOUR\n";
        cout << " 8. OTHER\n";

        int relationshipChoice = 0;

        cout << "\n Select Relationship [1-8] : ";

        if (!(cin >> relationshipChoice))
        {
            cin.clear();
            clearInputBuffer();

            cout << "\nERROR: Please enter a number "
                << "from 1 to 8.\n";

            delete representativeResult;
            delete representativeStatement;

            delete childResult;
            delete childStatement;

            return;
        }

        switch (relationshipChoice)
        {
        case 1:
            relationship = "UNCLE";
            break;

        case 2:
            relationship = "AUNT";
            break;

        case 3:
            relationship = "BROTHER";
            break;

        case 4:
            relationship = "SISTER";
            break;

        case 5:
            relationship = "GRANDFATHER";
            break;

        case 6:
            relationship = "GRANDMOTHER";
            break;

        case 7:
            relationship = "NEIGHBOUR";
            break;

        case 8:
            relationship = "OTHER";
            break;

        default:
            cout << "\nERROR: Please select an option "
                << "from 1 to 8.\n";

            delete representativeResult;
            delete representativeStatement;

            delete childResult;
            delete childStatement;

            return;
        }

        if (!isValidRepresentativeRelationship(
            relationship
        ))
        {
            cout << "\nERROR: Invalid relationship.\n";

            delete representativeResult;
            delete representativeStatement;

            delete childResult;
            delete childStatement;

            return;
        }

        // ==================================================
        // CHECK EXISTING AUTHORIZATION
        // ==================================================

        linkCheckStatement = con->prepareStatement(
            "SELECT Rep_ID, Child_ID, Relationship "
            "FROM AUTHOREP_CHILD "
            "WHERE Rep_ID = ? "
            "AND Child_ID = ?"
        );

        linkCheckStatement->setString(
            1,
            representativeId
        );

        linkCheckStatement->setString(
            2,
            childId
        );

        linkResult =
            linkCheckStatement->executeQuery();

        if (linkResult->next())
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "               AUTHORIZATION ALREADY EXISTS                \n";
            cout << "============================================================\n";
            cout << " Representative ID : "
                << representativeId << "\n";
            cout << " Child ID          : "
                << childId << "\n";
            cout << " Relationship      : "
                << linkResult->getString(
                    "Relationship"
                )
                << "\n";
            cout << "============================================================\n";

            delete representativeResult;
            delete representativeStatement;

            delete childResult;
            delete childStatement;

            delete linkResult;
            delete linkCheckStatement;

            return;
        }

        // ==================================================
        // CREATE AUTHOREP_CHILD RECORD
        // ==================================================

        insertStatement = con->prepareStatement(
            "INSERT INTO AUTHOREP_CHILD "
            "(Rep_ID, Child_ID, Relationship) "
            "VALUES (?, ?, ?)"
        );

        insertStatement->setString(
            1,
            representativeId
        );

        insertStatement->setString(
            2,
            childId
        );

        insertStatement->setString(
            3,
            relationship
        );

        const int affectedRows =
            insertStatement->executeUpdate();

        if (affectedRows > 0)
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "            REPRESENTATIVE ASSIGNMENT SUCCESSFUL            \n";
            cout << "============================================================\n";
            cout << " Representative ID   : "
                << representativeId << "\n";
            cout << " Representative Name : "
                << representativeName << "\n";
            cout << " Child ID            : "
                << childId << "\n";
            cout << " Child Name          : "
                << childName << "\n";
            cout << " Relationship        : "
                << relationship << "\n";
            cout << "============================================================\n";
        }
        else
        {
            cout << "\nThe authorization link was not created.\n";
        }

        // ==================================================
        // CLEANUP
        // ==================================================

        delete representativeResult;
        representativeResult = nullptr;

        delete representativeStatement;
        representativeStatement = nullptr;

        delete childResult;
        childResult = nullptr;

        delete childStatement;
        childStatement = nullptr;

        delete linkResult;
        linkResult = nullptr;

        delete linkCheckStatement;
        linkCheckStatement = nullptr;

        delete insertStatement;
        insertStatement = nullptr;
    }
    catch (sql::SQLException& e)
    {
        delete representativeResult;
        representativeResult = nullptr;

        delete representativeStatement;
        representativeStatement = nullptr;

        delete childResult;
        childResult = nullptr;

        delete childStatement;
        childStatement = nullptr;

        delete linkResult;
        linkResult = nullptr;

        delete linkCheckStatement;
        linkCheckStatement = nullptr;

        delete insertStatement;
        insertStatement = nullptr;

        cerr << "\nSQL Error while linking representative "
            << "to child:\n";

        cerr << "Message    : "
            << e.what() << "\n";

        cerr << "Error Code : "
            << e.getErrorCode() << "\n";

        cerr << "SQL State  : "
            << e.getSQLState() << "\n";
    }
}

// ==========================================================
// REMOVE REPRESENTATIVE AUTHORIZATION
// ==========================================================

void removeAuthorization(sql::Connection* con)
{
    string representativeId;
    string childId;

    sql::PreparedStatement* representativeStatement = nullptr;
    sql::PreparedStatement* childStatement = nullptr;
    sql::PreparedStatement* authorizationStatement = nullptr;
    sql::PreparedStatement* deleteStatement = nullptr;

    sql::ResultSet* representativeResult = nullptr;
    sql::ResultSet* childResult = nullptr;
    sql::ResultSet* authorizationResult = nullptr;

    cout << "\n";
    cout << "============================================================\n";
    cout << "                 REMOVE AUTHORIZATION                       \n";
    cout << "============================================================\n";

    clearInputBuffer();

    if (!displayAvailableRepresentativeRecords(con))
    {
        cout << "\nNOTICE: No representative record is available for authorization removal.\n";
        return;
    }

    if (!displayAvailableRepresentativeChildRecords(con))
    {
        cout << "\nNOTICE: No child record is available for authorization removal.\n";
        return;
    }

    cout << " Enter Representative ID : ";
    getline(cin, representativeId);

    // Convert Representative ID to uppercase
    for (char& character : representativeId)
    {
        character = static_cast<char>(
            std::toupper(
                static_cast<unsigned char>(character)
            )
            );
    }

    if (!isValidRepresentativeId(representativeId))
    {
        cout << "\nERROR: Invalid Representative ID format.\n";
        cout << "Expected format: R0001\n";
        return;
    }

    cout << " Enter Child ID          : ";
    getline(cin, childId);

    // Convert Child ID to uppercase
    for (char& character : childId)
    {
        character = static_cast<char>(
            std::toupper(
                static_cast<unsigned char>(character)
            )
            );
    }

    if (!isValidRepresentativeChildId(childId))
    {
        cout << "\nERROR: Invalid Child ID format.\n";
        cout << "Expected format: C0001\n";
        return;
    }

    try
    {
        // ==================================================
        // VERIFY REPRESENTATIVE
        // ==================================================

        representativeStatement = con->prepareStatement(
            "SELECT Rep_ID, Name "
            "FROM AUTHORIZED_REPRESENTATIVE "
            "WHERE Rep_ID = ?"
        );

        representativeStatement->setString(
            1,
            representativeId
        );

        representativeResult =
            representativeStatement->executeQuery();

        if (!representativeResult->next())
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "          AUTHORIZED REPRESENTATIVE NOT FOUND              \n";
            cout << "============================================================\n";
            cout << " No record was found for Representative ID: "
                << representativeId << "\n";
            cout << "============================================================\n";

            delete representativeResult;
            delete representativeStatement;

            return;
        }

        const string representativeName =
            string(
                representativeResult->getString(
                    "Name"
                ).c_str()
            );

        // ==================================================
        // VERIFY CHILD
        // ==================================================

        childStatement = con->prepareStatement(
            "SELECT Child_ID, Name "
            "FROM CHILD "
            "WHERE Child_ID = ?"
        );

        childStatement->setString(1, childId);

        childResult = childStatement->executeQuery();

        if (!childResult->next())
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "                 CHILD RECORD NOT FOUND                    \n";
            cout << "============================================================\n";
            cout << " No child record was found for Child ID: "
                << childId << "\n";
            cout << "============================================================\n";

            delete representativeResult;
            delete representativeStatement;

            delete childResult;
            delete childStatement;

            return;
        }

        const string childName =
            string(
                childResult->getString("Name").c_str()
            );

        // ==================================================
        // FIND EXISTING AUTHORIZATION
        // ==================================================

        authorizationStatement = con->prepareStatement(
            "SELECT Rep_ID, Child_ID, Relationship "
            "FROM AUTHOREP_CHILD "
            "WHERE Rep_ID = ? "
            "AND Child_ID = ?"
        );

        authorizationStatement->setString(
            1,
            representativeId
        );

        authorizationStatement->setString(
            2,
            childId
        );

        authorizationResult =
            authorizationStatement->executeQuery();

        if (!authorizationResult->next())
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "               AUTHORIZATION NOT FOUND                     \n";
            cout << "============================================================\n";
            cout << " No authorization exists between:\n";
            cout << " Representative ID : "
                << representativeId << "\n";
            cout << " Child ID          : "
                << childId << "\n";
            cout << "============================================================\n";

            delete representativeResult;
            delete representativeStatement;

            delete childResult;
            delete childStatement;

            delete authorizationResult;
            delete authorizationStatement;

            return;
        }

        const string relationship =
            string(
                authorizationResult->getString(
                    "Relationship"
                ).c_str()
            );

        // ==================================================
        // DISPLAY CURRENT AUTHORIZATION
        // ==================================================

        cout << "\n";
        cout << "============================================================\n";
        cout << "                 CURRENT AUTHORIZATION                      \n";
        cout << "============================================================\n";
        cout << " Representative ID   : "
            << representativeId << "\n";
        cout << " Representative Name : "
            << representativeName << "\n";
        cout << " Child ID            : "
            << childId << "\n";
        cout << " Child Name          : "
            << childName << "\n";
        cout << " Relationship        : "
            << relationship << "\n";
        cout << "============================================================\n";

        // ==================================================
        // CONFIRM REMOVAL
        // ==================================================

        int confirmation = 0;

        cout << "\nAre you sure you want to remove "
            << "this authorization?\n";

        cout << " 1. YES\n";
        cout << " 2. NO\n";

        cout << "\n Select an option [1-2] : ";

        if (!(cin >> confirmation))
        {
            cin.clear();
            clearInputBuffer();

            cout << "\nERROR: Please enter either 1 or 2.\n";

            delete representativeResult;
            delete representativeStatement;

            delete childResult;
            delete childStatement;

            delete authorizationResult;
            delete authorizationStatement;

            return;
        }

        if (confirmation == 2)
        {
            cout << "\nAuthorization removal cancelled.\n";

            delete representativeResult;
            delete representativeStatement;

            delete childResult;
            delete childStatement;

            delete authorizationResult;
            delete authorizationStatement;

            return;
        }

        if (confirmation != 1)
        {
            cout << "\nERROR: Please select either 1 or 2.\n";

            delete representativeResult;
            delete representativeStatement;

            delete childResult;
            delete childStatement;

            delete authorizationResult;
            delete authorizationStatement;

            return;
        }

        // ==================================================
        // DELETE AUTHOREP_CHILD RECORD
        // ==================================================

        deleteStatement = con->prepareStatement(
            "DELETE FROM AUTHOREP_CHILD "
            "WHERE Rep_ID = ? "
            "AND Child_ID = ?"
        );

        deleteStatement->setString(
            1,
            representativeId
        );

        deleteStatement->setString(
            2,
            childId
        );

        const int affectedRows =
            deleteStatement->executeUpdate();

        if (affectedRows > 0)
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "              AUTHORIZATION REMOVAL SUCCESSFUL             \n";
            cout << "============================================================\n";
            cout << " Representative ID : "
                << representativeId << "\n";
            cout << " Child ID          : "
                << childId << "\n";
            cout << " Relationship      : "
                << relationship << "\n";
            cout << "============================================================\n";
        }
        else
        {
            cout << "\nAuthorization was not removed.\n";
        }

        // ==========================================================
        // CLEANUP
        // ==========================================================

        delete representativeResult;
        representativeResult = nullptr;

        delete representativeStatement;
        representativeStatement = nullptr;

        delete childResult;
        childResult = nullptr;

        delete childStatement;
        childStatement = nullptr;

        delete authorizationResult;
        authorizationResult = nullptr;

        delete authorizationStatement;
        authorizationStatement = nullptr;

        delete deleteStatement;
        deleteStatement = nullptr;
}
catch (sql::SQLException& e)
{
    delete representativeResult;
    representativeResult = nullptr;

    delete representativeStatement;
    representativeStatement = nullptr;

    delete childResult;
    childResult = nullptr;

    delete childStatement;
    childStatement = nullptr;

    delete authorizationResult;
    authorizationResult = nullptr;

    delete authorizationStatement;
    authorizationStatement = nullptr;

    delete deleteStatement;
    deleteStatement = nullptr;

    cerr << "\nSQL Error while removing authorization:\n";
    cerr << "Message    : " << e.what() << "\n";
    cerr << "Error Code : " << e.getErrorCode() << "\n";
    cerr << "SQL State  : " << e.getSQLState() << "\n";
}
}

// ==========================================================
// GENERATE REPRESENTATIVE ID
// ==========================================================

string generateRepresentativeId(sql::Connection* con)
{
    sql::Statement* statement = nullptr;
    sql::ResultSet* result = nullptr;

    int nextNumber = 1;

    try
    {
        statement = con->createStatement();

        result = statement->executeQuery(
            "SELECT MAX("
            "CAST(SUBSTRING(Rep_ID, 2) AS UNSIGNED)"
            ") AS Maximum_ID "
            "FROM AUTHORIZED_REPRESENTATIVE"
        );

        if (result->next() &&
            !result->isNull("Maximum_ID"))
        {
            nextNumber =
                result->getInt("Maximum_ID") + 1;
        }

        delete result;
        delete statement;
    }
    catch (...)
    {
        delete result;
        delete statement;
        throw;
    }

    stringstream idStream;

    idStream << 'R'
        << setw(4)
        << setfill('0')
        << nextNumber;

    return idStream.str();
}

// ==========================================================
// REPRESENTATIVE ID VALIDATION
// ==========================================================

bool isValidRepresentativeId(
    const string& representativeId
)
{
    if (representativeId.length() != 5 ||
        representativeId[0] != 'R')
    {
        return false;
    }

    for (size_t index = 1;
        index < representativeId.length();
        index++)
    {
        if (!std::isdigit(
            static_cast<unsigned char>(
                representativeId[index]
                )
        ))
        {
            return false;
        }
    }

    return true;
}

// ==========================================================
// REPRESENTATIVE IC-NUMBER VALIDATION
// ==========================================================

bool isValidRepresentativeIcNumber(
    const string& icNo
)
{
    if (icNo.length() != 12)
    {
        return false;
    }

    for (char character : icNo)
    {
        if (!std::isdigit(
            static_cast<unsigned char>(character)
        ))
        {
            return false;
        }
    }

    return true;
}

bool isValidRepresentativePhoneNumber(
    const string& phoneNo
)
{
    if (phoneNo.length() < 10 ||
        phoneNo.length() > 12)
    {
        return false;
    }

    for (char character : phoneNo)  
    {
        if (!isdigit(
            static_cast<unsigned char>(character)
        ))
        {
            return false;
        }
    }

    return true;
}

bool isValidRepresentativePostcode(
    const string& postcode
)
{
    if (postcode.length() != 5)
    {
        return false;
    }

    for (char character : postcode)
    {
        if (!isdigit(
            static_cast<unsigned char>(character)
        ))
        {
            return false;
        }
    }

    return true;
}

// ==========================================================
// REPRESENTATIVE RELATIONSHIP VALIDATION
// ==========================================================

bool isValidRepresentativeRelationship(
    const string& relationship
)
{
    return relationship == "UNCLE" ||
        relationship == "AUNT" ||
        relationship == "BROTHER" ||
        relationship == "SISTER" ||
        relationship == "GRANDFATHER" ||
        relationship == "GRANDMOTHER" ||
        relationship == "NEIGHBOUR" ||
        relationship == "OTHER";
}

// ==========================================================
// CHILD ID VALIDATION FOR REPRESENTATIVE MODULE
// ==========================================================

bool isValidRepresentativeChildId(
    const string& childId
)
{
    if (childId.length() != 5 ||
        childId[0] != 'C')
    {
        return false;
    }

    for (size_t index = 1;
        index < childId.length();
        index++)
    {
        if (!std::isdigit(
            static_cast<unsigned char>(
                childId[index]
                )
        ))
        {
            return false;
        }
    }

    return true;
}
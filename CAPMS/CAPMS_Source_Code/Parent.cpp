#include "Parent.h"

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
// DISPLAY AVAILABLE PARENT RECORDS
// ==========================================================
static bool displayAvailableParentRecords(sql::Connection* con)
{
    sql::PreparedStatement* statement = nullptr;
    sql::ResultSet* result = nullptr;
    try
    {
        statement = con->prepareStatement(
            "SELECT p.Parent_ID, p.Name, p.Phone_No, "
            "COUNT(pc.Child_ID) AS Linked_Children "
            "FROM PARENT p "
            "LEFT JOIN PARENT_CHILD pc ON p.Parent_ID = pc.Parent_ID "
            "GROUP BY p.Parent_ID, p.Name, p.Phone_No "
            "ORDER BY p.Name ASC"
        );
        result = statement->executeQuery();
        cout << "\n";
        cout << "================================================================================\n";
        cout << "                         AVAILABLE PARENT RECORDS                               \n";
        cout << "================================================================================\n";
        cout << left << setw(6) << "No." << setw(12) << "Parent ID"
             << setw(30) << "Parent Name" << setw(17) << "Phone No."
             << setw(12) << "Children" << "\n";
        cout << string(80, '=') << "\n";
        int count = 0;
        while (result->next())
        {
            count++;
            string name = string(result->getString("Name").c_str());
            if (name.length() > 28) name = name.substr(0, 25) + "...";
            cout << left << setw(6) << count
                 << setw(12) << result->getString("Parent_ID")
                 << setw(30) << name
                 << setw(17) << result->getString("Phone_No")
                 << setw(12) << result->getInt("Linked_Children") << "\n";
        }
        cout << string(80, '=') << "\n";
        if (count == 0) cout << " No parent records are currently available.\n";
        else cout << " Total Available Parents : " << count << "\n";
        cout << string(80, '=') << "\n";
        delete result;
        delete statement;
        return count > 0;
    }
    catch (sql::SQLException& e)
    {
        delete result;
        delete statement;
        cerr << "\nSQL Error while retrieving available parent records:\n";
        cerr << "Message    : " << e.what() << "\n";
        cerr << "Error Code : " << e.getErrorCode() << "\n";
        cerr << "SQL State  : " << e.getSQLState() << "\n";
        return false;
    }
}

// ==========================================================
// DISPLAY AVAILABLE CHILD RECORDS FOR ASSIGNMENT
// ==========================================================
static bool displayAvailableChildRecordsForParent(sql::Connection* con)
{
    sql::PreparedStatement* statement = nullptr;
    sql::ResultSet* result = nullptr;
    try
    {
        statement = con->prepareStatement(
            "SELECT Child_ID, Name FROM CHILD ORDER BY Name ASC"
        );
        result = statement->executeQuery();
        cout << "\n";
        cout << "============================================================\n";
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


// ==========================================================
// MANAGE PARENT MENU
// ==========================================================

void manageParent(sql::Connection* con)
{
    int choice = 0;
    do
    {
        cout << "\n";
        cout << "============================================================\n";
        cout << "                       MANAGE PARENT                        \n";
        cout << "============================================================\n";
        cout << " 1. Register New Parent\n";
        cout << " 2. Search Parent Record\n";
        cout << " 3. View All Parent Records\n";
        cout << " 4. Update Parent Record\n";
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
        case 1: addParent(con); break;
        case 2: searchParent(con); break;
        case 3: viewAllParents(con); break;
        case 4: updateParent(con); break;
        case 5: cout << "\nReturning to Admin Main Menu...\n"; break;
        default:
            cout << "\nERROR: Please select an option from 1 to 5.\n";
            break;
        }
    } while (choice != 5);
}
// ==========================================================
// ADD PARENT
// ==========================================================

void addParent(sql::Connection* con)
{
    string parentId;
    string name;
    string phoneNo;
    string street;
    string city;
    string state;
    string postcode;
    string occupation;

    sql::PreparedStatement* insertStatement = nullptr;

    cout << "\n";
    cout << "============================================================\n";
    cout << "                         ADD PARENT                         \n";
    cout << "============================================================\n";

    clearInputBuffer();

    // ======================================================
    // ENTER AND VALIDATE PARENT NAME
    // ======================================================

    while (true)
    {
        cout << " Enter Parent Name : ";
        getline(cin, name);

        if (name.empty())
        {
            cout << "\nERROR: Parent name must not be empty.\n";
            cout << "Please enter the parent name again.\n\n";
            continue;
        }

        if (name.length() > 200)
        {
            cout << "\nERROR: Parent name must not exceed "
                << "200 characters.\n";
            cout << "Please enter the parent name again.\n\n";
            continue;
        }

        break;
    }

    // ======================================================
    // ENTER AND VALIDATE PHONE NUMBER
    // ======================================================

    while (true)
    {
        cout << " Enter Phone No    : ";
        getline(cin, phoneNo);

        if (!isValidPhoneNumber(phoneNo))
        {
            cout << "\nERROR: Phone number must contain "
                << "between 10 and 12 numeric digits.\n";

            cout << "\nCurrent valid information:\n";
            cout << " Parent Name : " << name << "\n";

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
        cout << " Enter Street      : ";
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
        cout << " Enter City        : ";
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
        cout << " Enter State       : ";
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
        cout << " Enter Postcode    : ";
        getline(cin, postcode);

        if (!isValidPostcode(postcode))
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
    // ENTER AND VALIDATE OCCUPATION
    // ======================================================

    while (true)
    {
        cout << " Enter Occupation  : ";
        getline(cin, occupation);

        if (occupation.length() > 50)
        {
            cout << "\nERROR: Occupation must not exceed "
                << "50 characters.\n";
            cout << "Please enter the occupation again.\n\n";
            continue;
        }

        break;
    }

    // ======================================================
    // DISPLAY DETAILS FOR CONFIRMATION
    // ======================================================

    cout << "\n";
    cout << "============================================================\n";
    cout << "                 PARENT DETAILS CONFIRMATION               \n";
    cout << "============================================================\n";
    cout << " Name       : " << name << "\n";
    cout << " Phone No   : " << phoneNo << "\n";
    cout << " Street     : " << street << "\n";
    cout << " City       : " << city << "\n";
    cout << " State      : " << state << "\n";
    cout << " Postcode   : " << postcode << "\n";
    cout << " Occupation : "
        << (occupation.empty() ? "Not Provided" : occupation)
        << "\n";
    cout << "============================================================\n";

    int confirmation = 0;

    while (true)
    {
        cout << "\nConfirm parent registration:\n";
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

        if (confirmation == 1 || confirmation == 2)
        {
            break;
        }

        cout << "\nERROR: Please select either 1 or 2.\n";
    }

    if (confirmation == 2)
    {
        cout << "\nParent registration cancelled.\n";
        return;
    }

    // ======================================================
    // INSERT PARENT RECORD
    // ======================================================

    try
    {
        parentId = generateParentId(con);

        insertStatement = con->prepareStatement(
            "INSERT INTO PARENT "
            "(Parent_ID, Name, Phone_No, Street, City, "
            "State, Postcode, Occupation) "
            "VALUES (?, ?, ?, ?, ?, ?, ?, ?)"
        );

        insertStatement->setString(1, parentId);
        insertStatement->setString(2, name);
        insertStatement->setString(3, phoneNo);
        insertStatement->setString(4, street);
        insertStatement->setString(5, city);
        insertStatement->setString(6, state);
        insertStatement->setString(7, postcode);

        if (occupation.empty())
        {
            insertStatement->setNull(8, 0);
        }
        else
        {
            insertStatement->setString(8, occupation);
        }

        const int affectedRows =
            insertStatement->executeUpdate();

        if (affectedRows > 0)
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "              PARENT REGISTRATION SUCCESSFUL               \n";
            cout << "============================================================\n";
            cout << " Parent ID  : " << parentId << "\n";
            cout << " Name       : " << name << "\n";
            cout << " Phone No   : " << phoneNo << "\n";
            cout << " Street     : " << street << "\n";
            cout << " City       : " << city << "\n";
            cout << " State      : " << state << "\n";
            cout << " Postcode   : " << postcode << "\n";
            cout << " Occupation : "
                << (occupation.empty()
                    ? "Not Provided"
                    : occupation)
                << "\n";
            cout << "============================================================\n";
        }
        else
        {
            cout << "\nParent record was not created.\n";
        }

        delete insertStatement;
        insertStatement = nullptr;
    }
    catch (sql::SQLException& e)
    {
        delete insertStatement;
        insertStatement = nullptr;

        cerr << "\nSQL Error while adding parent:\n";
        cerr << "Message    : " << e.what() << "\n";
        cerr << "Error Code : " << e.getErrorCode() << "\n";
        cerr << "SQL State  : " << e.getSQLState() << "\n";
    }
}

// ==========================================================
// VIEW ALL PARENTS
// ==========================================================
void viewAllParents(sql::Connection* con)
{
    displayAvailableParentRecords(con);
}

// ==========================================================
// SEARCH / VIEW PARENT
// ==========================================================

void searchParent(sql::Connection* con)
{
    string parentId;

    sql::PreparedStatement* pstmt = nullptr;
    sql::ResultSet* result = nullptr;

    cout << "\n";
    cout << "============================================================\n";
    cout << "                   SEARCH / VIEW PARENT                    \n";
    cout << "============================================================\n";

    if (!displayAvailableParentRecords(con))
    {
        cout << "\nNOTICE: No parent record is available for selection.\n";
        return;
    }

    cout << " Enter Parent ID : ";
    getline(cin, parentId);

    // Convert Parent ID to uppercase
    for (char& character : parentId)
    {
        character = static_cast<char>(
            std::toupper(
                static_cast<unsigned char>(character)
            )
            );
    }

    // Validate Parent ID
    if (!isValidParentId(parentId))
    {
        cout << "\nERROR: Invalid Parent ID format.\n";
        cout << "Expected format: P0001\n";
        return;
    }

    try
    {
        pstmt = con->prepareStatement(
            "SELECT "
            "Parent_ID, Name, Phone_No, Street, City, "
            "State, Postcode, Occupation "
            "FROM PARENT "
            "WHERE Parent_ID = ?"
        );

        pstmt->setString(1, parentId);

        result = pstmt->executeQuery();

        if (result->next())
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "                      PARENT RECORD                        \n";
            cout << "============================================================\n";

            cout << " Parent ID  : "
                << result->getString("Parent_ID")
                << "\n";

            cout << " Name       : "
                << result->getString("Name")
                << "\n";

            cout << " Phone No   : "
                << result->getString("Phone_No")
                << "\n";

            cout << " Street     : "
                << result->getString("Street")
                << "\n";

            cout << " City       : "
                << result->getString("City")
                << "\n";

            cout << " State      : "
                << result->getString("State")
                << "\n";

            cout << " Postcode   : "
                << result->getString("Postcode")
                << "\n";

            cout << " Occupation : ";

            if (result->isNull("Occupation"))
            {
                cout << "Not Provided\n";
            }
            else
            {
                cout << result->getString("Occupation")
                    << "\n";
            }

            cout << "============================================================\n";
        }
        else
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "                PARENT RECORD NOT FOUND                    \n";
            cout << "============================================================\n";
            cout << " No parent record was found for Parent ID: "
                << parentId << "\n";
            cout << "============================================================\n";
        }

        delete result;
        result = nullptr;

        delete pstmt;
        pstmt = nullptr;
    }
    catch (sql::SQLException& e)
    {
        delete result;
        result = nullptr;

        delete pstmt;
        pstmt = nullptr;

        cerr << "\nSQL Error while searching for parent:\n";
        cerr << "Message    : " << e.what() << "\n";
        cerr << "Error Code : " << e.getErrorCode() << "\n";
        cerr << "SQL State  : " << e.getSQLState() << "\n";
    }
}

// ==========================================================
// UPDATE PARENT
// ==========================================================

void updateParent(sql::Connection* con)
{
    string parentId;

    string currentName;
    string currentPhoneNo;
    string currentStreet;
    string currentCity;
    string currentState;
    string currentPostcode;
    string currentOccupation;

    string newName;
    string newPhoneNo;
    string newStreet;
    string newCity;
    string newState;
    string newPostcode;
    string newOccupation;

    sql::PreparedStatement* searchStatement = nullptr;
    sql::PreparedStatement* updateStatement = nullptr;
    sql::ResultSet* result = nullptr;

    cout << "\n";
    cout << "============================================================\n";
    cout << "                       UPDATE PARENT                        \n";
    cout << "============================================================\n";

    if (!displayAvailableParentRecords(con))
    {
        cout << "\nNOTICE: No parent record is available for selection.\n";
        return;
    }

    cout << " Enter Parent ID : ";
    getline(cin, parentId);

    // Convert Parent ID to uppercase
    for (char& character : parentId)
    {
        character = static_cast<char>(
            std::toupper(
                static_cast<unsigned char>(character)
            )
            );
    }

    if (!isValidParentId(parentId))
    {
        cout << "\nERROR: Invalid Parent ID format.\n";
        cout << "Expected format: P0001\n";
        return;
    }

    try
    {
        // Search for the existing parent record
        searchStatement = con->prepareStatement(
            "SELECT "
            "Parent_ID, Name, Phone_No, Street, City, "
            "State, Postcode, Occupation "
            "FROM PARENT "
            "WHERE Parent_ID = ?"
        );

        searchStatement->setString(1, parentId);

        result = searchStatement->executeQuery();

        if (!result->next())
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "                PARENT RECORD NOT FOUND                    \n";
            cout << "============================================================\n";
            cout << " No parent record was found for Parent ID: "
                << parentId << "\n";
            cout << "============================================================\n";

            delete result;
            result = nullptr;

            delete searchStatement;
            searchStatement = nullptr;

            return;
        }

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

        if (result->isNull("Occupation"))
        {
            currentOccupation = "";
        }
        else
        {
            currentOccupation =
                string(result->getString("Occupation").c_str());
        }

        cout << "\n";
        cout << "============================================================\n";
        cout << "                  CURRENT PARENT DETAILS                   \n";
        cout << "============================================================\n";
        cout << " Parent ID  : " << parentId << "\n";
        cout << " Name       : " << currentName << "\n";
        cout << " Phone No   : " << currentPhoneNo << "\n";
        cout << " Street     : " << currentStreet << "\n";
        cout << " City       : " << currentCity << "\n";
        cout << " State      : " << currentState << "\n";
        cout << " Postcode   : " << currentPostcode << "\n";
        cout << " Occupation : "
            << (currentOccupation.empty()
                ? "Not Provided"
                : currentOccupation)
            << "\n";
        cout << "============================================================\n";

        cout << "\nPress ENTER without entering a value "
            << "to keep the current information.\n\n";

        cout << " Enter New Name       : ";
        getline(cin, newName);

        cout << " Enter New Phone No   : ";
        getline(cin, newPhoneNo);

        cout << " Enter New Street     : ";
        getline(cin, newStreet);

        cout << " Enter New City       : ";
        getline(cin, newCity);

        cout << " Enter New State      : ";
        getline(cin, newState);

        cout << " Enter New Postcode   : ";
        getline(cin, newPostcode);

        cout << " Enter New Occupation : ";
        getline(cin, newOccupation);

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

        /*
         * For Occupation:
         * Pressing Enter keeps the current value.
         * This version does not remove an existing occupation.
         */
        if (newOccupation.empty())
        {
            newOccupation = currentOccupation;
        }

        // Validate updated name
        if (newName.empty() || newName.length() > 200)
        {
            cout << "\nERROR: Parent name must contain "
                << "between 1 and 200 characters.\n";

            delete result;
            delete searchStatement;

            return;
        }

        // Validate updated phone number
        if (!isValidPhoneNumber(newPhoneNo))
        {
            cout << "\nERROR: Phone number must contain "
                << "between 10 and 12 numeric digits.\n";

            delete result;
            delete searchStatement;

            return;
        }

        // Validate updated street
        if (newStreet.empty() || newStreet.length() > 200)
        {
            cout << "\nERROR: Street must contain "
                << "between 1 and 200 characters.\n";

            delete result;
            delete searchStatement;

            return;
        }

        // Validate updated city
        if (newCity.empty() || newCity.length() > 100)
        {
            cout << "\nERROR: City must contain "
                << "between 1 and 100 characters.\n";

            delete result;
            delete searchStatement;

            return;
        }

        // Validate updated state
        if (newState.empty() || newState.length() > 50)
        {
            cout << "\nERROR: State must contain "
                << "between 1 and 50 characters.\n";

            delete result;
            delete searchStatement;

            return;
        }

        // Validate updated postcode
        if (!isValidPostcode(newPostcode))
        {
            cout << "\nERROR: Postcode must contain "
                << "exactly 5 numeric digits.\n";

            delete result;
            delete searchStatement;

            return;
        }

        // Validate updated occupation
        if (newOccupation.length() > 50)
        {
            cout << "\nERROR: Occupation must not exceed "
                << "50 characters.\n";

            delete result;
            delete searchStatement;

            return;
        }

        // Update the parent record
        updateStatement = con->prepareStatement(
            "UPDATE PARENT "
            "SET Name = ?, Phone_No = ?, Street = ?, "
            "City = ?, State = ?, Postcode = ?, "
            "Occupation = ? "
            "WHERE Parent_ID = ?"
        );

        updateStatement->setString(1, newName);
        updateStatement->setString(2, newPhoneNo);
        updateStatement->setString(3, newStreet);
        updateStatement->setString(4, newCity);
        updateStatement->setString(5, newState);
        updateStatement->setString(6, newPostcode);

        if (newOccupation.empty())
        {
            updateStatement->setNull(7, 0);
        }
        else
        {
            updateStatement->setString(7, newOccupation);
        }

        updateStatement->setString(8, parentId);

        const int affectedRows =
            updateStatement->executeUpdate();

        if (affectedRows > 0)
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "                 PARENT UPDATE SUCCESSFUL                  \n";
            cout << "============================================================\n";
            cout << " Parent ID  : " << parentId << "\n";
            cout << " Name       : " << newName << "\n";
            cout << " Phone No   : " << newPhoneNo << "\n";
            cout << " Street     : " << newStreet << "\n";
            cout << " City       : " << newCity << "\n";
            cout << " State      : " << newState << "\n";
            cout << " Postcode   : " << newPostcode << "\n";
            cout << " Occupation : "
                << (newOccupation.empty()
                    ? "Not Provided"
                    : newOccupation)
                << "\n";
            cout << "============================================================\n";
        }
        else
        {
            cout << "\nNo changes were made to the parent record.\n";
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

        cerr << "\nSQL Error while updating parent:\n";
        cerr << "Message    : " << e.what() << "\n";
        cerr << "Error Code : " << e.getErrorCode() << "\n";
        cerr << "SQL State  : " << e.getSQLState() << "\n";
    }
}

// ==========================================================
// LINK PARENT TO CHILD
// ==========================================================

void linkParentToChild(sql::Connection* con)
{
    string parentId;
    string childId;
    string relationshipType;

    sql::PreparedStatement* parentStatement = nullptr;
    sql::PreparedStatement* childStatement = nullptr;
    sql::PreparedStatement* linkCheckStatement = nullptr;
    sql::PreparedStatement* insertStatement = nullptr;

    sql::ResultSet* parentResult = nullptr;
    sql::ResultSet* childResult = nullptr;
    sql::ResultSet* linkResult = nullptr;

    cout << "\n";
    cout << "============================================================\n";
    cout << "                 ASSIGN PARENT/GUARDIAN TO CHILD            \n";
    cout << "============================================================\n";

    clearInputBuffer();

    if (!displayAvailableParentRecords(con))
    {
        cout << "\nNOTICE: No parent record is available for assignment.\n";
        return;
    }

    if (!displayAvailableChildRecordsForParent(con))
    {
        cout << "\nNOTICE: No child record is available for assignment.\n";
        return;
    }

    cout << " Enter Parent ID : ";
    getline(cin, parentId);

    // Convert Parent ID to uppercase
    for (char& character : parentId)
    {
        character = static_cast<char>(
            std::toupper(
                static_cast<unsigned char>(character)
            )
            );
    }

    if (!isValidParentId(parentId))
    {
        cout << "\nERROR: Invalid Parent ID format.\n";
        cout << "Expected format: P0001\n";
        return;
    }

    cout << " Enter Child ID  : ";
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

    if (!isValidChildId(childId))
    {
        cout << "\nERROR: Invalid Child ID format.\n";
        cout << "Expected format: C0001\n";
        return;
    }

    try
    {
        // ==================================================
        // CHECK WHETHER PARENT EXISTS
        // ==================================================

        parentStatement = con->prepareStatement(
            "SELECT Parent_ID, Name "
            "FROM PARENT "
            "WHERE Parent_ID = ?"
        );

        parentStatement->setString(1, parentId);

        parentResult =
            parentStatement->executeQuery();

        if (!parentResult->next())
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "                PARENT RECORD NOT FOUND                    \n";
            cout << "============================================================\n";
            cout << " No parent record was found for Parent ID: "
                << parentId << "\n";
            cout << "============================================================\n";

            delete parentResult;
            delete parentStatement;

            return;
        }

        const string parentName =
            string(parentResult->getString("Name").c_str());

        // ==================================================
        // CHECK WHETHER CHILD EXISTS
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

            delete parentResult;
            delete parentStatement;

            delete childResult;
            delete childStatement;

            return;
        }

        const string childName =
            string(childResult->getString("Name").c_str());

        // ==================================================
        // DISPLAY MATCHING RECORDS
        // ==================================================

        cout << "\n";
        cout << " Parent ID   : " << parentId << "\n";
        cout << " Parent Name : " << parentName << "\n";
        cout << " Child ID    : " << childId << "\n";
        cout << " Child Name  : " << childName << "\n";

        cout << "\nAvailable Relationship Types:\n";
        cout << " 1. MOTHER\n";
        cout << " 2. FATHER\n";
        cout << " 3. LEGAL GUARDIAN\n";

        int relationshipChoice = 0;

        cout << "\n Select Relationship Type [1-3] : ";

        if (!(cin >> relationshipChoice))
        {
            cin.clear();
            clearInputBuffer();

            cout << "\nERROR: Please enter a number from 1 to 3.\n";

            delete parentResult;
            delete parentStatement;
            delete childResult;
            delete childStatement;

            return;
        }

        switch (relationshipChoice)
        {
        case 1:
            relationshipType = "MOTHER";
            break;

        case 2:
            relationshipType = "FATHER";
            break;

        case 3:
            relationshipType = "LEGAL_GUARDIAN";
            break;

        default:
            cout << "\nERROR: Please select an option from 1 to 3.\n";

            delete parentResult;
            delete parentStatement;
            delete childResult;
            delete childStatement;

            return;
        }

        // Convert relationship to uppercase
        for (char& character : relationshipType)
        {
            character = static_cast<char>(
                std::toupper(
                    static_cast<unsigned char>(character)
                )
                );
        }

        if (!isValidParentRelationship(
            relationshipType
        ))
        {
            cout << "\nERROR: Invalid relationship type.\n";
            cout << "Allowed values are MOTHER, FATHER, "
                << "or LEGAL_GUARDIAN.\n";

            delete parentResult;
            delete parentStatement;

            delete childResult;
            delete childStatement;

            return;
        }

        // ==================================================
        // CHECK WHETHER LINK ALREADY EXISTS
        // ==================================================

        linkCheckStatement = con->prepareStatement(
            "SELECT Parent_ID, Child_ID "
            "FROM PARENT_CHILD "
            "WHERE Parent_ID = ? "
            "AND Child_ID = ?"
        );

        linkCheckStatement->setString(1, parentId);
        linkCheckStatement->setString(2, childId);

        linkResult =
            linkCheckStatement->executeQuery();

        if (linkResult->next())
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "                PARENT/GUARDIAN ALREADY ASSIGNED            \n";
            cout << "============================================================\n";
            cout << " Parent " << parentId << " is already linked to child " << childId << ".\n";
            cout << "============================================================\n";

            delete parentResult;
            delete parentStatement;

            delete childResult;
            delete childStatement;

            delete linkResult;
            delete linkCheckStatement;

            return;
        }

        // ==================================================
        // CREATE PARENT_CHILD RECORD
        // ==================================================

        insertStatement = con->prepareStatement(
            "INSERT INTO PARENT_CHILD "
            "(Parent_ID, Child_ID, Relationship_Type) "
            "VALUES (?, ?, ?)"
        );

        insertStatement->setString(1, parentId);
        insertStatement->setString(2, childId);
        insertStatement->setString(
            3,
            relationshipType
        );

        const int affectedRows =
            insertStatement->executeUpdate();

        if (affectedRows > 0)
        {
            cout << "\n";
            cout << "============================================================\n";
            cout << "               PARENT/GUARDIAN ASSIGNMENT SUCCESSFUL        \n";
            cout << "============================================================\n";
            cout << " Parent ID        : " << parentId << "\n";
            cout << " Parent Name      : " << parentName << "\n";
            cout << " Child ID         : " << childId << "\n";
            cout << " Child Name       : " << childName << "\n";
            cout << " Relationship     : "
                << relationshipType << "\n";
            cout << "============================================================\n";
        }
        else
        {
            cout << "\nThe parent-child link was not created.\n";
        }

        delete parentResult;
        parentResult = nullptr;

        delete parentStatement;
        parentStatement = nullptr;

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
        delete parentResult;
        delete parentStatement;

        delete childResult;
        delete childStatement;

        delete linkResult;
        delete linkCheckStatement;

        delete insertStatement;

        cerr << "\nSQL Error while linking parent to child:\n";
        cerr << "Message    : " << e.what() << "\n";
        cerr << "Error Code : " << e.getErrorCode() << "\n";
        cerr << "SQL State  : " << e.getSQLState() << "\n";
    }
}

// ==========================================================
// GENERATE PARENT ID
// ==========================================================

string generateParentId(sql::Connection* con)
{
    sql::Statement* stmt = nullptr;
    sql::ResultSet* result = nullptr;

    int nextNumber = 1;

    try
    {
        stmt = con->createStatement();

        result = stmt->executeQuery(
            "SELECT MAX("
            "CAST(SUBSTRING(Parent_ID, 2) AS UNSIGNED)"
            ") AS Maximum_ID "
            "FROM PARENT"
        );

        if (result->next() &&
            !result->isNull("Maximum_ID"))
        {
            nextNumber =
                result->getInt("Maximum_ID") + 1;
        }

        delete result;
        result = nullptr;

        delete stmt;
        stmt = nullptr;
    }
    catch (...)
    {
        delete result;
        result = nullptr;

        delete stmt;
        stmt = nullptr;

        throw;
    }

    stringstream idStream;

    idStream << 'P'
        << setw(4)
        << setfill('0')
        << nextNumber;

    return idStream.str();
}


// ==========================================================
// PARENT ID VALIDATION
// ==========================================================

bool isValidParentId(const string& parentId)
{
    if (parentId.length() != 5 ||
        parentId[0] != 'P')
    {
        return false;
    }

    for (size_t index = 1;
        index < parentId.length();
        index++)
    {
        if (!std::isdigit(
            static_cast<unsigned char>(
                parentId[index]
                )
        ))
        {
            return false;
        }
    }

    return true;
}

// ==========================================================
// CHILD ID VALIDATION
// ==========================================================

bool isValidChildId(const string& childId)
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

// ==========================================================
// PARENT RELATIONSHIP VALIDATION
// ==========================================================

bool isValidParentRelationship(
    const string& relationshipType
)
{
    return relationshipType == "MOTHER" ||
        relationshipType == "FATHER" ||
        relationshipType == "LEGAL_GUARDIAN";
}

// ==========================================================
// PHONE-NUMBER VALIDATION
// ==========================================================

bool isValidPhoneNumber(const string& phoneNo)
{
    if (phoneNo.length() < 10 ||
        phoneNo.length() > 12)
    {
        return false;
    }

    for (char character : phoneNo)
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


// ==========================================================
// POSTCODE VALIDATION
// ==========================================================

bool isValidPostcode(const string& postcode)
{
    if (postcode.length() != 5)
    {
        return false;
    }

    for (char character : postcode)
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

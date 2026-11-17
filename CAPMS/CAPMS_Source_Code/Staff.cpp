#include "Staff.h"
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
// DISPLAY AVAILABLE STAFF RECORDS
// ==========================================================

static bool displayAvailableStaffRecords(sql::Connection* con)
{
    sql::PreparedStatement* statement = nullptr;
    sql::ResultSet* result = nullptr;

    try
    {
        statement = con->prepareStatement(
            "SELECT Staff_ID, Name, Role, Account_Status "
            "FROM STAFF "
            "ORDER BY "
            "CASE Role "
            "WHEN 'ADMIN' THEN 1 "
            "WHEN 'TEACHER' THEN 2 "
            "ELSE 3 END, "
            "Name ASC"
        );

        result = statement->executeQuery();

        cout << "\n";
        cout << "==========================================================================\n";
        cout << "                       AVAILABLE STAFF RECORDS                            \n";
        cout << "==========================================================================\n";
        cout << left
             << setw(6) << "No."
             << setw(12) << "Staff ID"
             << setw(28) << "Staff Name"
             << setw(14) << "Role"
             << setw(14) << "Status"
             << "\n";
        cout << string(74, '=') << "\n";

        int recordNumber = 0;

        while (result->next())
        {
            recordNumber++;

            string staffName =
                string(result->getString("Name").c_str());

            if (staffName.length() > 26)
            {
                staffName = staffName.substr(0, 23) + "...";
            }

            cout << left
                 << setw(6) << recordNumber
                 << setw(12) << result->getString("Staff_ID")
                 << setw(28) << staffName
                 << setw(14) << result->getString("Role")
                 << setw(14) << result->getString("Account_Status")
                 << "\n";
        }

        cout << string(74, '=') << "\n";

        if (recordNumber == 0)
        {
            cout << " No staff records are currently available.\n";
            cout << string(74, '=') << "\n";
        }
        else
        {
            cout << " Total Available Staff : " << recordNumber << "\n";
            cout << string(74, '=') << "\n";
        }

        delete result;
        result = nullptr;
        delete statement;
        statement = nullptr;

        return recordNumber > 0;
    }
    catch (sql::SQLException& e)
    {
        delete result;
        delete statement;

        cerr << "\nSQL Error while retrieving available staff records:\n";
        cerr << "Message    : " << e.what() << "\n";
        cerr << "Error Code : " << e.getErrorCode() << "\n";
        cerr << "SQL State  : " << e.getSQLState() << "\n";

        return false;
    }
}

void manageStaff(sql::Connection* con)
{
	int choice = 0;

	do
	{
		cout << "\n";
		cout << "============================================================\n";
		cout << "                       MANAGE STAFF                         \n";
		cout << "============================================================\n";
		cout << " 1. Register New Staff\n";
		cout << " 2. Search Staff Record\n";
		cout << " 3. View All Staff Records\n";
		cout << " 4. Update Staff Record\n";
		cout << " 5. Manage Staff Account Status\n";
		cout << " 6. Back to Admin Main Menu\n";
		cout << "------------------------------------------------------------\n";
		cout << " Select an option [1-6] : ";

		if (!(cin >> choice))
		{
			cin.clear();
			clearInputBuffer();

			cout << "\nERROR: Please enter a number from 1 to 6.\n";
			continue;
		}

		switch (choice)
		{
		case 1:
			addStaff(con);
			break;

		case 2:
			searchStaff(con);
			break;

		case 3:
			viewAllStaff(con);
			break;

		case 4:
			updateStaff(con);
			break;

		case 5:
			manageStaffAccountStatus(con);
			break;

		case 6:
			cout << "\nReturning to Admin Main Menu...\n";
			break;

		default:
			cout << "\nERROR: Please select an option from 1 to 6.\n";
			break;
		}

	} while (choice != 6);
}

// ==========================================================
// ADD STAFF
// ==========================================================

void addStaff(sql::Connection* con)
{
	string staffId;
	string icNo;
	string name;
	string phoneNo;
	string email;
	string role;

	sql::PreparedStatement* duplicateStatement = nullptr;
	sql::PreparedStatement* insertStatement = nullptr;

	sql::ResultSet* duplicateResult = nullptr;

	cout << "\n";
	cout << "============================================================\n";
	cout << "                         ADD STAFF                          \n";
	cout << "============================================================\n";

	clearInputBuffer();

	// ======================================================
	// ENTER AND VALIDATE STAFF NAME
	// ======================================================

	while (true)
	{
		cout << " Enter Staff Name : ";
		getline(cin, name);

		if (name.empty())
		{
			cout << "\nERROR: Staff name must not be empty.\n";
			cout << "Please enter the staff name again.\n\n";
			continue;
		}

		if (name.length() > 200)
		{
			cout << "\nERROR: Staff name must not exceed "
				<< "200 characters.\n";
			cout << "Please enter the staff name again.\n\n";
			continue;
		}

		break;
	}

	// ======================================================
	// ENTER, VALIDATE AND CHECK STAFF IC
	// ======================================================

	while (true)
	{
		cout << " Enter IC Number  : ";
		getline(cin, icNo);

		if (!isValidStaffIcNumber(icNo))
		{
			cout << "\nERROR: IC number must contain "
				<< "exactly 12 numeric digits.\n";

			cout << "\nCurrent valid information:\n";
			cout << " Staff Name : " << name << "\n";

			cout << "\nPlease enter the IC number again.\n\n";
			continue;
		}

		try
		{
			duplicateStatement = con->prepareStatement(
				"SELECT Staff_ID, Name "
				"FROM STAFF "
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
				cout << " IC Number        : " << icNo << "\n";
				cout << " Existing Staff ID: "
					<< duplicateResult->getString(
						"Staff_ID"
					)
					<< "\n";
				cout << " Existing Name    : "
					<< duplicateResult->getString(
						"Name"
					)
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

			cerr << "\nSQL Error while checking Staff IC number:\n";
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
		cout << " Enter Phone No   : ";
		getline(cin, phoneNo);

		if (!isValidStaffPhoneNumber(phoneNo))
		{
			cout << "\nERROR: Phone number must contain "
				<< "between 10 and 12 numeric digits.\n";

			cout << "\nCurrent valid information:\n";
			cout << " Staff Name : " << name << "\n";
			cout << " IC Number  : " << icNo << "\n";

			cout << "\nPlease enter the phone number again.\n\n";
			continue;
		}

		break;
	}

	// ======================================================
	// ENTER, VALIDATE AND CHECK EMAIL
	// ======================================================

	while (true)
	{
		cout << " Enter Email      : ";
		getline(cin, email);

		if (!isValidStaffEmail(email))
		{
			cout << "\nERROR: Invalid email address.\n";
			cout << "Example format: name@domain.com\n";

			cout << "\nCurrent valid information:\n";
			cout << " Staff Name : " << name << "\n";
			cout << " IC Number  : " << icNo << "\n";
			cout << " Phone No   : " << phoneNo << "\n";

			cout << "\nPlease enter the email address again.\n\n";
			continue;
		}

		// Convert email to lowercase
		for (char& character : email)
		{
			character = static_cast<char>(
				std::tolower(
					static_cast<unsigned char>(character)
				)
				);
		}

		try
		{
			duplicateStatement = con->prepareStatement(
				"SELECT Staff_ID, Name "
				"FROM STAFF "
				"WHERE LOWER(Email) = LOWER(?)"
			);

			duplicateStatement->setString(1, email);

			duplicateResult =
				duplicateStatement->executeQuery();

			if (duplicateResult->next())
			{
				cout << "\n";
				cout << "============================================================\n";
				cout << "                 DUPLICATE EMAIL ADDRESS                   \n";
				cout << "============================================================\n";
				cout << " Email            : " << email << "\n";
				cout << " Existing Staff ID: "
					<< duplicateResult->getString(
						"Staff_ID"
					)
					<< "\n";
				cout << " Existing Name    : "
					<< duplicateResult->getString(
						"Name"
					)
					<< "\n";
				cout << "============================================================\n";
				cout << "Please enter a different email address.\n\n";

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

			cerr << "\nSQL Error while checking Staff email:\n";
			cerr << "Message    : " << e.what() << "\n";
			cerr << "Error Code : " << e.getErrorCode() << "\n";
			cerr << "SQL State  : " << e.getSQLState() << "\n";

			return;
		}
	}

	// ======================================================
	// ENTER AND VALIDATE STAFF ROLE
	// ======================================================

	while (true)
	{
		cout << " Enter Role       : ";
		getline(cin, role);

		for (char& character : role)
		{
			character = static_cast<char>(
				std::toupper(
					static_cast<unsigned char>(character)
				)
				);
		}

		if (!isValidStaffRole(role))
		{
			cout << "\nERROR: Role must be TEACHER or ADMIN.\n";

			cout << "\nCurrent valid information:\n";
			cout << " Staff Name : " << name << "\n";
			cout << " IC Number  : " << icNo << "\n";
			cout << " Phone No   : " << phoneNo << "\n";
			cout << " Email      : " << email << "\n";

			cout << "\nPlease enter the staff role again.\n\n";
			continue;
		}

		break;
	}

	// ======================================================
	// DISPLAY DETAILS FOR CONFIRMATION
	// ======================================================

	cout << "\n";
	cout << "============================================================\n";
	cout << "                  STAFF DETAILS CONFIRMATION               \n";
	cout << "============================================================\n";
	cout << " Name               : " << name << "\n";
	cout << " IC Number          : " << icNo << "\n";
	cout << " Phone No           : " << phoneNo << "\n";
	cout << " Email              : " << email << "\n";
	cout << " Role               : " << role << "\n";
	cout << " Account Status     : ACTIVE\n";
	cout << " Temporary Password : capms123\n";
	cout << "============================================================\n";

	// ======================================================
	// CONFIRM REGISTRATION
	// ======================================================

	int confirmation = 0;

	while (true)
	{
		cout << "\nConfirm staff registration:\n";
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
		cout << "\nStaff registration cancelled.\n";
		return;
	}

	// ======================================================
	// INSERT STAFF RECORD
	// ======================================================

	try
	{
		staffId = generateStaffId(con);

		/*
		 * Password and Account_Status use database defaults:
		 *
		 * Password       = capms123
		 * Account_Status = ACTIVE
		 */
		insertStatement = con->prepareStatement(
			"INSERT INTO STAFF "
			"(Staff_ID, IC_No, Name, Phone_No, Email, Role) "
			"VALUES (?, ?, ?, ?, ?, ?)"
		);

		insertStatement->setString(1, staffId);
		insertStatement->setString(2, icNo);
		insertStatement->setString(3, name);
		insertStatement->setString(4, phoneNo);
		insertStatement->setString(5, email);
		insertStatement->setString(6, role);

		const int affectedRows =
			insertStatement->executeUpdate();

		if (affectedRows > 0)
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "              STAFF REGISTRATION SUCCESSFUL                \n";
			cout << "============================================================\n";
			cout << " Staff ID           : " << staffId << "\n";
			cout << " IC Number          : " << icNo << "\n";
			cout << " Name               : " << name << "\n";
			cout << " Phone No           : " << phoneNo << "\n";
			cout << " Email              : " << email << "\n";
			cout << " Role               : " << role << "\n";
			cout << " Account Status     : ACTIVE\n";
			cout << " Temporary Password : capms123\n";
			cout << "============================================================\n";
		}
		else
		{
			cout << "\nStaff record was not created.\n";
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

		cerr << "\nSQL Error while adding Staff:\n";
		cerr << "Message    : " << e.what() << "\n";
		cerr << "Error Code : " << e.getErrorCode() << "\n";
		cerr << "SQL State  : " << e.getSQLState() << "\n";
	}
}

// ==========================================================
// SEARCH / VIEW STAFF
// ==========================================================

void searchStaff(sql::Connection* con)
{
	string staffId;

	sql::PreparedStatement* pstmt = nullptr;
	sql::ResultSet* res = nullptr;

	cout << "\n";
	cout << "============================================================\n";
	cout << "                    SEARCH / VIEW STAFF                    \n";
	cout << "============================================================\n";

	clearInputBuffer();

	if (!displayAvailableStaffRecords(con))
	{
		cout << "\nNOTICE: No staff record is available for selection.\n";
		return;
	}

	cout << " Enter Staff ID : ";
	getline(cin, staffId);

	// Convert Staff ID to uppercase
	for (char& character : staffId)
	{
		character = static_cast<char>(
			std::toupper(
				static_cast<unsigned char>(character)
			)
			);
	}

	// Basic Staff ID validation
	if (staffId.empty())
	{
		cout << "\nERROR: Staff ID must not be empty.\n";
		return;
	}

	if (staffId.length() != 5 ||
		staffId[0] != 'S')
	{
		cout << "\nERROR: Invalid Staff ID format.\n";
		cout << "Expected format: S0001\n";
		return;
	}

	for (
		size_t index = 1;
		index < staffId.length();
		index++
		)
	{
		if (!std::isdigit(
			static_cast<unsigned char>(
				staffId[index]
				)
		))
		{
			cout << "\nERROR: Invalid Staff ID format.\n";
			cout << "Expected format: S0001\n";
			return;
		}
	}

	try
	{
		pstmt = con->prepareStatement(
			"SELECT "
			"Staff_ID, IC_No, Name, Phone_No, Email, "
			"Role, Account_Status "
			"FROM STAFF "
			"WHERE Staff_ID = ?"
		);

		pstmt->setString(1, staffId);

		res = pstmt->executeQuery();

		if (res->next())
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                       STAFF RECORD                        \n";
			cout << "============================================================\n";

			cout << " Staff ID       : "
				<< res->getString("Staff_ID") << "\n";

			cout << " IC Number      : "
				<< res->getString("IC_No") << "\n";

			cout << " Name           : "
				<< res->getString("Name") << "\n";

			cout << " Phone No       : "
				<< res->getString("Phone_No") << "\n";

			cout << " Email          : "
				<< res->getString("Email") << "\n";

			cout << " Role           : "
				<< res->getString("Role") << "\n";

			cout << " Account Status : "
				<< res->getString(
					"Account_Status"
				)
				<< "\n";

			cout << "============================================================\n";
		}
		else
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                 STAFF RECORD NOT FOUND                    \n";
			cout << "============================================================\n";
			cout << " No staff record was found for Staff ID: "
				<< staffId << "\n";
			cout << "============================================================\n";
		}

		delete res;
		res = nullptr;

		delete pstmt;
		pstmt = nullptr;
	}
	catch (sql::SQLException& e)
	{
		delete res;
		res = nullptr;

		delete pstmt;
		pstmt = nullptr;

		cerr << "\nSQL Error while searching for staff:\n";
		cerr << "Message    : " << e.what() << "\n";
		cerr << "Error Code : " << e.getErrorCode() << "\n";
		cerr << "SQL State  : " << e.getSQLState() << "\n";
	}
}

// ==========================================================
// VIEW ALL STAFF
// ==========================================================

void viewAllStaff(sql::Connection* con)
{
	sql::PreparedStatement* staffStatement = nullptr;
	sql::ResultSet* staffResult = nullptr;

	cout << "\n";
	cout << "============================================================\n";
	cout << "                     VIEW ALL STAFF                        \n";
	cout << "============================================================\n";

	try
	{
		staffStatement = con->prepareStatement(
			"SELECT "
			"Staff_ID, IC_No, Name, Phone_No, Email, "
			"Role, Account_Status "
			"FROM STAFF "
			"ORDER BY "
			"CASE Role "
			"WHEN 'ADMIN' THEN 1 "
			"WHEN 'TEACHER' THEN 2 "
			"ELSE 3 END, "
			"Name ASC"
		);

		staffResult =
			staffStatement->executeQuery();

		int staffCount = 0;
		int activeCount = 0;
		int inactiveCount = 0;
		int adminCount = 0;
		int teacherCount = 0;

		while (staffResult->next())
		{
			staffCount++;

			const string role =
				string(
					staffResult->getString(
						"Role"
					).c_str()
				);

			const string accountStatus =
				string(
					staffResult->getString(
						"Account_Status"
					).c_str()
				);

			if (role == "ADMIN")
			{
				adminCount++;
			}
			else if (role == "TEACHER")
			{
				teacherCount++;
			}

			if (accountStatus == "ACTIVE")
			{
				activeCount++;
			}
			else if (accountStatus == "INACTIVE")
			{
				inactiveCount++;
			}

			cout << "\n";
			cout << "------------------------------------------------------------\n";
			cout << " Staff Number   : "
				<< staffCount << "\n";

			cout << " Staff ID       : "
				<< staffResult->getString(
					"Staff_ID"
				)
				<< "\n";

			cout << " IC Number      : "
				<< staffResult->getString(
					"IC_No"
				)
				<< "\n";

			cout << " Name           : "
				<< staffResult->getString(
					"Name"
				)
				<< "\n";

			cout << " Phone No       : "
				<< staffResult->getString(
					"Phone_No"
				)
				<< "\n";

			cout << " Email          : "
				<< staffResult->getString(
					"Email"
				)
				<< "\n";

			cout << " Role           : "
				<< role << "\n";

			cout << " Account Status : "
				<< accountStatus << "\n";
		}

		cout << "------------------------------------------------------------\n";

		if (staffCount == 0)
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                  NO STAFF RECORDS FOUND                   \n";
			cout << "============================================================\n";
			cout << " No staff records are currently available.\n";
			cout << "============================================================\n";
		}
		else
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                     STAFF SUMMARY                         \n";
			cout << "============================================================\n";
			cout << " Total Staff     : "
				<< staffCount << "\n";
			cout << " Total Admin     : "
				<< adminCount << "\n";
			cout << " Total Teachers  : "
				<< teacherCount << "\n";
			cout << " Active Accounts : "
				<< activeCount << "\n";
			cout << " Inactive Accounts: "
				<< inactiveCount << "\n";
			cout << "============================================================\n";
		}

		delete staffResult;
		staffResult = nullptr;

		delete staffStatement;
		staffStatement = nullptr;
	}
	catch (sql::SQLException& e)
	{
		delete staffResult;
		staffResult = nullptr;

		delete staffStatement;
		staffStatement = nullptr;

		cerr << "\nSQL Error while retrieving all staff records:\n";
		cerr << "Message    : " << e.what() << "\n";
		cerr << "Error Code : " << e.getErrorCode() << "\n";
		cerr << "SQL State  : " << e.getSQLState() << "\n";
	}
}

// ==========================================================
// UPDATE STAFF
// ==========================================================

void updateStaff(sql::Connection* con)
{
	string staffId;

	string currentIcNo;
	string currentName;
	string currentPhoneNo;
	string currentEmail;
	string currentRole;
	string currentAccountStatus;

	string newIcNo;
	string newName;
	string newPhoneNo;
	string newEmail;
	string newRole;

	sql::PreparedStatement* searchStatement = nullptr;
	sql::PreparedStatement* duplicateStatement = nullptr;
	sql::PreparedStatement* updateStatement = nullptr;

	sql::ResultSet* result = nullptr;
	sql::ResultSet* duplicateResult = nullptr;

	cout << "\n";
	cout << "============================================================\n";
	cout << "                        UPDATE STAFF                       \n";
	cout << "============================================================\n";

	clearInputBuffer();

	// ======================================================
	// ENTER AND VALIDATE STAFF ID
	// ======================================================

	if (!displayAvailableStaffRecords(con))
	{
		cout << "\nNOTICE: No staff record is available for selection.\n";
		return;
	}

	cout << " Enter Staff ID : ";
	getline(cin, staffId);

	for (char& character : staffId)
	{
		character = static_cast<char>(
			std::toupper(
				static_cast<unsigned char>(character)
			)
			);
	}

	if (staffId.length() != 5 ||
		staffId[0] != 'S')
	{
		cout << "\nERROR: Invalid Staff ID format.\n";
		cout << "Expected format: S0001\n";
		return;
	}

	for (
		size_t index = 1;
		index < staffId.length();
		index++
		)
	{
		if (!std::isdigit(
			static_cast<unsigned char>(
				staffId[index]
				)
		))
		{
			cout << "\nERROR: Invalid Staff ID format.\n";
			cout << "Expected format: S0001\n";
			return;
		}
	}

	try
	{
		// ==================================================
		// RETRIEVE EXISTING STAFF RECORD
		// ==================================================

		searchStatement = con->prepareStatement(
			"SELECT "
			"Staff_ID, IC_No, Name, Phone_No, Email, "
			"Role, Account_Status "
			"FROM STAFF "
			"WHERE Staff_ID = ?"
		);

		searchStatement->setString(1, staffId);

		result = searchStatement->executeQuery();

		if (!result->next())
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                 STAFF RECORD NOT FOUND                    \n";
			cout << "============================================================\n";
			cout << " No staff record was found for Staff ID: "
				<< staffId << "\n";
			cout << "============================================================\n";

			delete result;
			result = nullptr;

			delete searchStatement;
			searchStatement = nullptr;

			return;
		}

		currentIcNo =
			string(
				result->getString("IC_No").c_str()
			);

		currentName =
			string(
				result->getString("Name").c_str()
			);

		currentPhoneNo =
			string(
				result->getString("Phone_No").c_str()
			);

		currentEmail =
			string(
				result->getString("Email").c_str()
			);

		currentRole =
			string(
				result->getString("Role").c_str()
			);

		currentAccountStatus =
			string(
				result->getString(
					"Account_Status"
				).c_str()
			);

		delete result;
		result = nullptr;

		delete searchStatement;
		searchStatement = nullptr;

		// ==================================================
		// DISPLAY CURRENT STAFF DETAILS
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "                  CURRENT STAFF DETAILS                    \n";
		cout << "============================================================\n";
		cout << " Staff ID       : " << staffId << "\n";
		cout << " IC Number      : " << currentIcNo << "\n";
		cout << " Name           : " << currentName << "\n";
		cout << " Phone No       : " << currentPhoneNo << "\n";
		cout << " Email          : " << currentEmail << "\n";
		cout << " Role           : " << currentRole << "\n";
		cout << " Account Status : " << currentAccountStatus << "\n";
		cout << "============================================================\n";

		cout << "\nPress ENTER without entering a value "
			<< "to keep the current information.\n\n";

		// ==================================================
		// ENTER AND VALIDATE NEW IC NUMBER
		// ==================================================

		while (true)
		{
			cout << " Enter New IC Number : ";
			getline(cin, newIcNo);

			if (newIcNo.empty())
			{
				newIcNo = currentIcNo;
				break;
			}

			if (!isValidStaffIcNumber(newIcNo))
			{
				cout << "\nERROR: IC number must contain "
					<< "exactly 12 numeric digits.\n";

				cout << "Please enter the IC number again, "
					<< "or press ENTER to keep the current IC.\n\n";

				continue;
			}

			duplicateStatement = con->prepareStatement(
				"SELECT Staff_ID, Name "
				"FROM STAFF "
				"WHERE IC_No = ? "
				"AND Staff_ID <> ?"
			);

			duplicateStatement->setString(1, newIcNo);
			duplicateStatement->setString(2, staffId);

			duplicateResult =
				duplicateStatement->executeQuery();

			if (duplicateResult->next())
			{
				cout << "\n";
				cout << "============================================================\n";
				cout << "                   DUPLICATE IC NUMBER                     \n";
				cout << "============================================================\n";
				cout << " IC Number        : " << newIcNo << "\n";
				cout << " Existing Staff ID: "
					<< duplicateResult->getString(
						"Staff_ID"
					)
					<< "\n";
				cout << " Existing Name    : "
					<< duplicateResult->getString(
						"Name"
					)
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

		// ==================================================
		// ENTER AND VALIDATE NEW NAME
		// ==================================================

		while (true)
		{
			cout << " Enter New Name      : ";
			getline(cin, newName);

			if (newName.empty())
			{
				newName = currentName;
				break;
			}

			if (newName.length() > 200)
			{
				cout << "\nERROR: Staff name must not exceed "
					<< "200 characters.\n";

				cout << "Please enter the staff name again, "
					<< "or press ENTER to keep the current name.\n\n";

				continue;
			}

			break;
		}

		// ==================================================
		// ENTER AND VALIDATE NEW PHONE NUMBER
		// ==================================================

		while (true)
		{
			cout << " Enter New Phone No  : ";
			getline(cin, newPhoneNo);

			if (newPhoneNo.empty())
			{
				newPhoneNo = currentPhoneNo;
				break;
			}

			if (!isValidStaffPhoneNumber(newPhoneNo))
			{
				cout << "\nERROR: Phone number must contain "
					<< "between 10 and 12 numeric digits.\n";

				cout << "Please enter the phone number again, "
					<< "or press ENTER to keep the current number.\n\n";

				continue;
			}

			break;
		}

		// ==================================================
		// ENTER, VALIDATE AND CHECK NEW EMAIL
		// ==================================================

		while (true)
		{
			cout << " Enter New Email     : ";
			getline(cin, newEmail);

			if (newEmail.empty())
			{
				newEmail = currentEmail;
				break;
			}

			if (!isValidStaffEmail(newEmail))
			{
				cout << "\nERROR: Invalid email address.\n";
				cout << "Example format: name@domain.com\n";

				cout << "Please enter the email address again, "
					<< "or press ENTER to keep the current email.\n\n";

				continue;
			}

			for (char& character : newEmail)
			{
				character = static_cast<char>(
					std::tolower(
						static_cast<unsigned char>(character)
					)
					);
			}

			duplicateStatement = con->prepareStatement(
				"SELECT Staff_ID, Name "
				"FROM STAFF "
				"WHERE LOWER(Email) = LOWER(?) "
				"AND Staff_ID <> ?"
			);

			duplicateStatement->setString(1, newEmail);
			duplicateStatement->setString(2, staffId);

			duplicateResult =
				duplicateStatement->executeQuery();

			if (duplicateResult->next())
			{
				cout << "\n";
				cout << "============================================================\n";
				cout << "                 DUPLICATE EMAIL ADDRESS                   \n";
				cout << "============================================================\n";
				cout << " Email            : " << newEmail << "\n";
				cout << " Existing Staff ID: "
					<< duplicateResult->getString(
						"Staff_ID"
					)
					<< "\n";
				cout << " Existing Name    : "
					<< duplicateResult->getString(
						"Name"
					)
					<< "\n";
				cout << "============================================================\n";
				cout << "Please enter a different email address, "
					<< "or press ENTER to keep the current email.\n\n";

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

		// ==================================================
		// ENTER AND VALIDATE NEW ROLE
		// ==================================================

		while (true)
		{
			cout << " Enter New Role      : ";
			getline(cin, newRole);

			if (newRole.empty())
			{
				newRole = currentRole;
				break;
			}

			for (char& character : newRole)
			{
				character = static_cast<char>(
					std::toupper(
						static_cast<unsigned char>(character)
					)
					);
			}

			if (!isValidStaffRole(newRole))
			{
				cout << "\nERROR: Role must be TEACHER or ADMIN.\n";

				cout << "Please enter the role again, "
					<< "or press ENTER to keep the current role.\n\n";

				continue;
			}

			break;
		}

		// ==================================================
		// CHECK WHETHER ANY INFORMATION CHANGED
		// ==================================================

		const bool informationChanged =
			newIcNo != currentIcNo ||
			newName != currentName ||
			newPhoneNo != currentPhoneNo ||
			newEmail != currentEmail ||
			newRole != currentRole;

		if (!informationChanged)
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                    NO CHANGES MADE                        \n";
			cout << "============================================================\n";
			cout << " All current staff information was retained.\n";
			cout << "============================================================\n";

			return;
		}

		// ==================================================
		// DISPLAY UPDATE CONFIRMATION
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "                 UPDATED STAFF DETAILS                     \n";
		cout << "============================================================\n";
		cout << " Staff ID       : " << staffId << "\n";
		cout << " IC Number      : " << newIcNo << "\n";
		cout << " Name           : " << newName << "\n";
		cout << " Phone No       : " << newPhoneNo << "\n";
		cout << " Email          : " << newEmail << "\n";
		cout << " Role           : " << newRole << "\n";
		cout << " Account Status : " << currentAccountStatus << "\n";
		cout << "============================================================\n";

		int confirmation = 0;

		while (true)
		{
			cout << "\nConfirm staff update:\n";
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
			cout << "\nStaff update cancelled.\n";
			return;
		}

		// ==================================================
		// UPDATE STAFF RECORD
		// ==================================================

		updateStatement = con->prepareStatement(
			"UPDATE STAFF "
			"SET IC_No = ?, Name = ?, Phone_No = ?, "
			"Email = ?, Role = ? "
			"WHERE Staff_ID = ?"
		);

		updateStatement->setString(1, newIcNo);
		updateStatement->setString(2, newName);
		updateStatement->setString(3, newPhoneNo);
		updateStatement->setString(4, newEmail);
		updateStatement->setString(5, newRole);
		updateStatement->setString(6, staffId);

		const int affectedRows =
			updateStatement->executeUpdate();

		if (affectedRows > 0)
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                  STAFF UPDATE SUCCESSFUL                  \n";
			cout << "============================================================\n";
			cout << " Staff ID       : " << staffId << "\n";
			cout << " IC Number      : " << newIcNo << "\n";
			cout << " Name           : " << newName << "\n";
			cout << " Phone No       : " << newPhoneNo << "\n";
			cout << " Email          : " << newEmail << "\n";
			cout << " Role           : " << newRole << "\n";
			cout << " Account Status : " << currentAccountStatus << "\n";
			cout << "============================================================\n";
		}
		else
		{
			cout << "\nThe staff record was not updated.\n";
		}

		// ==================================================
		// CLEANUP
		// ==================================================

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

		cerr << "\nSQL Error while updating staff:\n";
		cerr << "Message    : " << e.what() << "\n";
		cerr << "Error Code : " << e.getErrorCode() << "\n";
		cerr << "SQL State  : " << e.getSQLState() << "\n";
	}
}

// ==========================================================
// MANAGE STAFF ACCOUNT STATUS
// ==========================================================

void manageStaffAccountStatus(
	sql::Connection* con
)
{
	string staffId;
	string staffName;
	string staffRole;
	string currentStatus;
	string newStatus;

	sql::PreparedStatement* searchStatement = nullptr;
	sql::PreparedStatement* updateStatement = nullptr;
	sql::ResultSet* result = nullptr;

	cout << "\n";
	cout << "============================================================\n";
	cout << "              MANAGE STAFF ACCOUNT STATUS                 \n";
	cout << "============================================================\n";

	clearInputBuffer();

	if (!displayAvailableStaffRecords(con))
	{
		cout << "\nNOTICE: No staff record is available for selection.\n";
		return;
	}

	cout << " Enter Staff ID : ";
	getline(cin, staffId);

	// Convert Staff ID to uppercase
	for (char& character : staffId)
	{
		character = static_cast<char>(
			std::toupper(
				static_cast<unsigned char>(character)
			)
			);
	}

	// Validate Staff ID format
	if (
		staffId.length() != 5 ||
		staffId[0] != 'S'
		)
	{
		cout << "\nERROR: Invalid Staff ID format.\n";
		cout << "Expected format: S0001\n";
		return;
	}

	for (
		size_t index = 1;
		index < staffId.length();
		index++
		)
	{
		if (!std::isdigit(
			static_cast<unsigned char>(
				staffId[index]
				)
		))
		{
			cout << "\nERROR: Invalid Staff ID format.\n";
			cout << "Expected format: S0001\n";
			return;
		}
	}

	try
	{
		// ==================================================
		// SEARCH STAFF RECORD
		// ==================================================

		searchStatement = con->prepareStatement(
			"SELECT "
			"Staff_ID, Name, Role, Account_Status "
			"FROM STAFF "
			"WHERE Staff_ID = ?"
		);

		searchStatement->setString(1, staffId);

		result = searchStatement->executeQuery();

		if (!result->next())
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                  STAFF RECORD NOT FOUND                   \n";
			cout << "============================================================\n";
			cout << " No staff record was found for Staff ID: "
				<< staffId << "\n";
			cout << "============================================================\n";

			delete result;
			result = nullptr;

			delete searchStatement;
			searchStatement = nullptr;

			return;
		}

		staffName =
			string(
				result->getString("Name").c_str()
			);

		staffRole =
			string(
				result->getString("Role").c_str()
			);

		currentStatus =
			string(
				result->getString(
					"Account_Status"
				).c_str()
			);

		// ==================================================
		// DISPLAY CURRENT ACCOUNT DETAILS
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "                 CURRENT STAFF ACCOUNT                    \n";
		cout << "============================================================\n";
		cout << " Staff ID       : " << staffId << "\n";
		cout << " Name           : " << staffName << "\n";
		cout << " Role           : " << staffRole << "\n";
		cout << " Account Status : " << currentStatus << "\n";
		cout << "============================================================\n";

		// ==================================================
		// SELECT NEW STATUS
		// ==================================================

		int statusChoice = 0;

		cout << "\n Select New Account Status:\n";
		cout << " 1. ACTIVE\n";
		cout << " 2. INACTIVE\n";
		cout << " 3. Cancel\n";
		cout << "\n Select an option [1-3] : ";

		if (!(cin >> statusChoice))
		{
			cin.clear();
			clearInputBuffer();

			cout << "\nERROR: Please enter a number "
				<< "from 1 to 3.\n";

			delete result;
			delete searchStatement;

			return;
		}

		switch (statusChoice)
		{
		case 1:
			newStatus = "ACTIVE";
			break;

		case 2:
			newStatus = "INACTIVE";
			break;

		case 3:
			cout << "\nAccount status update cancelled.\n";

			delete result;
			delete searchStatement;

			return;

		default:
			cout << "\nERROR: Please select an option "
				<< "from 1 to 3.\n";

			delete result;
			delete searchStatement;

			return;
		}

		// ==================================================
		// CHECK WHETHER STATUS IS ALREADY THE SAME
		// ==================================================

		if (newStatus == currentStatus)
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                  NO STATUS CHANGE                         \n";
			cout << "============================================================\n";
			cout << " Staff ID       : " << staffId << "\n";
			cout << " Account Status : " << currentStatus << "\n";
			cout << "\nThe account already has the selected status.\n";
			cout << "============================================================\n";

			delete result;
			delete searchStatement;

			return;
		}

		// ==================================================
		// CONFIRM STATUS CHANGE
		// ==================================================

		int confirmation = 0;

		cout << "\nConfirm account status change:\n";
		cout << " Current Status : " << currentStatus << "\n";
		cout << " New Status     : " << newStatus << "\n";
		cout << "\n 1. Confirm\n";
		cout << " 2. Cancel\n";
		cout << "\n Select an option [1-2] : ";

		if (!(cin >> confirmation))
		{
			cin.clear();
			clearInputBuffer();

			cout << "\nERROR: Please enter either 1 or 2.\n";

			delete result;
			delete searchStatement;

			return;
		}

		if (confirmation == 2)
		{
			cout << "\nAccount status update cancelled.\n";

			delete result;
			delete searchStatement;

			return;
		}

		if (confirmation != 1)
		{
			cout << "\nERROR: Please select either 1 or 2.\n";

			delete result;
			delete searchStatement;

			return;
		}

		// ==================================================
		// UPDATE ACCOUNT STATUS
		// ==================================================

		updateStatement = con->prepareStatement(
			"UPDATE STAFF "
			"SET Account_Status = ? "
			"WHERE Staff_ID = ?"
		);

		updateStatement->setString(1, newStatus);
		updateStatement->setString(2, staffId);

		const int affectedRows =
			updateStatement->executeUpdate();

		if (affectedRows > 0)
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "             ACCOUNT STATUS UPDATE SUCCESSFUL              \n";
			cout << "============================================================\n";
			cout << " Staff ID       : " << staffId << "\n";
			cout << " Name           : " << staffName << "\n";
			cout << " Role           : " << staffRole << "\n";
			cout << " Previous Status: " << currentStatus << "\n";
			cout << " Current Status : " << newStatus << "\n";
			cout << "============================================================\n";

			if (newStatus == "INACTIVE")
			{
				cout << "The staff member will not be able "
					<< "to log in to CAPMS.\n";
				cout << "Historical transaction records "
					<< "have been retained.\n";
				cout << "============================================================\n";
			}
		}
		else
		{
			cout << "\nThe account status was not updated.\n";
		}

		// ==================================================
		// CLEANUP
		// ==================================================

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
			<< "staff account status:\n";

		cerr << "Message    : "
			<< e.what() << "\n";

		cerr << "Error Code : "
			<< e.getErrorCode() << "\n";

		cerr << "SQL State  : "
			<< e.getSQLState() << "\n";
	}
}

string generateStaffId(sql::Connection* con)
{
	sql::Statement* stmt = nullptr;
	sql::ResultSet* res = nullptr;

	int nextNumber = 1;

	try
	{
		stmt = con->createStatement();

		res = stmt->executeQuery(
			"SELECT MAX("
			"CAST(SUBSTRING(Staff_ID, 2) AS UNSIGNED)"
			") AS Maximum_ID "
			"FROM STAFF"
		);

		if (res->next() && !res->isNull("Maximum_ID"))
		{
			nextNumber = res->getInt("Maximum_ID") + 1;
		}

		delete res;
		res = nullptr;

		delete stmt;
		stmt = nullptr;
	}
	catch (...)
	{
		delete res;
		res = nullptr;

		delete stmt;
		stmt = nullptr;

		throw;
	}

	stringstream idStream;

	idStream << 'S'
		<< setw(4)
		<< setfill('0')
		<< nextNumber;

	return idStream.str();
}

bool isValidStaffRole(const string& role)
{
	return role == "TEACHER" || role == "ADMIN";
}

// ==========================================================
// STAFF IC-NUMBER VALIDATION
// ==========================================================

bool isValidStaffIcNumber(
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

// ==========================================================
// STAFF EMAIL VALIDATION
// ==========================================================

bool isValidStaffEmail(
	const string& email
)
{
	if (email.empty() ||
		email.length() > 100)
	{
		return false;
	}

	if (email.find(' ') != string::npos)
	{
		return false;
	}

	const size_t atPosition =
		email.find('@');

	if (atPosition == string::npos ||
		atPosition == 0)
	{
		return false;
	}

	if (email.find('@', atPosition + 1) !=
		string::npos)
	{
		return false;
	}

	const size_t dotPosition =
		email.find('.', atPosition + 1);

	if (dotPosition == string::npos ||
		dotPosition == atPosition + 1 ||
		dotPosition == email.length() - 1)
	{
		return false;
	}

	return true;
}

bool isValidStaffPhoneNumber(
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
		if (!std::isdigit(
			static_cast<unsigned char>(character)
		))
		{
			return false;
		}
	}

	return true;
}
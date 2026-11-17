#include "Login.h"

#include "InputUtils.h"
#include "Menu.h"
#include "TeacherMenu.h"
#include "UserSession.h"

#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

#include <cctype>
#include <iostream>
#include <string>

using namespace std;


// ==========================================================
// LOGIN AND ROLE-BASED ACCESS
// ==========================================================

void displayLogin(sql::Connection* con)
{
	int loginMenuChoice = 0;

	do
	{
		cout << "\n";
		cout << "============================================================\n";
		cout << "          CHILDCARE ATTENDANCE AND PICKUP SYSTEM             \n";
		cout << "============================================================\n";
		cout << "                         CAPMS                              \n";
		cout << "------------------------------------------------------------\n";
		cout << " Secure Login and Role-Based Access\n";
		cout << "------------------------------------------------------------\n";
		cout << " 1. Login to CAPMS\n";
		cout << " 2. Exit System\n";
		cout << "------------------------------------------------------------\n";
		cout << " Select an option [1-2] : ";

		if (!(cin >> loginMenuChoice))
		{
			cin.clear();
			clearInputBuffer();

			cout << "\nERROR: Please enter either 1 or 2.\n";
			continue;
		}

		if (loginMenuChoice == 2)
		{
			cout << "\nThank you for using CAPMS.\nExiting system...\n";
			break;
		}

		if (loginMenuChoice != 1)
		{
			cout << "\nERROR: Please select either 1 or 2.\n";
			continue;
		}

		clearInputBuffer();

		string staffId;
		string password;

		sql::PreparedStatement* loginStatement = nullptr;
		sql::ResultSet* loginResult = nullptr;

		cout << "\n";
		cout << "============================================================\n";
		cout << "                         USER LOGIN                        \n";
		cout << "============================================================\n";

		cout << " Enter Staff ID : ";
		getline(cin, staffId);

		cout << " Enter Password : ";
		getline(cin, password);

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
			continue;
		}

		bool validDigits = true;

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
				validDigits = false;
				break;
			}
		}

		if (!validDigits)
		{
			cout << "\nERROR: Invalid Staff ID format.\n";
			cout << "Expected format: S0001\n";
			continue;
		}

		if (password.empty())
		{
			cout << "\nERROR: Password must not be empty.\n";
			continue;
		}

		try
		{
			loginStatement = con->prepareStatement(
				"SELECT "
				"Staff_ID, Name, Role, Password, "
				"Account_Status "
				"FROM STAFF "
				"WHERE Staff_ID = ?"
			);

			loginStatement->setString(1, staffId);

			loginResult =
				loginStatement->executeQuery();

			if (!loginResult->next())
			{
				cout << "\n";
				cout << "============================================================\n";
				cout << "                       LOGIN FAILED                         \n";
				cout << "============================================================\n";
				cout << " ERROR: Invalid Staff ID or password.\n";
				cout << "============================================================\n";

				delete loginResult;
				loginResult = nullptr;

				delete loginStatement;
				loginStatement = nullptr;

				continue;
			}

			const string storedPassword =
				string(
					loginResult->getString(
						"Password"
					).c_str()
				);

			const string accountStatus =
				string(
					loginResult->getString(
						"Account_Status"
					).c_str()
				);

			const string staffRole =
				string(
					loginResult->getString(
						"Role"
					).c_str()
				);

			const string staffName =
				string(
					loginResult->getString(
						"Name"
					).c_str()
				);

			// Validate password
			if (password != storedPassword)
			{
				cout << "\n";
				cout << "============================================================\n";
				cout << "                       LOGIN FAILED                         \n";
				cout << "============================================================\n";
				cout << " ERROR: Invalid Staff ID or password.\n";
				cout << "============================================================\n";

				delete loginResult;
				loginResult = nullptr;

				delete loginStatement;
				loginStatement = nullptr;

				continue;
			}

			// Block inactive account
			if (accountStatus != "ACTIVE")
			{
				cout << "\n";
				cout << "============================================================\n";
				cout << "                    ACCOUNT INACTIVE                        \n";
				cout << "============================================================\n";
				cout << " Staff ID : " << staffId << "\n";
				cout << " Name     : " << staffName << "\n";
				cout << " Status   : " << accountStatus << "\n";
				cout << "\nThis account is not permitted to access CAPMS.\n";
				cout << "Please contact the administrator.\n";
				cout << "============================================================\n";

				delete loginResult;
				loginResult = nullptr;

				delete loginStatement;
				loginStatement = nullptr;

				continue;
			}

			// Login success
			cout << "\n";
			cout << "============================================================\n";
			cout << "                      LOGIN SUCCESSFUL                     \n";
			cout << "============================================================\n";
			cout << " Welcome to CAPMS, " << staffName << ".\n";
			cout << " Staff ID : " << staffId << "\n";
			cout << " Role     : " << staffRole << "\n";
			cout << "============================================================\n";

			delete loginResult;
			loginResult = nullptr;

			delete loginStatement;
			loginStatement = nullptr;

			UserSession session;

			session.staffId = staffId;
			session.staffName = staffName;
			session.role = staffRole;

			// Open menu based on role
			if (staffRole == "ADMIN")
			{
				displayAdminMenu(con, session);
			}
			else if (staffRole == "TEACHER")
			{
				displayTeacherMenu(con, session);
			}
			else
			{
				cout << "\nERROR: The account has an unsupported role.\n";
			}

			cout << "\nNOTICE: You have returned to the CAPMS Login System.\n";
		}
		catch (sql::SQLException& e)
		{
			delete loginResult;
			loginResult = nullptr;

			delete loginStatement;
			loginStatement = nullptr;

			cerr << "\nSQL Error during login:\n";
			cerr << "Message    : " << e.what() << "\n";
			cerr << "Error Code : " << e.getErrorCode() << "\n";
			cerr << "SQL State  : " << e.getSQLState() << "\n";
		}

	} while (loginMenuChoice != 2);
}
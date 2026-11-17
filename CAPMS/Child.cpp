#include "Child.h"
#include "InputUtils.h"
#include "Parent.h"
#include "Representative.h"

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
// DISPLAY AVAILABLE CHILD RECORDS
// ==========================================================
static bool displayAvailableChildRecords(sql::Connection* con)
{
    sql::PreparedStatement* statement = nullptr;
    sql::ResultSet* result = nullptr;

    try
    {
        statement = con->prepareStatement(
            "SELECT c.Child_ID, c.Name, "
            "DATE_FORMAT(c.DOB, '%d/%m/%Y') AS Formatted_DOB, "
            "COUNT(DISTINCT pc.Parent_ID) AS Parent_Count, "
            "COUNT(DISTINCT ac.Rep_ID) AS Representative_Count "
            "FROM CHILD c "
            "LEFT JOIN PARENT_CHILD pc ON c.Child_ID = pc.Child_ID "
            "LEFT JOIN AUTHOREP_CHILD ac ON c.Child_ID = ac.Child_ID "
            "GROUP BY c.Child_ID, c.Name, c.DOB "
            "ORDER BY c.Name ASC"
        );

        result = statement->executeQuery();

        cout << "\n";
        cout << "==========================================================================================\n";
        cout << "                              AVAILABLE CHILD RECORDS                                     \n";
        cout << "==========================================================================================\n";
        cout << left
             << setw(6) << "No."
             << setw(12) << "Child ID"
             << setw(30) << "Child Name"
             << setw(16) << "Date of Birth"
             << setw(12) << "Parents"
             << setw(10) << "Reps"
             << "\n";
        cout << string(90, '=') << "\n";

        int recordCount = 0;

        while (result->next())
        {
            recordCount++;
            string childName = string(result->getString("Name").c_str());

            if (childName.length() > 28)
            {
                childName = childName.substr(0, 25) + "...";
            }

            cout << left
                 << setw(6) << recordCount
                 << setw(12) << result->getString("Child_ID")
                 << setw(30) << childName
                 << setw(16) << result->getString("Formatted_DOB")
                 << setw(12) << result->getInt("Parent_Count")
                 << setw(10) << result->getInt("Representative_Count")
                 << "\n";
        }

        cout << string(90, '=') << "\n";

        if (recordCount == 0)
        {
            cout << " No child records are currently available.\n";
        }
        else
        {
            cout << " Total Available Children : " << recordCount << "\n";
        }

        cout << string(90, '=') << "\n";

        delete result;
        result = nullptr;
        delete statement;
        statement = nullptr;

        return recordCount > 0;
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
// MANAGE CHILD MENU
// ==========================================================

void manageChild(sql::Connection* con)
{
    int choice = 0;

    do
    {
        cout << "\n";
        cout << "============================================================\n";
        cout << "                        MANAGE CHILD                        \n";
        cout << "============================================================\n";
        cout << " 1. Register New Child\n";
        cout << " 2. Search Child Record\n";
        cout << " 3. View All Child Records\n";
        cout << " 4. Update Child Record\n";
        cout << " 5. Manage Child Guardians and Representatives\n";
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

        clearInputBuffer();

        switch (choice)
        {
        case 1: addChild(con); break;
        case 2: searchChild(con); break;
        case 3: viewAllChildren(con); break;
        case 4: updateChild(con); break;
        case 5: manageChildGuardiansAndRepresentatives(con); break;
        case 6: cout << "\nReturning to Admin Main Menu...\n"; break;
        default:
            cout << "\nERROR: Please select an option from 1 to 6.\n";
            break;
        }
    } while (choice != 6);
}
// ==========================================================
// MANAGE CHILD GUARDIANS AND REPRESENTATIVES
// ==========================================================

void manageChildGuardiansAndRepresentatives(
	sql::Connection* con
)
{
	int choice = 0;

	do
	{
		cout << "\n";
		cout << "============================================================\n";
		cout << "       MANAGE CHILD GUARDIANS AND REPRESENTATIVES          \n";
		cout << "============================================================\n";
		cout << " 1. Assign Parent/Guardian to Child\n";
		cout << " 2. Assign Authorized Representative to Child\n";
		cout << " 3. View Assigned Guardians and Representatives\n";
		cout << " 4. Remove Representative Authorization\n";
		cout << " 5. Back to Manage Child\n";
		cout << "------------------------------------------------------------\n";
		cout << " Select an option [1-5] : ";

		if (!(cin >> choice))
		{
			cin.clear();
			clearInputBuffer();

			cout << "\nERROR: Please enter a number from 1 to 5.\n";

			continue;
		}

		switch (choice)
		{
		case 1:
			linkParentToChild(con);
			break;

		case 2:
			linkRepresentativeToChild(con);
			break;

		case 3:
			viewAssignedGuardiansAndRepresentatives(con);
			break;

		case 4:
			removeAuthorization(con);
			break;

		case 5:
			cout << "\nReturning to Manage Child...\n";
			break;

		default:
			cout << "\nERROR: Please select an option from 1 to 5.\n";
			break;
		}

	} while (choice != 5);
}

// ==========================================================
// VIEW ALL CHILDREN
// ==========================================================
void viewAllChildren(sql::Connection* con)
{
    displayAvailableChildRecords(con);
}

// ==========================================================
// VIEW ASSIGNED GUARDIANS AND REPRESENTATIVES
// ==========================================================

void viewAssignedGuardiansAndRepresentatives(
	sql::Connection* con
)
{
	string childId;
	string childName;
	string myKidNo;

	sql::PreparedStatement* childStatement = nullptr;
	sql::PreparedStatement* parentStatement = nullptr;
	sql::PreparedStatement* representativeStatement = nullptr;

	sql::ResultSet* childResult = nullptr;
	sql::ResultSet* parentResult = nullptr;
	sql::ResultSet* representativeResult = nullptr;

	cout << "\n";
	cout << "============================================================\n";
	cout << "       VIEW ASSIGNED GUARDIANS AND REPRESENTATIVES         \n";
	cout << "============================================================\n";

	clearInputBuffer();

	if (!displayAvailableChildRecords(con))
	{
		cout << "\nNOTICE: No child record is available for selection.\n";
		return;
	}

	cout << " Enter Child ID : ";
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

	// Validate Child ID
	if (!isValidChildId(childId))
	{
		cout << "\nERROR: Invalid Child ID format.\n";
		cout << "Expected format: C0001\n";
		return;
	}

	try
	{
		// ==================================================
		// VERIFY CHILD RECORD
		// ==================================================

		childStatement = con->prepareStatement(
			"SELECT Child_ID, MyKid_No, Name "
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

			delete childResult;
			childResult = nullptr;

			delete childStatement;
			childStatement = nullptr;

			return;
		}

		childName =
			string(
				childResult->getString("Name").c_str()
			);

		myKidNo =
			string(
				childResult->getString("MyKid_No").c_str()
			);

		// ==================================================
		// DISPLAY CHILD INFORMATION
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "                      CHILD DETAILS                        \n";
		cout << "============================================================\n";
		cout << " Child ID     : " << childId << "\n";
		cout << " Child Name   : " << childName << "\n";
		cout << " MyKid Number : " << myKidNo << "\n";
		cout << "============================================================\n";

		// ==================================================
		// RETRIEVE ASSIGNED PARENTS AND GUARDIANS
		// ==================================================

		parentStatement = con->prepareStatement(
			"SELECT "
			"p.Parent_ID, "
			"p.Name, "
			"p.Phone_No, "
			"pc.Relationship_Type "
			"FROM PARENT_CHILD pc "
			"JOIN PARENT p "
			"ON pc.Parent_ID = p.Parent_ID "
			"WHERE pc.Child_ID = ? "
			"ORDER BY "
			"CASE pc.Relationship_Type "
			"WHEN 'MOTHER' THEN 1 "
			"WHEN 'FATHER' THEN 2 "
			"WHEN 'LEGAL_GUARDIAN' THEN 3 "
			"ELSE 4 END, "
			"p.Name"
		);

		parentStatement->setString(1, childId);

		parentResult =
			parentStatement->executeQuery();

		cout << "\n";
		cout << "============================================================\n";
		cout << "              ASSIGNED PARENTS / GUARDIANS                \n";
		cout << "============================================================\n";

		int parentCount = 0;

		while (parentResult->next())
		{
			parentCount++;

			cout << "\n";
			cout << " Parent/Guardian Number : "
				<< parentCount << "\n";

			cout << " Parent ID              : "
				<< parentResult->getString(
					"Parent_ID"
				)
				<< "\n";

			cout << " Name                   : "
				<< parentResult->getString(
					"Name"
				)
				<< "\n";

			cout << " Phone No               : "
				<< parentResult->getString(
					"Phone_No"
				)
				<< "\n";

			cout << " Relationship           : "
				<< (
					string(parentResult->getString(
						"Relationship_Type"
					).c_str()) == "LEGAL_GUARDIAN"
					? "LEGAL GUARDIAN"
					: string(parentResult->getString(
						"Relationship_Type"
					).c_str())
				)
				<< "\n";

			cout << "------------------------------------------------------------\n";
		}

		if (parentCount == 0)
		{
			cout << " No parent or legal guardian has been assigned.\n";
			cout << "------------------------------------------------------------\n";
		}

		cout << " Total Parents/Guardians : "
			<< parentCount << "\n";
		cout << "============================================================\n";

		// ==================================================
		// RETRIEVE AUTHORIZED REPRESENTATIVES
		// ==================================================

		representativeStatement = con->prepareStatement(
			"SELECT "
			"ar.Rep_ID, "
			"ar.IC_No, "
			"ar.Name, "
			"ar.Phone_No, "
			"ac.Relationship "
			"FROM AUTHOREP_CHILD ac "
			"JOIN AUTHORIZED_REPRESENTATIVE ar "
			"ON ac.Rep_ID = ar.Rep_ID "
			"WHERE ac.Child_ID = ? "
			"ORDER BY ar.Name"
		);

		representativeStatement->setString(
			1,
			childId
		);

		representativeResult =
			representativeStatement->executeQuery();

		cout << "\n";
		cout << "============================================================\n";
		cout << "             AUTHORIZED REPRESENTATIVES                    \n";
		cout << "============================================================\n";

		int representativeCount = 0;

		while (representativeResult->next())
		{
			representativeCount++;

			cout << "\n";
			cout << " Representative Number : "
				<< representativeCount << "\n";

			cout << " Representative ID     : "
				<< representativeResult->getString(
					"Rep_ID"
				)
				<< "\n";

			cout << " IC Number             : "
				<< representativeResult->getString(
					"IC_No"
				)
				<< "\n";

			cout << " Name                  : "
				<< representativeResult->getString(
					"Name"
				)
				<< "\n";

			cout << " Phone No              : "
				<< representativeResult->getString(
					"Phone_No"
				)
				<< "\n";

			cout << " Relationship          : "
				<< representativeResult->getString(
					"Relationship"
				)
				<< "\n";

			cout << "------------------------------------------------------------\n";
		}

		if (representativeCount == 0)
		{
			cout << " No authorized representative has been assigned.\n";
			cout << "------------------------------------------------------------\n";
		}

		cout << " Total Representatives : "
			<< representativeCount << "\n";
		cout << "============================================================\n";

		// ==================================================
		// DISPLAY RELATIONSHIP SUMMARY
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "                  ASSIGNMENT SUMMARY                       \n";
		cout << "============================================================\n";
		cout << " Child ID                 : "
			<< childId << "\n";
		cout << " Child Name               : "
			<< childName << "\n";
		cout << " Total Parents/Guardians  : "
			<< parentCount << "\n";
		cout << " Total Representatives    : "
			<< representativeCount << "\n";
		cout << " Total Assigned Collectors: "
			<< parentCount + representativeCount
			<< "\n";
		cout << "============================================================\n";

		// ==================================================
		// CLEANUP
		// ==================================================

		delete representativeResult;
		representativeResult = nullptr;

		delete representativeStatement;
		representativeStatement = nullptr;

		delete parentResult;
		parentResult = nullptr;

		delete parentStatement;
		parentStatement = nullptr;

		delete childResult;
		childResult = nullptr;

		delete childStatement;
		childStatement = nullptr;
	}
	catch (sql::SQLException& e)
	{
		delete representativeResult;
		representativeResult = nullptr;

		delete representativeStatement;
		representativeStatement = nullptr;

		delete parentResult;
		parentResult = nullptr;

		delete parentStatement;
		parentStatement = nullptr;

		delete childResult;
		childResult = nullptr;

		delete childStatement;
		childStatement = nullptr;

		cerr << "\nSQL Error while viewing assigned "
			<< "guardians and representatives:\n";

		cerr << "Message    : "
			<< e.what() << "\n";

		cerr << "Error Code : "
			<< e.getErrorCode() << "\n";

		cerr << "SQL State  : "
			<< e.getSQLState() << "\n";
	}
}

// ==========================================================
// ADD CHILD
// ==========================================================

void addChild(sql::Connection* con)
{
	string childId;
	string name;
	string myKidNo;
	string dateOfBirth;
	string databaseDateOfBirth;
	string gender;
	string allergiesMedical;

	bool registrationSuccessful = false;

	sql::PreparedStatement* duplicateStatement = nullptr;
	sql::PreparedStatement* insertStatement = nullptr;
	sql::ResultSet* duplicateResult = nullptr;

	cout << "\n";
	cout << "============================================================\n";
	cout << "                         ADD CHILD                          \n";
	cout << "============================================================\n";

	clearInputBuffer();

	// ======================================================
	// ENTER AND VALIDATE CHILD NAME
	// ======================================================

	while (true)
	{
		cout << " Enter Child Name                    : ";
		getline(cin, name);

		if (name.empty())
		{
			cout << "\nERROR: Child name must not be empty.\n";
			cout << "Please enter the child name again.\n\n";
			continue;
		}

		if (name.length() > 200)
		{
			cout << "\nERROR: Child name must not exceed "
				<< "200 characters.\n";
			cout << "Please enter the child name again.\n\n";
			continue;
		}

		break;
	}

	// ======================================================
	// ENTER, VALIDATE AND CHECK MYKID NUMBER
	// ======================================================

	while (true)
	{
		cout << " Enter MyKid Number                  : ";
		getline(cin, myKidNo);

		if (!isValidMyKidNumber(myKidNo))
		{
			cout << "\nERROR: MyKid number must contain "
				<< "exactly 12 numeric digits.\n";

			cout << "\nCurrent valid information:\n";
			cout << " Child Name : " << name << "\n";

			cout << "\nPlease enter the MyKid number again.\n\n";
			continue;
		}

		try
		{
			duplicateStatement = con->prepareStatement(
				"SELECT Child_ID, Name "
				"FROM CHILD "
				"WHERE MyKid_No = ?"
			);

			duplicateStatement->setString(1, myKidNo);

			duplicateResult =
				duplicateStatement->executeQuery();

			if (duplicateResult->next())
			{
				cout << "\n";
				cout << "============================================================\n";
				cout << "                  DUPLICATE MYKID NUMBER                   \n";
				cout << "============================================================\n";
				cout << " MyKid Number      : " << myKidNo << "\n";
				cout << " Existing Child ID : "
					<< duplicateResult->getString(
						"Child_ID"
					)
					<< "\n";
				cout << " Existing Name     : "
					<< duplicateResult->getString(
						"Name"
					)
					<< "\n";
				cout << "============================================================\n";
				cout << "Please enter a different MyKid number.\n\n";

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

			cerr << "\nSQL Error while checking MyKid number:\n";
			cerr << "Message    : " << e.what() << "\n";
			cerr << "Error Code : " << e.getErrorCode() << "\n";
			cerr << "SQL State  : " << e.getSQLState() << "\n";

			return;
		}
	}

	// ======================================================
	// ENTER AND VALIDATE DATE OF BIRTH
	// ======================================================

	while (true)
	{
		cout << " Enter Date of Birth [DD/MM/YYYY]   : ";
		getline(cin, dateOfBirth);

		if (!isValidDateOfBirth(dateOfBirth))
		{
			cout << "\nERROR: Invalid date of birth.\n";
			cout << "Expected format: DD/MM/YYYY\n";

			cout << "\nCurrent valid information:\n";
			cout << " Child Name   : " << name << "\n";
			cout << " MyKid Number : " << myKidNo << "\n";

			cout << "\nPlease enter the date of birth again.\n\n";
			continue;
		}

		databaseDateOfBirth =
			convertDateToDatabaseFormat(
				dateOfBirth
			);

		break;
	}

	// ======================================================
	// ENTER AND VALIDATE GENDER
	// ======================================================

	while (true)
	{
		int genderChoice = 0;

		cout << "\n Gender:\n";
		cout << " 1. MALE\n";
		cout << " 2. FEMALE\n";
		cout << "\n Select Gender [1-2]               : ";

		if (!(cin >> genderChoice))
		{
			cin.clear();
			clearInputBuffer();

			cout << "\nERROR: Please enter either 1 or 2.\n";
			cout << "Please select the gender again.\n";
			continue;
		}

		clearInputBuffer();

		if (genderChoice == 1)
		{
			gender = "MALE";
			break;
		}

		if (genderChoice == 2)
		{
			gender = "FEMALE";
			break;
		}

		cout << "\nERROR: Please select either 1 or 2.\n";
		cout << "Please select the gender again.\n";
	}

	// ======================================================
	// ENTER AND VALIDATE MEDICAL INFORMATION
	// ======================================================

	while (true)
	{
		cout << " Enter Allergies/Medical            : ";
		getline(cin, allergiesMedical);

		if (allergiesMedical.length() > 200)
		{
			cout << "\nERROR: Allergies/Medical information "
				<< "must not exceed 200 characters.\n";
			cout << "Please enter the information again.\n\n";
			continue;
		}

		break;
	}

	// ======================================================
	// DISPLAY DETAILS FOR CONFIRMATION
	// ======================================================

	cout << "\n";
	cout << "============================================================\n";
	cout << "                  CHILD DETAILS CONFIRMATION               \n";
	cout << "============================================================\n";
	cout << " Name              : " << name << "\n";
	cout << " MyKid Number      : " << myKidNo << "\n";
	cout << " Date of Birth     : " << dateOfBirth << "\n";
	cout << " Gender            : " << gender << "\n";
	cout << " Allergies/Medical : "
		<< (
			allergiesMedical.empty()
			? "Not Provided"
			: allergiesMedical
			)
		<< "\n";
	cout << "============================================================\n";

	// ======================================================
	// CONFIRM REGISTRATION
	// ======================================================

	int confirmation = 0;

	while (true)
	{
		cout << "\nConfirm child registration:\n";
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
		cout << "\nChild registration cancelled.\n";
		return;
	}

	// ======================================================
	// INSERT CHILD RECORD
	// ======================================================

	try
	{
		childId = generateChildId(con);

		insertStatement = con->prepareStatement(
			"INSERT INTO CHILD "
			"(Child_ID, MyKid_No, Name, DOB, Gender, "
			"Allergies_Medical) "
			"VALUES (?, ?, ?, ?, ?, ?)"
		);

		insertStatement->setString(1, childId);
		insertStatement->setString(2, myKidNo);
		insertStatement->setString(3, name);

		/*
		 * User enters DD/MM/YYYY, while MySQL stores the
		 * date using the standard YYYY-MM-DD format.
		 */
		insertStatement->setString(
			4,
			databaseDateOfBirth
		);

		insertStatement->setString(5, gender);

		if (allergiesMedical.empty())
		{
			insertStatement->setNull(6, 0);
		}
		else
		{
			insertStatement->setString(
				6,
				allergiesMedical
			);
		}

		const int affectedRows =
			insertStatement->executeUpdate();

		if (affectedRows > 0)
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "               CHILD REGISTRATION SUCCESSFUL               \n";
			cout << "============================================================\n";
			cout << " Child ID          : " << childId << "\n";
			cout << " Name              : " << name << "\n";
			cout << " MyKid Number      : " << myKidNo << "\n";
			cout << " Date of Birth     : " << dateOfBirth << "\n";
			cout << " Gender            : " << gender << "\n";
			cout << " Allergies/Medical : "
				<< (
					allergiesMedical.empty()
					? "Not Provided"
					: allergiesMedical
					)
				<< "\n";
			cout << "============================================================\n";

			registrationSuccessful = true;
		}
		else
		{
			cout << "\nChild record was not created.\n";
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

		cerr << "\nSQL Error while adding child:\n";
		cerr << "Message    : " << e.what() << "\n";
		cerr << "Error Code : " << e.getErrorCode() << "\n";
		cerr << "SQL State  : " << e.getSQLState() << "\n";

		return;
	}

	// ==========================================================
// OPTIONAL POST-REGISTRATION ASSIGNMENT
// ==========================================================

	if (!registrationSuccessful)
	{
		return;
	}

	int nextAction = 0;

	do
	{
		cout << "\n";
		cout << "============================================================\n";
		cout << "           CHILD POST-REGISTRATION OPTIONS                 \n";
		cout << "============================================================\n";
		cout << " Newly Registered Child ID : "
			<< childId << "\n";
		cout << " Child Name                : "
			<< name << "\n";
		cout << "------------------------------------------------------------\n";
		cout << " 1. Assign Parent/Guardian to Child\n";
		cout << " 2. Assign Authorized Representative to Child\n";
		cout << " 3. View Assigned Guardians and Representatives\n";
		cout << " 4. Finish Registration Process\n";
		cout << "------------------------------------------------------------\n";
		cout << " Select an option [1-4]: ";

		if (!(cin >> nextAction))
		{
			cin.clear();
			clearInputBuffer();

			cout << "\nERROR: Please enter a number "
				<< "from 1 to 4.\n";

			continue;
		}

		switch (nextAction)
		{
		case 1:
			cout << "\nUse the newly registered Child ID: "
				<< childId << "\n";

			linkParentToChild(con);
			break;

		case 2:
			cout << "\nUse the newly registered Child ID: "
				<< childId << "\n";

			linkRepresentativeToChild(con);
			break;

		case 3:
			cout << "\nUse the newly registered Child ID: "
				<< childId << "\n";

			viewAssignedGuardiansAndRepresentatives(con);
			break;

		case 4:
			cout << "\nChild registration process completed.\n";
			break;

		default:
			cout << "\nERROR: Please select an option "
				<< "from 1 to 4.\n";
			break;
		}

	} while (nextAction != 4);

}

// ==========================================================
// SEARCH / VIEW CHILD
// ==========================================================

void searchChild(sql::Connection* con)
{
	string childId;

	sql::PreparedStatement* searchStatement = nullptr;
	sql::ResultSet* result = nullptr;

	cout << "\n";
	cout << "============================================================\n";
	cout << "                    SEARCH / VIEW CHILD                    \n";
	cout << "============================================================\n";

	if (!displayAvailableChildRecords(con))
	{
		cout << "\nNOTICE: No child record is available for selection.\n";
		return;
	}

	cout << " Enter Child ID : ";
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

	// Validate Child ID format
	if (!isValidChildIdFormat(childId))
	{
		cout << "\nERROR: Invalid Child ID format.\n";
		cout << "Expected format: C0001\n";
		return;
	}

	try
	{
		/*
		 * DATE_FORMAT displays the database DATE value
		 * in DD/MM/YYYY format.
		 */
		searchStatement = con->prepareStatement(
			"SELECT "
			"Child_ID, "
			"MyKid_No, "
			"Name, "
			"DATE_FORMAT(DOB, '%d/%m/%Y') AS Formatted_DOB, "
			"Gender, "
			"Allergies_Medical "
			"FROM CHILD "
			"WHERE Child_ID = ?"
		);

		searchStatement->setString(1, childId);

		result = searchStatement->executeQuery();

		if (result->next())
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                       CHILD RECORD                        \n";
			cout << "============================================================\n";

			cout << " Child ID          : "
				<< result->getString("Child_ID")
				<< "\n";

			cout << " Name              : "
				<< result->getString("Name")
				<< "\n";

			cout << " MyKid Number      : "
				<< result->getString("MyKid_No")
				<< "\n";

			cout << " Date of Birth     : "
				<< result->getString("Formatted_DOB")
				<< "\n";

			cout << " Gender            : "
				<< result->getString("Gender")
				<< "\n";

			cout << " Allergies/Medical : ";

			if (result->isNull("Allergies_Medical"))
			{
				cout << "Not Provided\n";
			}
			else
			{
				cout << result->getString(
					"Allergies_Medical"
				) << "\n";
			}

			cout << "============================================================\n";
		}
		else
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                 CHILD RECORD NOT FOUND                    \n";
			cout << "============================================================\n";
			cout << " No child record was found for Child ID: "
				<< childId << "\n";
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

		cerr << "\nSQL Error while searching for child:\n";
		cerr << "Message    : " << e.what() << "\n";
		cerr << "Error Code : " << e.getErrorCode() << "\n";
		cerr << "SQL State  : " << e.getSQLState() << "\n";
	}
}

// ==========================================================
// UPDATE CHILD
// ==========================================================

void updateChild(sql::Connection* con)
{
	string childId;

	string currentName;
	string currentMyKidNo;
	string currentDatabaseDob;
	string currentDisplayDob;
	string currentGender;
	string currentAllergiesMedical;

	string newName;
	string newMyKidNo;
	string newDisplayDob;
	string newDatabaseDob;
	string newGender;
	string newAllergiesMedical;

	sql::PreparedStatement* searchStatement = nullptr;
	sql::PreparedStatement* duplicateStatement = nullptr;
	sql::PreparedStatement* updateStatement = nullptr;

	sql::ResultSet* result = nullptr;
	sql::ResultSet* duplicateResult = nullptr;

	cout << "\n";
	cout << "============================================================\n";
	cout << "                        UPDATE CHILD                        \n";
	cout << "============================================================\n";

	if (!displayAvailableChildRecords(con))
	{
		cout << "\nNOTICE: No child record is available for selection.\n";
		return;
	}

	cout << " Enter Child ID : ";
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

	if (!isValidChildIdFormat(childId))
	{
		cout << "\nERROR: Invalid Child ID format.\n";
		cout << "Expected format: C0001\n";
		return;
	}

	try
	{
		// ==================================================
		// RETRIEVE CURRENT CHILD RECORD
		// ==================================================

		searchStatement = con->prepareStatement(
			"SELECT "
			"Child_ID, MyKid_No, Name, "
			"DATE_FORMAT(DOB, '%Y-%m-%d') AS Database_DOB, "
			"DATE_FORMAT(DOB, '%d/%m/%Y') AS Display_DOB, "
			"Gender, Allergies_Medical "
			"FROM CHILD "
			"WHERE Child_ID = ?"
		);

		searchStatement->setString(1, childId);

		result = searchStatement->executeQuery();

		if (!result->next())
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                 CHILD RECORD NOT FOUND                    \n";
			cout << "============================================================\n";
			cout << " No child record was found for Child ID: "
				<< childId << "\n";
			cout << "============================================================\n";

			delete result;
			delete searchStatement;

			return;
		}

		currentName =
			string(result->getString("Name").c_str());

		currentMyKidNo =
			string(result->getString("MyKid_No").c_str());

		currentDatabaseDob =
			string(result->getString("Database_DOB").c_str());

		currentDisplayDob =
			string(result->getString("Display_DOB").c_str());

		currentGender =
			string(result->getString("Gender").c_str());

		if (result->isNull("Allergies_Medical"))
		{
			currentAllergiesMedical = "";
		}
		else
		{
			currentAllergiesMedical =
				string(
					result->getString(
						"Allergies_Medical"
					).c_str()
				);
		}

		// ==================================================
		// DISPLAY CURRENT DETAILS
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "                   CURRENT CHILD DETAILS                   \n";
		cout << "============================================================\n";
		cout << " Child ID          : " << childId << "\n";
		cout << " Name              : " << currentName << "\n";
		cout << " MyKid Number      : " << currentMyKidNo << "\n";
		cout << " Date of Birth     : " << currentDisplayDob << "\n";
		cout << " Gender            : " << currentGender << "\n";
		cout << " Allergies/Medical : "
			<< (currentAllergiesMedical.empty()
				? "Not Provided"
				: currentAllergiesMedical)
			<< "\n";
		cout << "============================================================\n";

		cout << "\nPress ENTER without entering a value "
			<< "to keep the current information.\n";

		cout << "Enter NONE for Allergies/Medical "
			<< "to remove the current information.\n\n";

		// ==================================================
		// ENTER UPDATED DETAILS
		// ==================================================

		cout << " Enter New Name                         : ";
		getline(cin, newName);

		cout << " Enter New MyKid Number                 : ";
		getline(cin, newMyKidNo);

		cout << " Enter New Date of Birth [DD/MM/YYYY]   : ";
		getline(cin, newDisplayDob);

		cout << "\n Gender:\n";
		cout << " 0. Keep Current Gender\n";
		cout << " 1. MALE\n";
		cout << " 2. FEMALE\n";

		int genderChoice = 0;

		cout << "\n Select Gender [0-2]                    : ";

		if (!(cin >> genderChoice))
		{
			cin.clear();
			clearInputBuffer();

			cout << "\nERROR: Please enter a number from 0 to 2.\n";

			delete result;
			delete searchStatement;

			return;
		}

		clearInputBuffer();

		switch (genderChoice)
		{
		case 0:
			newGender = currentGender;
			break;

		case 1:
			newGender = "MALE";
			break;

		case 2:
			newGender = "FEMALE";
			break;

		default:
			cout << "\nERROR: Please select an option "
				<< "from 0 to 2.\n";

			delete result;
			delete searchStatement;

			return;
		}

		cout << " Enter New Allergies/Medical            : ";
		getline(cin, newAllergiesMedical);

		// ==================================================
		// KEEP CURRENT VALUES
		// ==================================================

		if (newName.empty())
		{
			newName = currentName;
		}

		if (newMyKidNo.empty())
		{
			newMyKidNo = currentMyKidNo;
		}

		if (newDisplayDob.empty())
		{
			newDisplayDob = currentDisplayDob;
			newDatabaseDob = currentDatabaseDob;
		}
		else
		{
			if (!isValidDateOfBirth(newDisplayDob))
			{
				cout << "\nERROR: Invalid date of birth.\n";
				cout << "Expected format: DD/MM/YYYY\n";

				delete result;
				delete searchStatement;

				return;
			}

			newDatabaseDob =
				convertDateToDatabaseFormat(
					newDisplayDob
				);
		}

		if (newAllergiesMedical.empty())
		{
			newAllergiesMedical =
				currentAllergiesMedical;
		}
		else
		{
			string medicalCommand =
				newAllergiesMedical;

			for (char& character : medicalCommand)
			{
				character = static_cast<char>(
					std::toupper(
						static_cast<unsigned char>(
							character
							)
					)
					);
			}

			if (medicalCommand == "NONE")
			{
				newAllergiesMedical = "";
			}
		}

		// ==================================================
		// VALIDATE UPDATED DETAILS
		// ==================================================

		if (newName.empty() ||
			newName.length() > 200)
		{
			cout << "\nERROR: Child name must contain "
				<< "between 1 and 200 characters.\n";

			delete result;
			delete searchStatement;

			return;
		}

		if (!isValidMyKidNumber(newMyKidNo))
		{
			cout << "\nERROR: MyKid number must contain "
				<< "exactly 12 numeric digits.\n";

			delete result;
			delete searchStatement;

			return;
		}

		if (!isValidGender(newGender))
		{
			cout << "\nERROR: Gender must be MALE or FEMALE.\n";

			delete result;
			delete searchStatement;

			return;
		}

		if (newAllergiesMedical.length() > 200)
		{
			cout << "\nERROR: Allergies/Medical information "
				<< "must not exceed 200 characters.\n";

			delete result;
			delete searchStatement;

			return;
		}

		// ==================================================
		// CHECK DUPLICATE MYKID NUMBER
		// ==================================================

		duplicateStatement = con->prepareStatement(
			"SELECT Child_ID "
			"FROM CHILD "
			"WHERE MyKid_No = ? "
			"AND Child_ID <> ?"
		);

		duplicateStatement->setString(
			1,
			newMyKidNo
		);

		duplicateStatement->setString(
			2,
			childId
		);

		duplicateResult =
			duplicateStatement->executeQuery();

		if (duplicateResult->next())
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                  DUPLICATE MYKID NUMBER                   \n";
			cout << "============================================================\n";
			cout << " The MyKid number "
				<< newMyKidNo
				<< " is already registered to Child ID: "
				<< duplicateResult->getString("Child_ID")
				<< "\n";
			cout << "============================================================\n";

			delete duplicateResult;
			delete duplicateStatement;
			delete result;
			delete searchStatement;

			return;
		}

		// ==================================================
		// UPDATE CHILD RECORD
		// ==================================================

		updateStatement = con->prepareStatement(
			"UPDATE CHILD "
			"SET Name = ?, MyKid_No = ?, DOB = ?, "
			"Gender = ?, Allergies_Medical = ? "
			"WHERE Child_ID = ?"
		);

		updateStatement->setString(1, newName);
		updateStatement->setString(2, newMyKidNo);
		updateStatement->setString(3, newDatabaseDob);
		updateStatement->setString(4, newGender);

		if (newAllergiesMedical.empty())
		{
			updateStatement->setNull(5, 0);
		}
		else
		{
			updateStatement->setString(
				5,
				newAllergiesMedical
			);
		}

		updateStatement->setString(6, childId);

		const int affectedRows =
			updateStatement->executeUpdate();

		if (affectedRows > 0)
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                  CHILD UPDATE SUCCESSFUL                  \n";
			cout << "============================================================\n";
			cout << " Child ID          : " << childId << "\n";
			cout << " Name              : " << newName << "\n";
			cout << " MyKid Number      : " << newMyKidNo << "\n";
			cout << " Date of Birth     : " << newDisplayDob << "\n";
			cout << " Gender            : " << newGender << "\n";
			cout << " Allergies/Medical : "
				<< (newAllergiesMedical.empty()
					? "Not Provided"
					: newAllergiesMedical)
				<< "\n";
			cout << "============================================================\n";
		}
		else
		{
			cout << "\nNo changes were made to the child record.\n";
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

		cerr << "\nSQL Error while updating child:\n";
		cerr << "Message    : " << e.what() << "\n";
		cerr << "Error Code : " << e.getErrorCode() << "\n";
		cerr << "SQL State  : " << e.getSQLState() << "\n";
	}
}

// ==========================================================
// GENERATE CHILD ID
// ==========================================================

string generateChildId(sql::Connection* con)
{
	sql::Statement* statement = nullptr;
	sql::ResultSet* result = nullptr;

	int nextNumber = 1;

	try
	{
		statement = con->createStatement();

		result = statement->executeQuery(
			"SELECT MAX("
			"CAST(SUBSTRING(Child_ID, 2) AS UNSIGNED)"
			") AS Maximum_ID "
			"FROM CHILD"
		);

		if (result->next() &&
			!result->isNull("Maximum_ID"))
		{
			nextNumber =
				result->getInt("Maximum_ID") + 1;
		}

		delete result;
		result = nullptr;

		delete statement;
		statement = nullptr;
	}
	catch (...)
	{
		delete result;
		result = nullptr;

		delete statement;
		statement = nullptr;

		throw;
	}

	stringstream idStream;

	idStream << 'C'
		<< setw(4)
		<< setfill('0')
		<< nextNumber;

	return idStream.str();
}


// ==========================================================
// CHILD ID FORMAT VALIDATION
// ==========================================================

bool isValidChildIdFormat(const string& childId)
{
	if (childId.length() != 5 || childId[0] != 'C')
	{
		return false;
	}

	for (size_t index = 1; index < childId.length();
		index++)
	{
		if (!std::isdigit(static_cast<unsigned char>(childId[index])))
		{
			return false;
		}
	}

	return true;
}


// ==========================================================
// MYKID NUMBER VALIDATION
// ==========================================================

bool isValidMyKidNumber(const string& myKidNo)
{
	if (myKidNo.length() != 12)
	{
		return false;
	}

	for (char character : myKidNo)
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
// DATE OF BIRTH VALIDATION
// ==========================================================

bool isValidDateOfBirth(const string& dateOfBirth)
{
	// Expected format: DD/MM/YYYY
	if (dateOfBirth.length() != 10)
	{
		return false;
	}

	if (dateOfBirth[2] != '/' ||
		dateOfBirth[5] != '/')
	{
		return false;
	}

	for (size_t index = 0;
		index < dateOfBirth.length();
		index++)
	{
		if (index == 2 || index == 5)
		{
			continue;
		}

		if (!std::isdigit(
			static_cast<unsigned char>(
				dateOfBirth[index]
				)
		))
		{
			return false;
		}
	}

	const int day =
		stoi(dateOfBirth.substr(0, 2));

	const int month =
		stoi(dateOfBirth.substr(3, 2));

	const int year =
		stoi(dateOfBirth.substr(6, 4));

	if (year < 1900)
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

	const bool isLeapYear =
		(year % 400 == 0) ||
		(year % 4 == 0 &&
			year % 100 != 0);

	if (isLeapYear)
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

string convertDateToDatabaseFormat(
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


// ==========================================================
// CONVERT YYYY-MM-DD TO DD/MM/YYYY
// ==========================================================

string convertDateToDisplayFormat(
	const string& databaseDate
)
{
	if (databaseDate.length() != 10)
	{
		return databaseDate;
	}

	const string year =
		databaseDate.substr(0, 4);

	const string month =
		databaseDate.substr(5, 2);

	const string day =
		databaseDate.substr(8, 2);

	return day + "/" + month + "/" + year;
}

// ==========================================================
// GENDER VALIDATION
// ==========================================================

bool isValidGender(const string& gender)
{
	return gender == "MALE" ||
		gender == "FEMALE";
}
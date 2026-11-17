#include "Attendance.h"
#include "InputUtils.h"
#include "PdfExporter.h"

#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

using namespace std;

namespace fs = std::filesystem;

// ==========================================================
// FORMAT DATABASE VALUES FOR USER-FRIENDLY DISPLAY
// ==========================================================

static string formatAttendanceDisplayText(const string& value)
{
	if (value == "REPRESENTATIVE")
	{
		return "AUTHORIZED REPRESENTATIVE";
	}

	string displayText = value;

	for (char& character : displayText)
	{
		if (character == '_')
		{
			character = ' ';
		}
	}

	return displayText;
}

struct DailyAttendanceRecord
{
	string logId;
	string childId;
	string childName;

	string checkInTime;
	string checkInStatus;

	string checkOutTime;
	string pickupStatus;

	string collectorType;
	string collectorId;
	string collectorName;
	string relationship;
	string registeredParents;

	double checkInPoints = 0.0;
	double checkOutPoints = 0.0;
	double dailyPoints = 0.0;
	double latePickupCharge = 0.0;

	bool checkoutCompleted = false;
};

bool exportDailyAttendancePdf(
	const vector<DailyAttendanceRecord>& records,
	const string& attendanceDate,
	const UserSession& session
);

// ==========================================================
// DAILY CHECK-IN
// ==========================================================

void dailyCheckIn(sql::Connection* con, const UserSession& session)
{
	string staffId;
	string childId;
	string logId;

	string staffName;
	string childName;
	string checkInStatus;
	string currentDate;
	string currentTime;
	string timeCategory;

	double checkInPoints = 0.0;

	staffId = session.staffId;
	staffName = session.staffName;

	sql::PreparedStatement* childStatement = nullptr;
	sql::PreparedStatement* duplicateStatement = nullptr;
	sql::PreparedStatement* attendanceStatement = nullptr;
	sql::PreparedStatement* logRecordStatement = nullptr;

	sql::Statement* timeStatement = nullptr;
	sql::ResultSet* childResult = nullptr;
	sql::ResultSet* duplicateResult = nullptr;
	sql::ResultSet* timeResult = nullptr;

	cout << "\n";
	cout << "============================================================\n";
	cout << "                      DAILY CHECK-IN                       \n";
	cout << "============================================================\n";

	cout << " Logged-In Teacher : "
		<< staffName
		<< " (" << staffId << ")\n";

	cout << "------------------------------------------------------------\n";

	// ======================================================
	// ENTER AND VALIDATE CHILD ID
	// ======================================================

	cout << " Enter Child ID : ";
	getline(cin, childId);

	for (char& character : childId)
	{
		character = static_cast<char>(
			std::toupper(
				static_cast<unsigned char>(character)
			)
			);
	}

	if (!isValidAttendanceChildId(childId))
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

			delete childResult;
			delete childStatement;

			return;
		}

		childName =
			string(childResult->getString("Name").c_str());

		// ==================================================
		// CHECK DUPLICATE DAILY ATTENDANCE
		// ==================================================

		duplicateStatement = con->prepareStatement(
			"SELECT Log_ID "
			"FROM ATTENDANCE_LOG "
			"WHERE Child_ID = ? "
			"AND Log_Date = CURDATE()"
		);

		duplicateStatement->setString(1, childId);

		duplicateResult =
			duplicateStatement->executeQuery();

		if (duplicateResult->next())
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                  CHILD ALREADY CHECKED IN                 \n";
			cout << "============================================================\n";
			cout << " Child ID : " << childId << "\n";
			cout << " Name     : " << childName << "\n";
			cout << " Log ID   : "
				<< duplicateResult->getString("Log_ID")
				<< "\n";
			cout << "============================================================\n";

			delete childResult;
			delete childStatement;

			delete duplicateResult;
			delete duplicateStatement;

			return;
		}

		// ==================================================
		// GET CURRENT DATABASE DATE AND TIME
		// ==================================================

		timeStatement = con->createStatement();

		timeResult = timeStatement->executeQuery(
			"SELECT "
			"DATE_FORMAT(CURDATE(), '%d/%m/%Y') AS Formatted_Date, "
			"TIME_FORMAT(CURTIME(), '%H:%i:%s') AS Formatted_Time, "
			"CASE "
			"WHEN CURTIME() < '07:30:00' THEN 'EARLY' "
			"WHEN CURTIME() <= '08:00:00' THEN 'ON_TIME' "
			"ELSE 'LATE' "
			"END AS Time_Category"
		);

		if (!timeResult->next())
		{
			cout << "\nERROR: Unable to retrieve "
				<< "the current system time.\n";

			delete childResult;
			delete childStatement;

			delete duplicateResult;
			delete duplicateStatement;

			delete timeResult;
			delete timeStatement;

			return;
		}

		currentDate =
			string(
				timeResult->getString(
					"Formatted_Date"
				).c_str()
			);

		currentTime =
			string(
				timeResult->getString(
					"Formatted_Time"
				).c_str()
			);

		timeCategory =
			string(
				timeResult->getString(
					"Time_Category"
				).c_str()
			);

		// ==================================================
		// HANDLE EARLY CHECK-IN
		// ==================================================

		if (timeCategory == "EARLY")
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                  CHECK-IN NOT AVAILABLE                   \n";
			cout << "============================================================\n";
			cout << " Current Date : " << currentDate << "\n";
			cout << " Current Time : " << currentTime << "\n";
			cout << " Check-In begins at 07:30 AM.\n";
			cout << "============================================================\n";

			delete childResult;
			delete childStatement;

			delete duplicateResult;
			delete duplicateStatement;

			delete timeResult;
			delete timeStatement;

			return;
		}

		if (timeCategory == "ON_TIME")
		{
			checkInStatus = "ON_TIME";
			checkInPoints = 1.00;
		}
		else
		{
			checkInStatus = "LATE";
			checkInPoints = 0.50;
		}

		// ==================================================
		// GENERATE LOG ID
		// ==================================================

		logId = generateLogId(con);

		// ==================================================
		// BEGIN DATABASE TRANSACTION
		// ==================================================

		con->setAutoCommit(false);

		// Create ATTENDANCE_LOG record
		attendanceStatement = con->prepareStatement(
			"INSERT INTO ATTENDANCE_LOG "
			"(Log_ID, Log_Date, Child_ID, "
			"CheckIn_Points, CheckOut_Points, "
			"Daily_Points, Late_Pickup_Charge) "
			"VALUES (?, CURDATE(), ?, ?, NULL, NULL, 0.00)"
		);

		attendanceStatement->setString(1, logId);
		attendanceStatement->setString(2, childId);
		attendanceStatement->setDouble(3, checkInPoints);

		attendanceStatement->executeUpdate();

		// Create CHECK_IN transaction
		logRecordStatement = con->prepareStatement(
			"INSERT INTO LOG_RECORD "
			"(Log_ID, Staff_ID, Child_ID, Record_Type, "
			"Record_Time, Status, Parent_ID, Rep_ID) "
			"VALUES (?, ?, ?, 'CHECK_IN', NOW(), ?, NULL, NULL)"
		);

		logRecordStatement->setString(1, logId);
		logRecordStatement->setString(2, staffId);
		logRecordStatement->setString(3, childId);
		logRecordStatement->setString(4, checkInStatus);

		logRecordStatement->executeUpdate();

		// Commit both records together
		con->commit();
		con->setAutoCommit(true);

		// ==================================================
		// DISPLAY SUCCESS MESSAGE
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "                  CHECK-IN SUCCESSFUL                      \n";
		cout << "============================================================\n";
		cout << " Log ID        : " << logId << "\n";
		cout << " Date          : " << currentDate << "\n";
		cout << " Check-In Time : " << currentTime << "\n";
		cout << " Child ID      : " << childId << "\n";
		cout << " Child Name    : " << childName << "\n";
		cout << " Staff ID      : " << staffId << "\n";
		cout << " Staff Name    : " << staffName << "\n";
		cout << " Status        : " << formatAttendanceDisplayText(checkInStatus) << "\n";
		cout << " Check-In Points: " << fixed << setprecision(2) << checkInPoints << "\n";
		cout << "============================================================\n";

		// ==================================================
		// CLEANUP
		// ==================================================

		delete childResult;
		childResult = nullptr;

		delete childStatement;
		childStatement = nullptr;

		delete duplicateResult;
		duplicateResult = nullptr;

		delete duplicateStatement;
		duplicateStatement = nullptr;

		delete timeResult;
		timeResult = nullptr;

		delete timeStatement;
		timeStatement = nullptr;

		delete attendanceStatement;
		attendanceStatement = nullptr;

		delete logRecordStatement;
		logRecordStatement = nullptr;
	}
	catch (sql::SQLException& e)
	{
		try
		{
			con->rollback();
			con->setAutoCommit(true);
		}
		catch (...)
		{
			// Prevent secondary error during rollback.
		}

		delete childResult;
		delete childStatement;

		delete duplicateResult;
		delete duplicateStatement;

		delete timeResult;
		delete timeStatement;

		delete attendanceStatement;
		delete logRecordStatement;

		cerr << "\nSQL Error while recording Daily Check-In:\n";
		cerr << "Message    : " << e.what() << "\n";
		cerr << "Error Code : " << e.getErrorCode() << "\n";
		cerr << "SQL State  : " << e.getSQLState() << "\n";
	}
}

// ==========================================================
// PICKUP VERIFICATION AND CHECK-OUT
// ==========================================================

void pickupVerificationAndCheckOut(sql::Connection* con, const UserSession& session)
{
	string staffId;
	string childId;

	string staffName;
	string childName;
	string logId;
	string checkInTime;
	string checkInStatus;

	string collectorId;
	string collectorName;
	string collectorIcNo;
	string collectorRelationship;
	string collectorType;
	string parentIdForLog;
	string representativeIdForLog;

	string currentDate;
	string currentTime;
	string pickupStatus;

	double checkInPoints = 0.0;
	double checkOutPoints = 0.0;
	double dailyPoints = 0.0;
	double latePickupCharge = 0.0;

	int lateSeconds = 0;
	int lateMinutes = 0;

	const double FIXED_LATE_PICKUP_CHARGE = 5.00;

	staffId = session.staffId;
	staffName = session.staffName;

	sql::PreparedStatement* childStatement = nullptr;
	sql::PreparedStatement* attendanceStatement = nullptr;
	sql::PreparedStatement* checkoutCheckStatement = nullptr;
	sql::PreparedStatement* collectorStatement = nullptr;
	sql::PreparedStatement* checkoutStatement = nullptr;
	sql::PreparedStatement* pointsStatement = nullptr;

	sql::ResultSet* childResult = nullptr;
	sql::ResultSet* attendanceResult = nullptr;
	sql::ResultSet* checkoutCheckResult = nullptr;
	sql::ResultSet* collectorResult = nullptr;

	sql::Statement* pickupTimeStatement = nullptr;
	sql::ResultSet* pickupTimeResult = nullptr;

	cout << "\n";
	cout << "============================================================\n";
	cout << "          PICKUP VERIFICATION AND CHECK-OUT                \n";
	cout << "============================================================\n";

	cout << " Logged-In Teacher : "
		<< staffName
		<< " (" << staffId << ")\n";

	cout << "------------------------------------------------------------\n";

	// ======================================================
	// ENTER AND VALIDATE CHILD ID
	// ======================================================

	cout << " Enter Child ID : ";
	getline(cin, childId);

	for (char& character : childId)
	{
		character = static_cast<char>(
			std::toupper(
				static_cast<unsigned char>(character)
			)
			);
	}

	if (!isValidAttendanceChildId(childId))
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

			delete childResult;
			delete childStatement;

			return;
		}

		childName =
			string(
				childResult->getString("Name").c_str()
			);

		// ==================================================
		// FIND TODAY'S ATTENDANCE RECORD
		// ==================================================

		attendanceStatement = con->prepareStatement(
			"SELECT "
			"al.Log_ID, "
			"al.CheckIn_Points, "
			"DATE_FORMAT(lr.Record_Time, '%H:%i:%s') "
			"AS Formatted_CheckIn_Time, "
			"lr.Status AS CheckIn_Status "
			"FROM ATTENDANCE_LOG al "
			"JOIN LOG_RECORD lr "
			"ON al.Log_ID = lr.Log_ID "
			"WHERE al.Child_ID = ? "
			"AND al.Log_Date = CURDATE() "
			"AND lr.Record_Type = 'CHECK_IN'"
		);

		attendanceStatement->setString(1, childId);

		attendanceResult =
			attendanceStatement->executeQuery();

		if (!attendanceResult->next())
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                  NO ACTIVE CHECK-IN                       \n";
			cout << "============================================================\n";
			cout << " Child ID   : " << childId << "\n";
			cout << " Child Name : " << childName << "\n";
			cout << " No check-in record exists for today.\n";
			cout << "============================================================\n";

			delete childResult;
			delete childStatement;

			delete attendanceResult;
			delete attendanceStatement;

			return;
		}

		logId =
			string(
				attendanceResult->getString(
					"Log_ID"
				).c_str()
			);

		checkInTime =
			string(
				attendanceResult->getString(
					"Formatted_CheckIn_Time"
				).c_str()
			);

		checkInStatus =
			string(
				attendanceResult->getString(
					"CheckIn_Status"
				).c_str()
			);

		if (attendanceResult->isNull("CheckIn_Points"))
		{
			/*
			 * Fallback for legacy attendance data.
			 * New attendance records should already contain
			 * CheckIn_Points.
			 */
			if (checkInStatus == "ON_TIME")
			{
				checkInPoints = 1.00;
			}
			else
			{
				checkInPoints = 0.50;
			}
		}
		else
		{
			checkInPoints =
				attendanceResult->getDouble(
					"CheckIn_Points"
				);
		}

		// ==================================================
		// CHECK WHETHER CHECK_OUT ALREADY EXISTS
		// ==================================================

		checkoutCheckStatement = con->prepareStatement(
			"SELECT "
			"DATE_FORMAT(Record_Time, '%H:%i:%s') "
			"AS Formatted_CheckOut_Time, "
			"Status "
			"FROM LOG_RECORD "
			"WHERE Log_ID = ? "
			"AND Record_Type = 'CHECK_OUT'"
		);

		checkoutCheckStatement->setString(1, logId);

		checkoutCheckResult =
			checkoutCheckStatement->executeQuery();

		if (checkoutCheckResult->next())
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                CHILD ALREADY CHECKED OUT                  \n";
			cout << "============================================================\n";
			cout << " Log ID         : " << logId << "\n";
			cout << " Child ID       : " << childId << "\n";
			cout << " Child Name     : " << childName << "\n";
			cout << " Check-Out Time : "
				<< checkoutCheckResult->getString(
					"Formatted_CheckOut_Time"
				)
				<< "\n";
			cout << " Pickup Status  : "
				<< formatAttendanceDisplayText(
					string(
						checkoutCheckResult->getString(
							"Status"
						).c_str()
					)
				)
				<< "\n";
			cout << "============================================================\n";

			delete childResult;
			delete childStatement;

			delete attendanceResult;
			delete attendanceStatement;

			delete checkoutCheckResult;
			delete checkoutCheckStatement;

			return;
		}

		// ==================================================
		// DISPLAY ACTIVE ATTENDANCE
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "                  ACTIVE ATTENDANCE FOUND                  \n";
		cout << "============================================================\n";
		cout << " Log ID          : " << logId << "\n";
		cout << " Child ID        : " << childId << "\n";
		cout << " Child Name      : " << childName << "\n";
		cout << " Check-In Time   : " << checkInTime << "\n";
		cout << " Check-In Status : " << formatAttendanceDisplayText(checkInStatus) << "\n";
		cout << " Check-In Points : " << fixed << setprecision(2) << checkInPoints << "\n";
		cout << " Teacher ID      : " << staffId << "\n";
		cout << " Teacher Name    : " << staffName << "\n";
		cout << "============================================================\n";

		// ==================================================
		// SELECT COLLECTOR TYPE
		// ==================================================

		int collectorChoice = 0;

		cout << "\n Collector Type:\n";
		cout << " 1. Registered Parent\n";
		cout << " 2. Authorized Representative\n";
		cout << "\n Select Collector Type [1-2] : ";

		if (!(cin >> collectorChoice))
		{
			cin.clear();
			clearInputBuffer();

			cout << "\nERROR: Please enter either 1 or 2.\n";

			delete childResult;
			delete childStatement;

			delete attendanceResult;
			delete attendanceStatement;

			delete checkoutCheckResult;
			delete checkoutCheckStatement;

			return;
		}

		clearInputBuffer();

		// ==================================================
		// VERIFY REGISTERED PARENT
		// ==================================================

		if (collectorChoice == 1)
		{
			collectorType = "REGISTERED PARENT";

			cout << " Enter Parent ID                  : ";
			getline(cin, collectorId);

			for (char& character : collectorId)
			{
				character = static_cast<char>(
					std::toupper(
						static_cast<unsigned char>(character)
					)
					);
			}

			if (!isValidPickupParentId(collectorId))
			{
				cout << "\nERROR: Invalid Parent ID format.\n";
				cout << "Expected format: P0001\n";

				delete childResult;
				delete childStatement;

				delete attendanceResult;
				delete attendanceStatement;

				delete checkoutCheckResult;
				delete checkoutCheckStatement;

				return;
			}

			collectorStatement = con->prepareStatement(
				"SELECT "
				"p.Parent_ID, "
				"p.Name, "
				"pc.Relationship_Type "
				"FROM PARENT p "
				"JOIN PARENT_CHILD pc "
				"ON p.Parent_ID = pc.Parent_ID "
				"WHERE p.Parent_ID = ? "
				"AND pc.Child_ID = ?"
			);

			collectorStatement->setString(1, collectorId);
			collectorStatement->setString(2, childId);

			collectorResult =
				collectorStatement->executeQuery();

			if (!collectorResult->next())
			{
				cout << "\n";
				cout << "============================================================\n";
				cout << "                 VERIFICATION FAILED                       \n";
				cout << "============================================================\n";
				cout << " Parent ID : " << collectorId << "\n";
				cout << " Child ID  : " << childId << "\n";
				cout << " The parent is not assigned to this child.\n";
				cout << "\nDO NOT RELEASE THE CHILD.\n";
				cout << "============================================================\n";

				delete childResult;
				delete childStatement;

				delete attendanceResult;
				delete attendanceStatement;

				delete checkoutCheckResult;
				delete checkoutCheckStatement;

				delete collectorResult;
				delete collectorStatement;

				return;
			}

			collectorName =
				string(
					collectorResult->getString(
						"Name"
					).c_str()
				);

			collectorRelationship =
				string(
					collectorResult->getString(
						"Relationship_Type"
					).c_str()
				);

			parentIdForLog = collectorId;
			representativeIdForLog = "";
		}

		// ==================================================
		// VERIFY AUTHORIZED REPRESENTATIVE
		// ==================================================

		else if (collectorChoice == 2)
		{
			collectorType = "AUTHORIZED REPRESENTATIVE";

			cout << " Enter Representative ID          : ";
			getline(cin, collectorId);

			for (char& character : collectorId)
			{
				character = static_cast<char>(
					std::toupper(
						static_cast<unsigned char>(character)
					)
					);
			}

			if (!isValidPickupRepresentativeId(collectorId))
			{
				cout << "\nERROR: Invalid Representative ID format.\n";
				cout << "Expected format: R0001\n";

				delete childResult;
				delete childStatement;

				delete attendanceResult;
				delete attendanceStatement;

				delete checkoutCheckResult;
				delete checkoutCheckStatement;

				return;
			}

			collectorStatement = con->prepareStatement(
				"SELECT "
				"ar.Rep_ID, "
				"ar.IC_No, "
				"ar.Name, "
				"ac.Relationship "
				"FROM AUTHORIZED_REPRESENTATIVE ar "
				"JOIN AUTHOREP_CHILD ac "
				"ON ar.Rep_ID = ac.Rep_ID "
				"WHERE ar.Rep_ID = ? "
				"AND ac.Child_ID = ?"
			);

			collectorStatement->setString(1, collectorId);
			collectorStatement->setString(2, childId);

			collectorResult = collectorStatement->executeQuery();

			if (!collectorResult->next())
			{
				cout << "\n";
				cout << "============================================================\n";
				cout << "                 VERIFICATION FAILED                       \n";
				cout << "============================================================\n";
				cout << " Representative ID : "
					<< collectorId << "\n";
				cout << " Child ID          : "
					<< childId << "\n";
				cout << " The representative is not authorized "
					<< "to collect this child.\n";
				cout << "\nDO NOT RELEASE THE CHILD.\n";
				cout << "============================================================\n";

				delete childResult;
				delete childStatement;

				delete attendanceResult;
				delete attendanceStatement;

				delete checkoutCheckResult;
				delete checkoutCheckStatement;

				delete collectorResult;
				delete collectorStatement;

				return;
			}

			collectorIcNo =
				string(
					collectorResult->getString(
						"IC_No"
					).c_str()
				);

			collectorName =
				string(
					collectorResult->getString(
						"Name"
					).c_str()
				);

			collectorRelationship =
				string(
					collectorResult->getString(
						"Relationship"
					).c_str()
				);

			parentIdForLog = "";
			representativeIdForLog = collectorId;
		}
		else
		{
			cout << "\nERROR: Please select either 1 or 2.\n";

			delete childResult;
			delete childStatement;

			delete attendanceResult;
			delete attendanceStatement;

			delete checkoutCheckResult;
			delete checkoutCheckStatement;

			return;
		}

		// ==================================================
		// DISPLAY SUCCESSFUL VERIFICATION
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "               COLLECTOR VERIFICATION SUCCESSFUL           \n";
		cout << "============================================================\n";
		cout << " Log ID                 : " << logId << "\n";
		cout << " Child ID               : " << childId << "\n";
		cout << " Child Name             : " << childName << "\n";
		cout << " Collector Type         : " << collectorType << "\n";
		cout << " Collector ID           : " << collectorId << "\n";

		if (collectorType == "AUTHORIZED REPRESENTATIVE")
		{
			cout << " Collector IC Number    : "
				<< collectorIcNo << "\n";
		}

		cout << " Collector Name         : "
			<< collectorName << "\n";
		cout << " Relationship           : "
			<< formatAttendanceDisplayText(collectorRelationship) << "\n";
		cout << "============================================================\n";

		// ==================================================
		// GET CURRENT CHECK-OUT DATE AND TIME
		// ==================================================

		pickupTimeStatement = con->createStatement();

		pickupTimeResult = pickupTimeStatement->executeQuery(
			"SELECT "
			"DATE_FORMAT(CURDATE(), '%d/%m/%Y') "
			"AS Formatted_Date, "

			"TIME_FORMAT(CURTIME(), '%H:%i:%s') "
			"AS Formatted_Time, "

			"CASE "
			"WHEN CURTIME() < '11:30:00' "
			"THEN 'EARLY_PICKUP' "
			"WHEN CURTIME() <= '12:30:00' "
			"THEN 'ON_TIME' "
			"ELSE 'LATE_PICKUP' "
			"END AS Pickup_Category, "

			"CASE "
			"WHEN CURTIME() <= '12:30:00' "
			"THEN 0 "
			"ELSE TIMESTAMPDIFF("
			"SECOND, "
			"TIMESTAMP(CURDATE(), '12:30:00'), "
			"NOW()"
			") "
			"END AS Late_Seconds"
		);

		if (!pickupTimeResult->next())
		{
			cout << "\nERROR: Unable to retrieve "
				<< "the current checkout time.\n";

			delete childResult;
			delete childStatement;

			delete attendanceResult;
			delete attendanceStatement;

			delete checkoutCheckResult;
			delete checkoutCheckStatement;

			delete collectorResult;
			delete collectorStatement;

			delete pickupTimeResult;
			delete pickupTimeStatement;

			return;
		}

		currentDate =
			string(
				pickupTimeResult->getString(
					"Formatted_Date"
				).c_str()
			);

		currentTime =
			string(
				pickupTimeResult->getString(
					"Formatted_Time"
				).c_str()
			);

		const string pickupCategory =
			string(
				pickupTimeResult->getString(
					"Pickup_Category"
				).c_str()
			);

		lateSeconds =
			pickupTimeResult->getInt(
				"Late_Seconds"
			);

		if (lateSeconds < 0)
		{
			lateSeconds = 0;
		}

		// ==================================================
		// HANDLE EARLY PICKUP
		// ==================================================

		if (pickupCategory == "EARLY_PICKUP")
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                CHECK-OUT NOT AVAILABLE                    \n";
			cout << "============================================================\n";
			cout << " Current Date : " << currentDate << "\n";
			cout << " Current Time : " << currentTime << "\n";
			cout << " Check-Out begins at 11:30 AM.\n";
			cout << "============================================================\n";

			delete childResult;
			delete childStatement;

			delete attendanceResult;
			delete attendanceStatement;

			delete checkoutCheckResult;
			delete checkoutCheckStatement;

			delete collectorResult;
			delete collectorStatement;

			delete pickupTimeResult;
			delete pickupTimeStatement;

			return;
		}

		// ==================================================
		// DETERMINE PICKUP STATUS AND DAILY POINTS
		// ==================================================

		if (pickupCategory == "ON_TIME")
		{
			pickupStatus = "ON_TIME";
			checkOutPoints = 1.00;
			latePickupCharge = 0.00;
			lateMinutes = 0;
		}
		else
		{
			pickupStatus = "LATE_PICKUP";
			checkOutPoints = 0.50;

			/*
			 * A fixed RM5.00 charge is applied once for each
			 * pickup transaction recorded after 12:30 PM.
			 *
			 * The charge does not increase based on the number
			 * of minutes or hours late.
			 */
			latePickupCharge =
				FIXED_LATE_PICKUP_CHARGE;

			/*
			 * Late duration is retained for information only.
			 * It does not affect the amount charged.
			 *
			 * Any remaining seconds are rounded upward to the
			 * next whole minute for display.
			 */
			lateMinutes =
				(lateSeconds + 59) / 60;
		}

		dailyPoints =
			(checkInPoints + checkOutPoints) / 2.00;

		// ==================================================
		// BEGIN CHECK-OUT TRANSACTION
		// ==================================================

		con->setAutoCommit(false);

		checkoutStatement = con->prepareStatement(
			"INSERT INTO LOG_RECORD "
			"(Log_ID, Staff_ID, Child_ID, Record_Type, "
			"Record_Time, Status, Parent_ID, Rep_ID) "
			"VALUES (?, ?, ?, 'CHECK_OUT', NOW(), ?, ?, ?)"
		);

		checkoutStatement->setString(1, logId);
		checkoutStatement->setString(2, staffId);
		checkoutStatement->setString(3, childId);
		checkoutStatement->setString(4, pickupStatus);

		// Parent_ID
		if (parentIdForLog.empty())
		{
			checkoutStatement->setNull(5, 0);
		}
		else
		{
			checkoutStatement->setString(
				5,
				parentIdForLog
			);
		}

		// Rep_ID
		if (representativeIdForLog.empty())
		{
			checkoutStatement->setNull(6, 0);
		}
		else
		{
			checkoutStatement->setString(
				6,
				representativeIdForLog
			);
		}

		checkoutStatement->executeUpdate();

		// ==================================================
		// UPDATE DAILY POINTS
		// ==================================================

		pointsStatement = con->prepareStatement(
			"UPDATE ATTENDANCE_LOG "
			"SET CheckOut_Points = ?, "
			"Daily_Points = ?, "
			"Late_Pickup_Charge = ? "
			"WHERE Log_ID = ?"
		);

		pointsStatement->setDouble(
			1,
			checkOutPoints
		);

		pointsStatement->setDouble(
			2,
			dailyPoints
		);

		pointsStatement->setDouble(
			3,
			latePickupCharge
		);

		pointsStatement->setString(
			4,
			logId
		);

		const int updatedRows =
			pointsStatement->executeUpdate();

		if (updatedRows != 1)
		{
			con->rollback();
			con->setAutoCommit(true);

			cout << "\nERROR: Unable to update "
				<< "attendance scores and late pickup charge.\n";

			delete childResult;
			delete childStatement;

			delete attendanceResult;
			delete attendanceStatement;

			delete checkoutCheckResult;
			delete checkoutCheckStatement;

			delete collectorResult;
			delete collectorStatement;

			delete pickupTimeResult;
			delete pickupTimeStatement;

			delete checkoutStatement;
			delete pointsStatement;

			return;
		}

		con->commit();
		con->setAutoCommit(true);

		// ==================================================
		// DISPLAY CHECK-OUT SUCCESS
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "                  CHECK-OUT SUCCESSFUL                     \n";
		cout << "============================================================\n";
		cout << " Log ID                 : " << logId << "\n";
		cout << " Date                   : " << currentDate << "\n";
		cout << " Check-Out Time         : " << currentTime << "\n";
		cout << " Child ID               : " << childId << "\n";
		cout << " Child Name             : " << childName << "\n";
		cout << " Collector Type         : " << collectorType << "\n";
		cout << " Collector ID           : " << collectorId << "\n";

		if (collectorType == "AUTHORIZED REPRESENTATIVE")
		{
			cout << " Collector IC Number    : "
				<< collectorIcNo << "\n";
		}

		cout << " Collector Name         : "
			<< collectorName << "\n";
		cout << " Relationship           : "
			<< formatAttendanceDisplayText(collectorRelationship) << "\n";
		cout << " Teacher ID             : " << staffId << "\n";
		cout << " Teacher Name           : " << staffName << "\n";
		cout << " Pickup Status          : "
			<< formatAttendanceDisplayText(pickupStatus) << "\n";

		cout << " Check-In Points        : "
			<< fixed << setprecision(2)
			<< checkInPoints << "\n";

		cout << " Check-Out Points       : "
			<< fixed << setprecision(2)
			<< checkOutPoints << "\n";

		cout << " Daily Points           : "
			<< fixed << setprecision(2)
			<< dailyPoints << "\n";

		cout << " Late Duration          : "
			<< lateMinutes << " minute(s)\n";

		cout << " Late Pickup Charge     : RM"
			<< fixed << setprecision(2)
			<< latePickupCharge << "\n";
		cout << "============================================================\n";

		// ==================================================
		// CLEANUP
		// ==================================================

		delete childResult;
		childResult = nullptr;

		delete childStatement;
		childStatement = nullptr;

		delete attendanceResult;
		attendanceResult = nullptr;

		delete attendanceStatement;
		attendanceStatement = nullptr;

		delete checkoutCheckResult;
		checkoutCheckResult = nullptr;

		delete checkoutCheckStatement;
		checkoutCheckStatement = nullptr;

		delete collectorResult;
		collectorResult = nullptr;

		delete collectorStatement;
		collectorStatement = nullptr;

		delete pickupTimeResult;
		pickupTimeResult = nullptr;

		delete pickupTimeStatement;
		pickupTimeStatement = nullptr;

		delete checkoutStatement;
		checkoutStatement = nullptr;

		delete pointsStatement;
		pointsStatement = nullptr;
	}
	catch (sql::SQLException& e)
	{
		try
		{
			con->rollback();
			con->setAutoCommit(true);
		}
		catch (...)
		{
			// Prevent a secondary error during rollback.
		}

		delete childResult;
		delete childStatement;

		delete attendanceResult;
		delete attendanceStatement;

		delete checkoutCheckResult;
		delete checkoutCheckStatement;

		delete collectorResult;
		delete collectorStatement;

		delete pickupTimeResult;
		delete pickupTimeStatement;

		delete checkoutStatement;
		delete pointsStatement;

		cerr << "\nSQL Error while recording Check-Out:\n";
		cerr << "Message    : " << e.what() << "\n";
		cerr << "Error Code : " << e.getErrorCode() << "\n";
		cerr << "SQL State  : " << e.getSQLState() << "\n";
	}
}

// ==========================================================
// VIEW DAILY ATTENDANCE
// ==========================================================

void viewDailyAttendance(
	sql::Connection* con,
	const UserSession& session
)
{
	string attendanceDate;
	string databaseDate;
	string childId;

	sql::PreparedStatement* attendanceStatement = nullptr;
	sql::ResultSet* attendanceResult = nullptr;

	vector<DailyAttendanceRecord> records;

	cout << "\n";
	cout << "============================================================\n";
	cout << "                 VIEW DAILY ATTENDANCE                     \n";
	cout << "============================================================\n";

	cout << " Logged-In Teacher : "
		<< session.staffName
		<< " (" << session.staffId << ")\n";

	cout << "------------------------------------------------------------\n";

	// ======================================================
	// ENTER AND VALIDATE ATTENDANCE DATE
	// ======================================================

	cout << " Enter Attendance Date [DD/MM/YYYY] : ";
	getline(cin, attendanceDate);

	if (!isValidAttendanceDate(attendanceDate))
	{
		cout << "\nERROR: Invalid attendance date.\n";
		cout << "Expected format: DD/MM/YYYY\n";
		return;
	}

	databaseDate =
		convertAttendanceDateToDatabase(
			attendanceDate
		);

	// ======================================================
	// SELECT SEARCH SCOPE
	// ======================================================

	int searchChoice = 0;

	cout << "\n Search Scope:\n";
	cout << " 1. View All Children\n";
	cout << " 2. Search Specific Child\n";
	cout << "\n Select an option [1-2]            : ";

	if (!(cin >> searchChoice))
	{
		cin.clear();
		clearInputBuffer();

		cout << "\nERROR: Please enter either 1 or 2.\n";
		return;
	}

	clearInputBuffer();

	if (searchChoice == 2)
	{
		cout << " Enter Child ID                    : ";
		getline(cin, childId);

		for (char& character : childId)
		{
			character = static_cast<char>(
				std::toupper(
					static_cast<unsigned char>(character)
				)
				);
		}

		if (!isValidAttendanceChildId(childId))
		{
			cout << "\nERROR: Invalid Child ID format.\n";
			cout << "Expected format: C0001\n";
			return;
		}
	}
	else if (searchChoice != 1)
	{
		cout << "\nERROR: Please select either 1 or 2.\n";
		return;
	}

	try
	{
		// ==================================================
		// PREPARE DAILY ATTENDANCE QUERY
		// ==================================================

		const string baseQuery =
			"SELECT "
			"al.Log_ID, "
			"al.Child_ID, "
			"c.Name AS Child_Name, "

			"COALESCE(("
			"SELECT GROUP_CONCAT("
			"CONCAT(p2.Parent_ID, ' - ', p2.Name, "
			"' (', pc2.Relationship_Type, ')') "
			"ORDER BY FIELD(pc2.Relationship_Type, "
			"'MOTHER', 'FATHER'), p2.Name "
			"SEPARATOR '; ') "
			"FROM PARENT_CHILD pc2 "
			"JOIN PARENT p2 "
			"ON pc2.Parent_ID = p2.Parent_ID "
			"WHERE pc2.Child_ID = al.Child_ID"
			"), 'No registered parent') AS Registered_Parents, "

			"MAX(CASE "
			"WHEN lr.Record_Type = 'CHECK_IN' "
			"THEN DATE_FORMAT(lr.Record_Time, '%H:%i:%s') "
			"END) AS CheckIn_Time, "

			"MAX(CASE "
			"WHEN lr.Record_Type = 'CHECK_IN' "
			"THEN lr.Status "
			"END) AS CheckIn_Status, "

			"MAX(CASE "
			"WHEN lr.Record_Type = 'CHECK_OUT' "
			"THEN DATE_FORMAT(lr.Record_Time, '%H:%i:%s') "
			"END) AS CheckOut_Time, "

			"MAX(CASE "
			"WHEN lr.Record_Type = 'CHECK_OUT' "
			"THEN lr.Status "
			"END) AS Pickup_Status, "

			"MAX(CASE "
			"WHEN lr.Record_Type = 'CHECK_OUT' "
			"THEN lr.Parent_ID "
			"END) AS Checkout_Parent_ID, "

			"MAX(CASE "
			"WHEN lr.Record_Type = 'CHECK_OUT' "
			"THEN p.Name "
			"END) AS Parent_Name, "

			"MAX(CASE "
			"WHEN lr.Record_Type = 'CHECK_OUT' "
			"THEN pc.Relationship_Type "
			"END) AS Parent_Relationship, "

			"MAX(CASE "
			"WHEN lr.Record_Type = 'CHECK_OUT' "
			"THEN lr.Rep_ID "
			"END) AS Checkout_Rep_ID, "

			"MAX(CASE "
			"WHEN lr.Record_Type = 'CHECK_OUT' "
			"THEN ar.Name "
			"END) AS Representative_Name, "

			"MAX(CASE "
			"WHEN lr.Record_Type = 'CHECK_OUT' "
			"THEN ac.Relationship "
			"END) AS Representative_Relationship, "

			"al.CheckIn_Points, "
			"al.CheckOut_Points, "
			"al.Daily_Points, "
			"al.Late_Pickup_Charge "

			"FROM ATTENDANCE_LOG al "

			"JOIN CHILD c "
			"ON al.Child_ID = c.Child_ID "

			"LEFT JOIN LOG_RECORD lr "
			"ON al.Log_ID = lr.Log_ID "

			"LEFT JOIN PARENT p "
			"ON lr.Parent_ID = p.Parent_ID "

			"LEFT JOIN PARENT_CHILD pc "
			"ON lr.Parent_ID = pc.Parent_ID "
			"AND al.Child_ID = pc.Child_ID "

			"LEFT JOIN AUTHORIZED_REPRESENTATIVE ar "
			"ON lr.Rep_ID = ar.Rep_ID "

			"LEFT JOIN AUTHOREP_CHILD ac "
			"ON lr.Rep_ID = ac.Rep_ID "
			"AND al.Child_ID = ac.Child_ID "

			"WHERE al.Log_Date = ? ";

		const string groupAndOrder =
			"GROUP BY "
			"al.Log_ID, "
			"al.Child_ID, "
			"c.Name, "
			"al.CheckIn_Points, "
			"al.CheckOut_Points, "
			"al.Daily_Points, "
			"al.Late_Pickup_Charge "

			"ORDER BY "
			"CASE "
			"WHEN al.CheckOut_Points IS NULL THEN 1 "
			"WHEN MAX(CASE "
			"WHEN lr.Record_Type = 'CHECK_OUT' "
			"THEN lr.Parent_ID END) IS NOT NULL THEN 2 "
			"WHEN MAX(CASE "
			"WHEN lr.Record_Type = 'CHECK_OUT' "
			"THEN lr.Rep_ID END) IS NOT NULL THEN 3 "
			"ELSE 4 END, "
			"c.Name ASC";

		if (searchChoice == 1)
		{
			attendanceStatement =
				con->prepareStatement(
					baseQuery + groupAndOrder
				);

			attendanceStatement->setString(
				1,
				databaseDate
			);
		}
		else
		{
			attendanceStatement =
				con->prepareStatement(
					baseQuery +
					"AND al.Child_ID = ? " +
					groupAndOrder
				);

			attendanceStatement->setString(
				1,
				databaseDate
			);

			attendanceStatement->setString(
				2,
				childId
			);
		}

		attendanceResult =
			attendanceStatement->executeQuery();

		// ==================================================
		// STORE QUERY RESULTS
		// ==================================================

		while (attendanceResult->next())
		{
			DailyAttendanceRecord record;

			record.logId =
				string(
					attendanceResult->getString(
						"Log_ID"
					).c_str()
				);

			record.childId =
				string(
					attendanceResult->getString(
						"Child_ID"
					).c_str()
				);

			record.childName =
				string(
					attendanceResult->getString(
						"Child_Name"
					).c_str()
				);

			record.checkInTime =
				attendanceResult->isNull("CheckIn_Time")
				? "-"
				: string(
					attendanceResult->getString(
						"CheckIn_Time"
					).c_str()
				);

			record.checkInStatus =
				attendanceResult->isNull("CheckIn_Status")
				? "-"
				: string(
					attendanceResult->getString(
						"CheckIn_Status"
					).c_str()
				);

			record.checkoutCompleted =
				!attendanceResult->isNull(
					"CheckOut_Time"
				);

			record.checkOutTime =
				record.checkoutCompleted
				? string(
					attendanceResult->getString(
						"CheckOut_Time"
					).c_str()
				)
				: "-";

			record.pickupStatus =
				attendanceResult->isNull("Pickup_Status")
				? "PENDING"
				: string(
					attendanceResult->getString(
						"Pickup_Status"
					).c_str()
				);

			record.registeredParents =
				attendanceResult->isNull("Registered_Parents")
				? "No registered parent"
				: string(
					attendanceResult->getString(
						"Registered_Parents"
					).c_str()
				);

			record.collectorType = "-";
			record.collectorId = "-";
			record.collectorName = "-";
			record.relationship = "-";

			if (!attendanceResult->isNull(
				"Checkout_Parent_ID"
			))
			{
				record.collectorType = "PARENT";

				record.collectorId =
					string(
						attendanceResult->getString(
							"Checkout_Parent_ID"
						).c_str()
					);

				record.collectorName =
					attendanceResult->isNull("Parent_Name")
					? "-"
					: string(
						attendanceResult->getString(
							"Parent_Name"
						).c_str()
					);

				record.relationship =
					attendanceResult->isNull(
						"Parent_Relationship"
					)
					? "-"
					: string(
						attendanceResult->getString(
							"Parent_Relationship"
						).c_str()
					);
			}
			else if (!attendanceResult->isNull(
				"Checkout_Rep_ID"
			))
			{
				record.collectorType = "REPRESENTATIVE";

				record.collectorId =
					string(
						attendanceResult->getString(
							"Checkout_Rep_ID"
						).c_str()
					);

				record.collectorName =
					attendanceResult->isNull(
						"Representative_Name"
					)
					? "-"
					: string(
						attendanceResult->getString(
							"Representative_Name"
						).c_str()
					);

				record.relationship =
					attendanceResult->isNull(
						"Representative_Relationship"
					)
					? "-"
					: string(
						attendanceResult->getString(
							"Representative_Relationship"
						).c_str()
					);
			}
			else if (record.checkoutCompleted)
			{
				/*
				 * Applies only to legacy checkout records
				 * created before Parent_ID was stored.
				 */
				record.collectorType = "LEGACY";
				record.collectorId = "-";
				record.collectorName = "-";
				record.relationship = "-";
			}

			record.checkInPoints =
				attendanceResult->isNull("CheckIn_Points")
				? 0.00
				: attendanceResult->getDouble(
					"CheckIn_Points"
				);

			record.checkOutPoints =
				attendanceResult->isNull("CheckOut_Points")
				? 0.00
				: attendanceResult->getDouble(
					"CheckOut_Points"
				);

			record.dailyPoints =
				attendanceResult->isNull("Daily_Points")
				? 0.00
				: attendanceResult->getDouble(
					"Daily_Points"
				);

			record.latePickupCharge =
				attendanceResult->getDouble(
					"Late_Pickup_Charge"
				);

			records.push_back(record);
		}

		// ==================================================
		// HANDLE NO RECORDS
		// ==================================================

		if (records.empty())
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "              NO ATTENDANCE RECORDS FOUND                  \n";
			cout << "============================================================\n";
			cout << " No matching attendance records were found "
				<< "for " << attendanceDate << ".\n";
			cout << "============================================================\n";

			delete attendanceResult;
			attendanceResult = nullptr;

			delete attendanceStatement;
			attendanceStatement = nullptr;

			return;
		}

		// ==================================================
		// DISPLAY TABLE HEADER
		// ==================================================

		cout << "\n";
		cout << "DAILY ATTENDANCE TABLE\n";
		cout << "Date: " << attendanceDate << "\n\n";

		cout << left
			<< setw(4) << "No"
			<< setw(7) << "Child"
			<< setw(18) << "Child Name"
			<< setw(10) << "In Time"
			<< setw(10) << "In Status"
			<< setw(10) << "Out Time"
			<< setw(13) << "Pickup"
			<< setw(15) << "Collector"
			<< setw(8) << "ID"
			<< setw(16) << "Relationship"
			<< setw(8) << "Points"
			<< setw(10) << "Charge"
			<< "\n";

		cout << string(129, '=') << "\n";

		// ==================================================
		// DISPLAY TABLE ROWS
		// ==================================================

		int rowNumber = 0;

		for (const DailyAttendanceRecord& record : records)
		{
			rowNumber++;

			string displayedName = record.childName;

			if (displayedName.length() > 16)
			{
				displayedName =
					displayedName.substr(0, 16);
			}

			string displayedCollector =
				record.collectorType;

			if (displayedCollector == "REPRESENTATIVE")
			{
				displayedCollector = "REP";
			}

			string chargeText;

			if (!record.checkoutCompleted)
			{
				chargeText = "-";
			}
			else
			{
				ostringstream chargeStream;

				chargeStream << "RM"
					<< fixed
					<< setprecision(2)
					<< record.latePickupCharge;

				chargeText = chargeStream.str();
			}

			string pointsText;

			if (!record.checkoutCompleted)
			{
				pointsText = "-";
			}
			else
			{
				ostringstream pointsStream;

				pointsStream << fixed
					<< setprecision(2)
					<< record.dailyPoints;

				pointsText = pointsStream.str();
			}

			cout << left
				<< setw(4) << rowNumber
				<< setw(7) << record.childId
				<< setw(18) << displayedName
				<< setw(10) << record.checkInTime
				<< setw(10) << formatAttendanceDisplayText(record.checkInStatus)
				<< setw(10) << record.checkOutTime
				<< setw(13) << formatAttendanceDisplayText(record.pickupStatus)
				<< setw(15) << displayedCollector
				<< setw(8) << record.collectorId
				<< setw(16) << formatAttendanceDisplayText(record.relationship)
				<< setw(8) << pointsText
				<< setw(10) << chargeText
				<< "\n";
		}

		cout << string(129, '=') << "\n";

		// ==================================================
		// DAILY SUMMARY CALCULATION
		// ==================================================

		int totalPresent = 0;
		int totalCheckInOnTime = 0;
		int totalCheckInLate = 0;

		int totalPickupOnTime = 0;
		int totalLatePickup = 0;
		int totalPendingCheckout = 0;

		int totalParentPickup = 0;
		int totalRepresentativePickup = 0;
		int totalLegacyPickup = 0;

		double totalDailyPoints = 0.0;
		double totalLatePickupCharges = 0.0;

		int completedRecords = 0;

		for (const DailyAttendanceRecord& record : records)
		{
			totalPresent++;

			if (record.checkInStatus == "ON_TIME")
			{
				totalCheckInOnTime++;
			}
			else if (record.checkInStatus == "LATE")
			{
				totalCheckInLate++;
			}

			if (!record.checkoutCompleted)
			{
				totalPendingCheckout++;
				continue;
			}

			completedRecords++;

			if (record.pickupStatus == "ON_TIME")
			{
				totalPickupOnTime++;
			}
			else if (record.pickupStatus == "LATE_PICKUP")
			{
				totalLatePickup++;
			}

			if (record.collectorType == "PARENT")
			{
				totalParentPickup++;
			}
			else if (
				record.collectorType == "REPRESENTATIVE"
				)
			{
				totalRepresentativePickup++;
			}
			else if (record.collectorType == "LEGACY")
			{
				totalLegacyPickup++;
			}

			totalDailyPoints += record.dailyPoints;

			totalLatePickupCharges +=
				record.latePickupCharge;
		}

		double averageDailyPoints = 0.0;

		if (completedRecords > 0)
		{
			averageDailyPoints =
				totalDailyPoints /
				static_cast<double>(completedRecords);
		}

		// ==================================================
		// DISPLAY DAILY SUMMARY
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "                  DAILY ATTENDANCE SUMMARY                 \n";
		cout << "============================================================\n";
		cout << " Attendance Date          : "
			<< attendanceDate << "\n";
		cout << " Total Present            : "
			<< totalPresent << "\n";
		cout << " Check-In On Time         : "
			<< totalCheckInOnTime << "\n";
		cout << " Check-In Late            : "
			<< totalCheckInLate << "\n";
		cout << " Pickup On Time           : "
			<< totalPickupOnTime << "\n";
		cout << " Late Pickup              : "
			<< totalLatePickup << "\n";
		cout << " Pending Check-Out        : "
			<< totalPendingCheckout << "\n";
		cout << " Pickup by Parent         : "
			<< totalParentPickup << "\n";
		cout << " Pickup by Representative : "
			<< totalRepresentativePickup << "\n";

		if (totalLegacyPickup > 0)
		{
			cout << " Legacy Collector Records : "
				<< totalLegacyPickup << "\n";
		}

		cout << " Completed Records        : "
			<< completedRecords << "\n";
		cout << " Average Daily Points     : "
			<< fixed << setprecision(2)
			<< averageDailyPoints << "\n";
		cout << " Total Late Charges       : RM"
			<< fixed << setprecision(2)
			<< totalLatePickupCharges << "\n";
		cout << "============================================================\n";

		// ==================================================
		// DISPLAY CHILD, PARENT AND COLLECTOR DETAILS
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "          CHILD, PARENT AND COLLECTOR DETAILS              \n";
		cout << "============================================================\n";

		for (const DailyAttendanceRecord& record : records)
		{
			cout << " Child              : "
				<< record.childId
				<< " | "
				<< record.childName
				<< "\n";

			cout << " Registered Parent  : "
				<< formatAttendanceDisplayText(record.registeredParents)
				<< "\n";

			if (!record.checkoutCompleted)
			{
				cout << " Actual Collector   : Pending Check-Out\n";
				cout << " Collector Type     : PENDING\n";
				cout << " Relationship       : -\n";
			}
			else if (record.collectorType == "PARENT" ||
				record.collectorType == "REPRESENTATIVE")
			{
				cout << " Actual Collector   : "
					<< record.collectorId
					<< " | "
					<< record.collectorName
					<< "\n";

				cout << " Collector Type     : "
					<< formatAttendanceDisplayText(record.collectorType)
					<< "\n";

				cout << " Relationship       : "
					<< formatAttendanceDisplayText(record.relationship)
					<< "\n";
			}
			else
			{
				cout << " Actual Collector   : Not Available\n";
				cout << " Collector Type     : LEGACY\n";
				cout << " Relationship       : Not Available\n";
			}

			cout << "------------------------------------------------------------\n";
		}

		cout << "============================================================\n";

		// ==================================================
// PDF EXPORT OPTION
// ==================================================

		int exportChoice = 0;

		while (true)
		{
			cout << "\nExport this Daily Attendance Report to PDF?\n";
			cout << " 1. Export PDF\n";
			cout << " 2. Return to Teacher Main Menu\n";
			cout << "\n Select an option [1-2] : ";

			if (!(cin >> exportChoice))
			{
				cin.clear();
				clearInputBuffer();

				cout << "\nERROR: Please enter either 1 or 2.\n";
				continue;
			}

			clearInputBuffer();

			if (exportChoice == 1 ||
				exportChoice == 2)
			{
				break;
			}

			cout << "\nERROR: Please select either 1 or 2.\n";
		}

		if (exportChoice == 1)
		{
			exportDailyAttendancePdf(
				records,
				attendanceDate,
				session
			);
		}

		// ==================================================
		// CLEANUP
		// ==================================================

		delete attendanceResult;
		attendanceResult = nullptr;

		delete attendanceStatement;
		attendanceStatement = nullptr;
	}
	catch (sql::SQLException& e)
	{
		delete attendanceResult;
		attendanceResult = nullptr;

		delete attendanceStatement;
		attendanceStatement = nullptr;

		cerr << "\nSQL Error while viewing Daily Attendance:\n";
		cerr << "Message    : " << e.what() << "\n";
		cerr << "Error Code : " << e.getErrorCode() << "\n";
		cerr << "SQL State  : " << e.getSQLState() << "\n";
	}
}

// ==========================================================
// EXPORT DAILY ATTENDANCE PDF
// ==========================================================

bool exportDailyAttendancePdf(
	const vector<DailyAttendanceRecord>& records,
	const string& attendanceDate,
	const UserSession& session
)
{
	if (records.empty())
	{
		cout << "\nERROR: No attendance records "
			<< "are available for PDF export.\n";

		return false;
	}

	try
	{
		const string outputFolder =
			"CAPMS_Reports";

		fs::create_directories(
			outputFolder
		);

		// Convert DD/MM/YYYY to YYYY-MM-DD for file name.
		string fileDate = attendanceDate;

		if (attendanceDate.length() == 10)
		{
			fileDate =
				attendanceDate.substr(6, 4) +
				"-" +
				attendanceDate.substr(3, 2) +
				"-" +
				attendanceDate.substr(0, 2);
		}

		const string baseFileName =
			"Daily_Attendance_" +
			fileDate;

		const string htmlFilePath =
			outputFolder +
			"\\" +
			baseFileName +
			".html";

		const string pdfFilePath =
			outputFolder +
			"\\" +
			baseFileName +
			".pdf";

		// ==================================================
		// CALCULATE DAILY SUMMARY
		// ==================================================

		int totalPresent = 0;
		int totalCheckInOnTime = 0;
		int totalCheckInLate = 0;

		int totalPickupOnTime = 0;
		int totalLatePickup = 0;
		int totalPendingCheckout = 0;

		int totalParentPickup = 0;
		int totalRepresentativePickup = 0;
		int totalLegacyPickup = 0;

		int completedRecords = 0;

		double totalDailyPoints = 0.0;
		double totalLatePickupCharges = 0.0;

		for (const DailyAttendanceRecord& record : records)
		{
			totalPresent++;

			if (record.checkInStatus == "ON_TIME")
			{
				totalCheckInOnTime++;
			}
			else if (record.checkInStatus == "LATE")
			{
				totalCheckInLate++;
			}

			if (!record.checkoutCompleted)
			{
				totalPendingCheckout++;
				continue;
			}

			completedRecords++;

			if (record.pickupStatus == "ON_TIME")
			{
				totalPickupOnTime++;
			}
			else if (record.pickupStatus == "LATE_PICKUP")
			{
				totalLatePickup++;
			}

			if (record.collectorType == "PARENT")
			{
				totalParentPickup++;
			}
			else if (
				record.collectorType == "REPRESENTATIVE"
				)
			{
				totalRepresentativePickup++;
			}
			else if (record.collectorType == "LEGACY")
			{
				totalLegacyPickup++;
			}

			totalDailyPoints +=
				record.dailyPoints;

			totalLatePickupCharges +=
				record.latePickupCharge;
		}

		double averageDailyPoints = 0.0;

		if (completedRecords > 0)
		{
			averageDailyPoints =
				totalDailyPoints /
				static_cast<double>(
					completedRecords
					);
		}

		// ==================================================
		// CREATE HTML FILE
		// ==================================================

		ofstream htmlFile(
			htmlFilePath
		);

		if (!htmlFile.is_open())
		{
			cerr << "\nERROR: Unable to create "
				<< "Daily Attendance HTML file.\n";

			return false;
		}

		htmlFile << R"HTML(
<!DOCTYPE html>
<html lang="en">

<head>
<meta charset="UTF-8">

<style>
@page
{
    size: A4 landscape;
    margin: 12mm;
}

*
{
    box-sizing: border-box;
}

body
{
    margin: 0;
    color: #1f2937;
    font-family: Arial, Helvetica, sans-serif;
    font-size: 9pt;
    line-height: 1.35;
}

.report-header
{
    margin-bottom: 16px;
    padding-bottom: 10px;
    border-bottom: 3px solid #2563eb;
}

.report-title
{
    margin: 0;
    color: #1e3a8a;
    font-size: 20pt;
    font-weight: bold;
}

.report-subtitle
{
    margin-top: 4px;
    color: #4b5563;
}

.information-table
{
    width: 100%;
    margin-bottom: 16px;
    border-collapse: collapse;
}

.information-table td
{
    padding: 4px 8px 4px 0;
    border: none;
}

.information-label
{
    width: 130px;
    color: #374151;
    font-weight: bold;
}

h2
{
    margin-top: 18px;
    margin-bottom: 8px;
    color: #1e3a8a;
    font-size: 13pt;
}

.attendance-table
{
    width: 100%;
    margin-bottom: 18px;
    border-collapse: collapse;
    table-layout: fixed;
}

.attendance-table th
{
    padding: 7px 4px;
    border: 1px solid #cbd5e1;
    background-color: #1e3a8a;
    color: white;
    font-size: 8pt;
    text-align: center;
}

.attendance-table td
{
    padding: 6px 4px;
    border: 1px solid #cbd5e1;
    text-align: center;
    vertical-align: middle;
    overflow-wrap: break-word;
}

.attendance-table tbody tr:nth-child(even)
{
    background-color: #f8fafc;
}

.child-name
{
    text-align: left !important;
}

.status-on-time
{
    color: #166534;
    font-weight: bold;
}

.status-late
{
    color: #b91c1c;
    font-weight: bold;
}

.status-pending
{
    color: #92400e;
    font-weight: bold;
}

.summary-table
{
    width: 100%;
    margin-bottom: 18px;
    border-collapse: collapse;
}

.summary-table td
{
    padding: 7px 9px;
    border: 1px solid #d1d5db;
    vertical-align: middle;
}

.summary-label
{
    width: 20%;
    background-color: #eff6ff;
    font-weight: bold;
}

.summary-value
{
    width: 13%;
    text-align: center;
}

.charge-value
{
    color: #b91c1c;
    font-weight: bold;
}

.collector-table
{
    width: 100%;
    border-collapse: collapse;
}

.collector-table th
{
    padding: 7px;
    background-color: #334155;
    color: white;
    text-align: left;
}

.collector-table td
{
    padding: 7px;
    border: 1px solid #d1d5db;
    vertical-align: middle;
}

.report-footer
{
    margin-top: 22px;
    padding-top: 8px;
    border-top: 1px solid #cbd5e1;
    color: #64748b;
    font-size: 8pt;
}

thead
{
    display: table-header-group;
}

tr
{
    page-break-inside: avoid;
}
</style>

</head>

<body>

<div class="report-header">
    <div class="report-title">
        Daily Attendance Report
    </div>

    <div class="report-subtitle">
        Childcare Attendance and Pickup Management System
    </div>
</div>
)HTML";

		// ==================================================
		// REPORT INFORMATION
		// ==================================================

		htmlFile << "<table class=\"information-table\">\n";

		htmlFile << "<tr>\n";

		htmlFile << "<td class=\"information-label\">"
			<< "Attendance Date"
			<< "</td>\n";

		htmlFile << "<td>: "
			<< escapeHtml(attendanceDate)
			<< "</td>\n";

		htmlFile << "<td class=\"information-label\">"
			<< "Generated By"
			<< "</td>\n";

		htmlFile << "<td>: "
			<< escapeHtml(session.staffName)
			<< "</td>\n";

		htmlFile << "</tr>\n";

		htmlFile << "<tr>\n";

		htmlFile << "<td class=\"information-label\">"
			<< "Report Type"
			<< "</td>\n";

		htmlFile << "<td>: Daily Attendance</td>\n";

		htmlFile << "<td class=\"information-label\">"
			<< "Staff ID"
			<< "</td>\n";

		htmlFile << "<td>: "
			<< escapeHtml(session.staffId)
			<< "</td>\n";

		htmlFile << "</tr>\n";

		htmlFile << "</table>\n";

		// ==================================================
		// ATTENDANCE RECORDS
		// ==================================================

		htmlFile << "<h2>Attendance Records</h2>\n";

		htmlFile << "<table class=\"attendance-table\">\n";

		htmlFile << "<colgroup>\n";
		htmlFile << "<col style=\"width:3%\">\n";
		htmlFile << "<col style=\"width:6%\">\n";
		htmlFile << "<col style=\"width:13%\">\n";
		htmlFile << "<col style=\"width:7%\">\n";
		htmlFile << "<col style=\"width:8%\">\n";
		htmlFile << "<col style=\"width:7%\">\n";
		htmlFile << "<col style=\"width:10%\">\n";
		htmlFile << "<col style=\"width:10%\">\n";
		htmlFile << "<col style=\"width:7%\">\n";
		htmlFile << "<col style=\"width:10%\">\n";
		htmlFile << "<col style=\"width:8%\">\n";
		htmlFile << "<col style=\"width:11%\">\n";
		htmlFile << "</colgroup>\n";

		htmlFile << "<thead>\n";
		htmlFile << "<tr>\n";
		htmlFile << "<th>No.</th>\n";
		htmlFile << "<th>Child ID</th>\n";
		htmlFile << "<th>Child Name</th>\n";
		htmlFile << "<th>Check-In</th>\n";
		htmlFile << "<th>In Status</th>\n";
		htmlFile << "<th>Check-Out</th>\n";
		htmlFile << "<th>Pickup Status</th>\n";
		htmlFile << "<th>Collector Type</th>\n";
		htmlFile << "<th>Collector ID</th>\n";
		htmlFile << "<th>Relationship</th>\n";
		htmlFile << "<th>Points</th>\n";
		htmlFile << "<th>Late Charge</th>\n";
		htmlFile << "</tr>\n";
		htmlFile << "</thead>\n";

		htmlFile << "<tbody>\n";

		int rowNumber = 0;

		for (const DailyAttendanceRecord& record : records)
		{
			rowNumber++;

			string checkInClass =
				"status-late";

			if (record.checkInStatus == "ON_TIME")
			{
				checkInClass =
					"status-on-time";
			}

			string pickupClass =
				"status-late";

			if (!record.checkoutCompleted)
			{
				pickupClass =
					"status-pending";
			}
			else if (record.pickupStatus == "ON_TIME")
			{
				pickupClass =
					"status-on-time";
			}

			string collectorDisplay =
				record.collectorType;

			if (collectorDisplay == "REPRESENTATIVE")
			{
				collectorDisplay =
					"AUTHORIZED REP.";
			}

			string pointsText = "-";
			string chargeText = "-";

			if (record.checkoutCompleted)
			{
				ostringstream pointsStream;
				ostringstream chargeStream;

				pointsStream
					<< fixed
					<< setprecision(2)
					<< record.dailyPoints;

				chargeStream
					<< "RM"
					<< fixed
					<< setprecision(2)
					<< record.latePickupCharge;

				pointsText =
					pointsStream.str();

				chargeText =
					chargeStream.str();
			}

			htmlFile << "<tr>\n";

			htmlFile << "<td>"
				<< rowNumber
				<< "</td>\n";

			htmlFile << "<td>"
				<< escapeHtml(record.childId)
				<< "</td>\n";

			htmlFile << "<td class=\"child-name\">"
				<< escapeHtml(record.childName)
				<< "</td>\n";

			htmlFile << "<td>"
				<< escapeHtml(record.checkInTime)
				<< "</td>\n";

			htmlFile << "<td class=\""
				<< checkInClass
				<< "\">"
				<< escapeHtml(formatAttendanceDisplayText(record.checkInStatus))
				<< "</td>\n";

			htmlFile << "<td>"
				<< escapeHtml(record.checkOutTime)
				<< "</td>\n";

			htmlFile << "<td class=\""
				<< pickupClass
				<< "\">"
				<< escapeHtml(formatAttendanceDisplayText(record.pickupStatus))
				<< "</td>\n";

			htmlFile << "<td>"
				<< escapeHtml(collectorDisplay)
				<< "</td>\n";

			htmlFile << "<td>"
				<< escapeHtml(record.collectorId)
				<< "</td>\n";

			htmlFile << "<td>"
				<< escapeHtml(formatAttendanceDisplayText(record.relationship))
				<< "</td>\n";

			htmlFile << "<td>"
				<< pointsText
				<< "</td>\n";

			htmlFile << "<td>"
				<< chargeText
				<< "</td>\n";

			htmlFile << "</tr>\n";
		}

		htmlFile << "</tbody>\n";
		htmlFile << "</table>\n";

		// ==================================================
		// DAILY SUMMARY
		// ==================================================

		htmlFile << "<h2>Daily Attendance Summary</h2>\n";
		htmlFile << "<table class=\"summary-table\">\n";

		htmlFile << "<tr>\n";

		htmlFile << "<td class=\"summary-label\">"
			<< "Total Present"
			<< "</td>\n";

		htmlFile << "<td class=\"summary-value\">"
			<< totalPresent
			<< "</td>\n";

		htmlFile << "<td class=\"summary-label\">"
			<< "Completed Records"
			<< "</td>\n";

		htmlFile << "<td class=\"summary-value\">"
			<< completedRecords
			<< "</td>\n";

		htmlFile << "<td class=\"summary-label\">"
			<< "Pending Check-Out"
			<< "</td>\n";

		htmlFile << "<td class=\"summary-value\">"
			<< totalPendingCheckout
			<< "</td>\n";

		htmlFile << "</tr>\n";

		htmlFile << "<tr>\n";

		htmlFile << "<td class=\"summary-label\">"
			<< "Check-In On Time"
			<< "</td>\n";

		htmlFile << "<td class=\"summary-value\">"
			<< totalCheckInOnTime
			<< "</td>\n";

		htmlFile << "<td class=\"summary-label\">"
			<< "Check-In Late"
			<< "</td>\n";

		htmlFile << "<td class=\"summary-value\">"
			<< totalCheckInLate
			<< "</td>\n";

		htmlFile << "<td class=\"summary-label\">"
			<< "Pickup On Time"
			<< "</td>\n";

		htmlFile << "<td class=\"summary-value\">"
			<< totalPickupOnTime
			<< "</td>\n";

		htmlFile << "</tr>\n";

		htmlFile << "<tr>\n";

		htmlFile << "<td class=\"summary-label\">"
			<< "Late Pickup"
			<< "</td>\n";

		htmlFile << "<td class=\"summary-value\">"
			<< totalLatePickup
			<< "</td>\n";

		htmlFile << "<td class=\"summary-label\">"
			<< "Pickup by Parent"
			<< "</td>\n";

		htmlFile << "<td class=\"summary-value\">"
			<< totalParentPickup
			<< "</td>\n";

		htmlFile << "<td class=\"summary-label\">"
			<< "Pickup by Representative"
			<< "</td>\n";

		htmlFile << "<td class=\"summary-value\">"
			<< totalRepresentativePickup
			<< "</td>\n";

		htmlFile << "</tr>\n";

		htmlFile << "<tr>\n";

		htmlFile << "<td class=\"summary-label\">"
			<< "Average Daily Points"
			<< "</td>\n";

		htmlFile << "<td class=\"summary-value\">"
			<< fixed
			<< setprecision(2)
			<< averageDailyPoints
			<< "</td>\n";

		htmlFile << "<td class=\"summary-label\">"
			<< "Total Late Charges"
			<< "</td>\n";

		htmlFile << "<td class=\"summary-value charge-value\">"
			<< "RM"
			<< fixed
			<< setprecision(2)
			<< totalLatePickupCharges
			<< "</td>\n";

		htmlFile << "<td class=\"summary-label\">"
			<< "Legacy Records"
			<< "</td>\n";

		htmlFile << "<td class=\"summary-value\">"
			<< totalLegacyPickup
			<< "</td>\n";

		htmlFile << "</tr>\n";

		htmlFile << "</table>\n";

		// ==================================================
		// COLLECTOR DETAILS
		// ==================================================

		htmlFile << "<h2>Child, Parent and Collector Details</h2>\n";
		htmlFile << "<table class=\"collector-table\">\n";

		htmlFile << "<thead>\n";
		htmlFile << "<tr>\n";
		htmlFile << "<th>Child</th>\n";
		htmlFile << "<th>Registered Parents / Guardians</th>\n";
		htmlFile << "<th>Collector Type</th>\n";
		htmlFile << "<th>Collector ID</th>\n";
		htmlFile << "<th>Collector Name</th>\n";
		htmlFile << "<th>Relationship</th>\n";
		htmlFile << "</tr>\n";
		htmlFile << "</thead>\n";

		htmlFile << "<tbody>\n";

		for (const DailyAttendanceRecord& record : records)
		{
			string actualCollectorType = record.collectorType;
			string actualCollectorId = record.collectorId;
			string actualCollectorName = record.collectorName;
			string actualRelationship = record.relationship;

			if (!record.checkoutCompleted)
			{
				actualCollectorType = "PENDING";
				actualCollectorId = "-";
				actualCollectorName = "Pending Check-Out";
				actualRelationship = "-";
			}
			else if (record.collectorType == "LEGACY")
			{
				actualCollectorId = "-";
				actualCollectorName = "Not Available";
				actualRelationship = "Not Available";
			}

			htmlFile << "<tr>\n";

			htmlFile << "<td>"
				<< escapeHtml(record.childId)
				<< " - "
				<< escapeHtml(record.childName)
				<< "</td>\n";

			htmlFile << "<td>"
				<< escapeHtml(formatAttendanceDisplayText(record.registeredParents))
				<< "</td>\n";

			htmlFile << "<td>"
				<< escapeHtml(formatAttendanceDisplayText(actualCollectorType))
				<< "</td>\n";

			htmlFile << "<td>"
				<< escapeHtml(actualCollectorId)
				<< "</td>\n";

			htmlFile << "<td>"
				<< escapeHtml(actualCollectorName)
				<< "</td>\n";

			htmlFile << "<td>"
				<< escapeHtml(formatAttendanceDisplayText(actualRelationship))
				<< "</td>\n";

			htmlFile << "</tr>\n";
		}

		htmlFile << "</tbody>\n";
		htmlFile << "</table>\n";

		// ==================================================
		// REPORT FOOTER
		// ==================================================

		htmlFile << "<div class=\"report-footer\">\n";

		htmlFile << "Generated by CAPMS. "
			<< "A fixed RM5.00 late pickup charge is applied "
			<< "for each pickup recorded after 12:30 PM.";

		htmlFile << "</div>\n";

		htmlFile << "</body>\n";
		htmlFile << "</html>\n";

		htmlFile.close();

		// ==================================================
		// CONVERT HTML TO PDF
		// ==================================================

		cout << "\nGenerating Daily Attendance PDF...\n";

		const bool exportSuccessful =
			convertHtmlToPdf(
				htmlFilePath,
				pdfFilePath
			);

		if (!exportSuccessful)
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "          DAILY ATTENDANCE PDF EXPORT FAILED               \n";
			cout << "============================================================\n";

			return false;
		}

		cout << "\n";
		cout << "============================================================\n";
		cout << "            DAILY ATTENDANCE PDF EXPORTED                  \n";
		cout << "============================================================\n";
		cout << " Attendance Date : "
			<< attendanceDate << "\n";
		cout << " PDF File        : "
			<< fs::absolute(
				pdfFilePath
			).string()
			<< "\n";
		cout << "============================================================\n";

		return true;
	}
	catch (const fs::filesystem_error& e)
	{
		cerr << "\nFile system error while exporting "
			<< "Daily Attendance PDF:\n";

		cerr << e.what() << "\n";

		return false;
	}
	catch (const exception& e)
	{
		cerr << "\nUnexpected error while exporting "
			<< "Daily Attendance PDF:\n";

		cerr << e.what() << "\n";

		return false;
	}
}

// ==========================================================
// GENERATE ATTENDANCE LOG ID
// ==========================================================

string generateLogId(sql::Connection* con)
{
	sql::Statement* statement = nullptr;
	sql::ResultSet* result = nullptr;

	int nextNumber = 1;

	try
	{
		statement = con->createStatement();

		result = statement->executeQuery(
			"SELECT MAX("
			"CAST(SUBSTRING(Log_ID, 2) AS UNSIGNED)"
			") AS Maximum_ID "
			"FROM ATTENDANCE_LOG"
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

	idStream << 'L'
		<< setw(4)
		<< setfill('0')
		<< nextNumber;

	return idStream.str();
}


// ==========================================================
// CHILD ID VALIDATION
// ==========================================================

bool isValidAttendanceChildId(
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


// ==========================================================
// STAFF ID VALIDATION
// ==========================================================

bool isValidAttendanceStaffId(
	const string& staffId
)
{
	if (staffId.length() != 5 ||
		staffId[0] != 'S')
	{
		return false;
	}

	for (size_t index = 1;
		index < staffId.length();
		index++)
	{
		if (!std::isdigit(
			static_cast<unsigned char>(
				staffId[index]
				)
		))
		{
			return false;
		}
	}

	return true;
}

// ==========================================================
// PARENT ID VALIDATION FOR PICKUP
// ==========================================================

bool isValidPickupParentId(
	const string& parentId
)
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
// REPRESENTATIVE ID VALIDATION FOR PICKUP
// ==========================================================

bool isValidPickupRepresentativeId(
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
// ATTENDANCE DATE VALIDATION
// ==========================================================

bool isValidAttendanceDate(
	const string& displayDate
)
{
	// Expected format: DD/MM/YYYY
	if (displayDate.length() != 10)
	{
		return false;
	}

	if (displayDate[2] != '/' ||
		displayDate[5] != '/')
	{
		return false;
	}

	for (size_t index = 0;
		index < displayDate.length();
		index++)
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

	const bool leapYear =
		(year % 400 == 0) ||
		(year % 4 == 0 &&
			year % 100 != 0);

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

string convertAttendanceDateToDatabase(
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
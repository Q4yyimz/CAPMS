#include "Report.h"
#include "InputUtils.h"
#include "PdfExporter.h"
#include "MonthlyReportPdf.h"
#include "YearlyReportPdf.h"
#include "AllChildrenMonthlyReport.h"
#include "AllChildrenYearlyReport.h"

#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

#include <cctype>
#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>

using namespace std;

// ==========================================================
// MANAGE REPORTS
// ==========================================================

void manageReports(
	sql::Connection* con,
	const UserSession& session
)
{
	int choice = 0;

	do
	{
		cout << "\n";
		cout << "============================================================\n";
		cout << "                      MANAGE REPORTS                       \n";
		cout << "============================================================\n";
		cout << " Logged-In Administrator : "
			<< session.staffName
			<< " (" << session.staffId << ")\n";
		cout << "------------------------------------------------------------\n";
		cout << " 1. Individual Monthly Report\n";
		cout << " 2. All Children Monthly Report\n";
		cout << " 3. Individual Yearly Report\n";
		cout << " 4. All Children Yearly Report\n";
		cout << " 5. Export Individual Monthly PDF\n";
		cout << " 6. Export Individual Yearly PDF\n";
		cout << " 7. Back to Admin Main Menu\n";
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
			monthlyAnalyticsReport(
				con,
				session
			);
			break;

		case 2:
			allChildrenMonthlyPerformanceReport(
				con,
				session
			);
			break;

		case 3:
			yearlyChildPerformanceSummary(
				con,
				session
			);
			break;

		case 4:
			allChildrenYearlyPerformanceReport(
				con,
				session
			);
			break;

		case 5:
			exportMonthlyChildReportPdf(
				con,
				session
			);
			break;

		case 6:
			exportYearlyChildReportPdf(
				con,
				session
			);
			break;

		case 7:
			cout << "\nReturning to Admin Main Menu...\n";
			break;

		default:
			cout << "\nERROR: Please select an option from 1 to 7.\n";
			break;
		}

	} while (choice != 7);
}

// ==========================================================
// MONTHLY ANALYTICS AND PERFORMANCE REPORT
// ==========================================================

void monthlyAnalyticsReport(
	sql::Connection* con,
	const UserSession& session
)
{
	string childId;
	string childName;

	const string staffId = session.staffId;
	const string adminName = session.staffName;

	int reportMonth = 0;
	int reportYear = 0;

	int totalDaysPresent = 0;
	int completedRecords = 0;
	int pendingRecords = 0;

	int totalOperatingDays = 0;
	int totalDaysAbsent = 0;

	double attendancePercentage = 0.0;

	int onTimeCheckIns = 0;
	int lateCheckIns = 0;
	int onTimePickups = 0;
	int latePickups = 0;

	double totalCheckInPoints = 0.0;
	double totalCheckOutPoints = 0.0;
	double totalDailyPoints = 0.0;

	double averageCheckInPoints = 0.0;
	double averageCheckOutPoints = 0.0;
	double averageDailyPoints = 0.0;

	double checkInPerformance = 0.0;
	double checkOutPerformance = 0.0;
	double overallPerformance = 0.0;

	double totalLatePickupCharges = 0.0;

	sql::PreparedStatement* childStatement = nullptr;
	sql::PreparedStatement* operatingDayStatement = nullptr;
	sql::PreparedStatement* reportStatement = nullptr;
	sql::PreparedStatement* detailStatement = nullptr;

	sql::ResultSet* childResult = nullptr;
	sql::ResultSet* reportResult = nullptr;
	sql::ResultSet* detailResult = nullptr;
	sql::ResultSet* operatingDayResult = nullptr;

	const string monthNames[12] =
	{
		"JANUARY",
		"FEBRUARY",
		"MARCH",
		"APRIL",
		"MAY",
		"JUNE",
		"JULY",
		"AUGUST",
		"SEPTEMBER",
		"OCTOBER",
		"NOVEMBER",
		"DECEMBER"
	};

	cout << "\n";
	cout << "============================================================\n";
	cout << "          INDIVIDUAL MONTHLY PERFORMANCE REPORT             \n";
	cout << "============================================================\n";

	cout << " Logged-In Administrator : "
		<< adminName
		<< " (" << staffId << ")\n";

	cout << "------------------------------------------------------------\n";

	// ======================================================
	// ENTER AND VALIDATE CHILD ID
	// ======================================================

	cout << " Enter Child ID               : ";
	getline(cin, childId);

	for (char& character : childId)
	{
		character = static_cast<char>(
			std::toupper(
				static_cast<unsigned char>(character)
			)
			);
	}

	if (!isValidReportChildId(childId))
	{
		cout << "\nERROR: Invalid Child ID format.\n";
		cout << "Expected format: C0001\n";
		return;
	}

	// ======================================================
	// SELECT REPORT MONTH
	// ======================================================

	cout << "\n Report Month:\n";
	cout << "  1. JANUARY\n";
	cout << "  2. FEBRUARY\n";
	cout << "  3. MARCH\n";
	cout << "  4. APRIL\n";
	cout << "  5. MAY\n";
	cout << "  6. JUNE\n";
	cout << "  7. JULY\n";
	cout << "  8. AUGUST\n";
	cout << "  9. SEPTEMBER\n";
	cout << " 10. OCTOBER\n";
	cout << " 11. NOVEMBER\n";
	cout << " 12. DECEMBER\n";

	cout << "\n Select Report Month [1-12]   : ";

	if (!(cin >> reportMonth))
	{
		cin.clear();
		clearInputBuffer();

		cout << "\nERROR: Please enter a number from 1 to 12.\n";
		return;
	}

	if (reportMonth < 1 || reportMonth > 12)
	{
		clearInputBuffer();

		cout << "\nERROR: Please select a month from 1 to 12.\n";
		return;
	}

	// ======================================================
	// ENTER AND VALIDATE REPORT YEAR
	// ======================================================

	cout << " Enter Report Year            : ";

	if (!(cin >> reportYear))
	{
		cin.clear();
		clearInputBuffer();

		cout << "\nERROR: Please enter a valid year.\n";
		return;
	}

	clearInputBuffer();

	if (!isValidReportYear(reportYear))
	{
		cout << "\nERROR: Report year must be "
			<< "between 2020 and 2100.\n";

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

		// ==================================================
// RETRIEVE TOTAL OPERATING DAYS
// ==================================================

		operatingDayStatement = con->prepareStatement(
			"SELECT COUNT(*) AS Total_Operating_Days "
			"FROM OPERATING_DAY "
			"WHERE MONTH(Operating_Date) = ? "
			"AND YEAR(Operating_Date) = ? "
			"AND Day_Status = 'OPEN'"
		);

		operatingDayStatement->setInt(
			1,
			reportMonth
		);

		operatingDayStatement->setInt(
			2,
			reportYear
		);

		operatingDayResult =
			operatingDayStatement->executeQuery();

		if (operatingDayResult->next())
		{
			totalOperatingDays =
				operatingDayResult->getInt(
					"Total_Operating_Days"
				);
		}


		// ==================================================
		// CALCULATE MONTHLY ANALYTICS
		// ==================================================

		reportStatement = con->prepareStatement(
			"SELECT "

			"COUNT(*) AS Total_Days_Present, "

			"SUM(CASE "
			"WHEN al.CheckOut_Points IS NOT NULL "
			"THEN 1 ELSE 0 END) AS Completed_Records, "

			"SUM(CASE "
			"WHEN al.CheckOut_Points IS NULL "
			"THEN 1 ELSE 0 END) AS Pending_Records, "

			"SUM(CASE "
			"WHEN ci.Status = 'ON_TIME' "
			"THEN 1 ELSE 0 END) AS On_Time_CheckIns, "

			"SUM(CASE "
			"WHEN ci.Status = 'LATE' "
			"THEN 1 ELSE 0 END) AS Late_CheckIns, "

			"SUM(CASE "
			"WHEN co.Status = 'ON_TIME' "
			"THEN 1 ELSE 0 END) AS On_Time_Pickups, "

			"SUM(CASE "
			"WHEN co.Status = 'LATE_PICKUP' "
			"THEN 1 ELSE 0 END) AS Late_Pickups, "

			"COALESCE(SUM(al.CheckIn_Points), 0.00) "
			"AS Total_CheckIn_Points, "

			"COALESCE(SUM(al.CheckOut_Points), 0.00) "
			"AS Total_CheckOut_Points, "

			"COALESCE(SUM(al.Daily_Points), 0.00) "
			"AS Total_Daily_Points, "

			"COALESCE(AVG(al.CheckIn_Points), 0.00) "
			"AS Average_CheckIn_Points, "

			"COALESCE(AVG(al.CheckOut_Points), 0.00) "
			"AS Average_CheckOut_Points, "

			"COALESCE(AVG(al.Daily_Points), 0.00) "
			"AS Average_Daily_Points, "

			"COALESCE(SUM(al.Late_Pickup_Charge), 0.00) "
			"AS Total_Late_Pickup_Charges "

			"FROM ATTENDANCE_LOG al "

			"LEFT JOIN LOG_RECORD ci "
			"ON al.Log_ID = ci.Log_ID "
			"AND ci.Record_Type = 'CHECK_IN' "

			"LEFT JOIN LOG_RECORD co "
			"ON al.Log_ID = co.Log_ID "
			"AND co.Record_Type = 'CHECK_OUT' "

			"WHERE al.Child_ID = ? "
			"AND MONTH(al.Log_Date) = ? "
			"AND YEAR(al.Log_Date) = ?"
		);

		reportStatement->setString(1, childId);
		reportStatement->setInt(2, reportMonth);
		reportStatement->setInt(3, reportYear);

		reportResult =
			reportStatement->executeQuery();

		if (reportResult->next())
		{
			totalDaysPresent =
				reportResult->getInt(
					"Total_Days_Present"
				);

			completedRecords =
				reportResult->getInt(
					"Completed_Records"
				);

			pendingRecords =
				reportResult->getInt(
					"Pending_Records"
				);

			onTimeCheckIns =
				reportResult->getInt(
					"On_Time_CheckIns"
				);

			lateCheckIns =
				reportResult->getInt(
					"Late_CheckIns"
				);

			onTimePickups =
				reportResult->getInt(
					"On_Time_Pickups"
				);

			latePickups =
				reportResult->getInt(
					"Late_Pickups"
				);

			totalCheckInPoints =
				reportResult->getDouble(
					"Total_CheckIn_Points"
				);

			totalCheckOutPoints =
				reportResult->getDouble(
					"Total_CheckOut_Points"
				);

			totalDailyPoints =
				reportResult->getDouble(
					"Total_Daily_Points"
				);

			averageCheckInPoints =
				reportResult->getDouble(
					"Average_CheckIn_Points"
				);

			averageCheckOutPoints =
				reportResult->getDouble(
					"Average_CheckOut_Points"
				);

			averageDailyPoints =
				reportResult->getDouble(
					"Average_Daily_Points"
				);

			totalLatePickupCharges =
				reportResult->getDouble(
					"Total_Late_Pickup_Charges"
				);
		}

		// ==================================================
		// HANDLE PERIOD WITHOUT ATTENDANCE
		// ==================================================

		if (totalDaysPresent == 0)
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "               NO ATTENDANCE RECORDS FOUND                 \n";
			cout << "============================================================\n";
			cout << " Child ID     : " << childId << "\n";
			cout << " Child Name   : " << childName << "\n";
			cout << " Report Month : "
				<< monthNames[reportMonth - 1] << "\n";
			cout << " Report Year  : " << reportYear << "\n";
			cout << "\nNo attendance records were found "
				<< "for the selected child and period.\n";
			cout << "============================================================\n";

			delete reportResult;
			reportResult = nullptr;

			delete reportStatement;
			reportStatement = nullptr;

			delete childResult;
			childResult = nullptr;

			delete childStatement;
			childStatement = nullptr;

			delete operatingDayResult;
			operatingDayResult = nullptr;

			delete operatingDayStatement;
			operatingDayStatement = nullptr;

			return;
		}

		// ==================================================
		// CALCULATE PERFORMANCE
		// ==================================================

		checkInPerformance =
			averageCheckInPoints * 100.0;

		if (completedRecords > 0)
		{
			checkOutPerformance =
				averageCheckOutPoints * 100.0;

			overallPerformance =
				averageDailyPoints * 100.0;
		}

		// ==================================================
// CALCULATE ATTENDANCE PERCENTAGE
// ==================================================

		totalDaysAbsent =
			totalOperatingDays - totalDaysPresent;

		if (totalDaysAbsent < 0)
		{
			/*
			 * Defensive protection for inconsistent historical
			 * data or an incomplete operating calendar.
			 */
			totalDaysAbsent = 0;
		}

		if (totalOperatingDays > 0)
		{
			attendancePercentage =
				(
					static_cast<double>(
						totalDaysPresent
						) /
					static_cast<double>(
						totalOperatingDays
						)
					) * 100.0;
		}

		// ==================================================
		// DISPLAY MONTHLY SUMMARY
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "          INDIVIDUAL MONTHLY PERFORMANCE REPORT             \n";
		cout << "============================================================\n";
		cout << " Generated By                  : "
			<< adminName << "\n";
		cout << " Administrator Staff ID        : "
			<< staffId << "\n";
		cout << "------------------------------------------------------------\n";
		cout << " Child ID                      : "
			<< childId << "\n";
		cout << " Child Name                    : "
			<< childName << "\n";
		cout << " Report Month                  : "
			<< monthNames[reportMonth - 1] << "\n";
		cout << " Report Year                   : "
			<< reportYear << "\n";
		cout << "------------------------------------------------------------\n";
		cout << " Total Operating Days          : "
			<< totalOperatingDays << "\n";

		cout << " Total Days Present            : "
			<< totalDaysPresent << "\n";

		cout << " Total Days Absent             : "
			<< totalDaysAbsent << "\n";

		cout << " Attendance Percentage         : ";

		if (totalOperatingDays == 0)
		{
			cout << "Not Available\n";
		}
		else
		{
			cout << fixed
				<< setprecision(2)
				<< attendancePercentage
				<< "%\n";
		}

		cout << " Completed Attendance Records  : "
			<< completedRecords << "\n";

		cout << " Pending Check-Out Records     : "
			<< pendingRecords << "\n";
		cout << "------------------------------------------------------------\n";
		cout << " Check-In On Time              : "
			<< onTimeCheckIns << "\n";
		cout << " Check-In Late                 : "
			<< lateCheckIns << "\n";
		cout << " Pickup On Time                : "
			<< onTimePickups << "\n";
		cout << " Late Pickup                   : "
			<< latePickups << "\n";
		cout << "------------------------------------------------------------\n";
		cout << " Total Check-In Points         : "
			<< fixed << setprecision(2)
			<< totalCheckInPoints << "\n";
		cout << " Total Check-Out Points        : "
			<< fixed << setprecision(2)
			<< totalCheckOutPoints << "\n";
		cout << " Total Daily Points            : "
			<< fixed << setprecision(2)
			<< totalDailyPoints << "\n";
		cout << "------------------------------------------------------------\n";
		cout << " Average Check-In Points       : "
			<< fixed << setprecision(2)
			<< averageCheckInPoints << "\n";

		if (completedRecords > 0)
		{
			cout << " Average Check-Out Points      : "
				<< fixed << setprecision(2)
				<< averageCheckOutPoints << "\n";

			cout << " Average Daily Points          : "
				<< fixed << setprecision(2)
				<< averageDailyPoints << "\n";
		}
		else
		{
			cout << " Average Check-Out Points      : "
				<< "Not Available\n";

			cout << " Average Daily Points          : "
				<< "Not Available\n";
		}

		cout << "------------------------------------------------------------\n";
		cout << " Check-In Performance          : "
			<< fixed << setprecision(2)
			<< checkInPerformance << "%\n";

		if (completedRecords > 0)
		{
			cout << " Check-Out Performance         : "
				<< fixed << setprecision(2)
				<< checkOutPerformance << "%\n";

			cout << " Overall Performance           : "
				<< fixed << setprecision(2)
				<< overallPerformance << "%\n";
		}
		else
		{
			cout << " Check-Out Performance         : "
				<< "Not Available\n";

			cout << " Overall Performance           : "
				<< "Not Available\n";
		}

		cout << " Total Late Pickup Charges     : RM"
			<< fixed << setprecision(2)
			<< totalLatePickupCharges << "\n";
		cout << "============================================================\n";

		// ==================================================
		// DISPLAY PERFORMANCE INTERPRETATION
		// ==================================================

		cout << " Performance Interpretation    : ";

		if (completedRecords == 0)
		{
			cout << "NOT AVAILABLE\n";
		}
		else if (overallPerformance >= 90.0)
		{
			cout << "EXCELLENT\n";
		}
		else if (overallPerformance >= 75.0)
		{
			cout << "GOOD\n";
		}
		else if (overallPerformance >= 50.0)
		{
			cout << "SATISFACTORY\n";
		}
		else
		{
			cout << "NEEDS IMPROVEMENT\n";
		}

		cout << "============================================================\n";

		// ==================================================
		// RETRIEVE DETAILED MONTHLY RECORDS
		// ==================================================

		detailStatement = con->prepareStatement(
			"SELECT "
			"al.Log_ID, "
			"DATE_FORMAT(al.Log_Date, '%d/%m/%Y') "
			"AS Formatted_Date, "

			"DATE_FORMAT(ci.Record_Time, '%H:%i:%s') "
			"AS CheckIn_Time, "
			"ci.Status AS CheckIn_Status, "

			"DATE_FORMAT(co.Record_Time, '%H:%i:%s') "
			"AS CheckOut_Time, "
			"co.Status AS Pickup_Status, "

			"co.Parent_ID AS Checkout_Parent_ID, "
			"p.Name AS Parent_Name, "
			"pc.Relationship_Type AS Parent_Relationship, "

			"co.Rep_ID AS Checkout_Rep_ID, "
			"ar.Name AS Representative_Name, "
			"ac.Relationship "
			"AS Representative_Relationship, "

			"al.CheckIn_Points, "
			"al.CheckOut_Points, "
			"al.Daily_Points, "
			"al.Late_Pickup_Charge "

			"FROM ATTENDANCE_LOG al "

			"LEFT JOIN LOG_RECORD ci "
			"ON al.Log_ID = ci.Log_ID "
			"AND ci.Record_Type = 'CHECK_IN' "

			"LEFT JOIN LOG_RECORD co "
			"ON al.Log_ID = co.Log_ID "
			"AND co.Record_Type = 'CHECK_OUT' "

			"LEFT JOIN PARENT p "
			"ON co.Parent_ID = p.Parent_ID "

			"LEFT JOIN PARENT_CHILD pc "
			"ON co.Parent_ID = pc.Parent_ID "
			"AND al.Child_ID = pc.Child_ID "

			"LEFT JOIN AUTHORIZED_REPRESENTATIVE ar "
			"ON co.Rep_ID = ar.Rep_ID "

			"LEFT JOIN AUTHOREP_CHILD ac "
			"ON co.Rep_ID = ac.Rep_ID "
			"AND al.Child_ID = ac.Child_ID "

			"WHERE al.Child_ID = ? "
			"AND MONTH(al.Log_Date) = ? "
			"AND YEAR(al.Log_Date) = ? "

			"ORDER BY al.Log_Date ASC"
		);

		detailStatement->setString(1, childId);
		detailStatement->setInt(2, reportMonth);
		detailStatement->setInt(3, reportYear);

		detailResult =
			detailStatement->executeQuery();

		// ==================================================
		// DISPLAY DETAILED MONTHLY RECORDS
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "                DETAILED MONTHLY RECORDS                   \n";
		cout << "============================================================\n";

		int detailCount = 0;

		while (detailResult->next())
		{
			detailCount++;

			const bool checkoutCompleted =
				!detailResult->isNull(
					"CheckOut_Time"
				);

			cout << "\n";
			cout << "------------------------------------------------------------\n";
			cout << " Record Number       : "
				<< detailCount << "\n";
			cout << " Log ID              : "
				<< detailResult->getString(
					"Log_ID"
				)
				<< "\n";
			cout << " Attendance Date     : "
				<< detailResult->getString(
					"Formatted_Date"
				)
				<< "\n";

			// ==============================================
			// CHECK-IN INFORMATION
			// ==============================================

			cout << " Check-In Time       : ";

			if (detailResult->isNull("CheckIn_Time"))
			{
				cout << "Not Recorded\n";
			}
			else
			{
				cout << detailResult->getString(
					"CheckIn_Time"
				)
					<< "\n";
			}

			cout << " Check-In Status     : ";

			if (detailResult->isNull("CheckIn_Status"))
			{
				cout << "Not Recorded\n";
			}
			else
			{
				cout << detailResult->getString(
					"CheckIn_Status"
				)
					<< "\n";
			}

			cout << " Check-In Points     : ";

			if (detailResult->isNull("CheckIn_Points"))
			{
				cout << "Not Available\n";
			}
			else
			{
				cout << fixed << setprecision(2)
					<< detailResult->getDouble(
						"CheckIn_Points"
					)
					<< "\n";
			}

			// ==============================================
			// CHECK-OUT INFORMATION
			// ==============================================

			cout << " Check-Out Time      : ";

			if (!checkoutCompleted)
			{
				cout << "Pending\n";
			}
			else
			{
				cout << detailResult->getString(
					"CheckOut_Time"
				)
					<< "\n";
			}

			cout << " Pickup Status       : ";

			if (detailResult->isNull("Pickup_Status"))
			{
				cout << "PENDING\n";
			}
			else
			{
				cout << detailResult->getString(
					"Pickup_Status"
				)
					<< "\n";
			}

			cout << " Check-Out Points    : ";

			if (detailResult->isNull("CheckOut_Points"))
			{
				cout << "Pending\n";
			}
			else
			{
				cout << fixed << setprecision(2)
					<< detailResult->getDouble(
						"CheckOut_Points"
					)
					<< "\n";
			}

			// ==============================================
			// COLLECTOR INFORMATION
			// ==============================================

			if (!checkoutCompleted)
			{
				cout << " Collector Type      : "
					<< "Pending Check-Out\n";
				cout << " Collector ID        : -\n";
				cout << " Collector Name      : -\n";
				cout << " Relationship        : -\n";
			}
			else if (!detailResult->isNull(
				"Checkout_Parent_ID"
			))
			{
				cout << " Collector Type      : "
					<< "REGISTERED PARENT\n";

				cout << " Collector ID        : "
					<< detailResult->getString(
						"Checkout_Parent_ID"
					)
					<< "\n";

				cout << " Collector Name      : ";

				if (detailResult->isNull("Parent_Name"))
				{
					cout << "Not Available\n";
				}
				else
				{
					cout << detailResult->getString(
						"Parent_Name"
					)
						<< "\n";
				}

				cout << " Relationship        : ";

				if (detailResult->isNull(
					"Parent_Relationship"
				))
				{
					cout << "Not Available\n";
				}
				else
				{
					cout << detailResult->getString(
						"Parent_Relationship"
					)
						<< "\n";
				}
			}
			else if (!detailResult->isNull(
				"Checkout_Rep_ID"
			))
			{
				cout << " Collector Type      : "
					<< "AUTHORIZED REPRESENTATIVE\n";

				cout << " Collector ID        : "
					<< detailResult->getString(
						"Checkout_Rep_ID"
					)
					<< "\n";

				cout << " Collector Name      : ";

				if (detailResult->isNull(
					"Representative_Name"
				))
				{
					cout << "Not Available\n";
				}
				else
				{
					cout << detailResult->getString(
						"Representative_Name"
					)
						<< "\n";
				}

				cout << " Relationship        : ";

				if (detailResult->isNull(
					"Representative_Relationship"
				))
				{
					cout << "Not Available\n";
				}
				else
				{
					cout << detailResult->getString(
						"Representative_Relationship"
					)
						<< "\n";
				}
			}
			else
			{
				cout << " Collector Type      : LEGACY RECORD\n";
				cout << " Collector ID        : Not Available\n";
				cout << " Collector Name      : Not Available\n";
				cout << " Relationship        : Not Available\n";
			}

			// ==============================================
			// DAILY PERFORMANCE AND CHARGE
			// ==============================================

			cout << " Daily Points        : ";

			if (detailResult->isNull("Daily_Points"))
			{
				cout << "Pending\n";
			}
			else
			{
				cout << fixed << setprecision(2)
					<< detailResult->getDouble(
						"Daily_Points"
					)
					<< "\n";
			}

			cout << " Late Pickup Charge  : ";

			if (!checkoutCompleted)
			{
				cout << "Pending\n";
			}
			else
			{
				cout << "RM"
					<< fixed << setprecision(2)
					<< detailResult->getDouble(
						"Late_Pickup_Charge"
					)
					<< "\n";
			}
		}

		cout << "------------------------------------------------------------\n";
		cout << " Total Detailed Records : "
			<< detailCount << "\n";
		cout << "============================================================\n";

		// ==================================================
		// CLEANUP
		// ==================================================

		delete detailResult;
		detailResult = nullptr;

		delete detailStatement;
		detailStatement = nullptr;

		delete reportResult;
		reportResult = nullptr;

		delete reportStatement;
		reportStatement = nullptr;

		delete childResult;
		childResult = nullptr;

		delete childStatement;
		childStatement = nullptr;

		delete operatingDayResult;
		operatingDayResult = nullptr;

		delete operatingDayStatement;
		operatingDayStatement = nullptr;
	}
	catch (sql::SQLException& e)
	{
		delete detailResult;
		detailResult = nullptr;

		delete detailStatement;
		detailStatement = nullptr;

		delete reportResult;
		reportResult = nullptr;

		delete reportStatement;
		reportStatement = nullptr;

		delete childResult;
		childResult = nullptr;

		delete childStatement;
		childStatement = nullptr;

		delete operatingDayResult;
		operatingDayResult = nullptr;

		delete operatingDayStatement;
		operatingDayStatement = nullptr;

		cerr << "\nSQL Error while generating "
			<< "Monthly Analytics Report:\n";

		cerr << "Message    : "
			<< e.what() << "\n";

		cerr << "Error Code : "
			<< e.getErrorCode() << "\n";

		cerr << "SQL State  : "
			<< e.getSQLState() << "\n";
	}
}

// ==========================================================
// YEARLY CHILD PERFORMANCE SUMMARY
// ==========================================================

void yearlyChildPerformanceSummary(
	sql::Connection* con,
	const UserSession& session
)
{
	string childId;
	string childName;

	const string staffId = session.staffId;
	const string adminName = session.staffName;

	int reportYear = 0;

	sql::PreparedStatement* childStatement = nullptr;
	sql::PreparedStatement* yearlyStatement = nullptr;

	sql::ResultSet* childResult = nullptr;
	sql::ResultSet* yearlyResult = nullptr;

	const string monthNames[12] =
	{
		"JANUARY",
		"FEBRUARY",
		"MARCH",
		"APRIL",
		"MAY",
		"JUNE",
		"JULY",
		"AUGUST",
		"SEPTEMBER",
		"OCTOBER",
		"NOVEMBER",
		"DECEMBER"
	};

	cout << "\n";
	cout << "============================================================\n";
	cout << "          INDIVIDUAL YEARLY PERFORMANCE REPORT              \n";
	cout << "============================================================\n";
	cout << " Logged-In Administrator : "
		<< adminName
		<< " (" << staffId << ")\n";
	cout << "------------------------------------------------------------\n";

	// ======================================================
	// ENTER AND VALIDATE CHILD ID
	// ======================================================

	cout << " Enter Child ID    : ";
	getline(cin, childId);

	for (char& character : childId)
	{
		character = static_cast<char>(
			std::toupper(
				static_cast<unsigned char>(character)
			)
			);
	}

	if (!isValidReportChildId(childId))
	{
		cout << "\nERROR: Invalid Child ID format.\n";
		cout << "Expected format: C0001\n";
		return;
	}

	// ======================================================
	// ENTER AND VALIDATE REPORT YEAR
	// ======================================================

	cout << " Enter Report Year : ";

	if (!(cin >> reportYear))
	{
		cin.clear();
		clearInputBuffer();

		cout << "\nERROR: Please enter a valid year.\n";
		return;
	}

	clearInputBuffer();

	if (!isValidReportYear(reportYear))
	{
		cout << "\nERROR: Report year must be "
			<< "between 2020 and 2100.\n";

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

		childStatement->setString(
			1,
			childId
		);

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
				childResult->getString(
					"Name"
				).c_str()
			);

		// ==================================================
		// RETRIEVE MONTHLY AND YEARLY DATA
		// ==================================================

		yearlyStatement = con->prepareStatement(
			"SELECT "
			"months.Month_Number, "

			"COALESCE(operating.Total_Operating_Days, 0) "
			"AS Total_Operating_Days, "

			"COALESCE(attendance.Total_Days_Present, 0) "
			"AS Total_Days_Present, "

			"COALESCE(attendance.Completed_Records, 0) "
			"AS Completed_Records, "

			"COALESCE(attendance.Pending_Records, 0) "
			"AS Pending_Records, "

			"COALESCE(attendance.On_Time_CheckIns, 0) "
			"AS On_Time_CheckIns, "

			"COALESCE(attendance.Late_CheckIns, 0) "
			"AS Late_CheckIns, "

			"COALESCE(attendance.On_Time_Pickups, 0) "
			"AS On_Time_Pickups, "

			"COALESCE(attendance.Late_Pickups, 0) "
			"AS Late_Pickups, "

			"COALESCE(attendance.Total_CheckIn_Points, 0.00) "
			"AS Total_CheckIn_Points, "

			"COALESCE(attendance.Total_CheckOut_Points, 0.00) "
			"AS Total_CheckOut_Points, "

			"COALESCE(attendance.Total_Daily_Points, 0.00) "
			"AS Total_Daily_Points, "

			"COALESCE(attendance.Average_CheckIn_Points, 0.00) "
			"AS Average_CheckIn_Points, "

			"COALESCE(attendance.Average_CheckOut_Points, 0.00) "
			"AS Average_CheckOut_Points, "

			"COALESCE(attendance.Average_Daily_Points, 0.00) "
			"AS Average_Daily_Points, "

			"COALESCE(attendance.Total_Late_Charges, 0.00) "
			"AS Total_Late_Charges "

			"FROM "
			"( "
			"SELECT 1 AS Month_Number "
			"UNION ALL SELECT 2 "
			"UNION ALL SELECT 3 "
			"UNION ALL SELECT 4 "
			"UNION ALL SELECT 5 "
			"UNION ALL SELECT 6 "
			"UNION ALL SELECT 7 "
			"UNION ALL SELECT 8 "
			"UNION ALL SELECT 9 "
			"UNION ALL SELECT 10 "
			"UNION ALL SELECT 11 "
			"UNION ALL SELECT 12 "
			") months "

			"LEFT JOIN "
			"( "
			"SELECT "
			"MONTH(Operating_Date) AS Month_Number, "
			"COUNT(*) AS Total_Operating_Days "
			"FROM OPERATING_DAY "
			"WHERE YEAR(Operating_Date) = ? "
			"AND Day_Status = 'OPEN' "
			"GROUP BY MONTH(Operating_Date) "
			") operating "
			"ON months.Month_Number = operating.Month_Number "

			"LEFT JOIN "
			"( "
			"SELECT "
			"MONTH(al.Log_Date) AS Month_Number, "

			"COUNT(*) AS Total_Days_Present, "

			"SUM(CASE "
			"WHEN al.CheckOut_Points IS NOT NULL "
			"THEN 1 ELSE 0 END) AS Completed_Records, "

			"SUM(CASE "
			"WHEN al.CheckOut_Points IS NULL "
			"THEN 1 ELSE 0 END) AS Pending_Records, "

			"SUM(CASE "
			"WHEN ci.Status = 'ON_TIME' "
			"THEN 1 ELSE 0 END) AS On_Time_CheckIns, "

			"SUM(CASE "
			"WHEN ci.Status = 'LATE' "
			"THEN 1 ELSE 0 END) AS Late_CheckIns, "

			"SUM(CASE "
			"WHEN co.Status = 'ON_TIME' "
			"THEN 1 ELSE 0 END) AS On_Time_Pickups, "

			"SUM(CASE "
			"WHEN co.Status = 'LATE_PICKUP' "
			"THEN 1 ELSE 0 END) AS Late_Pickups, "

			"COALESCE(SUM(al.CheckIn_Points), 0.00) "
			"AS Total_CheckIn_Points, "

			"COALESCE(SUM(al.CheckOut_Points), 0.00) "
			"AS Total_CheckOut_Points, "

			"COALESCE(SUM(al.Daily_Points), 0.00) "
			"AS Total_Daily_Points, "

			"COALESCE(AVG(al.CheckIn_Points), 0.00) "
			"AS Average_CheckIn_Points, "

			"COALESCE(AVG(al.CheckOut_Points), 0.00) "
			"AS Average_CheckOut_Points, "

			"COALESCE(AVG(al.Daily_Points), 0.00) "
			"AS Average_Daily_Points, "

			"COALESCE(SUM(al.Late_Pickup_Charge), 0.00) "
			"AS Total_Late_Charges "

			"FROM ATTENDANCE_LOG al "

			"LEFT JOIN LOG_RECORD ci "
			"ON al.Log_ID = ci.Log_ID "
			"AND ci.Record_Type = 'CHECK_IN' "

			"LEFT JOIN LOG_RECORD co "
			"ON al.Log_ID = co.Log_ID "
			"AND co.Record_Type = 'CHECK_OUT' "

			"WHERE al.Child_ID = ? "
			"AND YEAR(al.Log_Date) = ? "

			"GROUP BY MONTH(al.Log_Date) "
			") attendance "
			"ON months.Month_Number = attendance.Month_Number "

			"ORDER BY months.Month_Number"
		);

		/*
		 * Parameter order:
		 * 1 = Operating calendar year
		 * 2 = Child ID
		 * 3 = Attendance year
		 */
		yearlyStatement->setInt(
			1,
			reportYear
		);

		yearlyStatement->setString(
			2,
			childId
		);

		yearlyStatement->setInt(
			3,
			reportYear
		);

		yearlyResult =
			yearlyStatement->executeQuery();

		// ==================================================
		// DISPLAY REPORT INFORMATION
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "          INDIVIDUAL YEARLY PERFORMANCE REPORT              \n";
		cout << "============================================================\n";
		cout << " Generated By           : "
			<< adminName << "\n";
		cout << " Administrator Staff ID : "
			<< staffId << "\n";
		cout << "------------------------------------------------------------\n";
		cout << " Child ID               : "
			<< childId << "\n";
		cout << " Child Name             : "
			<< childName << "\n";
		cout << " Report Year            : "
			<< reportYear << "\n";
		cout << "============================================================\n";

		// ==================================================
		// DISPLAY MONTHLY TABLE HEADER
		// ==================================================

		cout << "\n";
		cout << "MONTHLY PERFORMANCE OVERVIEW\n\n";

		cout << left
			<< setw(11) << "Month"
			<< setw(8) << "Open"
			<< setw(9) << "Present"
			<< setw(8) << "Absent"
			<< setw(11) << "Attend %"
			<< setw(10) << "In Perf"
			<< setw(11) << "Out Perf"
			<< setw(12) << "Overall"
			<< setw(9) << "Late"
			<< setw(12) << "Charges"
			<< "\n";

		cout << string(101, '=') << "\n";

		// ==================================================
		// YEARLY TOTAL VARIABLES
		// ==================================================

		int yearlyOperatingDays = 0;
		int yearlyDaysPresent = 0;
		int yearlyDaysAbsent = 0;

		int yearlyCompletedRecords = 0;
		int yearlyPendingRecords = 0;

		int yearlyOnTimeCheckIns = 0;
		int yearlyLateCheckIns = 0;
		int yearlyOnTimePickups = 0;
		int yearlyLatePickups = 0;

		double yearlyCheckInPoints = 0.0;
		double yearlyCheckOutPoints = 0.0;
		double yearlyDailyPoints = 0.0;
		double yearlyLateCharges = 0.0;

		int monthsWithAttendance = 0;
		int monthsWithOperatingCalendar = 0;

		// ==================================================
		// DISPLAY MONTHLY ROWS
		// ==================================================

		while (yearlyResult->next())
		{
			const int monthNumber =
				yearlyResult->getInt(
					"Month_Number"
				);

			const int operatingDays =
				yearlyResult->getInt(
					"Total_Operating_Days"
				);

			const int daysPresent =
				yearlyResult->getInt(
					"Total_Days_Present"
				);

			const int completedRecords =
				yearlyResult->getInt(
					"Completed_Records"
				);

			const int pendingRecords =
				yearlyResult->getInt(
					"Pending_Records"
				);

			const int onTimeCheckIns =
				yearlyResult->getInt(
					"On_Time_CheckIns"
				);

			const int lateCheckIns =
				yearlyResult->getInt(
					"Late_CheckIns"
				);

			const int onTimePickups =
				yearlyResult->getInt(
					"On_Time_Pickups"
				);

			const int latePickups =
				yearlyResult->getInt(
					"Late_Pickups"
				);

			const double totalCheckInPoints =
				yearlyResult->getDouble(
					"Total_CheckIn_Points"
				);

			const double totalCheckOutPoints =
				yearlyResult->getDouble(
					"Total_CheckOut_Points"
				);

			const double totalDailyPoints =
				yearlyResult->getDouble(
					"Total_Daily_Points"
				);

			const double averageCheckInPoints =
				yearlyResult->getDouble(
					"Average_CheckIn_Points"
				);

			const double averageCheckOutPoints =
				yearlyResult->getDouble(
					"Average_CheckOut_Points"
				);

			const double averageDailyPoints =
				yearlyResult->getDouble(
					"Average_Daily_Points"
				);

			const double monthlyLateCharges =
				yearlyResult->getDouble(
					"Total_Late_Charges"
				);

			int daysAbsent =
				operatingDays - daysPresent;

			if (daysAbsent < 0)
			{
				daysAbsent = 0;
			}

			double attendancePercentage = 0.0;

			if (operatingDays > 0)
			{
				attendancePercentage =
					(
						static_cast<double>(
							daysPresent
							) /
						static_cast<double>(
							operatingDays
							)
						) * 100.0;

				monthsWithOperatingCalendar++;
			}

			double checkInPerformance = 0.0;
			double checkOutPerformance = 0.0;
			double overallPerformance = 0.0;

			if (daysPresent > 0)
			{
				checkInPerformance =
					averageCheckInPoints * 100.0;

				monthsWithAttendance++;
			}

			if (completedRecords > 0)
			{
				checkOutPerformance =
					averageCheckOutPoints * 100.0;

				overallPerformance =
					averageDailyPoints * 100.0;
			}

			string attendanceText = "N/A";
			string checkInText = "N/A";
			string checkOutText = "N/A";
			string overallText = "N/A";
			string chargeText;

			if (operatingDays > 0)
			{
				ostringstream attendanceStream;

				attendanceStream
					<< fixed
					<< setprecision(2)
					<< attendancePercentage
					<< "%";

				attendanceText =
					attendanceStream.str();
			}

			if (daysPresent > 0)
			{
				ostringstream checkInStream;

				checkInStream
					<< fixed
					<< setprecision(2)
					<< checkInPerformance
					<< "%";

				checkInText =
					checkInStream.str();
			}

			if (completedRecords > 0)
			{
				ostringstream checkOutStream;
				ostringstream overallStream;

				checkOutStream
					<< fixed
					<< setprecision(2)
					<< checkOutPerformance
					<< "%";

				overallStream
					<< fixed
					<< setprecision(2)
					<< overallPerformance
					<< "%";

				checkOutText =
					checkOutStream.str();

				overallText =
					overallStream.str();
			}

			ostringstream chargeStream;

			chargeStream
				<< "RM"
				<< fixed
				<< setprecision(2)
				<< monthlyLateCharges;

			chargeText =
				chargeStream.str();

			cout << left
				<< setw(11)
				<< monthNames[monthNumber - 1]
				<< setw(8)
				<< operatingDays
				<< setw(9)
				<< daysPresent
				<< setw(8)
				<< daysAbsent
				<< setw(11)
				<< attendanceText
				<< setw(10)
				<< checkInText
				<< setw(11)
				<< checkOutText
				<< setw(12)
				<< overallText
				<< setw(9)
				<< latePickups
				<< setw(12)
				<< chargeText
				<< "\n";

			// ==============================================
			// ACCUMULATE YEARLY TOTALS
			// ==============================================

			yearlyOperatingDays +=
				operatingDays;

			yearlyDaysPresent +=
				daysPresent;

			yearlyDaysAbsent +=
				daysAbsent;

			yearlyCompletedRecords +=
				completedRecords;

			yearlyPendingRecords +=
				pendingRecords;

			yearlyOnTimeCheckIns +=
				onTimeCheckIns;

			yearlyLateCheckIns +=
				lateCheckIns;

			yearlyOnTimePickups +=
				onTimePickups;

			yearlyLatePickups +=
				latePickups;

			yearlyCheckInPoints +=
				totalCheckInPoints;

			yearlyCheckOutPoints +=
				totalCheckOutPoints;

			yearlyDailyPoints +=
				totalDailyPoints;

			yearlyLateCharges +=
				monthlyLateCharges;
		}

		cout << string(101, '=') << "\n";

		// ==================================================
		// CALCULATE YEARLY PERFORMANCE
		// ==================================================

		double yearlyAttendancePercentage = 0.0;
		double yearlyAverageCheckInPoints = 0.0;
		double yearlyAverageCheckOutPoints = 0.0;
		double yearlyAverageDailyPoints = 0.0;

		double yearlyCheckInPerformance = 0.0;
		double yearlyCheckOutPerformance = 0.0;
		double yearlyOverallPerformance = 0.0;

		if (yearlyOperatingDays > 0)
		{
			yearlyAttendancePercentage =
				(
					static_cast<double>(
						yearlyDaysPresent
						) /
					static_cast<double>(
						yearlyOperatingDays
						)
					) * 100.0;
		}

		if (yearlyDaysPresent > 0)
		{
			yearlyAverageCheckInPoints =
				yearlyCheckInPoints /
				static_cast<double>(
					yearlyDaysPresent
					);

			yearlyCheckInPerformance =
				yearlyAverageCheckInPoints * 100.0;
		}

		if (yearlyCompletedRecords > 0)
		{
			yearlyAverageCheckOutPoints =
				yearlyCheckOutPoints /
				static_cast<double>(
					yearlyCompletedRecords
					);

			yearlyAverageDailyPoints =
				yearlyDailyPoints /
				static_cast<double>(
					yearlyCompletedRecords
					);

			yearlyCheckOutPerformance =
				yearlyAverageCheckOutPoints * 100.0;

			yearlyOverallPerformance =
				yearlyAverageDailyPoints * 100.0;
		}

		// ==================================================
		// DISPLAY YEARLY SUMMARY
		// ==================================================

		cout << "\n";
		cout << "============================================================\n";
		cout << "                    YEARLY SUMMARY                         \n";
		cout << "============================================================\n";
		cout << " Total Operating Days          : "
			<< yearlyOperatingDays << "\n";
		cout << " Total Days Present            : "
			<< yearlyDaysPresent << "\n";
		cout << " Total Days Absent             : "
			<< yearlyDaysAbsent << "\n";

		cout << " Attendance Percentage         : ";

		if (yearlyOperatingDays == 0)
		{
			cout << "Not Available\n";
		}
		else
		{
			cout << fixed
				<< setprecision(2)
				<< yearlyAttendancePercentage
				<< "%\n";
		}

		cout << "------------------------------------------------------------\n";
		cout << " Completed Attendance Records  : "
			<< yearlyCompletedRecords << "\n";
		cout << " Pending Check-Out Records     : "
			<< yearlyPendingRecords << "\n";
		cout << " Check-In On Time              : "
			<< yearlyOnTimeCheckIns << "\n";
		cout << " Check-In Late                 : "
			<< yearlyLateCheckIns << "\n";
		cout << " Pickup On Time                : "
			<< yearlyOnTimePickups << "\n";
		cout << " Late Pickup                   : "
			<< yearlyLatePickups << "\n";
		cout << "------------------------------------------------------------\n";
		cout << " Total Check-In Points         : "
			<< fixed
			<< setprecision(2)
			<< yearlyCheckInPoints << "\n";
		cout << " Total Check-Out Points        : "
			<< fixed
			<< setprecision(2)
			<< yearlyCheckOutPoints << "\n";
		cout << " Total Daily Points            : "
			<< fixed
			<< setprecision(2)
			<< yearlyDailyPoints << "\n";
		cout << "------------------------------------------------------------\n";

		cout << " Average Check-In Points       : ";

		if (yearlyDaysPresent == 0)
		{
			cout << "Not Available\n";
		}
		else
		{
			cout << fixed
				<< setprecision(2)
				<< yearlyAverageCheckInPoints
				<< "\n";
		}

		cout << " Average Check-Out Points      : ";

		if (yearlyCompletedRecords == 0)
		{
			cout << "Not Available\n";
		}
		else
		{
			cout << fixed
				<< setprecision(2)
				<< yearlyAverageCheckOutPoints
				<< "\n";
		}

		cout << " Average Daily Points          : ";

		if (yearlyCompletedRecords == 0)
		{
			cout << "Not Available\n";
		}
		else
		{
			cout << fixed
				<< setprecision(2)
				<< yearlyAverageDailyPoints
				<< "\n";
		}

		cout << "------------------------------------------------------------\n";
		cout << " Check-In Performance          : ";

		if (yearlyDaysPresent == 0)
		{
			cout << "Not Available\n";
		}
		else
		{
			cout << fixed
				<< setprecision(2)
				<< yearlyCheckInPerformance
				<< "%\n";
		}

		cout << " Check-Out Performance         : ";

		if (yearlyCompletedRecords == 0)
		{
			cout << "Not Available\n";
		}
		else
		{
			cout << fixed
				<< setprecision(2)
				<< yearlyCheckOutPerformance
				<< "%\n";
		}

		cout << " Overall Performance           : ";

		if (yearlyCompletedRecords == 0)
		{
			cout << "Not Available\n";
		}
		else
		{
			cout << fixed
				<< setprecision(2)
				<< yearlyOverallPerformance
				<< "%\n";
		}

		cout << " Total Late Pickup Charges     : RM"
			<< fixed
			<< setprecision(2)
			<< yearlyLateCharges
			<< "\n";

		cout << "============================================================\n";

		// ==================================================
		// YEARLY PERFORMANCE INTERPRETATION
		// ==================================================

		cout << " Performance Interpretation    : ";

		if (yearlyCompletedRecords == 0)
		{
			cout << "NOT AVAILABLE\n";
		}
		else if (yearlyOverallPerformance >= 90.0)
		{
			cout << "EXCELLENT\n";
		}
		else if (yearlyOverallPerformance >= 75.0)
		{
			cout << "GOOD\n";
		}
		else if (yearlyOverallPerformance >= 50.0)
		{
			cout << "SATISFACTORY\n";
		}
		else
		{
			cout << "NEEDS IMPROVEMENT\n";
		}

		cout << "============================================================\n";

		// ==================================================
		// CALENDAR COMPLETENESS NOTICE
		// ==================================================

		if (monthsWithOperatingCalendar == 0)
		{
			cout << "\nNOTICE: No operating calendar was found "
				<< "for this year.\n";
			cout << "Attendance percentages are not available.\n";
		}
		else if (monthsWithOperatingCalendar < 12)
		{
			cout << "\nNOTICE: Operating calendars are available "
				<< "for "
				<< monthsWithOperatingCalendar
				<< " month(s) only.\n";

			cout << "Generate the remaining monthly calendars "
				<< "if a full-year attendance percentage "
				<< "is required.\n";
		}

		if (monthsWithAttendance == 0)
		{
			cout << "\nNo attendance records were found "
				<< "for this child in "
				<< reportYear << ".\n";
		}

		// ==================================================
		// CLEANUP
		// ==================================================

		delete yearlyResult;
		yearlyResult = nullptr;

		delete yearlyStatement;
		yearlyStatement = nullptr;

		delete childResult;
		childResult = nullptr;

		delete childStatement;
		childStatement = nullptr;
	}
	catch (sql::SQLException& e)
	{
		delete yearlyResult;
		yearlyResult = nullptr;

		delete yearlyStatement;
		yearlyStatement = nullptr;

		delete childResult;
		childResult = nullptr;

		delete childStatement;
		childStatement = nullptr;

		cerr << "\nSQL Error while generating "
			<< "Yearly Child Performance Summary:\n";

		cerr << "Message    : "
			<< e.what() << "\n";

		cerr << "Error Code : "
			<< e.getErrorCode() << "\n";

		cerr << "SQL State  : "
			<< e.getSQLState() << "\n";
	}
}

// ==========================================================
// REPORT STAFF ID VALIDATION
// ==========================================================

bool isValidReportStaffId(
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
// REPORT CHILD ID VALIDATION
// ==========================================================

bool isValidReportChildId(
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
// REPORT YEAR VALIDATION
// ==========================================================

bool isValidReportYear(
	int year
)
{
	return year >= 2020 &&
		year <= 2100;
}
#include "YearlyReportPdf.h"

#include "InputUtils.h"
#include "PdfExporter.h"
#include "Report.h"

#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>

#include <cctype>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

struct YearlyPdfMonthRecord
{
	int monthNumber = 0;
	int operatingDays = 0;
	int daysPresent = 0;
	int daysAbsent = 0;
	int completedRecords = 0;
	int pendingRecords = 0;
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
	double attendancePercentage = 0.0;
	double checkInPerformance = 0.0;
	double checkOutPerformance = 0.0;
	double overallPerformance = 0.0;
	double totalLateCharges = 0.0;
};

static string formatYearlyNumber(double value)
{
	ostringstream stream;
	stream << fixed << setprecision(2) << value;
	return stream.str();
}

static string yearlyPerformanceInterpretation(double performance)
{
	if (performance >= 90.0)
	{
		return "EXCELLENT";
	}

	if (performance >= 75.0)
	{
		return "GOOD";
	}

	if (performance >= 50.0)
	{
		return "SATISFACTORY";
	}

	return "NEEDS IMPROVEMENT";
}

void exportYearlyChildReportPdf(
	sql::Connection* con,
	const UserSession& session
)
{
	string childId;
	string childName;
	int reportYear = 0;

	sql::PreparedStatement* childStatement = nullptr;
	sql::PreparedStatement* yearlyStatement = nullptr;

	sql::ResultSet* childResult = nullptr;
	sql::ResultSet* yearlyResult = nullptr;

	const string monthNames[12] =
	{
		"JANUARY", "FEBRUARY", "MARCH", "APRIL",
		"MAY", "JUNE", "JULY", "AUGUST",
		"SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
	};

	cout << "\n";
	cout << "============================================================\n";
	cout << "           EXPORT INDIVIDUAL YEARLY REPORT PDF               \n";
	cout << "============================================================\n";
	cout << " Logged-In Administrator : "
		<< session.staffName
		<< " (" << session.staffId << ")\n";
	cout << "------------------------------------------------------------\n";

	cout << " Enter Child ID    : ";
	getline(cin, childId);

	for (char& character : childId)
	{
		character = static_cast<char>(
			std::toupper(static_cast<unsigned char>(character))
			);
	}

	if (!isValidReportChildId(childId))
	{
		cout << "\nERROR: Invalid Child ID format.\n";
		cout << "Expected format: C0001\n";
		return;
	}

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
		cout << "\nERROR: Report year must be between 2020 and 2100.\n";
		return;
	}

	try
	{
		childStatement = con->prepareStatement(
			"SELECT Name FROM CHILD WHERE Child_ID = ?"
		);
		childStatement->setString(1, childId);
		childResult = childStatement->executeQuery();

		if (!childResult->next())
		{
			cout << "\nERROR: Child record was not found.\n";
			delete childResult;
			delete childStatement;
			return;
		}

		childName = string(childResult->getString("Name").c_str());

		yearlyStatement = con->prepareStatement(
			"SELECT "
			"months.Month_Number, "
			"COALESCE(op.Total_Operating_Days, 0) AS Total_Operating_Days, "
			"COALESCE(a.Total_Days_Present, 0) AS Total_Days_Present, "
			"COALESCE(a.Completed_Records, 0) AS Completed_Records, "
			"COALESCE(a.Pending_Records, 0) AS Pending_Records, "
			"COALESCE(a.On_Time_CheckIns, 0) AS On_Time_CheckIns, "
			"COALESCE(a.Late_CheckIns, 0) AS Late_CheckIns, "
			"COALESCE(a.On_Time_Pickups, 0) AS On_Time_Pickups, "
			"COALESCE(a.Late_Pickups, 0) AS Late_Pickups, "
			"COALESCE(a.Total_CheckIn_Points, 0.00) AS Total_CheckIn_Points, "
			"COALESCE(a.Total_CheckOut_Points, 0.00) AS Total_CheckOut_Points, "
			"COALESCE(a.Total_Daily_Points, 0.00) AS Total_Daily_Points, "
			"COALESCE(a.Average_CheckIn_Points, 0.00) AS Average_CheckIn_Points, "
			"COALESCE(a.Average_CheckOut_Points, 0.00) AS Average_CheckOut_Points, "
			"COALESCE(a.Average_Daily_Points, 0.00) AS Average_Daily_Points, "
			"COALESCE(a.Total_Late_Charges, 0.00) AS Total_Late_Charges "
			"FROM ("
			"SELECT 1 AS Month_Number UNION ALL SELECT 2 UNION ALL SELECT 3 "
			"UNION ALL SELECT 4 UNION ALL SELECT 5 UNION ALL SELECT 6 "
			"UNION ALL SELECT 7 UNION ALL SELECT 8 UNION ALL SELECT 9 "
			"UNION ALL SELECT 10 UNION ALL SELECT 11 UNION ALL SELECT 12"
			") months "
			"LEFT JOIN ("
			"SELECT MONTH(Operating_Date) AS Month_Number, COUNT(*) AS Total_Operating_Days "
			"FROM OPERATING_DAY WHERE YEAR(Operating_Date) = ? AND Day_Status = 'OPEN' "
			"GROUP BY MONTH(Operating_Date)"
			") op ON months.Month_Number = op.Month_Number "
			"LEFT JOIN ("
			"SELECT MONTH(al.Log_Date) AS Month_Number, COUNT(*) AS Total_Days_Present, "
			"SUM(CASE WHEN al.CheckOut_Points IS NOT NULL THEN 1 ELSE 0 END) AS Completed_Records, "
			"SUM(CASE WHEN al.CheckOut_Points IS NULL THEN 1 ELSE 0 END) AS Pending_Records, "
			"SUM(CASE WHEN ci.Status = 'ON_TIME' THEN 1 ELSE 0 END) AS On_Time_CheckIns, "
			"SUM(CASE WHEN ci.Status = 'LATE' THEN 1 ELSE 0 END) AS Late_CheckIns, "
			"SUM(CASE WHEN co.Status = 'ON_TIME' THEN 1 ELSE 0 END) AS On_Time_Pickups, "
			"SUM(CASE WHEN co.Status = 'LATE_PICKUP' THEN 1 ELSE 0 END) AS Late_Pickups, "
			"COALESCE(SUM(al.CheckIn_Points), 0.00) AS Total_CheckIn_Points, "
			"COALESCE(SUM(al.CheckOut_Points), 0.00) AS Total_CheckOut_Points, "
			"COALESCE(SUM(al.Daily_Points), 0.00) AS Total_Daily_Points, "
			"COALESCE(AVG(al.CheckIn_Points), 0.00) AS Average_CheckIn_Points, "
			"COALESCE(AVG(al.CheckOut_Points), 0.00) AS Average_CheckOut_Points, "
			"COALESCE(AVG(al.Daily_Points), 0.00) AS Average_Daily_Points, "
			"COALESCE(SUM(al.Late_Pickup_Charge), 0.00) AS Total_Late_Charges "
			"FROM ATTENDANCE_LOG al "
			"LEFT JOIN LOG_RECORD ci ON al.Log_ID = ci.Log_ID AND ci.Record_Type = 'CHECK_IN' "
			"LEFT JOIN LOG_RECORD co ON al.Log_ID = co.Log_ID AND co.Record_Type = 'CHECK_OUT' "
			"WHERE al.Child_ID = ? AND YEAR(al.Log_Date) = ? "
			"GROUP BY MONTH(al.Log_Date)"
			") a ON months.Month_Number = a.Month_Number "
			"ORDER BY months.Month_Number"
		);

		yearlyStatement->setInt(1, reportYear);
		yearlyStatement->setString(2, childId);
		yearlyStatement->setInt(3, reportYear);
		yearlyResult = yearlyStatement->executeQuery();

		vector<YearlyPdfMonthRecord> months;

		int yearlyOperatingDays = 0;
		int yearlyDaysPresent = 0;
		int yearlyDaysAbsent = 0;
		int yearlyCompletedRecords = 0;
		int yearlyPendingRecords = 0;
		int yearlyOnTimeCheckIns = 0;
		int yearlyLateCheckIns = 0;
		int yearlyOnTimePickups = 0;
		int yearlyLatePickups = 0;
		int monthsWithOperatingCalendar = 0;

		double yearlyCheckInPoints = 0.0;
		double yearlyCheckOutPoints = 0.0;
		double yearlyDailyPoints = 0.0;
		double yearlyLateCharges = 0.0;

		while (yearlyResult->next())
		{
			YearlyPdfMonthRecord record;

			record.monthNumber = yearlyResult->getInt("Month_Number");
			record.operatingDays = yearlyResult->getInt("Total_Operating_Days");
			record.daysPresent = yearlyResult->getInt("Total_Days_Present");
			record.completedRecords = yearlyResult->getInt("Completed_Records");
			record.pendingRecords = yearlyResult->getInt("Pending_Records");
			record.onTimeCheckIns = yearlyResult->getInt("On_Time_CheckIns");
			record.lateCheckIns = yearlyResult->getInt("Late_CheckIns");
			record.onTimePickups = yearlyResult->getInt("On_Time_Pickups");
			record.latePickups = yearlyResult->getInt("Late_Pickups");
			record.totalCheckInPoints = yearlyResult->getDouble("Total_CheckIn_Points");
			record.totalCheckOutPoints = yearlyResult->getDouble("Total_CheckOut_Points");
			record.totalDailyPoints = yearlyResult->getDouble("Total_Daily_Points");
			record.averageCheckInPoints = yearlyResult->getDouble("Average_CheckIn_Points");
			record.averageCheckOutPoints = yearlyResult->getDouble("Average_CheckOut_Points");
			record.averageDailyPoints = yearlyResult->getDouble("Average_Daily_Points");
			record.totalLateCharges = yearlyResult->getDouble("Total_Late_Charges");

			record.daysAbsent = record.operatingDays - record.daysPresent;
			if (record.daysAbsent < 0)
			{
				record.daysAbsent = 0;
			}

			if (record.operatingDays > 0)
			{
				record.attendancePercentage =
					static_cast<double>(record.daysPresent) /
					static_cast<double>(record.operatingDays) * 100.0;
				monthsWithOperatingCalendar++;
			}

			if (record.daysPresent > 0)
			{
				record.checkInPerformance = record.averageCheckInPoints * 100.0;
			}

			if (record.completedRecords > 0)
			{
				record.checkOutPerformance = record.averageCheckOutPoints * 100.0;
				record.overallPerformance = record.averageDailyPoints * 100.0;
			}

			yearlyOperatingDays += record.operatingDays;
			yearlyDaysPresent += record.daysPresent;
			yearlyDaysAbsent += record.daysAbsent;
			yearlyCompletedRecords += record.completedRecords;
			yearlyPendingRecords += record.pendingRecords;
			yearlyOnTimeCheckIns += record.onTimeCheckIns;
			yearlyLateCheckIns += record.lateCheckIns;
			yearlyOnTimePickups += record.onTimePickups;
			yearlyLatePickups += record.latePickups;
			yearlyCheckInPoints += record.totalCheckInPoints;
			yearlyCheckOutPoints += record.totalCheckOutPoints;
			yearlyDailyPoints += record.totalDailyPoints;
			yearlyLateCharges += record.totalLateCharges;

			months.push_back(record);
		}

		if (yearlyDaysPresent == 0)
		{
			cout << "\nNo attendance records were found for "
				<< childId << " in " << reportYear << ".\n";

			delete yearlyResult;
			delete yearlyStatement;
			delete childResult;
			delete childStatement;
			return;
		}

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
				static_cast<double>(yearlyDaysPresent) /
				static_cast<double>(yearlyOperatingDays) * 100.0;
		}

		if (yearlyDaysPresent > 0)
		{
			yearlyAverageCheckInPoints =
				yearlyCheckInPoints / static_cast<double>(yearlyDaysPresent);
			yearlyCheckInPerformance = yearlyAverageCheckInPoints * 100.0;
		}

		if (yearlyCompletedRecords > 0)
		{
			yearlyAverageCheckOutPoints =
				yearlyCheckOutPoints / static_cast<double>(yearlyCompletedRecords);
			yearlyAverageDailyPoints =
				yearlyDailyPoints / static_cast<double>(yearlyCompletedRecords);
			yearlyCheckOutPerformance = yearlyAverageCheckOutPoints * 100.0;
			yearlyOverallPerformance = yearlyAverageDailyPoints * 100.0;
		}

		const string outputFolder = "CAPMS_Reports";
		fs::create_directories(outputFolder);

		const string baseName =
			"Yearly_Performance_" + childId + "_" + to_string(reportYear);
		const string htmlPath = outputFolder + "\\" + baseName + ".html";
		const string pdfPath = outputFolder + "\\" + baseName + ".pdf";

		ofstream htmlFile(htmlPath);

		if (!htmlFile.is_open())
		{
			throw runtime_error("Unable to create the Yearly Report HTML file.");
		}

		htmlFile << R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<style>
@page { size: A4 landscape; margin: 12mm; }
* { box-sizing: border-box; }
body { margin: 0; font-family: Arial, Helvetica, sans-serif; color: #1f2937; font-size: 9pt; line-height: 1.35; }
.header { border-bottom: 3px solid #2563eb; padding-bottom: 10px; margin-bottom: 16px; }
h1 { margin: 0; color: #1e3a8a; font-size: 20pt; }
.subtitle { margin-top: 4px; color: #4b5563; }
h2 { color: #1e3a8a; font-size: 13pt; margin: 18px 0 8px; }
table { width: 100%; border-collapse: collapse; }
.meta td { border: none; padding: 4px 8px 4px 0; }
.label { width: 145px; font-weight: bold; color: #374151; }
.months th { background: #1e3a8a; color: white; border: 1px solid #cbd5e1; padding: 7px 4px; font-size: 8pt; text-align: center; }
.months td { border: 1px solid #cbd5e1; padding: 6px 4px; text-align: center; vertical-align: middle; }
.months tbody tr:nth-child(even) { background: #f8fafc; }
.summary td { border: 1px solid #d1d5db; padding: 7px 9px; }
.summary .key { background: #eff6ff; font-weight: bold; }
.good { color: #166534; font-weight: bold; }
.warning { color: #92400e; font-weight: bold; }
.charge { color: #b91c1c; font-weight: bold; }
.notice { margin-top: 14px; padding: 10px 12px; border-left: 4px solid #d97706; background: #fffbeb; color: #78350f; }
.footer { margin-top: 20px; padding-top: 8px; border-top: 1px solid #cbd5e1; color: #64748b; font-size: 8pt; }
thead { display: table-header-group; }
tr { page-break-inside: avoid; }
</style>
</head>
<body>
<div class="header">
<h1>Individual Yearly Performance Report</h1>
<div class="subtitle">Childcare Attendance and Pickup Management System</div>
</div>
)HTML";

		htmlFile << "<table class=\"meta\">";
		htmlFile << "<tr><td class=\"label\">Child</td><td>: "
			<< escapeHtml(childId) << " - " << escapeHtml(childName) << "</td>";
		htmlFile << "<td class=\"label\">Generated By</td><td>: "
			<< escapeHtml(session.staffName) << " (" << escapeHtml(session.staffId) << ")</td></tr>";
		htmlFile << "<tr><td class=\"label\">Report Year</td><td>: "
			<< reportYear << "</td><td class=\"label\">Report Type</td>"
			<< "<td>: Individual Yearly Performance</td></tr></table>";

		htmlFile << "<h2>Monthly Performance Overview</h2>";
		htmlFile << "<table class=\"months\"><thead><tr>";
		htmlFile << "<th>Month</th><th>Open</th><th>Present</th><th>Absent</th>"
			<< "<th>Attendance</th><th>In Perf.</th><th>Out Perf.</th>"
			<< "<th>Overall</th><th>Late Pickup</th><th>Late Charges</th>";
		htmlFile << "</tr></thead><tbody>";

		for (const YearlyPdfMonthRecord& record : months)
		{
			const string attendanceText =
				record.operatingDays > 0
				? formatYearlyNumber(record.attendancePercentage) + "%"
				: "N/A";

			const string checkInText =
				record.daysPresent > 0
				? formatYearlyNumber(record.checkInPerformance) + "%"
				: "N/A";

			const string checkOutText =
				record.completedRecords > 0
				? formatYearlyNumber(record.checkOutPerformance) + "%"
				: "N/A";

			const string overallText =
				record.completedRecords > 0
				? formatYearlyNumber(record.overallPerformance) + "%"
				: "N/A";

			htmlFile << "<tr>";
			htmlFile << "<td>" << monthNames[record.monthNumber - 1] << "</td>";
			htmlFile << "<td>" << record.operatingDays << "</td>";
			htmlFile << "<td>" << record.daysPresent << "</td>";
			htmlFile << "<td>" << record.daysAbsent << "</td>";
			htmlFile << "<td>" << attendanceText << "</td>";
			htmlFile << "<td>" << checkInText << "</td>";
			htmlFile << "<td>" << checkOutText << "</td>";
			htmlFile << "<td>" << overallText << "</td>";
			htmlFile << "<td>" << record.latePickups << "</td>";
			htmlFile << "<td>RM" << formatYearlyNumber(record.totalLateCharges) << "</td>";
			htmlFile << "</tr>";
		}

		htmlFile << "</tbody></table>";

		htmlFile << "<h2>Yearly Summary</h2><table class=\"summary\">";
		htmlFile << "<tr><td class=\"key\">Operating Days</td><td>" << yearlyOperatingDays
			<< "</td><td class=\"key\">Days Present</td><td>" << yearlyDaysPresent
			<< "</td><td class=\"key\">Days Absent</td><td>" << yearlyDaysAbsent << "</td></tr>";
		htmlFile << "<tr><td class=\"key\">Attendance</td><td>"
			<< (yearlyOperatingDays > 0 ? formatYearlyNumber(yearlyAttendancePercentage) + "%" : "N/A")
			<< "</td><td class=\"key\">Completed</td><td>" << yearlyCompletedRecords
			<< "</td><td class=\"key\">Pending</td><td>" << yearlyPendingRecords << "</td></tr>";
		htmlFile << "<tr><td class=\"key\">Check-In On Time</td><td>" << yearlyOnTimeCheckIns
			<< "</td><td class=\"key\">Check-In Late</td><td>" << yearlyLateCheckIns
			<< "</td><td class=\"key\">Late Pickup</td><td>" << yearlyLatePickups << "</td></tr>";
		htmlFile << "<tr><td class=\"key\">Pickup On Time</td><td>" << yearlyOnTimePickups
			<< "</td><td class=\"key\">Check-In Performance</td><td>" << formatYearlyNumber(yearlyCheckInPerformance) << "%"
			<< "</td><td class=\"key\">Check-Out Performance</td><td>"
			<< (yearlyCompletedRecords > 0 ? formatYearlyNumber(yearlyCheckOutPerformance) + "%" : "N/A")
			<< "</td></tr>";
		htmlFile << "<tr><td class=\"key\">Overall Performance</td><td>"
			<< (yearlyCompletedRecords > 0 ? formatYearlyNumber(yearlyOverallPerformance) + "%" : "N/A")
			<< "</td><td class=\"key\">Interpretation</td><td class=\"good\">"
			<< (yearlyCompletedRecords > 0 ? yearlyPerformanceInterpretation(yearlyOverallPerformance) : "N/A")
			<< "</td><td class=\"key\">Late Charges</td><td class=\"charge\">RM"
			<< formatYearlyNumber(yearlyLateCharges) << "</td></tr>";
		htmlFile << "<tr><td class=\"key\">Average Check-In Points</td><td>" << formatYearlyNumber(yearlyAverageCheckInPoints)
			<< "</td><td class=\"key\">Average Check-Out Points</td><td>"
			<< (yearlyCompletedRecords > 0 ? formatYearlyNumber(yearlyAverageCheckOutPoints) : "N/A")
			<< "</td><td class=\"key\">Average Daily Points</td><td>"
			<< (yearlyCompletedRecords > 0 ? formatYearlyNumber(yearlyAverageDailyPoints) : "N/A")
			<< "</td></tr>";
		htmlFile << "</table>";

		if (monthsWithOperatingCalendar < 12)
		{
			htmlFile << "<div class=\"notice\"><strong>Calendar Notice:</strong> "
				<< "Operating calendars are available for "
				<< monthsWithOperatingCalendar
				<< " month(s) only. Generate the remaining calendars for a complete yearly attendance percentage.</div>";
		}

		htmlFile << "<div class=\"footer\">"
			<< "Generated by CAPMS. A fixed RM5.00 late pickup "
			<< "charge is applied for each pickup recorded "
			<< "after 12:30 PM."
			<< "</div>";
		htmlFile << "</body></html>";
		htmlFile.close();

		cout << "\nGenerating Yearly Child Performance PDF...\n";

		if (convertHtmlToPdf(htmlPath, pdfPath))
		{
			cout << "\n============================================================\n";
			cout << "              YEARLY REPORT PDF EXPORTED                   \n";
			cout << "============================================================\n";
			cout << " Child       : " << childId << " - " << childName << "\n";
			cout << " Report Year : " << reportYear << "\n";
			cout << " PDF File    : " << fs::absolute(pdfPath).string() << "\n";
			cout << "============================================================\n";
		}
		else
		{
			cout << "\nERROR: Yearly PDF export failed.\n";
		}

		delete yearlyResult;
		delete yearlyStatement;
		delete childResult;
		delete childStatement;
	}
	catch (sql::SQLException& e)
	{
		delete yearlyResult;
		delete yearlyStatement;
		delete childResult;
		delete childStatement;

		cerr << "\nSQL Error while exporting Yearly Report PDF:\n";
		cerr << "Message    : " << e.what() << "\n";
		cerr << "Error Code : " << e.getErrorCode() << "\n";
		cerr << "SQL State  : " << e.getSQLState() << "\n";
	}
	catch (const exception& e)
	{
		delete yearlyResult;
		delete yearlyStatement;
		delete childResult;
		delete childStatement;

		cerr << "\nError while exporting Yearly Report PDF:\n";
		cerr << e.what() << "\n";
	}
}

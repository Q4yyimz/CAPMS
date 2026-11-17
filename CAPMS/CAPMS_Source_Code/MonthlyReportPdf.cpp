#include "MonthlyReportPdf.h"

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
#include <string>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

struct MonthlyPdfRecord
{
    string logId;
    string attendanceDate;
    string checkInTime;
    string checkInStatus;
    string checkOutTime;
    string pickupStatus;
    string collectorType;
    string collectorId;
    string collectorName;
    string relationship;
    string checkInPoints;
    string checkOutPoints;
    string dailyPoints;
    string lateCharge;
};

static string formatNumber(double value)
{
    ostringstream stream;
    stream << fixed << setprecision(2) << value;
    return stream.str();
}

static string formatMonthlyDisplayText(const string& value)
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

static string monthlyStatusClass(const string& status)
{
    if (status == "ON_TIME")
    {
        return "status-good";
    }

    if (status == "PENDING")
    {
        return "status-pending";
    }

    if (status == "LATE" || status == "LATE_PICKUP")
    {
        return "status-late";
    }

    return "";
}

void exportMonthlyChildReportPdf(
    sql::Connection* con,
    const UserSession& session
)
{
    string childId;
    string childName;
    int reportMonth = 0;
    int reportYear = 0;

    int totalOperatingDays = 0;
    int totalDaysPresent = 0;
    int totalDaysAbsent = 0;
    int completedRecords = 0;
    int pendingRecords = 0;
    int onTimeCheckIns = 0;
    int lateCheckIns = 0;
    int onTimePickups = 0;
    int latePickups = 0;

    double attendancePercentage = 0.0;
    double averageCheckInPoints = 0.0;
    double averageCheckOutPoints = 0.0;
    double averageDailyPoints = 0.0;
    double checkInPerformance = 0.0;
    double checkOutPerformance = 0.0;
    double overallPerformance = 0.0;
    double totalLatePickupCharges = 0.0;

    sql::PreparedStatement* childStatement = nullptr;
    sql::PreparedStatement* operatingStatement = nullptr;
    sql::PreparedStatement* summaryStatement = nullptr;
    sql::PreparedStatement* detailStatement = nullptr;

    sql::ResultSet* childResult = nullptr;
    sql::ResultSet* operatingResult = nullptr;
    sql::ResultSet* summaryResult = nullptr;
    sql::ResultSet* detailResult = nullptr;

    const string monthNames[12] =
    {
        "JANUARY", "FEBRUARY", "MARCH", "APRIL",
        "MAY", "JUNE", "JULY", "AUGUST",
        "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
    };

    cout << "\n";
    cout << "============================================================\n";
    cout << "          EXPORT INDIVIDUAL MONTHLY REPORT PDF               \n";
    cout << "============================================================\n";
    cout << " Logged-In Administrator : "
         << session.staffName
         << " (" << session.staffId << ")\n";
    cout << "------------------------------------------------------------\n";

    cout << " Enter Child ID               : ";
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

    cout << " Enter Report Month [1-12]    : ";

    if (!(cin >> reportMonth))
    {
        cin.clear();
        clearInputBuffer();
        cout << "\nERROR: Please enter a valid month.\n";
        return;
    }

    if (reportMonth < 1 || reportMonth > 12)
    {
        clearInputBuffer();
        cout << "\nERROR: Month must be between 1 and 12.\n";
        return;
    }

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

        operatingStatement = con->prepareStatement(
            "SELECT COUNT(*) AS Total_Operating_Days "
            "FROM OPERATING_DAY "
            "WHERE MONTH(Operating_Date) = ? "
            "AND YEAR(Operating_Date) = ? "
            "AND Day_Status = 'OPEN'"
        );
        operatingStatement->setInt(1, reportMonth);
        operatingStatement->setInt(2, reportYear);
        operatingResult = operatingStatement->executeQuery();

        if (operatingResult->next())
        {
            totalOperatingDays = operatingResult->getInt("Total_Operating_Days");
        }

        summaryStatement = con->prepareStatement(
            "SELECT "
            "COUNT(*) AS Total_Days_Present, "
            "SUM(CASE WHEN al.CheckOut_Points IS NOT NULL THEN 1 ELSE 0 END) AS Completed_Records, "
            "SUM(CASE WHEN al.CheckOut_Points IS NULL THEN 1 ELSE 0 END) AS Pending_Records, "
            "SUM(CASE WHEN ci.Status = 'ON_TIME' THEN 1 ELSE 0 END) AS On_Time_CheckIns, "
            "SUM(CASE WHEN ci.Status = 'LATE' THEN 1 ELSE 0 END) AS Late_CheckIns, "
            "SUM(CASE WHEN co.Status = 'ON_TIME' THEN 1 ELSE 0 END) AS On_Time_Pickups, "
            "SUM(CASE WHEN co.Status = 'LATE_PICKUP' THEN 1 ELSE 0 END) AS Late_Pickups, "
            "COALESCE(AVG(al.CheckIn_Points), 0.00) AS Average_CheckIn_Points, "
            "COALESCE(AVG(al.CheckOut_Points), 0.00) AS Average_CheckOut_Points, "
            "COALESCE(AVG(al.Daily_Points), 0.00) AS Average_Daily_Points, "
            "COALESCE(SUM(al.Late_Pickup_Charge), 0.00) AS Total_Late_Charges "
            "FROM ATTENDANCE_LOG al "
            "LEFT JOIN LOG_RECORD ci ON al.Log_ID = ci.Log_ID AND ci.Record_Type = 'CHECK_IN' "
            "LEFT JOIN LOG_RECORD co ON al.Log_ID = co.Log_ID AND co.Record_Type = 'CHECK_OUT' "
            "WHERE al.Child_ID = ? "
            "AND MONTH(al.Log_Date) = ? "
            "AND YEAR(al.Log_Date) = ?"
        );
        summaryStatement->setString(1, childId);
        summaryStatement->setInt(2, reportMonth);
        summaryStatement->setInt(3, reportYear);
        summaryResult = summaryStatement->executeQuery();

        if (summaryResult->next())
        {
            totalDaysPresent = summaryResult->getInt("Total_Days_Present");
            completedRecords = summaryResult->getInt("Completed_Records");
            pendingRecords = summaryResult->getInt("Pending_Records");
            onTimeCheckIns = summaryResult->getInt("On_Time_CheckIns");
            lateCheckIns = summaryResult->getInt("Late_CheckIns");
            onTimePickups = summaryResult->getInt("On_Time_Pickups");
            latePickups = summaryResult->getInt("Late_Pickups");
            averageCheckInPoints = summaryResult->getDouble("Average_CheckIn_Points");
            averageCheckOutPoints = summaryResult->getDouble("Average_CheckOut_Points");
            averageDailyPoints = summaryResult->getDouble("Average_Daily_Points");
            totalLatePickupCharges = summaryResult->getDouble("Total_Late_Charges");
        }

        if (totalDaysPresent == 0)
        {
            cout << "\nNo attendance records were found for "
                 << childId << " in "
                 << monthNames[reportMonth - 1] << " "
                 << reportYear << ".\n";

            delete summaryResult;
            delete summaryStatement;
            delete operatingResult;
            delete operatingStatement;
            delete childResult;
            delete childStatement;
            return;
        }

        totalDaysAbsent = totalOperatingDays - totalDaysPresent;
        if (totalDaysAbsent < 0)
        {
            totalDaysAbsent = 0;
        }

        if (totalOperatingDays > 0)
        {
            attendancePercentage =
                static_cast<double>(totalDaysPresent) /
                static_cast<double>(totalOperatingDays) * 100.0;
        }

        checkInPerformance = averageCheckInPoints * 100.0;

        if (completedRecords > 0)
        {
            checkOutPerformance = averageCheckOutPoints * 100.0;
            overallPerformance = averageDailyPoints * 100.0;
        }

        detailStatement = con->prepareStatement(
            "SELECT "
            "al.Log_ID, DATE_FORMAT(al.Log_Date, '%d/%m/%Y') AS Formatted_Date, "
            "DATE_FORMAT(ci.Record_Time, '%H:%i:%s') AS CheckIn_Time, ci.Status AS CheckIn_Status, "
            "DATE_FORMAT(co.Record_Time, '%H:%i:%s') AS CheckOut_Time, co.Status AS Pickup_Status, "
            "co.Parent_ID AS Parent_ID, p.Name AS Parent_Name, pc.Relationship_Type AS Parent_Relationship, "
            "co.Rep_ID AS Rep_ID, ar.Name AS Rep_Name, ac.Relationship AS Rep_Relationship, "
            "al.CheckIn_Points, al.CheckOut_Points, al.Daily_Points, al.Late_Pickup_Charge "
            "FROM ATTENDANCE_LOG al "
            "LEFT JOIN LOG_RECORD ci ON al.Log_ID = ci.Log_ID AND ci.Record_Type = 'CHECK_IN' "
            "LEFT JOIN LOG_RECORD co ON al.Log_ID = co.Log_ID AND co.Record_Type = 'CHECK_OUT' "
            "LEFT JOIN PARENT p ON co.Parent_ID = p.Parent_ID "
            "LEFT JOIN PARENT_CHILD pc ON co.Parent_ID = pc.Parent_ID AND al.Child_ID = pc.Child_ID "
            "LEFT JOIN AUTHORIZED_REPRESENTATIVE ar ON co.Rep_ID = ar.Rep_ID "
            "LEFT JOIN AUTHOREP_CHILD ac ON co.Rep_ID = ac.Rep_ID AND al.Child_ID = ac.Child_ID "
            "WHERE al.Child_ID = ? AND MONTH(al.Log_Date) = ? AND YEAR(al.Log_Date) = ? "
            "ORDER BY al.Log_Date ASC"
        );
        detailStatement->setString(1, childId);
        detailStatement->setInt(2, reportMonth);
        detailStatement->setInt(3, reportYear);
        detailResult = detailStatement->executeQuery();

        vector<MonthlyPdfRecord> records;

        while (detailResult->next())
        {
            MonthlyPdfRecord record;
            const bool completed = !detailResult->isNull("CheckOut_Time");

            record.logId = string(detailResult->getString("Log_ID").c_str());
            record.attendanceDate = string(detailResult->getString("Formatted_Date").c_str());
            record.checkInTime = detailResult->isNull("CheckIn_Time") ? "-" : string(detailResult->getString("CheckIn_Time").c_str());
            record.checkInStatus = detailResult->isNull("CheckIn_Status") ? "-" : string(detailResult->getString("CheckIn_Status").c_str());
            record.checkOutTime = completed ? string(detailResult->getString("CheckOut_Time").c_str()) : "Pending";
            record.pickupStatus = detailResult->isNull("Pickup_Status") ? "PENDING" : string(detailResult->getString("Pickup_Status").c_str());
            record.checkInPoints = detailResult->isNull("CheckIn_Points") ? "N/A" : formatNumber(detailResult->getDouble("CheckIn_Points"));
            record.checkOutPoints = detailResult->isNull("CheckOut_Points") ? "Pending" : formatNumber(detailResult->getDouble("CheckOut_Points"));
            record.dailyPoints = detailResult->isNull("Daily_Points") ? "Pending" : formatNumber(detailResult->getDouble("Daily_Points"));
            record.lateCharge = completed ? "RM" + formatNumber(detailResult->getDouble("Late_Pickup_Charge")) : "Pending";

            if (!completed)
            {
                record.collectorType = "PENDING";
                record.collectorId = "-";
                record.collectorName = "-";
                record.relationship = "-";
            }
            else if (!detailResult->isNull("Parent_ID"))
            {
                record.collectorType = "PARENT";
                record.collectorId = string(detailResult->getString("Parent_ID").c_str());
                record.collectorName = detailResult->isNull("Parent_Name") ? "-" : string(detailResult->getString("Parent_Name").c_str());
                record.relationship = detailResult->isNull("Parent_Relationship") ? "-" : string(detailResult->getString("Parent_Relationship").c_str());
            }
            else if (!detailResult->isNull("Rep_ID"))
            {
                record.collectorType = "REPRESENTATIVE";
                record.collectorId = string(detailResult->getString("Rep_ID").c_str());
                record.collectorName = detailResult->isNull("Rep_Name") ? "-" : string(detailResult->getString("Rep_Name").c_str());
                record.relationship = detailResult->isNull("Rep_Relationship") ? "-" : string(detailResult->getString("Rep_Relationship").c_str());
            }
            else
            {
                record.collectorType = "LEGACY";
                record.collectorId = "-";
                record.collectorName = "-";
                record.relationship = "-";
            }

            records.push_back(record);
        }

        const string outputFolder = "CAPMS_Reports";
        fs::create_directories(outputFolder);

        ostringstream monthStream;
        monthStream << setw(2) << setfill('0') << reportMonth;

        const string baseName =
            "Monthly_Performance_" + childId + "_" +
            to_string(reportYear) + "-" + monthStream.str();

        const string htmlPath = outputFolder + "\\" + baseName + ".html";
        const string pdfPath = outputFolder + "\\" + baseName + ".pdf";

        ofstream htmlFile(htmlPath);

        if (!htmlFile.is_open())
        {
            throw runtime_error("Unable to create the Monthly Report HTML file.");
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
.label { font-weight: bold; color: #374151; width: 145px; }
.summary td { border: 1px solid #d1d5db; padding: 7px 9px; }
.summary .key { background: #eff6ff; font-weight: bold; }
.records { table-layout: fixed; }
.records th { background: #1e3a8a; color: white; border: 1px solid #cbd5e1; padding: 7px 3px; font-size: 7.5pt; }
.records td { border: 1px solid #cbd5e1; padding: 6px 3px; text-align: center; vertical-align: middle; overflow-wrap: break-word; }
.records tbody tr:nth-child(even) { background: #f8fafc; }
.status-good { color: #166534; font-weight: bold; }
.status-late { color: #b91c1c; font-weight: bold; }
.status-pending { color: #92400e; font-weight: bold; }
.footer { margin-top: 20px; border-top: 1px solid #cbd5e1; padding-top: 8px; color: #64748b; font-size: 8pt; }
thead { display: table-header-group; }
tr { page-break-inside: avoid; }
</style>
</head>
<body>
<div class="header">
<h1>Individual Monthly Performance Report</h1>
<div class="subtitle">Childcare Attendance and Pickup Management System</div>
</div>
)HTML";

        htmlFile << "<table class=\"meta\">";
        htmlFile << "<tr><td class=\"label\">Child</td><td>: " << escapeHtml(childId) << " - " << escapeHtml(childName) << "</td>";
        htmlFile << "<td class=\"label\">Generated By</td><td>: " << escapeHtml(session.staffName) << " (" << escapeHtml(session.staffId) << ")</td></tr>";
        htmlFile << "<tr><td class=\"label\">Report Period</td><td>: " << monthNames[reportMonth - 1] << " " << reportYear << "</td>";
        htmlFile << "<td class=\"label\">Report Type</td><td>: Individual Monthly Performance</td></tr></table>";

        htmlFile << "<h2>Monthly Summary</h2><table class=\"summary\">";
        htmlFile << "<tr><td class=\"key\">Operating Days</td><td>" << totalOperatingDays << "</td><td class=\"key\">Days Present</td><td>" << totalDaysPresent << "</td><td class=\"key\">Days Absent</td><td>" << totalDaysAbsent << "</td></tr>";
        htmlFile << "<tr><td class=\"key\">Attendance</td><td>" << (totalOperatingDays > 0 ? formatNumber(attendancePercentage) + "%" : "N/A") << "</td><td class=\"key\">Completed</td><td>" << completedRecords << "</td><td class=\"key\">Pending</td><td>" << pendingRecords << "</td></tr>";
        htmlFile << "<tr><td class=\"key\">Check-In Performance</td><td>" << formatNumber(checkInPerformance) << "%</td><td class=\"key\">Check-Out Performance</td><td>" << (completedRecords > 0 ? formatNumber(checkOutPerformance) + "%" : "N/A") << "</td><td class=\"key\">Overall Performance</td><td>" << (completedRecords > 0 ? formatNumber(overallPerformance) + "%" : "N/A") << "</td></tr>";
        htmlFile << "<tr><td class=\"key\">Check-In On Time</td><td>" << onTimeCheckIns << "</td><td class=\"key\">Check-In Late</td><td>" << lateCheckIns << "</td><td class=\"key\">Late Pickup</td><td>" << latePickups << "</td></tr>";
        htmlFile << "<tr><td class=\"key\">Pickup On Time</td><td>" << onTimePickups << "</td><td class=\"key\">Average Daily Points</td><td>" << (completedRecords > 0 ? formatNumber(averageDailyPoints) : "N/A") << "</td><td class=\"key\">Late Charges</td><td>RM" << formatNumber(totalLatePickupCharges) << "</td></tr>";
        htmlFile << "</table>";

        htmlFile << "<h2>Detailed Monthly Attendance</h2><table class=\"records\">";
        htmlFile << "<thead><tr><th>Date</th><th>In Time</th><th>In Status</th><th>In Points</th><th>Out Time</th><th>Pickup</th><th>Out Points</th><th>Daily</th><th>Collector</th><th>ID</th><th>Name</th><th>Relationship</th><th>Charge</th></tr></thead><tbody>";

        for (const MonthlyPdfRecord& record : records)
        {
            htmlFile << "<tr>";
            htmlFile << "<td>" << escapeHtml(record.attendanceDate) << "</td>";
            htmlFile << "<td>" << escapeHtml(record.checkInTime) << "</td>";
            htmlFile << "<td class=\"" << monthlyStatusClass(record.checkInStatus) << "\">" << escapeHtml(formatMonthlyDisplayText(record.checkInStatus)) << "</td>";
            htmlFile << "<td>" << record.checkInPoints << "</td>";
            htmlFile << "<td>" << escapeHtml(record.checkOutTime) << "</td>";
            htmlFile << "<td class=\"" << monthlyStatusClass(record.pickupStatus) << "\">" << escapeHtml(formatMonthlyDisplayText(record.pickupStatus)) << "</td>";
            htmlFile << "<td>" << record.checkOutPoints << "</td>";
            htmlFile << "<td>" << record.dailyPoints << "</td>";
            htmlFile << "<td>" << escapeHtml(formatMonthlyDisplayText(record.collectorType)) << "</td>";
            htmlFile << "<td>" << escapeHtml(record.collectorId) << "</td>";
            htmlFile << "<td>" << escapeHtml(record.collectorName) << "</td>";
            htmlFile << "<td>" << escapeHtml(formatMonthlyDisplayText(record.relationship)) << "</td>";
            htmlFile << "<td>" << record.lateCharge << "</td>";
            htmlFile << "</tr>";
        }

        htmlFile << "</tbody></table>";
        htmlFile << "<div class=\"footer\">"
            << "Generated by CAPMS. A fixed RM5.00 late pickup "
            << "charge is applied for each pickup recorded "
            << "after 12:30 PM."
            << "</div>";
        htmlFile << "</body></html>";
        htmlFile.close();

        cout << "\nGenerating Monthly Child Performance PDF...\n";

        if (convertHtmlToPdf(htmlPath, pdfPath))
        {
            cout << "\n============================================================\n";
            cout << "             MONTHLY REPORT PDF EXPORTED                   \n";
            cout << "============================================================\n";
            cout << " Child         : " << childId << " - " << childName << "\n";
            cout << " Report Period : " << monthNames[reportMonth - 1] << " " << reportYear << "\n";
            cout << " PDF File      : " << fs::absolute(pdfPath).string() << "\n";
            cout << "============================================================\n";
        }
        else
        {
            cout << "\nERROR: Monthly PDF export failed.\n";
        }

        delete detailResult;
        delete detailStatement;
        delete summaryResult;
        delete summaryStatement;
        delete operatingResult;
        delete operatingStatement;
        delete childResult;
        delete childStatement;
    }
    catch (sql::SQLException& e)
    {
        delete detailResult;
        delete detailStatement;
        delete summaryResult;
        delete summaryStatement;
        delete operatingResult;
        delete operatingStatement;
        delete childResult;
        delete childStatement;

        cerr << "\nSQL Error while exporting Monthly Report PDF:\n";
        cerr << "Message    : " << e.what() << "\n";
        cerr << "Error Code : " << e.getErrorCode() << "\n";
        cerr << "SQL State  : " << e.getSQLState() << "\n";
    }
    catch (const exception& e)
    {
        delete detailResult;
        delete detailStatement;
        delete summaryResult;
        delete summaryStatement;
        delete operatingResult;
        delete operatingStatement;
        delete childResult;
        delete childStatement;

        cerr << "\nError while exporting Monthly Report PDF:\n";
        cerr << e.what() << "\n";
    }
}

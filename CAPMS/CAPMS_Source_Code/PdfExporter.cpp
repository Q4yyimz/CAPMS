#include "PdfExporter.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <process.h>
#include <string>
#include <thread>
#include <vector>

using namespace std;
namespace fs = std::filesystem;

// ==========================================================
// FIND GOOGLE CHROME
// ==========================================================

string findGoogleChrome()
{
	vector<fs::path> possiblePaths =
	{
		"C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
		"C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe"
	};

	char* localAppData = nullptr;
	size_t localAppDataLength = 0;

	const errno_t environmentResult =
		_dupenv_s(
			&localAppData,
			&localAppDataLength,
			"LOCALAPPDATA"
		);

	if (environmentResult == 0 &&
		localAppData != nullptr)
	{
		possiblePaths.push_back(
			fs::path(localAppData) /
			"Google" /
			"Chrome" /
			"Application" /
			"chrome.exe"
		);
	}

	free(localAppData);
	localAppData = nullptr;

	for (const fs::path& path : possiblePaths)
	{
		if (fs::exists(path))
		{
			return path.string();
		}
	}

	return "";
}

// ==========================================================
// ESCAPE HTML SPECIAL CHARACTERS
// ==========================================================

string escapeHtml(
	const string& text
)
{
	string escapedText;

	for (char character : text)
	{
		switch (character)
		{
		case '&':
			escapedText += "&amp;";
			break;

		case '<':
			escapedText += "&lt;";
			break;

		case '>':
			escapedText += "&gt;";
			break;

		case '"':
			escapedText += "&quot;";
			break;

		case '\'':
			escapedText += "&#39;";
			break;

		default:
			escapedText += character;
			break;
		}
	}

	return escapedText;
}

// ==========================================================
// CONVERT LOCAL HTML TO PDF USING GOOGLE CHROME
// ==========================================================

bool convertHtmlToPdf(
	const string& htmlFilePath,
	const string& pdfFilePath
)
{
	const string chromePath =
		findGoogleChrome();

	if (chromePath.empty())
	{
		cerr << "\nERROR: Google Chrome was not found.\n";
		return false;
	}

	try
	{
		const fs::path originalHtmlPath =
			fs::absolute(htmlFilePath);

		const fs::path finalPdfPath =
			fs::absolute(pdfFilePath);

		if (!fs::exists(originalHtmlPath))
		{
			cerr << "\nERROR: HTML file was not found:\n";
			cerr << originalHtmlPath.string() << "\n";
			return false;
		}

		// ==================================================
		// PREPARE SIMPLE TEMPORARY PATHS
		// ==================================================

		const fs::path temporaryDirectory =
			"C:\\CAPMS_PDF_TEMP";

		const fs::path temporaryProfile =
			temporaryDirectory /
			"ChromeProfile";

		const fs::path temporaryHtmlPath =
			temporaryDirectory /
			"CAPMS_Report_Temp.html";

		const fs::path temporaryPdfPath =
			temporaryDirectory /
			"CAPMS_Report_Temp.pdf";

		fs::create_directories(
			temporaryDirectory
		);

		/*
		 * Remove files left by a previous export.
		 */
		if (fs::exists(temporaryHtmlPath))
		{
			fs::remove(
				temporaryHtmlPath
			);
		}

		if (fs::exists(temporaryPdfPath))
		{
			fs::remove(
				temporaryPdfPath
			);
		}

		/*
		 * A closed Chrome profile from an earlier attempt
		 * may interfere with the next export.
		 */
		if (fs::exists(temporaryProfile))
		{
			try
			{
				fs::remove_all(
					temporaryProfile
				);
			}
			catch (...)
			{
				cerr << "\nERROR: Unable to clear the temporary "
					<< "Chrome profile.\n";

				cerr << "Close all Chrome windows and try again.\n";
				return false;
			}
		}

		fs::create_directories(
			temporaryProfile
		);

		/*
		 * Copy the generated HTML from the project folder
		 * to a short path without spaces.
		 */
		fs::copy_file(
			originalHtmlPath,
			temporaryHtmlPath,
			fs::copy_options::overwrite_existing
		);

		// ==================================================
		// PREPARE CHROME COMMAND
		// ==================================================

		const string command =
			"\"\"" +
			chromePath +
			"\" "
			"--headless=new "
			"--disable-gpu "
			"--no-first-run "
			"--no-default-browser-check "
			"--no-pdf-header-footer "
			"--user-data-dir=\"C:\\CAPMS_PDF_TEMP\\ChromeProfile\" "
			"--print-to-pdf=\"C:\\CAPMS_PDF_TEMP\\CAPMS_Report_Temp.pdf\" "
			"\"file:///C:/CAPMS_PDF_TEMP/CAPMS_Report_Temp.html\"\"";

		cout << "\nGoogle Chrome location:\n";
		cout << chromePath << "\n";

		cout << "\nTemporary HTML:\n";
		cout << temporaryHtmlPath.string() << "\n";

		cout << "\nTemporary PDF:\n";
		cout << temporaryPdfPath.string() << "\n";

		const int exitCode =
			system(
				command.c_str()
			);

		// ==================================================
		// WAIT FOR PDF CREATION
		// ==================================================

		bool temporaryPdfCreated = false;

		for (
			int attempt = 0;
			attempt < 40;
			attempt++
			)
		{
			if (
				fs::exists(temporaryPdfPath) &&
				fs::file_size(temporaryPdfPath) > 0
				)
			{
				temporaryPdfCreated = true;
				break;
			}

			this_thread::sleep_for(
				chrono::milliseconds(250)
			);
		}

		if (!temporaryPdfCreated)
		{
			cerr << "\nERROR: Temporary PDF was not generated.\n";
			cerr << "Google Chrome exit code: "
				<< exitCode << "\n";

			cerr << "Close all Chrome windows and try again.\n";
			return false;
		}

		// ==================================================
		// COPY PDF TO FINAL REPORT LOCATION
		// ==================================================

		const fs::path finalPdfDirectory =
			finalPdfPath.parent_path();

		if (!finalPdfDirectory.empty())
		{
			fs::create_directories(
				finalPdfDirectory
			);
		}

		if (fs::exists(finalPdfPath))
		{
			fs::remove(
				finalPdfPath
			);
		}

		fs::copy_file(
			temporaryPdfPath,
			finalPdfPath,
			fs::copy_options::overwrite_existing
		);

		if (
			!fs::exists(finalPdfPath) ||
			fs::file_size(finalPdfPath) == 0
			)
		{
			cerr << "\nERROR: PDF could not be copied "
				<< "to the final report folder.\n";
			return false;
		}

		// ==================================================
		// CLEAN TEMPORARY REPORT FILES
		// ==================================================

		try
		{
			if (fs::exists(temporaryHtmlPath))
			{
				fs::remove(
					temporaryHtmlPath
				);
			}

			if (fs::exists(temporaryPdfPath))
			{
				fs::remove(
					temporaryPdfPath
				);
			}

			/*
			 * Chrome may still briefly hold files inside
			 * the profile. Failure to delete the profile
			 * does not indicate PDF export failure.
			 */
			if (fs::exists(temporaryProfile))
			{
				fs::remove_all(
					temporaryProfile
				);
			}
		}
		catch (...)
		{
			// Final PDF already exists and is valid.
		}

		return true;
	}
	catch (const fs::filesystem_error& e)
	{
		cerr << "\nFile system error during PDF export:\n";
		cerr << e.what() << "\n";
		return false;
	}
	catch (const exception& e)
	{
		cerr << "\nUnexpected PDF export error:\n";
		cerr << e.what() << "\n";
		return false;
	}
}

// ==========================================================
// TEST PDF EXPORT
// ==========================================================

void testPdfExport()
{
	const string outputFolder =
		"CAPMS_Reports";

	const string htmlFilePath =
		outputFolder +
		"\\CAPMS_PDF_Test.html";

	const string pdfFilePath =
		outputFolder +
		"\\CAPMS_PDF_Test.pdf";

	try
	{
		if (!fs::exists(outputFolder))
		{
			fs::create_directories(
				outputFolder
			);
		}

		ofstream htmlFile(
			htmlFilePath
		);

		if (!htmlFile.is_open())
		{
			cout << "\nERROR: Unable to create test HTML file.\n";
			return;
		}

		htmlFile << R"(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">

<style>
    @page
    {
        size: A4;
        margin: 18mm;
    }

    body
    {
        font-family: Arial, Helvetica, sans-serif;
        color: #1f2937;
        font-size: 11pt;
        line-height: 1.4;
        margin: 0;
    }

    .report-header
    {
        border-bottom: 3px solid #2563eb;
        padding-bottom: 12px;
        margin-bottom: 24px;
    }

    .report-title
    {
        color: #1e3a8a;
        font-size: 22pt;
        font-weight: bold;
        margin: 0;
    }

    .report-subtitle
    {
        color: #4b5563;
        font-size: 11pt;
        margin-top: 6px;
    }

    .information-box
    {
        background-color: #eff6ff;
        border-left: 5px solid #2563eb;
        padding: 12px 16px;
        margin-bottom: 22px;
    }

    table
    {
        width: 100%;
        border-collapse: collapse;
        margin-top: 12px;
    }

    th
    {
        background-color: #1e3a8a;
        color: white;
        padding: 9px;
        text-align: left;
        font-size: 10pt;
    }

    td
    {
        border: 1px solid #d1d5db;
        padding: 9px;
        vertical-align: middle;
    }

    tr:nth-child(even)
    {
        background-color: #f8fafc;
    }

    .success
    {
        color: #166534;
        font-weight: bold;
    }

    .report-footer
    {
        border-top: 1px solid #d1d5db;
        margin-top: 30px;
        padding-top: 10px;
        color: #6b7280;
        font-size: 9pt;
    }
</style>
</head>

<body>

<div class="report-header">
    <div class="report-title">
        CAPMS PDF Export Test
    </div>

    <div class="report-subtitle">
        Childcare Attendance and Pickup Management System
    </div>
</div>

<div class="information-box">
    <strong>Export Status:</strong>
    <span class="success">PDF generation is working.</span>
</div>

<h2>Test Report Information</h2>

<table>
    <thead>
        <tr>
            <th>Item</th>
            <th>Test Value</th>
            <th>Status</th>
        </tr>
    </thead>

    <tbody>
        <tr>
            <td>Report Format</td>
            <td>A4 PDF</td>
            <td class="success">Successful</td>
        </tr>

        <tr>
            <td>PDF Engine</td>
            <td>Google Chrome</td>
            <td class="success">Available</td>
        </tr>

        <tr>
            <td>CAPMS Reports</td>
            <td>Daily, Monthly and Yearly</td>
            <td>Ready for implementation</td>
        </tr>
    </tbody>
</table>

<div class="report-footer">
    Generated by CAPMS PDF Export Utility.
</div>

</body>
</html>
)";

		htmlFile.close();

		cout << "\nGenerating PDF test file...\n";

		const bool exportSuccessful =
			convertHtmlToPdf(
				htmlFilePath,
				pdfFilePath
			);

		if (exportSuccessful)
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                  PDF EXPORT SUCCESSFUL                    \n";
			cout << "============================================================\n";
			cout << " PDF File : "
				<< fs::absolute(
					pdfFilePath
				).string()
				<< "\n";
			cout << "============================================================\n";
		}
		else
		{
			cout << "\n";
			cout << "============================================================\n";
			cout << "                    PDF EXPORT FAILED                      \n";
			cout << "============================================================\n";
			cout << " Check whether Google Chrome is installed.\n";
			cout << "============================================================\n";
		}
	}
	catch (const exception& e)
	{
		cerr << "\nERROR while testing PDF export:\n";
		cerr << e.what() << "\n";
	}
}
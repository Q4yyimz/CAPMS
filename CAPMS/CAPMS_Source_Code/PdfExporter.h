#ifndef PDF_EXPORTER_H
#define PDF_EXPORTER_H

#include <string>

bool convertHtmlToPdf(
    const std::string& htmlFilePath,
    const std::string& pdfFilePath
);

std::string escapeHtml(
    const std::string& text
);

std::string findGoogleChrome();

void testPdfExport();

#endif
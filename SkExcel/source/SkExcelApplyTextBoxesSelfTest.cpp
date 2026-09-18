//=============================================================================
// SkExcelApplyTextBoxesSelfTest — CLI self-test for ApplyTextBoxes import path
//=============================================================================
#include "SkExcelApplyTextBoxesSelfTest.hpp"
#include "SkExcel2SpreadSheet.hpp"
#include "SkExcelPugiXMLReader.hpp"

#include <SkApi.hpp>
#include <SkFormatCssApi.hpp>

#include <iostream>

using namespace SkRoot;
using namespace SkSpreadSheet;
using namespace SkFormat;

namespace SkExcel {

tString DefaultApplyTextBoxesTestXlsxPath() {
#if defined(__EMSCRIPTEN__)
    return "/home/web_user/Projects/Excel/Feuille de bilan comptable bleue.xlsx";
#elif defined(SKER_FILE_DIR)
    return tString(SKER_FILE_DIR) + "/Feuille de bilan comptable bleue.xlsx";
#else
    return {};
#endif
}

tInt RunApplyTextBoxesSelfTest(const tString& sXlsxPath) {
    if (sXlsxPath.empty()) {
        std::cerr << "ApplyTextBoxes test: empty xlsx path" << std::endl;
        return 1;
    }

    tExcelPugiXMLReader wReader;
    if (!wReader.LoadExcelFile(sXlsxPath)) {
        std::cerr << "ApplyTextBoxes test: failed to load " << sXlsxPath << std::endl;
        return 1;
    }
    wReader.ParseWorkbook();
    wReader.ParseTheme();

    const std::vector<tExcelTextBoxEntry> wCollected = wReader.CollectTextBoxes();
    std::cout << "ApplyTextBoxes test: CollectTextBoxes=" << wCollected.size()
              << " from " << sXlsxPath << std::endl;
    for (const tExcelTextBoxEntry& wEntry : wCollected) {
        std::cout << "  [" << wEntry.SheetName << "] "
                  << (wEntry.Name.empty() ? "(unnamed)" : wEntry.Name)
                  << " text=" << wEntry.Text.substr(0, 40)
                  << (wEntry.Text.size() > 40 ? "..." : "")
                  << std::endl;
    }

    tApi wApi;
    tFormatCssApi wFormatApi;
    wApi.FormatApi(&wFormatApi);
    wApi.WorkBook("/ApplyTextBoxesTest.sker");
    wApi.DeleteSheet("Sheet1");

    tExcel2SpreadSheet wImporter;
    if (!wImporter.ImportXlsxToApi(sXlsxPath, wApi)) {
        std::cerr << "ApplyTextBoxes test: ImportXlsxToApi failed" << std::endl;
        return 1;
    }

    const tString wJson = wApi.WriteJson("/ApplyTextBoxesTest.sker");
    if (wJson.find("SkCellClassTextBox") == tString::npos) {
        if (wCollected.empty()) {
            std::cout << "ApplyTextBoxes test: SKIP (no text boxes in workbook)" << std::endl;
            return 0;
        }
        std::cerr << "ApplyTextBoxes test: JSON missing SkCellClassTextBox" << std::endl;
        return 1;
    }

    const tBool wIsBilanFile = sXlsxPath.find("bilan") != tString::npos
        || sXlsxPath.find("Bilan") != tString::npos;
    if (wIsBilanFile && wJson.find("Solde") == tString::npos) {
        std::cerr << "ApplyTextBoxes test: JSON missing expected text \"Solde\"" << std::endl;
        return 1;
    }

    std::cout << "ApplyTextBoxes test: PASS" << std::endl;
    return 0;
}

} // namespace SkExcel

//=============================================================================
// SkExcelApplyChartsSelfTest — CLI self-test for ApplyCharts import path
//=============================================================================
#include "SkExcelApplyChartsSelfTest.hpp"
#include "SkExcel2SpreadSheet.hpp"
#include "SkExcelChartImport.hpp"
#include "SkExcelPugiXMLReader.hpp"

#include <SkApi.hpp>
#include <SkFormatCssApi.hpp>

#include <iostream>

using namespace SkRoot;
using namespace SkSpreadSheet;
using namespace SkFormat;

namespace SkExcel {

tString DefaultApplyChartsTestXlsxPath() {
#if defined(__EMSCRIPTEN__)
    return "/home/web_user/Projects/Excel/Feuille de bilan comptable bleue.xlsx";
#elif defined(SKER_FILE_DIR)
    return tString(SKER_FILE_DIR) + "/Feuille de bilan comptable bleue.xlsx";
#else
    return {};
#endif
}

tInt RunApplyChartsSelfTest(const tString& sXlsxPath) {
    if (sXlsxPath.empty()) {
        std::cerr << "ApplyCharts test: empty xlsx path" << std::endl;
        return 1;
    }

    tExcelPugiXMLReader wReader;
    if (!wReader.LoadExcelFile(sXlsxPath)) {
        std::cerr << "ApplyCharts test: failed to load " << sXlsxPath << std::endl;
        return 1;
    }
    wReader.ParseWorkbook();
    wReader.ParseTheme();

    const std::vector<tExcelChartEntry> wCollected = wReader.CollectCharts();
    std::cout << "ApplyCharts test: CollectCharts=" << wCollected.size()
              << " from " << sXlsxPath << std::endl;
    for (const tExcelChartEntry& wEntry : wCollected) {
        std::cout << "  [" << wEntry.SheetName << "] "
                  << (wEntry.Name.empty() ? "(unnamed)" : wEntry.Name)
                  << " type=" << wEntry.SkerChartType
                  << " title=" << wEntry.Title
                  << " series=" << wEntry.Series.size()
                  << std::endl;
    }

    if (wCollected.empty()) {
        std::cout << "ApplyCharts test: SKIP (no charts in workbook)" << std::endl;
        return 2;
    }

    tApi wApi;
    tFormatCssApi wFormatApi;
    wApi.FormatApi(&wFormatApi);
    wApi.WorkBook("/ApplyChartsTest.sker");

    tExcel2SpreadSheet wConverter;
    if (!wConverter.ImportXlsxToApi(sXlsxPath, wApi)) {
        std::cerr << "ApplyCharts test: ImportXlsxToApi failed" << std::endl;
        return 1;
    }

    const tString wJson = wApi.WriteJson("/ApplyChartsTest.sker");
    if (wJson.find("SkCellClassLineChart") == tString::npos
        && wJson.find("SkCellClassPieChart") == tString::npos) {
        std::cerr << "ApplyCharts test: JSON missing SkCellClassLineChart/PieChart" << std::endl;
        return 1;
    }
    if (wJson.find("Actifs") == tString::npos) {
        std::cerr << "ApplyCharts test: JSON missing chart title \"Actifs\"" << std::endl;
        return 1;
    }
    if (wJson.find("chartType") == tString::npos || wJson.find("bar") == tString::npos) {
        std::cerr << "ApplyCharts test: JSON missing bar chartType" << std::endl;
        return 1;
    }
    if (wJson.find("DATARANGE") == tString::npos) {
        std::cerr << "ApplyCharts test: JSON missing DATARANGE on chart range attributes" << std::endl;
        return 1;
    }
    if (wJson.find("barDirection") == tString::npos || wJson.find("horizontal") == tString::npos) {
        std::cerr << "ApplyCharts test: JSON missing horizontal barDirection" << std::endl;
        return 1;
    }
    if (wJson.find("Passifs et") == tString::npos) {
        std::cerr << "ApplyCharts test: JSON missing Passifs chart title" << std::endl;
        return 1;
    }
    // Second chart uses union refs on sheet name with comma — chartData/DataRange must import quoted.
    if (wJson.find("'Passifs, Équité du Propriétaire'!") == tString::npos) {
        std::cerr << "ApplyCharts test: JSON missing quoted Passifs sheet in chart DATARANGE" << std::endl;
        return 1;
    }

    std::cout << "ApplyCharts test: PASS" << std::endl;
    return 0;
}

} // namespace SkExcel

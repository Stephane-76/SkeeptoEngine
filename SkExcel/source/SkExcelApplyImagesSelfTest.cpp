//=============================================================================
// SkExcelApplyImagesSelfTest — CLI self-test for ApplyImages import path
//=============================================================================
#include "SkExcelApplyImagesSelfTest.hpp"
#include "SkExcel2SpreadSheet.hpp"
#include "SkExcelPugiXMLReader.hpp"

#include <SkApi.hpp>
#include <SkFormatCssApi.hpp>

#include <iostream>

using namespace SkRoot;
using namespace SkSpreadSheet;
using namespace SkFormat;

namespace SkExcel {

tString DefaultApplyImagesTestXlsxPath() {
#if defined(__EMSCRIPTEN__)
    return "/home/web_user/Projects/Excel/Budget-familial.xlsx";
#else
    return "/Users/stephaneallez/Projects/Excel/Budget-familial.xlsx";
#endif
}

tInt RunApplyImagesSelfTest(const tString& sXlsxPath) {
    if (sXlsxPath.empty()) {
        std::cerr << "ApplyImages test: empty xlsx path" << std::endl;
        return 1;
    }

    tExcelPugiXMLReader wReader;
    if (!wReader.LoadExcelFile(sXlsxPath)) {
        std::cerr << "ApplyImages test: failed to load " << sXlsxPath << std::endl;
        return 1;
    }
    wReader.ParseWorkbook();

    const std::vector<tExcelImageEntry> wCollected = wReader.CollectImages();
    std::cout << "ApplyImages test: CollectImages=" << wCollected.size()
              << " from " << sXlsxPath << std::endl;

    tApi wApi;
    tFormatCssApi wFormatApi;
    wApi.FormatApi(&wFormatApi);
    wApi.WorkBook("/ApplyImagesTest.sker");
    wApi.DeleteSheet("Sheet1");

    tExcel2SpreadSheet wImporter;
    if (!wImporter.ImportXlsxToApi(sXlsxPath, wApi)) {
        std::cerr << "ApplyImages test: ImportXlsxToApi failed" << std::endl;
        return 1;
    }

    const tString wJson = wApi.WriteJson("/ApplyImagesTest.sker");
    if (wJson.find("SkCellClassImage") == tString::npos) {
        if (wCollected.empty()) {
            std::cout << "ApplyImages test: SKIP (no embeddable images in workbook)" << std::endl;
            return 0;
        }
        std::cerr << "ApplyImages test: JSON missing SkCellClassImage" << std::endl;
        return 1;
    }
    if (wJson.find("data:image/") == tString::npos) {
        std::cerr << "ApplyImages test: JSON missing data:image/ payload" << std::endl;
        return 1;
    }

    std::cout << "ApplyImages test: PASS" << std::endl;
    return 0;
}

} // namespace SkExcel

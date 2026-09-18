//=============================================================================
// SkExcel SpreadSheet Parser
//=============================================================================

#include <SkApplication.hpp>
#include <SkFormatCss.hpp>
#include <SkFormatRoot.hpp>
#include <SkFormatCssApi.hpp>
#include <SkApi.hpp>

#include <zip.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <memory>
#include <chrono>

#include <SkExcelPugiXMLReader.hpp>
#include <SkExcel2SpreadSheet.hpp>
#include <SkExcelApplyImagesSelfTest.hpp>
#include <SkExcelApplyTextBoxesSelfTest.hpp>
#include <SkExcelApplyChartsSelfTest.hpp>
#include <SkExcelFormulaMarkersSelfTest.hpp>
#include <SkSpreadSheet2Excel.hpp>

using namespace SkSpreadSheet;
using namespace SkFormat;
using namespace SkRoot;

// Opt-in post-export RecalculateAll + grid dump (was unconditional and made Release imports look hung).
// Enable with /o:recalculate-after-export or uncomment below for local debugging.
// #define drawcell

void DrawCell(tApi& sApi,tString sTitle,tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
#ifdef drawcell
    cout << "DrawCell "<< sApi.ActiveSheet()->Name() << " "  << sTitle <<  Base10ToAlpha(sColBegin) << sRowBegin << " to " << Base10ToAlpha(sColEnd) << sRowEnd << endl;
    for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
        for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
            tVariant wVariant = sApi.CellValue(wRow, wCol);
            tString wFormula = sApi.Formula(wRow, wCol);
            cout << Base10ToAlpha(wCol) << wRow;
            //cout << "=" << wFormula;
            cout  << ":";
             switch (wVariant.Type()) {
                case tVariantType::t_date: {
                    // tClassDate::FormatString expects tFormatString* (not tFormatStringType).
                    tClassDate wDate(wVariant.Date());
                    tFormatString wFormat;
                    wFormat.FormatType(tFormatStringType::dateddmmyyyyhmm);
                    cout << wDate.FormatString(&wFormat);
                    break;
                }
                default:
                    cout << wVariant;
                    break;
            }
            cout << "\t";
        }
        cout << endl;
    }
#endif
}

tCell* ShowCell(tApi* sApi,tString sRef) {
    tCell* wCell=sApi->Cell(sRef);
    wCell->InternalCalculation();
    cout << wCell->Debug() << endl;
    if (wCell!=nullptr) {
        cout << wCell->StrRef(true) << ":" << wCell->FormulaStr() << "=["<< wCell->Value() << "]" << endl;
        return(wCell);
    } else {
        cout << sRef << " nullptr on sheet " << sApi->ActiveSheet()->Name() << endl;
    }
    
    return(nullptr);
}
void CalculateAll(tString wFileName) {
    SkSpreadSheet::tApi* wApi=new SkSpreadSheet::tApi();
    tFormatCssApi* wFormatApi=new SkFormat::tFormatCssApi();
    wApi->FormatApi(wFormatApi);
    SkExcel::tExcel2SpreadSheet::RegisterJavascriptCellClasses(*wApi);

    tString wPathJson=wFileName;
    // Replace .xlsx extension with .sker
    tSize wDotPos = wPathJson.rfind(".xlsx");
    if (wDotPos != tString::npos) {
        wPathJson = wPathJson.substr(0, wDotPos) + ".sker";
    }
    cout << " Open Sker File " << wPathJson << endl;
  
    tFile wFile(wPathJson);
    if (wFile.Exist()) {
        tString wJsonStr=wFile.LoadString();
        tWorkBook* wWorkBook=wApi->ActiveWorkBook();
        if (wWorkBook!=nullptr) {
            wWorkBook->Clear();
        }
        wApi->ReadJson(wJsonStr);
        wWorkBook=wApi->ActiveWorkBook();
        tBool wOk=false;
        // Show Sheet
        cout << "Sheet --------------------" << endl;
        tVectorSheet wVectorSheet=wWorkBook->VectorPtSheet();
        for (auto wSheet : wVectorSheet) {
            cout << wSheet->Name() << endl;
            wOk=wApi->ActiveSheet(wSheet->Name());
        }
        cout << "--------------------------" << endl;
        cout << "Nombre de feuilles: " << wVectorSheet.size() << endl;
        // Get page by Index
        //wOk=wApi->ActiveSheet(wWorkBook->Sheet(0)->Name());
        if (wOk) {
            cout << "Recalculate All " << endl;
            // Performance measurement
            auto wStartTime = std::chrono::high_resolution_clock::now();
            wWorkBook->RecalculateAll();
            auto wEndTime = std::chrono::high_resolution_clock::now();
            auto wDuration = std::chrono::duration_cast<std::chrono::milliseconds>(wEndTime - wStartTime);
    #ifdef _DEBUG
        
            //cout << wWorkBook->Debug() << endl;
    #endif
            cout << "=== Performance: RecalculateAll ===" << endl;
            cout << "Total time: " << wDuration.count() << " ms" << endl;
            // RecalculateAll already processes every sheet; print one sample cell per sheet (B8 = first calendar row).
            cout << "Echantillon B8 par feuille (apres recalcul) :" << endl;
            for (auto wSheet : wVectorSheet) {
                if (!wApi->ActiveSheet(wSheet->Name())) {
                    continue;
                }
                tVariant wB8 = wApi->CellValue(8, 2);
                cout << "  " << wSheet->Name() << "  B8=" << wB8 << endl;
            }
           // wOk=wApi->ActiveSheet("Décaissements (hors compte d,,,");
           //wOk=wApi->ActiveSheet("JANVIER");
            tVectorSheet wVectorSheet=wWorkBook->VectorPtSheet();
            for(auto wSheet : wVectorSheet) {
                wWorkBook->ActiveSheet(wSheet->Name());
                cout << "Sheet "  << wApi->ActiveSheet()->Name()<< "-------------------------------------" << endl;
                DrawCell(*wApi,
                         wSheet->Name(),
                         1,
                         1,
                         min(wSheet->LastRow(),25),
                         min(wSheet->LastCol(),10));
            }
           /*
           cout << "Stress Loan schedule" << endl;
           for(tInt w=12;w<=36;w++) {
               wApi->UndoCellValue("E8", tVariant(w));
               tVariant wResult=wApi->Cell("H14")->Value();
               cout << "E8->" << w << " H14=" << wResult << endl;
           }
            
            cout << "Grille detail (feuille active): " << wApi->ActiveSheet()->Name() << endl;
            DrawCell(*wApi,"Result ",1,1,23,10);
            //DrawCell(*wApi,"Result 356..365",356,1,365,10);
            */
            
            /*
#ifdef _DEBUGSK
            ShowCell(wApi,"D7");
            ShowCell(wApi,"D8");
#endif
        
            
            tBool wOk=wWorkBook->ActiveSheet("_$$");
            if (wApi->ActiveSheet()!=nullptr) {
                DrawCell(*wApi,"Result ",1,1,10,2);
            }
            */
#ifdef _DEBUGSK
            //cout << wWorkBook->Debug();
            cout << wApi->JsonRangeNamed() << endl;
            cout << endl;
            cout << wApi->JsonRangeData()  << endl;
#endif
        }
        
   } else {
       cerr << wPathJson << " don't exist !" << endl;
   }

    delete(wApi);
    delete(wFormatApi);
}

tInt ExportFile(tString sUri,
                tString wFileName,
                tBool sMaterializeIndirectFrenchL1C1CellRefs = true,
                tBool sRecalculateAtImport = false) {
    // RAII owners: if ImportXlsxToApi throws on a malformed file (this target links with C++
    // exceptions enabled), these are still destroyed during unwinding — no leak. Declaration
    // order matters: wApi is declared AFTER wFormatApi so it is destroyed FIRST (locals unwind in
    // reverse), preserving the original delete(wApi) -> delete(wFormatApi) order, since tApi may
    // reference its FormatApi during teardown.
    std::unique_ptr<SkFormat::tFormatCssApi> wFormatApi(new SkFormat::tFormatCssApi());
    std::unique_ptr<SkSpreadSheet::tApi> wApi(new SkSpreadSheet::tApi());
    wApi->FormatApi(wFormatApi.get());
    // Workbook URI from /u: (e.g. /BudgetF.sker) for WriteJson; standalone runs pass only /f: and use xlsx path.
    const tString wBookUri = (!sUri.empty()) ? sUri : wFileName;
    wApi->WorkBook(wBookUri);
    wApi->DeleteSheet("Sheet1");
    SkExcel::tExcel2SpreadSheet wImporter2;
    wImporter2.SetMaterializeIndirectFrenchL1C1CellRefs(sMaterializeIndirectFrenchL1C1CellRefs);
    wImporter2.SetRecalculateAtImport(sRecalculateAtImport);
    const tBool wOk = wImporter2.ImportXlsxToApi(wFileName, *wApi);
    if (!wOk) {
        std::cerr << "SkExcel: ImportXlsxToApi failed for " << wFileName << std::endl;
        return 1;
    }
    return 0;
}

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
extern "C" {
// In-process entry point for a long-lived Node server: convert one xlsx to .sker on the
// already-instantiated wasm module, avoiding a process spawn + module instantiate per request.
//
// Isolation & safety (critical for a reused instance): SkExcel links with C++ exceptions enabled,
// so a malformed/corrupt file can throw mid-import. We therefore (1) catch every exception so it
// never escapes to JS / aborts the module, and (2) ALWAYS tear down the global singletons
// (tFormatRoot format registry + spreadsheet container) after each call. Without that teardown a
// partial/failed import would leave orphaned formats in the shared tFormatRoot and corrupt the
// NEXT conversion ("casser la gestion des formats"); DoneSpreadSheet()/DoneFormatRoot() reset the
// statics to nullptr and Instance() lazily recreates a clean state on the next call. Net effect:
// every request gets an isolated, freshly-initialized subsystem — no cross-request contamination
// and no unbounded memory growth — while still avoiding the process spawn overhead.
//
// Params mirror the CLI: sUri = /u: workbook URI (may be empty), sPath = /f: xlsx path.
// Returns 0 = ok, 1 = import failed, 2 = missing path, 3 = std::exception, 4 = unknown exception.
EMSCRIPTEN_KEEPALIVE
int skexcel_convert(const char* sUri, const char* sPath) {
    if (sPath == nullptr || sPath[0] == '\0') {
        return 2;
    }
    int wRc;
    try {
        tApplication::Instance()->Locale("us");
        wRc = ExportFile(sUri ? tString(sUri) : tString(""), tString(sPath));
    } catch (const std::exception& sError) {
        std::cerr << "SkExcel: exception while converting '" << sPath << "': " << sError.what() << std::endl;
        wRc = 3;
    } catch (...) {
        std::cerr << "SkExcel: unknown exception while converting '" << sPath << "'" << std::endl;
        wRc = 4;
    }
    // Reset shared state so a partial/failed import cannot leak into the next call.
    DoneSpreadSheet();
    DoneFormatRoot();
    return wRc;
}
}
#endif

// Derive .xlsx path from .sker input (same logic as SaveOutputFile, reversed).
static tString SkerPathToXlsxPath(const tString& sSkerPath) {
    tString wOutPath = sSkerPath;
    tSize wSlash = wOutPath.find_last_of("/\\");
    tSize wDot = wOutPath.find_last_of('.');
    if (wDot != tString::npos && (wSlash == tString::npos || wDot > wSlash)) {
        wOutPath = wOutPath.substr(0, wDot) + ".xlsx";
    } else {
        wOutPath += ".xlsx";
    }
    return wOutPath;
}

tInt ExportSkerToXlsx(const tString& sSkerPath, const tString& sOutputPath) {
    const tString wOutPath = !sOutputPath.empty() ? sOutputPath : SkerPathToXlsxPath(sSkerPath);

    SkSpreadSheet::tApi* wApi = new SkSpreadSheet::tApi();
    tFormatCssApi* wFormatApi = new SkFormat::tFormatCssApi();
    wApi->FormatApi(wFormatApi);
    SkExcel::tExcel2SpreadSheet::RegisterJavascriptCellClasses(*wApi);

    tFile wFile(sSkerPath);
    if (!wFile.Exist()) {
        std::cerr << "SkExcel: sker file not found: " << sSkerPath << std::endl;
        delete wApi;
        delete wFormatApi;
        return 1;
    }

    tString wJson = wFile.LoadString();
    wApi->WorkBook(sSkerPath);
    if (tWorkBook* wWorkBook = wApi->ActiveWorkBook()) {
        wWorkBook->Clear();
    }
    wApi->ReadJson(wJson);

    if (tWorkBook* wWorkBook = wApi->ActiveWorkBook()) {
        if (SkSpreadSheet::tRangeNamedContainer* wNamed = wWorkBook->RangeNamedContainer();
            wNamed != nullptr && wNamed->AllNames().empty()) {
            std::cerr << "SkExcel: warning — no named ranges in .sker; "
                         "formulas using HeureDeDébut/Intervalle/etc. will show #NOM? in Excel. "
                         "Use a .sker exported with namedranges metadata." << std::endl;
        }
    }

    const tBool wOk = SkExcel::ExportApiToXlsx(*wApi, wOutPath);

    delete wApi;
    delete wFormatApi;

    if (!wOk) {
        std::cerr << "SkExcel: ExportApiToXlsx failed for " << sSkerPath << " -> " << wOutPath << std::endl;
        return 1;
    }
    std::cout << "Exported " << sSkerPath << " -> " << wOutPath << std::endl;
    return 0;
}

// Main function
tInt main(tInt sArgc, char** sArgv) {
    tString wUri="";
    tString wFileName="";
    tString wImagesJsonPath="";
    tBool wRunApplyImagesTest = false;
    tBool wRunApplyTextBoxesTest = false;
    tBool wRunApplyChartsTest = false;
    tBool wRunFormulaMarkersTest = false;
    tBool wMaterializeIndirectFrenchL1C1CellRefs = true;
    tBool wRecalculateAtImport = false;
    tBool wRecalculateAfterExport = false;
    tBool wModeSkerToXlsx = false;
    tString wXlsxOutputPath = "";

    for (tInt wInd = 1; wInd < sArgc; wInd++) {
        tString wArg = sArgv[wInd];
        if (wArg == "/m:sker2xlsx") {
            wModeSkerToXlsx = true;
            continue;
        }
        if (wArg == "/o:recalculate-at-import") {
            wRecalculateAtImport = true;
            continue;
        }
        if (wArg == "/o:no-recalculate-at-import") {
            wRecalculateAtImport = false;
            continue;
        }
        if (wArg == "/o:recalculate-after-export") {
            wRecalculateAfterExport = true;
            continue;
        }
        if (wArg == "/o:no-materialize-indirect-l1c1") {
            wMaterializeIndirectFrenchL1C1CellRefs = false;
            continue;
        }
        if (wArg == "/o:materialize-indirect-l1c1") {
            wMaterializeIndirectFrenchL1C1CellRefs = true;
            continue;
        }
        if (wArg == "/t:apply-images") {
            wRunApplyImagesTest = true;
            continue;
        }
        if (wArg == "/t:apply-textboxes") {
            wRunApplyTextBoxesTest = true;
            continue;
        }
        if (wArg == "/t:apply-charts") {
            wRunApplyChartsTest = true;
            continue;
        }
        if (wArg == "/t:formula-markers") {
            wRunFormulaMarkersTest = true;
            continue;
        }
        if (wArg.size() < 3 || wArg[0] != '/') {
            continue;
        }
        tChar wArgChar = wArg[1];
        switch (wArgChar) {
            case 'u' :  wUri=wArg.substr(3, wArg.length() - 3); break;
            case 'f' :  wFileName=wArg.substr(3, wArg.length() - 3); break;
            case 'i' :  wImagesJsonPath=wArg.substr(3, wArg.length() - 3); break;
            case 'x' :  wXlsxOutputPath=wArg.substr(3, wArg.length() - 3); break;
        }
    }

    if (wFileName.empty() && wRunApplyImagesTest == false && wRunApplyTextBoxesTest == false &&
        wRunApplyChartsTest == false && wRunFormulaMarkersTest == false) {
        std::cout << "SkExcel 2026" << endl;
        std::cout << "------------" << endl;
    }

    if (wRunFormulaMarkersTest) {
        tApplication::Instance()->Locale("us");
        const tInt wCode = SkExcel::RunFormulaMarkersSelfTest();
        DoneSpreadSheet();
        DoneFormatRoot();
        return wCode;
    }

    if (wModeSkerToXlsx) {
        if (wFileName.empty()) {
            std::cout << "SkExcel sker -> xlsx" << endl;
            std::cout << "  /m:sker2xlsx" << endl;
            std::cout << "  /f: sker input file (required)" << endl;
            std::cout << "  /x: xlsx output file (optional; default: same path with .xlsx)" << endl;
            return 1;
        }
        tApplication::Instance()->Locale("us");
        const tInt wCode = ExportSkerToXlsx(wFileName, wXlsxOutputPath);
        DoneSpreadSheet();
        DoneFormatRoot();
        return wCode;
    }
   
    if (wRunApplyImagesTest) {
        tApplication::Instance()->Locale("us");
        const tString wTestXlsx = !wFileName.empty() ? wFileName : SkExcel::DefaultApplyImagesTestXlsxPath();
        const tInt wCode = SkExcel::RunApplyImagesSelfTest(wTestXlsx);
        DoneSpreadSheet();
        DoneFormatRoot();
        return wCode;
    }

    if (wRunApplyTextBoxesTest) {
        tApplication::Instance()->Locale("us");
        const tString wTestXlsx = !wFileName.empty() ? wFileName : SkExcel::DefaultApplyTextBoxesTestXlsxPath();
        const tInt wCode = SkExcel::RunApplyTextBoxesSelfTest(wTestXlsx);
        DoneSpreadSheet();
        DoneFormatRoot();
        return wCode;
    }

    if (wRunApplyChartsTest) {
        tApplication::Instance()->Locale("us");
        const tString wTestXlsx = !wFileName.empty() ? wFileName : SkExcel::DefaultApplyChartsTestXlsxPath();
        const tInt wCode = SkExcel::RunApplyChartsSelfTest(wTestXlsx);
        DoneSpreadSheet();
        DoneFormatRoot();
        return wCode;
    }
    
    // Pass the input with /f: (and optional /u:). Do not hardcode a local path here.
    if (wFileName=="") {
        std::cout << " Enter SkExcel /f:xxxx or d:xxxx" << endl;
        std::cout << "  /m:sker2xlsx  export .sker to .xlsx (use with /f: and optional /x:)" << endl;
        std::cout << "  /u: uri" << endl;
        std::cout << "  /f: Filename" << endl;
        std::cout << "  /x: xlsx output (sker2xlsx mode only)" << endl;
        std::cout << "  /i: images.json (optional Base64 export)" << endl;
        std::cout << "  /t:apply-images (optional; use with /f: or default test xlsx)" << endl;
        std::cout << "  /t:formula-markers (self-test: OOXML _xlfn/_xlpm stripping + array constants; no file)" << endl;
        std::cout << "  /o:no-materialize-indirect-l1c1 (optional; keep INDIRECT LC(-1) literals)" << endl;
        std::cout << "  /o:recalculate-at-import (optional; slow — full RecalculateAll instead of OOXML cache)" << endl;
        std::cout << "  /o:recalculate-after-export (optional; load .sker + RecalculateAll after import)" << endl;
        std::cout << endl;
        return(1);
    }

    if (!wImagesJsonPath.empty()) {
        tExcelPugiXMLReader wImageReader;
        if (!wImageReader.LoadExcelFile(wFileName)) {
            std::cerr << "Failed to load: " << wFileName << std::endl;
            return 1;
        }
        wImageReader.ParseWorkbook();
        const tInt wExported = wImageReader.SaveImagesJsonToFile(wImagesJsonPath);
        if (wExported < 0) {
            std::cerr << "Failed to write images JSON: " << wImagesJsonPath << std::endl;
            return 1;
        }
        std::cout << "Exported " << wExported << " image(s) to " << wImagesJsonPath << std::endl;
    }
    tApplication::Instance()->Locale("us");

    const tInt wExportCode = ExportFile(
        wUri, wFileName, wMaterializeIndirectFrenchL1C1CellRefs, wRecalculateAtImport);
#if defined(drawcell)
    if (wExportCode == 0) {
        CalculateAll(wFileName);
    }
#else
    if (wExportCode == 0 && wRecalculateAfterExport) {
        CalculateAll(wFileName);
    }
#endif
    DoneSpreadSheet();
    DoneFormatRoot();
    
#ifdef debuginfo
    cout << "SkExcel end..";
#endif
    return wExportCode;
}

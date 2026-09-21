//=============================================================================
// SkPressure2 
// Test Pressure
//=============================================================================
#include "../include/SkPressure3.hpp"

#include <string>
#include <vector>

// Append sValue in base 10. std::stringstream costs a locale + streambuf setup per cell,
// which dominates the fill loop on 11.5M cells and hides the engine cost being measured
// (the Rust counterpart builds its formulas with format!).
static void AppendNumber(tString& sOut, tInt sValue) {
    tChar wDigits[24];
    tInt wLength = 0;
    if (sValue < 0) {
        sOut.push_back('-');
        sValue = -sValue;
    }
    do {
        wDigits[wLength++] = tChar('0' + (sValue % 10));
        sValue /= 10;
    } while (sValue > 0);
    while (wLength > 0) {
        sOut.push_back(wDigits[--wLength]);
    }
}

void Pressure3(tApi*& sApi) {
    auto thousands = std::make_unique<separate_thousands>();
    std::cout.imbue(std::locale(std::cout.getloc(), thousands.release()));
    tCell* wCell;
    cout << "Start timer ensure cells with formula and SUM() " << (wDynamicNbRow * wDynamicNbCol) << " Cells on " << wDynamicNbRow << "  rows " << wDynamicNbCol << " cols." << endl;
    tApplication::Instance()->TimerStart();
    /*tCell* wCellDepend =*/ sApi->EnsureCell(0, 0);

    for (tInt wCol = 1; wCol <= wDynamicNbCol; wCol++) {
        sApi->ActiveSheet()->EnsureCol(wCol);
    }
    for (tInt wRow = 1; wRow <= wDynamicNbRow; wRow++) {
        sApi->ActiveSheet()->EnsureRow(wRow);
    }
    
    // Column letters never change during the fill: resolve them once instead of per cell.
    std::vector<tString> wColAlpha(wDynamicNbCol + 1);
    for (tInt wCol = 1; wCol <= wDynamicNbCol; wCol++) {
        wColAlpha[wCol] = Base10ToAlpha(wCol);
    }

    tString wFormula;
    wFormula.reserve(64);
    for (tInt wCol = 1; wCol <= wDynamicNbCol; wCol++) {
        if (wCol > 1) {
            wCell = sApi->EnsureCell(1, wCol);
            wFormula.assign(wColAlpha[wCol - 1]);
            AppendNumber(wFormula, wDynamicNbRow - 1);
            wFormula.append("+1");
            sApi->CompilCell(wCell, wFormula.c_str());
        }
    }

    for (tInt wRow = 2; wRow <= wDynamicNbRow; wRow++) {
        for (tInt wCol = 1; wCol <= wDynamicNbCol; wCol++) {
            wCell = sApi->EnsureCell(wRow, wCol);
            wFormula.clear();
            if ((wRow == wDynamicNbRow) && (wCol != wDynamicNbCol)) {
                // Bottom row totals the whole column above it.
                wFormula.append("SUM(").append(wColAlpha[wCol]).append("1:").append(wColAlpha[wCol]);
                AppendNumber(wFormula, wRow - 1);
                wFormula.push_back(')');
            }
            else if ((wRow == wDynamicNbRow) || (wCol == wDynamicNbCol)) {
                // Last column (and the bottom-right corner) totals its own row.
                wFormula.append("SUM(").append(wColAlpha[1]);
                AppendNumber(wFormula, wRow);
                wFormula.push_back(':');
                wFormula.append(wColAlpha[wDynamicNbCol - 1]);
                AppendNumber(wFormula, wRow);
                wFormula.push_back(')');
            }
            else {
                wFormula.assign(wColAlpha[wCol]);
                AppendNumber(wFormula, wRow - 1);
                wFormula.append("+1");
            }
            wCell->Path(0);
            sApi->CompilCell(wCell, wFormula.c_str());
        }
        if (wRow % wDynamicModulo == 0) cout << wRow * wDynamicNbCol << " Cells " << ElapsedSec() << " s" << endl;
    }
    tSheet* wSheet = wCell->Sheet();
    SetConsoleColor(SkColorConsole::lightblue);
    MemoryUses();
    cout << "Nb Cell=" << wSheet->NbCell() << endl;
    cout << "ColRow allocator  Memory Size=" << wSheet->MemoryColRowSize() << endl;
    cout << "Cell allocator Memory Size=" << wSheet->MemoryCellSize() << endl;
    cout << "Range allocator Memory Size=" << wSheet->MemoryRangeSize() << endl;
    cout << "Total allocator Memory Size " << wSheet->MemoryColRowSize() + wSheet->MemoryCellSize() + wSheet->MemoryRangeSize() << endl;
    cout << "Nb Shared String " << tApplication::Instance()->NbSharedString() << endl;
    cout << "SkSharedFormulaContainer::Size() " << tSpreadSheetContainer::Instance()->NbSharedFormula() << endl;

    SetConsoleColor(SkColorConsole::normal);

    cout << "Start timer calculate cell with Rect.. " << endl;
    tApplication::Instance()->TimerStart();
    tTempoRect* wTempoRect = new tTempoRect(1, 1, 10, 10);
    sApi->ActiveSheet()->Calculate(wTempoRect);
    delete wTempoRect;
    cout << "Elapsed Time " << ElapsedSec() << " s" << endl;
    tCell* wCellResult = sApi->EnsureCell(wDynamicNbRow, wDynamicNbCol);
    cout << "   Result  " << wCellResult->StrRef() <<"=" << wCellResult->FormulaStr() << ":" << wCellResult->Value() << endl;

    cout << endl;
    SetConsoleColor(SkColorConsole::lightred);
    DrawCell(sApi, 1, 1, 3, wDynamicNbCol);
    cout << endl;
    DrawCell(sApi, wDynamicNbRow - 5, 1, wDynamicNbRow, wDynamicNbCol);

    SetConsoleColor(SkColorConsole::normal);
    
    if (wDeleteRow) {
        cout << "Start timer Delete Row  2,2 " << endl;
        tApplication::Instance()->TimerStart();
        sApi->UndoDeleteRow(2, 2);
        cout << "Elapsed Time " << ElapsedSec() << " s" << endl;
#ifdef _DEBUG   
        cout << endl;
        SetConsoleColor(SkColorConsole::lightred);
        DrawCell(sApi, 1, 1, wDynamicNbRow, wDynamicNbCol);
#else
        cout << endl;
        SetConsoleColor(SkColorConsole::lightred);
        DrawCell(sApi, 1, 1, 3, wDynamicNbCol);
        cout << endl;
        DrawCell(sApi, wDynamicNbRow - 5, 1, wDynamicNbRow, wDynamicNbCol);
#endif
        SetConsoleColor(SkColorConsole::normal);
        MemoryUses();
        cout << "Start timer Undo Delete Row  2,2 " << endl;
        tApplication::Instance()->TimerStart();
        sApi->Undo();
        cout << "Elapsed Time " << ElapsedSec() << " s" << endl;
#ifdef _DEBUG   
        cout << endl;
        SetConsoleColor(SkColorConsole::lightred);
        DrawCell(sApi, 1, 1, wDynamicNbRow, wDynamicNbCol);
#else
        cout << endl;
        SetConsoleColor(SkColorConsole::lightred);
        DrawCell(sApi, 1, 1, 3, wDynamicNbCol);
        cout << endl;
        DrawCell(sApi, wDynamicNbRow - 5, 1, wDynamicNbRow, wDynamicNbCol);
#endif
    }

    SetConsoleColor(SkColorConsole::normal);
    MemoryUses();
    cout << "Elapsed Time " << ElapsedSec() << " s" << endl;
    /*
    tApplication::Instance()->TimerStart();
    tInt wLastTime = 0;
    cout << "Start timer calculate cell A1... " << endl;
    for (tInt wCol = 1; wCol <= 1;  wCol++) {
        tApplication::Instance()->TimerStart();
        wCell = sApi->EnsureCell(1, wCol);
        wCell->Value(wCol);
        wCell->Path(0);
        wLastTime = tApplication::Instance()->TimerElapsed();
        wCell->Calculation();

        cout << "Calcul col " << Base10ToAlpha(wCol) << "    Elapsed time " << ElapsedSec(tApplication::Instance()->TimerElapsed() - wLastTime) << " s" << endl;
        tCell* wCellResult = sApi->EnsureCell(wDynamicNbRow, wDynamicNbCol);
        cout << "   Result  " << wCellResult->StrRef() << "=" << wCellResult->FormulaStr() << ":" << wCellResult->Value() << endl;
    }

    SetConsoleColor(SkColorConsole::lightblue);
    MemoryUses();
    cout << "Nb Cell=" << wSheet->NbCell() << endl;
    cout << "ColRow allocator  Memory Size=" << wSheet->MemoryColRowSize() << endl;
    cout << "Cell allocator Memory Size=" << wSheet->MemoryCellSize() << endl;
    cout << "Range allocator Memory Size=" << wSheet->MemoryRangeSize() << endl;
    cout << "Total allocator Memory Size " << wSheet->MemoryColRowSize() + wSheet->MemoryCellSize() + wSheet->MemoryRangeSize() << endl;
    cout << "Nb Shared String " << tApplication::Instance()->NbSharedString() << endl;
    cout << "SkSharedFormulaContainer::Size() " << tSpreadSheetContainer::Instance()->NbSharedFormula() << endl;
#ifdef _DEBUG   
    cout << endl;
    SetConsoleColor(SkColorConsole::lightred);
    DrawCell(sApi, 1, 1, wDynamicNbRow, wDynamicNbCol);
#else
    cout << endl;
    SetConsoleColor(SkColorConsole::lightred);

    DrawCell(sApi, 1, 1, 3, wDynamicNbCol);
    cout << endl;
    MemoryUses();
    DrawCell(sApi, wDynamicNbRow - 5, 1, wDynamicNbRow, wDynamicNbCol);
#endif
    */
    SetConsoleColor(SkColorConsole::normal);
    if (wJsonTest) {
        cout << "Save Json " << endl;
        tApplication::Instance()->TimerStart();

        tString wJson = sApi->WriteJson("wwww.Sker.fr/SkPressure");
        tFile* wFile = new tFile("Test.json");
        wFile->SaveString(wJson);
        cout << "Elapsed Time " << ElapsedSec() << " s" << endl;
        delete(wFile);
        wJson = "";
        cout << "Before Api clear -> "; MemoryUses();
        tApplication::Instance()->TimerStart();
        delete(sApi);
        sApi = new tApi();
        sApi->IsUndoActif(true);
        cout << "After Api clear  -> "; MemoryUses();
        cout << "Elapsed Time " << ElapsedSec() << " s" << endl;

        cout << "Read Json " << endl;
        tApplication::Instance()->TimerStart();

        wFile = new tFile("Test.json");
        tString wJsonRead = wFile->LoadString();
        //cout << wJsonRead << endl;
        sApi->ReadJson(wJsonRead);
        cout << "Elapsed Time " << ElapsedSec() << " s" << endl;
        delete(wFile);
        cout << endl;
        SetConsoleColor(SkColorConsole::lightred);

        DrawCell(sApi, 1, 1, 3, wDynamicNbCol);
        cout << endl;
        DrawCell(sApi, wDynamicNbRow - 5, 1, wDynamicNbRow, wDynamicNbCol);
    }
    SetConsoleColor(SkColorConsole::normal);

}

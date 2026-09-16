//=============================================================================
// SkPressure2 
// Test Pressure
//=============================================================================
#include "../include/SkPressure2.hpp"

void Pressure2(tApi* sApi) {
    auto thousands = std::make_unique<separate_thousands>();
    std::cout.imbue(std::locale(std::cout.getloc(), thousands.release()));
    tCell* wCell;
    cout << "Start timer ensure cells with formula " << (wDynamicNbRow * wDynamicNbCol) << " Cells on " << wDynamicNbRow << " rows " << wDynamicNbCol << " cols." << endl;
    tApplication::Instance()->TimerStart();
    /*tCell* wCellDepend =*/ sApi->EnsureCell(0, 0);

    for (tInt wCol = 1; wCol <= wDynamicNbCol; wCol++) {
        sApi->ActiveSheet()->EnsureCol(wCol);
    }
    for (tInt wRow = 1; wRow <= wDynamicNbRow; wRow++) {
        sApi->ActiveSheet()->EnsureRow(wRow);
    }
  
    for (tInt wCol = 2; wCol <= wDynamicNbCol; wCol++) {
        wCell = sApi->EnsureCell(1, wCol);
        tStringStream wStream;
        wStream << Base10ToAlpha(wCol-1) << wDynamicNbRow << "+1";
        sApi->CompilCell(wCell, wStream.str().c_str());
    }

    for (tInt wRow = 2; wRow <= wDynamicNbRow; wRow++) {
        for (tInt wCol = 1; wCol <= wDynamicNbCol; wCol++) {
            wCell = sApi->EnsureCell(wRow, wCol);
            tStringStream wStream;
            wStream << Base10ToAlpha(wCol) << wRow - 1 << "+1";
            sApi->CompilCell(wCell, wStream.str().c_str());
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


    cout << "Elapsed Time " << ElapsedSec() << " s" << endl;
    tApplication::Instance()->TimerStart();
    tInt wLastTime = 0;
    cout << "Start timer calculate cell A1.. " << endl;
    wCell=sApi->EnsureCell(1, 1);
    wCell->Calculation();

    cout << "Calcul Elapsed time " << ElapsedSec(tApplication::Instance()->TimerElapsed() - wLastTime) << " s" << endl;
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
    DrawCell(sApi, wDynamicNbRow - 5, 1, wDynamicNbRow, wDynamicNbCol);
#endif
    SetConsoleColor(SkColorConsole::normal);
}

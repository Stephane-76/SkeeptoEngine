//=============================================================================
// SkPressure1 
// Test Pressure
//=============================================================================
#include "../include/SkPressure1.hpp"

void Pressure1(tApi* sApi) {
    // Empty Cell
    auto thousands = std::make_unique<separate_thousands>();
    std::cout.imbue(std::locale(std::cout.getloc(), thousands.release()));
    tCell* wCell;
    cout << "Start timer ensure empty cell " << (wDynamicNbRow * wDynamicNbCol) << " Cells on " << wDynamicNbRow << " rows " << wDynamicNbCol << " cols." << endl;
    tApplication::Instance()->TimerStart();
    for (tInt wRow = 1; wRow <= wDynamicNbRow; wRow++) {
        for (tInt wCol = 1; wCol <= wDynamicNbCol; wCol++) {
            if ((wRow == 40) && (wCol == 1)) {
                //int a = 1;
            }
            if ((wRow == 41) && (wCol == 1)) {
                //int a = 1;
            }
            wCell = sApi->EnsureCell(wRow, wCol);
        }
        if (wRow % wDynamicModulo == 0) cout << wRow * wDynamicNbCol << " Cells " << ElapsedSec() << " s" << endl;
    }
    tSheet* wSheet = wCell->Sheet();

    cout << "Elapsed Time " << ElapsedSec() << " s" << endl;

    SetConsoleColor(SkColorConsole::lightblue);
    MemoryUses();
    cout << "Nb Cell=" << wSheet->NbCell() << endl;
    cout << "ColRow allocator  Memory Size=" << wSheet->MemoryColRowSize() << endl;
    cout << "Cell allocator Memory Size=" << wSheet->MemoryCellSize() << endl;
    cout << "Range allocator Memory Size=" << wSheet->MemoryRangeSize() << endl;
    cout << "Total allocator Memory Size " << wSheet->MemoryColRowSize() + wSheet->MemoryCellSize() + wSheet->MemoryRangeSize() << endl;
    cout << "Nb Shared String " << tApplication::Instance()->NbSharedString() << endl;
    cout << "SkSharedFormulaContainer::Size() " << tSpreadSheetContainer::Instance()->NbSharedFormula() <<  endl;
    SetConsoleColor(SkColorConsole::normal);

}

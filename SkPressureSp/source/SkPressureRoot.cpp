//=============================================================================
// SkPressureRoot 
// Test Pressure
//=============================================================================
#include "../include/SkPressureRoot.hpp"
#include <iomanip>
#include <sstream>
#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/task.h>
#include <mach/task_info.h>
#include <libproc.h>
#include <unistd.h>
#endif



void DrawCell(tApi* sApi,tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
		for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
			tVariant wVariant = sApi->CellValue(wRow, wCol);
			tCell* wCell = sApi->Cell(wRow, wCol);
			tString wFormula = "";
			if (wCell != nullptr) {
				wFormula = wCell->FormulaStr();
			}
			tStringStream wCellStream;
			wCellStream << Base10ToAlpha(wCol) << wRow;
			cout << wCellStream.str() << "=" << wVariant << ";" << wFormula << "\t";
		}
		cout << endl;
	}
}

void SetConsoleColor(SkColorConsole sColor) {
#if _WIN64
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	switch (sColor)
	{
	case SkColorConsole::bluelow:
		SetConsoleTextAttribute(hConsole, 1);
		break;
	case SkColorConsole::green:
		SetConsoleTextAttribute(hConsole, 2);
		break;
	case SkColorConsole::blue:
		SetConsoleTextAttribute(hConsole, 3);
		break;
	case SkColorConsole::red:
		SetConsoleTextAttribute(hConsole, 4);
		break;
	case SkColorConsole::purple:
		SetConsoleTextAttribute(hConsole, 5);
		break;
	case SkColorConsole::brow:
		SetConsoleTextAttribute(hConsole, 6);
		break;
	case SkColorConsole::normal:
		SetConsoleTextAttribute(hConsole, 7);
		break;
	case SkColorConsole::lightgreen:
		SetConsoleTextAttribute(hConsole, 10);
		break;
	case SkColorConsole::lightblue:
		SetConsoleTextAttribute(hConsole, 11);
		break;
	case SkColorConsole::lightred:
		SetConsoleTextAttribute(hConsole, 12);
		break;
	default:
		break;
	}
#endif
}
void PrintSize() {
	cout << endl;
	cout << "Size of elem spreadsheet =======================================================" << endl;
	cout << "size of tInt " << sizeof(tInt) << endl;
	cout << "size of tShort " << sizeof(tShort) << endl;
	cout << "size of tLong " << sizeof(tLong) << endl;
	cout << "size of tLongLong " << sizeof(tLongLong) << endl;
	cout << "size of tFloat " << sizeof(tFloat) << endl;
	cout << "size of tDouble " << sizeof(tDouble) << endl;
	cout << "size of tDate " << sizeof(tDate) << endl;
	
	cout << "size of tChar " << sizeof(tChar) << endl;
	cout << "size of tString " << sizeof(tString) << endl;

	cout << "size of t16Char " << sizeof(t16Char) << endl;
	cout << "size of t16String " << sizeof(t16String) << endl;

	
	
	cout << "size of tBool " << sizeof(tBool) << endl;

	cout << "size of void* " << sizeof(void*) << endl;
	cout << "size of short int " << sizeof(short int) << endl;

	cout << "size of tVariantType " << sizeof(tVariantType) << endl;

	cout << "size of tVariant_Union " << sizeof(tVariant_Union) << endl;
	cout << "size of tVariant " << sizeof(tVariant) << endl;
	cout << "size of tClassVectorContainer<tItem>" << sizeof(tClassVectorContainer<tItem>) << endl;
	cout << "size of tClassUnorderedContainer<tItem>" << sizeof(tClassUnorderedContainer<tItem>) << endl;
	cout << "Size of tItem " << sizeof(tItem) << endl;
	cout << "size of tCell " << sizeof(tCell) << endl;
	cout << "size of tCell 1024 " << sizeof(tCell[1024]) << endl;
	cout << "size of tRange " << sizeof(tRange) << endl;

	cout << "size of tColRow " << sizeof(tColRow) << endl;
	cout << "size of tFormula " << sizeof(tFormula) << endl;
	cout << "size of vector<tItem>" << sizeof(vector<tItem*>) << endl;
	cout << "size of tSharedFormula " << sizeof(tSharedFormula) << endl;
	cout << "Size of tColRowCellRange " << sizeof(tColRowCellRange) << endl;

	cout << "Test offset " << endl;
	tItem wItem;
	cout << "Size of tItem " << sizeof(tItem) << endl;
	wItem.Offsetof();
	tCell wCellO;
	cout << "size of tCell " << sizeof(tCell) << endl;
	wCellO.Offsetof();
	cout << "size of tRange " << sizeof(tRange) << endl;
	tRange wRange;
	wRange.Offsetof();

	cout << "size of tAllocatorTrack " << sizeof(tAllocatorTrack < tCell, tInt, 1024>) << endl;
}

void StreamLocale() {

}

tString ElapsedSec(clock_t sTicks) {
	// Classic locale on purpose: cout is imbued with separate_thousands, which would group
	// the integer part with '.' and make "16.854" unreadable next to the decimal point.
	tStringStream wStream;
	wStream.imbue(std::locale::classic());
	wStream << std::fixed << std::setprecision(3)
	        << (static_cast<tDouble>(sTicks) / static_cast<tDouble>(CLOCKS_PER_SEC));
	return (wStream.str());
}

tString ElapsedSec() {
	return (ElapsedSec(tApplication::Instance()->TimerElapsed()));
}

void MemoryUses() {

	uint64_t currentUsedRAM(0);
#ifdef _WIN64
	PROCESS_MEMORY_COUNTERS info;
	GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
	currentUsedRAM = info.WorkingSetSize;
#elif defined(__APPLE__)
	// Prefer TASK_VM_INFO: MACH_TASK_BASIC_INFO often fails on current Xcode SDKs
	// and left Pressure printing "Current RAM used: 0".
	task_vm_info_data_t vm{};
	mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
	if (task_info(mach_task_self(), TASK_VM_INFO, (task_info_t)&vm, &count) == KERN_SUCCESS) {
		currentUsedRAM = vm.resident_size;
	} else {
		struct proc_taskinfo pti{};
		if (proc_pidinfo(getpid(), PROC_PIDTASKINFO, 0, &pti, sizeof(pti)) == (int)sizeof(pti)) {
			currentUsedRAM = pti.pti_resident_size;
		}
	}
#endif
	// Process RSS in GiB (`Go`), three decimal places (same as rust/SkPressure).
	const tDouble wGo = static_cast<tDouble>(currentUsedRAM) / (1024.0 * 1024.0 * 1024.0);
	std::ostringstream wOut;
	wOut << std::fixed << std::setprecision(3) << wGo;
	std::cout << "Current RAM used: " << wOut.str() << " Go\n";
}

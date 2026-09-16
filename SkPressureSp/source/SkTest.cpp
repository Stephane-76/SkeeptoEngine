//=============================================================================
// SkPressure1 
// Test Pressure
//=============================================================================
#include "../include/SkTest.hpp"


void DrawCell(tApi* sApi,tString sOperation, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
	//return; // Drop 
	cout << endl;
	cout << sOperation << endl;
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
		for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
			tVariant wVariant = sApi->CellValue(wRow, wCol);
			tString wFormula = sApi->Formula(wRow, wCol);
			cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant << "\t";
		}
		cout << endl;
	}
}

void DebugRange(tApi* sApi) {
	(void)sApi;
	return; // Drop
}

void Test(tApi* sApi) {
	tInt wNbRow = 4;
	tInt wNbCol = 3;
	tCell* wCell;
	for (tInt wCol = 1; wCol <= wNbCol; wCol++) {
		if (wCol > 1) {
			wCell = sApi->EnsureCell(1, wCol);
			tStringStream wStream;
			wStream << Base10ToAlpha(wCol - 1) << wNbRow - 1 << "+1";
			sApi->CompilCell(wCell, wStream.str().c_str());
		}
	}

	for (tInt wRow = 2; wRow <= wNbRow; wRow++) {
		for (tInt wCol = 1; wCol <= wNbCol; wCol++) {
			wCell = sApi->EnsureCell(wRow, wCol);
			tStringStream wStream;
			if (wRow == wNbRow) {
				if (wCol != wNbCol) {
					wStream << "SUM(" << Base10ToAlpha(wCol) << 1 << ":" << Base10ToAlpha(wCol) << wRow - 1 << ")";
					//wStream << "1";
				}
				else {
					wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(wNbCol - 1) << wRow << ")";
					//wStream << "1";
				}
			}
			else {
				if (wCol == wNbCol) {
					wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(wNbCol - 1) << wRow << ")";
					//wStream << "1";
				}
				else {
					wStream << Base10ToAlpha(wCol) << wRow - 1 << "+1";
				}
			}
			sApi->CompilCell(wCell, wStream.str().c_str());

		}
	}
	tTempoRect* wTempoRect = new  tTempoRect(1, 1, wNbRow, wNbCol);
	sApi->ActiveSheet()->Calculate(wTempoRect);
	DrawCell(sApi,"Before", 1, 1, 8, 10);
	sApi->UndoDeleteRow(2, 2);
	//DrawCell(sApi,"After", 1, 1, 8, 10);
	DebugRange(sApi);
#ifdef checksp
	sApi->Check();
#endif
	sApi->Undo();
	DrawCell(sApi,"Undo", 1, 1, 8, 10);
#ifdef checksp
	sApi->Check();
#endif

	tVariant wVariant = 12;
	sApi->CellValue(1, 2, wVariant);
	wVariant = 121;
	sApi->CellValue(3, 2, wVariant);

	wVariant = "=B1+B9";
	sApi->CellValue(2, 2, wVariant);

	wVariant = "=SUM(B2:C3)";
	sApi->CellValue(5, 4, wVariant);

	wVariant = "=SUM(A2:D3)";
	sApi->CellValue(5, 5, wVariant);


	// Test Recover
	wVariant = "=SUM(A2:D4)";
	sApi->CellValue(5, 6, wVariant);

	wVariant = "=SUM(A3:D4)";
	sApi->CellValue(5, 7, wVariant);

	wVariant = "=SUM(A4:D4)";
	sApi->CellValue(5, 8, wVariant);


	// Test Recover
	wVariant = "=SUM(B2:D4)";
	sApi->CellValue(6, 6, wVariant);
	wVariant = "=A12+SUM(B3:D4)";
	sApi->CellValue(6, 7, wVariant);
	wVariant = "=G46+SUM(B4:D4)";
	sApi->CellValue(6, 8, wVariant);
	wTempoRect = new  tTempoRect(1, 1, 10,10);
	sApi->ActiveSheet()->Calculate(wTempoRect);
#ifdef checksp
	sApi->Check();
#endif
	DrawCell(sApi,"SetValues", 1, 1, 8, 10);
	DebugRange(sApi);
	sApi->UndoDeleteRow(2, 2);

	DrawCell(sApi,"UndoDeleteRow 2,2", 1, 1, 8, 10);
	DrawCell(sApi,"After", 1, 1, 8, 10);
	DebugRange(sApi);
#ifdef checksp
	sApi->Check();
#endif
	sApi->Undo();
	DrawCell(sApi,"Undo", 1, 1, 8, 10);
	DebugRange(sApi);
#ifdef checksp
	sApi->Check();
#endif
}

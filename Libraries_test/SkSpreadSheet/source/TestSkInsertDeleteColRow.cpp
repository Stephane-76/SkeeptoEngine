//==============================================================================
// TestSkInsertDeleteColRow
// Test la librairie SparseArray
//==============================================================================
#include <SkAllocator.hpp>
#include "../include/TestSkInsertDeleteColRow.hpp"
#include "../include/SkSpreadSheet.hpp"


// We can send it to the API of a feature 
TestSkInsertDeleteColRow::TestSkInsertDeleteColRow() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
    m_NbRow =8;
    m_NbCol =10;
}

void TestSkInsertDeleteColRow::DrawCell(tString sOperation, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
    return; // Drop
	cout << endl;
	cout << sOperation << endl;
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
		for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
			tVariant wVariant = m_Api->CellValue(wRow, wCol);
			tString wFormula = m_Api->Formula(wRow, wCol);
			cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant << "\t";
		}
		cout << endl;
	}
}

void TestSkInsertDeleteColRow::UndoOperation() {
	return; // Drop
	tUndo* wUndo = m_Api->LastUndo();
	if (wUndo != nullptr) {
		cout << "Undo ->" << wUndo->OperationName() << endl;
	}
}


void TestSkInsertDeleteColRow::DebugRange() {
	return; // Drop
	//cout << "Debug Range" << endl;
    //tVectorRange* wVectorRange=m_Api->ActiveSheet()->ColRowCellRange()->VectorRange();
	//for (auto wRange : *wVectorRange) {
		//cout << wRange->Debug();
		//SkSpreadSheetContainer::Instance()->RangeDb(&wRange)->Debug();
	//}
}

void TestSkInsertDeleteColRow::Fill() {
	tCell* wCell;
	for (tInt wCol = 1; wCol <= m_NbCol; wCol++) {
		if (wCol > 1) {
			wCell = m_Api->EnsureCell(1, wCol);
			tStringStream wStream;
			wStream << Base10ToAlpha(wCol - 1) << m_NbRow - 1 << "+1";
			m_Api->CompilCell(wCell, wStream.str().c_str());
		}
	}

	for (tInt wRow = 2; wRow <= m_NbRow; wRow++) {
		for (tInt wCol = 1; wCol <= m_NbCol; wCol++) {
			wCell = m_Api->EnsureCell(wRow, wCol);
			tStringStream wStream;
			if (wRow == m_NbRow) {
				if (wCol != m_NbCol) {
					wStream << "SUM(" << Base10ToAlpha(wCol) << 1 << ":" << Base10ToAlpha(wCol) << wRow - 1 << ")";
					//wStream << "1";
				}
				else {
					wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(m_NbCol - 1) << wRow << ")";
					//wStream << "1";
				}
			}
			else {
				if (wCol == m_NbCol) {
					wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(m_NbCol - 1) << wRow << ")";
					//wStream << "1";
				}
				else {
					wStream << Base10ToAlpha(wCol) << wRow - 1 << "+1";
				}
			}
			m_Api->CompilCell(wCell, wStream.str().c_str());

		}
	}
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, m_NbRow, m_NbCol));
	DrawCell("Fill Sheet1", 1, 1, 8, 10);
}


void TestSkInsertDeleteColRow::TestDeleteColRowSimple() {
    m_Api->UndoCellValue("A1", 123);
    m_Api->UndoCellValue("B1", 12);
    m_Api->UndoCellValue("C1", "=SUM(A1:B1)");
    DrawCell("Before Delete Col", 1, 1, 1, 3);
    m_Api->UndoDeleteCol(1,1);
    tCell* wCell=m_Api->Cell("B1");
    
    CPPUNIT_ASSERT_MESSAGE("TestSkSimple B1", wCell->FormulaStr() == "SUM(A1:A1)");
    
    DrawCell("After Delete Col", 1, 1, 1, 3);
    m_Api->Undo();
    
    m_Api->UndoCellValue("A2", 12);
    m_Api->UndoCellValue("A3", "=SUM(A1:A2)");
    DrawCell("Before Delete Row", 1, 1, 3, 1);
   
    m_Api->UndoDeleteRow(1,1);
    wCell=m_Api->Cell("A2");
    DrawCell("After Delete Row", 1, 1, 3, 1);
    
    CPPUNIT_ASSERT_MESSAGE("TestSkSimple B1", wCell->FormulaStr() == "SUM(A1:A1)");
    
}

void TestSkInsertDeleteColRow::TestDeleteColRowFill() {
    Fill();
    DrawCell("Before Undo Redo  Delete Row", 1, 1, m_NbRow, m_NbCol);
    for(tIndex wIndRow =1; wIndRow<=m_NbRow;wIndRow++) {
        m_Api->UndoDeleteRow(wIndRow, 1);
    }
    for(tIndex wIndRow =1; wIndRow<=m_NbRow;wIndRow++) {
        m_Api->Undo();
    }
    tCell* wCellJ8=m_Api->Cell("J8");
    CPPUNIT_ASSERT_MESSAGE("TestSkSimple B1", wCellJ8->Value().Int() == 1953);
    
    DrawCell("Before Undo Redo  Delete Row", 1, 1, m_NbRow, m_NbCol);
    for(tIndex wIndRow =1; wIndRow<=m_NbRow;wIndRow++) {
        m_Api->Redo();
    }
    for(tIndex wIndRow =1; wIndRow<=m_NbRow;wIndRow++) {
        m_Api->Undo();
    }
    DrawCell("After Undo Redo  Delete Row", 1, 1, m_NbRow, m_NbCol);
    wCellJ8=m_Api->Cell("J8");
    CPPUNIT_ASSERT_MESSAGE("TestSkSimple B1", wCellJ8->Value().Int() == 1953);
    
    DrawCell("Before Undo Redo  Delete Row", 1, 1, m_NbRow, m_NbCol);
    for(tIndex wIndCol =1; wIndCol<=m_NbCol;wIndCol++) {
        m_Api->UndoDeleteCol(wIndCol, 1);
    }
    for(tIndex wIndCol =1; wIndCol<=m_NbCol;wIndCol++) {
        m_Api->Undo();
    }
    wCellJ8=m_Api->Cell("J8");
    CPPUNIT_ASSERT_MESSAGE("TestSkSimple B1", wCellJ8->Value().Int() == 1953);
    
    DrawCell("Before Undo Redo  Delete Row", 1, 1, m_NbRow, m_NbCol);
    for(tIndex wIndCol =1; wIndCol<=m_NbCol;wIndCol++) {
        m_Api->Redo();
    }
    for(tIndex wIndCol =1; wIndCol<=m_NbCol;wIndCol++) {
        m_Api->Undo();
    }
    DrawCell("After Undo Redo  Delete Row", 1, 1, m_NbRow, m_NbCol);
    wCellJ8=m_Api->Cell("J8");
    CPPUNIT_ASSERT_MESSAGE("TestSkSimple B1", wCellJ8->Value().Int() == 1953);
}

void TestSkInsertDeleteColRow::TestBottomRight() {
    tPoint wPoint=m_Api->BottomRight();
    tStringStream wStream;
    wStream  << Base10ToAlpha(wPoint.Col()) << wPoint.Row();
    CPPUNIT_ASSERT_MESSAGE("TestBottomRight 1", wStream.str()=="@0");
    
    m_Api->UndoCellValue("A12000",10);
    
    wPoint=m_Api->BottomRight();
    wStream.str("");
    wStream  << Base10ToAlpha(wPoint.Col()) << wPoint.Row();
    CPPUNIT_ASSERT_MESSAGE("TestBottomRight 2", wStream.str()=="A12000");

    m_Api->UndoRaz("A12000");

    wPoint=m_Api->BottomRight();
    wStream.str("");
    wStream  << Base10ToAlpha(wPoint.Col()) << wPoint.Row();
    CPPUNIT_ASSERT_MESSAGE("TestBottomRight 3", wStream.str()=="@0");

    m_Api->Undo();
    wPoint=m_Api->BottomRight();
    wStream.str("");
    wStream  << Base10ToAlpha(wPoint.Col()) << wPoint.Row();
    CPPUNIT_ASSERT_MESSAGE("TestBottomRight 4", wStream.str()=="A12000");

    m_Api->Undo();
    wPoint=m_Api->BottomRight();
    wStream.str("");
    wStream  << Base10ToAlpha(wPoint.Col()) << wPoint.Row();
    CPPUNIT_ASSERT_MESSAGE("TestBottomRight 5", wStream.str()=="@0");

}

void TestSkInsertDeleteColRow::TestInsertDeleteColRow() {
	for (tInt wRow = 0; wRow < 10; wRow++) {
		for (tInt wCol = 0; wCol < 10; wCol++) {
			tVariant wVariant = wRow * 10 + wCol;
			m_Api->CellValue(wRow, wCol, wVariant);
		}
	}
	tVariant wVariant = 99;
	m_Api->InsertRow(2, 3);
	CPPUNIT_ASSERT_MESSAGE("Test Cells Insert Row(2,3) !", m_Api->CellValue(12, 9) == wVariant);
	DrawCell("Test Cells Insert Row(2,3) !",0, 0, 12, 9);

	m_Api->InsertCol(2, 3);
	DrawCell("Test Cells Insert Col(2,3) !", 0, 0, 12, 12);
	CPPUNIT_ASSERT_MESSAGE("Test Cells Insert Col(2,3) !", m_Api->CellValue(12, 12) == wVariant);

	m_Api->DeleteRow(2, 3);
	DrawCell("Test Cells Erase Row(2, 3) !",0, 0, 9, 12);
	CPPUNIT_ASSERT_MESSAGE("Test Cells Erase Row(2,3) !", m_Api->CellValue(9, 12) == wVariant);

	m_Api->DeleteCol(2, 3);
	DrawCell("Test Cells Erase Col(2,3) !", 0, 0, 9, 9);
	CPPUNIT_ASSERT_MESSAGE("Test Cells Erase Col(2,3) !", m_Api->CellValue(9, 9) == wVariant);

	m_Api->DeleteCol(0, 5);
	DrawCell("Test Cells Erase Col(0,5) !", 0, 0, 9, 9);
	CPPUNIT_ASSERT_MESSAGE("Test Cells Erase Col(0,5) !", m_Api->CellValue(9, 4) == wVariant);
}

void  TestSkInsertDeleteColRow::DebugMerged(tRect sRect,tString sMessage) {
    (void)sRect;
    (void)sMessage;
}

void TestSkInsertDeleteColRow::TestRangeNamed() {
    Fill();
    m_Api->UndoInsertRangeNamed("COUCOU", "A4:A5");
   
    
    m_Api->UndoDeleteCol(1, 1);
    tRange* wRange=m_Api->FindRangeNamed("COUCOU");
    CPPUNIT_ASSERT_MESSAGE("After Delete Col Range Named nullptr", wRange==nullptr);
    
    m_Api->Undo();
    
    wRange=m_Api->FindRangeNamed("COUCOU");
    //cout << endl << wRange->StrRef(true) << endl;
    CPPUNIT_ASSERT_MESSAGE("After Undo Delete Col Range IsNamed", wRange->IsNamed());;
    CPPUNIT_ASSERT_MESSAGE("After Undo Delete Col Range Named exist",wRange->StrRef(true)=="Sheet1!A4:A5");
    
    DrawCell("Fill Sheet1", 1, 1, 5, 5);
    
    m_Api->UndoDeleteRow(2, 4);
    
    wRange=m_Api->FindRangeNamed("COUCOU");
    //cout << wRange->StrRef() << endl;
    CPPUNIT_ASSERT_MESSAGE("After Delete Row Range Named nullptr", wRange==nullptr);
    
    m_Api->Undo();
    
    wRange=m_Api->FindRangeNamed("COUCOU");
    //cout << endl << wRange->StrRef(true) << endl;
    CPPUNIT_ASSERT_MESSAGE("After Undo Delete Row Range IsNamed", wRange->IsNamed());;
    CPPUNIT_ASSERT_MESSAGE("After Undo Delete Row Range Named exist",wRange->StrRef(true)=="Sheet1!A4:A5");
}

void TestSkInsertDeleteColRow::TestMerged() {
    Fill();
    m_Api->UndoCellValue("A1", "=SUM(B3:C6)");
    tCell* wCell=m_Api->Cell("A1");
    //cout << endl << wCell->FormulaStr() << endl;
    tRect wCellRecover=tRect(1,1,6,5);
    DebugMerged(wCellRecover,"Merged Formlula");
    
    m_Api->UndoCellValue("J3", "=SUM(A3:I3)");

    //cout << endl << m_Api->Cell("J3")->Debug();
    
    CPPUNIT_ASSERT_MESSAGE("TestMerged A1 (0) !",wCell->FormulaStr() == "SUM(B3:C6)");
    
    m_Api->UndoApplyMerge("B3:C6");
    tRange* wRange=m_Api->FindRange("B3:C6");
    
    CPPUNIT_ASSERT_MESSAGE("TestMerged Merged (1) !", wRange->IsMerged());

    m_Api->UndoDeleteRow(3,2);
    DebugMerged(wCellRecover,"UndoDeleteRow 3,2");
    //cout << tApplication::Instance()->DebugUndo() << endl;
    wRange=m_Api->FindRange("B3:C4");
    //if (wRange!=nullptr) cout << wRange->Debug();
    m_Api->Undo();
  
    DebugMerged(wCellRecover,"Merged 1");
    
    //cout << endl << m_Api->FindRange("A3:I3")->Debug();
    
    wRange=m_Api->FindRange("B3:C6");
    CPPUNIT_ASSERT_MESSAGE("TestMerged  Merged (2) !", wRange->IsMerged());
    
    m_Api->UndoApplyMerge("B3:C6");
    wRange=m_Api->FindRange("B3:C6");
    CPPUNIT_ASSERT_MESSAGE("TestMerged Not Merged (3) !", !wRange->IsMerged());

    m_Api->UndoApplyMerge("B3:C6");
    // Delete Row =============================================================
    m_Api->UndoDeleteRow(3, 1);
    wRange=m_Api->FindRange("B3:C5");
    CPPUNIT_ASSERT_MESSAGE("TestMerged  Merged (4) !", wRange->IsMerged());
    
    DebugMerged(wCellRecover,"DeleteRow 3,1");
    
    m_Api->UndoDeleteRow(3, 1);
    DebugMerged(wCellRecover,"DeleteRow 3,1");
    DrawCell("Befor Opereration", 1, 1,10,8);
    wRange=m_Api->FindRange("B3:C4");
    CPPUNIT_ASSERT_MESSAGE("TestMerged  Merged (5) !", wRange->IsMerged());

    m_Api->UndoDeleteRow(3, 1);
    DebugMerged(wCellRecover,"DeleteRow 3,1");
    wRange=m_Api->FindRange("B3:C3");
    CPPUNIT_ASSERT_MESSAGE("TestMerged  Merged (6) !", wRange->IsMerged());

    m_Api->UndoDeleteRow(3, 1);
    DebugMerged(wCellRecover,"DeleteRow 3,1");
    
    m_Api->Undo();
    DebugMerged(wCellRecover,"Undo Delete Row");
    wRange=m_Api->FindRange("B3:C3");
    CPPUNIT_ASSERT_MESSAGE("TestMerged  Merged (7) !", wRange->IsMerged());

    
    m_Api->Undo();
    DebugMerged(wCellRecover,"Undo Delete Row");
    wRange=m_Api->FindRange("B3:C4");
    

    CPPUNIT_ASSERT_MESSAGE("TestMerged  Merged (8) !", wRange->IsMerged());
    DebugMerged(wCellRecover,"Undo Delete Row");
    m_Api->Undo();
    
    DebugMerged(wCellRecover,"Undo Delete Row");
    wRange=m_Api->FindRange("B3:C5");
    CPPUNIT_ASSERT_MESSAGE("TestMerged  Merged (9) !", wRange->IsMerged());

    m_Api->Undo();
    wRange=m_Api->FindRange("B3:C6");
    //cout << wRange->StrRef(true) << endl;
    CPPUNIT_ASSERT_MESSAGE("TestMerged  Merged (10) !", wRange->IsMerged());
    
    //cout << endl << "B10=" << m_Api->Cell("J3")->Debug();
    DebugMerged(wCellRecover,"End");
    
    CPPUNIT_ASSERT_MESSAGE("TestMerged A1 (0) !",wCell->FormulaStr() == "SUM(B3:C6)");
    
}

void TestSkInsertDeleteColRow::TestSkInsertDeleteColRowWithFormula() {
	tVariant wVariant = "=SUM(B4:E4)";
	m_Api->UndoCellValue("D8", wVariant);
#ifdef checksp
	m_Api->Check();
#endif
	wVariant = "1"; m_Api->UndoCellValue("C3",wVariant);
	wVariant = "2"; m_Api->UndoCellValue("C4", wVariant);
	wVariant = "3"; m_Api->UndoCellValue("C5", wVariant);
#ifdef checksp
	m_Api->Check();
#endif

	wVariant = "=SUM(C4:E4)";
	m_Api->UndoCellValue("F6", wVariant);

	wVariant = "=SUM(D4:E4)";
	m_Api->UndoCellValue("D7", wVariant);
	DebugRange();

	DrawCell("SetValues",1, 1, 8, 10);
	DebugRange();
#ifdef checksp
	m_Api->Check();
#endif
	DebugRange();
	m_Api->UndoDeleteCol(2, 2);

	DrawCell("Delete Col 2,2",1, 1, 8, 10);
	DebugRange();
#ifdef checksp
	m_Api->Check();
#endif
	m_Api->Undo();
	DebugRange();
	DrawCell("Undo Delete Col 2,2", 1, 1, 8, 10);
#ifdef checksp
	m_Api->Check();
#endif

}

void TestSkInsertDeleteColRow::TestSkInsertDeleteColRowWithCoveredRange() {}

void TestSkInsertDeleteColRow::TestSkInsertDeleteColRowWithFormulaUndo() {

	tVariant wVariant = "=SUM(B4:E4)";
	m_Api->UndoCellValue(4, 8, wVariant);

	wVariant = 1; m_Api->UndoCellValue("C4", wVariant);
	wVariant = 2; m_Api->UndoCellValue("D4", wVariant);
	wVariant = 3; m_Api->UndoCellValue("E4", wVariant);

	wVariant = "=SUM(C4:E4)";
	m_Api->UndoCellValue("F4",wVariant);

	wVariant = "=SUM(D4:E4)";
	m_Api->UndoCellValue("G4", wVariant);
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, 10, 10));
	DrawCell("SetValues",1, 1, 8, 10);
	DebugRange();
	tVariant wVariantResult = m_Api->CellValue(4, 6); CPPUNIT_ASSERT_MESSAGE("TestCell F4=6", wVariantResult.Int() == 6);
	wVariantResult = m_Api->CellValue(4,7); CPPUNIT_ASSERT_MESSAGE("TestCell G4=5", wVariantResult.Int() == 5);
	wVariantResult = m_Api->CellValue(4,8); CPPUNIT_ASSERT_MESSAGE("TestCell H4=6", wVariantResult.Int() == 6);

#ifdef checksp
	m_Api->Check();
#endif
	// Test Insert=============================================================
	m_Api->UndoInsertCol(2, 2);
	DrawCell("UndoInsertCol 2,2",1, 1, 8, 10);

	wVariantResult = m_Api->CellValue(4, 8); CPPUNIT_ASSERT_MESSAGE("TestCell H4='123'", wVariantResult.Int() == 6);
	wVariantResult = m_Api->CellValue(4, 9); CPPUNIT_ASSERT_MESSAGE("TestCell I4='23'", wVariantResult.Int() == 5);
	wVariantResult = m_Api->CellValue(4, 10); CPPUNIT_ASSERT_MESSAGE("TestCell J4='23'", wVariantResult.Int() == 6);

#ifdef checksp
	m_Api->Check();
#endif
	m_Api->UndoInsertRow(2, 3);
	DrawCell("UndoInsertRow 2,3",1, 1, 8, 10);

	wVariantResult = m_Api->CellValue(7, 8); CPPUNIT_ASSERT_MESSAGE("TestCell H7='123'", wVariantResult.Int() == 6);
	wVariantResult = m_Api->CellValue(7, 9); CPPUNIT_ASSERT_MESSAGE("TestCell I7='23'", wVariantResult.Int() == 5);
	wVariantResult = m_Api->CellValue(7, 10); CPPUNIT_ASSERT_MESSAGE("TestCell J7='23'", wVariantResult.Int() == 6);


	m_Api->Undo();
	DrawCell("Undo",1, 1, 8, 10);

	wVariantResult = m_Api->CellValue(4, 8); CPPUNIT_ASSERT_MESSAGE("TestCell H4='123'", wVariantResult.Int() == 6);
	wVariantResult = m_Api->CellValue(4, 9); CPPUNIT_ASSERT_MESSAGE("TestCell I4='23'", wVariantResult.Int() == 5);
	wVariantResult = m_Api->CellValue(4, 10); CPPUNIT_ASSERT_MESSAGE("TestCell J4='23'", wVariantResult.Int() == 6);


	m_Api->Undo();
	DrawCell("Undo", 1, 1, 8, 10);

	wVariantResult = m_Api->CellValue(4, 6); CPPUNIT_ASSERT_MESSAGE("TestCell F4='123'", wVariantResult.Int() == 6);
	wVariantResult = m_Api->CellValue(4, 7); CPPUNIT_ASSERT_MESSAGE("TestCell G4='23'", wVariantResult.Int() == 5);
	wVariantResult = m_Api->CellValue(4, 8); CPPUNIT_ASSERT_MESSAGE("TestCell H4='23'", wVariantResult.Int() == 6);

#ifdef checksp
	m_Api->Check();
#endif
	// Test Delete ============================================================
	m_Api->UndoInsertCol(2, 2);
	DrawCell("UndoInsertCol 2,2",1, 1, 8, 10);

	wVariantResult = m_Api->CellValue(4, 8); CPPUNIT_ASSERT_MESSAGE("TestCell H4='123'", wVariantResult.Int() == 6);
	wVariantResult = m_Api->CellValue(4, 9); CPPUNIT_ASSERT_MESSAGE("TestCell I4='23'", wVariantResult.Int() == 5);
	wVariantResult = m_Api->CellValue(4, 10); CPPUNIT_ASSERT_MESSAGE("TestCell J4='23'", wVariantResult.Int() == 6);

	DebugRange();
#ifdef checksp
	m_Api->Check();
#endif
	m_Api->UndoDeleteCol(2, 3);
	DrawCell("UndoDeleteCol 2,3)",2, 2, 8, 10);

	wVariantResult = m_Api->CellValue(4, 5); CPPUNIT_ASSERT_MESSAGE("TestCell E4='123'", wVariantResult.Int() == 6);
	wVariantResult = m_Api->CellValue(4, 6); CPPUNIT_ASSERT_MESSAGE("TestCell F4='23'", wVariantResult.Int() == 5);
	wVariantResult = m_Api->CellValue(4, 7); CPPUNIT_ASSERT_MESSAGE("TestCell G4='23'", wVariantResult.Int() == 6);

	DebugRange();
#ifdef checksp
    m_Api->Check();
#endif
	UndoOperation();
	m_Api->Undo();
	DrawCell("Undo",1, 1, 8, 10);

	wVariantResult = m_Api->CellValue(4, 8); CPPUNIT_ASSERT_MESSAGE("TestCell H4='123'", wVariantResult.Int() == 6);
	wVariantResult = m_Api->CellValue(4, 9); CPPUNIT_ASSERT_MESSAGE("TestCell I4='23'", wVariantResult.Int() == 5);
	wVariantResult = m_Api->CellValue(4, 10); CPPUNIT_ASSERT_MESSAGE("TestCell J4='23'", wVariantResult.Int() == 6);

#ifdef checksp
	m_Api->Check();
#endif
	UndoOperation();
	m_Api->Undo();
	DrawCell("Undo", 1, 1, 8, 10);

	wVariantResult = m_Api->CellValue(4, 6); CPPUNIT_ASSERT_MESSAGE("TestCell F4='123'", wVariantResult.Int() == 6);
	wVariantResult = m_Api->CellValue(4, 7); CPPUNIT_ASSERT_MESSAGE("TestCell G4='23'", wVariantResult.Int() == 5);
	wVariantResult = m_Api->CellValue(4, 8); CPPUNIT_ASSERT_MESSAGE("TestCell H4='23'", wVariantResult.Int() == 6);
#ifdef checksp
	m_Api->Check();
#endif
	UndoOperation();
	m_Api->Undo();
	DrawCell("Undo", 1, 1, 8, 10);
	UndoOperation();
	m_Api->Undo();
	UndoOperation();
	m_Api->Undo();
	UndoOperation();
	m_Api->Undo();
	UndoOperation();
	m_Api->Undo();
	UndoOperation();
	m_Api->Undo();
	DrawCell("Undo", 1, 1, 8, 10);

	for (tInt wRow = 1; wRow < 10; wRow++) {
		for (tInt wCol = 1; wCol < 10; wCol++) {
			tCell* wCell = m_Api->Cell(wRow, wCol);
			CPPUNIT_ASSERT_MESSAGE("Cell not Null", wCell==nullptr);
		}
	}
	
#ifdef checksp
	m_Api->Check();
#endif

};

void TestSkInsertDeleteColRow::TestSkInsertDeleteColRowWithCoveredRangeUndo() {
    m_NbRow = 4;
    m_NbCol = 3;
    Fill();
    DrawCell("UndoDeleteRow 2,2", 1, 1, 5, 4);
    // Delete in Sheet1
    m_Api->UndoDeleteRow(2, 2);
    
    DrawCell("After UndoDeleteRow 2,2", 1, 1, 5, 4);
#ifdef checksp
    m_Api->Check();
#endif
    m_Api->Undo();
#ifdef checksp
    m_Api->Check();
#endif

	m_NbRow = 8;
	m_NbCol = 10;
	Fill();
    
	// Delete in Sheet1
	m_Api->UndoDeleteRow(2, 2);
	
	DrawCell("After UndoDeleteRow", 1, 1, 8, 10);
	DebugRange();
#ifdef checksp
	m_Api->Check();
#endif
	m_Api->Undo();
	DrawCell("Undo", 1, 1, 8, 10);
#ifdef checksp
	m_Api->Check();
#endif

	tVariant wVariant = 12;
	m_Api->UndoCellValue("A2", wVariant);
	wVariant = 121;
	m_Api->UndoCellValue("C2", wVariant);

	wVariant = "=B1+B9+B9+B9+B9";
	m_Api->UndoCellValue("B2", wVariant);

	wVariant = "=SUM(B2:C3)";
	m_Api->UndoCellValue("E4", wVariant);

	wVariant = "=SUM(A2:D3)";
	m_Api->UndoCellValue("E5", wVariant);


	// Test Recover =========================================================
	wVariant = "=SUM(A2:D4)";
	m_Api->UndoCellValue("E6", wVariant);

	wVariant = "=SUM(A3:D4)";
	m_Api->UndoCellValue("E7", wVariant);

	wVariant = "=SUM(A4:D4)";
	m_Api->UndoCellValue("E8", wVariant);

	
	// Test Recover
	wVariant = "=SUM(B2:D4)";
	m_Api->UndoCellValue("F6", wVariant);
	wVariant = "=A12+SUM(B3:D4)";
	m_Api->UndoCellValue("F7", wVariant);
	wVariant = "=G46+SUM(B4:D4)";
	m_Api->UndoCellValue("F8", wVariant);
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, 10, 10));
#ifdef checksp
	m_Api->Check();
#endif
	DrawCell("SetValues",1, 1, 8, 10);
	DebugRange();

	m_Api->UndoDeleteRow(2, 2);
	DrawCell("UndoDeleteRow 2,2",1, 1, 8, 10);
	DrawCell("After", 1, 1, 8, 10);
	DebugRange();
#ifdef checksp
	m_Api->Check();
#endif
	m_Api->Undo();
	DrawCell("Undo",1, 1, 8, 10);
 	DebugRange();
#ifdef checksp
	m_Api->Check();
#endif

	DrawCell("UndoDeleteRow 2,2", 1, 1, 8, 10);
	m_Api->UndoDeleteCol(2, 2);
	DrawCell("After", 1, 1, 8, 10);
	DebugRange();
#ifdef checksp
	m_Api->Check();
#endif
	m_Api->Undo();
	DrawCell("Undo", 1, 1, 8, 10);
	DebugRange();
#ifdef checksp
	m_Api->Check();
#endif
	for (tInt wInd = 1; wInd < 12; wInd++) {
		m_Api->Undo();
	}
#ifdef checksp
	m_Api->Check();
#endif

}


void TestSkInsertDeleteColRow::TestSkPressure() {
    m_NbRow=10;
    m_NbCol=10;
    Fill();
    
    DrawCell("Before Delete Col", 1, 1, m_NbRow, m_NbCol);
    tCell* wCell=m_Api->Cell(m_NbRow,m_NbCol);
    //cout <<endl << wCell->StrRef()  << ":" << wCell->FormulaStr() << "=" << wCell->Value() << endl;
    for(tIndex wCol=1;wCol<=m_NbCol-1;wCol++) {
        m_Api->UndoDeleteCol(1,1);
        DrawCell("Delete ", 1, 1, m_NbRow, m_NbCol);
    }
   
    for(tIndex wCol=1;wCol<=m_NbCol-1;wCol++) {
        //cout << tApplication::Instance()->DebugUndo() << endl;
        m_Api->Undo();
        DrawCell("Undo ", 1, 1, m_NbRow, m_NbCol);
    }
    wCell=m_Api->Cell(m_NbRow,m_NbCol);
    
    
    
    wCell = m_Api->Cell(m_NbRow,m_NbCol);
    //cout <<endl << wCell->StrRef()  << ":" << wCell->FormulaStr() << "=" << wCell->Value() << endl;
 
    
    CPPUNIT_ASSERT_MESSAGE("TestSkPressure Delete Col J10", wCell->Value().Int() == 3240);
    CPPUNIT_ASSERT_MESSAGE("TestSkPressure Delete Col J10", wCell->FormulaStr() == "SUM(A10:I10)");

    DrawCell("Before Delete Row", 1, 1, m_NbRow, m_NbCol);
    wCell=m_Api->Cell(m_NbRow,m_NbCol);
    //cout <<endl << wCell->StrRef()  << ":" << wCell->FormulaStr() << "=" << wCell->Value() << endl;
    for(tIndex wRow=1;wRow<=m_NbRow-1;wRow++) {
        m_Api->UndoDeleteRow(1,1);
        //cout << tApplication::Instance()->DebugUndo() << endl;
        DrawCell("Delete ", 1, 1, m_NbRow, m_NbCol);
    }
   
    for(tIndex wRow=1;wRow<=m_NbRow-1;wRow++) {
        //cout << tApplication::Instance()->DebugUndo() << endl;
        m_Api->Undo();
        DrawCell("Undo ", 1, 1, m_NbRow, m_NbCol);
    }
    wCell=m_Api->Cell(m_NbRow,m_NbCol);
    
    
    //cout <<endl << wCell->StrRef()  << ":" << wCell->FormulaStr() << "=" << wCell->Value() << endl;
    
    wCell = m_Api->Cell(m_NbRow,m_NbCol);
    
    CPPUNIT_ASSERT_MESSAGE("TestSkPressure Delete Row J10", wCell->Value().Int() == 3240);
    CPPUNIT_ASSERT_MESSAGE("TestSkPressure Delete Row J10", wCell->FormulaStr() == "SUM(A10:I10)");
}

tString TestSkInsertDeleteColRow::SnapshotCell(tIndex sRow, tIndex sCol) const {
    tCell* wCell = m_Api->Cell(sRow, sCol);
    if (wCell == nullptr) {
        return "<null>";
    }
    tStringStream wStream;
    wStream << wCell->FormulaStr() << "|" << wCell->Value();
    return wStream.str();
}

tString TestSkInsertDeleteColRow::SnapshotRect(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) const {
    tStringStream wStream;
    for (tIndex wRow = sTop; wRow <= sBottom; ++wRow) {
        for (tIndex wCol = sLeft; wCol <= sRight; ++wCol) {
            wStream << Base10ToAlpha(wCol) << wRow << "=" << SnapshotCell(wRow, wCol) << ";";
        }
        wStream << '\n';
    }
    return wStream.str();
}

void TestSkInsertDeleteColRow::AssertGridMatchesSnapshot(const tString& sExpected,
                                                         tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight,
                                                         const tString& sMessage) const {
    const tString wActual = SnapshotRect(sTop, sLeft, sBottom, sRight);
    CPPUNIT_ASSERT_MESSAGE(sMessage + ":\nexpected:\n" + sExpected + "\nactual:\n" + wActual, wActual == sExpected);
}

void TestSkInsertDeleteColRow::TestUndoRedoGridSnapshot() {
    Fill();
    m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, m_NbRow, m_NbCol));
#ifdef checksp
    m_Api->Check();
#endif
    const tString wBefore = SnapshotRect(1, 1, m_NbRow, m_NbCol);

    CPPUNIT_ASSERT(m_Api->UndoDeleteRow(3, 2));
    CPPUNIT_ASSERT(m_Api->Undo());
    AssertGridMatchesSnapshot(wBefore, 1, 1, m_NbRow, m_NbCol, "UndoDeleteRow(3,2) then Undo");
#ifdef checksp
    m_Api->Check();
#endif

    CPPUNIT_ASSERT(m_Api->UndoDeleteCol(2, 2));
    CPPUNIT_ASSERT(m_Api->Undo());
    AssertGridMatchesSnapshot(wBefore, 1, 1, m_NbRow, m_NbCol, "UndoDeleteCol(2,2) then Undo");
#ifdef checksp
    m_Api->Check();
#endif

    CPPUNIT_ASSERT(m_Api->UndoInsertRow(3, 2));
    CPPUNIT_ASSERT(m_Api->Undo());
    AssertGridMatchesSnapshot(wBefore, 1, 1, m_NbRow, m_NbCol, "UndoInsertRow(3,2) then Undo");
#ifdef checksp
    m_Api->Check();
#endif

    CPPUNIT_ASSERT(m_Api->UndoInsertCol(2, 2));
    CPPUNIT_ASSERT(m_Api->Undo());
    AssertGridMatchesSnapshot(wBefore, 1, 1, m_NbRow, m_NbCol, "UndoInsertCol(2,2) then Undo");
#ifdef checksp
    m_Api->Check();
#endif
}

void TestSkInsertDeleteColRow::setUp() {
	std::filesystem::remove_all("./Spreadsheet");

	m_Application = tApplication::Instance();
	m_Api = new tApi;
	m_Api->NewWorkBook("wwww.skeema.fr/w1");


};

void TestSkInsertDeleteColRow::tearDown() {
	delete(m_Api);
}

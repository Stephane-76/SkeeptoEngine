//==============================================================================
// TestSkSheet
// Test la librairie SparseArray
//==============================================================================

#include "../include/TestSkSheet.hpp"
#include "../include/SkSpreadSheet.hpp"

#define _printdebug

// We can send it to the API of a feature
TestSkSheet::TestSkSheet() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
}

void TestSkSheet::DrawCell(tString sTitle, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
	return; // Drop
	cout << endl;
	cout << sTitle << " on " <<  m_Api->ActiveSheet()->Name() <<   endl;
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
		for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
			tVariant wVariant = m_Api->CellValue(wRow, wCol);
			tString wFormula = m_Api->Formula(wRow, wCol);
			cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant << "\t";
		}
		cout << endl;
	}
}

void TestSkSheet::DebugRange() {
	return; // Drop 
	cout << "Debug Range" << endl;
    tVectorRange* wVectorRange; // = m_Api->ActiveSheet()->ColRowCellRange()->VectorRange();
	for (auto wRange : *wVectorRange) {
		wRange->Debug();
	}

}

void TestSkSheet::Fill() {
	m_NbRow = 8;
	m_NbCol = 10;

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
    m_Api->ActiveSheet()->Calculate(new tTempoRect(0, 0, 10, 10));
}

void TestSkSheet::TestAddSheet() {
    m_Api->UndoAddSheet("Test1","");
    tString wJsonList=m_Api->JsonSheets();
#ifdef printdebug
    cout << endl << wJsonList << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("AddSheet Test1", "{\"list\":[\"Sheet1\",\"Test1\"]}"==wJsonList);
    
    
    m_Api->UndoCellValue("A1", "Coucou");
    m_Api->Undo();
    
    m_Api->Undo();
    
    m_Api->Redo();
    
    m_Api->UndoRenameSheet("Test1", "Stephane_Allez");
    wJsonList=m_Api->JsonSheets();
#ifdef printdebug
    cout << endl << wJsonList << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("AddSheet Rename Sheet ","{\"list\":[\"Sheet1\",\"Stephane_Allez\"]}"==wJsonList);
    m_Api->Undo();
    wJsonList=m_Api->JsonSheets();
    CPPUNIT_ASSERT_MESSAGE("AddSheet Undo Rename Sheet ","{\"list\":[\"Sheet1\",\"Test1\"]}"==wJsonList);
  
    m_Api->UndoSwapSheet("Test1", "Sheet1");
    wJsonList=m_Api->JsonSheets();
    CPPUNIT_ASSERT_MESSAGE("AddSheet swap Sheet ","{\"list\":[\"Test1\",\"Sheet1\"]}"==wJsonList);
 
    m_Api->Undo();
    wJsonList=m_Api->JsonSheets();
    CPPUNIT_ASSERT_MESSAGE("AddSheet Undo swap Sheet ","{\"list\":[\"Sheet1\",\"Test1\"]}"==wJsonList);
  
    m_Api->UndoDeleteSheet("Test1");
    wJsonList=m_Api->JsonSheets();
#ifdef printdebug
    cout << endl << wJsonList << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("AddSheet Delete Sheet ", "{\"list\":[\"Sheet1\"]}"==wJsonList);
    
    m_Api->Undo();
    wJsonList=m_Api->JsonSheets();
#ifdef printdebug
    cout << endl << wJsonList << endl;
#endif
    
    CPPUNIT_ASSERT_MESSAGE("AddSheet Undo Delete Sheet", "{\"list\":[\"Sheet1\",\"Test1\"]}"==wJsonList);
    
    m_Api->UndoAddSheet("Test2","Test1");
    wJsonList=m_Api->JsonSheets();
#ifdef printdebug
    cout << endl << wJsonList << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("AddSheet Test2 before Test1", "{\"list\":[\"Sheet1\",\"Test2\",\"Test1\"]}"==wJsonList);
    
    m_Api->Undo();
#ifdef checksp
    m_Api->Check();
#endif
    m_Api->Redo();
#ifdef checksp
    m_Api->Check();
#endif
    wJsonList=m_Api->JsonSheets();
#ifdef printdebug
    cout << endl << wJsonList << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("AddSheet Unodo Redo AddSheet", "{\"list\":[\"Sheet1\",\"Test2\",\"Test1\"]}"==wJsonList);
#ifdef checksp
    m_Api->Check();
#endif
    
    m_Api->ActiveSheet("Test2");
    m_Api->UndoCellValue("A1", "Coucou");
    m_Api->Undo();
    
    wJsonList=m_Api->JsonSheets();
#ifdef printdebug
    cout << endl << wJsonList << endl;
#endif
 
    m_Api->UndoDeleteSheet("Sheet1");
    wJsonList=m_Api->JsonSheets();
#ifdef printdebug
    cout << endl << wJsonList << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("AddSheet Delete Sheet1", "{\"list\":[\"Test2\",\"Test1\"]}"==wJsonList);
    
    m_Api->Undo();

    wJsonList=m_Api->JsonSheets();
#ifdef printdebug
    cout << endl << wJsonList << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("AddSheet Undo Delete Sheet1", "{\"list\":[\"Sheet1\",\"Test2\",\"Test1\"]}"==wJsonList);
    
    m_Api->Undo();
    
    wJsonList=m_Api->JsonSheets();
#ifdef printdebug
    cout << endl << wJsonList << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("AddSheet Undo Add Test2", "{\"list\":[\"Sheet1\",\"Test1\"]}"==wJsonList);
    m_Api->Undo();

    wJsonList=m_Api->JsonSheets();
#ifdef printdebug
    cout << endl << wJsonList << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("AddSheet Undo Add Test1", "{\"list\":[\"Sheet1\"]}"==wJsonList);
}

void TestSkSheet::TestAddSheetWithSpace() {
    Fill();
    DrawCell("TestAddSheetWithSpace Sheet1", 1, 1, 8, 10);
    m_Api->AddSheet("Calcul financier");
    Fill();
    DrawCell("TestAddSheetWithSpace 'Calcul financier'", 1, 1, 8, 10);
    m_Api->ActiveSheet("Sheet1");
    m_Api->UndoCellValue("A1", "='Calcul financier'!B1");
    
    tCell* wCell=m_Api->EnsureCell("A1");
    
    //cout << wCell->FormulaStr() << wCell->Value() << endl;
    
    CPPUNIT_ASSERT_MESSAGE("TestAddSheetWithSpace Formula", wCell->FormulaStr()=="'Calcul financier'!B1");
    CPPUNIT_ASSERT_MESSAGE("TestAddSheetWithSpace Formula", wCell->Value()==tVariant(7));
    
    m_Api->AddSheet("Stéphane♡");
    m_Api->ActiveSheet("Sheet1");
    m_Api->UndoCellValue("A1", "=Stéphane♡!B1");
    
    wCell=m_Api->EnsureCell("A1");
    //cout << wCell->FormulaStr() << endl;
    CPPUNIT_ASSERT_MESSAGE("TestAddSheetWithSpace Formula with UTF8", wCell->FormulaStr()=="Stéphane♡!B1");
    
}

void TestSkSheet::TestCalculateInterSheeet() {
	m_NbRow = 8;
	m_NbCol = 10;

	tSheet* wSheet1 = m_Api->ActiveSheet();
	Fill();

	tSheet* wSheet2=m_Api->AddSheet("Sheet2");
	m_Api->ActiveSheet(wSheet2->Name());
	tCell* wCell2 = m_Api->EnsureCell(1, 1);

	tStringStream wStream;
	wStream << "Sheet1!" << Base10ToAlpha(m_NbCol) << m_NbRow;
	m_Api->CompilCell(wCell2, wStream.str().c_str());

	// Test Sum on other Sheet
	tCell* wCell3 = m_Api->EnsureCell(3, 1);
	wStream.str("");
	wStream << "SUM(Sheet1!" << Base10ToAlpha(1) << 1 << ":" << Base10ToAlpha(m_NbCol) << m_NbRow << ")";
	m_Api->CompilCell(wCell3, wStream.str().c_str());


	m_Api->ActiveSheet(wSheet1->Name());
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, m_NbRow, m_NbCol));
	DrawCell("Sheet 1",1, 1, m_NbRow, m_NbCol);
	m_Api->ActiveSheet(wSheet2->Name());

	DrawCell("Sheet 2",1, 1, m_NbRow, m_NbCol);
    m_Api->ActiveSheet("Sheet1");
    m_Api->UndoDeleteSheet(wSheet2->Name());
    
    m_Api->Undo();
 
    m_Api->ActiveSheet("Sheet2");
}

void TestSkSheet::TestInterSheeetDeleteColRowUndo() {
	tSheet* wSheet1 = m_Api->ActiveSheet();
	tSheet* wSheet2 = m_Api->AddSheet("Sheet2");

	Fill();
    
	m_Api->ActiveSheet(wSheet2->Name());
	
	tCell* wCell = m_Api->EnsureCell(1, 1);

	tStringStream wStream;
	wStream << "Sheet1!" << Base10ToAlpha(m_NbCol) << m_NbRow;
	m_Api->CompilCell(wCell, wStream.str().c_str());

	wStream.str(std::string());
	wStream << "SUM(Sheet1!A2:A3)";
	
	wCell = m_Api->EnsureCell(3, 1);
	m_Api->CompilCell(wCell, wStream.str().c_str());

	//cout << "Cell 3 " << wCell->StrRef() << "=" << wCell->FormulaStr() << endl;

	m_Api->ActiveSheet(wSheet1->Name());
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, m_NbRow, m_NbCol));
	DrawCell("",1, 1, m_NbRow, m_NbCol);
	m_Api->ActiveSheet(wSheet2->Name());
	DrawCell("",1, 1, m_NbRow, m_NbCol);

	m_Api->ActiveSheet(wSheet1->Name());
	m_Api->UndoDeleteRow(2, 3);
	DrawCell("UndoDelete Row 2,3",1, 1, m_NbRow, m_NbCol);
	m_Api->ActiveSheet(wSheet2->Name());
	DrawCell("After",1, 1, m_NbRow, m_NbCol);
#ifdef checksp
	m_Api->Check();
#endif
	m_Api->Undo();

	m_Api->ActiveSheet(wSheet1->Name());
	DrawCell("Undo",1, 1, m_NbRow, m_NbCol);
	m_Api->ActiveSheet(wSheet2->Name());
	DrawCell("Undo",1, 1, m_NbRow, m_NbCol);


#ifdef checksp
	m_Api->Check();
#endif

	m_Api->ActiveSheet(wSheet1->Name());
	DrawCell("UndoDelete Col 2,2", 1, 1, m_NbRow, m_NbCol);
	m_Api->ActiveSheet(wSheet2->Name());
	DrawCell("UndoDelete Col 2,2", 1, 1, m_NbRow, m_NbCol);
	m_Api->ActiveSheet(wSheet1->Name());
	m_Api->UndoDeleteCol(2, 3);

	DrawCell("After", 1, 1, m_NbRow, m_NbCol);
	m_Api->ActiveSheet(wSheet2->Name());
	DrawCell("After", 1, 1, m_NbRow, m_NbCol);
	m_Api->ActiveSheet(wSheet1->Name());
#ifdef checksp

	m_Api->Check();
#endif
	m_Api->Undo();

	m_Api->ActiveSheet(wSheet1->Name());
	DrawCell("Undo", 1, 1, m_NbRow, m_NbCol);
	m_Api->ActiveSheet(wSheet2->Name());
	DrawCell("Undo", 1, 1, m_NbRow, m_NbCol);
#ifdef checksp
	m_Api->Check();
#endif

}

void TestSkSheet::TestInterSheeetDeleteColRowUndoCovered() {
    Fill();
    tSheet* wSheet1 = m_Api->ActiveSheet();
    
    m_Api->UndoInsertRangeNamed("NAMED_RANGE", "B2C3");
    
    tSheet* wSheet2 = m_Api->AddSheet("Sheet2");
    m_Api->ActiveSheet(wSheet2->Name());
    
    tVariant wVariant = "=Sheet1!B1+Sheet1!B9";
    m_Api->CellValue(2, 2, wVariant);
    
    wVariant = "=SUM(Sheet1!B2:C3)";
    m_Api->CellValue(5, 4, wVariant);
    
    wVariant = "=SUM(Sheet1!A2:D3)";
    m_Api->CellValue(5, 5, wVariant);
    
    
    // Test Recover col
    wVariant = "=SUM(Sheet1!B2:D2)";
    m_Api->CellValue(5, 6, wVariant);
    
    wVariant = "=SUM(Sheet1!C2:D2)";
    m_Api->CellValue(5, 7, wVariant);
    
    wVariant = "=SUM(Sheet1!D2:D2)";
    m_Api->CellValue(5, 8, wVariant);
    
    wVariant = "=SUM(C4:E4)";
    m_Api->CellValue(4, 6, wVariant);
    
    wVariant = "=SUM(D4:E4)";
    m_Api->CellValue(4, 7, wVariant);
    
    // Test Recover row
    wVariant = "=SUM(Sheet1!B2:D6)";
    m_Api->CellValue(6, 6, wVariant);
    wVariant = "=A12+SUM(Sheet1!B3:D6)";
    m_Api->CellValue(6, 7, wVariant);
    wVariant = "=G46+SUM(Sheet1!B4:D6)";
    m_Api->CellValue(6, 8, wVariant);
    
    // Active Sheet 1
    m_Api->ActiveSheet(wSheet1->Name());
    
    m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, 10, 10));
#ifdef checksp
    m_Api->Check();
#endif
    DrawCell("SetValues", 1, 1, 8, 10);
    m_Api->ActiveSheet(wSheet2->Name());
    DrawCell("SetValues", 1, 1, 8, 10);
    m_Api->ActiveSheet(wSheet1->Name());
    DebugRange();
    
    m_Api->UndoDeleteRow(2, 2);
    
    DrawCell("After", 1, 1, 8, 10);
    m_Api->ActiveSheet(wSheet2->Name());
    DrawCell("After", 1, 1, 8, 10);
    m_Api->ActiveSheet(wSheet1->Name());
    
    DebugRange();
#ifdef checksp
    m_Api->Check();
#endif
    m_Api->Undo();
 
	DrawCell("Undo", 1, 1, 8, 10);
	m_Api->ActiveSheet(wSheet2->Name());
	DrawCell("Undo", 1, 1, 8, 10);
	m_Api->ActiveSheet(wSheet1->Name());
	DebugRange();
#ifdef checksp
	m_Api->Check();
#endif
	DrawCell("UndoDeleteCol 2,2", 1, 1, 8, 10);
	m_Api->ActiveSheet(wSheet2->Name());
	DrawCell("UndoDeleteCol 2, 2", 1, 1, 8, 10);
	m_Api->ActiveSheet(wSheet1->Name());
    
    m_Api->UndoDeleteCol(2, 2);
	
	DrawCell("After", 1, 1, 8, 10);
	m_Api->ActiveSheet(wSheet2->Name());
	DrawCell("After", 1, 1, 8, 10);
	m_Api->ActiveSheet(wSheet1->Name());

	DebugRange();
#ifdef checksp
    m_Api->Check();
#endif
	m_Api->Undo();
	DrawCell("Undo", 1, 1, 8, 10);
	m_Api->ActiveSheet(wSheet2->Name());
	DrawCell("Undo", 1, 1, 8, 10);
	m_Api->ActiveSheet(wSheet1->Name());
	DebugRange();
#ifdef checksp
	m_Api->Check();
#endif
}

void TestSkSheet::TestDeleteSheet() {
	Fill();
	tSheet* wSheet1 = m_Api->ActiveSheet();
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, 10, 10));
    
    m_Api->UndoInsertRangeNamed("COUCOU", "A1:A3");
    
	tSheet* wSheet2 = m_Api->AddSheet("Sheet2");
	m_Api->ActiveSheet(wSheet2->Name());

	tVariant wVariant = "=Sheet1!B1+Sheet1!B9";
	m_Api->CellValue(2, 2, wVariant);

	wVariant = "=SUM(Sheet1!B2:C3)";
	m_Api->CellValue(5, 4, wVariant);

	wVariant = "=SUM(Sheet1!A2:D3)";
	m_Api->CellValue(5, 5, wVariant);

	// Test Recover col
	wVariant = "=SUM(Sheet1!B2:D2)";
	m_Api->CellValue(5, 6, wVariant);

	wVariant = "=SUM(Sheet1!C2:D2)";
	m_Api->CellValue(5, 7, wVariant);

	wVariant = "=SUM(Sheet1!D2:D2)";
	m_Api->CellValue(5, 8, wVariant);

	wVariant = "=SUM(C4:E4)";
	m_Api->CellValue(4, 6, wVariant);

	wVariant = "=SUM(D4:E4)";
	m_Api->CellValue(4, 7, wVariant);

	// Test Recover row
	wVariant = "=SUM(Sheet1!B2:D6)";
	m_Api->CellValue(6, 6, wVariant);
	wVariant = "=A12+SUM(Sheet1!B3:D6)";
	m_Api->CellValue(6, 7, wVariant);
	wVariant = "=G46+SUM(Sheet1!B4:D6)";
	m_Api->CellValue(6, 8, wVariant);
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, 10, 10));

	m_Api->ActiveSheet(wSheet1->Name());
	DrawCell("Before Delete Sheet 1", 1, 1, 8, 10);
	m_Api->ActiveSheet(wSheet2->Name());
	DrawCell("Before Delete Sheet 1", 1, 1, 8, 10);

    CPPUNIT_ASSERT_MESSAGE("UndoDeleteSheet", m_Api->UndoDeleteSheet(wSheet1->Name()));
	m_Api->ActiveSheet(wSheet2->Name());
	CPPUNIT_ASSERT_MESSAGE("DeleteSheet B2 formula",
	                       m_Api->Formula(2, 2) == "#REF!+#REF!");
	CPPUNIT_ASSERT_MESSAGE("DeleteSheet D5 SUM range",
	                       m_Api->Formula(5, 4) == "SUM(#REF!:#REF!)");
	CPPUNIT_ASSERT_MESSAGE("DeleteSheet E5 SUM range",
	                       m_Api->Formula(5, 5) == "SUM(#REF!:#REF!)");
	CPPUNIT_ASSERT_MESSAGE("DeleteSheet F6 SUM range",
	                       m_Api->Formula(6, 6) == "SUM(#REF!:#REF!)");
	CPPUNIT_ASSERT_MESSAGE("DeleteSheet G6 mixed",
	                       m_Api->Formula(6, 7) == "A12+SUM(#REF!:#REF!)");
	DrawCell("After Delete Sheet 1", 1, 1, 8, 10);

#ifdef checksp
	m_Api->Check();
#endif

	m_Api->Undo();

	m_Api->ActiveSheet(wSheet1->Name());
	DrawCell("After Undo Delete Sheet 1", 1, 1, 8, 10);
	m_Api->ActiveSheet(wSheet2->Name());
	DrawCell("After Undo Delete Sheet 1", 1, 1, 8, 10);

#ifdef checksp
	m_Api->Check();
#endif
}

void TestSkSheet::TestMoveToCell() {
    tIndex wCol=1;
    // Test Row ==================================================================
    for(tIndex wRow=3;wRow<=10; wRow++) {
        tStringStream wStream;
        wStream << Base10ToAlpha(wCol) << wRow;
        m_Api->CellValue(wStream.str(),"Row"+wStream.str());
#ifdef printdebug
        cout << wStream.str() << ",";
#endif
    }
    
    m_Api->CellValue(13,1,"CouCou !!");
#ifdef printdebug
    cout << Base10ToAlpha(1) << 13 << endl;
#endif
    tPoint wPoint(1,1);
    wPoint=m_Api->MoveToCell(wPoint, 4);
#ifdef printdebug
    cout << endl;
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell A3",  wPoint.StrRef()  == "A3");
    wPoint=m_Api->MoveToCell(wPoint, 4);
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell A11",  wPoint.StrRef()  == "A11");
    wPoint=m_Api->MoveToCell(wPoint, 4);
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell A13",  wPoint.StrRef()  == "A13");
    wPoint=m_Api->MoveToCell(wPoint, 4);
    
    wPoint=m_Api->MoveToCell(wPoint,4);
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell A1048576",  wPoint.StrRef()  == "A1048576");
// Up========================================================================
    wPoint=m_Api->MoveToCell(wPoint, 2);
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell A13",  wPoint.StrRef()  == "A13");
 
    wPoint=m_Api->MoveToCell(wPoint, 2);
    wPoint=m_Api->MoveToCell(wPoint, 2);
    
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell A10",  wPoint.StrRef()  == "A10");

    wPoint=m_Api->MoveToCell(wPoint, 2);
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell A2",  wPoint.StrRef()  == "A2");
 
    wPoint=m_Api->MoveToCell(wPoint, 2);
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell A1",  wPoint.StrRef()  == "A1");

    // Test Col ===============================================================
    tIndex wRow=1;
    for(tIndex wCol=3;wCol<=10; wCol++) {
        tStringStream wStream;
        wStream << Base10ToAlpha(wCol) << wRow;
        m_Api->CellValue(wStream.str(),"Col"+wStream.str());
#ifdef printdebug
        cout << wStream.str() << ",";
#endif
    }
    
    m_Api->CellValue(1,13,"CouCou !!");
#ifdef printdebug
    cout << Base10ToAlpha(13) << 1 << endl;
#endif
    wPoint.Row(1);
    wPoint.Col(1);
    wPoint=m_Api->MoveToCell(wPoint, 3);
#ifdef printdebug
    cout << endl;
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell C1",  wPoint.StrRef()  == "C1");
    wPoint=m_Api->MoveToCell(wPoint, 3 );
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell K1",  wPoint.StrRef()  == "K1");
    wPoint=m_Api->MoveToCell(wPoint, 3);
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell M1",  wPoint.StrRef()  == "M1");
    wPoint=m_Api->MoveToCell(wPoint, 3);
    wPoint=m_Api->MoveToCell(wPoint, 3);
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell XFD1",  wPoint.StrRef()  == "XFD1");
// Up========================================================================
    wPoint=m_Api->MoveToCell(wPoint, 1);
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell M1",  wPoint.StrRef()  == "M1");
 
    wPoint=m_Api->MoveToCell(wPoint, 1);
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell L1",  wPoint.StrRef()  == "L1");

    wPoint=m_Api->MoveToCell(wPoint, 1);
    wPoint=m_Api->MoveToCell(wPoint, 1);
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell B1",  wPoint.StrRef()  == "B1");
 
    wPoint=m_Api->MoveToCell(wPoint, 1);
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell A1",  wPoint.StrRef()  == "A1");

    m_Api->CellValue(1,1,"CouCou !!");
    m_Api->CellValue(2,1,"CouCou !!");
  
    wPoint.Col(1);
    wPoint.Row(9);
    
    wPoint=m_Api->MoveToCell(wPoint, 2);
#ifdef printdebug
    cout << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("TestMove to cell A1",  wPoint.StrRef()  == "A1");

    
    // Test Bottom Right ======================================================
    wPoint=m_Api->BottomRight();
#ifdef printdebug
    cout << "Bottom/Right :" << wPoint.StrRef() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Test Bottom/Right Cell M13",  wPoint.StrRef()  == "M13");
    
  
}

void TestSkSheet::TestJsonView() {
    // Left
    m_Api->CellValue("A1", "A1");
   
    m_Api->CellValue("D1","D1");
   
    // Right
    m_Api->CellValue("H1", "H1");
    
    tString wResult=m_Api->JsonColByPixel(1, 20);
    //cout << endl << "ColByPixel=" << wResult << endl;;
    
    //cout << "Row A=" << m_Api->ActiveSheet()->Row(1)->Size() << endl;
    
    for(int i=1;i<100;i++) {
        wResult=m_Api->JsonRowByPixel(1, i);
        //cout << "RowByPixel(" << i << ")=" << wResult << endl;;
    }
    wResult=m_Api->JsonView(1, 3, tUnitMetrics::pixels, 12, 300,-10,-2, true);
    
    //cout << endl << wResult << endl;
    
    Document wDocument;
    wDocument.Parse(wResult.c_str());
    
    Value& wName=wDocument["sheet"];
    //cout << wName.GetString() << endl;
    CPPUNIT_ASSERT_MESSAGE("TestJsonView sheet", tString(wName.GetString())  == "Sheet1");
    
    Value& wRows=wDocument["rows"];
    CPPUNIT_ASSERT_MESSAGE("TestJsonView rows", wRows.IsArray());
    
    Value& wRow=wRows[0];
    Value& wCells=wRow["cells"];
    CPPUNIT_ASSERT_MESSAGE("TestJsonView cells", wCells.IsArray());
    
    DrawCell("Undo", 1, 3, 1, 4);
    
}

void TestSkSheet::TestReadWriteJson() {
    const tString wUri="www.test.com";
    m_Api->AddWorkBook(wUri);
    m_Api->ActiveWorkBook(wUri);
    m_Api->AddSheet("Stéphane");
    m_Api->AddSheet("Allez");
    m_Api->UndoCellValue("A1","Coucou Stéphane");
    
    tString wJson=m_Api->WriteJson(wUri);
    tearDown();
    setUp();
    
    m_Api->ReadJson(wJson);
    m_Api->ActiveWorkBook(wUri);
    tString wResult= "{\"list\":[\"Stéphane\",\"Allez\"]}";
    //cout << m_Api->JsonSheets() << endl;
    CPPUNIT_ASSERT_MESSAGE("TestReadWrite ", wResult==m_Api->JsonSheets());

}
void TestSkSheet::setUp() {
	std::filesystem::remove_all("./Spreadsheet");

	m_Application = tApplication::Instance();
	m_Api = new tApi;
	m_Api->IsUndoActif(true);
	m_Api->NewWorkBook("wwww.skeema.fr/w1");
};

void TestSkSheet::tearDown() {
	delete(m_Api);
}

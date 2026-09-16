//==============================================================================
// TestSkCell
//==============================================================================
#include "../include/TestSkCell.hpp"
#include <SkInterfaceWeb.hpp>

// We can send it to the API of a feature 
TestSkCell::TestSkCell() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
}

void TestSkCell::DrawCell(tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
	return; // Drop
	cout << endl;
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
		for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
			tVariant wVariant = m_Api->CellValue(wRow, wCol);
			tString wFormula = m_Api->Formula(wRow, wCol);
			cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant << "\t";
		}
		cout << endl;
	}
}

void TestSkCell::UndoOperation() {
	return; // Drop
	tUndo* wUndo = m_Api->LastUndo();
	if (wUndo != nullptr) {
		cout << "Undo ->" << wUndo->OperationName() << endl;
	}
}

void TestSkCell::TestFormula() {
	// Test priority =========================================================
    tString wFormula = "=5*3+1";
    m_Api->UndoCellValue("A2", wFormula);
    tCell* wCell = m_Api->Cell("A2");
    CPPUNIT_ASSERT_MESSAGE("5*3+1", wCell->Value().Int()==16);
	wFormula = "=5*-3+1";
    m_Api->UndoCellValue("A2", wFormula);
    wCell->InternalCalculation();
    CPPUNIT_ASSERT_MESSAGE("5*-3+1", wCell->Value().Int()==-14);
	wFormula = "=5*-3+-1";
    m_Api->UndoCellValue("A2", wFormula);
    CPPUNIT_ASSERT_MESSAGE("5*-3+-1", wCell->Value().Int()==-16);
	wFormula = "=5*-3--1";
    m_Api->UndoCellValue("A2", wFormula);
    CPPUNIT_ASSERT_MESSAGE("5*-3--1", wCell->Value().Int()==-14);
	wFormula = "=5*-3--1";
    m_Api->UndoCellValue("A2", wFormula);
    CPPUNIT_ASSERT_MESSAGE("5*-3--1", wCell->Value().Int()==-14);
}

void TestSkCell::TestCalculate() {
    m_Api->UndoCellValue("A1", 1);
    // Test priority =========================================================
    m_Api->UndoCellValue("A2",2);
    m_Api->UndoCellValue("A3",3);
	
    m_Api->UndoCellValue("A5", "=SUM(A1:A3)");
    tCell* wCell=m_Api->Cell("A5");
    CPPUNIT_ASSERT_MESSAGE("Test Calculate SUM(A1:A3)", wCell->Value().Int()==6);
    
    // Imbrication
    m_Api->UndoCellValue("A6", "=SUM(A1:A3;A1:A5)");
    wCell=m_Api->Cell("A6");
	CPPUNIT_ASSERT_MESSAGE("Test Calculate SUM(A1:A3;SUM(A1:A5))", wCell->Value().Int()==18);
    //cout << endl << wCell->Debug();
    
    
    
	tInt wNbRow = 8;
	tInt wNbCol = 10;

	for (tInt wCol = 1; wCol <= wNbCol; wCol++) {
		if (wCol > 1) {
			wCell = m_Api->EnsureCell(1, wCol);
			tStringStream wStream;
			wStream << Base10ToAlpha(wCol - 1) << wNbRow - 1 << "+1";
            tString wCompilString=wStream.str();
            if (!m_Api->CompilCell(wCell,wCompilString.c_str())) {
                tStringStream wStreamError;
                wStreamError << "Test Calculate Compil error " << wStream.str() << " " << wCompilString;
                CPPUNIT_ASSERT_MESSAGE(wStreamError.str(), false);
            }
		}
	}

	for (tInt wRow = 2; wRow <= wNbRow; wRow++) {
		for (tInt wCol = 1; wCol <= wNbCol; wCol++) {
			wCell = m_Api->EnsureCell(wRow, wCol);
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
            tString wCompilString=wStream.str();
            if (!m_Api->CompilCell(wCell,wCompilString.c_str())) {
                tStringStream wStreamError;
                wStreamError << "Test Calculate Compil error " << wStream.str() << " " << wCompilString;
                CPPUNIT_ASSERT_MESSAGE(wStreamError.str(), false);
            }
#ifdef checksp
			wCell->Check();
#endif
		}
	}

#ifdef checksp	
	m_Api->Check();
#endif
	wCell= m_Api->EnsureCell(1, 1);
#ifdef checksp
	wCell->Check();
#endif // checksp
	wCell->Calculation();

	DrawCell(1, 1, wNbRow, wNbCol);
	tVariant wResult = m_Api->CellValue(8, 10);
	CPPUNIT_ASSERT_MESSAGE("CellCalculate ", wResult.Int() == 2016);

	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, wNbRow, wNbCol));
	DrawCell(1, 1, wNbRow, wNbCol);
	wResult=m_Api->CellValue(8, 10);
    CPPUNIT_ASSERT_MESSAGE("CellCalculate Rect ", wResult.Int() == 2016);
	//J8 = 1953
}

void TestSkCell::TestBlockedAcyclicSubgraphResolve() {
	// Mutual dependency with a *unique* fixed point (linear system):
	//   A1 = 1 + B1/2,  B1 = A1/2  =>  A1 = 4/3, B1 = 2/3.
	// Pairs like A1=10+B1 & B1=A1-10 are underdetermined (e.g. (0,-10) and (10,0) both work), so the engine
	// may converge to a different fixed point depending on evaluation order.
	// Reduce() may stall on m_NbDepend; ResolveBlockedAcyclicSubgraph() SCC iteration should converge without #RECURSIVE.
	tCell* wA1 = m_Api->EnsureCell(1, 1);
	tCell* wB1 = m_Api->EnsureCell(1, 2);
	CPPUNIT_ASSERT_MESSAGE("Compil A1=1+B1/2", m_Api->CompilCell(wA1, "1+B1/2"));
	CPPUNIT_ASSERT_MESSAGE("Compil B1=A1/2", m_Api->CompilCell(wB1, "A1/2"));
	wA1->Calculation();

	CPPUNIT_ASSERT_MESSAGE("A1 must not be #RECURSIVE after blocked-acyclic resolve",
		wA1->Value().Type() != tVariantType::t_error);
	CPPUNIT_ASSERT_MESSAGE("B1 must not be #RECURSIVE after blocked-acyclic resolve",
		wB1->Value().Type() != tVariantType::t_error);

	const tDouble wTol = 1e-9;
	const tDouble wExpectA1 = 4.0 / 3.0;
	const tDouble wExpectB1 = 2.0 / 3.0;
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("A1 unique fixed point (4/3)", wExpectA1, wA1->Value().Numeric(), wTol);
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("B1 unique fixed point (2/3)", wExpectB1, wB1->Value().Numeric(), wTol);

	// Downstream cell depending on both; full sheet Calculate should stay consistent.
	tCell* wC1 = m_Api->EnsureCell(1, 3);
	CPPUNIT_ASSERT_MESSAGE("Compil C1=A1+B1", m_Api->CompilCell(wC1, "A1+B1"));
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, 1, 3));
	CPPUNIT_ASSERT_MESSAGE("C1 must not be #RECURSIVE", wC1->Value().Type() != tVariantType::t_error);
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("C1 = A1+B1 (=2)", 2.0, wC1->Value().Numeric(), wTol);
}

void TestSkCell::TestCell() {
    tVariant wVariant="Hello World !";
	m_Api->CellValue(0, 0, wVariant);
	tVariant wVariantResult = m_Api->CellValue(0, 0);
	CPPUNIT_ASSERT_MESSAGE(wVariantResult.String(), wVariant.String() == "Hello World !");

    wVariant.Parse("31/12/2024");
    m_Api->CellValue("A9", wVariant);
    CPPUNIT_ASSERT_MESSAGE("Test Date", wVariant.Type()==tVariantType::t_date);
    
        
	wVariant = 12;
	m_Api->CellValue("A9", wVariant);

	wVariant = "=SUM(D1:D10)";
	m_Api->CellValue("E1", wVariant);

    tCell* wCell=m_Api->Cell("E1");
    
    wCell->InternalCalculation();
    //cout << endl << wCell->Debug() << endl;
	wVariant = "=A9";
	m_Api->CellValue("D1:D3", wVariant);

	wVariant = 48;
	m_Api->CellValue("D1:D3", wVariant);


	wVariant = "=SUM(A1:J9)";
	m_Api->CellValue("A10", wVariant);


	DrawCell(1, 1, 10, 10);
    
  
    
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, 10, 10));
	DrawCell(1, 1, 10, 10);

	wCell = m_Api->Cell(10, 1);
	wCell->Calculation();
	DrawCell(1, 1, 10, 10);
	wVariantResult = m_Api->CellValue(10, 1);
	CPPUNIT_ASSERT_MESSAGE("TestCell A10=300", wVariantResult.Int() == 300);

#ifdef checksp
	m_Api->Check();
#endif

	wVariant = "=A9";
	m_Api->UndoCellValue("D1:D5;D7", wVariant);
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, 10, 10));
	DrawCell(1, 1, 10, 10);
	wVariantResult = m_Api->CellValue(10, 1);
	CPPUNIT_ASSERT_MESSAGE("TestCell A10=156", wVariantResult.Int() == 156);

    
	wVariant = "=A9+1";
	m_Api->UndoCellValue("A8;B1:B10;E7", wVariant);
	DrawCell(1, 1, 10, 10);
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, 10, 10));
	DrawCell(1, 1, 10, 10);
	wVariantResult = m_Api->CellValue(10, 1);
	CPPUNIT_ASSERT_MESSAGE("TestCell A10=299", wVariantResult.Int() == 299);

#ifdef checksp
	m_Api->Check();
#endif
	for (tInt wRow = 0; wRow < 20; wRow++) {
		for (tInt wCol = 0; wCol < 20; wCol++) {
            wVariant=wRow * 10 + wCol;
			m_Api->CellValue(wRow, wCol, wVariant);
		}
	}
	//cout << "end --------------------------------" << endl;
	for (tInt wRow = 0; wRow < 10; wRow++) {
		for (tInt wCol = 0; wCol < 10; wCol++) {
			tVariant wVariant = m_Api->CellValue(wRow, wCol);
			tVariant wVariantResult = (wRow * 10 + wCol);
			tStringStream wStream;
			wStream << "(" << wRow << "," << wCol << ")";
			CPPUNIT_ASSERT_MESSAGE("Test Cells correspondence" + wStream.str() + " ! ", wVariant == wVariantResult);
		}
	}
}

void TestSkCell::TestUndoCell() {

	tVariant wVariant = 12;
    
    m_Api->UndoCellValue("A10",wVariant);
    
    
    m_Api->Undo();

    m_Api->Redo();
    
    m_Api->Undo();
    m_Api->Redo();
    
    wVariant = "=A10+1";
	m_Api->UndoCellValue("A5", wVariant);
	tCell* wCell = m_Api->Cell(5, 1);
	wCell->Calculation();
	
	wVariant = "=A10";
	m_Api->UndoCellValue("D1;D2;D3;D4", wVariant);
	DrawCell(1, 1, 10, 10);

    //m_Api->ActiveSheet()->Debug();
    
	tVariant wVariantResult = m_Api->CellValue(1, 4); CPPUNIT_ASSERT_MESSAGE("TestCell D1;D2;D3;D4=A10", wVariantResult.Int() == 12);


	wVariant = "=A9+1";
	m_Api->UndoCellValue("C10;B1:B11;E7", wVariant);
	DrawCell(1, 1, 10, 10);
#ifdef checksp
	m_Api->Check();
#endif
	UndoOperation();
	m_Api->Undo();
	DrawCell(1, 1, 10, 10);
	wVariantResult = m_Api->CellValue(10, 1);
	CPPUNIT_ASSERT_MESSAGE("TestCell A1=12", wVariantResult.Int() == 12);
	wVariantResult = m_Api->CellValue(1, 2); 
	CPPUNIT_ASSERT_MESSAGE("TestCell B1=Empty", wVariantResult.Type()==tVariantType::t_null);
	
	wVariantResult = m_Api->CellValue(2, 2); CPPUNIT_ASSERT_MESSAGE("TestCell B2=Empty", wVariantResult.Type() == tVariantType::t_null);
	wVariantResult = m_Api->CellValue(3, 2); CPPUNIT_ASSERT_MESSAGE("TestCell B3=Empty", wVariantResult.Type() == tVariantType::t_null);
	wVariantResult = m_Api->CellValue(4, 2); CPPUNIT_ASSERT_MESSAGE("TestCell B4=Empty", wVariantResult.Type() == tVariantType::t_null);
	wVariantResult = m_Api->CellValue(5, 2); CPPUNIT_ASSERT_MESSAGE("TestCell B5=Empty", wVariantResult.Type() == tVariantType::t_null);
	wVariantResult = m_Api->CellValue(6, 2); CPPUNIT_ASSERT_MESSAGE("TestCell B6=Empty", wVariantResult.Type() == tVariantType::t_null);
	wVariantResult = m_Api->CellValue(7, 2); CPPUNIT_ASSERT_MESSAGE("TestCell B7=Empty", wVariantResult.Type() == tVariantType::t_null);
	wVariantResult = m_Api->CellValue(8, 2); CPPUNIT_ASSERT_MESSAGE("TestCell B8=Empty", wVariantResult.Type() == tVariantType::t_null);
	wVariantResult = m_Api->CellValue(9, 2); CPPUNIT_ASSERT_MESSAGE("TestCell B9=Empty", wVariantResult.Type() == tVariantType::t_null);
	wVariantResult = m_Api->CellValue(10, 2); CPPUNIT_ASSERT_MESSAGE("TestCell B10=Empty", wVariantResult.Type() == tVariantType::t_null);

	wVariantResult = m_Api->CellValue(7, 5); CPPUNIT_ASSERT_MESSAGE("TestCell E7=Empty", wVariantResult.Type() == tVariantType::t_null);

#ifdef checksp
	m_Api->Check();
#endif
	UndoOperation();
	m_Api->Undo();
	DrawCell(1, 1, 10, 10);
	wVariantResult = m_Api->CellValue(1, 4); CPPUNIT_ASSERT_MESSAGE("TestCell D1=Empty", wVariantResult.Type() == tVariantType::t_null);
	wVariantResult = m_Api->CellValue(2, 4); CPPUNIT_ASSERT_MESSAGE("TestCell D2=Empty", wVariantResult.Type() == tVariantType::t_null);
	wVariantResult = m_Api->CellValue(3, 4); CPPUNIT_ASSERT_MESSAGE("TestCell D3=Empty", wVariantResult.Type() == tVariantType::t_null);
	wVariantResult = m_Api->CellValue(4, 4); CPPUNIT_ASSERT_MESSAGE("TestCell D4=Empty", wVariantResult.Type() == tVariantType::t_null);

#ifdef checksp
	m_Api->Check();
#endif
	UndoOperation();
 	m_Api->Redo();
 	DrawCell(1, 1, 10, 10);

	wVariantResult = m_Api->CellValue(1, 4); CPPUNIT_ASSERT_MESSAGE("TestCell D1=12", wVariantResult.Int() == 12);
	wVariantResult = m_Api->CellValue(2, 4); CPPUNIT_ASSERT_MESSAGE("TestCell D2=12", wVariantResult.Int() == 12);
	wVariantResult = m_Api->CellValue(3, 4); CPPUNIT_ASSERT_MESSAGE("TestCell D3=12", wVariantResult.Int() == 12);
	wVariantResult = m_Api->CellValue(4, 4); CPPUNIT_ASSERT_MESSAGE("TestCell D4=12", wVariantResult.Int() == 12);

#ifdef checksp
	m_Api->Check();
#endif
	wVariant = "=2+D1";
	m_Api->UndoCellValue("A1:A9;C1:C10", wVariant);
#ifdef checksp
    m_Api->Check();
#endif
	DrawCell(1, 1, 10, 10);

	//m_Api->Undo();
	//m_DrawCell(1, 1, 10, 10);

	m_Api->UndoRaz("A1:A9;C1:C10");
	DrawCell(1, 1, 10, 10);

	UndoOperation();
	m_Api->Undo();
	DrawCell(1, 1, 10, 10);

#ifdef checksp
	m_Api->Check();
#endif


	UndoOperation();
	m_Api->Undo();
	DrawCell(1, 1, 10, 10);
#ifdef checksp
	m_Api->Check();
#endif

	UndoOperation();
	m_Api->Undo();
	wVariant = m_Api->CellValue(5, 1);
	DrawCell(1, 1, 10, 10);
	wVariantResult = m_Api->CellValue(5, 1); 
	CPPUNIT_ASSERT_MESSAGE("TestCell A5=13", wVariantResult.Int() == 13);

#ifdef checksp
	m_Api->Check();
#endif
	UndoOperation();
	m_Api->Undo();
	UndoOperation();
	m_Api->Undo();
	UndoOperation();
	m_Api->Undo();

	DrawCell(1, 1, 10, 10);


#ifdef checksp
	m_Api->Check();
#endif

	//cout << "end --------------------------------" << endl;
	for (tInt wRow = 1; wRow < 10; wRow++) {
		for (tInt wCol = 0; wCol < 10; wCol++) {
			tVariant wVariant = m_Api->CellValue(wRow, wCol);
			tStringStream wStream;
			wStream << "(" << wRow << "," << wCol << ":" << wVariant << ")";
			CPPUNIT_ASSERT_MESSAGE("Test Cell null" + wStream.str() + " ! ", wVariant.Type()==tVariantType::t_null);
		}
	}
    
    // SyntaxError =========================================================
    wVariant=12;
    m_Api->UndoCellValue("A10",wVariant);
    
    tCell* wCellA10=m_Api->Cell("A10");
    CPPUNIT_ASSERT_MESSAGE("TestCell A10", wCellA10 != nullptr);

    //cout << endl << "A10=" << wA10->Value() <<endl;
    wVariant = "=n'importe quoi ?";
    tBool  wResult=m_Api->UndoCellValue("A10", wVariant);
    CPPUNIT_ASSERT_MESSAGE("Test error syntax  ! ",!wResult);
}

void TestSkCell::TestLastColRow() {
    
}

void TestSkCell::TestDate() {
    tVariant wVariant;
    
    tString wTitle = "TestDate";
    tString wSubTitle = "UndoCellValue 31/12/2024 10:30 ";
   
    wVariant.Parse("31/12/2024 10:30");
    m_Api->UndoCellValue("A1",wVariant);
    tCell* wCell=m_Api->Cell("A1");
    tClassDate ClassDateA1(wCell->Value().Date());
    
    tString wResult=ClassDateA1.FormatDateTime("%d-%m-%Y %H:%M:%S");
    //cout << endl << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wResult=="31-12-2024 10:30:00");
    
  
    wSubTitle = "UndoCellValue duration 01::30:02 ";
    wVariant.Parse("01/01/1970 01:30:01");
    m_Api->UndoCellValue("B1",wVariant);
    
    wCell=m_Api->Cell("B1");
    tClassDate ClassDateB1(wCell->Value().Date());
    wResult=ClassDateB1.FormatDateTime("%d-%m-%Y %H:%M:%S");
    //cout << wResult <<endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wResult =="01-01-1970 01:30:01");
    
    wSubTitle = "UndoCellValue Calcul Date  A1+B1";
    m_Api->UndoCellValue("C1","=A1+B1");
    wCell=m_Api->Cell("C1");
    tClassDate ClassDateC1(wCell->Value().Date());
    wResult=ClassDateC1.FormatDateTime("%d-%m-%Y %H:%M:%S");
    //cout << wResult <<endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wResult =="31-12-2024 12:00:01");
    
    wSubTitle = "UndoCellValue Calcul Duration B1+D1";
    wVariant.Parse("02/01/1970 00:00:01");
    m_Api->UndoCellValue("D1",wVariant);
    m_Api->UndoCellValue("D2","=B1+D1");
    wCell=m_Api->Cell("D2");
    tClassDate ClassDateD2(wCell->Value().Date());
    wResult=ClassDateD2.FormatDateTime("%d-%m-%Y %H:%M:%S");
    //cout << wResult <<endl;
    CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wResult =="02-01-1970 01:30:02");
    
    
    wSubTitle = "Copy A1";
    m_Api->Copy("A1");

    m_Api->UndoPaste("A2");
    tCell* wCellA2= m_Api->Cell("A2");
    tClassDate wDate(wCellA2->Value().Date());
    wResult=wDate.FormatDateTime("%d-%m-%Y %H:%M:%S");
    
    //cout << wResult;
    // False Parse US with
    //CPPUNIT_ASSERT_MESSAGE(wTitle + " " + wSubTitle, wResult =="02-01-1970 01:30:02");
    
    
    //CPPUNIT_ASSERT_MESSAGE("TestCopy B2  C9=135", wDate.UsDate() == "2024-12-31");
}

void TestSkCell::TestPostMessage() {
    tInterfaceWeb* wInterfaceWeb=new tInterfaceWeb();
    wInterfaceWeb->Client(true);
    wInterfaceWeb->NewWorkBook("wwww.skeema.fr/web");
    wInterfaceWeb->User("sallez&skeema.fr", "Allez", "Stéphane");
    
    wInterfaceWeb->UndoCellValue("A1", "Coucou");
    
    delete(wInterfaceWeb);
}

void TestSkCell::setUp() {
	std::filesystem::remove_all("./Spreadsheet");

	m_Application = tApplication::Instance();
    m_Application->Locale("fr");
	m_Api = new tApi;
	m_Api->NewWorkBook("wwww.skeema.fr/w1");
};

void TestSkCell::tearDown() {
	delete(m_Api);
}

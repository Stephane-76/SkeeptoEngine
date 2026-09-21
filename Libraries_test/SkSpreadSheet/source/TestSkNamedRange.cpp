//==============================================================================
// TestSkJon
// Test la librairie SparseArray
//==============================================================================

#include "../include/TestSkNamedRange.hpp"

// We can send it to the API of a feature 
TestSkRangeNamed::TestSkRangeNamed() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
}

void TestSkRangeNamed::DrawCell(tString sTitle,tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
	return; // Drop 
	cout << sTitle << endl;
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
		for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
			tVariant wVariant = m_Api->CellValue(wRow, wCol);
			tString wFormula = m_Api->Formula(wRow, wCol);
			cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant << "\t";
		}
		cout << endl;
	}
}


void TestSkRangeNamed::Fill() {
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
	DrawCell("",1, 1, m_NbRow, m_NbCol);
}


void TestSkRangeNamed::TestFormula(tCell* sCell, tString sFormula) {
	tVariant wFormula = "="+sFormula;
	if (m_Api->UndoCellValue(sCell->StrRef(), wFormula)) {
	}
	CPPUNIT_ASSERT_MESSAGE("TestFormula) " + sFormula + " !", sCell->FormulaStr() == sFormula);
}


void TestSkRangeNamed::Name() {
	m_Api->UndoInsertRangeNamed("ALLEZ", "A1:A2");
	tCell* wCell = m_Api->EnsureCell(1, 1);
	tVariant wVariant = 12;
	m_Api->UndoCellValue(wCell->StrRef(), wVariant);
#ifdef checksp	
	m_Api->Check();
#endif
	wCell = m_Api->EnsureCell(2, 2);
	TestFormula(wCell, "SUM(ALLEZ)+1");


	CPPUNIT_ASSERT_MESSAGE("TestName 1", wCell->FormulaStr() == "SUM(ALLEZ)+1");

	CPPUNIT_ASSERT_MESSAGE("TestName 2", wCell->Value().Int() == 13);
 
#ifdef checksp	
	m_Api->Check();
#endif
	DrawCell("Before UndoDeleteRangeNamed", 1, 1, 4, 5);

	m_Api->UndoDeleteRangeNamed("ALLEZ");

	DrawCell("After UndoDeleteRangeNamed", 1, 1, 4, 5);

#ifdef checksp	
	m_Api->Check();
#endif

	CPPUNIT_ASSERT_MESSAGE("TestName 3", wCell->FormulaStr() == "SUM(#NAME?)+1");

	CPPUNIT_ASSERT_MESSAGE("TestName 4", wCell->Value().Error().Code() == tTypeError::t_ref);

	m_Api->Undo();
 
	CPPUNIT_ASSERT_MESSAGE("TestName5  !", wCell->FormulaStr() == "SUM(ALLEZ)+1");

	CPPUNIT_ASSERT_MESSAGE("TestName6  !", wCell->Value().Int() == 13);


	DrawCell("After Undo ", 1, 1, 4, 5);


#ifdef checksp	
	m_Api->Check();
#endif
	m_Api->UndoInsertRangeNamed("Stéphane", "A1:A2");
    
    TestFormula(wCell, "SUM(Stéphane)+1");
    wCell->InternalCalculation();
    //cout << wCell->FormulaStr() << ":" << wCell->Value() <<  endl;
    
    CPPUNIT_ASSERT_MESSAGE("TestName 6", wCell->FormulaStr() == "SUM(Stéphane)+1");

    CPPUNIT_ASSERT_MESSAGE("TestName 7", wCell->Value().Int() == 13);

	DrawCell("UndoInsertRangeNamed Stéphane", 1, 1, 4, 5);
	m_Api->Undo();
	DrawCell("After Undo ", 1, 1, 4, 5);

	m_Api->UndoInsertRangeNamed("Anne Sophie","B1:B2");
	m_Api->UndoInsertRangeNamed("Thomas","C1:C2");
	m_Api->UndoInsertRangeNamed("Luc","D1:D2");
	m_Api->UndoInsertRangeNamed("David","E1:E2");
 
    tString wJsonNamed=m_Api->JsonRangeNamed();
    //cout << endl << wJsonNamed << endl;
    
    // Test that all named ranges are present in the JSON
    tString wExpectedJson = "{\"namedranges\":[{\"n\":\"Thomas\",\"s\":\"Sheet1\",\"r\":\"C1:C2\"},{\"n\":\"David\",\"s\":\"Sheet1\",\"r\":\"E1:E2\"},{\"n\":\"Anne Sophie\",\"s\":\"Sheet1\",\"r\":\"B1:B2\"},{\"n\":\"Luc\",\"s\":\"Sheet1\",\"r\":\"D1:D2\"},{\"n\":\"Stéphane\",\"s\":\"Sheet1\",\"r\":\"A1:A2\"}]}";
    
    // Verify that the JSON contains all expected named ranges
    CPPUNIT_ASSERT_MESSAGE("TestName 11: JSON should contain Thomas", wJsonNamed.find("\"n\":\"Thomas\"") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestName 12: JSON should contain David", wJsonNamed.find("\"n\":\"David\"") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestName 13: JSON should contain Anne Sophie", wJsonNamed.find("\"n\":\"Anne Sophie\"") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestName 14: JSON should contain Luc", wJsonNamed.find("\"n\":\"Luc\"") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestName 15: JSON should contain Stéphane", wJsonNamed.find("\"n\":\"Stéphane\"") != tString::npos);
    
    // Verify ranges
    CPPUNIT_ASSERT_MESSAGE("TestName 16: Thomas should have range C1:C2", wJsonNamed.find("\"r\":\"C1:C2\"") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestName 17: David should have range E1:E2", wJsonNamed.find("\"r\":\"E1:E2\"") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestName 18: Anne Sophie should have range B1:B2", wJsonNamed.find("\"r\":\"B1:B2\"") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestName 19: Luc should have range D1:D2", wJsonNamed.find("\"r\":\"D1:D2\"") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestName 20: Stéphane should have range A1:A2", wJsonNamed.find("\"r\":\"A1:A2\"") != tString::npos);
    
}



void TestSkRangeNamed::setUp() {
	std::filesystem::remove_all("./Spreadsheet");
	m_Application = tApplication::Instance();
	m_NbRow = 10;
	m_NbCol = 5;
	m_Api = new tApi;
	m_Api->IsUndoActif(true);
	m_Api->NewWorkBook("wwww.skeema.fr/w1");
};

void TestSkRangeNamed::tearDown() {
	delete(m_Api);
}

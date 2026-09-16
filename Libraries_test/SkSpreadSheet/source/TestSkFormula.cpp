//==============================================================================
// TestSkFormula
// Test la librairie SparseArray
//==============================================================================

#include "../include/TestSkFormula.hpp"
#include <cmath>

#define _printdebug
// We can send it to the API of a feature
TestSkFormula::TestSkFormula() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
};

// For view thousands on count 
struct separate_thousands : std::numpunct<char> {
	char_type do_thousands_sep() const override { return '.'; }  // separate with point
	string_type do_grouping() const override { return "\3"; } // groups of 3 digit
};

struct normal_punct : std::numpunct<char> {
};

// For debugging
void TestSkFormula::DrawCell(tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
	return; // Drop
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
		for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
			tVariant wVariant = m_Api->CellValue(wRow, wCol);
			cout << wVariant << ";";
		}
		cout << endl;
	}
}

void TestSkFormula::TestFormulaA1R1C1(tCell* sCell,tString sFormula) {
	if (!m_Api->CompilCell(sCell, sFormula.c_str())) {
        CPPUNIT_ASSERT_MESSAGE("(TestFormula) don't compil "+sFormula+" !", false);
	}
    //cout << sCell->FormulaStr() << endl;
	CPPUNIT_ASSERT_MESSAGE("(TestFormula) "+sFormula+" !", sCell->FormulaStr() == sFormula);
	// Test R1C1
	tString wFormula = sCell->FormulaStr(true);
	if (m_Api->CompilCell(sCell,wFormula.c_str())) {
		// ok
		//cout << endl;
		//cout << ">" << sCell->FormulaStr() << "<" << endl;
		//cout << ">" << sFormula << "<" << endl;
		CPPUNIT_ASSERT_MESSAGE("(TestFormula) " + sFormula + " !", sCell->FormulaStr() == sFormula);
	}
}

void TestSkFormula::TestFormulaPriority() {
    tApplication::Instance()->Locale("fr");
	tCell* wCell = m_Api->EnsureCell("A10");
    m_Api->UndoCellValue("A1", 1.3);
    m_Api->UndoCellValue("A2", 2.3);
    TestFormulaA1R1C1(wCell, "A1+A2");
    wCell->InternalCalculation();
    //cout << wCell->StrRef() << ":" << wCell->FormulaStr() << "=" << wCell->Value() << endl;
    CPPUNIT_ASSERT_MESSAGE("TestFormula Priority A1+A2", round(wCell->Value().Double() * 1e10) / 1e10 == 3.6);
    m_Api->UndoCellValue("A1", 3);
    m_Api->UndoCellValue("A2", 5);
    TestFormulaA1R1C1(wCell, "A1*A2+2");
     wCell->InternalCalculation();
    CPPUNIT_ASSERT_MESSAGE("TestFormula Priority A1*A2+2", wCell->Value().Int() == 17);
  
	TestFormulaA1R1C1(wCell, "3*5+2");
    wCell->InternalCalculation();
    CPPUNIT_ASSERT_MESSAGE("TestFormula Priority 3*5+2", wCell->Value().Int() == 17);
    
    TestFormulaA1R1C1(wCell, "3*(5+2)/2");
    wCell->InternalCalculation();
    CPPUNIT_ASSERT_MESSAGE("TestFormula Priority 3*(5+2)/2", round(wCell->Value().Double() * 1e10) / 1e10 == 10.5);
    
    TestFormulaA1R1C1(wCell, "3*(5+2)/2");
    wCell->InternalCalculation();
    CPPUNIT_ASSERT_MESSAGE("TestFormula Priority 3*(5+2)/2", round(wCell->Value().Double() * 1e10) / 1e10 == 10.5);
    
    
    m_Api->UndoCellValue("A1", 1.3);
    m_Api->UndoCellValue("A2", 2.3);
    m_Api->UndoCellValue("A3", 3);
    m_Api->UndoCellValue("A4", 5);
    TestFormulaA1R1C1(wCell, "A1+A2*A3/A4");
    wCell->InternalCalculation();
    CPPUNIT_ASSERT_MESSAGE("TestFormula Priority A1+A2*A3/A4", round(wCell->Value().Double() * 1e10) / 1e10 == 2.68);
    
    tApplication::Instance()->Locale("us");
}

void TestSkFormula::TestLeadingUnaryPlus() {
	tApplication::Instance()->Locale("us");
	m_Api->UndoCellValue("F143", 10);
	m_Api->UndoCellValue("F144", 3);
	m_Api->UndoCellValue("F145", 5);
	tCell* wCell = m_Api->EnsureCell("G145");
	TestFormulaA1R1C1(wCell, "+F143-F144+F145");
	wCell->InternalCalculation();
	CPPUNIT_ASSERT_MESSAGE("leading unary plus +F143-F144+F145", wCell->Value().Int() == 12);
}

void TestSkFormula::TestPercentLiteral() {
	tApplication::Instance()->Locale("us");
	tCell* wCell = m_Api->EnsureCell("AK3");
	m_Api->UndoCellValue("AJ3", 0.25);
	TestFormulaA1R1C1(wCell, "IF(AJ3<50%,\"inf à 50% AS\",\"sup à 50%\")");
	wCell->InternalCalculation();
	CPPUNIT_ASSERT_MESSAGE("AJ3=0.25 < 50% -> inf",
	                       wCell->Value().String() == "inf à 50% AS");

	m_Api->UndoCellValue("AJ3", 0.75);
	wCell->InternalCalculation();
	CPPUNIT_ASSERT_MESSAGE("AJ3=0.75 >= 50% -> sup",
	                       wCell->Value().String() == "sup à 50%");

	tCell* wNum = m_Api->EnsureCell("A20");
	TestFormulaA1R1C1(wNum, "50%");
	wNum->InternalCalculation();
	CPPUNIT_ASSERT_MESSAGE("50% == 0.5",
	                       std::abs(wNum->Value().Double() - 0.5) < 1e-12);

	TestFormulaA1R1C1(wNum, "1+50%");
	wNum->InternalCalculation();
	CPPUNIT_ASSERT_MESSAGE("1+50% == 1.5",
	                       std::abs(wNum->Value().Double() - 1.5) < 1e-12);

	TestFormulaA1R1C1(wNum, "2*50%");
	wNum->InternalCalculation();
	CPPUNIT_ASSERT_MESSAGE("2*50% == 1",
	                       std::abs(wNum->Value().Double() - 1.0) < 1e-12);
}

void TestSkFormula::TestIfIsNaDateNamedRange() {
	tApplication::Instance()->Locale("us");
	CPPUNIT_ASSERT_MESSAGE("insert _DATE_19000101",
	                       m_Api->UndoInsertRangeNamed("_DATE_19000101", "B9"));
	CPPUNIT_ASSERT_MESSAGE("insert _DATE_29991231",
	                       m_Api->UndoInsertRangeNamed("_DATE_29991231", "B10"));
	tCell* wCell = m_Api->EnsureCell("C19");
	const tString wFormulas[] = {
	    "IF(ISNA(B19),_DATE_19000101,B19)",
	    "IF(ISNA(R[0]C[-1]),_DATE_19000101,R[0]C[-1])",
	    "IF(ISNA(B12),_DATE_29991231,B12)",
	    "IF(ISNA(R[0]C[-1]),_DATE_29991231,R[0]C[-1])",
	};
	for (const tString& wFormula : wFormulas) {
		if (!m_Api->CompilCell(wCell, wFormula.c_str())) {
			CPPUNIT_ASSERT_MESSAGE(
			    ("compile " + wFormula + ": " + m_Api->ErrorWithDetail()).c_str(), false);
		}
	}
	m_Api->UndoDeleteRangeNamed("_DATE_19000101");
	m_Api->UndoDeleteRangeNamed("_DATE_29991231");
}

void TestSkFormula::TestFormula() {
	tCell* wCell = m_Api->EnsureCell(1, 1);

	TestFormulaA1R1C1(wCell, "1,3+2,3");
    
    TestFormulaA1R1C1(wCell, "3*5+2");

#ifdef checksp
	m_Api->Check();
#endif
	TestFormulaA1R1C1(wCell,"SUM(A1:A2)");
	TestFormulaA1R1C1(wCell, "SUM(B2:B8)");
	//wCell = m_Api->EnsureCell(1, 2);
	m_Api->UndoInsertRangeNamed("ALLEZ", "B4:B4");
    
	wCell = m_Api->EnsureCell(2, 1);
	TestFormulaA1R1C1(wCell, "A1:A2");
	wCell = m_Api->EnsureCell(3, 1);
	TestFormulaA1R1C1(wCell, "SUM(A1:A2;12,3;3,4)");
    
    // US
    tApplication::Instance()->Locale("us");
    TestFormulaA1R1C1(wCell, "SUM(A1:A2,12.3,3.4)");
    
    m_Api->CellValue("A2",2);
    m_Api->CellValue("A3",3);
    m_Api->CellValue("A4",20);
    m_Api->CellValue("A5",3);
    
    wCell = m_Api->EnsureCell(3, 2);
    TestFormulaA1R1C1(wCell, "A2+A3*A4/A5");
    tInt wTest=2+3*20/3;
    wCell->Calculation();
    //cout << endl << "Test "<< wTest << "=" << wCell->Value() << endl;
    CPPUNIT_ASSERT_MESSAGE("TestFormula SUM(A1:OK)) ",
                           tVariant(wTest) ==  wCell->Value());
     /*
    cout << wTest << "=" << wCell->StrRef() << ":" << wCell->Value() << endl;
    cout << endl << wCell->Formula()->FormulaKey() << endl;
    cout << wCell->Debug();
    cout <<  wCell->FormulaStr() << endl;
    */
    tApplication::Instance()->Locale("fr");
  
	wCell = m_Api->EnsureCell(4, 1);
	TestFormulaA1R1C1(wCell, "A1+A2");
	wCell = m_Api->EnsureCell(5, 1);
	TestFormulaA1R1C1(wCell, "ALLEZ+1/2");
	wCell = m_Api->EnsureCell(6, 1);

	TestFormulaA1R1C1(wCell, "A1+((A2+A3)*(A4+A5))");

	// Error
	tBool wResult=m_Api->CompilCell(wCell, "SUM(A1:OK");
    tString wError=m_Api->LemonInterface()->ErrorWithDetail();
    tString wWaitError="Syntax error ! [9]->SUM(A1:OK^....";
	CPPUNIT_ASSERT_MESSAGE("TestFormula SUM(A1:OK", wResult == false);
    CPPUNIT_ASSERT_MESSAGE("TestFormula SUM(A1:OK",wError ==  wWaitError);
    
	wResult = m_Api->CompilCell(wCell, "SUM(A1:A1))");
    wError=m_Api->ErrorWithDetail();
    wWaitError = "Syntax error ! [11]->SUM(A1:A1))^....";
    CPPUNIT_ASSERT_MESSAGE("TestFormula SUM(A1:OK)) ", wResult == false);
	CPPUNIT_ASSERT_MESSAGE("TestFormula SUM(A1:OK)) ", wError ==  wWaitError);
    
	wResult = m_Api->CompilCell(wCell, "AZR?");
    wError=m_Api->ErrorWithDetail();
    wWaitError="Syntax error ! [4]->AZR?^....";
	CPPUNIT_ASSERT_MESSAGE("TestFormula AZR?", wResult == false);
	CPPUNIT_ASSERT_MESSAGE("TestFormula SUM(A1:OK ", wError == wWaitError);
#ifdef checksp
	m_Api->Check();
#endif
    tVariant wVariant;
    wVariant.Parse("31/12/2024 10:30");
    m_Api->UndoCellValue("A1",wVariant);
    wCell=m_Api->Cell("A1");
    tClassDate wDateA1(wCell->Value().Date());
    /*
    cout << wDateA1.Year() << endl;
    cout << wDateA1.Month() << endl;
    cout << wDateA1.Day() << endl;
    cout << wDateA1.Hour() << endl;
    cout << wDateA1.Minute() << endl;
    cout << wDateA1.Second() << endl;
    */
    m_Api->UndoCellValue("A2","=A1+0,05");
    
    wCell=m_Api->Cell("A2");
    tClassDate wDateA2(wCell->Value().Date());
    /*
    cout << wDateA2.Year() << endl;
    cout << wDateA2.Month() << endl;
    cout << wDateA2.Day() << endl;
    cout << wDateA2.Hour() << endl;
    cout << wDateA2.Minute() << endl;
    cout << wDateA2.Second() << endl;
    */
    //cout << wDate.FormatString(tFormatStringType::dateddmmyyyyhmmss);
    
	//cout << endl << m_Api->LemonInterface()->Error() << endl;
    m_Api->UndoCellValue("B2","=COS(12,5)");
}

void TestSkFormula::TestRecursive() {
	tCell* wCellTest = nullptr;
	tCell* wCellA1 = m_Api->EnsureCell(1, 1);
	tCell* wCellA2 = m_Api->EnsureCell(2, 1);
	
	m_Api->CompilCell(wCellA1, "A2+1");
	m_Api->CompilCell(wCellA2, "A1+1");

	wCellA1->Calculation();

	wCellTest = wCellA1;
	CPPUNIT_ASSERT_MESSAGE("Test Recursive A1", (wCellTest->Value().Type() == tVariantType::t_error) && (wCellTest->Value().Error().Code() == tTypeError::t_recursive));
	wCellTest = wCellA2;
	CPPUNIT_ASSERT_MESSAGE("Test Recursive A2", (wCellTest->Value().Type() == tVariantType::t_error) && (wCellTest->Value().Error().Code() == tTypeError::t_recursive));
}

// Regression test: a user workbook with DUPONT=C6:F6 and ALIAN=D5:E7 creates a mutual
// dependency (D7=SUM(DUPONT) depends on F6 via C6:F6, F6=SUM(ALIAN) depends on D7 via
// D5:E7). ResolveBlockedAcyclicSubgraph used to iterate 40 times without detecting the
// additive divergence and leave large / #REF! garbage on D7. Both cells must end up
// marked with t_recursive (#RECURSIVE), exactly like a plain A1<->A2 cycle.
// Stale #RECURSIVE from a previous calc pass must be cleared when the cell re-enters the path.
void TestSkFormula::TestClearStaleRecursive() {
	tCell* wCellB1 = m_Api->EnsureCell(1, 2);
	tCell* wCellA1 = m_Api->EnsureCell(1, 1);
	CPPUNIT_ASSERT_MESSAGE("Compil B1=5", m_Api->CompilCell(wCellB1, "5"));
	CPPUNIT_ASSERT_MESSAGE("Compil A1=B1+1", m_Api->CompilCell(wCellA1, "B1+1"));
	m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, 1, 2));
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("A1 initial value", 6.0, wCellA1->Value().Numeric(), 1e-9);

	// Simulate a leftover #RECURSIVE after a dependency fix (e.g. Amort.sker scenario).
	wCellA1->Value(tVariant(tClassError(tTypeError::t_recursive, "")));
	CPPUNIT_ASSERT_MESSAGE("A1 poisoned with #RECURSIVE",
	                       wCellA1->Value().Error().Code() == tTypeError::t_recursive);

	wCellA1->Calculation();
	CPPUNIT_ASSERT_MESSAGE("A1 must not keep stale #RECURSIVE",
	                       wCellA1->Value().Type() != tVariantType::t_error);
	CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("A1 recalculated value", 6.0, wCellA1->Value().Numeric(), 1e-9);
}

void TestSkFormula::TestRecursiveNamedRange() {
	CPPUNIT_ASSERT_MESSAGE("Insert DUPONT", m_Api->UndoInsertRangeNamed("DUPONT", "C6:F6"));
	CPPUNIT_ASSERT_MESSAGE("Insert ALIAN",  m_Api->UndoInsertRangeNamed("ALIAN",  "D5:E7"));

	// Seed numeric data on the cells that the cycle will aggregate; makes sure the
	// divergent branch really emits a growing numeric iteration rather than a pure
	// zero fixed point (Excel SUM ignores text).
	m_Api->UndoCellValue("D5", 1);
	m_Api->UndoCellValue("D6", 12);
	m_Api->UndoCellValue("E5", 13);

	// Close the cycle: F6 -> D7 (via ALIAN), D7 -> F6 (via DUPONT).
	CPPUNIT_ASSERT_MESSAGE("F6=SUM(ALIAN)",  m_Api->UndoCellValue("F6", "=SUM(ALIAN)"));
	CPPUNIT_ASSERT_MESSAGE("D7=SUM(DUPONT)", m_Api->UndoCellValue("D7", "=SUM(DUPONT)"));

	tCell* wCellD7 = m_Api->Cell("D7");
	tCell* wCellF6 = m_Api->Cell("F6");
	CPPUNIT_ASSERT_MESSAGE("D7 exists", wCellD7 != nullptr);
	CPPUNIT_ASSERT_MESSAGE("F6 exists", wCellF6 != nullptr);

	CPPUNIT_ASSERT_MESSAGE("D7 is an error (not a numeric divergence)",
	                       wCellD7->Value().Type() == tVariantType::t_error);
	CPPUNIT_ASSERT_MESSAGE("D7 is #RECURSIVE (not #REF!)",
	                       wCellD7->Value().Error().Code() == tTypeError::t_recursive);
	CPPUNIT_ASSERT_MESSAGE("F6 is an error (not a numeric divergence)",
	                       wCellF6->Value().Type() == tVariantType::t_error);
	CPPUNIT_ASSERT_MESSAGE("F6 is #RECURSIVE (not #REF!)",
	                       wCellF6->Value().Error().Code() == tTypeError::t_recursive);
}


void TestSkFormula::TestNamedRangeR1C1Offset() {
	tApplication::Instance()->Locale("us");
	CPPUNIT_ASSERT_MESSAGE("insert named range",
	                       m_Api->UndoInsertRangeNamed("_RUBRIQUES_STATS", "A1:C10"));
	tCell* wCell = m_Api->EnsureCell(103, 7);
	const tString wFormula = "_RUBRIQUES_STATSR[-9]C[7]";
	if (!m_Api->CompilCell(wCell, wFormula.c_str())) {
		CPPUNIT_ASSERT_MESSAGE(
		    ("compile named range + R1C1 offset: " + m_Api->ErrorWithDetail()).c_str(), false);
	}
	CPPUNIT_ASSERT_MESSAGE("formula has one range ref",
	                       wCell->VectorRef() != nullptr && wCell->VectorRef()->size() == 1);
	tItem* wItem = wCell->VectorRef()->at(0);
	CPPUNIT_ASSERT_MESSAGE("ref is range", wItem != nullptr && wItem->Range() != nullptr);
	tRange* wRange = wItem->Range();
	CPPUNIT_ASSERT_MESSAGE("top row", wRange->TopIndex() == 94);
	CPPUNIT_ASSERT_MESSAGE("left col", wRange->LeftIndex() == 14);
	CPPUNIT_ASSERT_MESSAGE("bottom row", wRange->BottomIndex() == 103);
	CPPUNIT_ASSERT_MESSAGE("right col", wRange->RightIndex() == 16);
	m_Api->UndoDeleteRangeNamed("_RUBRIQUES_STATS");
}


void TestSkFormula::TestNamedRangeR1C1OffsetUnderscoreBeforeR() {
	tApplication::Instance()->Locale("us");
	CPPUNIT_ASSERT_MESSAGE("insert named range",
	                       m_Api->UndoInsertRangeNamed("_RUBRIQUES_STATS", "A1:C10"));
	// Excel: _RUBRIQUES_STATS_R[-102]C[7] — lexer peels _RUBRIQUES_STATS_ + R[…]; name is _RUBRIQUES_STATS.
	tCell* wCell = m_Api->EnsureCell(205, 7);
	const tString wFormula = "_RUBRIQUES_STATS_R[-102]C[7]";
	if (!m_Api->CompilCell(wCell, wFormula.c_str())) {
		CPPUNIT_ASSERT_MESSAGE(
		    ("compile named range + underscore + R1C1: " + m_Api->ErrorWithDetail()).c_str(), false);
	}
	CPPUNIT_ASSERT_MESSAGE("formula has one range ref",
	                       wCell->VectorRef() != nullptr && wCell->VectorRef()->size() == 1);
	tRange* wRange = wCell->VectorRef()->at(0)->Range();
	CPPUNIT_ASSERT_MESSAGE("top row", wRange != nullptr && wRange->TopIndex() == 103);
	CPPUNIT_ASSERT_MESSAGE("left col", wRange->LeftIndex() == 14);
	m_Api->UndoDeleteRangeNamed("_RUBRIQUES_STATS");
}


void TestSkFormula::TestVlookupNamedRangeR1C1Offset() {
	tApplication::Instance()->Locale("us");
	CPPUNIT_ASSERT_MESSAGE("insert named range",
	                       m_Api->UndoInsertRangeNamed("_RUBRIQUES_STATS_N1", "A1:C10"));
	m_Api->UndoInsertRangeNamed("_ETABLISSEMENT", "D1");
	m_Api->UndoInsertRangeNamed("_NATUREECR", "D2");
	tCell* wCell = m_Api->EnsureCell(104, 7);
	const tString wFragment =
	    "VLOOKUP(\"#R:BA_\" & R[0]C5 & \"#S:\"&R3C8 & \"#E:\"& _ETABLISSEMENT & \"#N:\"&_NATUREECR & "
	    "\"#D:\"& TEXT(R6C[0],\"aaaamm\"),_RUBRIQUES_STATS_N1,4,FALSE)";
	if (!m_Api->CompilCell(wCell, wFragment.c_str())) {
		CPPUNIT_ASSERT_MESSAGE(("VLOOKUP fragment: " + m_Api->ErrorWithDetail()).c_str(), false);
	}
	const tString wNested =
	    "IF( NOT(ISNA(VLOOKUP(\"#R:BA_\" & R[0]C5 & \"#S:\"&R3C8 & \"#E:\"& _ETABLISSEMENT & "
	    "\"#N:\"&_NATUREECR & \"#D:\"& TEXT(R6C[0],\"aaaamm\"),_RUBRIQUES_STATS_N1,4,FALSE))),1,0)";
	if (!m_Api->CompilCell(wCell, wNested.c_str())) {
		CPPUNIT_ASSERT_MESSAGE(("ISNA+VLOOKUP: " + m_Api->ErrorWithDetail()).c_str(), false);
	}
	m_Api->UndoDeleteRangeNamed("_RUBRIQUES_STATS_N1");
	m_Api->UndoDeleteRangeNamed("_ETABLISSEMENT");
	m_Api->UndoDeleteRangeNamed("_NATUREECR");
}


void TestSkFormula::TestCalculate() {
	tCell* wCell = m_Api->EnsureCell(1, 2);

	wCell = m_Api->EnsureCell(2, 2); m_Api->CompilCell(wCell, "B1+1");
	wCell = m_Api->EnsureCell(3, 2); m_Api->CompilCell(wCell, "B2+1");

	wCell = m_Api->EnsureCell(4, 2); m_Api->CompilCell(wCell, "B3+1");
	wCell = m_Api->EnsureCell(5, 2); m_Api->CompilCell(wCell, "B4+1");
	wCell = m_Api->EnsureCell(6, 2); m_Api->CompilCell(wCell, "B5+1");
	wCell = m_Api->EnsureCell(7, 2); m_Api->CompilCell(wCell, "B6+1");
	wCell = m_Api->EnsureCell(8, 2); m_Api->CompilCell(wCell, "B7+1");
	wCell = m_Api->EnsureCell(9, 2); m_Api->CompilCell(wCell, "B1+B2+B3+B4+B5+B6+B7+B8");

	wCell = m_Api->EnsureCell(10, 2); m_Api->CompilCell(wCell, "SUM(B1:B9)");

	wCell = m_Api->EnsureCell(1, 2);
	wCell->Value(2);

	wCell->Calculation();

	wCell = m_Api->EnsureCell(10, 2);
	CPPUNIT_ASSERT_MESSAGE("TestFormula) " + wCell->FormulaStr() + " !", wCell->Value().Int() == 88);
	wCell = m_Api->EnsureCell(1, 2);
	wCell->Value(4);
	wCell->Calculation();
	wCell = m_Api->EnsureCell(10, 2);
	CPPUNIT_ASSERT_MESSAGE("TestFormula) " + m_Api->EnsureCell(10, 2)->FormulaStr() + " !", wCell->Value().Int() == 120);

#ifdef checksp
	m_Api->Check();
#endif
    // Test Division Int to Double
    m_Api->UndoCellValue("A1", 3);
    m_Api->UndoCellValue("A2", 4);
    m_Api->UndoCellValue("A3", "=A2/A1");
    tCell* wCellA3=m_Api->Cell("A3");
    
    //cout << endl << wCellA3->Value() << endl;
    CPPUNIT_ASSERT_MESSAGE("TestFormula A2/A1 double" , wCellA3->Value().Type() == tVariantType::t_double);
    
    m_Api->UndoCellValue("A4", "=SUM(1;-2;3)");
    tCell* wCellA4=m_Api->Cell("A4");
   
    CPPUNIT_ASSERT_MESSAGE("TestFormula SUM(1;-2;3)" , wCellA4->Value().Int() == 2);
    m_Api->UndoCellValue("A4", "=ABS(-1)");
    wCellA4=m_Api->Cell("A4");
    
    CPPUNIT_ASSERT_MESSAGE("TestFormula ABS(-1)" , wCellA4->Value().Int() == 1);
    
   
}

void TestSkFormula::TestCalculateString() {
	tCell* wCell = m_Api->EnsureCell(1, 2);
	wCell->Value("Hello world !");

	wCell = m_Api->EnsureCell(2, 2);
    tBool wOk=m_Api->CompilCell(wCell, "B1+'..'+\"!\"");
	wCell->Calculation();
	CPPUNIT_ASSERT_MESSAGE("TestFormula String) " + m_Api->Cell(2, 2)->FormulaStr() + " !", wCell->Value().String() == "Hello world !..!");

    wCell = m_Api->EnsureCell(3, 2);
    wOk = m_Api->CompilCell(wCell, "REPT(\"ab\";3)");
    CPPUNIT_ASSERT_MESSAGE("TestFormula REPT compile", wOk == true);
    wCell->Calculation();
    CPPUNIT_ASSERT_MESSAGE("TestFormula REPT value", wCell->Value().String() == "ababab");
}


void TestSkFormula::TestFunctionLogical() {
	tCell* wCellA1 = m_Api->EnsureCell(1, 1);
	tCell* wCellA2 = m_Api->EnsureCell(2, 1);
    tApplication::Instance()->Locale("fr");
    m_Api->UndoCellValue("A1", "=false");
    tBool wOk=m_Api->CompilCell(wCellA2, "IF(A1=true;\"Ok\";\"Not Ok\")");
    wCellA2->Calculation();
    //cout << m_Api->Cell("A2")->FormulaStr() << endl;
    //cout << m_Api->Cell("A2")->Value() << endl;
    CPPUNIT_ASSERT_MESSAGE("TestFormula Function logical) " + m_Api->Cell(2, 1)->FormulaStr() + " !", wCellA2->Value().String() == "Not Ok");

    m_Api->UndoCellValue("A1", 1);
	m_Api->CompilCell(wCellA2, "IF(A1<100;\"Ok\";\"Not Ok\")");
	wCellA2->Calculation();
	CPPUNIT_ASSERT_MESSAGE("TestFormula Function Logical)" + m_Api->Cell(2, 1)->FormulaStr() + " !", wCellA2->Value().String() == "Ok");

	wCellA1->Value(120);
	wCellA2->Calculation();
	CPPUNIT_ASSERT_MESSAGE("TestFormula Function logical) " + m_Api->Cell(2, 1)->FormulaStr() + " !", wCellA2->Value().String() == "Not Ok");

	wOk=m_Api->CompilCell(wCellA2, "IF(OR(A1<100;A1=120);\"Ok\";\"Not Ok\")");
    if (!wOk) {
        cout << endl <<  m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
    }
	wCellA2->Calculation();
	CPPUNIT_ASSERT_MESSAGE("TestFormula Function logical) " + m_Api->Cell(2, 1)->FormulaStr() + " !", wCellA2->Value().String() == "Ok");

	wOk=m_Api->CompilCell(wCellA2, "IF(NOT(OR((A1<100);(A1=120)));\"Ok\";\"Not Ok\")");
    if (!wOk) {
        cout << endl <<  m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
    }
	wCellA2->Calculation();
	CPPUNIT_ASSERT_MESSAGE("TestFormula Function logical) " + m_Api->Cell(2, 1)->FormulaStr() + " !", wCellA2->Value().String() == "Not Ok");
    
    // US
    tApplication::Instance()->Locale("us");
    m_Api->CompilCell(wCellA2, "IF(NOT(OR((A1<100),(A1=120))),\"Ok\",\"Not Ok us\")");
    wCellA2->Calculation();
    CPPUNIT_ASSERT_MESSAGE("TestFormula Function logical us) " + m_Api->Cell(2, 1)->FormulaStr() + " !", wCellA2->Value().String() == "Not Ok us");
    tApplication::Instance()->Locale("fr");
    
    wOk=m_Api->CompilCell(wCellA2,"IF(A1=\"\";\"\";123)");
    wCellA2->Calculation();
    CPPUNIT_ASSERT_MESSAGE("TestFormula Function logical true \"\") ", wCellA2->Value()=tVariant(123));
    
    wOk=m_Api->CompilCell(wCellA2,"IF(A3=\"0\";\"\";123)");
    wCellA2->Calculation();
    CPPUNIT_ASSERT_MESSAGE("TestFormula Function logical false \"\") ", wCellA2->Value()==tVariant(123));
 
    tString wFormula="IF(IF(A3=0;1;2);3;4)";
    wOk=m_Api->CompilCell(wCellA2,wFormula.c_str());
    //cout << endl;
    //cout << wCellA2->FormulaStr() << endl;
    CPPUNIT_ASSERT_MESSAGE("TestFormula Function logical imbricated \"\") ",
                           wCellA2->FormulaStr()==wFormula);
 
}

void TestSkFormula::TestFunctionDate() {
    tBool wOk=m_Api->UndoCellValue("A1","=YEAR(TODAYERROR())");
    CPPUNIT_ASSERT_MESSAGE("TestFormula Function Date Error ! ", wOk == false);
    
    
    wOk=m_Api->UndoCellValue("B1","=TODAY()");
    tCell* wCellB1=m_Api->Cell("B1");
    
    tVariant wDate=wCellB1->Value();
    if (wDate.IsDate()) {
        tClassDate wClassDate(wDate.Date());
        // Not Last minute of the day 21/08/2024 -> 24:59  22/08/2024
        if ((wClassDate.Hour()!=24) && (wClassDate.Minute()<59)) {
            //out << wClassDate.UsDate(false);
            tClassDate wClassDateTest(tClassDate::Now());
            //cout << wClassDateTest.UsDate(false);
            CPPUNIT_ASSERT_MESSAGE("TestFormula Function Date Today() ! ", wClassDate.UsDate(false)==wClassDateTest.UsDate(false));
        }
    } else {
        CPPUNIT_ASSERT_MESSAGE("TestFormula Function Date Today() not date ! ", false);
    }
    
    
    
    tVariant wVariant;
    wVariant.Parse("31/12/2024");
    m_Api->UndoCellValue("A2", wVariant);
    wOk=m_Api->UndoCellValue("A3","=YEAR(A2)");
    //cout << m_Api->Cell("A3")->FormulaStr() << endl;
    //cout << m_Api->Cell("A3")->Value() << endl;
    tCell* wCell3=m_Api->Cell("A3");
    CPPUNIT_ASSERT_MESSAGE("TestFormula Function Year ! ", wCell3->Value() == tVariant(2024));
    
    wOk=m_Api->UndoCellValue("A4","=MONTH(A2)");
    tCell* wCell4=m_Api->Cell("A4");
    CPPUNIT_ASSERT_MESSAGE("TestFormula Function Year ! ", wCell4->Value() == tVariant(12));
    
    wOk=m_Api->UndoCellValue("A5","=DAY(A2)");
    tCell* wCell5=m_Api->Cell("A5");
    CPPUNIT_ASSERT_MESSAGE("TestFormula Function Year ! ", wCell5->Value() == tVariant(31));

    tApplication::Instance()->Locale("fr");
    wOk = m_Api->UndoCellValue("A6", "=DATEVALUE(\"01/06/2024\")");
    tCell* wCell6 = m_Api->Cell("A6");
    CPPUNIT_ASSERT_MESSAGE("TestFormula Function DATEVALUE compiles", wOk);
    CPPUNIT_ASSERT_MESSAGE("TestFormula Function DATEVALUE is date", wCell6->Value().IsDate());
    {
        tClassDate wParsed(wCell6->Value().Date());
        CPPUNIT_ASSERT_MESSAGE("TestFormula Function DATEVALUE day", wParsed.Day() == 1);
        CPPUNIT_ASSERT_MESSAGE("TestFormula Function DATEVALUE month", wParsed.Month() == 6);
        CPPUNIT_ASSERT_MESSAGE("TestFormula Function DATEVALUE year", wParsed.Year() == 2024);
    }

    // ISO 8601 text must parse regardless of locale (Excel DATEVALUE). Workbooks build ISO via
    // TEXT(date,"aaaa-mm-jj"); tVariant::Parse only matches the single locale separator ('/' in FR),
    // so DATEVALUE falls back to a tolerant numeric parser for the year-first form.
    wOk = m_Api->UndoCellValue("A7", "=DATEVALUE(\"2017-05-31\")");
    tCell* wCell7 = m_Api->Cell("A7");
    CPPUNIT_ASSERT_MESSAGE("DATEVALUE ISO compiles", wOk);
    CPPUNIT_ASSERT_MESSAGE("DATEVALUE ISO is date", wCell7->Value().IsDate());
    {
        tClassDate wParsed(wCell7->Value().Date());
        CPPUNIT_ASSERT_MESSAGE("DATEVALUE ISO day", wParsed.Day() == 31);
        CPPUNIT_ASSERT_MESSAGE("DATEVALUE ISO month", wParsed.Month() == 5);
        CPPUNIT_ASSERT_MESSAGE("DATEVALUE ISO year", wParsed.Year() == 2017);
    }

    // Regression (INSIDE workbook): DATEVALUE("01/"&TEXT(date,"mm/aaaa")). In FR the slash form is
    // dd/mm/yyyy -> "01/05/2017" is 1 May 2017. This chain used to fail with #VALUE! and fall back
    // to a -1 sentinel, breaking downstream date cells.
    wOk = m_Api->UndoCellValue("A8", "=DATEVALUE(\"01/\"&TEXT(A7;\"mm/aaaa\"))");
    tCell* wCell8 = m_Api->Cell("A8");
    CPPUNIT_ASSERT_MESSAGE("DATEVALUE concat compiles", wOk);
    CPPUNIT_ASSERT_MESSAGE("DATEVALUE concat is date", wCell8->Value().IsDate());
    {
        tClassDate wParsed(wCell8->Value().Date());
        CPPUNIT_ASSERT_MESSAGE("DATEVALUE concat day", wParsed.Day() == 1);
        CPPUNIT_ASSERT_MESSAGE("DATEVALUE concat month", wParsed.Month() == 5);
        CPPUNIT_ASSERT_MESSAGE("DATEVALUE concat year", wParsed.Year() == 2017);
    }

    // Regression (INSIDE workbook / BILAN ACTIF F14): the lookup key uses
    // TEXT(date,"aaaamm"). Excel returns "201705"; the date formatter used to reject compact
    // numeric formats and TEXT fell back to the full US date, so VLOOKUP missed #D:201705.
    wOk = m_Api->UndoCellValue("A9", "=TEXT(A8;\"aaaamm\")");
    tCell* wCell9 = m_Api->Cell("A9");
    CPPUNIT_ASSERT_MESSAGE("TEXT compact date compiles", wOk);
    CPPUNIT_ASSERT_MESSAGE("TEXT compact date is string", wCell9->Value().IsString());
    CPPUNIT_ASSERT_MESSAGE("TEXT compact date yyyymm", wCell9->Value().String() == tString("201705"));

    //cout << wCell3->FormulaStr() << "=" << wCell3->Value() << endl;
}

void TestSkFormula::TestMultiSheet() {
    m_Api->UndoAddSheet("S2");
    m_Api->UndoCellValue("A1", 1);
    m_Api->UndoCellValue("A2", 2);
    m_Api->UndoCellValue("A3", 3);
    
    
    m_Api->ActiveSheet("Sheet1");
    m_Api->UndoCellValue("B1", 1);
    m_Api->UndoCellValue("B2", 1.5);
    m_Api->UndoCellValue("B3", 2.5);
    
    
    m_Api->UndoCellValue("A1", "=S2!A1-S2!A3");
#ifdef printdebug
    {
    tCell* wCell=m_Api->Cell("A1");
    cout << wCell->Debug();
    }
#endif
    m_Api->UndoCellValue("A3","=SUM(S2!A1:A3)");
#ifdef printdebug
    {
    tCell* wCell=m_Api->Cell("A3");
    cout << wCell->Debug();
    }
#endif
    
    m_Api->ActiveSheet("S2");
    m_Api->UndoCellValue("A5","=SUM(Sheet1!B1:B3)");
#ifdef printdebug
    {
    tCell* wCell=m_Api->Cell("A5");
    cout << wCell->Debug();
    }
#endif
    
}


void TestSkFormula::TestFormulaRef() {
    tString wTitle="TestFormulaRef";
    tString wFormula="=#NAME?";
    if (!m_Api->UndoCellValue("A1", wFormula)) {
        tString wSubTitle="Error Compil  "+wFormula;
        CPPUNIT_FAIL((wTitle + " " + wSubTitle).c_str());
    }
    wFormula="=#REF!+1";
    if (!m_Api->UndoCellValue("A1", wFormula)) {
        tString wSubTitle="Error Compil  "+wFormula;
        CPPUNIT_FAIL((wTitle + " " + wSubTitle).c_str());
    }
    wFormula="=SUM(#REF!)";
    if (!m_Api->UndoCellValue("A1", wFormula)) {
        tString wSubTitle="Error Compil  "+wFormula;
        CPPUNIT_FAIL((wTitle + " " + wSubTitle).c_str());
    }
    CPPUNIT_ASSERT_MESSAGE(wTitle+"", m_Api->Formula(1,1)=="SUM(#REF!)");
    // Excel accepts #REF! bound to a named range (e.g. after row delete left a broken ref).
    m_Api->EnsureRangeNamed("TestRefRange", 1, 1, 3, 3);
    wFormula="=SUM(#REF!:TestRefRange)";
    if (!m_Api->UndoCellValue("B1", wFormula)) {
        tString wSubTitle="Error Compil  "+wFormula;
        CPPUNIT_FAIL((wTitle + " " + wSubTitle).c_str());
    }
    CPPUNIT_ASSERT_MESSAGE(wTitle+" B1 formula", m_Api->Formula(1,2)=="SUM(#REF!:TestRefRange)");
    //cout << m_Api->Formula(1,1) << endl;
}

void TestSkFormula::TestFormulaNamedSingleCellInConcat() {
    tApplication::Instance()->Locale("us");
	CPPUNIT_ASSERT_MESSAGE("lookup table",
	                       m_Api->UndoInsertRangeNamed("_GENERAUX_STATS", "A1:B10"));
	CPPUNIT_ASSERT_MESSAGE("named range on D17",
	                       m_Api->UndoInsertRangeNamed("_SIT_DATESREF_VALEUR", "D17"));
	tCell* wCell = m_Api->EnsureCell(17, 9);
	const tString wFormula =
	    "VLOOKUP(\"#R:\"&R[0]C2&\"#G:\"&R[0]C4&\"#E:\"&R3C[0],_GENERAUX_STATS,2,FALSE)";
	CPPUNIT_ASSERT_MESSAGE("compile VLOOKUP concat with named single-cell ref",
	                       m_Api->CompilCell(wCell, wFormula.c_str()));
	const tString wRendered = wCell->FormulaStr(true);
	CPPUNIT_ASSERT_MESSAGE("FormulaStr must keep ref after named cell",
	                       wRendered.find("&\"#E:\"&,") == tString::npos);
	CPPUNIT_ASSERT_MESSAGE("FormulaStr shows named range for D17",
	                       wRendered.find("_SIT_DATESREF_VALEUR") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("FormulaStr keeps #E anchor",
	                       wRendered.find("R3C[0]") != tString::npos
	                       || wRendered.find("R[-14]C[0]") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("FormulaStr round-trip compiles",
	                       m_Api->CompilCell(wCell, wRendered.c_str()));
	m_Api->UndoDeleteRangeNamed("_SIT_DATESREF_VALEUR");
	m_Api->UndoDeleteRangeNamed("_GENERAUX_STATS");
}

void TestSkFormula::TestFormulaOffset() {
	// FR locale: ';' as function argument separator (Excel-style).
	tApplication::Instance()->Locale("fr");

	m_Api->UndoCellValue("A1", 10);
	m_Api->UndoCellValue("A2", 20);
	m_Api->UndoCellValue("A3", 30);
	m_Api->UndoCellValue("B1", 1);
	m_Api->UndoCellValue("B2", 2);
	m_Api->UndoCellValue("B3", 3);
	m_Api->UndoCellValue("C1", 100);

	// 3-arg: OFFSET(A1;2;0) moves to A3, same 1x1 shape -> SUM = 30
	{
		tBool wOk = m_Api->UndoCellValue("D1", "=SUM(OFFSET(A1;2;0))");
		CPPUNIT_ASSERT_MESSAGE("compile SUM(OFFSET(A1;2;0))", wOk);
		tCell* wCell = m_Api->Cell("D1");
		CPPUNIT_ASSERT(wCell != nullptr);
		wCell->InternalCalculation();
		CPPUNIT_ASSERT_MESSAGE("SUM(OFFSET(A1;2;0))", wCell->Value().Int() == 30);
	}

	// 3-arg range: OFFSET(A1:A2;0;1) -> B1:B2, SUM = 1+2
	{
		tBool wOk = m_Api->UndoCellValue("D2", "=SUM(OFFSET(A1:A2;0;1))");
		CPPUNIT_ASSERT_MESSAGE("compile SUM(OFFSET(A1:A2;0;1))", wOk);
		tCell* wCell = m_Api->Cell("D2");
		wCell->InternalCalculation();
		CPPUNIT_ASSERT_MESSAGE("SUM(OFFSET(A1:A2;0;1))", wCell->Value().Int() == 3);
	}

	// 5-arg: OFFSET(A1;1;1;2;1) -> B2:B3, SUM = 2+3
	{
		tBool wOk = m_Api->UndoCellValue("D3", "=SUM(OFFSET(A1;1;1;2;1))");
		CPPUNIT_ASSERT_MESSAGE("compile SUM(OFFSET(A1;1;1;2;1))", wOk);
		tCell* wCell = m_Api->Cell("D3");
		wCell->InternalCalculation();
		CPPUNIT_ASSERT_MESSAGE("SUM(OFFSET(A1;1;1;2;1))", wCell->Value().Int() == 5);
	}

	// 4-arg: OFFSET(A1:A3;0;1;2) -> B1:B2 (height 2, width from ref = 1 column)
	{
		tBool wOk = m_Api->UndoCellValue("D4", "=SUM(OFFSET(A1:A3;0;1;2))");
		CPPUNIT_ASSERT_MESSAGE("compile SUM(OFFSET(A1:A3;0;1;2))", wOk);
		tCell* wCell = m_Api->Cell("D4");
		wCell->InternalCalculation();
		CPPUNIT_ASSERT_MESSAGE("SUM(OFFSET(A1:A3;0;1;2))", wCell->Value().Int() == 3);
	}

	// Negative column offset: OFFSET(C1;0;-1) -> B1
	{
		tBool wOk = m_Api->UndoCellValue("D5", "=SUM(OFFSET(C1;0;-1))");
		CPPUNIT_ASSERT_MESSAGE("compile SUM(OFFSET(C1;0;-1))", wOk);
		tCell* wCell = m_Api->Cell("D5");
		wCell->InternalCalculation();
		CPPUNIT_ASSERT_MESSAGE("SUM(OFFSET(C1;0;-1))", wCell->Value().Int() == 1);
	}

	// Invalid: row moves above sheet -> #REF!
	{
		tBool wOk = m_Api->UndoCellValue("D6", "=OFFSET(A1;-5;0)");
		CPPUNIT_ASSERT_MESSAGE("compile OFFSET(A1;-5;0)", wOk);
		tCell* wCell = m_Api->Cell("D6");
		wCell->InternalCalculation();
		CPPUNIT_ASSERT_MESSAGE("OFFSET out of range is error", wCell->Value().IsError());
	}

	// Omitted height/width default to the reference dimensions (Excel), even though the parser
	// pushes empty args as 0. OFFSET(A1:B3;0;0;;) must keep the 3x2 reference shape ->
	// SUM = 10+20+30+1+2+3 = 66.
	{
		tBool wOk = m_Api->UndoCellValue("D7", "=SUM(OFFSET(A1:B3;0;0;;))");
		CPPUNIT_ASSERT_MESSAGE("compile SUM(OFFSET(A1:B3;0;0;;))", wOk);
		tCell* wCell = m_Api->Cell("D7");
		wCell->InternalCalculation();
		CPPUNIT_ASSERT_MESSAGE("OFFSET omitted height/width keeps ref shape", wCell->Value().Int() == 66);
	}

	// Regression (INSIDE workbook): VLOOKUP over OFFSET with omitted rows+height used to return
	// #REF! (empty height -> 0 -> #REF!), which cascaded to #ARG!/#N/A. OFFSET(A1:B3;;1;;1) shifts
	// one column right keeping the 3-row height (B1:B3); VLOOKUP finds 2 there -> returns 2.
	{
		tBool wOk = m_Api->UndoCellValue("D8", "=VLOOKUP(2;OFFSET(A1:B3;;1;;1);1;FALSE)");
		CPPUNIT_ASSERT_MESSAGE("compile VLOOKUP over OFFSET omitted args", wOk);
		tCell* wCell = m_Api->Cell("D8");
		wCell->InternalCalculation();
		CPPUNIT_ASSERT_MESSAGE("VLOOKUP over OFFSET omitted args must not error", !wCell->Value().IsError());
		CPPUNIT_ASSERT_MESSAGE("VLOOKUP over OFFSET omitted args value", wCell->Value().Int() == 2);
	}
}

void TestSkFormula::TestFormulaIndirect() {
	tApplication::Instance()->Locale("fr");

	m_Api->UndoCellValue("A1", 10);
	m_Api->UndoCellValue("A2", 20);
	m_Api->UndoCellValue("A3", 30);
	m_Api->UndoCellValue("B2", 42);

	auto wAssertIntValue = [](tCell* sCell, const char* sMsg, tInt sExpected) {
		CPPUNIT_ASSERT_MESSAGE(sMsg, sCell != nullptr);
		const tVariant wValue = sCell->Value();
		if (wValue.IsError()) {
			tStringStream wStream;
			wStream << sMsg << " got error " << wValue.Error().Error();
			CPPUNIT_FAIL(wStream.str().c_str());
		}
		if (wValue.Type() == tVariantType::t_int) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sMsg, sExpected, wValue.Int());
			return;
		}
		if (wValue.Type() == tVariantType::t_double) {
			CPPUNIT_ASSERT_EQUAL_MESSAGE(sMsg, sExpected, static_cast<tInt>(wValue.Double()));
			return;
		}
		tStringStream wStream;
		wStream << sMsg << " unexpected variant type";
		CPPUNIT_FAIL(wStream.str().c_str());
	};

	// Scalar: INDIRECT("A1") -> 10
	{
		tBool wOk = m_Api->UndoCellValue("D1", "=INDIRECT(\"A1\")");
		CPPUNIT_ASSERT_MESSAGE("compile INDIRECT(\"A1\")", wOk);
		tCell* wCell = m_Api->Cell("D1");
		wCell->InternalCalculation();
		wAssertIntValue(wCell, "INDIRECT(\"A1\")", 10);
	}

	// Dynamic text ref: INDIRECT("B"&2) -> 42
	{
		tBool wOk = m_Api->UndoCellValue("D2", "=INDIRECT(\"B\"&2)");
		CPPUNIT_ASSERT_MESSAGE("compile INDIRECT(\"B\"&2)", wOk);
		tCell* wCell = m_Api->Cell("D2");
		wCell->InternalCalculation();
		wAssertIntValue(wCell, "INDIRECT(\"B\"&2)", 42);
	}

	// Range: SUM(INDIRECT("A1:A3")) -> 60
	{
		tBool wOk = m_Api->UndoCellValue("D3", "=SUM(INDIRECT(\"A1:A3\"))");
		CPPUNIT_ASSERT_MESSAGE("compile SUM(INDIRECT(\"A1:A3\"))", wOk);
		tCell* wCell = m_Api->Cell("D3");
		wCell->InternalCalculation();
		wAssertIntValue(wCell, "SUM(INDIRECT(\"A1:A3\"))", 60);
	}

	// Named range
	{
		m_Api->EnsureRangeNamed("IndirectData", 1, 1, 3, 1);
		m_Api->UndoCellValue("A1", 10);
		m_Api->UndoCellValue("A2", 20);
		m_Api->UndoCellValue("A3", 30);
		tBool wOk = m_Api->UndoCellValue("D4", "=SUM(INDIRECT(\"IndirectData\"))");
		CPPUNIT_ASSERT_MESSAGE("compile SUM(INDIRECT(\"IndirectData\"))", wOk);
		tCell* wCell = m_Api->Cell("D4");
		wCell->InternalCalculation();
		wAssertIntValue(wCell, "SUM(INDIRECT named range)", 60);
	}

	// Invalid reference -> #REF!
	{
		tBool wOk = m_Api->UndoCellValue("D5", "=INDIRECT(\"XFD1048577\")");
		CPPUNIT_ASSERT_MESSAGE("compile INDIRECT invalid ref", wOk);
		tCell* wCell = m_Api->Cell("D5");
		wCell->InternalCalculation();
		CPPUNIT_ASSERT_MESSAGE("INDIRECT invalid ref is error", wCell->Value().IsError());
		CPPUNIT_ASSERT_EQUAL(wCell->Value().Error().Code(), tTypeError::t_ref);
	}

	// Optional a1 argument (explicit TRUE keeps A1 parsing)
	{
		tBool wOk = m_Api->UndoCellValue("D6", "=INDIRECT(\"A1\";TRUE)");
		CPPUNIT_ASSERT_MESSAGE("compile INDIRECT with TRUE", wOk);
		tCell* wCell = m_Api->Cell("D6");
		wCell->InternalCalculation();
		wAssertIntValue(wCell, "INDIRECT(\"A1\";TRUE)", 10);
	}

	// R1C1 absolute: INDIRECT("R2C2",FALSE) -> B2 = 42
	{
		tBool wOk = m_Api->UndoCellValue("D7", "=INDIRECT(\"R2C2\";FALSE)");
		CPPUNIT_ASSERT_MESSAGE("compile INDIRECT R1C1", wOk);
		tCell* wCell = m_Api->Cell("D7");
		wCell->InternalCalculation();
		wAssertIntValue(wCell, "INDIRECT R1C1 absolute", 42);
	}

	// A1 address with a1=FALSE (Excel shared formulas may store W2574 this way)
	{
		tBool wOk = m_Api->UndoCellValue("D8", "=INDIRECT(\"B2\";FALSE)");
		CPPUNIT_ASSERT_MESSAGE("compile INDIRECT A1 with FALSE", wOk);
		tCell* wCell = m_Api->Cell("D8");
		wCell->InternalCalculation();
		wAssertIntValue(wCell, "INDIRECT A1 with FALSE", 42);
	}

	// R1C1 relative: cell to the left
	{
		m_Api->UndoCellValue("W5", 99);
		tBool wOk = m_Api->UndoCellValue("X5", "=INDIRECT(\"R[0]C[-1]\";FALSE)");
		CPPUNIT_ASSERT_MESSAGE("compile INDIRECT R[0]C[-1]", wOk);
		tCell* wCell = m_Api->Cell("X5");
		wCell->InternalCalculation();
		wAssertIntValue(wCell, "INDIRECT R[0]C[-1]", 99);
	}

	// French L1C1: INDIRECT("LC(-1)") at J5 -> I5 (default a1=TRUE still resolves LC notation)
	{
		m_Api->UndoCellValue("I5", 77);
		tBool wOk = m_Api->UndoCellValue("J5", "=INDIRECT(\"LC(-1)\")");
		CPPUNIT_ASSERT_MESSAGE("compile INDIRECT LC(-1)", wOk);
		tCell* wCell = m_Api->Cell("J5");
		wCell->InternalCalculation();
		wAssertIntValue(wCell, "INDIRECT LC(-1)", 77);
	}

	// French L1C1 with explicit a1=0 (Excel INSIDE: INDIRECT("LC(-1)";0))
	{
		m_Api->UndoCellValue("I5", 88);
		tBool wOk = m_Api->UndoCellValue("J5", "=INDIRECT(\"LC(-1)\",0)");
		CPPUNIT_ASSERT_MESSAGE("compile INDIRECT LC(-1) a1=0", wOk);
		tCell* wCell = m_Api->Cell("J5");
		wCell->InternalCalculation();
		wAssertIntValue(wCell, "INDIRECT LC(-1) a1=0", 88);
	}

	// INSIDE Saisie Budget T27 pattern: blank Manuel (S) -> SUMIF on materialized column T.
	{
		for (const tChar* wRef : {"G28", "G29", "G30", "S27", "T28", "T29", "T30"}) {
			m_Api->UndoCellValue(wRef, tVariant());
		}
		m_Api->UndoCellValue("G28", tString("BA_AH"));
		m_Api->UndoCellValue("G29", tString("BA_AH"));
		m_Api->UndoCellValue("G30", tString("BA_AH"));
		m_Api->UndoCellValue("T28", tDouble(0.0));
		m_Api->UndoCellValue("T29", tDouble(0.0));
		m_Api->UndoCellValue("T30", tDouble(96841.04));
		tBool wOk = m_Api->UndoCellValue("T27",
		    "=IF(NOT(ISBLANK(INDIRECT(\"LC(-1)\",0))),INDIRECT(\"LC(-1)\",0),"
		    "SUMIF($G$28:$G$30,\"BA_AH\",$T$28:$T$30))");
		CPPUNIT_ASSERT_MESSAGE("compile T27 INSIDE IF/SUMIF", wOk);
		tCell* wCell = m_Api->Cell("T27");
		wCell->InternalCalculation();
		CPPUNIT_ASSERT_DOUBLES_EQUAL_MESSAGE("T27 SUMIF materialized column", 96841.04, wCell->Value().Double(), 0.01);
	}
}

void OnCellChange(tCell* sCell,tVariant& sValue) {
    //cout << "OnCellChange---->" <<sCell->StrRef() << "=" << sValue.Str()<< endl;
}
void TestSkFormula::setUp() {
    std::filesystem::remove_all("./Spreadsheet");
    
    m_Application = tApplication::Instance();
    m_Application->Locale("fr");
    // Test On CellChange
    tSpreadSheetContainer::SetOnCellChange(&OnCellChange);
    m_Api = new tApi;
    m_Api->NewWorkBook("wwww.skeema.fr/w1");

};


void TestSkFormula::tearDown() {
	delete(m_Api);
}

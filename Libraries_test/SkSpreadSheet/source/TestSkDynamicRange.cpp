//==============================================================================
// TestSkDynamicRange - CppUnit tests for dynamic range with INDEX
//==============================================================================

#include "../include/TestSkDynamicRange.hpp"
#include <SkSpreadSheet.hpp>
#include <SkCell.hpp>
#include <SkFormula.hpp>
#include <SkLexerSpreadSheet.hpp>
#include <SkSheet.hpp>
#include <SkTools.hpp>
#include <string>

TestSkDynamicRange::TestSkDynamicRange()
	: CPPUNIT_NS::TestFixture()
	, m_Application(nullptr)
	, m_Api(nullptr)
{
}

void TestSkDynamicRange::setUp() {
	std::filesystem::remove_all("./Spreadsheet");
	m_Application = tApplication::Instance();
	// Use "us" so INDEX(A1:C3,1,3) has three args (comma = list separator, not decimal)
	m_Application->Locale("us");
	m_Api = new tApi;
	m_Api->IsUndoActif(true);
	m_Api->NewWorkBook("test_dynamic_range");
}

void TestSkDynamicRange::tearDown() {
	delete m_Api;
}

void TestSkDynamicRange::CompilFormulaAndAssertKind(const tChar* sFormula, tKind sExpectedKind) {
	tCell* wCell = m_Api->EnsureCell(20, 10);
	CPPUNIT_ASSERT_MESSAGE("EnsureCell(20,10) failed", wCell != nullptr);
    
    tBool wResult = m_Api->CellValue(20,10,tVariant(sFormula));
	CPPUNIT_ASSERT_MESSAGE(std::string("Compil failed: ") + sFormula + " -> " + m_Api->LemonInterface()->ErrorWithDetail().c_str(), wResult);

	const tFormula* wFormula = wCell->Formula();
	CPPUNIT_ASSERT_MESSAGE("Formula is null after Compil", wFormula != nullptr);

	const tVectorItemFormula* wVec = wFormula->VectorItemFormula();
	CPPUNIT_ASSERT_MESSAGE("VectorItemFormula is null", wVec != nullptr);

	tBool wFound = false;
	for (const auto& wItem : *wVec) {
		if (wItem.Kind() == sExpectedKind) {
			wFound = true;
			break;
		}
	}
	CPPUNIT_ASSERT_MESSAGE(std::string("Compiled formula does not contain expected kind for: ") + sFormula, wFound);
}

void TestSkDynamicRange::TestDynamicRangeRightCompil() {
	// Left = static cell A1, right = INDEX(A1:C3, 1, 3)
	CompilFormulaAndAssertKind("=SUM(A1:INDEX(A1:C3,1,3))", tKind::DynamicRangeRight);
}

void TestSkDynamicRange::TestDynamicRangeLeftCompil() {
	// Left = INDEX(A1:C3,1,1), right = static cell C3
	CompilFormulaAndAssertKind("=SUM(INDEX(A1:C3,1,1):C3)", tKind::DynamicRangeLeft);
}

void TestSkDynamicRange::TestDynamicRangeBothCompil() {
	// Both bounds from INDEX
	CompilFormulaAndAssertKind("=SUM(INDEX(A1:C3,1,1):INDEX(A1:C3,1,3))", tKind::DynamicRangeBoth);
}

void TestSkDynamicRange::TestIndexResult() {
	// Fill A1:C3 with 1..9 (row-wise), then =INDEX(A1:C3,2,2) in D1 -> expect 5
	m_Api->CellValue("A1", 1);
	m_Api->CellValue("B1", 2);
	m_Api->CellValue("C1", 3);
	m_Api->CellValue("A2", 4);
	m_Api->CellValue("B2", 5);
	m_Api->CellValue("C2", 6);
	m_Api->CellValue("A3", 7);
	m_Api->CellValue("B3", 8);
	m_Api->CellValue("C3", 9);
	tBool wOk = m_Api->UndoCellValue(1, 4, tVariant("=INDEX(A1:C3,2,2)"));
	CPPUNIT_ASSERT_MESSAGE("Set formula INDEX(A1:C3,2,2) in D1", wOk);
	tTempoRect wRect(1, 1, 5, 5);
	m_Api->ActiveSheet()->Calculate(&wRect);
	tCell* wCell = m_Api->Cell("D1");
	CPPUNIT_ASSERT_MESSAGE("D1 cell exists", wCell != nullptr);
	CPPUNIT_ASSERT_MESSAGE("INDEX(A1:C3,2,2) result is 5", wCell->Value().Type() == tVariantType::t_int && wCell->Value().Int() == 5);
}

void TestSkDynamicRange::TestIndexWithNamedRangeResult() {
	// Named range "Data" = A1:C3, same fill, =INDEX(Data,2,2) in E1 -> expect 5
	m_Api->UndoInsertRangeNamed("Data", "A1:C3");
	m_Api->CellValue("A1", 1);
	m_Api->CellValue("B1", 2);
	m_Api->CellValue("C1", 3);
	m_Api->CellValue("A2", 4);
	m_Api->CellValue("B2", 5);
	m_Api->CellValue("C2", 6);
	m_Api->CellValue("A3", 7);
	m_Api->CellValue("B3", 8);
	m_Api->CellValue("C3", 9);
	tBool wOk = m_Api->UndoCellValue("E1", tVariant("=INDEX(Data,2,2)"));
	CPPUNIT_ASSERT_MESSAGE("Set formula INDEX(Data,2,2) in E1", wOk);
    tCell* wCell = m_Api->Cell("E1");
    //cout << endl << wCell->StrRef()<< ":" << wCell->FormulaStr() << "=[" << wCell->Value() << "]" << endl;
	CPPUNIT_ASSERT_MESSAGE("INDEX(Data,2,2) result is 5", wCell->Value().Type() == tVariantType::t_int && wCell->Value().Int() == 5);
}

void TestSkDynamicRange::TestDynamicRangeWithNamedRangeResult() {
	// Named range "Data" = A1:C3, row1 = 1,2,3; =SUM(A1:INDEX(Data,1,3)) in E1 -> expect 6
	m_Api->UndoInsertRangeNamed("Data", "A1:C3");
	m_Api->CellValue("A1", 1);
	m_Api->CellValue("B1", 2);
	m_Api->CellValue("C1", 3);
	tBool wOk = m_Api->UndoCellValue(1, 5, tVariant("=SUM(A1:INDEX(Data,1,3))"));
 
	CPPUNIT_ASSERT_MESSAGE("Set formula SUM(A1:INDEX(Data,1,3)) in E1", wOk);
    tCell* wCell=m_Api->Cell(1,5);

    //cout << endl << wCell->StrRef()<< ":" << wCell->FormulaStr() << "=[" << wCell->Value() << "]" << endl;
 
 
	tTempoRect wRect(1, 1, 5, 6);
	m_Api->ActiveSheet()->Calculate(&wRect);
	wCell = m_Api->Cell("E1");
 
    wOk = m_Api->UndoCellValue("F2", tVariant("=INDEX(Data,1,3)"));
    //cout << m_Api->Cell("F2")->Value() << endl;
    //cout << wCellF2->FormulaStr() << endl;
    //cout << wCellF2->Formula()->FormulaKey() << endl;
	CPPUNIT_ASSERT_MESSAGE("E1 cell exists", wCell != nullptr);
    //cout << endl << "  E1=" << wCell->Value() << endl;
	CPPUNIT_ASSERT_MESSAGE("SUM(A1:INDEX(Data,1,3)) result is 6", wCell->Value().Type() == tVariantType::t_int && wCell->Value().Int() == 6);
}

void TestSkDynamicRange::TestDynamicRangeLeftWithNamedRangeResult() {
	// Named range "Data" = A1:C3, row1 = 1,2,3; =SUM(INDEX(Data,1,1):C3) in E1 -> range A1:C3, expect 6
	m_Api->UndoInsertRangeNamed("Data", "A1:C3");
	m_Api->CellValue("A1", 1);
	m_Api->CellValue("B1", 2);
	m_Api->CellValue("C1", 3);
	tBool wOk = m_Api->UndoCellValue(1, 5, tVariant("=SUM(INDEX(Data,1,1):C3)"));
	CPPUNIT_ASSERT_MESSAGE("Set formula SUM(INDEX(Data,1,1):C3) in E1", wOk);
	tTempoRect wRect(1, 1, 5, 6);
	m_Api->ActiveSheet()->Calculate(&wRect);
	tCell* wCell = m_Api->Cell("E1");
    //cout << wCell->FormulaStr() << endl;
    //cout << "E1=" << wCell->Value() << endl;
	CPPUNIT_ASSERT_MESSAGE("E1 cell exists", wCell != nullptr);
	CPPUNIT_ASSERT_MESSAGE("SUM(INDEX(Data,1,1):C3) result is 6", wCell->Value().Type() == tVariantType::t_int && wCell->Value().Int() == 6);
}

void TestSkDynamicRange::TestDynamicRangeBothWithNamedRangeResult() {
	// Named range "Data" = A1:C3, row1 = 1,2,3; =SUM(INDEX(Data,1,1):INDEX(Data,1,3)) in E1 -> range A1:C1, expect 6
	m_Api->UndoInsertRangeNamed("Data", "A1:C3");
	m_Api->CellValue("A1", 1);
	m_Api->CellValue("B1", 2);
	m_Api->CellValue("C1", 3);
	tBool wOk = m_Api->UndoCellValue(1, 5, tVariant("=SUM(INDEX(Data,1,1):INDEX(Data,1,3))"));
	CPPUNIT_ASSERT_MESSAGE("Set formula SUM(INDEX(Data,1,1):INDEX(Data,1,3)) in E1", wOk);
	tTempoRect wRect(1, 1, 5, 6);
	m_Api->ActiveSheet()->Calculate(&wRect);
    tCell* wCell=m_Api->Cell("E1");
	CPPUNIT_ASSERT_MESSAGE("E1 cell exists", wCell != nullptr);
	CPPUNIT_ASSERT_MESSAGE("SUM(INDEX(Data,1,1):INDEX(Data,1,3)) result is 6", wCell->Value().Type() == tVariantType::t_int && wCell->Value().Int() == 6);
}

void TestSkDynamicRange::TestIndexWithTableNamedRangeResult() {
	// "Table" as named range Table1 = A1:C3 (3x3 table), values 10..90; =INDEX(Table1,3,2) in F1 -> expect 80
	m_Api->UndoInsertRangeNamed("Table1", "A1:C3");
	m_Api->CellValue("A1", 10);
	m_Api->CellValue("B1", 20);
	m_Api->CellValue("C1", 30);
	m_Api->CellValue("A2", 40);
	m_Api->CellValue("B2", 50);
	m_Api->CellValue("C2", 60);
	m_Api->CellValue("A3", 70);
	m_Api->CellValue("B3", 80);
	m_Api->CellValue("C3", 90);
	tBool wOk = m_Api->UndoCellValue(1, 6, tVariant("=INDEX(Table1,3,2)"));
	CPPUNIT_ASSERT_MESSAGE("Set formula INDEX(Table1,3,2) in F1", wOk);
	tTempoRect wRect(1, 1, 5, 7);
	m_Api->ActiveSheet()->Calculate(&wRect);
	tCell* wCell = m_Api->Cell("F1");
	CPPUNIT_ASSERT_MESSAGE("F1 cell exists", wCell != nullptr);
	CPPUNIT_ASSERT_MESSAGE("INDEX(Table1,3,2) result is 80", wCell->Value().Type() == tVariantType::t_int && wCell->Value().Int() == 80);
}

void TestSkDynamicRange::TestDynamicRangeCombinedResult() {
	// =SUM(A1:INDEX(A1:C3,1,3)) in E1 -> expect 6
	m_Api->UndoInsertRangeNamed("Data", "A1:C3");
	m_Api->CellValue("A1", 1);
	m_Api->CellValue("B1", 2);
	m_Api->CellValue("C1", 3);
	m_Api->CellValue("A2", 4);
	m_Api->CellValue("B2", 5);	
	m_Api->CellValue("C2", 3);
	m_Api->CellValue("A3", 7);
	m_Api->CellValue("B3", 8);
	m_Api->CellValue("C3", 9);


	m_Api->CellValue("J1", 1);
	m_Api->CellValue("J2", 3);
	tBool wOk = m_Api->UndoCellValue("E1", tVariant("=SUM(A1:INDEX(A1:C3,J1,2))"));
	CPPUNIT_ASSERT_MESSAGE("Set formula SUM(A1:INDEX(A1:C3,J1,2)) in E1", wOk);

	tCell* wCell = m_Api->Cell("E1");
	CPPUNIT_ASSERT_MESSAGE("E1 cell exists", wCell != nullptr);
	// J1=1 -> INDEX(A1:C3,1,2)=B1 -> SUM(A1:B1)=1+2=3
	CPPUNIT_ASSERT_MESSAGE("When J1=1, E1=SUM(A1:B1) is 3", wCell->Value().Type() == tVariantType::t_int && wCell->Value().Int() == 3);

	m_Api->UndoCellValue("J1", 2);
	tTempoRect wRect(1, 1, 10, 10);
	m_Api->ActiveSheet()->Calculate(&wRect);
	// J1=2 -> INDEX(A1:C3,2,2)=B2 -> SUM(A1:B2)=1+2+4+5=12
	CPPUNIT_ASSERT_MESSAGE("When J1=2, E1=SUM(A1:B2) is 12", wCell->Value().Type() == tVariantType::t_int && wCell->Value().Int() == 12);
}

void TestSkDynamicRange::TestIfSumDynamicRangeLeftLikeExcel() {
	// Pattern from Excel: =IF(PaymentSchedule3[[#This Row],[Payment number]]<>"", SUM(INDEX(PaymentSchedule3[Interest],1,1):PaymentSchedule3[[#This Row],[Interest]]), "")
	// Simplified: IF(A1<>"", SUM(INDEX(Interest,1,1):B2), "") with Interest=B1:B3, A1=payment flag, B2=this row Interest.
	m_Api->UndoInsertRangeNamed("Interest", "B1:B3");
	m_Api->CellValue("B1", 10);
	m_Api->CellValue("B2", 20);
	m_Api->CellValue("B3", 30);
	m_Api->CellValue("A1", tVariant("1"));  // Payment number non-empty -> SUM branch
	tBool wOk = m_Api->UndoCellValue("C2", tVariant("=IF(A1<>\"\",SUM(INDEX(Interest,1,1):B2),\"\")"));
	CPPUNIT_ASSERT_MESSAGE("Set formula IF(A1<>\"\",SUM(INDEX(Interest,1,1):B2),\"\") in C2", wOk);
	tTempoRect wRect(1, 1, 5, 5);
	m_Api->ActiveSheet()->Calculate(&wRect);
	tCell* wCell = m_Api->Cell("C2");
	CPPUNIT_ASSERT_MESSAGE("C2 cell exists", wCell != nullptr);
	// INDEX(Interest,1,1)=B1=10, range B1:B2 -> SUM(10,20)=30
	CPPUNIT_ASSERT_MESSAGE("IF(<>\"\") SUM(INDEX(Interest,1,1):B2) result is 30",
		wCell->Value().Type() == tVariantType::t_int && wCell->Value().Int() == 30);
	// When A1 is empty, result is empty string
	m_Api->CellValue("A1", tVariant(""));
	m_Api->ActiveSheet()->Calculate(&wRect);
	CPPUNIT_ASSERT_MESSAGE("When A1 empty, C2 is empty string",
		wCell->Value().Type() == tVariantType::t_string && wCell->Value().String() == "");

}

void TestSkDynamicRange::InsertTableWithColumns(tString sName, tString sRef, const std::vector<tString>& sColumnNames) {
	// Build RangeData JSON in same format as workbook (filterop, filtervalue, order)
	tString wJson("{\"columns\":[");
	for (size_t i = 0; i < sColumnNames.size(); ++i) {
		if (i > 0) wJson += ",";
		wJson += "{\"index\":";
		wJson += std::to_string(static_cast<int>(i));
		wJson += ",\"name\":\"";
		wJson += sColumnNames[i];
		wJson += "\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"None\"}";
	}
	wJson += "]}";
	tBool wOk = m_Api->UndoInsertRangeData(sName, sRef, wJson, nullptr);
	CPPUNIT_ASSERT_MESSAGE(std::string("InsertTableWithColumns ") + sName.c_str() + " " + sRef.c_str(), wOk);
}

void TestSkDynamicRange::TestTableThisRowAndDynamicRange() {
	// Table Table1 = A1:B4 with columns "Payment number" and "Interest" (header row 1, data rows 2-4).
	// Formula in D2: IF(Table1[[#This Row],[Payment number]]<>"", SUM(INDEX(Table1[Interest],1,1):Table1[[#This Row],[Interest]]), "")
	std::vector<tString> wCols;
	wCols.push_back(tString("Payment number"));
	wCols.push_back(tString("Interest"));
	InsertTableWithColumns("Table1", "A1:B4", wCols);
	m_Api->CellValue("A1", tVariant("Payment number"));
	m_Api->CellValue("B1", tVariant("Interest"));
	m_Api->CellValue("A2", tVariant("1"));
	m_Api->CellValue("B2", 10);
	m_Api->CellValue("A3", tVariant("2"));
	m_Api->CellValue("B3", 20);
	m_Api->CellValue("A4", tVariant(""));
	m_Api->CellValue("B4", 30);
    tString wFormulaUS="IF(Table1[[#This Row],[Payment number]]<>\"\",SUM(INDEX(Table1[Interest],1,1):Table1[[#This Row],[Interest]]),\"\")";
    tBool wOk = m_Api->UndoCellValue("D2","="+wFormulaUS);
	CPPUNIT_ASSERT_MESSAGE("Set formula with Table1 and [#This Row] in D2", wOk);
    tCell* wCell = m_Api->Cell("D2");
    CPPUNIT_ASSERT_MESSAGE("D2 cell exists", wCell != nullptr);
	
   /*
    cout << endl;
    cout << wCell->Formula()->FormulaKey() << endl;
    cout << wCell->FormulaStr() << endl;
    cout << wFormulaUS << endl;
	   
    cout << endl << "D2:" << wCell->FormulaStr() << "=" << wCell->Value() << endl;
    */
    CPPUNIT_ASSERT_MESSAGE("Set formula with Table1 and [#This Row] in D2 Formula US",
                           wCell->FormulaStr()==wFormulaUS);
    
    tApplication::Instance()->Locale("fr");
    tString wFormulaFR=wCell->FormulaStr();
    //cout << wFormulaFR << endl;
    
    wOk = m_Api->UndoCellValue("D2","="+wFormulaFR);
    CPPUNIT_ASSERT_MESSAGE("Set formula with Table1 and [#This Row] in D2 (fr)", wOk);
     CPPUNIT_ASSERT_MESSAGE("Set formula with Table1 and [#This Row] in D2 Formula FR",
                           wCell->FormulaStr()==wFormulaFR);
    
    tApplication::Instance()->Locale("us");
    
    
	// Row 2: INDEX(T1[Interest],1,1)=first data cell of Interest=B2=10, [#This Row],[Interest]=B2, so SUM(10:10)=10
	CPPUNIT_ASSERT_MESSAGE("D2 with [#This Row] and dynamic range is 10",
		wCell->Value().Type() == tVariantType::t_int && wCell->Value().Int() == 10);
	// Formula in D3: same pattern, row 3 -> SUM(B2:B3)=10+20=30
	wOk = m_Api->UndoCellValue("D3", tVariant("=IF(Table1[[#This Row],[Payment number]]<>\"\",SUM(INDEX(Table1[Interest],1,1):Table1[[#This Row],[Interest]]),\"\")"));
	CPPUNIT_ASSERT_MESSAGE("Set formula in D3", wOk);

	tCell* wCell3 = m_Api->Cell("D3");
	CPPUNIT_ASSERT_MESSAGE("D3 with [#This Row] is 30",
		wCell3 != nullptr && wCell3->Value().Type() == tVariantType::t_int && wCell3->Value().Int() == 30);
	// Row 4: Payment number empty -> result ""
	wOk = m_Api->UndoCellValue("D4", tVariant("=IF(Table1[[#This Row],[Payment number]]<>\"\",SUM(INDEX(Table1[Interest],1,1):Table1[[#This Row],[Interest]]),\"\")"));
	CPPUNIT_ASSERT_MESSAGE("Set formula in D4", wOk);

	tCell* wCell4 = m_Api->Cell("D4");
	CPPUNIT_ASSERT_MESSAGE("D4 when [#This Row] Payment number empty is \"\"",
		wCell4 != nullptr && wCell4->Value().Type() == tVariantType::t_string && wCell4->Value().String() == "");
}

void TestSkDynamicRange::TestIndexNestedIfIferrorCrossSheet() {
	// Reproduce: simple INDEX works, nested IF(B15="";NA();IFERROR(INDEX(...;$A15;C$6);NA())) returns #N/A.
	// Setup: sheet "Entrées des données financières" with B4:I28, value 125000 at (1,2) = C4.
	// Sheet1: A15=ROWS($B$15:B15)=1, B15=REVENU, C6=2. Formula in D15.
	tBool wOk = m_Api->UndoAddSheet("Entrées des données financières", "");
	CPPUNIT_ASSERT_MESSAGE("Add sheet Entrées des données financières", wOk);
	tSheet* wDataSheet = m_Api->ActiveSheet("Entrées des données financières");
	CPPUNIT_ASSERT_MESSAGE("ActiveSheet(Entrées des données financières)", wDataSheet != nullptr);
	// B4:I28: put 125000 in C4 (row 1, col 2 of range)
	wOk = m_Api->CellValue("C4", 125000, wDataSheet);
	CPPUNIT_ASSERT_MESSAGE("Set C4=125000 on data sheet", wOk);
	// Back to Sheet1
	tSheet* wSheet1 = m_Api->ActiveSheet("Sheet1");
	CPPUNIT_ASSERT_MESSAGE("ActiveSheet(Sheet1)", wSheet1 != nullptr);
	// A15 = ROWS($B$15:B15) -> 1
	wOk = m_Api->UndoCellValue("A15", tVariant("=ROWS($B$15:B15)"), wSheet1);
	CPPUNIT_ASSERT_MESSAGE("Set A15=ROWS($B$15:B15)", wOk);
	// B15 = REVENU
	wOk = m_Api->CellValue("B15", tVariant("REVENU"), wSheet1);
	CPPUNIT_ASSERT_MESSAGE("Set B15=REVENU", wOk);
	// C6 = 2
	wOk = m_Api->CellValue("C6", 2, wSheet1);
	CPPUNIT_ASSERT_MESSAGE("Set C6=2", wOk);
	// D15 = IF(B15="",NA(),IFERROR(INDEX('Entrées des données financières'!$B$4:$I$28,$A15,C$6),NA()))
	// US locale: commas as list separator
	tString wFormula = "=IF(B15=\"\",NA(),IFERROR(INDEX('Entrées des données financières'!$B$4:$I$28,$A15,C$6),NA()))";
	wOk = m_Api->UndoCellValue("D15", tVariant(wFormula), wSheet1);
	CPPUNIT_ASSERT_MESSAGE("Set nested formula in D15: ", wOk);

	tCell* wCellD15 = m_Api->Cell("D15", wSheet1);
	CPPUNIT_ASSERT_MESSAGE("D15 cell exists", wCellD15 != nullptr);
	// Expect 125000 (INDEX row 1, col 2 of data sheet range)
	tVariant wVal = wCellD15->Value();
    CPPUNIT_ASSERT_MESSAGE("D15 should be 125000 (INDEX cross-sheet), got type ",
                           wVal.Int()==125000);
}

void TestSkDynamicRange::TestIndexSimpleCrossSheetRegression() {
	// Regression: =INDEX('Entrées...'!$B$4:$I$18;$A8;C$6) with A8=1, C6=2 must return 125000 (same as plan's "formule qui fonctionne").
	tBool wOk = m_Api->UndoAddSheet("Entrées des données financières", "");
	CPPUNIT_ASSERT_MESSAGE("Add sheet for regression test", wOk);
	tSheet* wDataSheet = m_Api->ActiveSheet("Entrées des données financières");
	CPPUNIT_ASSERT_MESSAGE("ActiveSheet data", wDataSheet != nullptr);
	wOk = m_Api->CellValue("C4", 125000, wDataSheet);
	CPPUNIT_ASSERT_MESSAGE("Set C4=125000 on data sheet", wOk);
	tSheet* wSheet1 = m_Api->ActiveSheet("Sheet1");
	CPPUNIT_ASSERT_MESSAGE("ActiveSheet(Sheet1)", wSheet1 != nullptr);
	wOk = m_Api->CellValue("A8", 1, wSheet1);
	CPPUNIT_ASSERT_MESSAGE("Set A8=1", wOk);
	wOk = m_Api->CellValue("C6", 2, wSheet1);
	CPPUNIT_ASSERT_MESSAGE("Set C6=2", wOk);
	// Simple formula (no IF/IFERROR): INDEX(plage, A8, C6)
	tString wFormula = "=INDEX('Entrées des données financières'!$B$4:$I$28,$A8,C$6)";
	wOk = m_Api->UndoCellValue("E8", tVariant(wFormula), wSheet1);
	CPPUNIT_ASSERT_MESSAGE(std::string("Set simple INDEX in E8: ") + wFormula.c_str(), wOk);
	
	tCell* wCellE8 = m_Api->Cell("E8", wSheet1);
	CPPUNIT_ASSERT_MESSAGE("E8 cell exists", wCellE8 != nullptr);
	tVariant wVal = wCellE8->Value();

	CPPUNIT_ASSERT_MESSAGE("E8 (simple INDEX) should be 125000, got ", wVal.Int()== 125000);
}

void TestSkDynamicRange::TestSpillRefHashCompil() {
	CompilFormulaAndAssertKind("=COUNTA(D14#)", tKind::SpillRef);
	CompilFormulaAndAssertKind("=SEQUENCE(COUNTA(D14#))", tKind::SpillRef);
}

//==============================================================================
// TestSkMatrix
// Tests for matrix operations
//==============================================================================
#include "../include/TestSkMatrix.hpp"
#include <SkInterfaceWeb.hpp>
#include <SkWorkBook.hpp>
#include <SkTypesClass.hpp>
#include <SkFile.hpp>
#include <SkFormula.hpp>
#include <SkCell.hpp>
#include <SkLexerSpreadSheet.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

tString MatrixTestExcelDataDir() {
	const char* wDir = std::getenv("SKER_EXCEL_TEST_DIR");
	if (wDir != nullptr && wDir[0] != '\0') {
		tString p(wDir);
		while (!p.empty() && (p.back() == '/' || p.back() == '\\'))
			p.pop_back();
		if (!p.empty())
			p += '/';
		return p;
	}
#if defined(SKER_FILE_DIR) && !defined(__EMSCRIPTEN__)
	{
		tString p(SKER_FILE_DIR);
		while (!p.empty() && (p.back() == '/' || p.back() == '\\'))
			p.pop_back();
		if (!p.empty()) {
			p += '/';
			return p;
		}
	}
#endif
	const char* wHome = std::getenv("HOME");
#if defined(_WIN32) || defined(_WIN64)
	if (wHome == nullptr || wHome[0] == '\0')
		wHome = std::getenv("USERPROFILE");
#endif
	if (wHome != nullptr && wHome[0] != '\0') {
		tString p(wHome);
		if (!p.empty() && p.back() != '/' && p.back() != '\\')
			p += '/';
		p += "Projects/Excel/";
		return p;
	}
	return {};
}

} // namespace


namespace {
// CellValue may be t_int or t_double; compare numerically instead of calling Int().
static bool VariantNumericEq(const tVariant& sVariant, tDouble sExpected) {
	if (!sVariant.IsNumeric()) return false;
	const tDouble d = sVariant.Numeric();
	if (d == sExpected) return true;
	const tDouble scale = std::max<tDouble>(1.0, std::max(std::fabs(sExpected), std::fabs(d)));
	return std::fabs(d - sExpected) <= 1e-9 * scale;
}

// Excel date serials may surface as t_double; tClassDate(variant) accepts t_date and numeric serial.
static bool VariantCalendarDateOk(const tVariant& v) {
	if (v.Type() == tVariantType::t_date) return true;
	if (!v.IsNumeric()) return false;
	tClassDate cd(v);
	const tInt y = cd.Year();
	return y >= 1990 && y <= 2100;
}

// Exact calendar day (t_date or Excel serial) for parity checks vs Excel.
static bool VariantDateEq(const tVariant& v, tInt sYear, tInt sMonth, tInt sDay) {
	if (v.Type() == tVariantType::t_date) {
		tClassDate cd(v.Date());
		return cd.Year() == sYear && cd.Month() == sMonth && cd.Day() == sDay;
	}
	if (!v.IsNumeric()) return false;
	tClassDate cd(v);
	return cd.Year() == sYear && cd.Month() == sMonth && cd.Day() == sDay;
}
static bool VariantStringEq(const tVariant& sVariant, const tString& sExpected) {
	return sVariant.IsString() && sVariant.String() == sExpected;
}
// Api Formula() may omit leading '='; match by substring for spill ref.
static bool FormulaStrContains(const tString& sFormula, const tChar* sNeedle) {
	return sFormula.find(sNeedle) != tString::npos;
}
// Reference: Excel/Calendar.sker (export of Calendar.xlsx). Named formula there: "{0,1,2,3,4,5,6} + {0;1;2;3;4;5}*7".
// Full 6×7 spill from one cell with +1 matches Excel's first dynamic row (fi[3]); rows 2–6 of the matrix match fi[4]..fi[8]
// (+8,+15,…) when each block is clipped to one output row (B8:H8, B10:H10, …).
static const tChar* kCalendarMatrixFormula =
	"=JoursEtSemaines+DATE(AnnéeCalendrier;1;1)-WEEKDAY(DATE(AnnéeCalendrier;1;1);(DébutSemaine=\"l\")+1)+1";

// Calendar.sker fi[3]..fi[8]: same base formula with tail +1, +8, +15, +22, +29, +36 (comma in sker is Excel US; tests use ';').
static const tInt kCalendarSkerTailOffsets[] = { 1, 8, 15, 22, 29, 36 };

static tString CalendarSkerRowFormula(tInt sTailOffset) {
	tStringStream wSs;
	wSs << "=JoursEtSemaines+DATE(AnnéeCalendrier;1;1)-WEEKDAY(DATE(AnnéeCalendrier;1;1);(DébutSemaine=\"l\")+1)+"
	    << sTailOffset;
	return wSs.str();
}

// Expected 5×5 grid for TestMatrixListFormulaNamed (=list on A1, =list*10 on A4).
static void AssertMatrixListFormulaNamedGrid(tApi* sApi, const char* sTag) {
	// Row 1: 0,1,2,3,4
	for (tInt c = 1; c <= 5; ++c) {
		CPPUNIT_ASSERT_MESSAGE(sTag, VariantNumericEq(sApi->CellValue(1, c), static_cast<tDouble>(c - 1)));
	}
	// Row 2: 1,2,3,4,5
	for (tInt c = 1; c <= 5; ++c) {
		CPPUNIT_ASSERT_MESSAGE(sTag, VariantNumericEq(sApi->CellValue(2, c), static_cast<tDouble>(c)));
	}
	// Row 3: empty (no spill on this row)
	for (tInt c = 1; c <= 5; ++c) {
		CPPUNIT_ASSERT_MESSAGE(sTag, sApi->CellValue(3, c).IsExcelNull());
	}
	// Row 4: 0,10,20,30,40
	for (tInt c = 1; c <= 5; ++c) {
		CPPUNIT_ASSERT_MESSAGE(sTag, VariantNumericEq(sApi->CellValue(4, c), static_cast<tDouble>((c - 1) * 10)));
	}
	// Row 5: 10,20,30,40,50
	for (tInt c = 1; c <= 5; ++c) {
		CPPUNIT_ASSERT_MESSAGE(sTag, VariantNumericEq(sApi->CellValue(5, c), static_cast<tDouble>(c * 10)));
	}
}
} // namespace

TestSkMatrix::TestSkMatrix() : CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
}

void TestSkMatrix::setUp() {
    std::filesystem::remove_all("./Spreadsheet");

    m_Application = tApplication::Instance();
    m_Api = new tApi;
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
}

void TestSkMatrix::tearDown() {
    delete(m_Api);
}

void TestSkMatrix::DrawCell(tString sTitle, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
	return; // Drop - Uncomment to disable debug output
	cout << sTitle << endl;
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
		for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
			tVariant wVariant = m_Api->CellValue(wRow, wCol);
			tString wFormula = m_Api->Formula(wRow, wCol);
			cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";";
            switch (wVariant.Type()) {
                case tVariantType::t_date: {
                    // tClassDate::FormatString expects tFormatString* (not tFormatStringType).
                    tClassDate wDate(wVariant.Date());
                    tFormatString wFormat;
                    wFormat.FormatType(tFormatStringType::dateddmmyyyyhmm);
                    cout << wDate.FormatString(&wFormat);
                    break;
                }
                default:
                    cout << wVariant;
                    break;
            }
            cout << "\t";
		}
		cout << endl;
	}
}
void TestSkMatrix::TestMatrixEmpty() {
    tBool wOk=m_Api->UndoCellValue("A1", 1);
    wOk=m_Api->UndoCellValue("A2", 2);
    
    wOk = m_Api->UndoCellValue("B1", "=A1:A2");
    CPPUNIT_ASSERT_MESSAGE("Compile B1=A1:A2", wOk);
    DrawCell("Simple B1=A1:A2", 1, 1,2, 2);
    
    
    CPPUNIT_ASSERT_MESSAGE("-> B1", VariantNumericEq(m_Api->CellValue(1, 2),1));
    CPPUNIT_ASSERT_MESSAGE("-> B2", VariantNumericEq(m_Api->CellValue(2,2),2));
    /*
    cout << endl;
    tCell* wCell=m_Api->Cell("B1");
    cout << wCell->StrRef() << "=" << wCell->Debug() << endl;
    wCell=m_Api->Cell("B2");
    cout << wCell->StrRef() << "=" << wCell->Debug() << endl;
    */
    m_Api->Undo();
    tCell* wCell=m_Api->Cell("B1");
    CPPUNIT_ASSERT(wCell==nullptr);
    wCell=m_Api->Cell("B2");
    CPPUNIT_ASSERT(wCell->Value().IsNull());
    
}


void TestSkMatrix::TestMatrixUndoRedoSpill() {
	// Row 20: isolate from other tests; same pattern as B1==A1:A2 (range copy spill).
	const tIndex kR0 = 20;
	const tIndex kC = 2;
	tBool wOk = m_Api->UndoCellValue("A20", 1);
	CPPUNIT_ASSERT_MESSAGE("setup A20", wOk);
	wOk = m_Api->UndoCellValue("A21", 2);
	CPPUNIT_ASSERT_MESSAGE("setup A21", wOk);
	// UI inplace edit calls EnsureCell before commit — that creates an empty SaveCell and used to
	// take the ClearFormula-only undo path, leaving MatExtend slaves (B21) populated.
	CPPUNIT_ASSERT_MESSAGE("EnsureCell B20 (UI edit path)", m_Api->EnsureCell("B20") != nullptr);
	wOk = m_Api->UndoCellValue("B20", "=A20:A21");
	CPPUNIT_ASSERT_MESSAGE("UndoCellValue B20 =A20:A21", wOk);

	CPPUNIT_ASSERT_MESSAGE("spill B20 value", VariantNumericEq(m_Api->CellValue(kR0, kC), 1.0));
	CPPUNIT_ASSERT_MESSAGE("spill B21 value", VariantNumericEq(m_Api->CellValue(21, kC), 2.0));
	CPPUNIT_ASSERT_MESSAGE("B20 has spill formula", FormulaStrContains(m_Api->Formula(kR0, kC), "A20"));

	// One Undo: revert the last UndoCellValue (B20 formula + spill materialization).
	wOk = m_Api->Undo();
	CPPUNIT_ASSERT_MESSAGE("Undo() after spill", wOk);

	CPPUNIT_ASSERT_MESSAGE("after Undo A20 unchanged", VariantNumericEq(m_Api->CellValue(kR0, 1), 1.0));
	CPPUNIT_ASSERT_MESSAGE("after Undo A21 unchanged", VariantNumericEq(m_Api->CellValue(21, 1), 2.0));
	CPPUNIT_ASSERT_MESSAGE(
		"after Undo B20 formula cleared",
		!FormulaStrContains(m_Api->Formula(kR0, kC), "A20:A21"));
  
    //tCell* wCellkC=m_Api->Cell(21,kC);
    //cout << endl <<  wCellkC->StrRef() << "="  << wCellkC->Value() << endl;
	CPPUNIT_ASSERT_MESSAGE(
		"after Undo B21 spill cleared",
		m_Api->CellValue(21, kC).Type() == tVariantType::t_null);

	wOk = m_Api->Redo();
	CPPUNIT_ASSERT_MESSAGE("Redo() restores spill", wOk);

	CPPUNIT_ASSERT_MESSAGE("after Redo B20", VariantNumericEq(m_Api->CellValue(kR0, kC), 1.0));
	CPPUNIT_ASSERT_MESSAGE("after Redo B21", VariantNumericEq(m_Api->CellValue(21, kC), 2.0));
	CPPUNIT_ASSERT_MESSAGE("after Redo B20 formula", FormulaStrContains(m_Api->Formula(kR0, kC), "A20"));

	// 2x2 spill (user case: fill A1:B2, type =A1:B2 in A3, Undo must clear A3:B4).
	wOk = m_Api->UndoCellValue("A30", 1);
	CPPUNIT_ASSERT_MESSAGE("setup A30", wOk);
	wOk = m_Api->UndoCellValue("B30", 2);
	CPPUNIT_ASSERT_MESSAGE("setup B30", wOk);
	wOk = m_Api->UndoCellValue("A31", 3);
	CPPUNIT_ASSERT_MESSAGE("setup A31", wOk);
	wOk = m_Api->UndoCellValue("B31", 4);
	CPPUNIT_ASSERT_MESSAGE("setup B31", wOk);
	CPPUNIT_ASSERT_MESSAGE("EnsureCell A32 (UI edit path)", m_Api->EnsureCell("A32") != nullptr);
	wOk = m_Api->UndoCellValue("A32", "=A30:B31");
	CPPUNIT_ASSERT_MESSAGE("UndoCellValue A32 =A30:B31", wOk);
	CPPUNIT_ASSERT_MESSAGE("spill A32", VariantNumericEq(m_Api->CellValue(32, 1), 1.0));
	CPPUNIT_ASSERT_MESSAGE("spill B32", VariantNumericEq(m_Api->CellValue(32, 2), 2.0));
	CPPUNIT_ASSERT_MESSAGE("spill A33", VariantNumericEq(m_Api->CellValue(33, 1), 3.0));
	CPPUNIT_ASSERT_MESSAGE("spill B33", VariantNumericEq(m_Api->CellValue(33, 2), 4.0));
	wOk = m_Api->Undo();
	CPPUNIT_ASSERT_MESSAGE("Undo() after 2x2 spill", wOk);
	CPPUNIT_ASSERT_MESSAGE("after Undo A32 cleared", m_Api->CellValue(32, 1).Type() == tVariantType::t_null);
	CPPUNIT_ASSERT_MESSAGE("after Undo B32 spill cleared", m_Api->CellValue(32, 2).Type() == tVariantType::t_null);
	CPPUNIT_ASSERT_MESSAGE("after Undo A33 spill cleared", m_Api->CellValue(33, 1).Type() == tVariantType::t_null);
	CPPUNIT_ASSERT_MESSAGE("after Undo B33 spill cleared", m_Api->CellValue(33, 2).Type() == tVariantType::t_null);
}

void TestSkMatrix::TestMatrixList() {
	// Excel-style literal arrays: comma = next column, semicolon = next row.
	// Curly and pipe forms share the same evaluation (LeftCurly/RightCurly bytecode).

	// One row x three columns: ={12,34,4} spills A1:C1
	tBool wOk = m_Api->UndoCellValue("A1", "={12,34,4}");
	CPPUNIT_ASSERT_MESSAGE("Compile row literal {12,34,4}", wOk);
	DrawCell("TestMatrixList - row literal A1:C1:", 1, 1, 1, 3);
	CPPUNIT_ASSERT_MESSAGE("Literal A1", VariantNumericEq(m_Api->CellValue(1, 1), 12.0));
	CPPUNIT_ASSERT_MESSAGE("Literal B1", VariantNumericEq(m_Api->CellValue(1, 2), 34.0));
	CPPUNIT_ASSERT_MESSAGE("Literal C1", VariantNumericEq(m_Api->CellValue(1, 3), 4.0));

	// Two rows x two columns: ={1,2;3,4} spills E1:F2
	wOk = m_Api->UndoCellValue("E1", "={1,2;3,4}");
	CPPUNIT_ASSERT_MESSAGE("Compile 2x2 literal", wOk);
	DrawCell("TestMatrixList - 2x2 literal E1:F2:", 1, 5, 2, 6);
	CPPUNIT_ASSERT_MESSAGE("Literal E1", VariantNumericEq(m_Api->CellValue(1, 5), 1.0));
	CPPUNIT_ASSERT_MESSAGE("Literal F1", VariantNumericEq(m_Api->CellValue(1, 6), 2.0));
	CPPUNIT_ASSERT_MESSAGE("Literal E2", VariantNumericEq(m_Api->CellValue(2, 5), 3.0));
	CPPUNIT_ASSERT_MESSAGE("Literal F2", VariantNumericEq(m_Api->CellValue(2, 6), 4.0));
	{
		tCell* wE1 = m_Api->Cell("E1");
		CPPUNIT_ASSERT(wE1 != nullptr);
        //cout << wE1->Debug() << endl;
		tRange* wMr = wE1->MatrixRange();
		CPPUNIT_ASSERT(wMr != nullptr);
		CPPUNIT_ASSERT_MESSAGE("Literal matrix range E1:F2", wMr->StrRef() == "E1:F2");
	}

	// Pipe form |...| (same semantics as braces)
	wOk = m_Api->UndoCellValue("H1", "=|7,8|");
	CPPUNIT_ASSERT_MESSAGE("Compile pipe literal", wOk);
	DrawCell("TestMatrixList - pipe literal H1:I1:", 1, 8, 1, 9);
	CPPUNIT_ASSERT_MESSAGE("Pipe H1", VariantNumericEq(m_Api->CellValue(1, 8), 7.0));
	CPPUNIT_ASSERT_MESSAGE("Pipe I1", VariantNumericEq(m_Api->CellValue(1, 9), 8.0));

	// Literal + scalar (broadcast like a named range)
	wOk = m_Api->UndoCellValue("K1", "={1,2,3}+10");
	CPPUNIT_ASSERT_MESSAGE("Compile literal+scalar", wOk);
	DrawCell("TestMatrixList - literal+10 K1:M1:", 1, 11, 1, 13);
	CPPUNIT_ASSERT_MESSAGE("Literal+scalar K1", VariantNumericEq(m_Api->CellValue(1, 11), 11.0));
	CPPUNIT_ASSERT_MESSAGE("Literal+scalar L1", VariantNumericEq(m_Api->CellValue(1, 12), 12.0));
	CPPUNIT_ASSERT_MESSAGE("Literal+scalar M1", VariantNumericEq(m_Api->CellValue(1, 13), 13.0));

	// Column literal + DATE: same as Excel — add each row's number as whole days to the base date.
	wOk = m_Api->UndoCellValue("A10", "={1;2;3;4}+DATE(2026;1;2)");
	CPPUNIT_ASSERT_MESSAGE("Compile column literal + DATE", wOk);
	{
		tClassDate wJan2(2026, 1, 2);
		for (tInt r = 0; r < 4; ++r) {
			tClassDate wExp = wJan2 + (r + 1);
			const tVariant wV = m_Api->CellValue(10 + r, 1);
			// DATE results may be stored as Excel serial (double); tClassDate(variant) normalizes.
			CPPUNIT_ASSERT_EQUAL(wExp.Value(), tClassDate(wV).Value());
		}
	}
    DrawCell("A10=={1;2;3;4}+DATE(2026,1,2)", 10, 1, 14, 8);
    /*
    tCell* wCell=m_Api->Cell("A11");
    tClassDate wDate(wCell->Value());
                    tFormatString wFormat;
                    wFormat.FormatType(tFormatStringType::dateddmmyyyyhmm);
                    cout << wDate.FormatString(&wFormat);
    */
    
	// Function args with semicolon (Excel FR); lexer accepts both separators.
	wOk = m_Api->UndoCellValue("A15", "={1;2;3;4}+DATE(2026;1;2)");
	CPPUNIT_ASSERT_MESSAGE("Compile literal + DATE(…;…;…)", wOk);
	{
		tClassDate wJan2(2026, 1, 2);
		for (tInt r = 0; r < 4; ++r) {
			tClassDate wExp = wJan2 + (r + 1);
			CPPUNIT_ASSERT_EQUAL(wExp.Value(), tClassDate(m_Api->CellValue(15 + r, 1)).Value());
		}
	}
    DrawCell("Calandar", 6, 1, 20,7);
	// Single cell in braces (1x1)
	wOk = m_Api->UndoCellValue("N1", "={42}");
	CPPUNIT_ASSERT_MESSAGE("Compile singleton literal", wOk);
	CPPUNIT_ASSERT_MESSAGE("Singleton N1", VariantNumericEq(m_Api->CellValue(1, 14), 42.0));

	// Jagged rows: first row one column, second row two columns -> #VALUE!-style error
	wOk = m_Api->UndoCellValue("P1", "={1;1,2}");
	CPPUNIT_ASSERT_MESSAGE("Compile jagged literal (invalid)", wOk);
	tCell* wP1 = m_Api->Cell("P1");
	CPPUNIT_ASSERT(wP1 != nullptr);
	CPPUNIT_ASSERT_MESSAGE("Jagged literal is error", wP1->Value().IsError());
	CPPUNIT_ASSERT_MESSAGE("Jagged literal error code", wP1->Value().Error().Code() == tTypeError::t_value);
}

void TestSkMatrix::TestMatrixListFormulaNamed() {
	// {0;1} (2x1) + {0,1,2,3,4} (1x5) broadcasts to a 2x5 grid: row1 = 0..4, row2 = 1..5.
	// (Previously written {1;1}: it only produced an ascending row1 because the old literal path spilled both
	//  literals onto the same origin cell and the second one clobbered the first — a bug now fixed.)
	tBool wResult = m_Api->UndoInsertFormulaNamed("list", "{0;1}+{0,1,2,3,4}");
	CPPUNIT_ASSERT_MESSAGE("insert named formula list", wResult);

	wResult = m_Api->UndoCellValue("A1", "=list");
	CPPUNIT_ASSERT_MESSAGE("A1 =list", wResult);

	wResult = m_Api->UndoCellValue("A4", "=list*10");
	CPPUNIT_ASSERT_MESSAGE("A4 =list*10", wResult);

	AssertMatrixListFormulaNamedGrid(m_Api, "grid after insert");

	CPPUNIT_ASSERT_MESSAGE("A1 formula references list", FormulaStrContains(m_Api->Formula(1, 1), "list"));
	CPPUNIT_ASSERT_MESSAGE("A4 formula references list", FormulaStrContains(m_Api->Formula(4, 1), "list"));

	tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT(wWorkBook != nullptr);
	tString wJson = m_Api->WriteJson(wWorkBook->Uri());
	CPPUNIT_ASSERT_MESSAGE("WriteJson non-empty", !wJson.empty());
	CPPUNIT_ASSERT_MESSAGE("JSON lists formulanamed list", wJson.find("\"n\":\"list\"") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("JSON keeps named definition literal", wJson.find("{0;1}") != tString::npos);

	tearDown();
	setUp();

	CPPUNIT_ASSERT_MESSAGE("ReadJson round-trip", m_Api->ReadJson(wJson));

	wWorkBook = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT(wWorkBook != nullptr);

	AssertMatrixListFormulaNamedGrid(m_Api, "grid after ReadJson");

	CPPUNIT_ASSERT_MESSAGE("A1 formula after JSON", FormulaStrContains(m_Api->Formula(1, 1), "list"));
	CPPUNIT_ASSERT_MESSAGE("A4 formula after JSON", FormulaStrContains(m_Api->Formula(4, 1), "list"));

	wWorkBook->RecalculateAll();

	// Full-sheet RecalculateAll can leave CellValue reads inconsistent with in-memory grid for some origins.
	// Values are already asserted after ReadJson.
	AssertMatrixListFormulaNamedGrid(m_Api, "grid after RecalculateAll");

	CPPUNIT_ASSERT_MESSAGE("A1 formula after RecalculateAll", FormulaStrContains(m_Api->Formula(1, 1), "list"));
	CPPUNIT_ASSERT_MESSAGE("A4 formula after RecalculateAll", FormulaStrContains(m_Api->Formula(4, 1), "list"));
}
  
void TestSkMatrix::TestMatrixSpillError() {
	// Range copy spill: B1=A1:A2 needs B1:B2 — a static value in B2 must yield #SPILL! on B1.
	tBool wOk = m_Api->UndoCellValue("A1", 1);
	wOk = m_Api->UndoCellValue("A2", 2);
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("B2", 999);
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("B1", "=A1:A2");
	CPPUNIT_ASSERT_MESSAGE("Compile B1=A1:A2 with B2 blocked", wOk);
	{
		const tVariant wB1 = m_Api->CellValue(1, 2);
		CPPUNIT_ASSERT_MESSAGE("B1 is #SPILL! when B2 has a value", wB1.IsError());
		CPPUNIT_ASSERT_MESSAGE("B1 error code t_spill", wB1.Error().Code() == tTypeError::t_spill);
	}

	// Literal row spill D1:F1 = {1,2,3}: a value in E1 blocks the spill.
	wOk = m_Api->UndoCellValue("E1", 1);
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("D1", "={1,2,3}");
	CPPUNIT_ASSERT_MESSAGE("Compile D1 literal with E1 blocked", wOk);
	{
		const tVariant wD1 = m_Api->CellValue(1, 4);
		CPPUNIT_ASSERT_MESSAGE("D1 is #SPILL! when E1 has a value", wD1.IsError());
		CPPUNIT_ASSERT_MESSAGE("D1 error code t_spill", wD1.Error().Code() == tTypeError::t_spill);
	}

	// Excel: after a successful spill, typing into a slave cell clears the spill and origin → #SPILL!.
	CPPUNIT_ASSERT_MESSAGE("compile SEQUENCE spill", m_Api->UndoCellValue("G1", "=SEQUENCE(3)"));
	CPPUNIT_ASSERT_MESSAGE("G1 spilled", VariantNumericEq(m_Api->CellValue(1, 7), 1.0));
	CPPUNIT_ASSERT_MESSAGE("G2 spilled", VariantNumericEq(m_Api->CellValue(2, 7), 2.0));
	CPPUNIT_ASSERT_MESSAGE("G3 spilled", VariantNumericEq(m_Api->CellValue(3, 7), 3.0));
	CPPUNIT_ASSERT_MESSAGE("type into spill slave G2", m_Api->UndoCellValue("G2", "dffd"));
	{
		const tVariant wG1 = m_Api->CellValue(1, 7);
		CPPUNIT_ASSERT_MESSAGE("G1 is #SPILL! after overwrite of G2", wG1.IsError());
		CPPUNIT_ASSERT_MESSAGE("G1 error code t_spill after overwrite", wG1.Error().Code() == tTypeError::t_spill);
		CPPUNIT_ASSERT_MESSAGE("G2 keeps user value", m_Api->CellValue(2, 7).Str() == tString("dffd"));
		CPPUNIT_ASSERT_MESSAGE("G3 cleared after spill break", m_Api->CellValue(3, 7).IsExcelNull());
	}
	// Undo must restore the full spill (not leave #SPILL! / cleared slaves).
	CPPUNIT_ASSERT_MESSAGE("undo overwrite of spill slave", m_Api->Undo());
	CPPUNIT_ASSERT_MESSAGE("G1 restored after undo", VariantNumericEq(m_Api->CellValue(1, 7), 1.0));
	CPPUNIT_ASSERT_MESSAGE("G2 restored after undo", VariantNumericEq(m_Api->CellValue(2, 7), 2.0));
	CPPUNIT_ASSERT_MESSAGE("G3 restored after undo", VariantNumericEq(m_Api->CellValue(3, 7), 3.0));
}

void TestSkMatrix::TestMatrixCalendar() {
    tApplication::Instance()->Locale("fr");
	// Full 6×7 spill from A10: no OOXML array-formula ref on the cell, so the matrix is not clipped to one row.
	// Same numeric grid as Calendar.xlsx when six separate rows use +1,+8,+15,+22,+29,+36 (see Excel/Calendar.sker fi[]).
	//
	// Calendar.xlsx / .sker layout:
	// (1) Only rows 8,10,12,14,16,18 hold a formula; rows 9,11… are blank by design.
	// (2) Per block, OOXML ref is one row (e.g. B8:H8). SkExcel calls SetArrayFormulaOutputRect then RecalculateAll —
	//     TestMatrixCalendarOoxmlSixBlocksLikeExcel matches fi[3]..fi[8] tail constants.
	//
	// Named formula JoursEtSemaines = {0,1,2,3,4,5,6}+{0;1;2;3;4;5}*7 (same as workbook definedName).
	// Use ';' as function-arg separator (same as TestMatrixList DATE(2026;1;2)) — ',' breaks DATE in FR-style locale.
	tBool wOk = m_Api->UndoCellValue("A1", "=DATE(2026;1;1)");
	CPPUNIT_ASSERT(wOk);
	CPPUNIT_ASSERT(VariantCalendarDateOk(m_Api->CellValue("A1")));

	wOk = m_Api->UndoCellValue("A2", "=YEAR(A1)");
	CPPUNIT_ASSERT(wOk);
    
	CPPUNIT_ASSERT_EQUAL(2026, m_Api->CellValue("A2").Int());

    wOk = m_Api->UndoInsertFormulaNamed("JoursEtSemaines", "{0,1,2,3,4,5,6}+{0;1;2;3;4;5}*7");
    CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoInsertRangeNamed("AnnéeCalendrier", "A2");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("B2", "l");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoInsertRangeNamed("DébutSemaine", "B2");
	CPPUNIT_ASSERT(wOk);
 
    

	//wOk = m_Api->UndoCellValue(
	//	"A10",
	//	"={0,1,2,3,4,5,6}+{0;1;2;3;4;5}*7+DATE(AnnéeCalendrier;1;1)-WEEKDAY(DATE(AnnéeCalendrier;1;1);(DébutSemaine=\"l\")+1)+1");
	wOk = m_Api->UndoCellValue("A10", kCalendarMatrixFormula);
	CPPUNIT_ASSERT(wOk);
 
	// Weekday header row: one TEXT per column above the spill (same idea as Calendar.sker R[1]C[0] + "jjj").
	// FR locale uses ';' as the argument separator (TEXT(D10;"jjj")).
	wOk = m_Api->UndoCellValue("A9", "=TEXT(A10;\"jjj\")");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("B9", "=TEXT(B10;\"jjj\")");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("C9", "=TEXT(C10;\"jjj\")");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("D9", "=TEXT(D10;\"jjj\")");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("E9", "=TEXT(E10;\"jjj\")");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("F9", "=TEXT(F10;\"jjj\")");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("G9", "=TEXT(G10;\"jjj\")");
	CPPUNIT_ASSERT(wOk);

	DrawCell("TestMatrixCalendar spill A10:G15", 9, 1, 15, 7);

	// FR locale abbreviated weekdays for 29/12/2025 … 04/01/2026 (Mon … Sun).
	CPPUNIT_ASSERT_MESSAGE("A9 Lun.", VariantStringEq(m_Api->CellValue("A9"), "Lun."));
	CPPUNIT_ASSERT_MESSAGE("B9 Mar.", VariantStringEq(m_Api->CellValue("B9"), "Mar."));
	CPPUNIT_ASSERT_MESSAGE("C9 Mer.", VariantStringEq(m_Api->CellValue("C9"), "Mer."));
	CPPUNIT_ASSERT_MESSAGE("D9 Jeu.", VariantStringEq(m_Api->CellValue("D9"), "Jeu."));
	CPPUNIT_ASSERT_MESSAGE("E9 Ven.", VariantStringEq(m_Api->CellValue("E9"), "Ven."));
	CPPUNIT_ASSERT_MESSAGE("F9 Sam.", VariantStringEq(m_Api->CellValue("F9"), "Sam."));
	CPPUNIT_ASSERT_MESSAGE("G9 Dim.", VariantStringEq(m_Api->CellValue("G9"), "Dim."));

	// 2026-01-01 is Thursday; week starting Monday "l" → row 10 = 29/12/2025 … 04/01/2026; 01/01/2026 = column D.
	CPPUNIT_ASSERT_MESSAGE("A10 Monday of week containing 1 Jan", VariantDateEq(m_Api->CellValue("A10"), 2025, 12, 29));
	CPPUNIT_ASSERT_MESSAGE("D10 New Year's Day", VariantDateEq(m_Api->CellValue("D10"), 2026, 1, 1));
	CPPUNIT_ASSERT_MESSAGE("G10 end of first week", VariantDateEq(m_Api->CellValue("G10"), 2026, 1, 4));
	CPPUNIT_ASSERT_MESSAGE("A11 second week Monday", VariantDateEq(m_Api->CellValue("A11"), 2026, 1, 5));
	CPPUNIT_ASSERT_MESSAGE("G15 last cell 6×7 grid", VariantDateEq(m_Api->CellValue("G15"), 2026, 2, 8));

	// AppendCellsInSpillRange: full 6×7 matrix from A10:G15 (42 cells), row-major.
	{
		tCell* wA10 = m_Api->Cell("A10");
		tCell* wG15 = m_Api->Cell("G15");
		CPPUNIT_ASSERT(wA10 != nullptr);
		CPPUNIT_ASSERT(wG15 != nullptr);
		std::vector<tCell*> wFromOrigin;
        wA10->AppendCellsInSpillRange(wFromOrigin);
		CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(42), wFromOrigin.size());
		CPPUNIT_ASSERT(wFromOrigin.front() == wA10);
		CPPUNIT_ASSERT(wFromOrigin.back() == wG15);
		// Same set when called from an extend cell (CellMatrixRoot → origin).
		tCell* wD12 = m_Api->Cell("D12");
		CPPUNIT_ASSERT(wD12 != nullptr);
		std::vector<tCell*> wFromExtend;
        wD12->AppendCellsInSpillRange(wFromExtend);
		CPPUNIT_ASSERT_EQUAL(static_cast<size_t>(42), wFromExtend.size());
		CPPUNIT_ASSERT(wFromExtend == wFromOrigin);
	}
}

void TestSkMatrix::TestMatrixCalendarOoxmlSingleRowRef() {
	// Mirrors SkExcel after Calendar.xlsx: one array-formula block = one row ref (e.g. B8:H8).
	// Order matches SkExcel2SpreadSheet::ApplyFormulas: apply formula first, then SetArrayFormulaOutputRect, then recalc.
	// (UndoCellValue runs ClearFormula() before assigning; that clears any prior array output rect — do not set afo before Undo.)
	// RecalculateAll() then uses ClampResultRectToArrayFormulaOutput so the matrix does not spill past the OOXML ref row.
	tBool 	wOk = m_Api->UndoCellValue("A1", "=DATE(2026;3;20)");
	CPPUNIT_ASSERT(wOk);
	CPPUNIT_ASSERT(VariantCalendarDateOk(m_Api->CellValue("A1")));
	wOk = m_Api->UndoCellValue("A2", "=YEAR(A1)");
	CPPUNIT_ASSERT(wOk);
	CPPUNIT_ASSERT_EQUAL(2026, m_Api->CellValue("A2").Int());

	wOk = m_Api->UndoInsertFormulaNamed("JoursEtSemaines", "{0,1,2,3,4,5,6}+{0;1;2;3;4;5}*7");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoInsertRangeNamed("AnnéeCalendrier", "A2");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("B2", "l");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoInsertRangeNamed("DébutSemaine", "B2");
	CPPUNIT_ASSERT(wOk);

	wOk = m_Api->UndoCellValue("A10", kCalendarMatrixFormula);
	CPPUNIT_ASSERT(wOk);

	tCell* wA10 = m_Api->Cell("A10");
	CPPUNIT_ASSERT(wA10 != nullptr);
	// One row × seven columns: same shape as Excel ref B8:H8 (here A10:G10 — row 10, cols 1..7).
	wA10->SetSpillRect(10, 1, 10, 7);
    CPPUNIT_ASSERT(wA10->IsSpillRange());

	tWorkBook* wWb = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT(wWb != nullptr);
	wWb->RecalculateAll();

	DrawCell("TestMatrixCalendarOoxmlSingleRowRef A10:G15", 10, 1, 15, 7);

	// Same first result row as TestMatrixCalendar (seven weekday cells); OOXML ref is one row × seven cols (e.g. B8:H8).
	for (tIndex wCol = 1; wCol <= 7; ++wCol) {
		CPPUNIT_ASSERT_MESSAGE(
			"OOXML output row must contain one week of dates",
			VariantCalendarDateOk(m_Api->CellValue(10, wCol)));
	}
	// Full spill in TestMatrixCalendar fills A10:G15; with a single-row ref, nothing below row 10 (stale spill cleared).
	for (tIndex wRow = 11; wRow <= 15; ++wRow) {
		for (tIndex wCol = 1; wCol <= 7; ++wCol) {
			CPPUNIT_ASSERT_MESSAGE(
				"No matrix spill below the OOXML ref (SkExcel parity)",
				m_Api->CellValue(wRow, wCol).Type() == tVariantType::t_null);
		}
	}
}

void TestSkMatrix::TestMatrixCalendarOoxmlSixBlocksLikeExcel() {
	// Matches Excel/Calendar.sker: six origins at rows 8,10,12,14,16,18 (cols A–G here vs B–H in xlsx), each spillrange one row × 7,
	// with distinct tails +1,+8,+15,+22,+29,+36 (sker fi[3]..fi[8]). Not the same formula on every row.
	tBool wOk = m_Api->UndoCellValue("A1", "=DATE(2026;3;20)");
	CPPUNIT_ASSERT(wOk);
	CPPUNIT_ASSERT(VariantCalendarDateOk(m_Api->CellValue("A1")));
	wOk = m_Api->UndoCellValue("A2", "=YEAR(A1)");
	CPPUNIT_ASSERT(wOk);
	CPPUNIT_ASSERT_EQUAL(2026, m_Api->CellValue("A2").Int());

	wOk = m_Api->UndoInsertFormulaNamed("JoursEtSemaines", "{0,1,2,3,4,5,6}+{0;1;2;3;4;5}*7");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoInsertRangeNamed("AnnéeCalendrier", "A2");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("B2", "l");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoInsertRangeNamed("DébutSemaine", "B2");
	CPPUNIT_ASSERT(wOk);

	static const tIndex kBlockRows[] = { 8, 10, 12, 14, 16, 18 };
	const tIndex wNbBlocks = static_cast<tIndex>(sizeof(kBlockRows) / sizeof(kBlockRows[0]));
	CPPUNIT_ASSERT_EQUAL(static_cast<tIndex>(6), wNbBlocks);
	for (tIndex wBi = 0; wBi < wNbBlocks; ++wBi) {
		const tIndex wRow = kBlockRows[wBi];
		tStringStream wRefStream;
		wRefStream << Base10ToAlpha(1) << wRow;
		const tString wRef = wRefStream.str();
        tCell* wOrigin = m_Api->EnsureCell(wRef);
		CPPUNIT_ASSERT(wOrigin != nullptr);
		wOrigin->SetSpillRect(wRow, 1, wRow, 7);
		const tString wFormula = CalendarSkerRowFormula(kCalendarSkerTailOffsets[wBi]);
		wOk = m_Api->UndoCellValue(wRef, wFormula);
		CPPUNIT_ASSERT(wOk);
        tStringStream wStream;
        wStream << "Set Cell " << wRef;
        DrawCell(wStream.str(), 8, 1, 21, 5);
		
        CPPUNIT_ASSERT(wOrigin->IsSpillRange());
	}

	tWorkBook* wWb = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT(wWb != nullptr);
	wWb->RecalculateAll();


	DrawCell("TestMatrixCalendarOoxmlSixBlocksLikeExcel A10:G21", 8, 1, 21, 5);


	for (const tIndex wRow : kBlockRows) {
		for (tIndex wCol = 1; wCol <= 7; ++wCol) {
			CPPUNIT_ASSERT_MESSAGE(
				"Each OOXML block row must show seven dates",
				VariantCalendarDateOk(m_Api->CellValue(wRow, wCol)));
		}
	}
	// Spot-check vs Calendar.xlsx / .sker for AnnéeCalendrier=2026, DébutSemaine="l" (Monday): row 8 = week of 1 Jan; row 10 = +7 days.
	CPPUNIT_ASSERT_MESSAGE("sker row 8 Mon", VariantDateEq(m_Api->CellValue(8, 1), 2025, 12, 29));
	CPPUNIT_ASSERT_MESSAGE("sker row 10 Mon", VariantDateEq(m_Api->CellValue(10, 1), 2026, 1, 5));
	CPPUNIT_ASSERT_MESSAGE("sker row 18 Mon (last block)", VariantDateEq(m_Api->CellValue(18, 1), 2026, 2, 2));
	// Separator rows between block rows must stay empty (no spill from the row above).
	static const tIndex kGapRows[] = { 9, 11, 13, 15, 17 };
	for (const tIndex wRow : kGapRows) {
		for (tIndex wCol = 1; wCol <= 7; ++wCol) {
			CPPUNIT_ASSERT_MESSAGE(
				"Gap row between calendar blocks must stay empty",
				m_Api->CellValue(wRow, wCol).Type() == tVariantType::t_null);
		}
	}
	for (tIndex wRow = 19; wRow <= 21; ++wRow) {
		for (tIndex wCol = 1; wCol <= 7; ++wCol) {
			CPPUNIT_ASSERT_MESSAGE(
				"No spill below the last block ref (row 18)",
				m_Api->CellValue(wRow, wCol).Type() == tVariantType::t_null);
		}
	}
}

void TestSkMatrix::TestMatrixCalendarSkerReadJson() {
	const tString wFileName = MatrixTestExcelDataDir() + "Calendar.sker";
	tFile wFile(wFileName);
	if (!wFile.Exist()) {
		cout << "\nTestMatrixCalendarSkerReadJson: file not found, skipping:\n  " << wFileName << endl;
		return;
	}

	tearDown();
	setUp();

	CPPUNIT_ASSERT_MESSAGE("ReadJson Calendar.sker", m_Api->ReadJson(wFile.LoadString()));

	m_Api->ActiveSheet("JANVIER");

	tWorkBook* wWb = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT(wWb != nullptr);
	// Spill slaves C9:H9 have no persisted v/t; origin B9 is cached. Recalc fills the week
	// and TEXT(R[2]C[0]) headers that depend on those slaves.
	wWb->RecalculateAll();

	// Calendar.sker (2026-08-15): date blocks on rows 9,11,13,15,17,19 (headers on 7–8).
	static const tIndex kBlockRows[] = { 9, 11, 13, 15, 17, 19 };
	for (const tIndex wRow : kBlockRows) {
		for (tIndex wCol = 2; wCol <= 8; ++wCol) {
			CPPUNIT_ASSERT_MESSAGE(
				"Calendar.sker block row must show seven dates after ReadJson",
				VariantCalendarDateOk(m_Api->CellValue(wRow, wCol)));
		}
	}
	static const tIndex kGapRows[] = { 10, 12, 14, 16, 18 };
	for (const tIndex wRow : kGapRows) {
		for (tIndex wCol = 2; wCol <= 8; ++wCol) {
			CPPUNIT_ASSERT_MESSAGE(
				"Gap row between calendar blocks must stay empty after ReadJson",
				m_Api->CellValue(wRow, wCol).Type() == tVariantType::t_null);
		}
	}

	tApplication::Instance()->Locale("fr");
	wWb->RecalculateAll();
	// Default file: DébutSemaine empty / not "l" → Sunday week; 2026-01-01 is Thursday.
	// Row 9 = 28/12/2025 … 03/01/2026; TEXT(R[2]C[0];"jjj") on row 7.
	static const tChar* kSundayWeek[] = { "Dim.", "Lun.", "Mar.", "Mer.", "Jeu.", "Ven.", "Sam." };
	for (tIndex wCol = 2; wCol <= 8; ++wCol) {
		CPPUNIT_ASSERT_MESSAGE(
			"Calendar.sker TEXT header must match the date column (not a neighbour)",
			VariantStringEq(m_Api->CellValue(7, wCol), kSundayWeek[wCol - 2]));
	}
}

void TestSkMatrix::TestMatrixCalendar12MonthSkerReadJson() {
	// Filename uses NBSP (U+00A0) between "12" and "mois", same as the shared workbook.
	const tString wFileName = MatrixTestExcelDataDir() + "Calendrier sur 12\u00a0mois1.sker";
	tFile wFile(wFileName);
	if (!wFile.Exist()) {
		cout << "\nTestMatrixCalendar12MonthSkerReadJson: file not found, skipping:\n  " << wFileName << endl;
		return;
	}

	tearDown();
	setUp();

	CPPUNIT_ASSERT_MESSAGE("ReadJson 12-month calendar", m_Api->ReadJson(wFile.LoadString()));
	m_Api->ActiveSheet("JANVIER");
	tWorkBook* wWb = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT(wWb != nullptr);
	wWb->RecalculateAll();

	static const tIndex kBlockRows[] = { 8, 10, 12, 14, 16, 18 };
	auto wAssertSevenDates = [&](const char* sTag) {
		for (const tIndex wRow : kBlockRows) {
			for (tIndex wCol = 2; wCol <= 8; ++wCol) {
				CPPUNIT_ASSERT_MESSAGE(sTag, VariantCalendarDateOk(m_Api->CellValue(wRow, wCol)));
			}
		}
	};
	wAssertSevenDates("12-month calendar must spill seven dates per week row after ReadJson");

	CPPUNIT_ASSERT(m_Api->UndoCellValue("E3", "l"));
	wAssertSevenDates("E3=l must keep C–H filled (Monday week)");
	CPPUNIT_ASSERT_MESSAGE("B8 Monday 29 Dec 2025", VariantDateEq(m_Api->CellValue("B8"), 2025, 12, 29));
	CPPUNIT_ASSERT_MESSAGE("C8 Tuesday 30 Dec 2025", VariantDateEq(m_Api->CellValue("C8"), 2025, 12, 30));

	CPPUNIT_ASSERT(m_Api->UndoCellValue("E3", "d"));
	wAssertSevenDates("E3=d must keep C–H filled (Sunday week)");
	CPPUNIT_ASSERT_MESSAGE("B8 Sunday 28 Dec 2025", VariantDateEq(m_Api->CellValue("B8"), 2025, 12, 28));
	CPPUNIT_ASSERT_MESSAGE("C8 Monday 29 Dec 2025", VariantDateEq(m_Api->CellValue("C8"), 2025, 12, 29));
}

void TestSkMatrix::TestMatrixCalendarR1C1Headers() {
	// Same layout as Calendar.sker JANVIER: one-row spill on B9:H9, TEXT(R[2]C[0];"jjj") on B7:H7.
	tApplication::Instance()->Locale("fr");
	tBool wOk = m_Api->UndoCellValue("A1", "=DATE(2026;1;1)");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("A2", "=YEAR(A1)");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoInsertFormulaNamed("JoursEtSemaines", "{0,1,2,3,4,5,6}+{0;1;2;3;4;5}*7");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoInsertRangeNamed("AnnéeCalendrier", "A2");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoCellValue("E3", "d");
	CPPUNIT_ASSERT(wOk);
	wOk = m_Api->UndoInsertRangeNamed("DébutSemaine", "E3");
	CPPUNIT_ASSERT(wOk);

	wOk = m_Api->UndoCellValue("B9", CalendarSkerRowFormula(1));
	CPPUNIT_ASSERT(wOk);
	tCell* wB9 = m_Api->Cell("B9");
	CPPUNIT_ASSERT(wB9 != nullptr);
	wB9->SetSpillRect(9, 2, 9, 8);

	tWorkBook* wWb = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT(wWb != nullptr);
	wWb->RecalculateAll();

	static const tChar* kHeaderRefs[] = { "B7", "C7", "D7", "E7", "F7", "G7", "H7" };
	for (const tChar* wRef : kHeaderRefs) {
		wOk = m_Api->UndoCellValue(wRef, "=TEXT(R[2]C[0];\"jjj\")");
		CPPUNIT_ASSERT(wOk);
	}

	DrawCell("TestMatrixCalendarR1C1Headers B7:H9", 7, 2, 9, 8);

	static const tChar* kSundayWeek[] = { "Dim.", "Lun.", "Mar.", "Mer.", "Jeu.", "Ven.", "Sam." };
	for (tIndex wCol = 2; wCol <= 8; ++wCol) {
		CPPUNIT_ASSERT_MESSAGE(
			"R1C1 TEXT(R[2]C[0]) on a spill slave must be that column's weekday",
			VariantStringEq(m_Api->CellValue(7, wCol), kSundayWeek[wCol - 2]));
	}
	CPPUNIT_ASSERT_MESSAGE("B9 Sunday 28 Dec 2025", VariantDateEq(m_Api->CellValue("B9"), 2025, 12, 28));
	CPPUNIT_ASSERT_MESSAGE("C9 Monday 29 Dec 2025", VariantDateEq(m_Api->CellValue("C9"), 2025, 12, 29));
	CPPUNIT_ASSERT_MESSAGE("F9 Thursday 1 Jan 2026", VariantDateEq(m_Api->CellValue("F9"), 2026, 1, 1));
}

void TestSkMatrix::TestMatrixPlusScalar() {
	// Create a 2x2 matrix with values 1, 2, 3, 4
	m_Api->UndoCellValue("A1", 1);
	m_Api->UndoCellValue("B1", 2);
	m_Api->UndoCellValue("A2", 3);
	m_Api->UndoCellValue("B2", 4);
	
	DrawCell("TestMatrixPlusScalar - Source matrix A1:B2:", 1, 1, 2, 2);
	
	// Test Matrix + Scalar: =A1:B2+10
	// Note: The formula is entered in D1, and should fill D1:E2 with results
	m_Api->UndoCellValue("D1", "=A1:B2+10");
	
	DrawCell("TestMatrixPlusScalar - Result range D1:E2:", 1, 4, 2, 5);
	
	// Verify results: should be 11, 12, 13, 14
	// Check if values are written in the result range
	tVariant wValD1 = m_Api->CellValue(1, 4);
	tVariant wValE1 = m_Api->CellValue(1, 5);
	tVariant wValD2 = m_Api->CellValue(2, 4);
	tVariant wValE2 = m_Api->CellValue(2, 5);
	
	CPPUNIT_ASSERT_MESSAGE("Matrix+Scalar D1", VariantNumericEq(wValD1, 11.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix+Scalar E1", VariantNumericEq(wValE1, 12.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix+Scalar D2", VariantNumericEq(wValD2, 13.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix+Scalar E2", VariantNumericEq(wValE2, 14.0));
}

void TestSkMatrix::TestMatrixPlusMatrix() {
	// Create first matrix: 1, 2, 3, 4
	m_Api->UndoCellValue("A1", 1);
	m_Api->UndoCellValue("B1", 2);
	m_Api->UndoCellValue("A2", 3);
	m_Api->UndoCellValue("B2", 4);
	
	// Create second matrix: 5, 6, 7, 8
	m_Api->UndoCellValue("C1", 5);
	m_Api->UndoCellValue("D1", 6);
	m_Api->UndoCellValue("C2", 7);
	m_Api->UndoCellValue("D2", 8);
	
	DrawCell("TestMatrixPlusMatrix - Matrix 1 A1:B2:", 1, 1, 2, 2);
	DrawCell("TestMatrixPlusMatrix - Matrix 2 C1:D2:", 1, 3, 2, 4);
	
	// Test Matrix + Matrix: =A1:B2+C1:D2
	m_Api->UndoCellValue("E1", "=A1:B2+C1:D2");
 
    
	DrawCell("TestMatrixPlusMatrix - Result range E1:F2:", 1, 5, 2, 6);
	
	// Verify results: should be 6, 8, 10, 12
	CPPUNIT_ASSERT_MESSAGE("Matrix+Matrix E1", VariantNumericEq(m_Api->CellValue(1, 5), 6.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix+Matrix F1", VariantNumericEq(m_Api->CellValue(1, 6), 8.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix+Matrix E2", VariantNumericEq(m_Api->CellValue(2, 5), 10.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix+Matrix F2", VariantNumericEq(m_Api->CellValue(2, 6), 12.0));
 
 
    tCell* wCellE1=m_Api->Cell("E1");
    tCell* wCellE2=m_Api->Cell("E2");
    //cout << endl << wRect.StrRef() << endl;
    CPPUNIT_ASSERT_MESSAGE("Matrix+Matrix E1:F2", wCellE1->MatrixRange()->StrRef()=="E1:F2");
    CPPUNIT_ASSERT_MESSAGE("Matrix+Matrix E1:F2", wCellE2->MatrixRange()->StrRef()=="E1:F2");
}

void TestSkMatrix::TestMatrixMinusScalar() {
	// Create a 2x2 matrix with values 10, 20, 30, 40
	m_Api->UndoCellValue("A1", 10);
	m_Api->UndoCellValue("B1", 20);
	m_Api->UndoCellValue("A2", 30);
	m_Api->UndoCellValue("B2", 40);
	
	DrawCell("TestMatrixMinusScalar - Source matrix A1:B2:", 1, 1, 2, 2);
	
	// Test Matrix - Scalar: =A1:B2-5
	m_Api->UndoCellValue("D1", "=A1:B2-5");
	
	DrawCell("TestMatrixMinusScalar - Result range D1:E2:", 1, 4, 2, 5);
	
	// Verify results: should be 5, 15, 25, 35
	CPPUNIT_ASSERT_MESSAGE("Matrix-Scalar D1", VariantNumericEq(m_Api->CellValue(1, 4), 5.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix-Scalar E1", VariantNumericEq(m_Api->CellValue(1, 5), 15.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix-Scalar D2", VariantNumericEq(m_Api->CellValue(2, 4), 25.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix-Scalar E2", VariantNumericEq(m_Api->CellValue(2, 5), 35.0));
 
    m_Api->Undo();
    
    DrawCell("TestMatrixMinusScalar - Result range D1:E2:", 1, 4, 2, 5);
	
}

void TestSkMatrix::TestMatrixMinusMatrix() {
	// Create first matrix: 10, 20, 30, 40
	m_Api->UndoCellValue("A1", 10);
	m_Api->UndoCellValue("B1", 20);
	m_Api->UndoCellValue("A2", 30);
	m_Api->UndoCellValue("B2", 40);
	
	// Create second matrix: 1, 2, 3, 4
	m_Api->UndoCellValue("C1", 1);
	m_Api->UndoCellValue("D1", 2);
	m_Api->UndoCellValue("C2", 3);
	m_Api->UndoCellValue("D2", 4);
	
	DrawCell("TestMatrixMinusMatrix - Source matrices A1:D2:", 1, 1, 2, 4);
	
	// Test Matrix - Matrix: =A1:B2-C1:D2
	m_Api->UndoCellValue("E1", "=A1:B2-C1:D2");
	
	DrawCell("TestMatrixMinusMatrix - Result range E1:F2:", 1, 5, 2, 6);
	
	// Verify results: should be 9, 18, 27, 36
	CPPUNIT_ASSERT_MESSAGE("Matrix-Matrix E1", VariantNumericEq(m_Api->CellValue(1, 5), 9.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix-Matrix F1", VariantNumericEq(m_Api->CellValue(1, 6), 18.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix-Matrix E2", VariantNumericEq(m_Api->CellValue(2, 5), 27.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix-Matrix F2", VariantNumericEq(m_Api->CellValue(2, 6), 36.0));
}

void TestSkMatrix::TestMatrixMultiplyScalar() {
	// Create a 2x2 matrix with values 2, 3, 4, 5
	m_Api->UndoCellValue("A1", 2);
	m_Api->UndoCellValue("B1", 3);
	m_Api->UndoCellValue("A2", 4);
	m_Api->UndoCellValue("B2", 5);
	
	DrawCell("TestMatrixMultiplyScalar - Source matrix A1:B2:", 1, 1, 2, 2);
	
	// Test Matrix * Scalar: =A1:B2*3
	m_Api->UndoCellValue("D1", "=A1:B2*3");
	
	DrawCell("TestMatrixMultiplyScalar - Result range D1:E2:", 1, 4, 2, 5);
	
	// Verify results: should be 6, 9, 12, 15
	CPPUNIT_ASSERT_MESSAGE("Matrix*Scalar D1", VariantNumericEq(m_Api->CellValue(1, 4), 6.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix*Scalar E1", VariantNumericEq(m_Api->CellValue(1, 5), 9.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix*Scalar D2", VariantNumericEq(m_Api->CellValue(2, 4), 12.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix*Scalar E2", VariantNumericEq(m_Api->CellValue(2, 5), 15.0));
}

void TestSkMatrix::TestMatrixMultiplyMatrix() {
	// Create first matrix: 2, 3, 4, 5
	m_Api->UndoCellValue("A1", 2);
	m_Api->UndoCellValue("B1", 3);
	m_Api->UndoCellValue("A2", 4);
	m_Api->UndoCellValue("B2", 5);
	
	// Create second matrix: 1, 2, 3, 4
	m_Api->UndoCellValue("C1", 1);
	m_Api->UndoCellValue("D1", 2);
	m_Api->UndoCellValue("C2", 3);
	m_Api->UndoCellValue("D2", 4);
	
	DrawCell("TestMatrixMultiplyMatrix - Source matrices A1:D2:", 1, 1, 2, 4);
	
	// Test Matrix * Matrix (element-wise): =A1:B2*C1:D2
	m_Api->UndoCellValue("E1", "=A1:B2*C1:D2");
	
	DrawCell("TestMatrixMultiplyMatrix - Result range E1:F2:", 1, 5, 2, 6);
	
	// Verify results: should be 2, 6, 12, 20
	CPPUNIT_ASSERT_MESSAGE("Matrix*Matrix E1", VariantNumericEq(m_Api->CellValue(1, 5), 2.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix*Matrix F1", VariantNumericEq(m_Api->CellValue(1, 6), 6.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix*Matrix E2", VariantNumericEq(m_Api->CellValue(2, 5), 12.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix*Matrix F2", VariantNumericEq(m_Api->CellValue(2, 6), 20.0));
}

void TestSkMatrix::TestMatrixDivideScalar() {
	// Create a 2x2 matrix with values 10, 20, 30, 40
	m_Api->UndoCellValue("A1", 10);
	m_Api->UndoCellValue("B1", 20);
	m_Api->UndoCellValue("A2", 30);
	m_Api->UndoCellValue("B2", 40);
	
	DrawCell("TestMatrixDivideScalar - Source matrix A1:B2:", 1, 1, 2, 2);
	
	// Test Matrix / Scalar: =A1:B2/2
	m_Api->UndoCellValue("D1", "=A1:B2/2");
	
	DrawCell("TestMatrixDivideScalar - Result range D1:E2:", 1, 4, 2, 5);
	
	// Verify results: should be 5, 10, 15, 20
	CPPUNIT_ASSERT_MESSAGE("Matrix/Scalar D1", VariantNumericEq(m_Api->CellValue(1, 4), 5.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix/Scalar E1", VariantNumericEq(m_Api->CellValue(1, 5), 10.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix/Scalar D2", VariantNumericEq(m_Api->CellValue(2, 4), 15.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix/Scalar E2", VariantNumericEq(m_Api->CellValue(2, 5), 20.0));
}

void TestSkMatrix::TestMatrixDivideMatrix() {
	// Create first matrix: 20, 30, 40, 50
	m_Api->UndoCellValue("A1", 20);
	m_Api->UndoCellValue("B1", 30);
	m_Api->UndoCellValue("A2", 40);
	m_Api->UndoCellValue("B2", 50);
	
	// Create second matrix: 2, 3, 4, 5
	m_Api->UndoCellValue("C1", 2);
	m_Api->UndoCellValue("D1", 3);
	m_Api->UndoCellValue("C2", 4);
	m_Api->UndoCellValue("D2", 5);
	
	DrawCell("TestMatrixDivideMatrix - Source matrices A1:D2:", 1, 1, 2, 4);
	
	// Test Matrix / Matrix (element-wise): =A1:B2/C1:D2
	m_Api->UndoCellValue("E1", "=A1:B2/C1:D2");
	
	DrawCell("TestMatrixDivideMatrix - Result range E1:F2:", 1, 5, 2, 6);
	
	// Verify results: should be 10, 10, 10, 10
	CPPUNIT_ASSERT_MESSAGE("Matrix/Matrix E1", VariantNumericEq(m_Api->CellValue(1, 5), 10.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix/Matrix F1", VariantNumericEq(m_Api->CellValue(1, 6), 10.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix/Matrix E2", VariantNumericEq(m_Api->CellValue(2, 5), 10.0));
	CPPUNIT_ASSERT_MESSAGE("Matrix/Matrix F2", VariantNumericEq(m_Api->CellValue(2, 6), 10.0));
}

void TestSkMatrix::TestMatrixUnaryMinus() {
	// Create a 2x2 matrix with values 5, 10, 15, 20
	m_Api->UndoCellValue("A1", 5);
	m_Api->UndoCellValue("B1", 10);
	m_Api->UndoCellValue("A2", 15);
	m_Api->UndoCellValue("B2", 20);
	
	DrawCell("TestMatrixUnaryMinus - Source matrix A1:B2:", 1, 1, 2, 2);
	
	// Test UnaryMinus: =-A1:B2
	m_Api->UndoCellValue("D1", "=-A1:B2");
	
	DrawCell("TestMatrixUnaryMinus - Result range D1:E2:", 1, 4, 2, 5);
	
	// Verify results: should be -5, -10, -15, -20
	// Note: For UnaryMinus, we need to check if the value is a range or a variant
	tVariant wValD1 = m_Api->CellValue(1, 4);
	tVariant wValE1 = m_Api->CellValue(1, 5);
	tVariant wValD2 = m_Api->CellValue(2, 4);
	tVariant wValE2 = m_Api->CellValue(2, 5);
	
	CPPUNIT_ASSERT_MESSAGE("UnaryMinus D1", VariantNumericEq(wValD1, -5.0));
	CPPUNIT_ASSERT_MESSAGE("UnaryMinus E1", VariantNumericEq(wValE1, -10.0));
	CPPUNIT_ASSERT_MESSAGE("UnaryMinus D2", VariantNumericEq(wValD2, -15.0));
	CPPUNIT_ASSERT_MESSAGE("UnaryMinus E2", VariantNumericEq(wValE2, -20.0));
}

void TestSkMatrix::TestMatrixAmpersand() {
	// Create a 2x2 matrix with string values
	m_Api->UndoCellValue("A1", "Hello");
	m_Api->UndoCellValue("B1", "World");
	m_Api->UndoCellValue("A2", "Test");
	m_Api->UndoCellValue("B2", "Matrix");
	
	DrawCell("TestMatrixAmpersand - Source matrix A1:B2:", 1, 1, 2, 2);
	
	// Test Matrix & Scalar (string concatenation): =A1:B2&"!"
	m_Api->UndoCellValue("D1", "=A1:B2&\"!\"");
	
	DrawCell("TestMatrixAmpersand - Result range D1:E2:", 1, 4, 2, 5);
	
	// Verify results: should be "Hello!", "World!", "Test!", "Matrix!"
	CPPUNIT_ASSERT_MESSAGE("Matrix&Scalar D1", VariantStringEq(m_Api->CellValue(1, 4), "Hello!"));
	CPPUNIT_ASSERT_MESSAGE("Matrix&Scalar E1", VariantStringEq(m_Api->CellValue(1, 5), "World!"));
	CPPUNIT_ASSERT_MESSAGE("Matrix&Scalar D2", VariantStringEq(m_Api->CellValue(2, 4), "Test!"));
	CPPUNIT_ASSERT_MESSAGE("Matrix&Scalar E2", VariantStringEq(m_Api->CellValue(2, 5), "Matrix!"));
}

// Dynamic arrays ============================================================
void TestSkMatrix::TestSequence() {
	// Vertical: =SEQUENCE(3) at A1 -> A1..A3 = 1,2,3
	CPPUNIT_ASSERT_MESSAGE("compile SEQUENCE(3)", m_Api->UndoCellValue("A1", "=SEQUENCE(3)"));
	DrawCell("SEQUENCE(3) A1:A3:", 1, 1, 3, 1);
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE(3) A1", VariantNumericEq(m_Api->CellValue(1, 1), 1.0));
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE(3) A2", VariantNumericEq(m_Api->CellValue(2, 1), 2.0));
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE(3) A3", VariantNumericEq(m_Api->CellValue(3, 1), 3.0));

	// Horizontal: =SEQUENCE(1;3) at E1 -> E1..G1 = 1,2,3 (cols 5,6,7).
	// Note: the engine's argument separator is ';' ('.,' is a decimal separator, so "1,3" parses as the number 1.3).
	CPPUNIT_ASSERT_MESSAGE("compile SEQUENCE(1;3)", m_Api->UndoCellValue("E1", "=SEQUENCE(1;3)"));
	DrawCell("SEQUENCE(1;3) E1:G1:", 1, 5, 1, 7);
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE(1;3) E1", VariantNumericEq(m_Api->CellValue(1, 5), 1.0));
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE(1;3) F1", VariantNumericEq(m_Api->CellValue(1, 6), 2.0));
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE(1;3) G1", VariantNumericEq(m_Api->CellValue(1, 7), 3.0));

	// 2D: =SEQUENCE(2;3) at A10 -> row-major 1..6 over A10:C11
	CPPUNIT_ASSERT_MESSAGE("compile SEQUENCE(2;3)", m_Api->UndoCellValue("A10", "=SEQUENCE(2;3)"));
	DrawCell("SEQUENCE(2;3) A10:C11:", 10, 1, 11, 3);
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE(2;3) A10", VariantNumericEq(m_Api->CellValue(10, 1), 1.0));
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE(2;3) B10", VariantNumericEq(m_Api->CellValue(10, 2), 2.0));
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE(2;3) C10", VariantNumericEq(m_Api->CellValue(10, 3), 3.0));
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE(2;3) A11", VariantNumericEq(m_Api->CellValue(11, 1), 4.0));
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE(2;3) B11", VariantNumericEq(m_Api->CellValue(11, 2), 5.0));
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE(2;3) C11", VariantNumericEq(m_Api->CellValue(11, 3), 6.0));

	// start/step: =SEQUENCE(4;1;10;5) at E10 -> 10,15,20,25 (E10:E13)
	CPPUNIT_ASSERT_MESSAGE("compile SEQUENCE(4;1;10;5)", m_Api->UndoCellValue("E10", "=SEQUENCE(4;1;10;5)"));
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE start/step E10", VariantNumericEq(m_Api->CellValue(10, 5), 10.0));
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE start/step E11", VariantNumericEq(m_Api->CellValue(11, 5), 15.0));
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE start/step E12", VariantNumericEq(m_Api->CellValue(12, 5), 20.0));
	CPPUNIT_ASSERT_MESSAGE("SEQUENCE start/step E13", VariantNumericEq(m_Api->CellValue(13, 5), 25.0));

	// RANDARRAY: spill shape + integer bounds (volatile values — only range-check).
	CPPUNIT_ASSERT_MESSAGE("compile RANDARRAY(2;3)", m_Api->UndoCellValue("A20", "=RANDARRAY(2;3)"));
	DrawCell("RANDARRAY(2;3) A20:C21:", 20, 1, 21, 3);
	for (int r = 20; r <= 21; ++r) {
		for (int c = 1; c <= 3; ++c) {
			const tVariant wV = m_Api->CellValue(r, c);
			CPPUNIT_ASSERT_MESSAGE("RANDARRAY cell numeric", wV.IsNumeric());
			CPPUNIT_ASSERT_MESSAGE("RANDARRAY in [0,1]",
				wV.Numeric() >= 0.0 && wV.Numeric() <= 1.0);
		}
	}
	CPPUNIT_ASSERT_MESSAGE("compile RANDARRAY(1;2;5;10;TRUE)",
		m_Api->UndoCellValue("E20", "=RANDARRAY(1;2;5;10;TRUE)"));
	{
		const tVariant wE20 = m_Api->CellValue(20, 5);
		const tVariant wF20 = m_Api->CellValue(20, 6);
		CPPUNIT_ASSERT_MESSAGE("RANDARRAY int E20", wE20.IsNumeric());
		CPPUNIT_ASSERT_MESSAGE("RANDARRAY int F20", wF20.IsNumeric());
		CPPUNIT_ASSERT_MESSAGE("RANDARRAY int E20 range",
			wE20.Numeric() >= 5.0 && wE20.Numeric() <= 10.0);
		CPPUNIT_ASSERT_MESSAGE("RANDARRAY int F20 range",
			wF20.Numeric() >= 5.0 && wF20.Numeric() <= 10.0);
	}
}

void TestSkMatrix::TestSort() {
	// Source values J1..J3 = 3,1,2 (col 10).
	CPPUNIT_ASSERT_MESSAGE("J1", m_Api->UndoCellValue("J1", 3));
	CPPUNIT_ASSERT_MESSAGE("J2", m_Api->UndoCellValue("J2", 1));
	CPPUNIT_ASSERT_MESSAGE("J3", m_Api->UndoCellValue("J3", 2));

	// Ascending: =SORT(J1:J3) at L1 -> 1,2,3 (col 12).
	CPPUNIT_ASSERT_MESSAGE("compile SORT asc", m_Api->UndoCellValue("L1", "=SORT(J1:J3)"));
	DrawCell("SORT(J1:J3) L1:L3:", 1, 12, 3, 12);
	CPPUNIT_ASSERT_MESSAGE("SORT asc L1", VariantNumericEq(m_Api->CellValue(1, 12), 1.0));
	CPPUNIT_ASSERT_MESSAGE("SORT asc L2", VariantNumericEq(m_Api->CellValue(2, 12), 2.0));
	CPPUNIT_ASSERT_MESSAGE("SORT asc L3", VariantNumericEq(m_Api->CellValue(3, 12), 3.0));

	// Descending: =SORT(J1:J3;1;-1) at N1 -> 3,2,1 (col 14).
	CPPUNIT_ASSERT_MESSAGE("compile SORT desc", m_Api->UndoCellValue("N1", "=SORT(J1:J3;1;-1)"));
	DrawCell("SORT(J1:J3;1;-1) N1:N3:", 1, 14, 3, 14);
	CPPUNIT_ASSERT_MESSAGE("SORT desc N1", VariantNumericEq(m_Api->CellValue(1, 14), 3.0));
	CPPUNIT_ASSERT_MESSAGE("SORT desc N2", VariantNumericEq(m_Api->CellValue(2, 14), 2.0));
	CPPUNIT_ASSERT_MESSAGE("SORT desc N3", VariantNumericEq(m_Api->CellValue(3, 14), 1.0));
}

void TestSkMatrix::TestSortNested() {
	// Nested in-memory array: =SORT(SEQUENCE(5);1;-1) at A1 -> 5,4,3,2,1.
	CPPUNIT_ASSERT_MESSAGE("compile SORT(SEQUENCE(5);1;-1)",
		m_Api->UndoCellValue("A1", "=SORT(SEQUENCE(5);1;-1)"));
	DrawCell("SORT(SEQUENCE(5);1;-1) A1:A5:", 1, 1, 5, 1);
	CPPUNIT_ASSERT_MESSAGE("nested A1", VariantNumericEq(m_Api->CellValue(1, 1), 5.0));
	CPPUNIT_ASSERT_MESSAGE("nested A2", VariantNumericEq(m_Api->CellValue(2, 1), 4.0));
	CPPUNIT_ASSERT_MESSAGE("nested A3", VariantNumericEq(m_Api->CellValue(3, 1), 3.0));
	CPPUNIT_ASSERT_MESSAGE("nested A4", VariantNumericEq(m_Api->CellValue(4, 1), 2.0));
	CPPUNIT_ASSERT_MESSAGE("nested A5", VariantNumericEq(m_Api->CellValue(5, 1), 1.0));

	// Literal array argument: =SORT({3;1;2}) at H1 -> 1,2,3 (col 8).
	CPPUNIT_ASSERT_MESSAGE("compile SORT({3;1;2})", m_Api->UndoCellValue("H1", "=SORT({3;1;2})"));
	DrawCell("SORT({3;1;2}) H1:H3:", 1, 8, 3, 8);
	CPPUNIT_ASSERT_MESSAGE("literal H1", VariantNumericEq(m_Api->CellValue(1, 8), 1.0));
	CPPUNIT_ASSERT_MESSAGE("literal H2", VariantNumericEq(m_Api->CellValue(2, 8), 2.0));
	CPPUNIT_ASSERT_MESSAGE("literal H3", VariantNumericEq(m_Api->CellValue(3, 8), 3.0));
}

void TestSkMatrix::TestSortMultiKey() {
	// Multi-key sort: sort_index and sort_order are arrays, applied in order (first key
	// decides, ties fall through to the next). Source P1:R4 (cols 16,17,18):
	//   key1  key2  label
	//    2     5     A
	//    1     9     B
	//    2     8     C
	//    1     3     D
	m_Api->UndoCellValue("P1", 2); m_Api->UndoCellValue("Q1", 5); m_Api->UndoCellValue("R1", "A");
	m_Api->UndoCellValue("P2", 1); m_Api->UndoCellValue("Q2", 9); m_Api->UndoCellValue("R2", "B");
	m_Api->UndoCellValue("P3", 2); m_Api->UndoCellValue("Q3", 8); m_Api->UndoCellValue("R3", "C");
	m_Api->UndoCellValue("P4", 1); m_Api->UndoCellValue("Q4", 3); m_Api->UndoCellValue("R4", "D");

	// sort_index / sort_order supplied as ranges (col 27 = {1,2}, col 28 = {-1,-1}, col 29 = {1,-1}).
	m_Api->UndoCellValue("AA1", 1);  m_Api->UndoCellValue("AA2", 2);   // keys: col1 then col2
	m_Api->UndoCellValue("AB1", -1); m_Api->UndoCellValue("AB2", -1);  // orders: DESC, DESC
	m_Api->UndoCellValue("AC1", 1);  m_Api->UndoCellValue("AC2", -1);  // orders: ASC, DESC

	// Sort by key1 DESC then key2 DESC -> rows C(2,8), A(2,5), B(1,9), D(1,3).
	CPPUNIT_ASSERT_MESSAGE("compile SORT multi-key",
		m_Api->UndoCellValue("T1", "=SORT(P1:R4;AA1:AA2;AB1:AB2)"));
	DrawCell("SORT multi-key T1:V4:", 1, 20, 4, 22);
	// key1 column (col 20): 2,2,1,1.
	CPPUNIT_ASSERT_MESSAGE("multi T1", VariantNumericEq(m_Api->CellValue(1, 20), 2.0));
	CPPUNIT_ASSERT_MESSAGE("multi T2", VariantNumericEq(m_Api->CellValue(2, 20), 2.0));
	CPPUNIT_ASSERT_MESSAGE("multi T3", VariantNumericEq(m_Api->CellValue(3, 20), 1.0));
	CPPUNIT_ASSERT_MESSAGE("multi T4", VariantNumericEq(m_Api->CellValue(4, 20), 1.0));
	// key2 column (col 21): 8,5,9,3 (tiebreak within each key1 group).
	CPPUNIT_ASSERT_MESSAGE("multi U1", VariantNumericEq(m_Api->CellValue(1, 21), 8.0));
	CPPUNIT_ASSERT_MESSAGE("multi U2", VariantNumericEq(m_Api->CellValue(2, 21), 5.0));
	CPPUNIT_ASSERT_MESSAGE("multi U3", VariantNumericEq(m_Api->CellValue(3, 21), 9.0));
	CPPUNIT_ASSERT_MESSAGE("multi U4", VariantNumericEq(m_Api->CellValue(4, 21), 3.0));

	// Mixed order: key1 ASC then key2 DESC -> rows B(1,9), D(1,3), C(2,8), A(2,5).
	CPPUNIT_ASSERT_MESSAGE("compile SORT multi-key mixed",
		m_Api->UndoCellValue("X1", "=SORT(P1:R4;AA1:AA2;AC1:AC2)"));
	DrawCell("SORT multi-key mixed X1:Z4:", 1, 24, 4, 26);
	CPPUNIT_ASSERT_MESSAGE("mixed X1", VariantNumericEq(m_Api->CellValue(1, 24), 1.0));
	CPPUNIT_ASSERT_MESSAGE("mixed Y1", VariantNumericEq(m_Api->CellValue(1, 25), 9.0));
	CPPUNIT_ASSERT_MESSAGE("mixed X2", VariantNumericEq(m_Api->CellValue(2, 24), 1.0));
	CPPUNIT_ASSERT_MESSAGE("mixed Y2", VariantNumericEq(m_Api->CellValue(2, 25), 3.0));
	CPPUNIT_ASSERT_MESSAGE("mixed X3", VariantNumericEq(m_Api->CellValue(3, 24), 2.0));
	CPPUNIT_ASSERT_MESSAGE("mixed Y3", VariantNumericEq(m_Api->CellValue(3, 25), 8.0));
	CPPUNIT_ASSERT_MESSAGE("mixed X4", VariantNumericEq(m_Api->CellValue(4, 24), 2.0));
	CPPUNIT_ASSERT_MESSAGE("mixed Y4", VariantNumericEq(m_Api->CellValue(4, 25), 5.0));

	// Literal positive multi-key array: {1;2} => both keys ascending (default order).
	// key1 ASC then key2 ASC -> rows D(1,3), B(1,9), A(2,5), C(2,8).
	CPPUNIT_ASSERT_MESSAGE("compile SORT multi-key literal",
		m_Api->UndoCellValue("AE1", "=SORT(P1:R4;{1;2})"));
	DrawCell("SORT multi-key literal AE1:AG4:", 1, 31, 4, 33);
	CPPUNIT_ASSERT_MESSAGE("lit AE1", VariantNumericEq(m_Api->CellValue(1, 31), 1.0));
	CPPUNIT_ASSERT_MESSAGE("lit AF1", VariantNumericEq(m_Api->CellValue(1, 32), 3.0));
	CPPUNIT_ASSERT_MESSAGE("lit AE2", VariantNumericEq(m_Api->CellValue(2, 31), 1.0));
	CPPUNIT_ASSERT_MESSAGE("lit AF2", VariantNumericEq(m_Api->CellValue(2, 32), 9.0));
	CPPUNIT_ASSERT_MESSAGE("lit AE3", VariantNumericEq(m_Api->CellValue(3, 31), 2.0));
	CPPUNIT_ASSERT_MESSAGE("lit AF3", VariantNumericEq(m_Api->CellValue(3, 32), 5.0));
	CPPUNIT_ASSERT_MESSAGE("lit AE4", VariantNumericEq(m_Api->CellValue(4, 31), 2.0));
	CPPUNIT_ASSERT_MESSAGE("lit AF4", VariantNumericEq(m_Api->CellValue(4, 32), 8.0));
}

void TestSkMatrix::TestUnique() {
	// Source J1..J5 = 1,2,2,3,1 (col 10): 1 and 2 repeat, 3 appears once.
	m_Api->UndoCellValue("J1", 1);
	m_Api->UndoCellValue("J2", 2);
	m_Api->UndoCellValue("J3", 2);
	m_Api->UndoCellValue("J4", 3);
	m_Api->UndoCellValue("J5", 1);

	// Distinct values, first-occurrence order: =UNIQUE(J1:J5) at L1 -> 1,2,3 (col 12).
	CPPUNIT_ASSERT_MESSAGE("compile UNIQUE", m_Api->UndoCellValue("L1", "=UNIQUE(J1:J5)"));
	DrawCell("UNIQUE(J1:J5) L1:L3:", 1, 12, 3, 12);
	CPPUNIT_ASSERT_MESSAGE("UNIQUE L1", VariantNumericEq(m_Api->CellValue(1, 12), 1.0));
	CPPUNIT_ASSERT_MESSAGE("UNIQUE L2", VariantNumericEq(m_Api->CellValue(2, 12), 2.0));
	CPPUNIT_ASSERT_MESSAGE("UNIQUE L3", VariantNumericEq(m_Api->CellValue(3, 12), 3.0));

	// exactly_once: =UNIQUE(J1:J5;0;1) at N1 -> only 3 (1 and 2 repeat) (col 14).
	CPPUNIT_ASSERT_MESSAGE("compile UNIQUE once", m_Api->UndoCellValue("N1", "=UNIQUE(J1:J5;0;1)"));
	CPPUNIT_ASSERT_MESSAGE("UNIQUE once N1", VariantNumericEq(m_Api->CellValue(1, 14), 3.0));

	// Literal in-memory array: =UNIQUE({1;2;2;3}) at H1 -> 1,2,3 (col 8).
	CPPUNIT_ASSERT_MESSAGE("compile UNIQUE literal", m_Api->UndoCellValue("H1", "=UNIQUE({1;2;2;3})"));
	DrawCell("UNIQUE({1;2;2;3}) H1:H3:", 1, 8, 3, 8);
	CPPUNIT_ASSERT_MESSAGE("UNIQUE literal H1", VariantNumericEq(m_Api->CellValue(1, 8), 1.0));
	CPPUNIT_ASSERT_MESSAGE("UNIQUE literal H2", VariantNumericEq(m_Api->CellValue(2, 8), 2.0));
	CPPUNIT_ASSERT_MESSAGE("UNIQUE literal H3", VariantNumericEq(m_Api->CellValue(3, 8), 3.0));

	// exactly_once with every value duplicated -> empty result -> #CALC!.
	m_Api->UndoCellValue("P1", 5);
	m_Api->UndoCellValue("P2", 5);
	m_Api->UndoCellValue("P3", 6);
	m_Api->UndoCellValue("P4", 6);
	m_Api->UndoCellValue("R1", "=UNIQUE(P1:P4;0;1)");
	const tVariant wCalc = m_Api->CellValue(1, 18); // R1 (col 18)
	CPPUNIT_ASSERT_MESSAGE("UNIQUE empty is error", wCalc.IsError());
	CPPUNIT_ASSERT_MESSAGE("UNIQUE empty is #CALC!", wCalc.Error().Code() == tTypeError::t_calc);
}

void TestSkMatrix::TestRangeCompareMask() {
	// Source J1..J3 = 1,2,3 (col 10). =J1:J3>1 spills a boolean array FALSE,TRUE,TRUE into L1:L3 (col 12).
	m_Api->UndoCellValue("J1", 1);
	m_Api->UndoCellValue("J2", 2);
	m_Api->UndoCellValue("J3", 3);
	CPPUNIT_ASSERT_MESSAGE("compile J1:J3>1", m_Api->UndoCellValue("L1", "=J1:J3>1"));
	DrawCell("J1:J3>1 L1:L3:", 1, 12, 3, 12);
	const tVariant wL1 = m_Api->CellValue(1, 12);
	const tVariant wL2 = m_Api->CellValue(2, 12);
	const tVariant wL3 = m_Api->CellValue(3, 12);
	CPPUNIT_ASSERT_MESSAGE("L1 == FALSE", wL1.Bool() == false);
	CPPUNIT_ASSERT_MESSAGE("L2 == TRUE", wL2.Bool() == true);
	CPPUNIT_ASSERT_MESSAGE("L3 == TRUE", wL3.Bool() == true);
}

void TestSkMatrix::TestTranspose() {
	// A1:B2 = 1,2 / 3,4 -> TRANSPOSE -> D1:E2 = 1,3 / 2,4
	m_Api->UndoCellValue("A1", 1);
	m_Api->UndoCellValue("B1", 2);
	m_Api->UndoCellValue("A2", 3);
	m_Api->UndoCellValue("B2", 4);
	CPPUNIT_ASSERT_MESSAGE("compile TRANSPOSE", m_Api->UndoCellValue("D1", "=TRANSPOSE(A1:B2)"));
	DrawCell("TRANSPOSE D1:E2:", 1, 4, 2, 5);
	CPPUNIT_ASSERT_MESSAGE("TRANSPOSE D1", VariantNumericEq(m_Api->CellValue(1, 4), 1.0));
	CPPUNIT_ASSERT_MESSAGE("TRANSPOSE E1", VariantNumericEq(m_Api->CellValue(1, 5), 3.0));
	CPPUNIT_ASSERT_MESSAGE("TRANSPOSE D2", VariantNumericEq(m_Api->CellValue(2, 4), 2.0));
	CPPUNIT_ASSERT_MESSAGE("TRANSPOSE E2", VariantNumericEq(m_Api->CellValue(2, 5), 4.0));
}

void TestSkMatrix::TestMUnit() {
	// MUNIT(3) at A1 → identity 3×3 over A1:C3
	CPPUNIT_ASSERT_MESSAGE("compile MUNIT(3)", m_Api->UndoCellValue("A1", "=MUNIT(3)"));
	DrawCell("MUNIT(3) A1:C3:", 1, 1, 3, 3);
	CPPUNIT_ASSERT_MESSAGE("MUNIT A1", VariantNumericEq(m_Api->CellValue(1, 1), 1.0));
	CPPUNIT_ASSERT_MESSAGE("MUNIT B1", VariantNumericEq(m_Api->CellValue(1, 2), 0.0));
	CPPUNIT_ASSERT_MESSAGE("MUNIT C1", VariantNumericEq(m_Api->CellValue(1, 3), 0.0));
	CPPUNIT_ASSERT_MESSAGE("MUNIT A2", VariantNumericEq(m_Api->CellValue(2, 1), 0.0));
	CPPUNIT_ASSERT_MESSAGE("MUNIT B2", VariantNumericEq(m_Api->CellValue(2, 2), 1.0));
	CPPUNIT_ASSERT_MESSAGE("MUNIT C2", VariantNumericEq(m_Api->CellValue(2, 3), 0.0));
	CPPUNIT_ASSERT_MESSAGE("MUNIT A3", VariantNumericEq(m_Api->CellValue(3, 1), 0.0));
	CPPUNIT_ASSERT_MESSAGE("MUNIT B3", VariantNumericEq(m_Api->CellValue(3, 2), 0.0));
	CPPUNIT_ASSERT_MESSAGE("MUNIT C3", VariantNumericEq(m_Api->CellValue(3, 3), 1.0));
	CPPUNIT_ASSERT_MESSAGE("compile MUNIT(0)", m_Api->UndoCellValue("E1", "=MUNIT(0)"));
	CPPUNIT_ASSERT_MESSAGE("MUNIT(0) #VALUE!", m_Api->CellValue(1, 5).IsError());
	CPPUNIT_ASSERT_EQUAL(tTypeError::t_value, m_Api->CellValue(1, 5).Error().Code());
}

void TestSkMatrix::TestTrimRange() {
	// Note: this locale uses ';' as the argument separator.
	// Build a 4×3 block with blank border around a 2×2 core of values.
	// Row1 blank, Row2: blank,10,20 ; Row3: blank,30,40 ; Row4 blank
	// Col A blank throughout the used block.
	m_Api->UndoCellValue("B2", 10);
	m_Api->UndoCellValue("C2", 20);
	m_Api->UndoCellValue("B3", 30);
	m_Api->UndoCellValue("C3", 40);
	CPPUNIT_ASSERT_MESSAGE("compile TRIMRANGE",
		m_Api->UndoCellValue("E1", "=TRIMRANGE(A1:C4)"));
	DrawCell("TRIMRANGE E1:F2:", 1, 5, 2, 6);
	CPPUNIT_ASSERT_MESSAGE("TRIMRANGE E1", VariantNumericEq(m_Api->CellValue(1, 5), 10.0));
	CPPUNIT_ASSERT_MESSAGE("TRIMRANGE F1", VariantNumericEq(m_Api->CellValue(1, 6), 20.0));
	CPPUNIT_ASSERT_MESSAGE("TRIMRANGE E2", VariantNumericEq(m_Api->CellValue(2, 5), 30.0));
	CPPUNIT_ASSERT_MESSAGE("TRIMRANGE F2", VariantNumericEq(m_Api->CellValue(2, 6), 40.0));

	// Mode 0 keeps the full footprint (no trim).
	CPPUNIT_ASSERT_MESSAGE("compile TRIMRANGE mode0",
		m_Api->UndoCellValue("H1", "=TRIMRANGE(A1:C4;0;0)"));
	CPPUNIT_ASSERT_MESSAGE("TRIMRANGE mode0 H1 blank/null or empty",
		m_Api->CellValue(1, 8).IsNull() ||
		(m_Api->CellValue(1, 8).IsString() && m_Api->CellValue(1, 8).String().empty()) ||
		!m_Api->CellValue(1, 8).IsNumeric());
	CPPUNIT_ASSERT_MESSAGE("TRIMRANGE mode0 I2", VariantNumericEq(m_Api->CellValue(2, 9), 10.0));

	// All-blank range → #CALC!
	CPPUNIT_ASSERT_MESSAGE("compile TRIMRANGE empty",
		m_Api->UndoCellValue("K1", "=TRIMRANGE(Z1:Z3)"));
	CPPUNIT_ASSERT_MESSAGE("TRIMRANGE empty #CALC!", m_Api->CellValue(1, 11).IsError());
	CPPUNIT_ASSERT_EQUAL(tTypeError::t_calc, m_Api->CellValue(1, 11).Error().Code());
}

void TestSkMatrix::TestMMult() {
	// Note: this locale uses ';' as the argument separator.
	// A = [1 2 3; 4 5 6] (2x3), B = [7 8; 9 10; 11 12] (3x2)
	// C = A*B = [58 64; 139 154]
	m_Api->UndoCellValue("A1", 1);
	m_Api->UndoCellValue("B1", 2);
	m_Api->UndoCellValue("C1", 3);
	m_Api->UndoCellValue("A2", 4);
	m_Api->UndoCellValue("B2", 5);
	m_Api->UndoCellValue("C2", 6);
	m_Api->UndoCellValue("E1", 7);
	m_Api->UndoCellValue("F1", 8);
	m_Api->UndoCellValue("E2", 9);
	m_Api->UndoCellValue("F2", 10);
	m_Api->UndoCellValue("E3", 11);
	m_Api->UndoCellValue("F3", 12);
	CPPUNIT_ASSERT_MESSAGE("compile MMULT", m_Api->UndoCellValue("H1", "=MMULT(A1:C2;E1:F3)"));
	CPPUNIT_ASSERT_MESSAGE("MMULT H1", VariantNumericEq(m_Api->CellValue(1, 8), 58.0));
	CPPUNIT_ASSERT_MESSAGE("MMULT I1", VariantNumericEq(m_Api->CellValue(1, 9), 64.0));
	CPPUNIT_ASSERT_MESSAGE("MMULT H2", VariantNumericEq(m_Api->CellValue(2, 8), 139.0));
	CPPUNIT_ASSERT_MESSAGE("MMULT I2", VariantNumericEq(m_Api->CellValue(2, 9), 154.0));
}

void TestSkMatrix::TestMDeterm() {
	// [[1,2],[3,4]] → det = -2
	m_Api->UndoCellValue("A1", 1);
	m_Api->UndoCellValue("B1", 2);
	m_Api->UndoCellValue("A2", 3);
	m_Api->UndoCellValue("B2", 4);
	CPPUNIT_ASSERT_MESSAGE("compile MDETERM", m_Api->UndoCellValue("D1", "=MDETERM(A1:B2)"));
	CPPUNIT_ASSERT_MESSAGE("MDETERM 2x2", VariantNumericEq(m_Api->CellValue(1, 4), -2.0));
	// Identity 3×3 → 1
	CPPUNIT_ASSERT_MESSAGE("compile MDETERM(MUNIT)", m_Api->UndoCellValue("F1", "=MDETERM(MUNIT(3))"));
	CPPUNIT_ASSERT_MESSAGE("MDETERM identity", VariantNumericEq(m_Api->CellValue(1, 6), 1.0));
	// Non-square → #VALUE!
	m_Api->UndoCellValue("A3", 5);
	CPPUNIT_ASSERT_MESSAGE("compile MDETERM nonsquare", m_Api->UndoCellValue("H1", "=MDETERM(A1:B3)"));
	CPPUNIT_ASSERT_MESSAGE("MDETERM nonsquare #VALUE!", m_Api->CellValue(1, 8).IsError());
	CPPUNIT_ASSERT_EQUAL(tTypeError::t_value, m_Api->CellValue(1, 8).Error().Code());
}

void TestSkMatrix::TestMInverse() {
	// Note: this locale uses ';' as the argument separator.
	// [[4,7],[2,6]]⁻¹ = [[0.6,-0.7],[-0.2,0.4]]
	m_Api->UndoCellValue("A1", 4);
	m_Api->UndoCellValue("B1", 7);
	m_Api->UndoCellValue("A2", 2);
	m_Api->UndoCellValue("B2", 6);
	CPPUNIT_ASSERT_MESSAGE("compile MINVERSE", m_Api->UndoCellValue("D1", "=MINVERSE(A1:B2)"));
	DrawCell("MINVERSE D1:E2:", 1, 4, 2, 5);
	CPPUNIT_ASSERT_MESSAGE("MINVERSE D1", VariantNumericEq(m_Api->CellValue(1, 4), 0.6));
	CPPUNIT_ASSERT_MESSAGE("MINVERSE E1", VariantNumericEq(m_Api->CellValue(1, 5), -0.7));
	CPPUNIT_ASSERT_MESSAGE("MINVERSE D2", VariantNumericEq(m_Api->CellValue(2, 4), -0.2));
	CPPUNIT_ASSERT_MESSAGE("MINVERSE E2", VariantNumericEq(m_Api->CellValue(2, 5), 0.4));
	// MMULT(MINVERSE(A), A) ≈ I
	CPPUNIT_ASSERT_MESSAGE("compile MMULT(MINVERSE)",
		m_Api->UndoCellValue("G1", "=MMULT(MINVERSE(A1:B2);A1:B2)"));
	CPPUNIT_ASSERT_MESSAGE("I11", VariantNumericEq(m_Api->CellValue(1, 7), 1.0));
	CPPUNIT_ASSERT_MESSAGE("I12", VariantNumericEq(m_Api->CellValue(1, 8), 0.0));
	CPPUNIT_ASSERT_MESSAGE("I21", VariantNumericEq(m_Api->CellValue(2, 7), 0.0));
	CPPUNIT_ASSERT_MESSAGE("I22", VariantNumericEq(m_Api->CellValue(2, 8), 1.0));
	// Singular → #NUM!
	m_Api->UndoCellValue("A1", 1);
	m_Api->UndoCellValue("B1", 2);
	m_Api->UndoCellValue("A2", 2);
	m_Api->UndoCellValue("B2", 4);
	CPPUNIT_ASSERT_MESSAGE("compile MINVERSE singular", m_Api->UndoCellValue("J1", "=MINVERSE(A1:B2)"));
	CPPUNIT_ASSERT_MESSAGE("MINVERSE singular #NUM!", m_Api->CellValue(1, 10).IsError());
	CPPUNIT_ASSERT_EQUAL(tTypeError::t_num, m_Api->CellValue(1, 10).Error().Code());
}

void TestSkMatrix::TestToColToRow() {
	// Note: this locale uses ';' as the argument separator.
	// A1:B2 = 1,2 / 3,4
	m_Api->UndoCellValue("A1", 1);
	m_Api->UndoCellValue("B1", 2);
	m_Api->UndoCellValue("A2", 3);
	m_Api->UndoCellValue("B2", 4);

	// TOCOL row-major -> 1,2,3,4 as a column
	CPPUNIT_ASSERT_MESSAGE("compile TOCOL", m_Api->UndoCellValue("D1", "=TOCOL(A1:B2)"));
	CPPUNIT_ASSERT_MESSAGE("TOCOL D1", VariantNumericEq(m_Api->CellValue(1, 4), 1.0));
	CPPUNIT_ASSERT_MESSAGE("TOCOL D2", VariantNumericEq(m_Api->CellValue(2, 4), 2.0));
	CPPUNIT_ASSERT_MESSAGE("TOCOL D3", VariantNumericEq(m_Api->CellValue(3, 4), 3.0));
	CPPUNIT_ASSERT_MESSAGE("TOCOL D4", VariantNumericEq(m_Api->CellValue(4, 4), 4.0));

	// TOCOL scan_by_col -> 1,3,2,4
	CPPUNIT_ASSERT_MESSAGE("compile TOCOL by col", m_Api->UndoCellValue("F1", "=TOCOL(A1:B2;0;TRUE)"));
	CPPUNIT_ASSERT_MESSAGE("TOCOL F1", VariantNumericEq(m_Api->CellValue(1, 6), 1.0));
	CPPUNIT_ASSERT_MESSAGE("TOCOL F2", VariantNumericEq(m_Api->CellValue(2, 6), 3.0));
	CPPUNIT_ASSERT_MESSAGE("TOCOL F3", VariantNumericEq(m_Api->CellValue(3, 6), 2.0));
	CPPUNIT_ASSERT_MESSAGE("TOCOL F4", VariantNumericEq(m_Api->CellValue(4, 6), 4.0));

	// TOROW row-major -> single row
	CPPUNIT_ASSERT_MESSAGE("compile TOROW", m_Api->UndoCellValue("H1", "=TOROW(A1:B2)"));
	CPPUNIT_ASSERT_MESSAGE("TOROW H1", VariantNumericEq(m_Api->CellValue(1, 8), 1.0));
	CPPUNIT_ASSERT_MESSAGE("TOROW I1", VariantNumericEq(m_Api->CellValue(1, 9), 2.0));
	CPPUNIT_ASSERT_MESSAGE("TOROW J1", VariantNumericEq(m_Api->CellValue(1, 10), 3.0));
	CPPUNIT_ASSERT_MESSAGE("TOROW K1", VariantNumericEq(m_Api->CellValue(1, 11), 4.0));

	// ignore blanks: A4 blank, B4=9 -> TOCOL(A4:B4;1) -> just 9
	m_Api->UndoCellValue("B4", 9);
	CPPUNIT_ASSERT_MESSAGE("compile TOCOL ignore blank", m_Api->UndoCellValue("M1", "=TOCOL(A4:B4;1)"));
	CPPUNIT_ASSERT_MESSAGE("TOCOL ignore M1", VariantNumericEq(m_Api->CellValue(1, 13), 9.0));
}

void TestSkMatrix::TestChooseColsRows() {
	// Note: this locale uses ';' as the argument separator.
	m_Api->UndoCellValue("A1", 1);
	m_Api->UndoCellValue("B1", 2);
	m_Api->UndoCellValue("C1", 3);
	m_Api->UndoCellValue("A2", 4);
	m_Api->UndoCellValue("B2", 5);
	m_Api->UndoCellValue("C2", 6);

	// CHOOSECOLS pick col 2 then col 1
	CPPUNIT_ASSERT_MESSAGE("compile CHOOSECOLS", m_Api->UndoCellValue("E1", "=CHOOSECOLS(A1:C2;2;1)"));
	CPPUNIT_ASSERT_MESSAGE("CHOOSECOLS E1", VariantNumericEq(m_Api->CellValue(1, 5), 2.0));
	CPPUNIT_ASSERT_MESSAGE("CHOOSECOLS F1", VariantNumericEq(m_Api->CellValue(1, 6), 1.0));
	CPPUNIT_ASSERT_MESSAGE("CHOOSECOLS E2", VariantNumericEq(m_Api->CellValue(2, 5), 5.0));
	CPPUNIT_ASSERT_MESSAGE("CHOOSECOLS F2", VariantNumericEq(m_Api->CellValue(2, 6), 4.0));

	// Negative index: -1 = last column
	CPPUNIT_ASSERT_MESSAGE("compile CHOOSECOLS neg", m_Api->UndoCellValue("H1", "=CHOOSECOLS(A1:C2;-1)"));
	CPPUNIT_ASSERT_MESSAGE("CHOOSECOLS H1", VariantNumericEq(m_Api->CellValue(1, 8), 3.0));
	CPPUNIT_ASSERT_MESSAGE("CHOOSECOLS H2", VariantNumericEq(m_Api->CellValue(2, 8), 6.0));

	// CHOOSEROWS pick row 2
	CPPUNIT_ASSERT_MESSAGE("compile CHOOSEROWS", m_Api->UndoCellValue("J1", "=CHOOSEROWS(A1:C2;2)"));
	CPPUNIT_ASSERT_MESSAGE("CHOOSEROWS J1", VariantNumericEq(m_Api->CellValue(1, 10), 4.0));
	CPPUNIT_ASSERT_MESSAGE("CHOOSEROWS K1", VariantNumericEq(m_Api->CellValue(1, 11), 5.0));
	CPPUNIT_ASSERT_MESSAGE("CHOOSEROWS L1", VariantNumericEq(m_Api->CellValue(1, 12), 6.0));
}

void TestSkMatrix::TestExpandWrap() {
	// Note: this locale uses ';' as the argument separator.
	m_Api->UndoCellValue("A1", 1);
	m_Api->UndoCellValue("B1", 2);
	m_Api->UndoCellValue("A2", 3);
	m_Api->UndoCellValue("B2", 4);

	// EXPAND to 3x3, pad with 0
	CPPUNIT_ASSERT_MESSAGE("compile EXPAND", m_Api->UndoCellValue("D1", "=EXPAND(A1:B2;3;3;0)"));
	CPPUNIT_ASSERT_MESSAGE("EXPAND D1", VariantNumericEq(m_Api->CellValue(1, 4), 1.0));
	CPPUNIT_ASSERT_MESSAGE("EXPAND E1", VariantNumericEq(m_Api->CellValue(1, 5), 2.0));
	CPPUNIT_ASSERT_MESSAGE("EXPAND F1", VariantNumericEq(m_Api->CellValue(1, 6), 0.0));
	CPPUNIT_ASSERT_MESSAGE("EXPAND D2", VariantNumericEq(m_Api->CellValue(2, 4), 3.0));
	CPPUNIT_ASSERT_MESSAGE("EXPAND E2", VariantNumericEq(m_Api->CellValue(2, 5), 4.0));
	CPPUNIT_ASSERT_MESSAGE("EXPAND F2", VariantNumericEq(m_Api->CellValue(2, 6), 0.0));
	CPPUNIT_ASSERT_MESSAGE("EXPAND D3", VariantNumericEq(m_Api->CellValue(3, 4), 0.0));
	CPPUNIT_ASSERT_MESSAGE("EXPAND E3", VariantNumericEq(m_Api->CellValue(3, 5), 0.0));
	CPPUNIT_ASSERT_MESSAGE("EXPAND F3", VariantNumericEq(m_Api->CellValue(3, 6), 0.0));

	// Shrink -> #VALUE!
	CPPUNIT_ASSERT_MESSAGE("compile EXPAND shrink", m_Api->UndoCellValue("H1", "=EXPAND(A1:B2;1;2)"));
	CPPUNIT_ASSERT_MESSAGE("EXPAND shrink #VALUE!", m_Api->CellValue(1, 8).IsError());
	CPPUNIT_ASSERT_EQUAL(m_Api->CellValue(1, 8).Error().Code(), tTypeError::t_value);

	// WRAPROWS: 1..7 row, wrap 3, pad "x"
	m_Api->UndoCellValue("A10", 1);
	m_Api->UndoCellValue("B10", 2);
	m_Api->UndoCellValue("C10", 3);
	m_Api->UndoCellValue("D10", 4);
	m_Api->UndoCellValue("E10", 5);
	m_Api->UndoCellValue("F10", 6);
	m_Api->UndoCellValue("G10", 7);
	CPPUNIT_ASSERT_MESSAGE("compile WRAPROWS", m_Api->UndoCellValue("A12", "=WRAPROWS(A10:G10;3;\"x\")"));
	CPPUNIT_ASSERT_MESSAGE("WRAPROWS A12", VariantNumericEq(m_Api->CellValue(12, 1), 1.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPROWS B12", VariantNumericEq(m_Api->CellValue(12, 2), 2.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPROWS C12", VariantNumericEq(m_Api->CellValue(12, 3), 3.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPROWS A13", VariantNumericEq(m_Api->CellValue(13, 1), 4.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPROWS B13", VariantNumericEq(m_Api->CellValue(13, 2), 5.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPROWS C13", VariantNumericEq(m_Api->CellValue(13, 3), 6.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPROWS A14", VariantNumericEq(m_Api->CellValue(14, 1), 7.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPROWS B14 pad", VariantStringEq(m_Api->CellValue(14, 2), "x"));
	CPPUNIT_ASSERT_MESSAGE("WRAPROWS C14 pad", VariantStringEq(m_Api->CellValue(14, 3), "x"));

	// WRAPCOLS: same vector, wrap 3 -> fill by columns
	CPPUNIT_ASSERT_MESSAGE("compile WRAPCOLS", m_Api->UndoCellValue("E12", "=WRAPCOLS(A10:G10;3;0)"));
	CPPUNIT_ASSERT_MESSAGE("WRAPCOLS E12", VariantNumericEq(m_Api->CellValue(12, 5), 1.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPCOLS E13", VariantNumericEq(m_Api->CellValue(13, 5), 2.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPCOLS E14", VariantNumericEq(m_Api->CellValue(14, 5), 3.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPCOLS F12", VariantNumericEq(m_Api->CellValue(12, 6), 4.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPCOLS F13", VariantNumericEq(m_Api->CellValue(13, 6), 5.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPCOLS F14", VariantNumericEq(m_Api->CellValue(14, 6), 6.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPCOLS G12", VariantNumericEq(m_Api->CellValue(12, 7), 7.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPCOLS G13 pad", VariantNumericEq(m_Api->CellValue(13, 7), 0.0));
	CPPUNIT_ASSERT_MESSAGE("WRAPCOLS G14 pad", VariantNumericEq(m_Api->CellValue(14, 7), 0.0));

	// Default pad #N/A on WRAPROWS leftover
	CPPUNIT_ASSERT_MESSAGE("compile WRAPROWS default pad", m_Api->UndoCellValue("A16", "=WRAPROWS(A10:G10;3)"));
	CPPUNIT_ASSERT_MESSAGE("WRAPROWS default pad #N/A", m_Api->CellValue(18, 2).IsError());
	CPPUNIT_ASSERT_EQUAL(m_Api->CellValue(18, 2).Error().Code(), tTypeError::t_na);
}

void TestSkMatrix::TestTake() {
	// Note: this locale uses ';' as the argument separator.
	m_Api->UndoCellValue("A1", 10);
	m_Api->UndoCellValue("A2", 20);
	m_Api->UndoCellValue("A3", 30);
	m_Api->UndoCellValue("A4", 40);
	CPPUNIT_ASSERT_MESSAGE("compile TAKE rows", m_Api->UndoCellValue("C1", "=TAKE(A1:A4;2)"));
	CPPUNIT_ASSERT_MESSAGE("TAKE C1", VariantNumericEq(m_Api->CellValue(1, 3), 10.0));
	CPPUNIT_ASSERT_MESSAGE("TAKE C2", VariantNumericEq(m_Api->CellValue(2, 3), 20.0));

	CPPUNIT_ASSERT_MESSAGE("compile TAKE negative", m_Api->UndoCellValue("E1", "=TAKE(A1:A4;-2)"));
	CPPUNIT_ASSERT_MESSAGE("TAKE E1 last", VariantNumericEq(m_Api->CellValue(1, 5), 30.0));
	CPPUNIT_ASSERT_MESSAGE("TAKE E2 last", VariantNumericEq(m_Api->CellValue(2, 5), 40.0));

	m_Api->UndoCellValue("G1", 1);
	m_Api->UndoCellValue("H1", 2);
	m_Api->UndoCellValue("G2", 3);
	m_Api->UndoCellValue("H2", 4);
	CPPUNIT_ASSERT_MESSAGE("compile TAKE 2d", m_Api->UndoCellValue("J1", "=TAKE(G1:H2;1;1)"));
	CPPUNIT_ASSERT_MESSAGE("TAKE J1", VariantNumericEq(m_Api->CellValue(1, 10), 1.0));
}

void TestSkMatrix::TestDrop() {
	// Note: this locale uses ';' as the argument separator.
	m_Api->UndoCellValue("A1", 10);
	m_Api->UndoCellValue("A2", 20);
	m_Api->UndoCellValue("A3", 30);
	m_Api->UndoCellValue("A4", 40);
	CPPUNIT_ASSERT_MESSAGE("compile DROP rows", m_Api->UndoCellValue("C1", "=DROP(A1:A4;2)"));
	CPPUNIT_ASSERT_MESSAGE("DROP C1", VariantNumericEq(m_Api->CellValue(1, 3), 30.0));
	CPPUNIT_ASSERT_MESSAGE("DROP C2", VariantNumericEq(m_Api->CellValue(2, 3), 40.0));

	CPPUNIT_ASSERT_MESSAGE("compile DROP negative", m_Api->UndoCellValue("E1", "=DROP(A1:A4;-2)"));
	CPPUNIT_ASSERT_MESSAGE("DROP E1 keep head", VariantNumericEq(m_Api->CellValue(1, 5), 10.0));
	CPPUNIT_ASSERT_MESSAGE("DROP E2 keep head", VariantNumericEq(m_Api->CellValue(2, 5), 20.0));

	m_Api->UndoCellValue("G1", 1);
	m_Api->UndoCellValue("H1", 2);
	m_Api->UndoCellValue("G2", 3);
	m_Api->UndoCellValue("H2", 4);
	CPPUNIT_ASSERT_MESSAGE("compile DROP cols", m_Api->UndoCellValue("J1", "=DROP(G1:H2;0;1)"));
	CPPUNIT_ASSERT_MESSAGE("DROP J1", VariantNumericEq(m_Api->CellValue(1, 10), 2.0));
	CPPUNIT_ASSERT_MESSAGE("DROP J2", VariantNumericEq(m_Api->CellValue(2, 10), 4.0));
}

void TestSkMatrix::TestHStack() {
	m_Api->UndoCellValue("A1", 1);
	m_Api->UndoCellValue("A2", 2);
	m_Api->UndoCellValue("B1", 3);
	m_Api->UndoCellValue("B2", 4);
	CPPUNIT_ASSERT_MESSAGE("compile HSTACK", m_Api->UndoCellValue("D1", "=HSTACK(A1:A2;B1:B2)"));
	CPPUNIT_ASSERT_MESSAGE("HSTACK D1", VariantNumericEq(m_Api->CellValue(1, 4), 1.0));
	CPPUNIT_ASSERT_MESSAGE("HSTACK E1", VariantNumericEq(m_Api->CellValue(1, 5), 3.0));
	CPPUNIT_ASSERT_MESSAGE("HSTACK D2", VariantNumericEq(m_Api->CellValue(2, 4), 2.0));
	CPPUNIT_ASSERT_MESSAGE("HSTACK E2", VariantNumericEq(m_Api->CellValue(2, 5), 4.0));
}

void TestSkMatrix::TestVStack() {
	m_Api->UndoCellValue("A1", 1);
	m_Api->UndoCellValue("B1", 2);
	m_Api->UndoCellValue("A2", 3);
	m_Api->UndoCellValue("B2", 4);
	CPPUNIT_ASSERT_MESSAGE("compile VSTACK", m_Api->UndoCellValue("D1", "=VSTACK(A1:B1;A2:B2)"));
	CPPUNIT_ASSERT_MESSAGE("VSTACK D1", VariantNumericEq(m_Api->CellValue(1, 4), 1.0));
	CPPUNIT_ASSERT_MESSAGE("VSTACK E1", VariantNumericEq(m_Api->CellValue(1, 5), 2.0));
	CPPUNIT_ASSERT_MESSAGE("VSTACK D2", VariantNumericEq(m_Api->CellValue(2, 4), 3.0));
	CPPUNIT_ASSERT_MESSAGE("VSTACK E2", VariantNumericEq(m_Api->CellValue(2, 5), 4.0));
}

void TestSkMatrix::TestSortBy() {
	// Sort names by scores descending: scores in B, names in A.
	m_Api->UndoCellValue("A1", "ann");
	m_Api->UndoCellValue("A2", "bob");
	m_Api->UndoCellValue("A3", "cara");
	m_Api->UndoCellValue("B1", 2);
	m_Api->UndoCellValue("B2", 3);
	m_Api->UndoCellValue("B3", 1);
	CPPUNIT_ASSERT_MESSAGE("compile SORTBY", m_Api->UndoCellValue("D1", "=SORTBY(A1:A3;B1:B3;-1)"));
	tVariant wD1 = m_Api->CellValue(1, 4);
	tVariant wD2 = m_Api->CellValue(2, 4);
	tVariant wD3 = m_Api->CellValue(3, 4);
	CPPUNIT_ASSERT_EQUAL(wD1.String(), tString("bob"));
	CPPUNIT_ASSERT_EQUAL(wD2.String(), tString("ann"));
	CPPUNIT_ASSERT_EQUAL(wD3.String(), tString("cara"));
}

void TestSkMatrix::TestFilter() {
	// Data J1..J3 = 10,20,30 (col 10). Keep values > 15 -> 20,30 into L1:L2 (col 12).
	// Note: this locale uses ';' as the argument separator (',' is the decimal separator).
	m_Api->UndoCellValue("J1", 10);
	m_Api->UndoCellValue("J2", 20);
	m_Api->UndoCellValue("J3", 30);
	CPPUNIT_ASSERT_MESSAGE("compile FILTER rows", m_Api->UndoCellValue("L1", "=FILTER(J1:J3;J1:J3>15)"));
	DrawCell("FILTER rows L1:L2:", 1, 12, 2, 12);
	CPPUNIT_ASSERT_MESSAGE("FILTER L1", VariantNumericEq(m_Api->CellValue(1, 12), 20.0));
	CPPUNIT_ASSERT_MESSAGE("FILTER L2", VariantNumericEq(m_Api->CellValue(2, 12), 30.0));

	// if_empty: nothing > 100 -> "none" at N1 (col 14).
	m_Api->UndoCellValue("N1", "=FILTER(J1:J3;J1:J3>100;\"none\")");
	tVariant wNone = m_Api->CellValue(1, 14);
	tString wExpected = "none";
	CPPUNIT_ASSERT_EQUAL(wNone.String(), wExpected);

	// Empty without if_empty -> #CALC! at N3 (col 14).
	m_Api->UndoCellValue("N3", "=FILTER(J1:J3;J1:J3>100)");
	tVariant wCalc = m_Api->CellValue(3, 14);
	CPPUNIT_ASSERT_MESSAGE("FILTER empty is error", wCalc.IsError());
	CPPUNIT_ASSERT_MESSAGE("FILTER empty is #CALC!", wCalc.Error().Code() == tTypeError::t_calc);

	// Multi-column row filter: data P1:Q3, mask on column P (P1:P3>1) keeps rows 2,3 -> (2,"b"),(3,"c").
	m_Api->UndoCellValue("P1", 1);
	m_Api->UndoCellValue("Q1", "a");
	m_Api->UndoCellValue("P2", 2);
	m_Api->UndoCellValue("Q2", "b");
	m_Api->UndoCellValue("P3", 3);
	m_Api->UndoCellValue("Q3", "c");
	CPPUNIT_ASSERT_MESSAGE("compile FILTER multicol", m_Api->UndoCellValue("T1", "=FILTER(P1:Q3;P1:P3>1)"));
	DrawCell("FILTER multicol T1:U2:", 1, 20, 2, 21);
	CPPUNIT_ASSERT_MESSAGE("FILTER T1", VariantNumericEq(m_Api->CellValue(1, 20), 2.0));
	tVariant wU1 = m_Api->CellValue(1, 21);
	tString wB = "b";
	CPPUNIT_ASSERT_EQUAL(wU1.String(), wB);
	CPPUNIT_ASSERT_MESSAGE("FILTER T2", VariantNumericEq(m_Api->CellValue(2, 20), 3.0));
	tVariant wU2 = m_Api->CellValue(2, 21);
	tString wC = "c";
	CPPUNIT_ASSERT_EQUAL(wU2.String(), wC);

	// Multiple conditions combined with element-wise array arithmetic.
	// AC1:AC4 = 10,20,30,40 (values) ; AD1:AD4 = 5,15,25,35 ; AE1:AE4 = 100,200,300,400.
	m_Api->UndoCellValue("AC1", 10); m_Api->UndoCellValue("AD1", 5);  m_Api->UndoCellValue("AE1", 100);
	m_Api->UndoCellValue("AC2", 20); m_Api->UndoCellValue("AD2", 15); m_Api->UndoCellValue("AE2", 200);
	m_Api->UndoCellValue("AC3", 30); m_Api->UndoCellValue("AD3", 25); m_Api->UndoCellValue("AE3", 300);
	m_Api->UndoCellValue("AC4", 40); m_Api->UndoCellValue("AD4", 35); m_Api->UndoCellValue("AE4", 400);

	// AND via "*": (AD>10) -> F,T,T,T and (AE<400) -> T,T,T,F ; product -> F,T,T,F -> keep rows 2,3 => 20,30.
	CPPUNIT_ASSERT_MESSAGE("compile FILTER AND",
		m_Api->UndoCellValue("AG1", "=FILTER(AC1:AC4;(AD1:AD4>10)*(AE1:AE4<400))"));
	DrawCell("FILTER AND AG1:AG2:", 1, 33, 2, 33);
	CPPUNIT_ASSERT_MESSAGE("FILTER AND AG1", VariantNumericEq(m_Api->CellValue(1, 33), 20.0));
	CPPUNIT_ASSERT_MESSAGE("FILTER AND AG2", VariantNumericEq(m_Api->CellValue(2, 33), 30.0));

	// OR via "+": (AD>30) -> F,F,F,T and (AE<150) -> T,F,F,F ; sum -> T,F,F,T -> keep rows 1,4 => 10,40.
	CPPUNIT_ASSERT_MESSAGE("compile FILTER OR",
		m_Api->UndoCellValue("AI1", "=FILTER(AC1:AC4;(AD1:AD4>30)+(AE1:AE4<150))"));
	DrawCell("FILTER OR AI1:AI2:", 1, 35, 2, 35);
	CPPUNIT_ASSERT_MESSAGE("FILTER OR AI1", VariantNumericEq(m_Api->CellValue(1, 35), 10.0));
	CPPUNIT_ASSERT_MESSAGE("FILTER OR AI2", VariantNumericEq(m_Api->CellValue(2, 35), 40.0));
}

void TestSkMatrix::TestCountSumIfsArray() {
	// Teams P1:P6 (col 16), values Q1:Q6 (col 17), played flags R1:R6 (col 18).
	// Note: this locale uses ';' as the argument separator.
	m_Api->UndoCellValue("P1", "X"); m_Api->UndoCellValue("Q1", "=1"); m_Api->UndoCellValue("R1", "=1");
	m_Api->UndoCellValue("P2", "Y"); m_Api->UndoCellValue("Q2", "=2"); m_Api->UndoCellValue("R2", "=1");
	m_Api->UndoCellValue("P3", "X"); m_Api->UndoCellValue("Q3", "=3"); m_Api->UndoCellValue("R3", "=1");
	m_Api->UndoCellValue("P4", "Z"); m_Api->UndoCellValue("Q4", "=4"); m_Api->UndoCellValue("R4", "=1");
	m_Api->UndoCellValue("P5", "Y"); m_Api->UndoCellValue("Q5", "=5"); m_Api->UndoCellValue("R5", "=0");
	m_Api->UndoCellValue("P6", "X"); m_Api->UndoCellValue("Q6", "=6"); m_Api->UndoCellValue("R6", "=1");

	// COUNTIFS with a single array criterion (UNIQUE -> X,Y,Z) spills counts 3,2,1 into T1:T3 (col 20).
	CPPUNIT_ASSERT_MESSAGE("compile COUNTIFS array",
		m_Api->UndoCellValue("T1", "=COUNTIFS(P1:P6;UNIQUE(P1:P6))"));
	DrawCell("COUNTIFS array T1:T3:", 1, 20, 3, 20);
	CPPUNIT_ASSERT_MESSAGE("COUNTIFS T1", VariantNumericEq(m_Api->CellValue(1, 20), 3.0));
	CPPUNIT_ASSERT_MESSAGE("COUNTIFS T2", VariantNumericEq(m_Api->CellValue(2, 20), 2.0));
	CPPUNIT_ASSERT_MESSAGE("COUNTIFS T3", VariantNumericEq(m_Api->CellValue(3, 20), 1.0));

	// SUMIFS(values; teams; UNIQUE(teams)) spills 10,7,4 into V1:V3 (col 22).
	CPPUNIT_ASSERT_MESSAGE("compile SUMIFS array",
		m_Api->UndoCellValue("V1", "=SUMIFS(Q1:Q6;P1:P6;UNIQUE(P1:P6))"));
	DrawCell("SUMIFS array V1:V3:", 1, 22, 3, 22);
	CPPUNIT_ASSERT_MESSAGE("SUMIFS V1", VariantNumericEq(m_Api->CellValue(1, 22), 10.0));
	CPPUNIT_ASSERT_MESSAGE("SUMIFS V2", VariantNumericEq(m_Api->CellValue(2, 22), 7.0));
	CPPUNIT_ASSERT_MESSAGE("SUMIFS V3", VariantNumericEq(m_Api->CellValue(3, 22), 4.0));

	// MAXIFS / MINIFS with the same array criterion: X->6/1, Y->5/2, Z->4/4 (cols 28 / 30).
	CPPUNIT_ASSERT_MESSAGE("compile MAXIFS array",
		m_Api->UndoCellValue("AB1", "=MAXIFS(Q1:Q6;P1:P6;UNIQUE(P1:P6))"));
	DrawCell("MAXIFS array AB1:AB3:", 1, 28, 3, 28);
	CPPUNIT_ASSERT_MESSAGE("MAXIFS AB1", VariantNumericEq(m_Api->CellValue(1, 28), 6.0));
	CPPUNIT_ASSERT_MESSAGE("MAXIFS AB2", VariantNumericEq(m_Api->CellValue(2, 28), 5.0));
	CPPUNIT_ASSERT_MESSAGE("MAXIFS AB3", VariantNumericEq(m_Api->CellValue(3, 28), 4.0));

	CPPUNIT_ASSERT_MESSAGE("compile MINIFS array",
		m_Api->UndoCellValue("AD1", "=MINIFS(Q1:Q6;P1:P6;UNIQUE(P1:P6))"));
	DrawCell("MINIFS array AD1:AD3:", 1, 30, 3, 30);
	CPPUNIT_ASSERT_MESSAGE("MINIFS AD1", VariantNumericEq(m_Api->CellValue(1, 30), 1.0));
	CPPUNIT_ASSERT_MESSAGE("MINIFS AD2", VariantNumericEq(m_Api->CellValue(2, 30), 2.0));
	CPPUNIT_ASSERT_MESSAGE("MINIFS AD3", VariantNumericEq(m_Api->CellValue(3, 30), 4.0));

	// Array criterion broadcast against a scalar criterion (played=1): counts 3,1,1 into X1:X3 (col 24).
	CPPUNIT_ASSERT_MESSAGE("compile COUNTIFS array+scalar",
		m_Api->UndoCellValue("X1", "=COUNTIFS(P1:P6;UNIQUE(P1:P6);R1:R6;1)"));
	DrawCell("COUNTIFS array+scalar X1:X3:", 1, 24, 3, 24);
	CPPUNIT_ASSERT_MESSAGE("COUNTIFS+scalar X1", VariantNumericEq(m_Api->CellValue(1, 24), 3.0));
	CPPUNIT_ASSERT_MESSAGE("COUNTIFS+scalar X2", VariantNumericEq(m_Api->CellValue(2, 24), 1.0));
	CPPUNIT_ASSERT_MESSAGE("COUNTIFS+scalar X3", VariantNumericEq(m_Api->CellValue(3, 24), 1.0));

	// Single-criterion COUNTIF with an array criterion also spills: 3,2,1 into Z1:Z3 (col 26).
	CPPUNIT_ASSERT_MESSAGE("compile COUNTIF array",
		m_Api->UndoCellValue("Z1", "=COUNTIF(P1:P6;UNIQUE(P1:P6))"));
	DrawCell("COUNTIF array Z1:Z3:", 1, 26, 3, 26);
	CPPUNIT_ASSERT_MESSAGE("COUNTIF Z1", VariantNumericEq(m_Api->CellValue(1, 26), 3.0));
	CPPUNIT_ASSERT_MESSAGE("COUNTIF Z2", VariantNumericEq(m_Api->CellValue(2, 26), 2.0));
	CPPUNIT_ASSERT_MESSAGE("COUNTIF Z3", VariantNumericEq(m_Api->CellValue(3, 26), 1.0));

	// SUMIF(range; array criterion; sum_range) spills 10,7,4 into AB1:AB3 (col 28).
	CPPUNIT_ASSERT_MESSAGE("compile SUMIF array",
		m_Api->UndoCellValue("AB1", "=SUMIF(P1:P6;UNIQUE(P1:P6);Q1:Q6)"));
	DrawCell("SUMIF array AB1:AB3:", 1, 28, 3, 28);
	CPPUNIT_ASSERT_MESSAGE("SUMIF AB1", VariantNumericEq(m_Api->CellValue(1, 28), 10.0));
	CPPUNIT_ASSERT_MESSAGE("SUMIF AB2", VariantNumericEq(m_Api->CellValue(2, 28), 7.0));
	CPPUNIT_ASSERT_MESSAGE("SUMIF AB3", VariantNumericEq(m_Api->CellValue(3, 28), 4.0));

	// A plain scalar criterion must still return a single value (no accidental spill).
	CPPUNIT_ASSERT_MESSAGE("compile COUNTIF scalar",
		m_Api->UndoCellValue("AD1", "=COUNTIF(P1:P6;\"X\")"));
	CPPUNIT_ASSERT_MESSAGE("COUNTIF scalar AD1", VariantNumericEq(m_Api->CellValue(1, 30), 3.0));

	// League-Table Part B: COUNTIFS(..., D14#) — the # operator pushes a Range, not t_Array.
	CPPUNIT_ASSERT_MESSAGE("compile UNIQUE origin for #",
		m_Api->UndoCellValue("AF1", "=UNIQUE(P1:P6)"));
	CPPUNIT_ASSERT_MESSAGE("compile COUNTIFS spill-ref",
		m_Api->UndoCellValue("AH1", "=COUNTIFS(P1:P6;AF1#)"));
	DrawCell("COUNTIFS AF1# AH1:AH3:", 1, 34, 3, 34);
	CPPUNIT_ASSERT_MESSAGE("COUNTIFS # AH1", VariantNumericEq(m_Api->CellValue(1, 34), 3.0));
	CPPUNIT_ASSERT_MESSAGE("COUNTIFS # AH2", VariantNumericEq(m_Api->CellValue(2, 34), 2.0));
	CPPUNIT_ASSERT_MESSAGE("COUNTIFS # AH3", VariantNumericEq(m_Api->CellValue(3, 34), 1.0));

	CPPUNIT_ASSERT_MESSAGE("compile SUMIFS spill-ref",
		m_Api->UndoCellValue("AJ1", "=SUMIFS(Q1:Q6;P1:P6;AF1#)"));
	CPPUNIT_ASSERT_MESSAGE("SUMIFS # AJ1", VariantNumericEq(m_Api->CellValue(1, 36), 10.0));
	CPPUNIT_ASSERT_MESSAGE("SUMIFS # AJ2", VariantNumericEq(m_Api->CellValue(2, 36), 7.0));
	CPPUNIT_ASSERT_MESSAGE("SUMIFS # AJ3", VariantNumericEq(m_Api->CellValue(3, 36), 4.0));

	// Numeric unique-list (Excel loan template J/K): COUNTIF/SUMIF with year numbers as array criterion.
	m_Api->UndoCellValue("AL1", "=2024"); m_Api->UndoCellValue("AM1", "=10");
	m_Api->UndoCellValue("AL2", "=2024"); m_Api->UndoCellValue("AM2", "=20");
	m_Api->UndoCellValue("AL3", "=2025"); m_Api->UndoCellValue("AM3", "=30");
	m_Api->UndoCellValue("AL4", "=2025"); m_Api->UndoCellValue("AM4", "=40");
	m_Api->UndoCellValue("AL5", "=2025"); m_Api->UndoCellValue("AM5", "=50");
	m_Api->UndoCellValue("AL6", "=2026"); m_Api->UndoCellValue("AM6", "=60");
	CPPUNIT_ASSERT_MESSAGE("compile COUNTIF numeric unique-list",
		m_Api->UndoCellValue("AN1", "=COUNTIF(AL1:AL6;UNIQUE(AL1:AL6))"));
	CPPUNIT_ASSERT_MESSAGE("COUNTIF year 2024", VariantNumericEq(m_Api->CellValue(1, 40), 2.0));
	CPPUNIT_ASSERT_MESSAGE("COUNTIF year 2025", VariantNumericEq(m_Api->CellValue(2, 40), 3.0));
	CPPUNIT_ASSERT_MESSAGE("COUNTIF year 2026", VariantNumericEq(m_Api->CellValue(3, 40), 1.0));
	CPPUNIT_ASSERT_MESSAGE("compile SUMIF numeric unique-list",
		m_Api->UndoCellValue("AP1", "=SUMIF(AL1:AL6;UNIQUE(AL1:AL6);AM1:AM6)"));
	CPPUNIT_ASSERT_MESSAGE("SUMIF year 2024", VariantNumericEq(m_Api->CellValue(1, 42), 30.0));
	CPPUNIT_ASSERT_MESSAGE("SUMIF year 2025", VariantNumericEq(m_Api->CellValue(2, 42), 120.0));
	CPPUNIT_ASSERT_MESSAGE("SUMIF year 2026", VariantNumericEq(m_Api->CellValue(3, 42), 60.0));
}

void TestSkMatrix::TestLetLeagueTableLiterals() {
	// Reproduces the Excel "LET Example" D17 pattern end-to-end: a LET that derives standings with array
	// COUNTIFS (criterion = UNIQUE(...)), builds a table via SWITCH over a *literal* header row, then sorts
	// it with a multi-key SORT whose keys are *literals*. Three literals ({…}) live in one formula and must
	// coexist as in-memory arrays (previously each spilled onto the formula cell and corrupted the others).
	//
	// Matches (6): home=col H(8), away=col I(9), played=col J(10), result=col K(11).
	//   r1 A-B H | r2 B-C A | r3 A-C D | r4 C-A H | r5 B-A A | r6 C-B D
	m_Api->UndoCellValue("H1", "A"); m_Api->UndoCellValue("I1", "B"); m_Api->UndoCellValue("J1", "=1"); m_Api->UndoCellValue("K1", "H");
	m_Api->UndoCellValue("H2", "B"); m_Api->UndoCellValue("I2", "C"); m_Api->UndoCellValue("J2", "=1"); m_Api->UndoCellValue("K2", "A");
	m_Api->UndoCellValue("H3", "A"); m_Api->UndoCellValue("I3", "C"); m_Api->UndoCellValue("J3", "=1"); m_Api->UndoCellValue("K3", "D");
	m_Api->UndoCellValue("H4", "C"); m_Api->UndoCellValue("I4", "A"); m_Api->UndoCellValue("J4", "=1"); m_Api->UndoCellValue("K4", "H");
	m_Api->UndoCellValue("H5", "B"); m_Api->UndoCellValue("I5", "A"); m_Api->UndoCellValue("J5", "=1"); m_Api->UndoCellValue("K5", "A");
	m_Api->UndoCellValue("H6", "C"); m_Api->UndoCellValue("I6", "B"); m_Api->UndoCellValue("J6", "=1"); m_Api->UndoCellValue("K6", "D");

	// Expected standings (PTS = W*3 + D): C=8, A=7, B=1  -> sorted by PTS desc.
	//   TEAM P W D L PTS
	//   C    4 2 2 0 8
	//   A    4 2 1 1 7
	//   B    4 0 1 3 1
	const tString wFormula =
		"=LET("
		"TEAM;UNIQUE(H1:H6);"
		"P;COUNTIFS(H1:H6;TEAM;J1:J6;1)+COUNTIFS(I1:I6;TEAM;J1:J6;1);"
		"W;COUNTIFS(H1:H6;TEAM;K1:K6;\"H\")+COUNTIFS(I1:I6;TEAM;K1:K6;\"A\");"
		"D;COUNTIFS(H1:H6;TEAM;K1:K6;\"D\")+COUNTIFS(I1:I6;TEAM;K1:K6;\"D\");"
		"L;COUNTIFS(H1:H6;TEAM;K1:K6;\"A\")+COUNTIFS(I1:I6;TEAM;K1:K6;\"H\");"
		"PTS;(W*3)+(D*1);"
		"SORT(SWITCH({\"TEAM\",\"P\",\"W\",\"D\",\"L\",\"PTS\"};"
		"\"TEAM\";TEAM;\"P\";P;\"W\";W;\"D\";D;\"L\";L;\"PTS\";PTS);{6};{-1})"
		")";

	tBool wOk = m_Api->UndoCellValue("N1", wFormula);
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile LET league table", wOk);

	// Result spills at N1: 3 rows x 6 cols (N=14 TEAM .. S=19 PTS).
	DrawCell("LET league table N1:S3:", 1, 14, 3, 19);

	// The top-left cell must NOT be an error (#VALUE!/#SPILL!).
	const tVariant wN1 = m_Api->CellValue(1, 14);
	CPPUNIT_ASSERT_MESSAGE("N1 not an error", !wN1.IsError());

	auto wStr = [&](tInt sRow, tInt sCol) -> tString {
		tVariant wV = m_Api->CellValue(sRow, sCol);
		return wV.String();
	};
	tString wC = "C", wA = "A", wB = "B";
	// Row 1: C, 4,2,2,0,8
	CPPUNIT_ASSERT_EQUAL(wStr(1, 14), wC);
	CPPUNIT_ASSERT_MESSAGE("row1 P", VariantNumericEq(m_Api->CellValue(1, 15), 4.0));
	CPPUNIT_ASSERT_MESSAGE("row1 W", VariantNumericEq(m_Api->CellValue(1, 16), 2.0));
	CPPUNIT_ASSERT_MESSAGE("row1 D", VariantNumericEq(m_Api->CellValue(1, 17), 2.0));
	CPPUNIT_ASSERT_MESSAGE("row1 L", VariantNumericEq(m_Api->CellValue(1, 18), 0.0));
	CPPUNIT_ASSERT_MESSAGE("row1 PTS", VariantNumericEq(m_Api->CellValue(1, 19), 8.0));
	// Row 2: A, 4,2,1,1,7
	CPPUNIT_ASSERT_EQUAL(wStr(2, 14), wA);
	CPPUNIT_ASSERT_MESSAGE("row2 PTS", VariantNumericEq(m_Api->CellValue(2, 19), 7.0));
	CPPUNIT_ASSERT_MESSAGE("row2 L", VariantNumericEq(m_Api->CellValue(2, 18), 1.0));
	// Row 3: B, 4,0,1,3,1
	CPPUNIT_ASSERT_EQUAL(wStr(3, 14), wB);
	CPPUNIT_ASSERT_MESSAGE("row3 W", VariantNumericEq(m_Api->CellValue(3, 16), 0.0));
	CPPUNIT_ASSERT_MESSAGE("row3 PTS", VariantNumericEq(m_Api->CellValue(3, 19), 1.0));
}

void TestSkMatrix::TestNamedLambda() {
	// Define a named LAMBDA and call it like a function. The body is compiled with the parameters as local
	// scope references (LetVarRef); arguments are bound at call time (see tCell::CallNamedLambda).
	tBool wOk = m_Api->UndoInsertFormulaNamed("SURFACE", "LAMBDA(l;h;l*h)");
	CPPUNIT_ASSERT_MESSAGE("insert named lambda SURFACE", wOk);

	// Literal arguments: SURFACE(3;4) = 12.
	wOk = m_Api->UndoCellValue("A1", "=SURFACE(3;4)");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile =SURFACE(3;4)", wOk);
	CPPUNIT_ASSERT_MESSAGE("SURFACE(3;4)=12", VariantNumericEq(m_Api->CellValue(1, 1), 12.0));

	// Cell arguments + dependency propagation: A2 depends on B1/C1 (compiled as refs in the caller).
	m_Api->UndoCellValue("B1", "=5");
	m_Api->UndoCellValue("C1", "=6");
	wOk = m_Api->UndoCellValue("A2", "=SURFACE(B1;C1)");
	CPPUNIT_ASSERT_MESSAGE("compile =SURFACE(B1;C1)", wOk);
	CPPUNIT_ASSERT_MESSAGE("SURFACE(B1;C1)=30", VariantNumericEq(m_Api->CellValue(2, 1), 30.0));

	// Changing an argument cell must recompute the caller.
	m_Api->UndoCellValue("B1", "=7");
	CPPUNIT_ASSERT_MESSAGE("SURFACE recompute after B1=7 -> 42", VariantNumericEq(m_Api->CellValue(2, 1), 42.0));

	// Multi-parameter body with an expression: HYP(a;b) = a*a + b*b.
	wOk = m_Api->UndoInsertFormulaNamed("HYP", "LAMBDA(a;b;a*a+b*b)");
	CPPUNIT_ASSERT_MESSAGE("insert named lambda HYP", wOk);
	wOk = m_Api->UndoCellValue("A3", "=HYP(3;4)");
	CPPUNIT_ASSERT_MESSAGE("compile =HYP(3;4)", wOk);
	CPPUNIT_ASSERT_MESSAGE("HYP(3;4)=25", VariantNumericEq(m_Api->CellValue(3, 1), 25.0));

	// Fewer arguments than parameters: the missing trailing param is bound to the omitted sentinel.
	// SURFACE's body uses both params (l*h), so the omitted h propagates as #VALUE!.
	wOk = m_Api->UndoCellValue("A4", "=SURFACE(3)");
	CPPUNIT_ASSERT_MESSAGE("compile =SURFACE(3)", wOk);
	CPPUNIT_ASSERT_MESSAGE("SURFACE(3) omitted h used in body -> error", m_Api->CellValue(4, 1).IsError());

	// Round-trip: a caller cell reads back with the lambda name preserved (FormulaStr re-lexes the key).
	tString wCallerA1 = m_Api->Formula("A1");
	CPPUNIT_ASSERT_MESSAGE("A1 formula keeps SURFACE(...)", wCallerA1.find("SURFACE") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("A1 formula not #REF!", wCallerA1.find("#REF!") == tString::npos);

	// Round-trip: the named-lambda definition reads back with the full LAMBDA(...) wrapper (not just the body).
	tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT_MESSAGE("active workbook", wWorkBook != nullptr);
	tString wDef = wWorkBook->RangeNamedContainer()->FormulaNamedStr("SURFACE");
	CPPUNIT_ASSERT_MESSAGE("SURFACE definition keeps LAMBDA wrapper", wDef.find("LAMBDA") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("SURFACE definition keeps body l*h", wDef.find("l*h") != tString::npos);
}

void TestSkMatrix::TestNamedLambdaRecursive() {
	// A recursive named LAMBDA compiles (the self-reference resolves as a lambda call, so no self-dependency is
	// created) AND terminates now that IF is short-circuit: only the taken branch is evaluated, so the base case
	// stops the recursion. FACT=LAMBDA(n;IF(n<=1;1;n*FACT(n-1))).
	tBool wOk = m_Api->UndoInsertFormulaNamed("FACT", "LAMBDA(n;IF(n<=1;1;n*FACT(n-1)))");
	CPPUNIT_ASSERT_MESSAGE("insert recursive named lambda FACT", wOk);

	wOk = m_Api->UndoCellValue("A1", "=FACT(5)");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile =FACT(5)", wOk);
	CPPUNIT_ASSERT_MESSAGE("FACT(5)=120", VariantNumericEq(m_Api->CellValue(1, 1), 120.0));

	wOk = m_Api->UndoCellValue("A2", "=FACT(0)");
	CPPUNIT_ASSERT_MESSAGE("compile =FACT(0)", wOk);
	CPPUNIT_ASSERT_MESSAGE("FACT(0)=1 (base case)", VariantNumericEq(m_Api->CellValue(2, 1), 1.0));

	wOk = m_Api->UndoCellValue("A3", "=FACT(1)");
	CPPUNIT_ASSERT_MESSAGE("compile =FACT(1)", wOk);
	CPPUNIT_ASSERT_MESSAGE("FACT(1)=1", VariantNumericEq(m_Api->CellValue(3, 1), 1.0));
}

void TestSkMatrix::TestLambdaHigherOrder() {
	// Source data A1:A3 = 1,2,3.
	m_Api->UndoCellValue("A1", "=1");
	m_Api->UndoCellValue("A2", "=2");
	m_Api->UndoCellValue("A3", "=3");

	// Named lambdas used as first-class arguments to higher-order functions.
	tBool wOk = m_Api->UndoInsertFormulaNamed("DOUBLE", "LAMBDA(x;x*2)");
	CPPUNIT_ASSERT_MESSAGE("insert named lambda DOUBLE", wOk);
	wOk = m_Api->UndoInsertFormulaNamed("ADD", "LAMBDA(a;b;a+b)");
	CPPUNIT_ASSERT_MESSAGE("insert named lambda ADD", wOk);

	// MAP(A1:A3; DOUBLE) -> {2;4;6} spilled into B1:B3.
	wOk = m_Api->UndoCellValue("B1", "=MAP(A1:A3;DOUBLE)");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile =MAP(A1:A3;DOUBLE)", wOk);
	CPPUNIT_ASSERT_MESSAGE("MAP B1=2", VariantNumericEq(m_Api->CellValue(1, 2), 2.0));
	CPPUNIT_ASSERT_MESSAGE("MAP B2=4", VariantNumericEq(m_Api->CellValue(2, 2), 4.0));
	CPPUNIT_ASSERT_MESSAGE("MAP B3=6", VariantNumericEq(m_Api->CellValue(3, 2), 6.0));

	// REDUCE(0; A1:A3; ADD) -> 0+1+2+3 = 6 (scalar in C1).
	wOk = m_Api->UndoCellValue("C1", "=REDUCE(0;A1:A3;ADD)");
	CPPUNIT_ASSERT_MESSAGE("compile =REDUCE(0;A1:A3;ADD)", wOk);
	CPPUNIT_ASSERT_MESSAGE("REDUCE C1=6", VariantNumericEq(m_Api->CellValue(1, 3), 6.0));

	// SCAN(0; A1:A3; ADD) -> running sums {1;3;6} spilled into E1:E3.
	wOk = m_Api->UndoCellValue("E1", "=SCAN(0;A1:A3;ADD)");
	CPPUNIT_ASSERT_MESSAGE("compile =SCAN(0;A1:A3;ADD)", wOk);
	CPPUNIT_ASSERT_MESSAGE("SCAN E1=1", VariantNumericEq(m_Api->CellValue(1, 5), 1.0));
	CPPUNIT_ASSERT_MESSAGE("SCAN E2=3", VariantNumericEq(m_Api->CellValue(2, 5), 3.0));
	CPPUNIT_ASSERT_MESSAGE("SCAN E3=6", VariantNumericEq(m_Api->CellValue(3, 5), 6.0));

	// MAP over two arrays with a 2-arg lambda: MAP(A1:A3; A1:A3; ADD) -> {2;4;6} in G1:G3.
	wOk = m_Api->UndoCellValue("G1", "=MAP(A1:A3;A1:A3;ADD)");
	CPPUNIT_ASSERT_MESSAGE("compile =MAP(A1:A3;A1:A3;ADD)", wOk);
	CPPUNIT_ASSERT_MESSAGE("MAP2 G1=2", VariantNumericEq(m_Api->CellValue(1, 7), 2.0));
	CPPUNIT_ASSERT_MESSAGE("MAP2 G3=6", VariantNumericEq(m_Api->CellValue(3, 7), 6.0));

	// Round-trip: a bare named lambda passed as a value (LambdaRef) pushes no VectorRef entry, so the caller
	// must still read back with the range AND the lambda name intact (no ref-index misalignment -> no #REF!).
	tString wMapFormula = m_Api->Formula("B1");
	CPPUNIT_ASSERT_MESSAGE("B1 keeps MAP", wMapFormula.find("MAP") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("B1 keeps DOUBLE", wMapFormula.find("DOUBLE") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("B1 keeps range A1:A3", wMapFormula.find("A1:A3") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("B1 not #REF!", wMapFormula.find("#REF!") == tString::npos);

	// Round-trip: named-lambda definition keeps the LAMBDA(...) wrapper.
	tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT_MESSAGE("active workbook", wWorkBook != nullptr);
	tString wDoubleDef = wWorkBook->RangeNamedContainer()->FormulaNamedStr("DOUBLE");
	CPPUNIT_ASSERT_MESSAGE("DOUBLE definition keeps LAMBDA wrapper", wDoubleDef.find("LAMBDA") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("DOUBLE definition keeps body x*2", wDoubleDef.find("x*2") != tString::npos);
}

void TestSkMatrix::TestByRowByColMakeArray() {
	// Note: this locale uses ';' as the argument separator.
	m_Api->UndoCellValue("A1", 1);
	m_Api->UndoCellValue("B1", 2);
	m_Api->UndoCellValue("A2", 3);
	m_Api->UndoCellValue("B2", 4);

	// BYROW: sum each row -> {3;7} in D1:D2.
	tBool wOk = m_Api->UndoCellValue("D1", "=BYROW(A1:B2;LAMBDA(r;SUM(r)))");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile BYROW", wOk);
	CPPUNIT_ASSERT_MESSAGE("BYROW D1", VariantNumericEq(m_Api->CellValue(1, 4), 3.0));
	CPPUNIT_ASSERT_MESSAGE("BYROW D2", VariantNumericEq(m_Api->CellValue(2, 4), 7.0));

	// BYCOL: sum each column -> {4;6} in F1:G1.
	wOk = m_Api->UndoCellValue("F1", "=BYCOL(A1:B2;LAMBDA(c;SUM(c)))");
	CPPUNIT_ASSERT_MESSAGE("compile BYCOL", wOk);
	CPPUNIT_ASSERT_MESSAGE("BYCOL F1", VariantNumericEq(m_Api->CellValue(1, 6), 4.0));
	CPPUNIT_ASSERT_MESSAGE("BYCOL G1", VariantNumericEq(m_Api->CellValue(1, 7), 6.0));

	// MAKEARRAY: multiplication table 2x3 -> H1:J2 = r*c (1-based).
	wOk = m_Api->UndoCellValue("H1", "=MAKEARRAY(2;3;LAMBDA(r;c;r*c))");
	CPPUNIT_ASSERT_MESSAGE("compile MAKEARRAY", wOk);
	CPPUNIT_ASSERT_MESSAGE("MAKEARRAY H1", VariantNumericEq(m_Api->CellValue(1, 8), 1.0));
	CPPUNIT_ASSERT_MESSAGE("MAKEARRAY I1", VariantNumericEq(m_Api->CellValue(1, 9), 2.0));
	CPPUNIT_ASSERT_MESSAGE("MAKEARRAY J1", VariantNumericEq(m_Api->CellValue(1, 10), 3.0));
	CPPUNIT_ASSERT_MESSAGE("MAKEARRAY H2", VariantNumericEq(m_Api->CellValue(2, 8), 2.0));
	CPPUNIT_ASSERT_MESSAGE("MAKEARRAY I2", VariantNumericEq(m_Api->CellValue(2, 9), 4.0));
	CPPUNIT_ASSERT_MESSAGE("MAKEARRAY J2", VariantNumericEq(m_Api->CellValue(2, 10), 6.0));
}

void TestSkMatrix::TestInlineLambda() {
	// Inline (anonymous) LAMBDA: desugared into a hidden named lambda (_INLLMB_...) at compile time, then
	// used exactly like a named lambda. Two shapes: immediate application LAMBDA(...)(args) and passing an
	// inline LAMBDA as a first-class argument to a higher-order function (MAP/REDUCE/SCAN).

	// --- Immediate application ------------------------------------------------------------------------
	// =LAMBDA(x;x+1)(5) = 6.
	tBool wOk = m_Api->UndoCellValue("A1", "=LAMBDA(x;x+1)(5)");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile =LAMBDA(x;x+1)(5)", wOk);
	CPPUNIT_ASSERT_MESSAGE("LAMBDA(x;x+1)(5)=6", VariantNumericEq(m_Api->CellValue(1, 1), 6.0));

	// Multi-parameter immediate application: =LAMBDA(l;h;l*h)(3;4) = 12.
	wOk = m_Api->UndoCellValue("A2", "=LAMBDA(l;h;l*h)(3;4)");
	CPPUNIT_ASSERT_MESSAGE("compile =LAMBDA(l;h;l*h)(3;4)", wOk);
	CPPUNIT_ASSERT_MESSAGE("LAMBDA(l;h;l*h)(3;4)=12", VariantNumericEq(m_Api->CellValue(2, 1), 12.0));

	// Round-trip: the cell reads back with the verbatim LAMBDA(...) text (the hidden name is expanded), and
	// never leaks the internal _INLLMB_ placeholder nor a #REF!.
	tString wA1 = m_Api->Formula("A1");
	CPPUNIT_ASSERT_MESSAGE("A1 round-trip keeps LAMBDA", wA1.find("LAMBDA") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("A1 round-trip keeps body x+1", wA1.find("x+1") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("A1 round-trip no hidden name", wA1.find("_INLLMB_") == tString::npos);
	CPPUNIT_ASSERT_MESSAGE("A1 round-trip not #REF!", wA1.find("#REF!") == tString::npos);

	// --- Inline lambda as a higher-order-function argument --------------------------------------------
	// Source data B1:B3 = 1,2,3.
	m_Api->UndoCellValue("B1", "=1");
	m_Api->UndoCellValue("B2", "=2");
	m_Api->UndoCellValue("B3", "=3");

	// MAP(B1:B3; LAMBDA(x;x*2)) -> {2;4;6} spilled into C1:C3.
	wOk = m_Api->UndoCellValue("C1", "=MAP(B1:B3;LAMBDA(x;x*2))");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile =MAP(B1:B3;LAMBDA(x;x*2))", wOk);
	CPPUNIT_ASSERT_MESSAGE("inline MAP C1=2", VariantNumericEq(m_Api->CellValue(1, 3), 2.0));
	CPPUNIT_ASSERT_MESSAGE("inline MAP C2=4", VariantNumericEq(m_Api->CellValue(2, 3), 4.0));
	CPPUNIT_ASSERT_MESSAGE("inline MAP C3=6", VariantNumericEq(m_Api->CellValue(3, 3), 6.0));

	// REDUCE(0; B1:B3; LAMBDA(a;b;a+b)) -> 6 (scalar in D1).
	wOk = m_Api->UndoCellValue("D1", "=REDUCE(0;B1:B3;LAMBDA(a;b;a+b))");
	CPPUNIT_ASSERT_MESSAGE("compile =REDUCE(0;B1:B3;LAMBDA(a;b;a+b))", wOk);
	CPPUNIT_ASSERT_MESSAGE("inline REDUCE D1=6", VariantNumericEq(m_Api->CellValue(1, 4), 6.0));

	// SCAN(0; B1:B3; LAMBDA(a;b;a+b)) -> running sums {1;3;6} in E1:E3.
	wOk = m_Api->UndoCellValue("E1", "=SCAN(0;B1:B3;LAMBDA(a;b;a+b))");
	CPPUNIT_ASSERT_MESSAGE("compile =SCAN(0;B1:B3;LAMBDA(a;b;a+b))", wOk);
	CPPUNIT_ASSERT_MESSAGE("inline SCAN E1=1", VariantNumericEq(m_Api->CellValue(1, 5), 1.0));
	CPPUNIT_ASSERT_MESSAGE("inline SCAN E2=3", VariantNumericEq(m_Api->CellValue(2, 5), 3.0));
	CPPUNIT_ASSERT_MESSAGE("inline SCAN E3=6", VariantNumericEq(m_Api->CellValue(3, 5), 6.0));

	// Round-trip: the MAP caller keeps the range AND the verbatim inline LAMBDA, without leaking the hidden
	// name or misaligning the VectorRef (#REF!).
	tString wC1 = m_Api->Formula("C1");
	CPPUNIT_ASSERT_MESSAGE("C1 keeps MAP", wC1.find("MAP") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("C1 keeps range B1:B3", wC1.find("B1:B3") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("C1 keeps inline LAMBDA body x*2", wC1.find("x*2") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("C1 no hidden name", wC1.find("_INLLMB_") == tString::npos);
	CPPUNIT_ASSERT_MESSAGE("C1 not #REF!", wC1.find("#REF!") == tString::npos);

	// Two identical inline lambdas share one hidden helper (dedup by content); both still evaluate.
	wOk = m_Api->UndoCellValue("F1", "=LAMBDA(x;x+1)(9)");
	CPPUNIT_ASSERT_MESSAGE("compile second =LAMBDA(x;x+1)(9)", wOk);
	CPPUNIT_ASSERT_MESSAGE("shared inline LAMBDA(x;x+1)(9)=10", VariantNumericEq(m_Api->CellValue(1, 6), 10.0));

	// --- Closures: an inline lambda captures outer names (LET vars / enclosing params) --------------------

	// Immediate application capturing a LET variable: =LET(a;5;LAMBDA(x;x+a)(3)) = 8.
	wOk = m_Api->UndoCellValue("A3", "=LET(a;5;LAMBDA(x;x+a)(3))");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile closure immediate", wOk);
	CPPUNIT_ASSERT_MESSAGE("closure LET(a;5;LAMBDA(x;x+a)(3))=8", VariantNumericEq(m_Api->CellValue(3, 1), 8.0));

	// A LAMBDA parameter shadows a captured LET variable of the same name: =LET(x;99;LAMBDA(x;x+1)(4)) = 5.
	wOk = m_Api->UndoCellValue("A4", "=LET(x;99;LAMBDA(x;x+1)(4))");
	CPPUNIT_ASSERT_MESSAGE("compile closure shadow", wOk);
	CPPUNIT_ASSERT_MESSAGE("closure shadow LET(x;99;LAMBDA(x;x+1)(4))=5", VariantNumericEq(m_Api->CellValue(4, 1), 5.0));

	// Closure captured by a higher-order function: =LET(a;5;MAP(B1:B3;LAMBDA(x;x+a))) spills {6;7;8} into G1:G3.
	// The value is created (LambdaRef) inside the LET scope, so a=5 is snapshotted and bound at each MAP call.
	wOk = m_Api->UndoCellValue("G1", "=LET(a;5;MAP(B1:B3;LAMBDA(x;x+a)))");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile closure MAP", wOk);
	CPPUNIT_ASSERT_MESSAGE("closure MAP G1=6", VariantNumericEq(m_Api->CellValue(1, 7), 6.0));
	CPPUNIT_ASSERT_MESSAGE("closure MAP G2=7", VariantNumericEq(m_Api->CellValue(2, 7), 7.0));
	CPPUNIT_ASSERT_MESSAGE("closure MAP G3=8", VariantNumericEq(m_Api->CellValue(3, 7), 8.0));

	// Closure captured by REDUCE: =LET(k;10;REDUCE(0;B1:B3;LAMBDA(acc;x;acc+x*k))) = 10*(1+2+3) = 60.
	wOk = m_Api->UndoCellValue("D2", "=LET(k;10;REDUCE(0;B1:B3;LAMBDA(acc;x;acc+x*k)))");
	CPPUNIT_ASSERT_MESSAGE("compile closure REDUCE", wOk);
	CPPUNIT_ASSERT_MESSAGE("closure REDUCE D2=60", VariantNumericEq(m_Api->CellValue(2, 4), 60.0));

	// Same span in a NON-closure context must NOT alias the closure helper (hidden name keyed on captures):
	// H1 uses a as a plain reference-less name -> the body still compiles, but no capture happens here.
	// Verify the closure MAP round-trips with its verbatim body and no hidden-name leak.
	tString wG1 = m_Api->Formula("G1");
	CPPUNIT_ASSERT_MESSAGE("G1 keeps inline body x+a", wG1.find("x+a") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("G1 no hidden name", wG1.find("_INLLMB_") == tString::npos);
}

void TestSkMatrix::TestLetBoundLambdaCall() {
	// Step 5: a LAMBDA stored in a LET variable can be called like a function from within the LET body,
	// e.g. LET(convert999; LAMBDA(n; ...); convert999(x)). The name resolves against the LET scope
	// (a first-class t_Lambda value) at evaluation time, in addition to the global named-lambda container.

	// Single-parameter call: =LET(f;LAMBDA(x;x+1);f(3)) = 4.
	tBool wOk = m_Api->UndoCellValue("A1", "=LET(f;LAMBDA(x;x+1);f(3))");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile LET(f;LAMBDA(x;x+1);f(3))", wOk);
	CPPUNIT_ASSERT_MESSAGE("LET(f;LAMBDA(x;x+1);f(3))=4", VariantNumericEq(m_Api->CellValue(1, 1), 4.0));

	// Multi-parameter call: =LET(g;LAMBDA(a;b;a*b);g(6;7)) = 42.
	wOk = m_Api->UndoCellValue("A2", "=LET(g;LAMBDA(a;b;a*b);g(6;7))");
	CPPUNIT_ASSERT_MESSAGE("compile LET(g;LAMBDA(a;b;a*b);g(6;7))", wOk);
	CPPUNIT_ASSERT_MESSAGE("LET(g;...;g(6;7))=42", VariantNumericEq(m_Api->CellValue(2, 1), 42.0));

	// The LET-bound lambda used several times in the same body.
	wOk = m_Api->UndoCellValue("A3", "=LET(f;LAMBDA(x;x*x);f(3)+f(4))");
	CPPUNIT_ASSERT_MESSAGE("compile LET(f;LAMBDA(x;x*x);f(3)+f(4))", wOk);
	CPPUNIT_ASSERT_MESSAGE("f(3)+f(4)=25", VariantNumericEq(m_Api->CellValue(3, 1), 25.0));

	// Closure: the LET-bound lambda captures an earlier LET variable (k). Its closure is snapshotted when
	// the value is bound, so calling f(5) later still sees k=10: =LET(k;10;f;LAMBDA(x;x*k);f(5)) = 50.
	wOk = m_Api->UndoCellValue("A4", "=LET(k;10;f;LAMBDA(x;x*k);f(5))");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile LET(k;10;f;LAMBDA(x;x*k);f(5))", wOk);
	CPPUNIT_ASSERT_MESSAGE("captured k: f(5)=50", VariantNumericEq(m_Api->CellValue(4, 1), 50.0));

	// A LET-bound helper that itself calls another LET-bound helper (mirrors NumberToWords' chunk helpers).
	wOk = m_Api->UndoCellValue("A5", "=LET(dbl;LAMBDA(x;x*2);quad;LAMBDA(x;dbl(dbl(x)));quad(3))");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile nested LET-bound helpers", wOk);
	CPPUNIT_ASSERT_MESSAGE("quad(3)=12", VariantNumericEq(m_Api->CellValue(5, 1), 12.0));

	// Calling a LET variable that is NOT a lambda is an error (Excel #CALC!/#NAME family), not a crash.
	wOk = m_Api->UndoCellValue("A6", "=LET(v;5;v(2))");
	// Compilation may succeed (ID '(' is a call form); evaluation must yield an error value.
	tVariant wErr = m_Api->CellValue(6, 1);
	CPPUNIT_ASSERT_MESSAGE("calling a non-lambda LET var is an error", wErr.IsError());
}

void TestSkMatrix::TestLambdaIsOmitted() {
	// Excel ISOMITTED + optional LAMBDA parameters: trailing arguments may be omitted; missing ones are
	// bound to a sentinel that only ISOMITTED reads as TRUE. Bracket notation [name] in the definition is
	// accepted and stripped to the bare parameter name (NumberToWords-style).

	// Named lambda with optional [currency]: GREET(n;[c];IF(ISOMITTED(c);"none";c))
	tBool wOk = m_Api->UndoInsertFormulaNamed(
		"GREET", "LAMBDA(n;[c];IF(ISOMITTED(c);\"none\";c))");
	CPPUNIT_ASSERT_MESSAGE("insert GREET with [c] optional param", wOk);

	// One argument: currency omitted -> "none".
	wOk = m_Api->UndoCellValue("A1", "=GREET(1234)");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile =GREET(1234)", wOk);
	CPPUNIT_ASSERT_EQUAL(tString("none"), m_Api->CellValue(1, 1).String());

	// Two arguments: currency supplied -> "USD".
	wOk = m_Api->UndoCellValue("A2", "=GREET(1234;\"USD\")");
	CPPUNIT_ASSERT_MESSAGE("compile =GREET(1234;\"USD\")", wOk);
	CPPUNIT_ASSERT_EQUAL(tString("USD"), m_Api->CellValue(2, 1).String());

	// Round-trip: named definition keeps the [c] notation in FormulaNamedStr.
	tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT_MESSAGE("active workbook", wWorkBook != nullptr);
	tString wDef = wWorkBook->RangeNamedContainer()->FormulaNamedStr("GREET");
	CPPUNIT_ASSERT_MESSAGE("GREET definition keeps LAMBDA", wDef.find("LAMBDA") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("GREET definition keeps [c]", wDef.find("[c]") != tString::npos ||
	                                                     wDef.find("[C]") != tString::npos);

	// Inline optional param (same semantics, desugared to a hidden named lambda).
	wOk = m_Api->UndoCellValue("A3", "=LAMBDA(n;[c];IF(ISOMITTED(c);n;n&c))(5)");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile inline ISOMITTED omitted", wOk);
	CPPUNIT_ASSERT_MESSAGE("inline omitted -> 5", VariantNumericEq(m_Api->CellValue(3, 1), 5.0));

	wOk = m_Api->UndoCellValue("A4", "=LAMBDA(n;[c];IF(ISOMITTED(c);n;n&c))(5;\"x\")");
	CPPUNIT_ASSERT_MESSAGE("compile inline ISOMITTED supplied", wOk);
	CPPUNIT_ASSERT_EQUAL(tString("5x"), m_Api->CellValue(4, 1).String());

	// Using an omitted parameter without ISOMITTED -> #VALUE! (sentinel surfaces as #VALUE!).
	wOk = m_Api->UndoCellValue("A5", "=LAMBDA(x;[y];x+y)(3)");
	CPPUNIT_ASSERT_MESSAGE("compile omitted used in arithmetic", wOk);
	CPPUNIT_ASSERT_MESSAGE("omitted y in x+y is an error", m_Api->CellValue(5, 1).IsError());

	// Too many arguments still #VALUE!.
	wOk = m_Api->UndoCellValue("A6", "=GREET(1;\"USD\";\"extra\")");
	CPPUNIT_ASSERT_MESSAGE("compile GREET too many args", wOk);
	CPPUNIT_ASSERT_MESSAGE("too many args -> error", m_Api->CellValue(6, 1).IsError());

	// LET-bound lambda with optional param (NumberToWords helper shape).
	wOk = m_Api->UndoCellValue(
		"A7", "=LET(f;LAMBDA(n;[c];IF(ISOMITTED(c);\"-\";c));f(1)&f(1;\"EUR\"))");
	if (!wOk) {
		cout << m_Api->Error() << ":" << m_Api->ErrorWithDetail() << endl;
	}
	CPPUNIT_ASSERT_MESSAGE("compile LET-bound ISOMITTED", wOk);
	CPPUNIT_ASSERT_EQUAL(tString("-EUR"), m_Api->CellValue(7, 1).String());
}

void TestSkMatrix::TestSpillSlaveDependentsRecalc() {
	// B1 spills A1+{0,1,2} → B1:D1. C2 and D2 reference the slaves C1/D1 (not the origin).
	// Changing A1 must update C2/D2 in the same pass (Calendar TEXT headers on C9:H9).
	tApplication::Instance()->Locale("fr");
	CPPUNIT_ASSERT(m_Api->UndoCellValue("A1", 10));
	CPPUNIT_ASSERT(m_Api->UndoCellValue("B1", "=A1+{0,1,2}"));
	CPPUNIT_ASSERT(m_Api->UndoCellValue("C2", "=C1*10"));
	CPPUNIT_ASSERT(m_Api->UndoCellValue("D2", "=D1*10"));
	CPPUNIT_ASSERT_MESSAGE("C1 first spill", VariantNumericEq(m_Api->CellValue("C1"), 11.0));
	CPPUNIT_ASSERT_MESSAGE("D1 first spill", VariantNumericEq(m_Api->CellValue("D1"), 12.0));
	CPPUNIT_ASSERT_MESSAGE("C2 first", VariantNumericEq(m_Api->CellValue("C2"), 110.0));
	CPPUNIT_ASSERT_MESSAGE("D2 first", VariantNumericEq(m_Api->CellValue("D2"), 120.0));

	CPPUNIT_ASSERT(m_Api->UndoCellValue("A1", 20));
	DrawCell("TestSpillSlaveDependentsRecalc after A1=20", 1, 1, 2, 4);
	CPPUNIT_ASSERT_MESSAGE("C1 respill", VariantNumericEq(m_Api->CellValue("C1"), 21.0));
	CPPUNIT_ASSERT_MESSAGE("D1 respill", VariantNumericEq(m_Api->CellValue("D1"), 22.0));
	CPPUNIT_ASSERT_MESSAGE("C2 must follow slave C1", VariantNumericEq(m_Api->CellValue("C2"), 210.0));
	CPPUNIT_ASSERT_MESSAGE("D2 must follow slave D1", VariantNumericEq(m_Api->CellValue("D2"), 220.0));
}

void TestSkMatrix::TestSpillSlaveCycleTerminates() {
	// B1 = C2+{0,1,2} spills B1:D1. C2 = C1 (slave). Without a pass guard, Resolve(B1)
	// re-Adds C2 forever (C2 updates → B1 respills → Add(C2) → …).
	tApplication::Instance()->Locale("fr");
	CPPUNIT_ASSERT(m_Api->UndoCellValue("C2", "=C1"));
	CPPUNIT_ASSERT(m_Api->UndoCellValue("B1", "=C2+{0,1,2}"));
	const tVariant wB1 = m_Api->CellValue("B1");
	const tVariant wC1 = m_Api->CellValue("C1");
	const tVariant wC2 = m_Api->CellValue("C2");
	CPPUNIT_ASSERT_MESSAGE("cycle must finish with a numeric or #RECURSIVE, not hang",
		wB1.IsNumeric() || (wB1.IsError() && wB1.Error().Code() == tTypeError::t_recursive));
	CPPUNIT_ASSERT_MESSAGE("C1 after cycle",
		wC1.IsNumeric() || (wC1.IsError() && wC1.Error().Code() == tTypeError::t_recursive)
		|| wC1.IsExcelNull());
	CPPUNIT_ASSERT_MESSAGE("C2 after cycle",
		wC2.IsNumeric() || (wC2.IsError() && wC2.Error().Code() == tTypeError::t_recursive)
		|| wC2.IsExcelNull());
}

void TestSkMatrix::TestMatrixReSpillOnModify() {
	// Two 2x2 source matrices: A1:B2 = {1,2;3,4}, C1:D2 = {5,6;7,8}.
	m_Api->UndoCellValue("A1", 1);
	m_Api->UndoCellValue("B1", 2);
	m_Api->UndoCellValue("A2", 3);
	m_Api->UndoCellValue("B2", 4);
	m_Api->UndoCellValue("C1", 5);
	m_Api->UndoCellValue("D1", 6);
	m_Api->UndoCellValue("C2", 7);
	m_Api->UndoCellValue("D2", 8);

	// First spill: =A1:B2+C1:D2 at E1 -> E1:F2 = {6,8;10,12}.
	CPPUNIT_ASSERT_MESSAGE("compile +", m_Api->UndoCellValue("E1", "=A1:B2+C1:D2"));
	DrawCell("ReSpill first E1:F2:", 1, 5, 2, 6);
	CPPUNIT_ASSERT_MESSAGE("first E1", VariantNumericEq(m_Api->CellValue(1, 5), 6.0));
	CPPUNIT_ASSERT_MESSAGE("first F2", VariantNumericEq(m_Api->CellValue(2, 6), 12.0));

	// Modify the same cell (re-enter): must re-spill without blocking on its own previous slaves.
	CPPUNIT_ASSERT_MESSAGE("recompile +", m_Api->UndoCellValue("E1", "=A1:B2+C1:D2"));
	DrawCell("ReSpill same E1:F2:", 1, 5, 2, 6);
	CPPUNIT_ASSERT_MESSAGE("re E1 not #SPILL", VariantNumericEq(m_Api->CellValue(1, 5), 6.0));
	CPPUNIT_ASSERT_MESSAGE("re F1", VariantNumericEq(m_Api->CellValue(1, 6), 8.0));
	CPPUNIT_ASSERT_MESSAGE("re E2", VariantNumericEq(m_Api->CellValue(2, 5), 10.0));
	CPPUNIT_ASSERT_MESSAGE("re F2", VariantNumericEq(m_Api->CellValue(2, 6), 12.0));

	// Modify to a different operation (=A1:B2*C1:D2) -> {5,12;21,32}. Same output size, still no #SPILL.
	CPPUNIT_ASSERT_MESSAGE("recompile *", m_Api->UndoCellValue("E1", "=A1:B2*C1:D2"));
	DrawCell("ReSpill mul E1:F2:", 1, 5, 2, 6);
	CPPUNIT_ASSERT_MESSAGE("mul E1 not #SPILL", VariantNumericEq(m_Api->CellValue(1, 5), 5.0));
	CPPUNIT_ASSERT_MESSAGE("mul F1", VariantNumericEq(m_Api->CellValue(1, 6), 12.0));
	CPPUNIT_ASSERT_MESSAGE("mul E2", VariantNumericEq(m_Api->CellValue(2, 5), 21.0));
	CPPUNIT_ASSERT_MESSAGE("mul F2", VariantNumericEq(m_Api->CellValue(2, 6), 32.0));
}

void TestSkMatrix::TestSpillPersistOnReadJson() {
	// Dynamic array spill: =SEQUENCE(3;2) at A1 -> A1:B3 = 1..6 (row-major).
	CPPUNIT_ASSERT_MESSAGE("compile SEQUENCE(3;2)", m_Api->UndoCellValue("A1", "=SEQUENCE(3;2)"));
	// Matrix spill: two 2x2 sources at D1:E2 and G1:H2, result =D1:E2+G1:H2 at D5 -> D5:E6.
	m_Api->UndoCellValue("D1", 1); m_Api->UndoCellValue("E1", 2);
	m_Api->UndoCellValue("D2", 3); m_Api->UndoCellValue("E2", 4);
	m_Api->UndoCellValue("G1", 5); m_Api->UndoCellValue("H1", 6);
	m_Api->UndoCellValue("G2", 7); m_Api->UndoCellValue("H2", 8);
	CPPUNIT_ASSERT_MESSAGE("compile matrix +", m_Api->UndoCellValue("D5", "=D1:E2+G1:H2"));

	// Sanity before round-trip.
	CPPUNIT_ASSERT_MESSAGE("seq A1", VariantNumericEq(m_Api->CellValue(1, 1), 1.0));
	CPPUNIT_ASSERT_MESSAGE("seq B3", VariantNumericEq(m_Api->CellValue(3, 2), 6.0));
	CPPUNIT_ASSERT_MESSAGE("mat D5", VariantNumericEq(m_Api->CellValue(5, 4), 6.0));
	CPPUNIT_ASSERT_MESSAGE("mat E6", VariantNumericEq(m_Api->CellValue(6, 5), 12.0));

	tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT(wWorkBook != nullptr);
	const tString wUri = wWorkBook->Uri();
	tString wJson = m_Api->WriteJson(wUri);
	CPPUNIT_ASSERT_MESSAGE("WriteJson non-empty", !wJson.empty());
	// The dynamic-array origin must persist its spill footprint (AFO) so load can restore it.
	CPPUNIT_ASSERT_MESSAGE("JSON keeps spillrange", wJson.find("spillrange") != tString::npos);

	tearDown();
	setUp();

	// Reload WITHOUT RecalculateAll: the spilled cells must reappear from persisted v/t + AFO.
	CPPUNIT_ASSERT_MESSAGE("ReadJson round-trip", m_Api->ReadJson(wJson));

	CPPUNIT_ASSERT_MESSAGE("seq A1 after load", VariantNumericEq(m_Api->CellValue(1, 1), 1.0));
	CPPUNIT_ASSERT_MESSAGE("seq A2 after load", VariantNumericEq(m_Api->CellValue(2, 1), 3.0));
	CPPUNIT_ASSERT_MESSAGE("seq B3 after load", VariantNumericEq(m_Api->CellValue(3, 2), 6.0));
	CPPUNIT_ASSERT_MESSAGE("mat D5 after load", VariantNumericEq(m_Api->CellValue(5, 4), 6.0));
	CPPUNIT_ASSERT_MESSAGE("mat E6 after load", VariantNumericEq(m_Api->CellValue(6, 5), 12.0));

	// Force a recompute of the matrix origin while its slaves still hold cached values: change a source cell (D1).
	// This is the exact reload scenario that used to yield #SPILL! for matrices (the persisted slaves blocked the
	// origin's re-spill). With the persisted AFO it must re-spill cleanly. D5=D1+G1, E6=E2+H2 (unchanged).
	CPPUNIT_ASSERT_MESSAGE("edit source D1", m_Api->UndoCellValue("D1", 10));
	DrawCell("After recompute D5:E6:", 5, 4, 6, 5);
	tVariant wD5 = m_Api->CellValue(5, 4);
	CPPUNIT_ASSERT_MESSAGE("D5 not #SPILL after recompute", !wD5.IsError());
	CPPUNIT_ASSERT_MESSAGE("mat D5 recompute", VariantNumericEq(m_Api->CellValue(5, 4), 15.0));
	CPPUNIT_ASSERT_MESSAGE("mat E5 recompute", VariantNumericEq(m_Api->CellValue(5, 5), 8.0));
	CPPUNIT_ASSERT_MESSAGE("mat D6 recompute", VariantNumericEq(m_Api->CellValue(6, 4), 10.0));
	CPPUNIT_ASSERT_MESSAGE("mat E6 recompute", VariantNumericEq(m_Api->CellValue(6, 5), 12.0));
}

void TestSkMatrix::TestSpillEditOriginAfterReload() {
	// Reproduce the app scenario: spill, exit/re-enter (WriteJson/ReadJson WITHOUT RecalculateAll), then edit the
	// spill origin's own formula. A restored spill must behave like an in-session one so the edit re-spills cleanly.
	CPPUNIT_ASSERT_MESSAGE("compile SEQUENCE(5)", m_Api->UndoCellValue("A1", "=SEQUENCE(5)"));
	// Range+Range matrix at D5 -> D5:E6.
	m_Api->UndoCellValue("D1", 1); m_Api->UndoCellValue("E1", 2);
	m_Api->UndoCellValue("D2", 3); m_Api->UndoCellValue("E2", 4);
	m_Api->UndoCellValue("G1", 5); m_Api->UndoCellValue("H1", 6);
	m_Api->UndoCellValue("G2", 7); m_Api->UndoCellValue("H2", 8);
	CPPUNIT_ASSERT_MESSAGE("compile matrix +", m_Api->UndoCellValue("D5", "=D1:E2+G1:H2"));
	CPPUNIT_ASSERT_MESSAGE("seq A5 before", VariantNumericEq(m_Api->CellValue(5, 1), 5.0));
	CPPUNIT_ASSERT_MESSAGE("mat E6 before", VariantNumericEq(m_Api->CellValue(6, 5), 12.0));

	tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT(wWorkBook != nullptr);
	tString wJson = m_Api->WriteJson(wWorkBook->Uri());
	CPPUNIT_ASSERT_MESSAGE("WriteJson non-empty", !wJson.empty());
	tearDown();
	setUp();
	// Reload WITHOUT RecalculateAll (mirrors the app: _ReadJson does not recalc).
	CPPUNIT_ASSERT_MESSAGE("ReadJson round-trip", m_Api->ReadJson(wJson));

	// Edit the dynamic-array origin: SEQUENCE(5) -> SEQUENCE(10). Must respill A1:A10 without #SPILL.
	CPPUNIT_ASSERT_MESSAGE("edit SEQUENCE(10)", m_Api->UndoCellValue("A1", "=SEQUENCE(10)"));
	DrawCell("After edit SEQUENCE(10):", 1, 1, 10, 1);
	CPPUNIT_ASSERT_MESSAGE("A1 not #SPILL after edit", !m_Api->CellValue(1, 1).IsError());
	CPPUNIT_ASSERT_MESSAGE("seq A1 after edit", VariantNumericEq(m_Api->CellValue(1, 1), 1.0));
	CPPUNIT_ASSERT_MESSAGE("seq A10 after edit", VariantNumericEq(m_Api->CellValue(10, 1), 10.0));

	// Edit the matrix origin (same size, different op): =D1:E2*G1:H2 -> {5,12;21,32}. Must respill without #SPILL.
	CPPUNIT_ASSERT_MESSAGE("edit matrix *", m_Api->UndoCellValue("D5", "=D1:E2*G1:H2"));
	DrawCell("After edit matrix *:", 5, 4, 6, 5);
	CPPUNIT_ASSERT_MESSAGE("D5 not #SPILL after edit", !m_Api->CellValue(5, 4).IsError());
	CPPUNIT_ASSERT_MESSAGE("mat D5 after edit", VariantNumericEq(m_Api->CellValue(5, 4), 5.0));
	CPPUNIT_ASSERT_MESSAGE("mat E6 after edit", VariantNumericEq(m_Api->CellValue(6, 5), 32.0));
}

void TestSkMatrix::TestSpillLegacyMatrixEditAfterReload() {
	// Reproduce a legacy / partial .sker (like Array.sker) where a matrix-operator spill (=N19:O20+Q19:R20 at N22)
	// was saved with cached slave <v> but WITHOUT its "spillrange" (AFO). On reload the origin keeps stale
	// MatOrigin/SpillRange flags with no materialized range, and the slave cells reload as orphaned MatExtend
	// value cells. Editing the origin must re-spill without #SPILL! (orphaned slaves must not block; load-time
	// flag reconciliation drops the stale origin flags).
	m_Api->UndoCellValue("N19", 12); m_Api->UndoCellValue("O19", 2);
	m_Api->UndoCellValue("Q19", 23); m_Api->UndoCellValue("R19", 7);
	m_Api->UndoCellValue("N20", 3);  m_Api->UndoCellValue("O20", 4);
	m_Api->UndoCellValue("Q20", 6);  m_Api->UndoCellValue("R20", 8);
	// Spill the matrix at N22 -> N22:O23 = {35,9;9,12} (col 14/15, rows 22/23).
	CPPUNIT_ASSERT_MESSAGE("compile N22 matrix", m_Api->UndoCellValue("N22", "=N19:O20+Q19:R20"));
	CPPUNIT_ASSERT_MESSAGE("N22 spilled O23", VariantNumericEq(m_Api->CellValue(23, 15), 12.0));

	tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT(wWorkBook != nullptr);
	tString wJson = m_Api->WriteJson(wWorkBook->Uri());
	CPPUNIT_ASSERT_MESSAGE("WriteJson non-empty", !wJson.empty());
	CPPUNIT_ASSERT_MESSAGE("JSON has spillrange before strip", wJson.find("spillrange") != tString::npos);

	// Degrade to the legacy shape: drop the persisted AFO ("spillrange") while keeping the origin flags and the
	// cached slave <v>. Only N22 spills here, so there is a single "spillrange" occurrence to remove.
	{
		const tString wNeedle = "\"spillrange\":";
		size_t wPos = wJson.find(wNeedle);
		CPPUNIT_ASSERT_MESSAGE("locate spillrange", wPos != tString::npos);
		size_t wStart = (wPos > 0 && wJson[wPos - 1] == ',') ? wPos - 1 : wPos;
		size_t wOpen = wJson.find('"', wPos + wNeedle.size()); // opening quote of the ref value
		size_t wClose = wJson.find('"', wOpen + 1);            // closing quote of the ref value
		CPPUNIT_ASSERT(wOpen != tString::npos && wClose != tString::npos);
		wJson.erase(wStart, (wClose + 1) - wStart);
	}
	CPPUNIT_ASSERT_MESSAGE("spillrange removed", wJson.find("spillrange") == tString::npos);

	tearDown();
	setUp();
	CPPUNIT_ASSERT_MESSAGE("ReadJson round-trip", m_Api->ReadJson(wJson));

	// Edit the matrix origin after load: orphaned slaves must not block, so it re-spills cleanly.
	CPPUNIT_ASSERT_MESSAGE("edit N22 matrix", m_Api->UndoCellValue("N22", "=N19:O20+Q19:R20"));
	DrawCell("After legacy reload edit N22:O23:", 22, 14, 23, 15);
	tVariant wN22v = m_Api->CellValue(22, 14);
	CPPUNIT_ASSERT_MESSAGE("N22 not #SPILL after edit", !wN22v.IsError());
	CPPUNIT_ASSERT_MESSAGE("N22", VariantNumericEq(m_Api->CellValue(22, 14), 35.0));
	CPPUNIT_ASSERT_MESSAGE("O22", VariantNumericEq(m_Api->CellValue(22, 15), 9.0));
	CPPUNIT_ASSERT_MESSAGE("N23", VariantNumericEq(m_Api->CellValue(23, 14), 9.0));
	CPPUNIT_ASSERT_MESSAGE("O23", VariantNumericEq(m_Api->CellValue(23, 15), 12.0));
}

void TestSkMatrix::TestMatrixResizeRespill() {
	// Sources: P1:R3 (cols 16..18) and T1:V3 (cols 20..22), each 3x3.
	m_Api->UndoCellValue("P1", 1);  m_Api->UndoCellValue("Q1", 2);  m_Api->UndoCellValue("R1", 3);
	m_Api->UndoCellValue("P2", 4);  m_Api->UndoCellValue("Q2", 5);  m_Api->UndoCellValue("R2", 6);
	m_Api->UndoCellValue("P3", 7);  m_Api->UndoCellValue("Q3", 8);  m_Api->UndoCellValue("R3", 9);
	m_Api->UndoCellValue("T1", 10); m_Api->UndoCellValue("U1", 20); m_Api->UndoCellValue("V1", 30);
	m_Api->UndoCellValue("T2", 40); m_Api->UndoCellValue("U2", 50); m_Api->UndoCellValue("V2", 60);
	m_Api->UndoCellValue("T3", 70); m_Api->UndoCellValue("U3", 80); m_Api->UndoCellValue("V3", 90);

	// First a 2x2 matrix at X1 (cols 24..25): =P1:Q2+T1:U2 -> {11,22;44,55}.
	CPPUNIT_ASSERT_MESSAGE("compile 2x2", m_Api->UndoCellValue("X1", "=P1:Q2+T1:U2"));
	DrawCell("2x2 X1:Y2:", 1, 24, 2, 25);
	CPPUNIT_ASSERT_MESSAGE("2x2 X1", VariantNumericEq(m_Api->CellValue(1, 24), 11.0));
	CPPUNIT_ASSERT_MESSAGE("2x2 Y2", VariantNumericEq(m_Api->CellValue(2, 25), 55.0));

	// Resize to 3x3: =P1:R3+T1:V3 -> {11,22,33;44,55,66;77,88,99}. The grown corner Z3 (row 3, col 26) must fill,
	// i.e. the native AFO is re-derived, not clamped to the previous 2x2 footprint.
	CPPUNIT_ASSERT_MESSAGE("resize 3x3", m_Api->UndoCellValue("X1", "=P1:R3+T1:V3"));
	DrawCell("3x3 X1:Z3:", 1, 24, 3, 26);
	tVariant wZ3 = m_Api->CellValue(3, 26);
	CPPUNIT_ASSERT_MESSAGE("Z3 not #SPILL after resize", !wZ3.IsError());
	CPPUNIT_ASSERT_MESSAGE("3x3 X1", VariantNumericEq(m_Api->CellValue(1, 24), 11.0));
	CPPUNIT_ASSERT_MESSAGE("3x3 Z1", VariantNumericEq(m_Api->CellValue(1, 26), 33.0));
	CPPUNIT_ASSERT_MESSAGE("3x3 X3", VariantNumericEq(m_Api->CellValue(3, 24), 77.0));
	CPPUNIT_ASSERT_MESSAGE("3x3 Z3", VariantNumericEq(m_Api->CellValue(3, 26), 99.0));
}

void TestSkMatrix::TestSpillAdjacentNoConflict() {
	// Two adjacent vertical spills: A1=SEQUENCE(3) -> A1:A3, B1=SEQUENCE(3) -> B1:B3. No shared cell -> no conflict.
	CPPUNIT_ASSERT_MESSAGE("compile A SEQUENCE(3)", m_Api->UndoCellValue("A1", "=SEQUENCE(3)"));
	CPPUNIT_ASSERT_MESSAGE("compile B SEQUENCE(3)", m_Api->UndoCellValue("B1", "=SEQUENCE(3)"));
	DrawCell("Adjacent A1:B3:", 1, 1, 3, 2);
	CPPUNIT_ASSERT_MESSAGE("A col not #SPILL", !m_Api->CellValue(1, 1).IsError());
	CPPUNIT_ASSERT_MESSAGE("B col not #SPILL", !m_Api->CellValue(1, 2).IsError());
	CPPUNIT_ASSERT_MESSAGE("A3", VariantNumericEq(m_Api->CellValue(3, 1), 3.0));
	CPPUNIT_ASSERT_MESSAGE("B3", VariantNumericEq(m_Api->CellValue(3, 2), 3.0));

	// Modify the first spill; the adjacent one must stay intact (its cells are never wiped).
	CPPUNIT_ASSERT_MESSAGE("modify A", m_Api->UndoCellValue("A1", "=SEQUENCE(3;1;100)"));
	DrawCell("After modify A1:B3:", 1, 1, 3, 2);
	CPPUNIT_ASSERT_MESSAGE("A1 modified", VariantNumericEq(m_Api->CellValue(1, 1), 100.0));
	CPPUNIT_ASSERT_MESSAGE("A3 modified", VariantNumericEq(m_Api->CellValue(3, 1), 102.0));
	CPPUNIT_ASSERT_MESSAGE("B1 intact", VariantNumericEq(m_Api->CellValue(1, 2), 1.0));
	CPPUNIT_ASSERT_MESSAGE("B3 intact", VariantNumericEq(m_Api->CellValue(3, 2), 3.0));

	// Overlap must still error: a static value blocks the spill destination -> #SPILL!.
	m_Api->UndoCellValue("E1", 99);
	CPPUNIT_ASSERT_MESSAGE("compile overlap D1", m_Api->UndoCellValue("D1", "=SEQUENCE(1;2)"));
	DrawCell("Overlap D1:E1:", 1, 4, 1, 5);
	tVariant wD1 = m_Api->CellValue(1, 4);
	CPPUNIT_ASSERT_MESSAGE("D1 is #SPILL! (E1 occupied)", wD1.IsError());
	CPPUNIT_ASSERT_MESSAGE("D1 error code t_spill", wD1.Error().Code() == tTypeError::t_spill);
}

void TestSkMatrix::TestArrayLiteralSemicolonKeepsVertical() {
	// User bug: in FR locale (arg separator ';'), typing =SORT({3;1;2;4}) came back as =SORT({3,1,2,4}). The stored
	// formula key rewrote EVERY ';' to the internal ',' — including the ROW separator inside the literal — turning a
	// vertical array into a horizontal one. In-session eval is fine (the parser reads the original ';'), but a
	// reload/RecalculateAll re-parses the corrupted key and spills sideways, colliding with the layout (#SPILL!).
	// Excel rule the user confirmed: {1,2,3} = columns (horizontal), {1;2;3} = rows (vertical).
	const tString wPrevLang = tApplication::Instance()->Locale()->Lang();
	tApplication::Instance()->Locale("fr");

	CPPUNIT_ASSERT_MESSAGE("compile =SORT({3;1;2;4})", m_Api->UndoCellValue("A1", "=SORT({3;1;2;4})"));

	// The stored/displayed formula must keep the ';' row separator, never a ','.
	const tString wFormula = m_Api->Formula(1, 1);
	CPPUNIT_ASSERT_MESSAGE("formula keeps ';' rows", FormulaStrContains(wFormula, "3;1;2;4"));
	CPPUNIT_ASSERT_MESSAGE("formula not flipped to ',' columns", !FormulaStrContains(wFormula, "3,1,2,4"));

	// Vertical spill A1:A4 = {1;2;3;4} (SORT ascending). If it were horizontal these column cells (A2:A4) would be
	// empty and the numeric checks below would fail — so they alone prove the orientation was preserved.
	CPPUNIT_ASSERT_MESSAGE("A1 not #SPILL", !m_Api->CellValue(1, 1).IsError());
	CPPUNIT_ASSERT_MESSAGE("A1", VariantNumericEq(m_Api->CellValue(1, 1), 1.0));
	CPPUNIT_ASSERT_MESSAGE("A2", VariantNumericEq(m_Api->CellValue(2, 1), 2.0));
	CPPUNIT_ASSERT_MESSAGE("A3", VariantNumericEq(m_Api->CellValue(3, 1), 3.0));
	CPPUNIT_ASSERT_MESSAGE("A4", VariantNumericEq(m_Api->CellValue(4, 1), 4.0));

	// Reload + RecalculateAll: re-parses the stored key. With the fix it stays vertical; the old bug spilled sideways.
	tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
	CPPUNIT_ASSERT(wWorkBook != nullptr);
	const tString wJson = m_Api->WriteJson(wWorkBook->Uri());
	CPPUNIT_ASSERT_MESSAGE("WriteJson non-empty", !wJson.empty());
	tearDown();
	setUp();
	CPPUNIT_ASSERT_MESSAGE("ReadJson round-trip", m_Api->ReadJson(wJson));
	CPPUNIT_ASSERT_MESSAGE("formula keeps ';' after load", FormulaStrContains(m_Api->Formula(1, 1), "3;1;2;4"));

	m_Api->ActiveWorkBook()->RecalculateAll();
	CPPUNIT_ASSERT_MESSAGE("A1 not #SPILL after recalc", !m_Api->CellValue(1, 1).IsError());
	CPPUNIT_ASSERT_MESSAGE("A1 after recalc", VariantNumericEq(m_Api->CellValue(1, 1), 1.0));
	CPPUNIT_ASSERT_MESSAGE("A2 after recalc", VariantNumericEq(m_Api->CellValue(2, 1), 2.0));
	CPPUNIT_ASSERT_MESSAGE("A4 after recalc", VariantNumericEq(m_Api->CellValue(4, 1), 4.0));

	tApplication::Instance()->Locale(wPrevLang);
}

void TestSkMatrix::TestSpillRefHash() {
	// D14 = SEQUENCE(3) spills D14:D16. COUNTA(D14#) counts the whole spill, not just D14.
	CPPUNIT_ASSERT_MESSAGE("compile SEQUENCE(3) at D14", m_Api->UndoCellValue("D14", "=SEQUENCE(3)"));
	CPPUNIT_ASSERT_MESSAGE("D14 origin", VariantNumericEq(m_Api->CellValue(14, 4), 1.0));
	CPPUNIT_ASSERT_MESSAGE("D15 slave", VariantNumericEq(m_Api->CellValue(15, 4), 2.0));
	CPPUNIT_ASSERT_MESSAGE("D16 slave", VariantNumericEq(m_Api->CellValue(16, 4), 3.0));

	CPPUNIT_ASSERT_MESSAGE("compile COUNTA(D14#)", m_Api->UndoCellValue("F14", "=COUNTA(D14#)"));
	tCell* wCountA = m_Api->Cell("F14");
	CPPUNIT_ASSERT_MESSAGE("F14 exists", wCountA != nullptr);
	CPPUNIT_ASSERT_MESSAGE("COUNTA(D14#)=3", VariantNumericEq(wCountA->Value(), 3.0));
	CPPUNIT_ASSERT_MESSAGE("FormulaStr keeps #", FormulaStrContains(wCountA->FormulaStr(), "D14#"));

	tBool wFoundSpillRef = false;
	const tFormula* wFormula = wCountA->Formula();
	CPPUNIT_ASSERT_MESSAGE("F14 has formula", wFormula != nullptr);
	for (const auto& wItem : *wFormula->VectorItemFormula()) {
		if (wItem.Kind() == tKind::SpillRef) {
			wFoundSpillRef = true;
			break;
		}
	}
	CPPUNIT_ASSERT_MESSAGE("bytecode contains SpillRef", wFoundSpillRef);

	// SEQUENCE(COUNTA(D14#)) at H14 -> 1,2,3
	CPPUNIT_ASSERT_MESSAGE("compile SEQUENCE(COUNTA(D14#))",
		m_Api->UndoCellValue("H14", "=SEQUENCE(COUNTA(D14#))"));
	CPPUNIT_ASSERT_MESSAGE("H14", VariantNumericEq(m_Api->CellValue(14, 8), 1.0));
	CPPUNIT_ASSERT_MESSAGE("H15", VariantNumericEq(m_Api->CellValue(15, 8), 2.0));
	CPPUNIT_ASSERT_MESSAGE("H16", VariantNumericEq(m_Api->CellValue(16, 8), 3.0));

	// Resize the origin spill: COUNTA(D14#) and SEQUENCE(COUNTA(D14#)) follow.
	CPPUNIT_ASSERT_MESSAGE("resize SEQUENCE(5)", m_Api->UndoCellValue("D14", "=SEQUENCE(5)"));
	CPPUNIT_ASSERT_MESSAGE("COUNTA after resize", VariantNumericEq(m_Api->CellValue(14, 6), 5.0));
	CPPUNIT_ASSERT_MESSAGE("H18 after resize", VariantNumericEq(m_Api->CellValue(18, 8), 5.0));

	// Not a spill origin → #REF! (test the operator itself; COUNTA would count a scalar error as 1).
	CPPUNIT_ASSERT_MESSAGE("typed value at A1", m_Api->UndoCellValue("A1", 42));
	CPPUNIT_ASSERT_MESSAGE("compile A1#", m_Api->UndoCellValue("B1", "=A1#"));
	tVariant wRef = m_Api->CellValue(1, 2);
	CPPUNIT_ASSERT_MESSAGE("A1# is #REF!", wRef.IsError());
	CPPUNIT_ASSERT_MESSAGE("A1# error is #REF!", wRef.Error().Code() == tTypeError::t_ref);

	// Cross-sheet: Sheet2!D14#
	tSheet* wSheet1 = m_Api->ActiveSheet();
	CPPUNIT_ASSERT_MESSAGE("sheet1 exists", wSheet1 != nullptr);
	const tString wSheet1Name = wSheet1->Name();
	CPPUNIT_ASSERT_MESSAGE("add Sheet2", m_Api->UndoAddSheet("Sheet2", ""));
	tSheet* wSheet2 = m_Api->ActiveSheet("Sheet2");
	CPPUNIT_ASSERT_MESSAGE("Sheet2 exists", wSheet2 != nullptr);
	CPPUNIT_ASSERT_MESSAGE("Sheet2 SEQUENCE", m_Api->UndoCellValue("D14", "=SEQUENCE(4)", wSheet2));
	m_Api->ActiveSheet(wSheet1Name);
	CPPUNIT_ASSERT_MESSAGE("compile COUNTA(Sheet2!D14#)",
		m_Api->UndoCellValue("J14", "=COUNTA(Sheet2!D14#)"));
	CPPUNIT_ASSERT_MESSAGE("Sheet2!D14# count", VariantNumericEq(m_Api->CellValue(14, 10), 4.0));
	CPPUNIT_ASSERT_MESSAGE("Sheet2 FormulaStr keeps #",
		FormulaStrContains(m_Api->Formula(14, 10), "D14#"));
}

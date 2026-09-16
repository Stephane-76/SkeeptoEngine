//==============================================================================
// TestSkFindCell
// Tests for FindCell match case / entire cell options
//==============================================================================
#include "../include/TestSkFindCell.hpp"

#include <SkFormatRoot.hpp>
#include <SkFormatCssApi.hpp>

#include <set>

using namespace SkFormat;

namespace {

static std::set<tString> RefSetFromFind(tVectorCell* sCells) {
	std::set<tString> wRefs;
	if (sCells != nullptr) {
		for (tCell* wCell : *sCells) {
			wRefs.insert(wCell->StrRef());
		}
		delete sCells;
	}
	return(wRefs);
}

static tString RefSetToString(const std::set<tString>& sRefs) {
	tStringStream wStream;
	for (const tString& wRef : sRefs) {
		if (!wStream.str().empty()) {
			wStream << ",";
		}
		wStream << wRef;
	}
	return(wStream.str());
}

static void AssertFindRefs(tSheet* sSheet, const tString& sSearch,
	tBool sMatchCase, tBool sMatchEntireCell,
	const std::initializer_list<const char*>& sExpected, const char* sMessage) {
	std::set<tString> wGot = RefSetFromFind(sSheet->FindCell(sSearch, sMatchCase, sMatchEntireCell));
	std::set<tString> wWant;
	for (const char* wRef : sExpected) {
		wWant.insert(wRef);
	}
	if (wGot != wWant) {
		tStringStream wDetail;
		wDetail << sMessage << " expected={" << RefSetToString(wWant) << "} got={" << RefSetToString(wGot) << "}";
		CPPUNIT_FAIL(wDetail.str().c_str());
	}
}

static tBool JsonContainsRef(const tString& sJson, const tString& sRef) {
	return(sJson.find("\"" + sRef + "\"") != tString::npos);
}

} // namespace

TestSkFindCell::TestSkFindCell() : CPPUNIT_NS::TestFixture(), m_Application(nullptr), m_Api(nullptr), m_FormatApi(nullptr) {
}

void TestSkFindCell::setUp() {
	std::filesystem::remove_all("./Spreadsheet");

	m_Application = tApplication::Instance();
	m_Application->Locale("us");
	tFormatRoot::Instance();
	m_Api = new tApi;
	m_Api->NewWorkBook("wwww.skeema.fr/w1");
	m_FormatApi = new tFormatCssApi();
	m_Api->FormatApi(m_FormatApi);
}

void TestSkFindCell::tearDown() {
	delete(m_Api);
	if (m_FormatApi != nullptr) {
		delete(m_FormatApi);
		m_FormatApi = nullptr;
	}
	DoneFormatRoot();
}

void TestSkFindCell::SetupFindGrid() {
	m_Api->UndoCellValue("A1", "Hello World");
	m_Api->UndoCellValue("A2", "hello world");
	m_Api->UndoCellValue("A3", "HELLO");
	m_Api->UndoCellValue("A4", "Say Hello");
	m_Api->UndoCellValue("B1", "hello");
	m_Api->UndoCellValue("B2", "hell");
}

void TestSkFindCell::TestFindCellPartialIgnoreCase() {
	SetupFindGrid();
	tSheet* wSheet = m_Api->ActiveSheet();

	AssertFindRefs(wSheet, "hello", false, false,
		{ "A1", "A2", "A3", "A4", "B1" },
		"partial search ignores case");
	AssertFindRefs(wSheet, "HELLO", false, false,
		{ "A1", "A2", "A3", "A4", "B1" },
		"partial search ignores case (upper needle)");
}

void TestSkFindCell::TestFindCellPartialMatchCase() {
	SetupFindGrid();
	tSheet* wSheet = m_Api->ActiveSheet();

	AssertFindRefs(wSheet, "hello", true, false,
		{ "A2", "B1" },
		"partial search with match case");
	AssertFindRefs(wSheet, "Hello", true, false,
		{ "A1", "A4" },
		"partial search with match case (capital H)");
}

void TestSkFindCell::TestFindCellEntireIgnoreCase() {
	SetupFindGrid();
	tSheet* wSheet = m_Api->ActiveSheet();

	AssertFindRefs(wSheet, "hello", false, true,
		{ "A3", "B1" },
		"entire cell ignores case");
	AssertFindRefs(wSheet, "HELLO", false, true,
		{ "A3", "B1" },
		"entire cell ignores case (upper needle)");
}

void TestSkFindCell::TestFindCellEntireMatchCase() {
	SetupFindGrid();
	tSheet* wSheet = m_Api->ActiveSheet();

	AssertFindRefs(wSheet, "hello", true, true,
		{ "B1" },
		"entire cell with match case (lower)");
	AssertFindRefs(wSheet, "HELLO", true, true,
		{ "A3" },
		"entire cell with match case (upper)");
}

void TestSkFindCell::TestFindCellNoMatch() {
	SetupFindGrid();
	tSheet* wSheet = m_Api->ActiveSheet();

	AssertFindRefs(wSheet, "xyz", false, false, {}, "no partial match");
	AssertFindRefs(wSheet, "hello world", false, true, { "A1", "A2" }, "entire cell exact ignore case");
	AssertFindRefs(wSheet, "Hello World", true, true, { "A1" }, "entire cell exact A1");
	AssertFindRefs(wSheet, "hello world", true, true, { "A2" }, "entire cell exact A2 match case");
	AssertFindRefs(wSheet, "orld", false, false, { "A1", "A2" }, "partial match inside cells");
	AssertFindRefs(wSheet, "orld", false, true, {}, "partial needle is not entire cell match");
}

void TestSkFindCell::TestFindCellNumbers() {
	m_Api->UndoCellValue("A1", 1234.5);
	m_Api->UndoCellValue("B1", 42);
	m_Api->UndoCellFormat("A1", "format-string :\"#,##0.00\";");
	m_Api->UndoCellValue("C1", 42);
	m_Api->UndoCellFormat("C1", "background-color:#FFFF00;");

	tSheet* wSheet = m_Api->ActiveSheet();
	const tString wDisplay = m_Api->CellFormatString("A1");
	CPPUNIT_ASSERT_MESSAGE("formatted display not empty", !wDisplay.empty());
	CPPUNIT_ASSERT_MESSAGE("formatted display differs from raw string",
		wDisplay != "1234.5");
	CPPUNIT_ASSERT_MESSAGE("formatted display contains digit run 234",
		wDisplay.find("234") != tString::npos);

	AssertFindRefs(wSheet, "234", false, false, { "A1" },
		"find partial on FormatString display");
	AssertFindRefs(wSheet, wDisplay, false, true, { "A1" },
		"find entire on FormatString display");
	AssertFindRefs(wSheet, "42", false, false, { "B1", "C1" },
		"find plain integers");
}

void TestSkFindCell::TestJsonFindCell() {
	SetupFindGrid();

	tString wJson = m_Api->JsonFindCell("hello", false, false);
	CPPUNIT_ASSERT_MESSAGE("JsonFindCell returns object", wJson.find("cells") != tString::npos);
	CPPUNIT_ASSERT_MESSAGE("JsonFindCell A1", JsonContainsRef(wJson, "A1"));
	CPPUNIT_ASSERT_MESSAGE("JsonFindCell B1", JsonContainsRef(wJson, "B1"));
	CPPUNIT_ASSERT_MESSAGE("JsonFindCell excludes B2", !JsonContainsRef(wJson, "B2"));

	wJson = m_Api->JsonFindCell("hello", true, true);
	CPPUNIT_ASSERT_MESSAGE("JsonFindCell entire+match case B1", JsonContainsRef(wJson, "B1"));
	CPPUNIT_ASSERT_MESSAGE("JsonFindCell entire+match case excludes A2", !JsonContainsRef(wJson, "A2"));

	wJson = m_Api->JsonFindCell("missing", false, false);
	CPPUNIT_ASSERT_MESSAGE("JsonFindCell no match", wJson.find("\"cells\":[]") != tString::npos);
}

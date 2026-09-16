// TestSkRazFormat.hpp
//
//  Created on: 20/07/2026
//      Author: Stéphane Allez
//

#ifndef TestSkRazFormat_hpp
#define TestSkRazFormat_hpp

#include <cppunit/extensions/HelperMacros.h>
#include <SkApplication.hpp>
#include <SkApi.hpp>
#include <SkFormatRoot.hpp>
#include <SkFormatCssApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;
using namespace SkFormat;

// Covers UndoRazFormat: clear the CSS format only, keep the cell content
// (value/formula). Content-preservation and format-restore must survive undo/redo.
class TestSkRazFormat : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkRazFormat);
    CPPUNIT_TEST(TestRazFormatValueCell);
    CPPUNIT_TEST(TestRazFormatFormulaCell);
    CPPUNIT_TEST(TestRazFormatRangeKeepsContent);
    CPPUNIT_TEST(TestRazFormatWholeColumn);
    CPPUNIT_TEST(TestRazFormatWholeRow);
    CPPUNIT_TEST(TestRazFormatWholeSheet);
    CPPUNIT_TEST_SUITE_END();
public:
    TestSkRazFormat();
    void setUp();
    void tearDown();
private:
    // A plain value cell keeps its value; only the CSS format is cleared.
    void TestRazFormatValueCell();
    // A formula cell keeps its formula and computed value; only the format is cleared.
    void TestRazFormatFormulaCell();
    // A multi-cell RazFormat keeps every cell's content and clears every format.
    void TestRazFormatRangeKeepsContent();
    // A whole-column RazFormat clears the column-level CSS (tColRow) and undo restores it.
    void TestRazFormatWholeColumn();
    // A whole-row RazFormat clears the row-level CSS (tColRow) and undo restores it.
    void TestRazFormatWholeRow();
    // A whole-sheet RazFormat clears the sheet-level CSS (tSheet) and undo restores it.
    void TestRazFormatWholeSheet();
    tApplication* m_Application;
    tFormatRoot* m_FormatRoot;
    tApi* m_Api;
    // CSS format engine: CellFormat/UndoCellFormat need it to parse/serialize CSS.
    tFormatCssApi* m_FormatApi;
};

#endif

//==============================================================================
// TestSkSpreadSheetFormat Test For SpreadSheet
// le 17/11/2023
//==============================================================================
#ifndef TestSkSpreadSheetFormat_hpp
#define TestSkSpreadSheetFormat_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

#include <SkFormatRoot.hpp>

#include <SkLemonInterface.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;
using namespace SkFormat;


#define TestAllSpreadSheet

class TestSkFormatSpreadSheet : public CPPUNIT_NS::TestFixture {

    // Sequence of tests
    CPPUNIT_TEST_SUITE(TestSkFormatSpreadSheet);
    
#ifndef TestAllSpreadSheet
    //CPPUNIT_TEST(TestSkPressure);
    //CPPUNIT_TEST(TestJsonViewMergedBorders);
    CPPUNIT_TEST(TestSkFormatBorder);
#endif

#ifdef TestAllSpreadSheet
    CPPUNIT_TEST(TestSkFormatCellAlloc);
    CPPUNIT_TEST(TestSkFormatApi);
    CPPUNIT_TEST(TestSkFormatApiText);
    CPPUNIT_TEST(TestSkFormatRazCell);
    CPPUNIT_TEST(TestSkFormatDeleteColRow);
    CPPUNIT_TEST(TestSkFormatInsertRowCol);
    CPPUNIT_TEST(TestSkFormatCopyPaste);
    CPPUNIT_TEST(TestSkFormatRecover);
    CPPUNIT_TEST(TestSkFormatDeleteSheet);
    CPPUNIT_TEST(TestSkFormatApplySheet);
    CPPUNIT_TEST(TestSkFormatApplyCol);
    CPPUNIT_TEST(TestSkFormatApplyRow);
    CPPUNIT_TEST(TestSkFormatBorder);
    CPPUNIT_TEST(TestSkFormatBorderUndoRedo);
    CPPUNIT_TEST(TestSaveWorkBook);
    CPPUNIT_TEST(TestLoadWorkBook);
    CPPUNIT_TEST(TestCopy);
    CPPUNIT_TEST(TestJsonView);
    CPPUNIT_TEST(TestJsonViewMergedBorders);
    CPPUNIT_TEST(TestBorderApplyClearBlockCheckfo);
    CPPUNIT_TEST(TestAdjacentCells);
#ifndef __EMSCRIPTEN__MEMORY__
    CPPUNIT_TEST(TestSkFormatRead);
#endif
    CPPUNIT_TEST(TestSkPressure);
#endif

    CPPUNIT_TEST_SUITE_END();
private:
    tApplication* m_Application; // Pointer of static application
    SkFormat::tFormatRoot* m_FormatRoot;
    SkSpreadSheet::tApi* m_Api;
    
    tVirtualClass* m_FormatApi; // Conflict with tSpreadSheet
    
    // For Fill
    tInt m_NbCol;
    tInt m_NbRow;
    
    void Fill();
    void DrawCell(tString sOperation, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
    
    void ApplyWidth(tFormatCss* sFormatCssPool);
    void ApplyColor(tFormatCss* sFormatCssPool);
    void ApplyBackgroundColor(tFormatCss* sFormatCssPool);
    void ApplyBorder(tFormatCss* sFormatCssPool);
    void ApplyMargin(tFormatCss* sFormatCssPool);
    void ApplyPadding(tFormatCss* sFormatCssPool);
    void ApplyFont(tFormatCss* sFormatCssPool);
    void ApplyText(tFormatCss* sFormatCssPool);
    
    tFormatRef ApplyCellFormat(tString sValue);
public:
    TestSkFormatSpreadSheet();
    
    void Debug();
    
    void TestSkFormatCellAlloc();
    void TestSkFormatApi();
    void TestSkFormatApiText();
    void TestSkFormatRazCell();
    void TestSkFormatDeleteColRow();
    void TestSkFormatInsertRowCol();
    void TestSkFormatCopyPaste();
    void TestSkFormatRecover();
    void TestSkFormatDeleteSheet();

    void TestSkFormatApplySheet();
    void TestSkFormatApplyCol();
    void TestSkFormatApplyRow();
    
    void TestSkFormatBorder();
    void TestSkFormatBorderUndoRedo();
    void TestSkFormatRead();
    
    void TestSaveWorkBook();
    void TestLoadWorkBook();
    
    void TestJsonView();
    void TestJsonViewMergedBorders();
    void TestBorderApplyClearBlockCheckfo();
    
    void TestAdjacentCells();
    
    void TestCopy();
    
    void TestSkPressure();
    
private:
    void UndoOperation();
    void RedoOperation();
public:
    void setUp();
    void tearDown();
};


#endif    /* TestSkSpreadSheetFormat */


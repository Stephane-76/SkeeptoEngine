// TestSkRazFormat.cpp
//
//  Created on: 20/07/2026
//      Author: Stéphane Allez
//

#include "../include/TestSkRazFormat.hpp"

#include <SkSheet.hpp>
#include <SkColRow.hpp>
#include <SkWorkBook.hpp>
#include <SkTools.hpp>

#include <filesystem>

TestSkRazFormat::TestSkRazFormat() : CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
    m_Application = nullptr;
    m_FormatRoot = nullptr;
    m_Api = nullptr;
    m_FormatApi = nullptr;
}

// True when sCss declares the given "property:value" token.
static tBool HasCss(const tString& sCss, const tString& sToken) {
    return sCss.find(sToken) != tString::npos;
}

// Column-level CSS (tColRow) as a string, "" when none. Column A == index 1.
static tString ColFormat(tApi* sApi, tIndex sCol) {
    tSheet* wSheet = sApi->ActiveSheet();
    tColRow* wColRow = wSheet->Col(sCol);
    if (wColRow == nullptr || wColRow->Css() == 0) return "";
    return wSheet->WorkBook()->CellFormat(wColRow->Css());
}

// Row-level CSS (tColRow) as a string, "" when none. Row 1 == index 1.
static tString RowFormat(tApi* sApi, tIndex sRow) {
    tSheet* wSheet = sApi->ActiveSheet();
    tColRow* wColRow = wSheet->Row(sRow);
    if (wColRow == nullptr || wColRow->Css() == 0) return "";
    return wSheet->WorkBook()->CellFormat(wColRow->Css());
}

// Sheet-level CSS (tSheet) as a string, "" when none.
static tString SheetFormat(tApi* sApi) {
    tSheet* wSheet = sApi->ActiveSheet();
    if (wSheet->Css() == 0) return "";
    return wSheet->WorkBook()->CellFormat(wSheet->Css());
}

void TestSkRazFormat::TestRazFormatValueCell() {
    // Format first, then value: this is the order the app uses (format a cell,
    // then type into it) and the one the engine round-trips reliably.
    m_Api->UndoCellFormat("A1", "color:red;background-color:black;");
    m_Api->UndoCellValue("A1", 42);

    CPPUNIT_ASSERT_MESSAGE("Initial A1=42", m_Api->CellValue("A1").Int() == 42);
    CPPUNIT_ASSERT_MESSAGE("Initial A1 color", HasCss(m_Api->CellFormat("A1"), "color:red"));
    CPPUNIT_ASSERT_MESSAGE("Initial A1 background", HasCss(m_Api->CellFormat("A1"), "background-color:black"));

    // Clear format only: value stays, our CSS is removed.
    m_Api->UndoRazFormat("A1");
    CPPUNIT_ASSERT_MESSAGE("RazFormat keeps A1=42", m_Api->CellValue("A1").Int() == 42);
    CPPUNIT_ASSERT_MESSAGE("RazFormat clears A1 color", !HasCss(m_Api->CellFormat("A1"), "color:red"));
    CPPUNIT_ASSERT_MESSAGE("RazFormat clears A1 background", !HasCss(m_Api->CellFormat("A1"), "background-color:black"));

    // Undo restores the format without touching the value.
    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Undo keeps A1=42", m_Api->CellValue("A1").Int() == 42);
    CPPUNIT_ASSERT_MESSAGE("Undo restores A1 color", HasCss(m_Api->CellFormat("A1"), "color:red"));
    CPPUNIT_ASSERT_MESSAGE("Undo restores A1 background", HasCss(m_Api->CellFormat("A1"), "background-color:black"));

    // Redo clears the format again, value still intact.
    m_Api->Redo();
    CPPUNIT_ASSERT_MESSAGE("Redo keeps A1=42", m_Api->CellValue("A1").Int() == 42);
    CPPUNIT_ASSERT_MESSAGE("Redo clears A1 color", !HasCss(m_Api->CellFormat("A1"), "color:red"));
}

void TestSkRazFormat::TestRazFormatFormulaCell() {
    m_Api->UndoCellValue("A1", 10);
    m_Api->UndoCellFormat("A2", "background-color:yellow;");
    m_Api->UndoCellValue("A2", "=A1*2");

    CPPUNIT_ASSERT_MESSAGE("Initial A2=20", m_Api->CellValue("A2").Int() == 20);
    CPPUNIT_ASSERT_MESSAGE("Initial A2 has format", HasCss(m_Api->CellFormat("A2"), "background-color:yellow"));

    // Clear format only: the formula and its computed value survive.
    m_Api->UndoRazFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("RazFormat keeps A2=20", m_Api->CellValue("A2").Int() == 20);
    CPPUNIT_ASSERT_MESSAGE("RazFormat clears A2 format", !HasCss(m_Api->CellFormat("A2"), "background-color:yellow"));

    // The formula still recalculates after clearing its format.
    m_Api->UndoCellValue("A1", 30);
    CPPUNIT_ASSERT_MESSAGE("A2 recalculates to 60", m_Api->CellValue("A2").Int() == 60);

    // Undo the source change, then undo the RazFormat to restore the format.
    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Undo source A2=20", m_Api->CellValue("A2").Int() == 20);
    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Undo RazFormat keeps A2=20", m_Api->CellValue("A2").Int() == 20);
    CPPUNIT_ASSERT_MESSAGE("Undo restores A2 format", HasCss(m_Api->CellFormat("A2"), "background-color:yellow"));
}

void TestSkRazFormat::TestRazFormatRangeKeepsContent() {
    m_Api->UndoCellFormat("A1:C1", "color:white;background-color:blue;");
    m_Api->UndoCellValue("A1", 1);
    m_Api->UndoCellValue("B1", 2);
    m_Api->UndoCellValue("C1", 3);

    CPPUNIT_ASSERT_MESSAGE("A1 has format", HasCss(m_Api->CellFormat("A1"), "background-color:blue"));
    CPPUNIT_ASSERT_MESSAGE("C1 has format", HasCss(m_Api->CellFormat("C1"), "background-color:blue"));

    m_Api->UndoRazFormat("A1:C1");

    CPPUNIT_ASSERT_MESSAGE("Range keeps A1=1", m_Api->CellValue("A1").Int() == 1);
    CPPUNIT_ASSERT_MESSAGE("Range keeps B1=2", m_Api->CellValue("B1").Int() == 2);
    CPPUNIT_ASSERT_MESSAGE("Range keeps C1=3", m_Api->CellValue("C1").Int() == 3);
    CPPUNIT_ASSERT_MESSAGE("Range clears A1 format", !HasCss(m_Api->CellFormat("A1"), "background-color:blue"));
    CPPUNIT_ASSERT_MESSAGE("Range clears B1 format", !HasCss(m_Api->CellFormat("B1"), "background-color:blue"));
    CPPUNIT_ASSERT_MESSAGE("Range clears C1 format", !HasCss(m_Api->CellFormat("C1"), "background-color:blue"));

    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Undo keeps B1=2", m_Api->CellValue("B1").Int() == 2);
    CPPUNIT_ASSERT_MESSAGE("Undo restores A1 format", HasCss(m_Api->CellFormat("A1"), "background-color:blue"));
    CPPUNIT_ASSERT_MESSAGE("Undo restores C1 format", HasCss(m_Api->CellFormat("C1"), "background-color:blue"));
}

void TestSkRazFormat::TestRazFormatWholeColumn() {
    // Whole-column format is stored at the tColRow level, not on cells.
    tRect wCol = m_Api->GetColSelect(1, 1); // column A
    m_Api->UndoCellFormat(wCol, "background-color:blue;");
    CPPUNIT_ASSERT_MESSAGE("Col A has format", HasCss(ColFormat(m_Api, 1), "background-color:blue"));

    m_Api->UndoRazFormat(wCol.StrRef());
    CPPUNIT_ASSERT_MESSAGE("RazFormat clears col A format", ColFormat(m_Api, 1).empty());

    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Undo restores col A format", HasCss(ColFormat(m_Api, 1), "background-color:blue"));

    m_Api->Redo();
    CPPUNIT_ASSERT_MESSAGE("Redo clears col A format", ColFormat(m_Api, 1).empty());
}

void TestSkRazFormat::TestRazFormatWholeRow() {
    // Whole-row format is stored at the tColRow level, not on cells.
    tRect wRow = m_Api->GetRowSelect(1, 1); // row 1
    m_Api->UndoCellFormat(wRow, "background-color:green;");
    CPPUNIT_ASSERT_MESSAGE("Row 1 has format", HasCss(RowFormat(m_Api, 1), "background-color:green"));

    m_Api->UndoRazFormat(wRow.StrRef());
    CPPUNIT_ASSERT_MESSAGE("RazFormat clears row 1 format", RowFormat(m_Api, 1).empty());

    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Undo restores row 1 format", HasCss(RowFormat(m_Api, 1), "background-color:green"));

    m_Api->Redo();
    CPPUNIT_ASSERT_MESSAGE("Redo clears row 1 format", RowFormat(m_Api, 1).empty());
}

void TestSkRazFormat::TestRazFormatWholeSheet() {
    // Whole-sheet format is stored at the tSheet level, not on cells.
    tRect wSheetRect = m_Api->GetSheetSelect();
    m_Api->UndoCellFormat(wSheetRect, "background-color:red;");
    CPPUNIT_ASSERT_MESSAGE("Sheet has format", HasCss(SheetFormat(m_Api), "background-color:red"));

    m_Api->UndoRazFormat(wSheetRect.StrRef());
    CPPUNIT_ASSERT_MESSAGE("RazFormat clears sheet format", SheetFormat(m_Api).empty());

    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Undo restores sheet format", HasCss(SheetFormat(m_Api), "background-color:red"));

    m_Api->Redo();
    CPPUNIT_ASSERT_MESSAGE("Redo clears sheet format", SheetFormat(m_Api).empty());
}

void TestSkRazFormat::setUp() {
    std::filesystem::remove_all("./Spreadsheet");
    m_Application = tApplication::Instance();
    m_FormatRoot = tFormatRoot::Instance();
    m_Api = new tApi;
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
    // Attach the CSS engine so UndoCellFormat/CellFormat parse and serialize CSS.
    m_FormatApi = new tFormatCssApi();
    m_Api->FormatApi((tFormatApi*)m_FormatApi);
}

void TestSkRazFormat::tearDown() {
    delete(m_Api);
    m_Api = nullptr;
    delete(m_FormatApi);
    m_FormatApi = nullptr;
    DoneFormatRoot();
}

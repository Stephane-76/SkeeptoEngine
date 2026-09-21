//=============================================================================
// TestSkInsertRowCol.cpp
// Stéphane Allez *
//  Created on: 15 juil. 2026
//=============================================================================

#include "../include/TestSkInsertRowCol.hpp"
#include "../include/TestSkFormatTeardownHelpers.hpp"
#include <SkFormatApi.hpp>

#include <SkFormatCssApi.hpp>

TestSkInsertRowCol::TestSkInsertRowCol()
    : CPPUNIT_NS::TestFixture(), m_Application(nullptr), m_FormatRoot(nullptr),
      m_FormatApi(nullptr), m_Api(nullptr) {}


void TestSkInsertRowCol::TestInsertColRow() {
    const tString wBgCss = "background-color:#D0E0E3;";

    CPPUNIT_ASSERT_MESSAGE("format background block", m_Api->UndoCellFormat("A1:C12", wBgCss));
    CPPUNIT_ASSERT_MESSAGE("format outside border",
        m_Api->UndoCellBorder("A1:C12", tBorderOutside, "solid 1px black;"));

    CPPUNIT_ASSERT_MESSAGE("insert col after right edge", m_Api->UndoInsertCol(4, 1));

#ifdef checkfo
    m_Api->CheckFormat();
#endif

    CPPUNIT_ASSERT_MESSAGE("undo insert col", m_Api->Undo());

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkInsertRowCol::TestInsertColRowRect() {
    const tString wBgCss = "background-color:#D0E0E3;";

    CPPUNIT_ASSERT_MESSAGE("format background block", m_Api->UndoCellFormat("A1:C12", wBgCss));
    CPPUNIT_ASSERT_MESSAGE("format outside border",
        m_Api->UndoCellBorder("B2:E12", tBorderOutside, "solid 1px black;"));

    tString wJsonSave = m_Api->WriteJson("wwww.skeema.fr/w1");
    CPPUNIT_ASSERT_MESSAGE("json save non-empty", !wJsonSave.empty());

    // No combined col+row rect API: insert columns then rows over the same rect.
    const tRect wInsertRect(2, 3, 10, 10);
    CPPUNIT_ASSERT_MESSAGE("insert col by rect", m_Api->UndoInsertColByRect(wInsertRect));
    CPPUNIT_ASSERT_MESSAGE("insert row by rect", m_Api->UndoInsertRowByRect(wInsertRect));

    CPPUNIT_ASSERT_MESSAGE("undo insert col by rect", m_Api->Undo());
    CPPUNIT_ASSERT_MESSAGE("undo insert row by rect", m_Api->Undo());

    tString wJsonLoad = m_Api->WriteJson("wwww.skeema.fr/w1");
    CPPUNIT_ASSERT_MESSAGE("json load matches save", wJsonSave == wJsonLoad);


    CPPUNIT_ASSERT_MESSAGE("redo insert col by rect", m_Api->Redo());
    CPPUNIT_ASSERT_MESSAGE("redo insert row by rect", m_Api->Redo());

    // Redo is only possible after Undo, so unwind both inserts before redoing them.
    CPPUNIT_ASSERT_MESSAGE("undo insert row by rect", m_Api->Undo());
    CPPUNIT_ASSERT_MESSAGE("undo insert col by rect", m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE("redo insert col by rect", m_Api->Redo());
    CPPUNIT_ASSERT_MESSAGE("redo insert row by rect", m_Api->Redo());

    CPPUNIT_ASSERT_MESSAGE("undo insert row by rect", m_Api->Undo());
    CPPUNIT_ASSERT_MESSAGE("undo insert col by rect", m_Api->Undo());
    
    wJsonLoad = m_Api->WriteJson("wwww.skeema.fr/w1");
    CPPUNIT_ASSERT_MESSAGE("json load matches save", wJsonSave == wJsonLoad);

}

void TestSkInsertRowCol::setUp() {
    m_Application = tApplication::Instance();
    m_FormatRoot = tFormatRoot::Instance();
    m_Api = new tApi();
    m_Api->IsUndoActif(true);
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
    m_FormatApi = new tFormatCssApi();
    m_Api->FormatApi(m_FormatApi);
}

void TestSkInsertRowCol::tearDown() {
    delete(m_Api);
    m_Api = nullptr;
    TestSkFormatTeardown::AssertFormatPoolEmptyAfterApiDelete(m_FormatApi, "TestSkInsertRowCol");
    delete(m_FormatApi);
    m_FormatApi = nullptr;

    DoneFormatRoot();
 }

// TestSkRaz.cpp
//
//  Created on: 28/11/2025
//      Author: Stéphane Allez
//

#include "../include/TestSkRaz.hpp"

TestSkRaz::TestSkRaz() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
    m_Application = nullptr;
    m_Api = nullptr;
}   

void TestSkRaz::TestUndoRaz() {
    m_Api->UndoCellValue("A1", 1);
    m_Api->UndoCellValue("A2", 2);
    m_Api->UndoCellValue("B1", 3);
    m_Api->UndoCellValue("B2", 4);
    m_Api->UndoCellValue("C1", 5);
    m_Api->UndoCellValue("C2", 6);
    m_Api->UndoCellValue("D1", 7);
    m_Api->UndoCellValue("D2", 8);
    m_Api->UndoCellValue("A3", "=SUM(A1:A2)");
    m_Api->UndoCellValue("B3", "=SUM(B1:B2)");
    m_Api->UndoCellValue("C3", "=SUM(C1:C2)");
    m_Api->UndoCellValue("D3", "=SUM(D1:D2)");
    
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz A3=3", m_Api->CellValue("A3").Int() == 3);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz B3=7", m_Api->CellValue("B3").Int() == 7);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz C3=11", m_Api->CellValue("C3").Int() == 11);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz D3=15", m_Api->CellValue("D3").Int() == 15);

    m_Api->UndoRaz("A1:D1");
    
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz A3=3", m_Api->CellValue("A3").Int() == 2);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz B3=7", m_Api->CellValue("B3").Int() == 4);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz C3=11", m_Api->CellValue("C3").Int() == 6);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz D3=15", m_Api->CellValue("D3").Int() == 8);
    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz A3=3", m_Api->CellValue("A3").Int() == 3);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz B3=7", m_Api->CellValue("B3").Int() == 7);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz C3=11", m_Api->CellValue("C3").Int() == 11);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz D3=15", m_Api->CellValue("D3").Int() == 15);

    m_Api->Redo();
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz A3=3", m_Api->CellValue("A3").Int() == 2);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz B3=7", m_Api->CellValue("B3").Int() == 4);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz C3=11", m_Api->CellValue("C3").Int() == 6);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz D3=15", m_Api->CellValue("D3").Int() == 8);

    
    m_Api->Undo();
    
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz A3=3", m_Api->CellValue("A3").Int() == 3);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz B3=7", m_Api->CellValue("B3").Int() == 7);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz C3=11", m_Api->CellValue("C3").Int() == 11);
    CPPUNIT_ASSERT_MESSAGE("TestSkRaz D3=15", m_Api->CellValue("D3").Int() == 15);

}

void TestSkRaz::TestUndoRazFormulaCell() {
    // Layout:
    //   A1=10, A2=20
    //   A3 = SUM(A1:A2)      -> 30     (this is "a cell with a function")
    //   A4 = A3+1             -> 31
    //   A5 = A4*2             -> 62
    // After Raz on A3 (delete a function cell):
    //   A3 becomes empty     -> value empty / 0
    //   A4 must recalc       -> 0+1 = 1
    //   A5 must recalc       -> 1*2 = 2
    m_Api->UndoCellValue("A1", 10);
    m_Api->UndoCellValue("A2", 20);
    m_Api->UndoCellValue("A3", "=SUM(A1:A2)");
    m_Api->UndoCellValue("A4", "=A3+1");
    m_Api->UndoCellValue("A5", "=A4*2");

    CPPUNIT_ASSERT_MESSAGE("Initial A3=30", m_Api->CellValue("A3").Int() == 30);
    CPPUNIT_ASSERT_MESSAGE("Initial A4=31", m_Api->CellValue("A4").Int() == 31);
    CPPUNIT_ASSERT_MESSAGE("Initial A5=62", m_Api->CellValue("A5").Int() == 62);

    m_Api->UndoRaz("A3");

    CPPUNIT_ASSERT_MESSAGE("After Raz A3 empty",
        m_Api->CellValue("A3").Type() == tVariantType::t_null);
    CPPUNIT_ASSERT_MESSAGE("After Raz A4=1", m_Api->CellValue("A4").Int() == 1);
    CPPUNIT_ASSERT_MESSAGE("After Raz A5=2", m_Api->CellValue("A5").Int() == 2);

    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Undo restore A3=30", m_Api->CellValue("A3").Int() == 30);
    CPPUNIT_ASSERT_MESSAGE("Undo restore A4=31", m_Api->CellValue("A4").Int() == 31);
    CPPUNIT_ASSERT_MESSAGE("Undo restore A5=62", m_Api->CellValue("A5").Int() == 62);

    m_Api->Redo();
    CPPUNIT_ASSERT_MESSAGE("Redo A3 empty",
        m_Api->CellValue("A3").Type() == tVariantType::t_null);
    CPPUNIT_ASSERT_MESSAGE("Redo A4=1", m_Api->CellValue("A4").Int() == 1);
    CPPUNIT_ASSERT_MESSAGE("Redo A5=2", m_Api->CellValue("A5").Int() == 2);
}

void TestSkRaz::TestUndoRazErrorCell() {
    // Layout:
    //   A1=0, A2=10
    //   B1 = A2/A1            -> #DIV/0!  (this is "a cell with an error")
    //   B2 = B1+5             -> propagates error
    // After Raz on B1 (delete an error cell):
    //   B1 becomes empty
    //   B2 must recalc        -> 0+5 = 5 (error should disappear)
    m_Api->UndoCellValue("A1", 0);
    m_Api->UndoCellValue("A2", 10);
    m_Api->UndoCellValue("B1", "=A2/A1");
    m_Api->UndoCellValue("B2", "=B1+5");

    CPPUNIT_ASSERT_MESSAGE("Initial B1 is error",
        m_Api->CellValue("B1").Type() == tVariantType::t_error);
    CPPUNIT_ASSERT_MESSAGE("Initial B2 is error",
        m_Api->CellValue("B2").Type() == tVariantType::t_error);

    m_Api->UndoRaz("B1");

    CPPUNIT_ASSERT_MESSAGE("After Raz B1 empty",
        m_Api->CellValue("B1").Type() == tVariantType::t_null);
    // After deletion, the stale error must be cleared from dependents.
    CPPUNIT_ASSERT_MESSAGE("After Raz B2 no longer error",
        m_Api->CellValue("B2").Type() != tVariantType::t_error);
    CPPUNIT_ASSERT_MESSAGE("After Raz B2=5", m_Api->CellValue("B2").Int() == 5);
}

void TestSkRaz::TestUndoRazValueCellInsideRange() {
    // Reproduces the user-reported case:
    //   G9=12, G10=23 (both plain values)
    //   G12 = SUM(G9:G10)  -> 35
    // Then a single-cell Raz on G10 must recalc G12 to 12.
    // Previously, AddSelect's tTempoPoint branch returned early when the
    // cell was deleted, so dependents referencing the range G9:G10 were
    // never added to the path.
    m_Api->UndoCellValue("G9", 12);
    m_Api->UndoCellValue("G10", 23);
    m_Api->UndoCellValue("G12", "=SUM(G9:G10)");

    CPPUNIT_ASSERT_MESSAGE("Initial G12=35", m_Api->CellValue("G12").Int() == 35);

    m_Api->UndoRaz("G10");

    CPPUNIT_ASSERT_MESSAGE("After Raz G10 empty",
        m_Api->CellValue("G10").Type() == tVariantType::t_null);
    CPPUNIT_ASSERT_MESSAGE("After Raz G12=12", m_Api->CellValue("G12").Int() == 12);

    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Undo G10=23", m_Api->CellValue("G10").Int() == 23);
    CPPUNIT_ASSERT_MESSAGE("Undo G12=35", m_Api->CellValue("G12").Int() == 35);

    m_Api->Redo();
    CPPUNIT_ASSERT_MESSAGE("Redo G10 empty",
        m_Api->CellValue("G10").Type() == tVariantType::t_null);
    CPPUNIT_ASSERT_MESSAGE("Redo G12=12", m_Api->CellValue("G12").Int() == 12);
}

void TestSkRaz::setUp() {
    std::filesystem::remove_all("./Spreadsheet");
    m_Application = tApplication::Instance();
    m_Api = new tApi;
    m_Api->IsUndoActif(true);
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
}

void TestSkRaz::tearDown() {
    delete(m_Api);
}

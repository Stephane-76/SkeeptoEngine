//=============================================================================  
// TestSkRangeInsertDelete
// author : Stéphane Allez
//=============================================================================  

#include "../include/TestSkInsertDeleteColRowByRect.hpp"
#include "../include/SkSpreadSheet.hpp"

// We can send it to the API of a feature 
TestSkInsertDeleteColRowByRect::TestSkInsertDeleteColRowByRect() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
    m_NbCol = 5;
    m_NbRow = 7;
}

void TestSkInsertDeleteColRowByRect::DrawCell(tString sTitle, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
    (void)sTitle;
    (void)sRowBegin;
    (void)sColBegin;
    (void)sRowEnd;
    (void)sColEnd;
}

void TestSkInsertDeleteColRowByRect::DebugRange() {
    return; // Drop
    cout << "Debug Range" << endl;
    tVectorRange* wVectorRange; // = m_Api->ActiveSheet()->ColRowCellRange()->VectorRange();
    for (auto wRange : *wVectorRange) {
        cout << wRange->Debug();
    }
}

void TestSkInsertDeleteColRowByRect::Fill() {
    for (tInt wRow = 1; wRow <= m_NbRow; wRow++) {
        for (tInt wCol =1; wCol <= m_NbCol; wCol++) {
            tVariant wVariant = wRow * 10 + wCol;
            m_Api->CellValue(wRow, wCol, wVariant);
        }
    }
}


void TestSkInsertDeleteColRowByRect::FillWithSum() {
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
    DrawCell("Fill Sheet1", 1, 1, 8, 10);
}

tBool TestSkInsertDeleteColRowByRect::TestValue(tString sRef,tVariant sValue) {
    tCell* wCell=m_Api->Cell(sRef);
    if (wCell==nullptr) return(false);
    
    return(wCell->Value()==sValue);
}

void TestSkInsertDeleteColRowByRect::TestInsertRowByRect() {
    Fill();
  
    //m_Api->UndoCellValue("B6","12");
    m_Api->UndoCellValue("B4","=SUM(B2:B3)");
    
    m_Api->UndoCellValue("B7","=SUM(B1:B6)");
    
    m_Api->UndoCellValue("D1","=B2");

    
    tRect wRect(2, 2, 4, 3);
    DrawCell("Before Row Insert"+wRect.StrRef(), 1, 1, 10, 5);
   
    CPPUNIT_ASSERT_MESSAGE("Row Insert B7",TestValue("B7",tVariant(234)));
    CPPUNIT_ASSERT_MESSAGE("Row Insert B4",TestValue("B4",tVariant(54)));
 
    m_Api->UndoInsertRowByRect(wRect);
    DrawCell("AFter Insert "+wRect.StrRef(), 1, 1, 10, 5);
    CPPUNIT_ASSERT_MESSAGE("Row Insert B10",TestValue("B10",tVariant(234)));
    CPPUNIT_ASSERT_MESSAGE("Row Insert B7",TestValue("B7",tVariant(54)));
    
    m_Api->Undo();
    DrawCell("After Undo "+wRect.StrRef(), 1, 1, 10, 5);
    CPPUNIT_ASSERT_MESSAGE("Row Insert B7",TestValue("B7",tVariant(234)));
    CPPUNIT_ASSERT_MESSAGE("Row Insert B4",TestValue("B4",tVariant(54)));
    
    m_Api->UndoDeleteRowByRect(wRect);
    DrawCell("After Delete "+wRect.StrRef(), 1, 1, 10, 5);
    CPPUNIT_ASSERT_MESSAGE("Row Insert B4",TestValue("B4",tVariant(126)));
 
    m_Api->Undo();
    DrawCell("After Undo Delete "+wRect.StrRef(), 1, 1, 10, 5);
    CPPUNIT_ASSERT_MESSAGE("Row Insert B7",TestValue("B7",tVariant(234)));
    CPPUNIT_ASSERT_MESSAGE("Row Insert B4",TestValue("B4",tVariant(54)));
   
    m_Api->UndoDeleteRowByRect(wRect);
    DrawCell("After  Delete * 1 "+wRect.StrRef(), 1, 1, 10, 5);
    CPPUNIT_ASSERT_MESSAGE("Row Delete B4",TestValue("B4",tVariant(126)));
 
    m_Api->UndoDeleteRowByRect(wRect);
    DrawCell("After  Delete Row "+wRect.StrRef(), 1, 1, 10, 5);
    tVariant wVariantError=tVariant(tClassError(tTypeError::t_ref, "#REF!"));
    CPPUNIT_ASSERT_MESSAGE("Row Delete D1",TestValue("D1",wVariantError));
    
    m_Api->Undo();
    DrawCell("After Undo Delete Row "+wRect.StrRef(), 1, 1, 10, 5);
    CPPUNIT_ASSERT_MESSAGE("Row Delete B4",TestValue("B4",tVariant(126)));
    
    m_Api->Undo();
    DrawCell("After Undo Delete Row "+wRect.StrRef(), 1, 1, 10, 5);
    CPPUNIT_ASSERT_MESSAGE("Row Insert B7",TestValue("B7",tVariant(234)));
    CPPUNIT_ASSERT_MESSAGE("Row Insert B4",TestValue("B4",tVariant(54)));
}

void TestSkInsertDeleteColRowByRect::TestInsertColByRect() {   
    Fill();
    
    m_Api->UndoCellValue("B4","=SUM(B2:B3)");
    
    m_Api->UndoCellValue("B7","=SUM(B1:B6)");
    
    m_Api->UndoCellValue("D1","=B2");
    
    tRect wRect(2, 2, 4, 3);
    DrawCell("Before Insert Col "+wRect.StrRef(), 1, 1, 10, 5);
    CPPUNIT_ASSERT_MESSAGE("Row Insert B7",TestValue("B7",tVariant(234)));
    CPPUNIT_ASSERT_MESSAGE("Row Insert B4",TestValue("B4",tVariant(54)));
  
   
    //m_Api->UndoInsertColByRect(wRect);
    m_Api->UndoInsertColByRect(wRect);
    DrawCell("After Insert Col "+wRect.StrRef(), 1, 1, 10, 7);
    CPPUNIT_ASSERT_MESSAGE("Row Insert B7",TestValue("B7",tVariant(234)));
    CPPUNIT_ASSERT_MESSAGE("Row Insert D4",TestValue("D4",tVariant(54)));
    
    m_Api->Undo();
    DrawCell("After Undo Col "+wRect.StrRef(), 1, 1, 10, 5);
    CPPUNIT_ASSERT_MESSAGE("Row Insert B7",TestValue("B7",tVariant(234)));
    CPPUNIT_ASSERT_MESSAGE("Row Insert B4",TestValue("B4",tVariant(54)));
 
    m_Api->UndoDeleteColByRect(wRect);
    DrawCell("After Delete Col "+wRect.StrRef(), 1, 1, 10, 5);
    tCell* wCellB7=m_Api->Cell("B7");
    //cout << wCellB7->FormulaStr() << endl;
    
    CPPUNIT_ASSERT_MESSAGE("Row Delete B7",wCellB7->FormulaStr()=="SUM(B1:B6)");
    CPPUNIT_ASSERT_MESSAGE("Row Delete B4",TestValue("B4",tVariant(44)));
 
    m_Api->Undo();
    DrawCell("After Undo Delete Col "+wRect.StrRef(), 1, 1, 10, 5);
    CPPUNIT_ASSERT_MESSAGE("Row Insert B7",TestValue("B7",tVariant(234)));
    CPPUNIT_ASSERT_MESSAGE("Row Insert B4",TestValue("B4",tVariant(54)));

    m_Api->UndoDeleteColByRect(wRect);
    DrawCell("After Delete Col"+wRect.StrRef(), 1, 1, 10, 5);
    wCellB7=m_Api->Cell("B7");
    CPPUNIT_ASSERT_MESSAGE("Row Delete B7",wCellB7->FormulaStr()=="SUM(B1:B6)");
    CPPUNIT_ASSERT_MESSAGE("Row Delete B4",TestValue("B4",tVariant(44)));
    
    m_Api->UndoDeleteColByRect(wRect);
    DrawCell("After Delete Col "+wRect.StrRef(), 1, 1, 10, 5);
    wCellB7=m_Api->Cell("B7");
    CPPUNIT_ASSERT_MESSAGE("Row Delete B7",wCellB7->FormulaStr()=="SUM(B1:B6)");
    
    m_Api->Undo();
    m_Api->Undo();
    DrawCell("After Undo Delete Col * 2  "+wRect.StrRef(), 1, 1, 10, 5);
    
    CPPUNIT_ASSERT_MESSAGE("Row Insert B7",TestValue("B7",tVariant(234)));
    CPPUNIT_ASSERT_MESSAGE("Row Insert B4",TestValue("B4",tVariant(54)));
}

void TestSkInsertDeleteColRowByRect::TestUndoInsertRowByRect() {
    FillWithSum();
    const tInt wNbDelete=7;
    tRect wRect(2, 2, 4, 3);
    DrawCell("Before Undo Delete Row ByRect "+wRect.StrRef(), 1, 1, m_NbRow, m_NbCol);
  
    for (tInt wRow = 1; wRow <= wNbDelete; wRow++) {
      m_Api->UndoInsertRowByRect(wRect);
    }
    DrawCell("After  Delete Row by Rect "+wRect.StrRef(), 1, 1, m_NbRow, m_NbCol);
    for (tInt wRow = 1; wRow <= wNbDelete; wRow++) {
        m_Api->Undo();
    }
    DrawCell("After  Undo Delete Row byRect "+wRect.StrRef(), 1, 1, m_NbRow, m_NbCol);
    for(tIndex wRow =1; wRow<=wNbDelete;wRow++) {
        m_Api->Redo();
    }
    for(tIndex wIndRow =1; wIndRow<=wNbDelete;wIndRow++) {
        m_Api->Undo();
    }
    DrawCell("After Undo Delete Row by Rect "+wRect.StrRef(), 1, 1, m_NbRow, m_NbCol);
 
    CPPUNIT_ASSERT_MESSAGE("Row Insert E7",m_Api->Cell("E7")->Value().Int()==276);
}

void TestSkInsertDeleteColRowByRect::TestUndoInsertColByRect() {
    FillWithSum();
    tRect wRect(2, 2, 4, 3);
    DrawCell("Before Undo Delete Col "+wRect.StrRef(), 1, 1, m_NbRow, m_NbCol);
    for (tInt wCol = 1; wCol <= m_NbCol; wCol++) {
      m_Api->UndoInsertColByRect(wRect);
    }
    DrawCell("After Undo Delete Col "+wRect.StrRef(), 1, 1, m_NbRow, m_NbCol);
    for (tInt wCol = 1; wCol <= m_NbCol; wCol++) {
        m_Api->Undo();
    }
    for(tIndex wIndCol =1; wIndCol<=m_NbCol;wIndCol++) {
        m_Api->Redo();
    }
    for(tIndex wIndCol =1; wIndCol<=m_NbCol;wIndCol++) {
        m_Api->Undo();
    }
    DrawCell("After Undo Delete Col "+wRect.StrRef(), 1, 1, m_NbRow, m_NbCol);
    
    CPPUNIT_ASSERT_MESSAGE("Row Insert E7",m_Api->Cell("E7")->Value().Int()==276);
    
}

void TestSkInsertDeleteColRowByRect::TestUndoDeleteRowByRect() {
    FillWithSum();
    tRect wRect(2, 2, 4, 3);
    DrawCell("Before Undo Delete Row "+wRect.StrRef(), 1, 1, m_NbRow, m_NbCol);
    for (tInt wRow = 1; wRow <= m_NbRow; wRow++) {
      m_Api->UndoDeleteRowByRect(wRect);
    }
    DrawCell("After Undo Delete Row "+wRect.StrRef(), 1, 1, m_NbRow, m_NbCol);
    for (tInt wRow = 1; wRow <= m_NbRow; wRow++) {
        m_Api->Undo();
    }
    for(tIndex wIndRow =1; wIndRow<=m_NbRow;wIndRow++) {
        m_Api->Redo();
    }
    for(tIndex wIndRow =1; wIndRow<=m_NbRow;wIndRow++) {
        m_Api->Undo();
    }
    DrawCell("After Undo Delete Row "+wRect.StrRef(), 1, 1, m_NbRow, m_NbCol);
    CPPUNIT_ASSERT_MESSAGE("Row Insert E7",m_Api->Cell("E7")->Value().Int()==276);
}

void TestSkInsertDeleteColRowByRect::TestUndoDeleteColByRect() {
    FillWithSum();
    tRect wRect(2, 2, 4, 3);
    DrawCell("Before Undo Delete Col "+wRect.StrRef(), 1, 1, m_NbRow, m_NbCol);
    for (tInt wCol = 1; wCol <= m_NbCol; wCol++) {
      m_Api->UndoDeleteColByRect(wRect);
    }
    DrawCell("After Undo Delete Col "+wRect.StrRef(), 1, 1, m_NbRow, m_NbCol);
 
    for (tInt wCol = 1; wCol <= m_NbCol; wCol++) {
        m_Api->Undo();
    }
    for(tIndex wIndCol =1; wIndCol<=m_NbCol;wIndCol++) {
        m_Api->Redo();
    }
    for(tIndex wIndCol =1; wIndCol<=m_NbCol;wIndCol++) {
        m_Api->Undo();
    }
    DrawCell("After Undo Delete Col "+wRect.StrRef(), 1, 1, m_NbRow, m_NbCol);
    CPPUNIT_ASSERT_MESSAGE("Row Insert E7",m_Api->Cell("E7")->Value().Int()==276);
}

tString TestSkInsertDeleteColRowByRect::SnapshotCell(tIndex sRow, tIndex sCol) const {
    tCell* wCell = m_Api->Cell(sRow, sCol);
    if (wCell == nullptr) {
        return "<null>";
    }
    tStringStream wStream;
    wStream << wCell->FormulaStr() << "|" << wCell->Value();
    return wStream.str();
}

tString TestSkInsertDeleteColRowByRect::SnapshotRect(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight) const {
    tStringStream wStream;
    for (tIndex wRow = sTop; wRow <= sBottom; ++wRow) {
        for (tIndex wCol = sLeft; wCol <= sRight; ++wCol) {
            wStream << Base10ToAlpha(wCol) << wRow << "=" << SnapshotCell(wRow, wCol) << ";";
        }
        wStream << '\n';
    }
    return wStream.str();
}

void TestSkInsertDeleteColRowByRect::AssertGridMatchesSnapshot(const tString& sExpected,
                                                               tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight,
                                                               const tString& sMessage) const {
    const tString wActual = SnapshotRect(sTop, sLeft, sBottom, sRight);
    CPPUNIT_ASSERT_MESSAGE(sMessage + ":\nexpected:\n" + sExpected + "\nactual:\n" + wActual, wActual == sExpected);
}

void TestSkInsertDeleteColRowByRect::TestUndoRedoGridSnapshotByRect() {
    FillWithSum();
    m_Api->UndoCellValue("B4", "=SUM(B2:B3)");
    m_Api->UndoCellValue("B7", "=SUM(B1:B6)");
    m_Api->UndoCellValue("D1", "=B2");
    m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, m_NbRow, m_NbCol));
#ifdef checksp
    m_Api->Check();
#endif

    const tRect wRect(2, 2, 4, 3);
    const tString wBefore = SnapshotRect(1, 1, m_NbRow, m_NbCol);

    CPPUNIT_ASSERT(m_Api->UndoDeleteRowByRect(wRect));
    CPPUNIT_ASSERT(m_Api->Undo());
    AssertGridMatchesSnapshot(wBefore, 1, 1, m_NbRow, m_NbCol, "UndoDeleteRowByRect then Undo");
#ifdef checksp
    m_Api->Check();
#endif

    CPPUNIT_ASSERT(m_Api->UndoDeleteColByRect(wRect));
    CPPUNIT_ASSERT(m_Api->Undo());
    AssertGridMatchesSnapshot(wBefore, 1, 1, m_NbRow, m_NbCol, "UndoDeleteColByRect then Undo");
#ifdef checksp
    m_Api->Check();
#endif

    CPPUNIT_ASSERT(m_Api->UndoInsertRowByRect(wRect));
    CPPUNIT_ASSERT(m_Api->Undo());
    AssertGridMatchesSnapshot(wBefore, 1, 1, m_NbRow, m_NbCol, "UndoInsertRowByRect then Undo");
#ifdef checksp
    m_Api->Check();
#endif

    CPPUNIT_ASSERT(m_Api->UndoInsertColByRect(wRect));
    CPPUNIT_ASSERT(m_Api->Undo());
    AssertGridMatchesSnapshot(wBefore, 1, 1, m_NbRow, m_NbCol, "UndoInsertColByRect then Undo");
#ifdef checksp
    m_Api->Check();
#endif
}

void TestSkInsertDeleteColRowByRect::setUp() {
    std::filesystem::remove_all("./Spreadsheet");

    m_Application = tApplication::Instance();
    m_Api = new tApi;
    m_Api->IsUndoActif(true);
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
}

void TestSkInsertDeleteColRowByRect::tearDown() {
    delete(m_Api);
    tClassFactory::Instance()->Clear();
}

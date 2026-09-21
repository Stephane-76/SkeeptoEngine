//=============================================================================  
// TestSkRangeInsertDelete
// author : Stéphane Allez
//=============================================================================  

#include "../include/TestSkRangeInsertDelete.hpp"
#include "../include/SkSpreadSheet.hpp"

// We can send it to the API of a feature 
TestSkRangeInsertDelete::TestSkRangeInsertDelete() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
    m_NbCol = 5;
    m_NbRow = 7;
}

void TestSkRangeInsertDelete::DrawCell(tString sTitle, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
    return; // Drop
    cout << endl;
    cout << sTitle << endl;
    for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
        for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
            tVariant wVariant = m_Api->CellValue(wRow, wCol);
            tString wFormula = m_Api->Formula(wRow, wCol);
            cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant << "\t";
        }
        cout << endl;
    }
}   

void TestSkRangeInsertDelete::DebugRange() {
    return; // Drop
    cout << "Debug Range" << endl;
    tVectorRange* wVectorRange; // = m_Api->ActiveSheet()->ColRowCellRange()->VectorRange();
    for (auto wRange : *wVectorRange) {
        cout << wRange->Debug();
    }
}

void TestSkRangeInsertDelete::Fill() {
    for (tInt wRow = 1; wRow <= m_NbRow; wRow++) {
        for (tInt wCol =1; wCol <= m_NbCol; wCol++) {
            tVariant wVariant = wRow * 10 + wCol;
            m_Api->CellValue(wRow, wCol, wVariant);
        }
    }
}

void TestSkRangeInsertDelete::TestInsertRowByRect() {   
    Fill();
    
    m_Api->UndoCellValue("B4","=SUM(B2:B3)");
    
    m_Api->UndoCellValue("B7","=SUM(B1:B6)");
    
    m_Api->UndoCellValue("D1","=B2");
    
    tRect wRect(2, 2, 4, 3);
    DrawCell("Before Insert"+wRect.StrRef(), 1, 1, 10, 5);
   
    
   
    //m_Api->UndoInsertRowByRect(wRect);
    m_Api->UndoInsertRowByRect(wRect);
    
    DrawCell("AFter Insert "+wRect.StrRef(), 1, 1, 10, 5);
    m_Api->Undo();
    DrawCell("AFter Undo "+wRect.StrRef(), 1, 1, 10, 5);
    
    m_Api->UndoDeleteRowByRect(wRect);
    
    DrawCell("After Delete "+wRect.StrRef(), 1, 1, 10, 5);
    
    m_Api->Undo();
    
    DrawCell("After Undo Delete "+wRect.StrRef(), 1, 1, 10, 5);
    m_Api->UndoDeleteRowByRect(wRect);
    m_Api->UndoDeleteRowByRect(wRect);
    
    DrawCell("After  Delete * 2 "+wRect.StrRef(), 1, 1, 10, 5);
    
    m_Api->Undo();
    m_Api->Undo();
    DrawCell("After Undo Delete "+wRect.StrRef(), 1, 1, 10, 5);
}

void TestSkRangeInsertDelete::setUp() {
    std::filesystem::remove_all("./Spreadsheet");

    m_Application = tApplication::Instance();
    m_Api = new tApi;
    m_Api->IsUndoActif(true);
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
}

void TestSkRangeInsertDelete::tearDown() {
    delete(m_Api);
    tClassFactory::Instance()->Clear();
}

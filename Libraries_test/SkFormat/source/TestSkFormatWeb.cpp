//==============================================================================
// TestSkFormatWeb Test For SpreadSheet Web
// le 17/11/2023
//==============================================================================
#include "../include/TestSkFormatWeb.hpp"
#include "../include/TestSkFormatTeardownHelpers.hpp"

#include <SkFormatCssApi.hpp>


using namespace SkRoot;
using namespace SkSpreadSheet;
using namespace SkFormat;

#define _drawdebug

// We can send it to the API of a feature
TestSkFormatWeb::TestSkFormatWeb()  : CPPUNIT_NS::TestFixture(), m_Application(nullptr),m_FormatRoot(nullptr){
    m_Application = nullptr;
    m_Api = nullptr;
    m_NbRow=42;
    m_NbCol=10;
}

void TestSkFormatWeb::UndoOperation() {
    return; // Drop
    tUndo* wUndo = m_Api->LastUndo();
    if (wUndo != nullptr) {
        cout << "Undo ->" << wUndo->OperationName() << endl;
    }
}

void TestSkFormatWeb::Debug() {
}

void  TestSkFormatWeb::Fill() {
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
}


void TestSkFormatWeb::DrawCell(tString sOperation, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
    return; // Drop
    cout << endl;
    cout << sOperation << endl;
    for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
        for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
            tVariant wVariant = m_Api->CellValue(wRow, wCol);
            tString wFormula = m_Api->Formula(wRow, wCol);
            tCell* wCell=m_Api->Cell(wRow,wCol);
            
            cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant;
            if (wCell!=nullptr) cout << ":" << wCell->Css() << ":" << m_Api->CellFormat(wRow, wCol) << "\t";
        }
        cout << endl;
    }
}

void TestSkFormatWeb::Pressure(tInt sDynamicNbRow, tInt sDynamicNbCol) {
    tCell* wCell;
    tWorkBook* wWorkBook= m_Api->ActiveSheet()->WorkBook();
    for (tInt wCol = 1; wCol <= sDynamicNbCol; wCol++) {
        if (wCol > 1) {
            wCell = m_Api->EnsureCell(1, wCol);
            tStringStream wStream;
            wStream << Base10ToAlpha(wCol - 1) << sDynamicNbRow - 1 << "+1";
            wWorkBook->CompilCell(wCell, wStream.str().c_str());
        }
    }

    for (tInt wRow = 2; wRow <= sDynamicNbRow; wRow++) {
        for (tInt wCol = 1; wCol <= sDynamicNbCol; wCol++) {
            wCell = m_Api->EnsureCell(wRow, wCol);
            tStringStream wStream;
            if (wRow == sDynamicNbRow) {
                if (wCol != sDynamicNbCol) {
                    wStream << "SUM(" << Base10ToAlpha(wCol) << 1 << ":" << Base10ToAlpha(wCol) << wRow - 1 << ")";
                }
                else {
                    wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(sDynamicNbCol - 1) << wRow << ")";
                }
            }
            else {
                if (wCol == sDynamicNbCol) {
                    wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(sDynamicNbCol - 1) << wRow << ")";
                    //wStream << "1";
                }
                else {
                    wStream << Base10ToAlpha(wCol) << wRow - 1 << "+1";
                }
            }
            wCell->Path(0);
            wWorkBook->CompilCell(wCell, wStream.str().c_str());
        }
    }
    
}

void TestSkFormatWeb::TestSkWebBorder() {
    m_Api->UndoCellFormat("D3:D7","background-color:yellow;");
    
    m_Api->UndoCellBorder("H4:J6",1,"solid black 1px;");
    
    //cout << m_Api->FormatApi()->Debug();
    
    //cout << endl << "I4 format=" << m_Api->CellFormat("I4") << endl;
    
    m_Api->UndoCellBorder("I4:I6",1,"");
    
    //cout << endl << "I4 format=" << m_Api->CellFormat("I4") << endl;
    
    //cout << m_Api->FormatApi()->Debug();
    
    CPPUNIT_ASSERT_MESSAGE("TestSkWebBorder I4 Format ",m_Api->CellFormat("I4")=="");

    m_Api->Undo();

    // Owner-model: undo restores all four sides on I4 (interior of H4:J6 block).
    tString wResultat="border-left:solid 1px black;border-top:solid 1px black;border-right:solid 1px black;border-bottom:solid 1px black;";

    CPPUNIT_ASSERT_MESSAGE("TestSkWebBorder I4 Format Undo ",m_Api->CellFormat("I4")==wResultat);

    
    
    m_Api->Redo();
    
    CPPUNIT_ASSERT_MESSAGE("TestSkWebBorder I4 Format Redo ",m_Api->CellFormat("I4")=="");
}

void TestSkFormatWeb::TestSkWebPressure() {
    Pressure(50,10);
    
    m_Api->UndoCellFormat("D3:F20","background-color:yellow;");
    m_Api->UndoCellFormat("E7:H10;B16:L17","background-color:lightblue;");
           
    m_Api->UndoCellBorder("C2:E12;J18:O22",5,"dashed black 2px;");
           
    m_Api->UndoCellBorder("D5:E7;J6:O9",5,"solid navy 2px;");
     
    m_Api->UndoCellBorder("A20:G20",5,"solid red 5px;");
    m_Api->UndoCellBorder("A12:Z22",5,"solid navy 3px;");
       
    m_Api->UndoCellBorder("E1:E12",1,"solid red 3px;");
    
    m_Api->UndoCellBorder("A12:Z22",1,"");
    
    m_Api->UndoSizeRow(4, 4,30);
    m_Api->UndoSizeRow(9, 9,30);
    m_Api->UndoSizeRow(7, 7,30);
    m_Api->UndoSizeCol(8, 8,130);
    m_Api->UndoSizeCol(12, 12,180);
    
    m_Api->UndoSizeRow(44,44,30);
    
    m_Api->UndoSizeCol(AlphaToBase10("L"),AlphaToBase10("L"), 120);
    m_Api->UndoSizeCol(AlphaToBase10("M"),AlphaToBase10("N"), 120);
    m_Api->UndoSizeCol(AlphaToBase10("P"),AlphaToBase10("Q"), 120);
    
    m_Api->UndoApplyMerge("D5:E7");
    m_Api->UndoCellFormat("D5","background-color:lightblue;font:\"Arial\",serif 32pt;");
    m_Api->UndoCellFormat("A4:E4","background-color:red;");
      
    m_Api->UndoApplyMerge("C35:J47");
    m_Api->UndoCellFormat("C35","background-color:lightblue;font:\"Arial\",serif 64pt;");
 
    m_Api->UndoApplyMerge("L42:O53");
    m_Api->UndoCellFormat("L42","background-color:green;font:\"Courier New\",serif 64pt;");
 
    m_Api->UndoApplyMerge("D53:F54");
    m_Api->UndoCellFormat("D53","background-color:red;color:blue;font:\"Courier New\",serif 32pt;");
      
    m_Api->UndoApplyMerge("B55:H60");
    m_Api->UndoCellFormat("B55","background-color:yellow;font:\"Courier New\",serif 32pt;");
       
    m_Api->UndoApplyMerge("J52:J59");
    m_Api->UndoCellFormat("J52","background-color:yellow;font:\"Courier New\",serif 32pt;");

    tString wResultFormat=m_Api->CellFormat("D5");
    //cout << wResultFormat << endl;
 
    tString wFormatTest="background-color:lightblue;font:\"Arial\",serif 32pt;";
    CPPUNIT_ASSERT_MESSAGE("Test D5 1",wResultFormat==wFormatTest);
    m_Api->UndoRaz("D5");
    // Multiple Undo Redo
    wResultFormat=m_Api->CellFormat("D5");
    CPPUNIT_ASSERT_MESSAGE("Test Raz D5 1",wResultFormat=="");
    m_Api->Undo();
    wResultFormat=m_Api->CellFormat("D5");
    CPPUNIT_ASSERT_MESSAGE("Test D5 1",wResultFormat==wFormatTest);
    
    m_Api->Redo();
    wResultFormat=m_Api->CellFormat("D5");
    CPPUNIT_ASSERT_MESSAGE("Test Raz D5 1",wResultFormat=="");
   
    m_Api->Undo();
    wResultFormat=m_Api->CellFormat("D5");
    CPPUNIT_ASSERT_MESSAGE("Test D5 1",wResultFormat==wFormatTest);
    

    m_Api->Redo();
    wResultFormat=m_Api->CellFormat("D5");
    CPPUNIT_ASSERT_MESSAGE("Test Raz D5 1",wResultFormat=="");
  
    m_Api->Undo();
    wResultFormat=m_Api->CellFormat("D5");
    CPPUNIT_ASSERT_MESSAGE("Test D5 1",wResultFormat==wFormatTest);
    
    const tInt wTestRows=10;
    for(tInt wNb=1; wNb<=wTestRows; wNb++) {
        m_Api->UndoDeleteRow(1, 3);
    }
    
    for(tInt wNb=1; wNb<=wTestRows; wNb++) {
        m_Api->Undo();
    }
    CPPUNIT_ASSERT_MESSAGE("TestSkWeb Delete Row D5",wResultFormat==m_Api->CellFormat("D5"));
    
    //cout << wResultFormat << endl;
    DrawCell("Before Delete Col", 1, 1, m_NbRow, m_NbCol);
    //cout <<endl << m_Api->Cell(m_NbRow,m_NbCol)->StrRef()  << ":" << m_Api->Cell(m_NbRow,m_NbCol)->FormulaStr() << "=" << m_Api->Cell(m_NbRow,m_NbCol)->Value() << endl;
    for(tIndex wCol=1;wCol<=m_NbCol-2;wCol++) {
        m_Api->UndoDeleteCol(1,1);
        DrawCell("Delete ", 1, 1, m_NbRow, m_NbCol);
    }
    
    for(tIndex wCol=1;wCol<=m_NbCol-2;wCol++) {
        //cout << tApplication::Instance()->DebugUndo() << endl;
        m_Api->Undo();
        DrawCell("Undo ", 1, 1, m_NbRow, m_NbCol);
    }
    
    //cout <<endl << m_Api->Cell(m_NbRow,m_NbCol)->StrRef()  << ":" << m_Api->Cell(m_NbRow,m_NbCol)->FormulaStr() << "=" << m_Api->Cell(m_NbRow,m_NbCol)->Value() << endl;
    
    CPPUNIT_ASSERT_MESSAGE("TestSkWeb Delete Col D5",wResultFormat==m_Api->CellFormat("D5"));

    DrawCell("Before Delete Row", 1, 1, m_NbRow, m_NbCol);
    //cout <<endl << m_Api->Cell(m_NbRow,m_NbCol)->StrRef()  << ":" << m_Api->Cell(m_NbRow,m_NbCol)->FormulaStr() << "=" << m_Api->Cell(m_NbRow,m_NbCol)->Value() << endl;
    for(tIndex wRow=1;wRow<=m_NbRow-1;wRow++) {
        m_Api->UndoDeleteRow(1,1);
        //cout << tApplication::Instance()->DebugUndo() << endl;
        DrawCell("Delete ", 1, 1, m_NbRow, m_NbCol);
    }
   
    for(tIndex wRow=1;wRow<=m_NbRow-1;wRow++) {
        //cout << tApplication::Instance()->DebugUndo() << endl;
        m_Api->Undo();
        DrawCell("Undo ", 1, 1, m_NbRow, m_NbCol);
    }
  
    
    CPPUNIT_ASSERT_MESSAGE("TestSkWeb Delete Col D5",wResultFormat==m_Api->CellFormat("D5"));

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkFormatWeb::TestSkWebDemo() {
    // Just xcode
    tString wFileName="/Users/stephaneallez/Projects/Excel/Budget.sker";
    tFile wFile=tFile(wFileName);
    if (wFile.Exist()) {
        tString wJson = wFile.LoadString();
        m_Api->ReadJson(wJson);
        //m_Api->ActiveSheet("TestIndirection");
        
        //m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, m_NbRow, m_NbCol));
        DrawCell("Read demo", 1, 1, 5, 8);
        
        
        m_Api->DeleteRow(1, 3);
        
        m_Api->Undo();
#ifdef checksp
        m_Api->Check();
#endif
    } else {
        //cout << wFileName << "Dont't exist" << endl;
    }
}

void TestSkFormatWeb::TestSkWebBudget() {
    // Just xcode
    tString wFileName="/Users/stephaneallez/Projects/Excel/BilanEntreprise.json";
    tFile wFile=tFile(wFileName);
    if (wFile.Exist()) {
        tString wJson = wFile.LoadString();
        m_Api->ReadJson(wJson);
        //m_Api->ActiveSheet("TestIndirection");
        
        //m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, m_NbRow, m_NbCol));
        DrawCell("Read Budget", 1, 1, 5, 8);
        
        
        m_Api->DeleteRow(1, 3);
        
        m_Api->Undo();
#ifdef checksp
        m_Api->Check();
#endif
    } else {
        //cout << wFileName << "Dont't exist" << endl;
    }
}

void TestSkFormatWeb::TestSkWebClass() {
    
    Pressure(50,10);
    
    m_Api->RegisterClassAttribute("SkCellButton","Button","Test");
    tBool wOk=m_Api->AddProperty("name", "string", "Test string name", 1,"Allez");
    CPPUNIT_ASSERT_MESSAGE("TestSkWebClass AddProperty Name",wOk);
    wOk=m_Api->AddProperty("now", "date", "Test date", 1,"Allez");
    CPPUNIT_ASSERT_MESSAGE("TestSkWebClass AddProperty Date",wOk);
    
    m_Api->UndoCellClass("L3","SkCellButton");
    
    m_Api->UndoSizeRow(3,3, 120);
    m_Api->UndoCellValue("L4","=L3.name+' .... '");
    m_Api->UndoCellValue("L5","=L3.now");
    
    m_Api->UndoCellFormat("D3:F20","background-color:yellow;");
    m_Api->UndoCellFormat("E7:H10;B16:L17","background-color:lightblue;");
           
    m_Api->UndoCellBorder("C2:E12;J18:O22",5,"dashed black 2px;");
           
    m_Api->UndoCellBorder("D5:E7;J6:O9",5,"solid navy 2px;");
     
    m_Api->UndoCellBorder("A20:G20",5,"solid red 5px;");
    m_Api->UndoCellBorder("A12:Z22",5,"solid navy 3px;");
       
    m_Api->UndoCellBorder("E1:E12",1,"solid red 3px;");
    
    m_Api->UndoSizeRow(4, 4,30);
    m_Api->UndoSizeRow(9, 9,30);
    m_Api->UndoSizeRow(7, 7,30);
    m_Api->UndoSizeCol(8, 8,130);
    m_Api->UndoSizeCol(12, 12,180);
    
    m_Api->UndoSizeRow(44,44,30);
    
    m_Api->UndoSizeCol(AlphaToBase10("L"),AlphaToBase10("L"), 120);
    m_Api->UndoSizeCol(AlphaToBase10("M"),AlphaToBase10("N"), 120);
    m_Api->UndoSizeCol(AlphaToBase10("P"),AlphaToBase10("Q"), 120);
    
    m_Api->UndoApplyMerge("D5:E7");
    m_Api->UndoCellFormat("D5","background-color:lightblue;font:\"Arial\",serif 32pt;");
    m_Api->UndoCellFormat("A4:E4","background-color:red;");
      
    m_Api->UndoApplyMerge("C35:J47");
    m_Api->UndoCellFormat("C35","background-color:lightblue;font:\"Arial\",serif 64pt;");
 
    m_Api->UndoApplyMerge("L42:O53");
    m_Api->UndoCellFormat("L42","background-color:green;font:\"Courier New\",serif 64pt;");
 
    m_Api->UndoApplyMerge("D53:F54");
    m_Api->UndoCellFormat("D53","background-color:red;color:blue;font:\"Courier New\",serif 32pt;");
      
    m_Api->UndoApplyMerge("B55:H60");
    m_Api->UndoCellFormat("B55","background-color:yellow;font:\"Courier New\",serif 32pt;");
       
    m_Api->UndoApplyMerge("J52:J59");
    m_Api->UndoCellFormat("J52","background-color:yellow;font:\"Courier New\",serif 32pt;");
        
    
    m_Api->UndoDeleteRow(3,1);
    
    m_Api->Undo();
    
}
void TestSkFormatWeb::setUp() {
    std::filesystem::remove_all("./Spreadsheet");
    
    m_Application = tApplication::Instance();
    m_FormatRoot = tFormatRoot::Instance();
    m_Api = new SkSpreadSheet::tApi;
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
    m_FormatApi=new SkFormat::tFormatCssApi();
    m_Api->FormatApi((tFormatApi*)m_FormatApi);
};

void TestSkFormatWeb::tearDown() {
    delete(m_Api);
    m_Api = nullptr;
    TestSkFormatTeardown::AssertFormatPoolEmptyAfterApiDelete(m_FormatApi, "TestSkFormatWeb");
    delete(m_FormatApi);
    m_FormatApi = nullptr;

    DoneFormatRoot();
}

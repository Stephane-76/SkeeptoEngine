#include "../include/TestSkInterface.hpp"
#include <SkFormatCssApi.hpp>
#include <SkUndoRedoJsonCallBack.hpp>

#ifdef TestMultiUser

const tString CstPathBudget="/Users/stephaneallez/Projects/Excel/Budget-familial.sker";

namespace {
    const tString kBudgetCellFormatSuffix =
        R"(format-string:"#,##0  €" 0;border-top:solid 1px #95B3D7;)";
}

void TestSkInterface::DrawCell(tInterfaceWeb* sInterface,tString sOperation, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
    return; // Drop
    sInterface->SetActiveWorkBook();
    tWorkBook* wWorkBook=sInterface->ActiveWorkBook();
    cout << wWorkBook->Uri() <<  endl;
    cout << wWorkBook->UndoRebaseLog().DebugOperations() << endl;
  
    cout << endl;
    cout << sOperation << endl;
    for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
        for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
            tVariant wVariant =sInterface->CellValue(wRow, wCol);
            tString wFormula = sInterface->Formula(wRow, wCol);
            cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant << "\t";
        }
        cout << endl;
    }
}

void TestSkInterface::ListWorkBook() {
    tString wJsonList=m_Interface->JsonWorkBooksList();
    Document wDocument;
    wDocument.Parse(wJsonList.c_str());
    
    Value& wList=wDocument["list"];
    for (Value::ConstValueIterator wIterator=wList.Begin();wIterator!=wList.End();wIterator++) {
        cout << (*wIterator).GetString() << endl;
    }
}

void TestSkInterface::Fill(tInterfaceWeb* sInterface) {
    tCell* wCell;
    for (tInt wCol = 1; wCol <= m_NbCol; wCol++) {
        if (wCol > 1) {
            wCell = sInterface->EnsureCell(1, wCol);
            tStringStream wStream;
            wStream << "=" << Base10ToAlpha(wCol - 1) << m_NbRow - 1 << "+1";
            sInterface->UndoCellValue(wCell->StrRef(),  wStream.str().c_str());
        }
    }
    
    for (tInt wRow = 2; wRow <= m_NbRow; wRow++) {
        for (tInt wCol = 1; wCol <= m_NbCol; wCol++) {
            wCell = sInterface->EnsureCell(wRow, wCol);
            tStringStream wStream;
            if (wRow == m_NbRow) {
                if (wCol != m_NbCol) {
                    wStream << "=SUM(" << Base10ToAlpha(wCol) << 1 << ":" << Base10ToAlpha(wCol) << wRow - 1 << ")";
                    //wStream << "1";
                }
                else {
                    wStream << "=SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(m_NbCol - 1) << wRow << ")";
                    //wStream << "1";
                }
            }
            else {
                if (wCol == m_NbCol) {
                    wStream << "=SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(m_NbCol - 1) << wRow << ")";
                    //wStream << "1";
                }
                else {
                    wStream << "=" << Base10ToAlpha(wCol) << wRow - 1 << "+1";
                }
            }
            sInterface->UndoCellValue(wCell->StrRef(),  wStream.str().c_str());
        }
    }
}

tString TestSkInterface::DebugCell(tInterfaceWeb* sInterface,tString sRef) {
    sInterface->SetActiveWorkBook();
    tCell* wCell=sInterface->Cell(sRef);
    tStringStream wStream;
    if (wCell!=nullptr) {
        wStream <<  wCell->StrRef(true) << "=" <<  wCell->FormulaStr()  << ":" << sInterface->CellFormat(sRef);
    } else {
        wStream <<  sRef << "=nullptr";
    }
    return(wStream.str());
}

void TestSkInterface::InitInterface() {
    tSpreadSheetContainer* wContainer = tSpreadSheetContainer::Instance();
    wContainer->DeleteWorkBook(m_WorkBookUri + m_UserServerStr);
    wContainer->DeleteWorkBook(m_WorkBookUri + m_User1Str);
    wContainer->DeleteWorkBook(m_WorkBookUri + m_User2Str);
    wContainer->DeleteWorkBook("wwww.skeema.fr/w1");
    wContainer->ActiveWorkBook(nullptr);

    //User
    m_UserServer = new tTestUser(m_UserServerStr);
    m_User1 = new tTestUser(m_User1Str);
    m_User2 = new tTestUser(m_User2Str);
    // Interface
    m_InterfaceServer = new tInterfaceWeb(m_UserServerStr,m_WorkBookUri+m_UserServerStr);;
    m_InterfaceUser1 = new tInterfaceWeb(m_User1Str,m_WorkBookUri+m_User1Str);
    m_InterfaceUser2 = new tInterfaceWeb(m_User2Str,m_WorkBookUri+m_User2Str);

    m_InterfaceServer->NewWorkBook(m_WorkBookUri+m_UserServerStr);
    m_InterfaceUser1->NewWorkBook(m_WorkBookUri+m_User1Str);
    m_InterfaceUser2->NewWorkBook(m_WorkBookUri+m_User2Str);

    // Add User
    m_InterfaceServer->Dispatcher().AddUser(m_User1);
    m_InterfaceServer->Dispatcher().AddUser(m_User2);
    
    // Add Server
    m_InterfaceUser1->Dispatcher().AddUser(m_UserServer);
    m_InterfaceUser2->Dispatcher().AddUser(m_UserServer);
    
    m_User1->Interface(m_InterfaceUser1);
    m_User2->Interface(m_InterfaceUser2);
    m_UserServer->Interface(m_InterfaceServer);

    //m_InterfaceServer->Dispatcher().AddUser(m_UserServer);
    m_InterfaceServer->Dispatcher().AddUser(m_User1);
    m_InterfaceServer->Dispatcher().AddUser(m_User2);

    m_InterfaceUser1->Dispatcher().AddUser(m_UserServer);
    m_InterfaceUser2->Dispatcher().AddUser(m_UserServer);

    m_InterfaceServer->Server(true);
    m_InterfaceUser1->Client(true);
    m_InterfaceUser2->Client(true);
    m_InterfaceServer->IsJson(true);
    m_InterfaceUser1->IsJson(true);
    m_InterfaceUser2->IsJson(true);
    if (tFormatApi* wFormatApi = tSpreadSheetContainer::Instance()->FormatApi()) {
        m_InterfaceServer->FormatApi(wFormatApi);
        m_InterfaceUser1->FormatApi(wFormatApi);
        m_InterfaceUser2->FormatApi(wFormatApi);
    }
#if defined(checkfo) && defined(TestMultiUser)
    tCheckFormat::Instance()->AddUser(m_User1);
    tCheckFormat::Instance()->AddUser(m_User2);
    tCheckFormat::Instance()->AddUser(m_UserServer);
#endif
}

void TestSkInterface::DoneInterface() {
#if defined(checkfo) && defined(TestMultiUser)
    tCheckFormat::Instance()->Clear();
#endif
    m_InterfaceUser1->Clear();
    m_InterfaceUser2->Clear();
    m_InterfaceServer->Clear();

    tSpreadSheetContainer* wContainer = tSpreadSheetContainer::Instance();
    wContainer->ClearUndoRedo();
    tApplication::Instance()->ClearUndoRedo();
    tApplication::Instance()->Clipboard()->Text("");
    wContainer->Clear();
    wContainer->ActiveWorkBook(nullptr);
    tClassFactory::Instance()->Clear();

    delete(m_InterfaceServer);
    delete(m_InterfaceUser1);
    delete(m_InterfaceUser2);
    delete(m_UserServer);
    
    delete(m_User1);
    delete(m_User2);
}

void TestSkInterface::TestSkInterfaceUndoJson() {
    //ListWorkBook();
    
    Fill(m_InterfaceUser1);
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->UndoCellFormat("A2", "color:red;");
    m_InterfaceUser1->UndoCellFormat("A1", "color:blue;");
    m_InterfaceUser1->UndoCellFormat("A3", "color:white;");
    m_InterfaceUser1->UndoCellFormat("A4", "color:black;");
    
    
    tString wFormatA2=m_InterfaceUser1->CellFormat("A2");
    //cout << endl << "A2=" << wFormatA2 << endl;
    CPPUNIT_ASSERT_MESSAGE("Do Color Red  A2", wFormatA2 == "color:red;");
    
    m_InterfaceUser1->UndoCellFormat("A2", "color:blue;");
    m_InterfaceUser2->SetActiveWorkBook();
    // Search on user2
    wFormatA2=m_InterfaceUser2->CellFormat("A2");
    //cout << "A2="<< wFormatA2 << endl;
    CPPUNIT_ASSERT_MESSAGE("Do Color Blue  A2", wFormatA2 == "color:blue;");
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->Undo();
    
    wFormatA2=m_InterfaceUser1->CellFormat("A2");
    //cout << "A2=" << wFormatA2 << endl;
    CPPUNIT_ASSERT_MESSAGE("Undo Color Red  A2", wFormatA2 == "color:red;");
    
    m_InterfaceUser1->Redo();
    
    wFormatA2=m_InterfaceUser1->CellFormat("A2");
    //cout <<  "A2=" << wFormatA2 << endl;
    CPPUNIT_ASSERT_MESSAGE("Do Color Blue A2", wFormatA2 == "color:blue;");
        
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->UndoCellFormat("A2", "color:blue;background-color:black;");
    
    wFormatA2=m_InterfaceUser1->CellFormat("A2");
    //cout <<  "A2=" << wFormatA2 << endl;
    CPPUNIT_ASSERT_MESSAGE("Do color:blue;background-color:black;  A2", wFormatA2 == "color:blue;background-color:black;");
    
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->Undo();
    wFormatA2=m_InterfaceUser2->CellFormat("A2");
    //cout <<  "A2=" << wFormatA2 << endl;
  
    m_InterfaceUser1->SetActiveWorkBook();
    wFormatA2=m_InterfaceUser1->CellFormat("A2");
    //cout <<  "A2=" << wFormatA2 << endl;
    CPPUNIT_ASSERT_MESSAGE("Undo color:blue; A2", wFormatA2 == "color:blue;");
   
    m_InterfaceUser1->Undo();
    
    wFormatA2=m_InterfaceUser2->CellFormat("A2");
    //cout <<  "A2=" << wFormatA2 << endl;
    CPPUNIT_ASSERT_MESSAGE("Undo Color Red A2", wFormatA2 == "color:red;");
    
    
    m_InterfaceUser1->UndoRaz("A2:A3");
    
    m_InterfaceUser1->Undo();
    
}

void TestSkInterface::TestSkInterfaceFormatString() {
    
    tApplication::Instance()->Locale("fr");
    m_InterfaceUser1->SetActiveWorkBook();
    tVariant wVariant;
    wVariant.Parse("10245,7");
    m_InterfaceUser1->UndoCellValue("A1", wVariant);
    
    wVariant.Parse("-1211233323,74");
    m_InterfaceUser1->UndoCellValue("A2", wVariant);
 
    wVariant.Parse("1234567,89");
    m_InterfaceUser1->UndoCellValue("A3", wVariant);
    tString wFormatAccount="'#,##0.00 $;(#,##0.00) $'";
    tString wFormatScientific="'0.00E+00'";
    m_InterfaceUser1->UndoCellFormat("A1", "format-string :"+ wFormatScientific+";");
    m_InterfaceUser1->UndoCellFormat("A2", "format-string :"+ wFormatAccount+" 3;");
    m_InterfaceUser1->UndoCellFormat("A3", "format-string :"+ wFormatAccount+" 4;");
    tCell* wCellA1 = m_InterfaceUser1->EnsureCell("A1");
    tCell* wCellA2 = m_InterfaceUser1->EnsureCell("A2");
    tCell* wCellA3 = m_InterfaceUser1->EnsureCell("A3");
    
    tString wFormatStr=m_InterfaceUser1->CellFormat("A4");
    //cout << endl << wFormatStr << endl;

    tString wResult=m_InterfaceUser1->ActiveWorkBook()->CellFormatString(wCellA1);
    //cout << endl << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format Scientific fr", wResult == "1,02e+04");
    wResult=m_InterfaceUser1->ActiveWorkBook()->CellFormatString(wCellA2);
    //cout  << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format accounting 2 fr->"+wResult, wResult == "(1 211 233 323,740) €");

    wResult=m_InterfaceUser1->ActiveWorkBook()->CellFormatString(wCellA3);
    //cout  << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format number 2 fr ", wResult == "1 234 567,8900 €");

    tApplication::Instance()->Locale("us");
    wResult=m_InterfaceUser1->ActiveWorkBook()->CellFormatString(wCellA1);
    //cout << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format Scientific us "+wResult, wResult == "1.02e+04");
    
    wResult=m_InterfaceUser1->ActiveWorkBook()->CellFormatString(wCellA2);
    //cout  << wResult << endl;
    CPPUNIT_ASSERT_MESSAGE("Format accounting 2 us", wResult == "(1,211,233,323.740) €"); // Bug later
    
   // m_InterfaceUser1->SetActiveWorkBook();
    tString wFormat=m_InterfaceUser1->CellFormat("A2");
    // cout << wFormat << endl;
    
    tBool wOk=m_InterfaceUser1->UndoCellPrecision("A1:A3",true);
    CPPUNIT_ASSERT_MESSAGE("UndoCellPrecision A1:A3", wOk);
    
    wFormat=m_InterfaceUser1->CellFormat("A2");
    //cout << wFormat << endl;
    wResult=m_InterfaceUser1->CellFormatString("A2");
    CPPUNIT_ASSERT_MESSAGE("Format accounting 2 us", wResult == "(1,211,233,323.7400) €");
    
    //cout << wResult << endl;
    m_InterfaceUser2->SetActiveWorkBook();
   
    tCell* wCell=m_InterfaceUser2->Cell("A2");
    wResult=m_InterfaceUser2->CellFormatString("A2");
    CPPUNIT_ASSERT_MESSAGE("Format accounting 2 us", wResult == "(1,211,233,323.7400) €");
}

void TestSkInterface::TestSkInterfaceDeleteRow() {
    m_InterfaceUser1->SetActiveWorkBook();
    Fill(m_InterfaceUser1);
    DrawCell(m_InterfaceUser2,"Fill Sheet1", 1, 1, m_NbRow, m_NbCol);
    
    m_InterfaceUser1->UndoCellFormat("A2:Z12", "color:white;background-color:black;");
 
    tString wFormatA2=m_InterfaceUser2->CellFormat("A2");
    //cout <<  "A2=" << wFormatA2 << endl;
    CPPUNIT_ASSERT_MESSAGE("Do format A2:Z12 ", wFormatA2 == "color:white;background-color:black;");
    
    m_InterfaceUser1->UndoDeleteRow(3, 5);
    
    m_InterfaceUser1->Undo();
    wFormatA2=m_InterfaceUser2->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Do format A2:Z12 ", wFormatA2 == "color:white;background-color:black;");
    m_InterfaceUser2->UndoDeleteCol(1,8);
    
    m_InterfaceUser2->Undo();
    
    wFormatA2=m_InterfaceUser2->CellFormat("A2");
    //cout <<  "A2=" << wFormatA2 << endl;
    CPPUNIT_ASSERT_MESSAGE("Do format A2:Z12 ", wFormatA2 == "color:white;background-color:black;");
    m_InterfaceUser1->SetActiveWorkBook();
    wFormatA2=m_InterfaceUser1->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Do format A2:Z12 ", wFormatA2 == "color:white;background-color:black;");
}
void TestSkInterface::TestSkInterfaceRaz() {
    m_InterfaceUser1->SetActiveWorkBook();
    Fill(m_InterfaceUser1);
    DrawCell(m_InterfaceUser2,"Fill Sheet1", 1, 1, m_NbRow, m_NbCol);
    
    m_InterfaceUser1->UndoCellFormat("A2:Z12", "color:white;background-color:black;");
    
    tString wFormatA2=m_InterfaceUser2->CellFormat("A2");
    //cout <<  "A2=" << wFormatA2 << endl;
    CPPUNIT_ASSERT_MESSAGE("Format A2:Z12", wFormatA2 == "color:white;background-color:black;");
    
    tString wFormatTest=wFormatA2;
    m_InterfaceUser1->UndoRaz("A2:Z12");
    wFormatA2=m_InterfaceUser1->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Undo Redo Interface 1", wFormatA2 == "");
    wFormatA2=m_InterfaceUser2->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Undo Redo Interface 2", wFormatA2 == "");
    
    m_InterfaceUser1->Undo();
    wFormatA2=m_InterfaceUser1->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Undo Redo Interface 1", wFormatA2 == wFormatTest);
    wFormatA2=m_InterfaceUser2->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Undo Redo Interface 2", wFormatA2 == wFormatTest);
    
    m_InterfaceUser1->Redo();
    wFormatA2=m_InterfaceUser1->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Undo Redo Interface 1", wFormatA2 == "");
    wFormatA2=m_InterfaceUser2->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Undo Redo Interface 2", wFormatA2 == "");
    
    m_InterfaceUser1->Undo();
    wFormatA2=m_InterfaceUser1->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Undo Redo Interface 1", wFormatA2 == wFormatTest);
    wFormatA2=m_InterfaceUser2->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Undo Redo Interface 2", wFormatA2 == wFormatTest);
    
    m_InterfaceUser1->Redo();
    wFormatA2=m_InterfaceUser1->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Undo Redo Interface 1", wFormatA2 == "");
    wFormatA2=m_InterfaceUser2->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Undo Redo Interface 2", wFormatA2 == "");
    
    m_InterfaceUser1->Undo();
    wFormatA2=m_InterfaceUser1->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Undo Redo Interface 1", wFormatA2 == wFormatTest);
    wFormatA2=m_InterfaceUser2->CellFormat("A2");
    CPPUNIT_ASSERT_MESSAGE("Undo Redo Interface 2", wFormatA2 == wFormatTest);
    
}

void TestSkInterface::TestSkInterfaceCopy() {
    
}

void TestSkInterface::TestSkInterfaceRebaseConditionalFormat() {
    m_InterfaceUser1->SetActiveWorkBook();
    tBool wOK = m_InterfaceUser1->UndoConditionalFormat("ColorScales", "A1:A10",
        "#FF0000",   // sParam1 = min_color
        "",          // sParam2 = empty (no mid color)
        "#00FF00",   // sParam3 = max_color
        "",          // sParam4 = empty (calculated automatically)
        "",          // sParam5 = empty (calculated automatically)
        "",          // sParam6 = empty (calculated automatically)
        "",          // sParam7 = empty
        "",          // sParam8 = empty
        "",          // sParam9 = empty
        "");         // sParam10 = empty

    tConditionalFormat* wFormatCustomFormula=m_InterfaceUser1->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_ColorScales, "A1:A10");
    CPPUNIT_ASSERT_MESSAGE("CustomFormula  A1:A10",wFormatCustomFormula!=nullptr);
    
    //cout << endl << wFormatCustomFormula->Key()<< endl;
    CPPUNIT_ASSERT_MESSAGE("CustomFormula A1:A10",wFormatCustomFormula->Key()=="A1:A10.CS");

    m_InterfaceUser2->SetActiveWorkBook();
    wFormatCustomFormula=m_InterfaceUser2->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_ColorScales, "A1:A10");
    CPPUNIT_ASSERT_MESSAGE("CustomFormula  A1:A10",wFormatCustomFormula!=nullptr);
    
    //cout << endl << wFormatCustomFormula->Key()<< endl;
    CPPUNIT_ASSERT_MESSAGE("CustomFormula A1:A10",wFormatCustomFormula->Key()=="A1:A10.CS");

    m_InterfaceUser1->SetActiveWorkBook();
    tSheet* wSheet1=m_InterfaceUser1->ActiveSheet();
    //cout << wSheet1->Debug();

    m_InterfaceUser2->SetActiveWorkBook();
    tSheet* wSheet2=m_InterfaceUser1->ActiveSheet();
    //cout << wSheet2->Debug();

    
    m_InterfaceUser2->UndoInsertCol(1, 3);
    
    m_InterfaceUser1->SetActiveWorkBook();
    
    wSheet1=m_InterfaceUser1->ActiveSheet();
    //cout << wSheet1->Debug();

    m_InterfaceUser2->SetActiveWorkBook();
    wSheet2=m_InterfaceUser1->ActiveSheet();
    //cout << wSheet2->Debug();
    
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->Undo();
    wFormatCustomFormula=m_InterfaceUser1->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_ColorScales, "A1:A10");
    CPPUNIT_ASSERT_MESSAGE("CustomFormula  A1:A10",wFormatCustomFormula==nullptr);
    
    wSheet1=m_InterfaceUser1->ActiveSheet();
    //cout << wSheet1->Debug();
}

void TestSkInterface::LoadDocument(tInterfaceWeb* sInterfaceWeb,rapidjson::Document& wDocument) {
    sInterfaceWeb->SetActiveWorkBook();
    tWorkBook* wWorkBook=sInterfaceWeb->ActiveWorkBook();
    tString wSaveUri=wWorkBook->Uri();
    tSpreadSheetContainer::Instance()->JsonBegin();
    wWorkBook->Json(wDocument);
    tSpreadSheetContainer::Instance()->JsonEnd();
    wWorkBook->Uri(wSaveUri);
    sInterfaceWeb->ActiveSheet("Budget mensuel");
}

void TestSkInterface::TestSkInterfaceBudget() {
    tString wFileName="/Users/stephaneallez/Projects/Excel/Budget-familial.sker";
    tFile wFile=tFile(wFileName);
    if (wFile.Exist()) {
        tString wJson = wFile.LoadString();
        
        rapidjson::Document wDocument;
        
        rapidjson::ParseResult wParseResult = wDocument.Parse(wJson.c_str());

        if (wParseResult) {
            
            //cout << endl << wJson << endl;
            LoadDocument(m_InterfaceUser1,wDocument);
            LoadDocument(m_InterfaceServer,wDocument);
            LoadDocument(m_InterfaceUser2,wDocument);
          
            m_InterfaceUser2->SetActiveWorkBook();
            m_InterfaceUser2->UndoCellValue("B5",2);
            m_InterfaceUser2->UndoCellValue("C5",3);
            m_InterfaceUser2->UndoCellValue("D5",4);
          
            m_InterfaceUser1->SetActiveWorkBook();
            m_InterfaceUser1->UndoDeleteCol(1, 1);

            DrawCell(m_InterfaceUser1,"UndoDeleteCol(A)",1,2,20,3);
            
            DrawCell(m_InterfaceUser2,"UndoDeleteCol(A)",1,2,20,3);
            
            m_InterfaceUser1->UndoDeleteCol(3, 1);
            m_InterfaceUser1->UndoDeleteCol(4, 1);
            m_InterfaceUser1->UndoDeleteCol(5, 1);
            m_InterfaceUser1->UndoDeleteCol(6, 1);
            m_InterfaceUser2->SetActiveWorkBook();
            m_InterfaceUser2->InsertCol(2,2);
            
            m_InterfaceUser2->DeleteCol(1,1);
            m_InterfaceUser2->Undo();
            
            
            m_InterfaceUser1->SetActiveWorkBook();
            
            m_InterfaceUser1->Undo();
            m_InterfaceUser1->Undo();
            m_InterfaceUser1->Undo();
            m_InterfaceUser1->Undo();
            m_InterfaceUser1->Undo();
            
            DrawCell(m_InterfaceUser1,"Undo UndoDeleteCol(A)",1,2,20,3);
            
            DrawCell(m_InterfaceUser2,"Undo UndoDeleteCol(A)",1,2,20,3);
            
            
            tIndex wCol=1;
            for(tIndex wInd =1; wInd<20;wInd++) {
                m_InterfaceUser1->SetActiveWorkBook();
                tString wFormat1=m_InterfaceUser1->CellFormat(wInd, wCol);
               
                m_InterfaceUser2->SetActiveWorkBook();
                tString wFormat2=m_InterfaceUser2->CellFormat(wInd, wCol);
#ifdef printdebug
                if (wFormat1!=wFormat2) {
                    cout << Base10ToAlpha(wCol) << wInd << endl;
                    cout << "Loca->" << wFormat1 << endl;
                    cout << "Dist->" << wFormat2 << endl;
                }
#endif
            }
        }
#ifdef checksp
        m_InterfaceUser1->Check();
#endif
    } else {
        //cout << wFileName << "Dont't exist" << endl;
    }
}

void TestSkInterface::TestSkInterfaceBudgetCopyPaste() {
 
    tFile wFile=tFile(CstPathBudget);
    if (wFile.Exist()) {
        tString wJson = wFile.LoadString();
        
        rapidjson::Document wDocument;
        
        rapidjson::ParseResult wParseResult = wDocument.Parse(wJson.c_str());
        
        if (wParseResult) {
            
            //cout << endl << wJson << endl;
            LoadDocument(m_InterfaceUser1,wDocument);
            LoadDocument(m_InterfaceServer,wDocument);
            LoadDocument(m_InterfaceUser2,wDocument);
            
            m_InterfaceUser2->SetActiveWorkBook();
            m_InterfaceUser2->UndoCellValue("B4",1);
            m_InterfaceUser2->UndoCellValue("B5","=B4+1");
            m_InterfaceUser2->Copy("B5");
            
            m_InterfaceUser1->SetActiveWorkBook();
            m_InterfaceUser1->UndoInsertRow(1,1);
            m_InterfaceUser1->UndoInsertCol(1,1);
            m_InterfaceUser1->Undo();
            m_InterfaceUser1->Undo();
            
            
            //cout << endl;
            //cout << "Before Copy " << DebugCell(m_InterfaceUser2,"B6") << endl;
            
            tString wTest = R"(Budget mensuel!B6=:color:#28415F;background-color:#DCE6F2;font:"Trebuchet MS",serif 12pt;text-align:center;vertical-align:middle;)" + kBudgetCellFormatSuffix;
            CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceBudget Copy Paste before Copy", wTest == DebugCell(m_InterfaceUser2,"B6"));
            CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceBudget Copy Paste before Copy", wTest == DebugCell(m_InterfaceUser1,"B6"));
            
            
            m_InterfaceUser2->SetActiveWorkBook();
            m_InterfaceUser2->Undo();
            
            m_InterfaceUser2->Redo();
            
            
            m_InterfaceUser2->UndoPaste("B6:B14");
            
            m_InterfaceUser1->SetActiveWorkBook();
            m_InterfaceUser1->UndoDeleteCol(1,1);
            m_InterfaceUser1->UndoInsertRow(1,1);
            m_InterfaceUser1->UndoInsertCol(1,2);
            
            
            m_InterfaceUser2->SetActiveWorkBook();
            //m_InterfaceUser2->Undo();
            //m_InterfaceUser2->Redo();
            //cout << "A15=" << m_InterfaceUser2->Formula("A15") << endl;
            m_InterfaceUser1->SetActiveWorkBook();
            m_InterfaceUser1->Undo();
            m_InterfaceUser1->Undo();
            m_InterfaceUser1->Undo();
            
            
            /// Agter Copy
            //cout << "After Copy " << DebugCell(m_InterfaceUser1, "B6") << endl;
            wTest = R"(Budget mensuel!B6=B5+1:color:#28415F;background-color:#DCE6F2;font:"Trebuchet MS",serif 12pt;text-align:center;vertical-align:middle;)" + kBudgetCellFormatSuffix;
            CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceBudget Copy Paste after Copy", wTest == DebugCell( m_InterfaceUser2,"B6"));
            CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceBudget Copy Paste after Copy", wTest == DebugCell( m_InterfaceUser1,"B6"));

            m_InterfaceUser2->SetActiveWorkBook();
            // Undo Paste
            m_InterfaceUser2->Undo();
            
            /* DEBUG
            cout << "Undo 1 B6" << DebugCell(m_InterfaceUser1,"B6") << endl;
            cout << "Undo 2 B6" << DebugCell(m_InterfaceUser2,"B6") << endl;
            
            cout << "Undo 1 B14" << DebugCell(m_InterfaceUser1,"B14") << endl;
            cout << "Undo 2 B14" << DebugCell(m_InterfaceUser2,"B14") << endl;
            */
            wTest = R"(Budget mensuel!B6=:color:#28415F;background-color:#DCE6F2;font:"Trebuchet MS",serif 12pt;text-align:center;vertical-align:middle;)" + kBudgetCellFormatSuffix;
            CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceBudget Copy Paste Undo", wTest == DebugCell(m_InterfaceUser2,"B6"));
            CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceBudget Copy Paste Undo", wTest == DebugCell(m_InterfaceUser1,"B6"));
            
            
            m_InterfaceUser1->SetActiveWorkBook();
            m_InterfaceUser1->UndoDeleteCol(1,10);
            m_InterfaceUser1->Undo();
            
            m_InterfaceUser2->SetActiveWorkBook();
            m_InterfaceUser2->Redo();
            /*
            cout << "Undo 1 B6" << DebugCell(m_InterfaceUser1,"B6") << endl;
            cout << "Undo 2 B6" << DebugCell(m_InterfaceUser2,"B6") << endl;
            
            cout << "Undo 1 B14" << DebugCell(m_InterfaceUser1,"B14") << endl;
            cout << "Undo 2 B14" << DebugCell(m_InterfaceUser2,"B14") << endl;
            */
            wTest = R"(Budget mensuel!B7=B6+1:color:#28415F;background-color:#DCE6F2;font:"Trebuchet MS",serif 12pt;text-align:center;vertical-align:middle;)" + kBudgetCellFormatSuffix;
            CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceBudget Copy Paste Undo", wTest == DebugCell(m_InterfaceUser2,"B7"));
            CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceBudget Copy Paste Undo", wTest == DebugCell(m_InterfaceUser1,"B7"));
        }
    }
}

void TestSkInterface::TestSkInterfaceBudgetInsertCopyPaste() {
    tFile wFile=tFile(CstPathBudget);
    if (wFile.Exist()) {
        tString wJson = wFile.LoadString();
        
        rapidjson::Document wDocument;
        
        rapidjson::ParseResult wParseResult = wDocument.Parse(wJson.c_str());
        
        if (wParseResult) {
            
            //cout << endl << wJson << endl;
            LoadDocument(m_InterfaceUser1,wDocument);
            LoadDocument(m_InterfaceServer,wDocument);
            LoadDocument(m_InterfaceUser2,wDocument);
            
            m_InterfaceUser2->SetActiveWorkBook();
            m_InterfaceUser2->UndoCellValue("B4",1);
            m_InterfaceUser2->UndoCellValue("B5","=B4+1");
            m_InterfaceUser2->Copy("B5");
            m_InterfaceUser2->UndoPaste("B6:B14");
            
            m_InterfaceUser1->SetActiveWorkBook();
            m_InterfaceUser1->UndoInsertRow(1,1);
            m_InterfaceUser1->UndoInsertCol(1,1);
          
            m_InterfaceUser1->Undo();
            m_InterfaceUser2->Undo();
            
          
            tTempoPoint wDepl(1,1);
            tTempoPoint wPointB5;
            wPointB5.ParseRef("B5");
            tTempoPoint wPointB5d=wPointB5;
            wPointB5d.Col(wPointB5.Col()+wDepl.Col());
            wPointB5d.Row(wPointB5.Row()+wDepl.Row());
            //cout << endl << wPointB5d.StrRef() << endl;
            tTempoPoint wPointB6;
            wPointB6.ParseRef("B6");
            
            tTempoPoint wPointB6d=wPointB6;
            wPointB6d.Col(wPointB6.Col()+wDepl.Col());
            wPointB6d.Row(wPointB6.Row()+wDepl.Row());
            //cout << endl << wPointB6d.StrRef() << endl;
          
            tCell* wCellC6=m_InterfaceUser2->Cell("C6");
            //cout << wCellC6->Debug() << endl;
            
            m_InterfaceUser2->SetActiveWorkBook();
            // 2 UNDO ==========================================================
            m_InterfaceUser2->Undo();
            m_InterfaceUser2->Undo();
            m_InterfaceUser2->Undo();
            //cout << DebugCell(m_InterfaceUser2,wPointB6d.StrRef())  << endl;
            //cout << DebugCell(m_InterfaceUser2,wPointB5d.StrRef())  << endl;
    
            //wCellC6=m_InterfaceUser2->Cell("C6");
            //cout << wCellC6->Debug() << endl;
            
            // 2 REDO ==========================================================
            m_InterfaceUser2->Redo();
            m_InterfaceUser2->Redo();
            m_InterfaceUser2->Redo();
            //wCellC6=m_InterfaceUser2->Cell("C6");
            //cout << wCellC6->Debug() << endl;
            
            //cout << DebugCell(m_InterfaceUser2,wPointB6d.StrRef())  << endl;
            //cout << DebugCell(m_InterfaceUser2,wPointB5d.StrRef())  << endl;
            //cout << DebugCell(m_InterfaceUser2,wPointB6d.StrRef())  << endl;
            
            m_InterfaceUser1->SetActiveWorkBook();
            m_InterfaceUser1->Undo();
            m_InterfaceUser1->Undo();
            
            //cout << DebugCell(m_InterfaceUser2,"B6")  << endl;
            //cout << DebugCell(m_InterfaceUser2,wPointB5d.StrRef())  << endl;
            tString wTest = R"(Budget mensuel!B6=B5+1:color:#28415F;background-color:#DCE6F2;font:"Trebuchet MS",serif 12pt;text-align:center;vertical-align:middle;)" + kBudgetCellFormatSuffix;
            CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceBudget Copy Paste Undo", wTest == DebugCell(m_InterfaceUser2,"B6"));
            CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceBudget Copy Paste Undo", wTest == DebugCell(m_InterfaceUser1,"B6"));
            
        }
    }
}

void TestSkInterface::TestSkInterfaceDeleteSheet() {
    CPPUNIT_ASSERT_MESSAGE("SetActiveWorkBook User1", m_InterfaceUser1->SetActiveWorkBook());
    CPPUNIT_ASSERT_EQUAL(tString("{\"list\":[\"Sheet1\"]}"), m_InterfaceUser1->ActiveWorkBook()->JsonSheets());
    CPPUNIT_ASSERT_MESSAGE("UndoAddSheet Sheet2", m_InterfaceUser1->UndoAddSheet("Sheet2"));

    m_InterfaceUser1->ActiveSheet("Sheet2");
    m_InterfaceUser1->UndoCellFormat("A1:G10", "color:red;");
    m_InterfaceUser1->UndoCellFormat("A2:B8", "border : 1px solid black;margin : 1px;");
    m_InterfaceUser1->UndoCellFormat("A2:F8", "padding-left : 12px;");

    const tString wFormatSheet2 = "color:red;";
    const tString wFormatA2Merged = "color:red;margin:1px;padding-left:12px;border:solid 1px black;";
    CPPUNIT_ASSERT_EQUAL(wFormatSheet2, m_InterfaceUser1->CellFormat("A1"));
    CPPUNIT_ASSERT_EQUAL(wFormatA2Merged, m_InterfaceUser1->CellFormat("A2"));
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->ActiveSheet("Sheet2");
    CPPUNIT_ASSERT_EQUAL(wFormatSheet2, m_InterfaceUser2->CellFormat("A1"));
    CPPUNIT_ASSERT_EQUAL(wFormatA2Merged, m_InterfaceUser2->CellFormat("A2"));

    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->ActiveSheet("Sheet1");
    m_InterfaceUser1->UndoCellValue("A1", "=Sheet2!E1+Sheet2!E2");
    m_InterfaceUser1->UndoCellValue("A2", "=Sheet2!E1+Sheet2!E2+Sheet2!F1");

    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->ActiveSheet("Sheet2");
    m_InterfaceUser2->UndoCellValue("E1", 10);
    m_InterfaceUser2->UndoCellValue("E2", 20);
    m_InterfaceUser2->UndoCellValue("F1", 30);

    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->ActiveSheet("Sheet1");
    m_InterfaceUser1->ActiveSheet()->Calculate(new tTempoRect(1, 1, 10, 10));
    CPPUNIT_ASSERT_EQUAL(tString("30"), m_InterfaceUser1->Cell("A1")->Value().Str());
    CPPUNIT_ASSERT_EQUAL(tString("60"), m_InterfaceUser1->Cell("A2")->Value().Str());

    CPPUNIT_ASSERT_MESSAGE("UndoDeleteSheet Sheet2", m_InterfaceUser1->UndoDeleteSheet("Sheet2"));

    CPPUNIT_ASSERT_EQUAL(tString("{\"list\":[\"Sheet1\"]}"), m_InterfaceUser1->ActiveWorkBook()->JsonSheets());
    m_InterfaceUser2->SetActiveWorkBook();
    CPPUNIT_ASSERT_EQUAL(tString("{\"list\":[\"Sheet1\"]}"), m_InterfaceUser2->ActiveWorkBook()->JsonSheets());

    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->ActiveSheet("Sheet1");
    CPPUNIT_ASSERT_EQUAL(tString("#REF!+#REF!"), m_InterfaceUser1->Cell("A1")->FormulaStr());
    CPPUNIT_ASSERT_EQUAL(tString("#REF!+#REF!+#REF!"), m_InterfaceUser1->Cell("A2")->FormulaStr());

    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->ActiveSheet("Sheet1");
    CPPUNIT_ASSERT_EQUAL(tString("#REF!+#REF!"), m_InterfaceUser2->Cell("A1")->FormulaStr());
    CPPUNIT_ASSERT_EQUAL(tString("#REF!+#REF!+#REF!"), m_InterfaceUser2->Cell("A2")->FormulaStr());

    CPPUNIT_ASSERT_MESSAGE("Undo DeleteSheet User1", m_InterfaceUser1->SetActiveWorkBook() && m_InterfaceUser1->Undo());

    m_InterfaceUser1->SetActiveWorkBook();
    CPPUNIT_ASSERT_EQUAL(tString("{\"list\":[\"Sheet1\",\"Sheet2\"]}"), m_InterfaceUser1->ActiveWorkBook()->JsonSheets());
    m_InterfaceUser1->ActiveSheet("Sheet1");
    CPPUNIT_ASSERT_EQUAL(tString("Sheet2!E1+Sheet2!E2"), m_InterfaceUser1->Cell("A1")->FormulaStr());
    CPPUNIT_ASSERT_EQUAL(tString("Sheet2!E1+Sheet2!E2+Sheet2!F1"), m_InterfaceUser1->Cell("A2")->FormulaStr());
    m_InterfaceUser1->ActiveSheet()->Calculate(new tTempoRect(1, 1, 10, 10));
    CPPUNIT_ASSERT_EQUAL(tString("30"), m_InterfaceUser1->Cell("A1")->Value().Str());
    CPPUNIT_ASSERT_EQUAL(tString("60"), m_InterfaceUser1->Cell("A2")->Value().Str());

    m_InterfaceUser1->ActiveSheet("Sheet2");
    CPPUNIT_ASSERT_EQUAL(wFormatSheet2, m_InterfaceUser1->CellFormat("A1"));
    CPPUNIT_ASSERT_EQUAL(wFormatA2Merged, m_InterfaceUser1->CellFormat("A2"));
    CPPUNIT_ASSERT_EQUAL(tString("10"), m_InterfaceUser1->Cell("E1")->Value().Str());
    CPPUNIT_ASSERT_EQUAL(tString("20"), m_InterfaceUser1->Cell("E2")->Value().Str());
    CPPUNIT_ASSERT_EQUAL(tString("30"), m_InterfaceUser1->Cell("F1")->Value().Str());

    m_InterfaceUser2->SetActiveWorkBook();
    CPPUNIT_ASSERT_EQUAL(tString("{\"list\":[\"Sheet1\",\"Sheet2\"]}"), m_InterfaceUser2->ActiveWorkBook()->JsonSheets());
    m_InterfaceUser2->ActiveSheet("Sheet1");
    CPPUNIT_ASSERT_EQUAL(tString("Sheet2!E1+Sheet2!E2"), m_InterfaceUser2->Cell("A1")->FormulaStr());
    CPPUNIT_ASSERT_EQUAL(tString("Sheet2!E1+Sheet2!E2+Sheet2!F1"), m_InterfaceUser2->Cell("A2")->FormulaStr());
    m_InterfaceUser2->ActiveSheet()->Calculate(new tTempoRect(1, 1, 10, 10));
    CPPUNIT_ASSERT_EQUAL(tString("30"), m_InterfaceUser2->Cell("A1")->Value().Str());
    CPPUNIT_ASSERT_EQUAL(tString("60"), m_InterfaceUser2->Cell("A2")->Value().Str());

    m_InterfaceUser2->ActiveSheet("Sheet2");
    CPPUNIT_ASSERT_EQUAL(wFormatSheet2, m_InterfaceUser2->CellFormat("A1"));
    CPPUNIT_ASSERT_EQUAL(wFormatA2Merged, m_InterfaceUser2->CellFormat("A2"));
    CPPUNIT_ASSERT_EQUAL(tString("10"), m_InterfaceUser2->Cell("E1")->Value().Str());
    CPPUNIT_ASSERT_EQUAL(tString("20"), m_InterfaceUser2->Cell("E2")->Value().Str());
    CPPUNIT_ASSERT_EQUAL(tString("30"), m_InterfaceUser2->Cell("F1")->Value().Str());
}

void TestSkInterface::setUp() {
	std::filesystem::remove_all("./Spreadsheet");
    // Fill
    m_NbCol=8;
    m_NbRow=8;
    
	m_Application = tApplication::Instance();
	m_Interface = new tInterfaceWeb;
	m_Interface->NewWorkBook("wwww.skeema.fr/w1");
    
    // Format API go to tSpreadSheetContainer
    m_FormatApi=new SkFormat::tFormatCssApi();
    //m_Interface->FormatApi((tFormatApi*)m_FormatApi);
    tSpreadSheetContainer::Instance()->FormatApi((tFormatApi*)m_FormatApi);
    m_FormatRoot = tFormatRoot::Instance();
    
    InitInterface();
};

void TestSkInterface::tearDown() {
    DoneInterface();
    delete(m_Interface);
    m_Interface = nullptr;
    DoneSpreadSheet();
    // Collab Budget tests may leave stray format refs in undo snapshots; always reset
    // FormatRoot so later fixtures (TestSkFormatString, etc.) start from a clean pool.
    delete(m_FormatApi);
    m_FormatApi = nullptr;
    DoneFormatRoot();
}
#endif

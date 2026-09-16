#include "../include/TestSkInterface.hpp"
#include <SkMessage.hpp>
#include <SkJsonKey.hpp>

#ifdef TestMultiUser
void TestSkInterface::DrawCell(tInterfaceWeb* sInterface,tString sOperation, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
#ifdef printdebug
    sInterface->SetActiveWorkBook();
    tWorkBook* wWorkBook=sInterface->ActiveWorkBook();
    cout << wWorkBook->Uri() <<  endl;
    cout << wWorkBook->UndoRebaseLog().DebugOperations() << endl;

    cout << endl;
    cout << sInterface->UriWorkBook() << "!" << sInterface->ActiveSheet()->Name() << ":";
    cout << sOperation << endl;
    for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
        for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
            tCell* wCell=sInterface->Cell(wRow,wCol);
            tVariant wVariant =sInterface->CellValue(wRow, wCol);
            tString wFormula = sInterface->Formula(wRow, wCol);
            cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant << "\t";
        }
        cout << endl;
    }
#endif
}

void TestSkInterface::DebugRange(tInterfaceWeb* sInterface) {
#ifdef printdebug
    cout << "Debug Range" << endl;

    tVectorRange wVectorRange;
    tRect wRect(1,1,m_NbRow,m_NbCol);
    sInterface->ActiveSheet()->ColRowCellRange()->FindRanges(wRect,&wVectorRange);
    for (auto wRange : wVectorRange) {
        cout << wRange->Debug();
    }
#endif
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
            wCell =sInterface->EnsureCell(wRow, wCol);
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

void TestSkInterface::InitInterface() {
    tSpreadSheetContainer* wContainer = tSpreadSheetContainer::Instance();
    wContainer->DeleteWorkBook(m_WorkBookUri + m_UserServerStr);
    wContainer->DeleteWorkBook(m_WorkBookUri + m_User1Str);
    wContainer->DeleteWorkBook(m_WorkBookUri + m_User2Str);
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
}

void TestSkInterface::DoneInterface() {
    m_InterfaceServer->Clear();
    m_InterfaceUser1->Clear();
    m_InterfaceUser2->Clear();
    
    delete(m_InterfaceServer);
    delete(m_InterfaceUser1);
    delete(m_InterfaceUser2);
    delete(m_UserServer);
    delete(m_User1);
    delete(m_User2);
}

void TestSkInterface::TestSkInterfaceValue() {
#ifdef TestMultiUser
    m_InterfaceUser1->SetActiveWorkBook();
    Fill(m_InterfaceUser1);
    
    m_InterfaceUser1->UndoSizeRow( 1, 3, 6.75);
    
    m_InterfaceUser1->UndoCellValue("A1", "Coucou Stéphane");
    m_InterfaceUser1->UndoConditionalFormat("CustomFormulas","A1:A20","% >= 10","color:white;","color:red;","","","","","","","");
    
    tCell* wCellA1  =m_InterfaceUser1->Cell("A1");
    
    tVariant wVariantA1=wCellA1->Value();
#ifdef printdebug
    cout << wCellA1->Sheet()->WorkBook()->Uri() <<"=" <<  m_WorkBookUri+m_User1Str << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("USER 1 WORKBOOK ", wCellA1->Sheet()->WorkBook()->Uri()==m_WorkBookUri+m_User1Str);
    CPPUNIT_ASSERT_MESSAGE("User 1 Is String ", wVariantA1.IsString());
    CPPUNIT_ASSERT_MESSAGE("User 1 = Coucou Stéphane", wVariantA1.String()=="Coucou Stéphane");

    m_InterfaceUser2->SetActiveWorkBook();
    wCellA1=m_InterfaceUser2->Cell("A1");
    CPPUNIT_ASSERT_MESSAGE("Cell A1=nullptr",wCellA1!=nullptr);
#ifdef printdebug
    cout << wCellA1->Sheet()->WorkBook()->Uri() <<"=" <<  m_WorkBookUri+m_User2Str << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("USER 2 WORKBOOK ", wCellA1->Sheet()->WorkBook()->Uri()==m_WorkBookUri+m_User2Str);
    wVariantA1=wCellA1->Value();
#ifdef printdebug
    cout << wVariantA1 << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("User 2 Is String ", wVariantA1.IsString());
    CPPUNIT_ASSERT_MESSAGE("User 2 = Coucou Stéphane", wVariantA1.String()=="Coucou Stéphane");

    m_InterfaceServer->SetActiveWorkBook();
    wCellA1=m_InterfaceServer->Cell("A1");
    CPPUNIT_ASSERT_MESSAGE("SERVER A1 cell exists", wCellA1 != nullptr);
#ifdef printdebug
    cout << wCellA1->Sheet()->WorkBook()->Uri() <<"=" <<  m_WorkBookUri+m_UserServerStr << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("SERVER WORKBOOK  ", wCellA1->Sheet()->WorkBook()->Uri()==m_WorkBookUri+m_UserServerStr);
    wVariantA1=wCellA1->Value();
#ifdef printdebug
    cout << wVariantA1 << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("SERVER A1 Is String ", wVariantA1.IsString());
    CPPUNIT_ASSERT_MESSAGE("SERVER A1 = Coucou Stéphane", wVariantA1.String()=="Coucou Stéphane");

    m_InterfaceUser1->SetActiveWorkBook();
    
    m_InterfaceUser2->UndoSizeRow( 3, 19, 6.75);
   
    m_InterfaceUser2->Undo();
    
    m_InterfaceUser1->Undo();
    
    //m_InterfaceUser1->Undo();
    
#endif
}

void TestSkInterface::TestSkInterfaceInsertDeleteColRow() {
    m_InterfaceUser1->SetActiveWorkBook();
    for (tInt wRow = 1; wRow < 10; wRow++) {
        for (tInt wCol = 1; wCol < 10; wCol++) {
            tVariant wVariant = wRow * 10 + wCol;
            tCell* wCell = m_InterfaceUser1->EnsureCell(wRow, wCol);
            m_InterfaceUser1->UndoCellValue(wCell->StrRef(), wVariant);
        }
    }
    tVariant wVariant = 99;
    
    m_InterfaceUser1->UndoCellValue("A3", "Coucou Stéphane");
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->UndoInsertRow(2,3);
    
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->Undo();
    
    CPPUNIT_ASSERT_MESSAGE("Test Cells Insert Row(2,3) !", m_InterfaceUser1->CellValue(12, 9) == wVariant);
    DrawCell(m_InterfaceUser1, "Test Cells Insert Row(2,3) !",1, 1, 12, 9);

    m_InterfaceUser1->UndoInsertCol(2, 3);
    DrawCell(m_InterfaceUser1,"Test Cells Insert Col(2,3) !", 1, 1, 12, 12);
    CPPUNIT_ASSERT_MESSAGE("Test Cells Insert Col(2,3) !", m_InterfaceUser1->CellValue(12, 12) == wVariant);

    m_InterfaceUser1->UndoDeleteRow(2, 3);
    DrawCell(m_InterfaceUser1,"Test Cells Erase Row(2, 3) !",1, 1, 9, 12);
    CPPUNIT_ASSERT_MESSAGE("Test Cells Erase Row(2,3) !", m_InterfaceUser1->CellValue(9, 12) == wVariant);

    m_InterfaceUser1->DeleteCol(2, 3);
    DrawCell(m_InterfaceUser1,"Test Cells Erase Col(2,3) !", 1, 1, 9, 9);
    CPPUNIT_ASSERT_MESSAGE("Test Cells Erase Col(2,3) !", m_InterfaceUser1->CellValue(9, 9) == wVariant);

    m_InterfaceUser1->DeleteCol(1, 5);
    DrawCell(m_InterfaceUser1,"Test Cells Erase Col(0,5) !", 0, 0, 9, 9);
    CPPUNIT_ASSERT_MESSAGE("Test Cells Erase Col(0,5) !", m_InterfaceUser1->CellValue(9, 4) == wVariant);
}

void TestSkInterface::TestSkInsertDeleteColRowWithCoveredRangeUndo() {
    m_InterfaceUser1->SetActiveWorkBook();
    m_NbRow = 4;
    m_NbCol = 3;
    Fill(m_InterfaceUser1);
    DrawCell(m_InterfaceUser1,"UndoDeleteRow 2,2", 1, 1, 5, 4);
    // Delete in Sheet1
    m_InterfaceUser1->UndoDeleteRow(2, 2);
    
    DrawCell(m_InterfaceUser1,"After UndoDeleteRow 2,2", 1, 1, 5, 4);
#ifdef checksp
    m_InterfaceUser1->Check();
#endif
    m_InterfaceUser1->Undo();
#ifdef checksp
    m_InterfaceUser1->Check();
#endif

    m_NbRow = 8;
    m_NbCol = 10;
    Fill(m_InterfaceUser1);
    
    // Delete in Sheet1
    m_InterfaceUser1->UndoDeleteRow(2, 2);
    
    DrawCell(m_InterfaceUser1,"After UndoDeleteRow", 1, 1, 8, 10);
    DebugRange(m_InterfaceUser1);
#ifdef checksp
    m_InterfaceUser1->Check();
#endif
    m_InterfaceUser1->Undo();
    DrawCell(m_InterfaceUser1,"Undo", 1, 1, 8, 10);
#ifdef checksp
    m_InterfaceUser1->Check();
#endif

    tVariant wVariant = 12;
    m_InterfaceUser1->UndoCellValue("A2", wVariant);
    wVariant = 121;
    m_InterfaceUser1->UndoCellValue("C2", wVariant);

    wVariant = "=B1+B9+B9+B9+B9";
    m_InterfaceUser1->UndoCellValue("B2", wVariant);

    wVariant = "=SUM(B2:C3)";
    m_InterfaceUser1->UndoCellValue("E4", wVariant);

    wVariant = "=SUM(A2:D3)";
    m_InterfaceUser1->UndoCellValue("E5", wVariant);

    // Test Recover =========================================================
    wVariant = "=SUM(A2:D4)";
    m_InterfaceUser1->UndoCellValue("E6", wVariant);

    wVariant = "=SUM(A3:D4)";
    m_InterfaceUser1->UndoCellValue("E7", wVariant);

    wVariant = "=SUM(A4:D4)";
    m_InterfaceUser1->UndoCellValue("E8", wVariant);

    
    // Test Recover
    wVariant = "=SUM(B2:D4)";
    m_InterfaceUser1->UndoCellValue("F6", wVariant);
    wVariant = "=A12+SUM(B3:D4)";
    m_InterfaceUser1->UndoCellValue("F7", wVariant);
    wVariant = "=G46+SUM(B4:D4)";
    m_InterfaceUser1->UndoCellValue("F8", wVariant);
    m_InterfaceUser1->ActiveSheet()->Calculate(new tTempoRect(1, 1, 10, 10));
#ifdef checksp
    m_InterfaceUser1->Check();
#endif
    DrawCell(m_InterfaceUser1,"SetValues",1, 1, 8, 10);
    DebugRange(m_InterfaceUser1);

    m_InterfaceUser1->UndoDeleteRow(2, 2);
    DrawCell(m_InterfaceUser1,"UndoDeleteRow 2,2",1, 1, 8, 10);
    DrawCell(m_InterfaceUser1,"After", 1, 1, 8, 10);
    DebugRange(m_InterfaceUser1);
#ifdef checksp
    m_InterfaceUser1->Check();
#endif
    m_InterfaceUser1->Undo();
    DrawCell(m_InterfaceUser1,"Undo",1, 1, 8, 10);
    DebugRange(m_InterfaceUser1);
#ifdef checksp
    m_InterfaceUser1->Check();
#endif

    DrawCell(m_InterfaceUser1,"UndoDeleteRow 2,2", 1, 1, 8, 10);
    m_InterfaceUser1->UndoDeleteCol(2, 2);
    DrawCell(m_InterfaceUser1,"After", 1, 1, 8, 10);
    DebugRange(m_InterfaceUser1);
#ifdef checksp
    m_InterfaceUser1->Check();
#endif
    m_InterfaceUser1->Undo();
    DrawCell(m_InterfaceUser1,"Undo", 1, 1, 8, 10);
    DebugRange(m_InterfaceUser1);
#ifdef checksp
    m_InterfaceUser1->Check();
#endif
    for (tInt wInd = 1; wInd < 12; wInd++) {
        m_InterfaceUser1->Undo();
    }
#ifdef checksp
    m_InterfaceUser1->Check();
#endif
}

void TestSkInterface::TestSkInterfaceRaz() {
    m_InterfaceUser1->SetActiveWorkBook();
    Fill(m_InterfaceUser1);
    DrawCell(m_InterfaceUser2,"Fill Sheet1", 1, 1, m_NbRow, m_NbCol);
   
    
    m_InterfaceUser1->UndoRaz("A2:Z12");
    
    m_InterfaceUser1->Undo();
}

void TestSkInterface::TestSkInterfaceCopy() {
    m_InterfaceUser1->SetActiveWorkBook();
    //1 Simple Copy
    m_InterfaceUser1->UndoCellValue("A1", tVariant(123));
    m_InterfaceUser1->Copy("A1");
    
    tCell* wCellA1 = m_InterfaceUser1->Cell("A1");
    CPPUNIT_ASSERT_MESSAGE("TestCopy A1", wCellA1 != nullptr);

    m_InterfaceUser1->UndoCellValue("B1", tVariant("Coucou !"));
    tCell* wCellB1= m_InterfaceUser1->Cell("B1");
    
    m_InterfaceUser1->UndoCellValue("C1", tVariant(("=B1")));
    
    tCell* wCellC1 = m_InterfaceUser1->Cell("C1");
    CPPUNIT_ASSERT_MESSAGE("TestCopy C1 (width formula)", wCellB1->Value()==tVariant("Coucou !"));

    m_InterfaceUser1->UndoPaste("B1");
    CPPUNIT_ASSERT_MESSAGE("TestCopy B1", wCellB1->Value()==tVariant(123));
    m_InterfaceUser1->UndoPaste("C1");
    CPPUNIT_ASSERT_MESSAGE("TestCopy C1 (width formula) after copy", wCellB1->Value()==tVariant(123));
    
    tCell* wCellC8 = m_InterfaceUser1->Cell("C8");
    CPPUNIT_ASSERT_MESSAGE("TestCopy before C8=nullptr", wCellC8 == nullptr);

    // 2  copy paste simple (with null after undo copy)
    m_InterfaceUser1->UndoPaste("C8");

    wCellC8 = m_InterfaceUser1->Cell("C8");

    tVariant wVariant = wCellC8->Value();
    CPPUNIT_ASSERT_MESSAGE("TestCopy C8=123", (wVariant.Type() == tVariantType::t_int) &&
                                                (wVariant.Int()==123));

    m_InterfaceUser1->Undo();

    wCellC8 = m_InterfaceUser1->Cell("C8");
    CPPUNIT_ASSERT_MESSAGE("TestCopy C8=nullptr", wCellC8 == nullptr);

    m_InterfaceUser1->Redo();
    m_InterfaceUser1->UndoCellValue("B2", "=A1+12");

    tCell* wCellB2 = m_InterfaceUser1->Cell("B2");
    wVariant = wCellB2->Value();
    CPPUNIT_ASSERT_MESSAGE("TestCopy B2 C8=135", wVariant.Int() == 135);

    m_InterfaceUser1->Copy("B2");

    m_InterfaceUser1->UndoPaste("D9");
    tCell* wCellD9 = m_InterfaceUser1->Cell("D9");
    CPPUNIT_ASSERT_MESSAGE("TestCopy D9", wCellD9 != nullptr);
    //cout << wCellD9->StrRef() << ":" << wCellD9->FormulaStr() << "=" << wCellD9->Value();

    //DrawCell("Final", 1, 1, 10, 10);
    CPPUNIT_ASSERT_MESSAGE("TestCopy B2  C9=135", wVariant.Int() == 135);
}


void TestSkInterface::CreateGenericClass(tInterfaceWeb* sInterface) {
    sInterface->SetActiveWorkBook();
    sInterface->ActiveSheet("Sheet1");
    sInterface->RegisterClassAttribute("SkCellButton","Button","Test");
    
    tBool wOk=sInterface->AddProperty("name", "string", "Name", 0, "Coucou Stéphane");
    CPPUNIT_ASSERT_MESSAGE("Test AddProperty name", wOk == true);
 
    wOk= sInterface->AddProperty("now","date","now",1,"31/12/2024");
    CPPUNIT_ASSERT_MESSAGE("Test AddProperty now", wOk == true);
 
    wOk= sInterface->AddProperty("now","string","now in string ",1,"31/12/2024");
    CPPUNIT_ASSERT_MESSAGE("Test AddProperty now twice", wOk == false);
}

void TestSkInterface::TestSkInterfaceClasss() {
    CreateGenericClass(m_InterfaceUser1);
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->UndoCellClass("A1", "SkCellButton");
    tCell* wCellA1=m_InterfaceUser2->Cell("A1");
    CPPUNIT_ASSERT_MESSAGE("Cell A1==nullptr",wCellA1!=nullptr);
    
    tVariant wVariantA1=wCellA1->Value();
    tCellClassAttribute* wCellClassAttribute=dynamic_cast<tCellClassAttribute*>(wVariantA1.Class());
#ifdef printdebug
    cout << wCellClassAttribute->ClassName() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Test Class SkButton A1",wCellClassAttribute->ClassName()=="SkCellButton");
    
    m_InterfaceUser1->Undo();
    wCellA1=m_InterfaceUser1->Cell("A1");
    CPPUNIT_ASSERT_MESSAGE("Unso Test Class SkButton A1",wCellA1==nullptr);
    
    
    m_InterfaceUser1->Redo();
    
    m_InterfaceUser1->UndoCellValue("B2", tVariant(123));
    m_InterfaceUser1->UndoCellAttribute("A1", "Int", "=B2+1");
    m_InterfaceUser1->UndoCellAttribute("A1", "Temperature", "=A1.Int");
    
    // Get on Workbook User2
    tCellAttribute* wA1_Int = m_InterfaceUser2->CellAttribute("A1", "Int");
    tCellAttribute* wA1_Temperature = m_InterfaceUser2->CellAttribute("A1", "Temperature");

#ifdef printdebug
    cout << wA1_Int->StrRef() << ":" << wA1_Int->FormulaStr(false) << "=" << wA1_Int->Value() << endl;
    cout << wA1_Temperature->StrRef() << ":" << wA1_Temperature->FormulaStr(false) << "=" << wA1_Temperature->Value() << endl;
#endif
    
    CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 2 "+ wA1_Int->StrRef(), wA1_Int->FormulaStr() == "B2+1");
    tString wFormulaTest=wA1_Temperature->FormulaStr();
        CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 2 " + wA1_Temperature->StrRef(), wA1_Temperature->FormulaStr() == "SkCellButton.Int");
    CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 2 " + wA1_Int->StrRef(), wA1_Int->Value().Int() == 124);
    CPPUNIT_ASSERT_MESSAGE("TestUndoRedo 2 " + wA1_Temperature->StrRef(), wA1_Temperature->Value().Int() == 124);
    
    m_InterfaceUser1->Undo();
    m_InterfaceUser1->Undo();
}

void TestSkInterface::TestSkInterfaceNameRange() {
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->UndoCellValue("A1", 123);
    m_InterfaceUser1->UndoInsertRangeNamed("ALLEZY", "A1:A3");
    
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->UndoCellValue("B3", "=SUM(ALLEZY)+1");
    
    m_InterfaceUser1->SetActiveWorkBook();
    tRange* wRange=m_InterfaceUser1->FindRangeNamed("ALLEZY");
    CPPUNIT_ASSERT_MESSAGE("Verify wRange exists", wRange != nullptr);
    CPPUNIT_ASSERT_MESSAGE("Verify wRange Ref Sheet1!A1:A",wRange->StrRef(true)=="Sheet1!A1:A3");
    
    tCell* wCell=m_InterfaceUser1->Cell("B3");
    CPPUNIT_ASSERT_MESSAGE("Verify B3 cell exists", wCell != nullptr);
#ifdef printdebug
    cout << "B3=" << wCell->Value()  << ":" << wCell->FormulaStr() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Verify B3 Value ",wCell->Value().Int() ==124);
    CPPUNIT_ASSERT_MESSAGE("Verify B3 Formula ",wCell->FormulaStr() =="SUM(ALLEZY)+1");
    
   
    m_InterfaceUser1->Undo();
    
    
    wCell=m_InterfaceUser1->Cell("B3");
#ifdef printdebug
    cout << "B3=" << wCell->Value() << ":" << wCell->FormulaStr() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Undo Verify B3 Formula ",wCell->FormulaStr() =="SUM($A$1:$A$3)+1");
    
    m_InterfaceUser1->Redo();
    wCell=m_InterfaceUser1->Cell("B3");
    
#ifdef printdebug
    cout << "B3=" << wCell->Value() << ":" << wCell->FormulaStr() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Redo Verify B3 Formula ",wCell->FormulaStr() =="SUM(ALLEZY)+1");
    
    
    m_InterfaceUser1->UndoDeleteRangeNamed("ALLEZY");

    wCell=m_InterfaceUser1->Cell("B3");
#ifdef printdebug
    cout << "B3=" << wCell->Value() << ":" << wCell->FormulaStr() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Undo Verify B3 Formula User 1",wCell->FormulaStr() =="SUM(#NAME?)+1");
    

    wCell=m_InterfaceUser2->Cell("B3");
#ifdef printdebug
    cout << "B3=" << wCell->Value() << ":" << wCell->FormulaStr() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Undo Verify B3 Formula User 2",wCell->FormulaStr() =="SUM(#NAME?)+1");
    
    m_InterfaceUser1->Undo();
    
    wCell=m_InterfaceUser2->Cell("B3");
#ifdef printdebug
    cout << "B3=" << wCell->Value() << ":" << wCell->FormulaStr() << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Undo Verify B3 Formula ",wCell->FormulaStr() =="SUM(ALLEZY)+1");
}

void TestSkInterface::TestSkInterfaceSheet() {
    m_InterfaceUser1->SetActiveWorkBook();
    
    tWorkBook* wWorkBook=m_InterfaceUser1->ActiveWorkBook();
    
    m_InterfaceUser1->UndoAddSheet("Sheet2");
    tString wJson=wWorkBook->JsonSheets();
#ifdef printdebug
    cout << endl << wJson << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Do AddSheet  ",wJson =="{\"list\":[\"Sheet1\",\"Sheet2\"]}");
    
    
    m_InterfaceUser1->Undo();
    wJson=wWorkBook->JsonSheets();
#ifdef printdebug
    cout << endl << wJson << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Undo AddSheet  ",wJson =="{\"list\":[\"Sheet1\"]}");
    
    m_InterfaceUser1->Redo();
    wJson=wWorkBook->JsonSheets();
#ifdef printdebug
    cout << endl << wJson << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Redo AddSheet  ",wJson =="{\"list\":[\"Sheet1\",\"Sheet2\"]}");

    m_InterfaceUser1->UndoSwapSheet("Sheet1", "Sheet2");
    wJson=wWorkBook->JsonSheets();
#ifdef printdebug
    cout << endl << wJson << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Undo SwapSheet  ",wJson =="{\"list\":[\"Sheet2\",\"Sheet1\"]}");    

    m_InterfaceUser1->Undo();
    wJson=wWorkBook->JsonSheets();
#ifdef printdebug
    cout << endl << wJson << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Undo SwapSheet",wJson =="{\"list\":[\"Sheet1\",\"Sheet2\"]}");
    
    m_InterfaceUser1->UndoRenameSheet("Sheet1", "Rename_Sheet");
    
    wJson=wWorkBook->JsonSheets();
#ifdef printdebug
    cout << endl << wJson << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Undo Rename_Sheet",wJson =="{\"list\":[\"Rename_Sheet\",\"Sheet2\"]}");
    
    m_InterfaceUser1->Undo();
    wJson=wWorkBook->JsonSheets();
#ifdef printdebug
    cout << endl << wJson << endl;
#endif
    CPPUNIT_ASSERT_MESSAGE("Undo SwapSheet",wJson =="{\"list\":[\"Sheet1\",\"Sheet2\"]}");
    
    m_InterfaceUser1->ActiveSheet("Sheet1");
    m_InterfaceUser1->UndoCellValue("A1","=Sheet2!A3+1");
    tCell* wCell=m_InterfaceUser1->Cell("A1");
#ifdef printdebug
    cout << wCell->Debug() << endl;
#endif
    
    m_InterfaceUser1->UndoCellValue("A2","=SUM(Sheet2!B1:B3)");
    wCell=m_InterfaceUser1->Cell("A2");
#ifdef printdebug
    cout << wCell->Debug() << endl;
#endif
    
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->ActiveSheet("Sheet2");
    m_InterfaceUser2->UndoCellValue("A1","=Sheet1!A3+1");
    wCell=m_InterfaceUser2->Cell("A1");
#ifdef printdebug
    cout << wCell->Debug() << endl;
#endif
    
#ifdef printdebug
    m_InterfaceServer->SetActiveWorkBook();
    tWorkBook* wWorkBookServer=m_InterfaceServer->ActiveWorkBook();
    cout << wWorkBookServer->Uri() << ":"  << wWorkBookServer->JsonSheets() << endl;
#endif
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->UndoDeleteSheet("Sheet2");
    
    m_InterfaceUser1->Undo();
    
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->ActiveSheet("Sheet2");
    wCell=m_InterfaceUser2->Cell("A1");
#ifdef printdebug
    cout << wCell->Debug() << endl;
#endif
    
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->ActiveSheet("Sheet1");
    wCell=m_InterfaceUser2->Cell("A1");
#ifdef printdebug
    cout << wCell->Debug() << endl;
#endif
}

void TestSkInterface::TestSkInterfaceConditionalFormat() {
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->UndoConditionalFormat("HighlightCellsRules", "A1:A3", "%>15", "color:red;", "color:blue;", "", "", "", "", "", "","");
    tConditionalFormat* wFormat = m_InterfaceUser1->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_HighlightCellsRules, "A1:A3");
    CPPUNIT_ASSERT(wFormat != nullptr);
    CPPUNIT_ASSERT_EQUAL(wFormat->Type(), tConditionalFormatType::t_HighlightCellsRules);
    CPPUNIT_ASSERT_EQUAL(wFormat->Param2(), tString("color:red;"));

    m_InterfaceUser2->SetActiveWorkBook();
    wFormat = m_InterfaceUser2->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_HighlightCellsRules,"A1:A3");
    CPPUNIT_ASSERT(wFormat != nullptr);
    CPPUNIT_ASSERT_EQUAL(wFormat->Type(), tConditionalFormatType::t_HighlightCellsRules);
    CPPUNIT_ASSERT_EQUAL(wFormat->Param2(), tString("color:red;"));

    m_InterfaceUser1->Undo();
    wFormat = m_InterfaceUser1->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_HighlightCellsRules,"A1:A3");
    CPPUNIT_ASSERT(wFormat == nullptr);

    m_InterfaceUser1->Redo();
    wFormat = m_InterfaceUser1->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_HighlightCellsRules,"A1:A3");
    CPPUNIT_ASSERT(wFormat != nullptr);
    CPPUNIT_ASSERT_EQUAL(wFormat->Type(), tConditionalFormatType::t_HighlightCellsRules);
    CPPUNIT_ASSERT_EQUAL(wFormat->Param2(), tString("color:red;"));

    m_InterfaceUser2->SetActiveWorkBook();
    wFormat = m_InterfaceUser2->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_HighlightCellsRules,"A1:A3");
    CPPUNIT_ASSERT(wFormat != nullptr);
    CPPUNIT_ASSERT_EQUAL(wFormat->Type(), tConditionalFormatType::t_HighlightCellsRules);
    CPPUNIT_ASSERT_EQUAL(wFormat->Param2(), tString("color:red;"));

    m_InterfaceUser1->Undo();
    wFormat = m_InterfaceUser1->ActiveSheet()->ConditionalFormat(tConditionalFormatType::t_HighlightCellsRules,"A1:A3");
    CPPUNIT_ASSERT(wFormat == nullptr);
}


void TestSkInterface::AssertRebase(tString sLabel,tString sRef,tVariant& sValue) {
    m_InterfaceUser1->SetActiveWorkBook();
    tVariant wInter1Value=m_InterfaceUser1->CellValue(sRef);
    
    
    m_InterfaceUser2->SetActiveWorkBook();
    tVariant wInter2Value= m_InterfaceUser2->CellValue(sRef);
    
    CPPUNIT_ASSERT_MESSAGE("Rebase "+sLabel+" Equal ->"+sRef,
                            wInter1Value== wInter2Value);
    
    CPPUNIT_ASSERT_MESSAGE("Rebase "+sLabel+" Bad Value ",
                            wInter2Value == sValue);
}

void TestSkInterface::TestSkInterfaceRebaseFormula() {
    tUndoRedoRebaseFormula wUndoRedoRebaseFormula(static_cast<tAllocatorRef>(1), static_cast<tIndex>(1), static_cast<tIndex>(1), "=Sheet1!A1+1");
    tRebasePlan wRebasePlan;
    wUndoRedoRebaseFormula.Rebase(wRebasePlan);

    // InsertRowByRect (rows 2-3) rebases B2 -> B4; column must not shift with row-only rect.
    tUndoRebaseLog wLog;
    const tAllocatorRef wSheetAlloc = static_cast<tAllocatorRef>(1);
    wLog.RegisterInsertRect(wSheetAlloc, tRect(2, 1, 3, 10), 0, 0, true);
    tRebasePlan wPlan = wLog.BuildPlan(0, wLog.LatestSequence(), wSheetAlloc);
    auto wRebasedB2 = wPlan.RebasePoint(tPoint(2, 2));
    CPPUNIT_ASSERT(wRebasedB2.has_value());
    CPPUNIT_ASSERT_EQUAL(static_cast<tIndex>(4), wRebasedB2->Row());
    CPPUNIT_ASSERT_EQUAL(static_cast<tIndex>(2), wRebasedB2->Col());

    // InsertColByRect rebases column only.
    tUndoRebaseLog wLogCol;
    wLogCol.RegisterInsertRect(wSheetAlloc, tRect(1, 2, 10, 4), 0, 0, false);
    tRebasePlan wPlanCol = wLogCol.BuildPlan(0, wLogCol.LatestSequence(), wSheetAlloc);
    auto wRebasedCol = wPlanCol.RebasePoint(tPoint(2, 2));
    CPPUNIT_ASSERT(wRebasedCol.has_value());
    CPPUNIT_ASSERT_EQUAL(static_cast<tIndex>(2), wRebasedCol->Row());
    CPPUNIT_ASSERT_EQUAL(static_cast<tIndex>(5), wRebasedCol->Col());

    // Move undo refs: InsertRowByRect rows 2-3 shifts rows only (B19 -> B21, B9 -> B11).
    tUndoRebaseLog wLogMove;
    wLogMove.RegisterInsertRect(wSheetAlloc, tRect(2, 1, 3, 10), 0, 0, true);
    tRebasePlan wPlanMove = wLogMove.BuildPlan(0, wLogMove.LatestSequence(), wSheetAlloc);
    auto wDestB19 = wPlanMove.RebasePoint(tPoint(19, 2));
    auto wSrcB9 = wPlanMove.RebasePoint(tPoint(9, 2));
    CPPUNIT_ASSERT(wDestB19.has_value());
    CPPUNIT_ASSERT(wSrcB9.has_value());
    CPPUNIT_ASSERT_EQUAL(static_cast<tIndex>(21), wDestB19->Row());
    CPPUNIT_ASSERT_EQUAL(static_cast<tIndex>(2), wDestB19->Col());
    CPPUNIT_ASSERT_EQUAL(static_cast<tIndex>(11), wSrcB9->Row());
    CPPUNIT_ASSERT_EQUAL(static_cast<tIndex>(2), wSrcB9->Col());
}
void TestSkInterface::TestSkInterfaceRebaseInsertDeleteColRow() {
    // Test rebase of positions when multiple users operate on the same workbook
    // The rebase is needed when:
    // - User1 does an operation (e.g., insert row at position 10)
    // - User2 does an operation (e.g., insert row at position 5)
    // - User1 does an Undo of his operation
    // In this case, User1's operation must be rebased to account for User2's operation
    
    m_InterfaceUser1->SetActiveWorkBook();
    
    // Fill some cells to have data to work with
    for (tInt wRow = 1; wRow < 20; wRow++) {
        for (tInt wCol = 1; wCol < 10; wCol++) {
            tVariant wVariant = wRow * 10 + wCol;
            tCell* wCell = m_InterfaceUser1->EnsureCell(wRow, wCol);
            m_InterfaceUser1->UndoCellValue(wCell->StrRef(), wVariant);
        }
    }
    
    
    tVariant wVariant = 99;
    // Put a value in a cell at A12 (row 11, 0-based)
    m_InterfaceUser1->UndoCellValue("A12", wVariant+1);
    m_InterfaceUser1->UndoCellValue("A12", wVariant);
    
    DrawCell(m_InterfaceUser1, "setValue", 1, 1, 20, 5);
    DrawCell(m_InterfaceUser2, "setValue", 1, 1, 20, 5);
   
    
    // Verify both workbooks are synchronized after User1's operation
    AssertRebase("UndoCellValue","A12",wVariant);
    
    // User2: Insert row at position 5 (before User1's insert at 10)
    // This operation is automatically dispatched to User1 via the dispatcher
    // This should cause User1's operation to be rebased in the log
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->UndoInsertRow(12, 5);
    
    DrawCell(m_InterfaceUser1, "InsertRow 12,5", 1, 1, 20, 5);
    DrawCell(m_InterfaceUser2, "InsertRow 12,5", 1, 1, 20, 5);
    
    // Verify both workbooks are still synchronized after User2's operation
    // After User2 inserts 1 row at position 5, the value that was at A12 is now at A17
    // (because User1 inserted 2 rows at 10, then User2 inserted 1 row at 5)
    AssertRebase("User2 UndoInsertRow","A17",wVariant);
    
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->Undo();
    DrawCell(m_InterfaceUser1, "Undo Insert 12,5", 1, 1, 20, 5);
    DrawCell(m_InterfaceUser2, "Undo Insert 12,5", 1, 1, 20, 5);
    
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->Undo();
    
    tVariant wVariant1=wVariant+1;
    
    DrawCell(m_InterfaceUser1, "Undo CellValue A12", 1, 1, 20, 5);
    DrawCell(m_InterfaceUser2, "Undo CellValue A12", 1, 1, 20, 5);
    
    AssertRebase("User1 Undo CellValue","A12", wVariant1);
    
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->Redo();
  
    DrawCell(m_InterfaceUser1, "Redo Insert 12,5", 1, 1, 20, 5);
    DrawCell(m_InterfaceUser2, "Redo Insert 12,5", 1, 1, 20, 5);
  
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->Undo();
    
    DrawCell(m_InterfaceUser1, "Undo CellValue A12", 1, 1, 20, 5);
    DrawCell(m_InterfaceUser2, "Undo CellValue A12", 1, 1, 20, 5);
    
    tVariant wVariant121(121);
    AssertRebase("User1 Undo CellValue","A17", wVariant121);
    
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->UndoDeleteRow(3, 1);
 

    DrawCell(m_InterfaceUser1, "Redo CellValue A12", 1, 1, 20, 5);
    DrawCell(m_InterfaceUser2, "Redo CellValue A12", 1, 1, 20, 5);

    AssertRebase("User1 Undo CellValue","A16", wVariant121);
    
    // Test 2: User1 deletes a row, then User2 inserts a row, then User1 undos
    
    // Verify synchronization after User1 delete
    //AssertRebase("UndoDeleteRow","A16",wVariant1);
    
    
    DrawCell(m_InterfaceUser1, "DeleteRow 3,1", 1, 1, 20, 5);
    DrawCell(m_InterfaceUser2, "DeleteRow 3,1", 1, 1, 20, 5);
    
    
    // User2: Insert row at position 2 (before User1's delete at 3)
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->UndoInsertRow(1, 2);
    
    
    DrawCell(m_InterfaceUser1, "InsertRow 1,2", 1, 1, 20, 5);
    DrawCell(m_InterfaceUser2, "InsertRow 1,2", 1, 1, 20, 5);
    
    
    // Verify synchronization after User2 insert
    AssertRebase("User2 InsertRow 1,2","A18",wVariant121);
   
    // User1: Undo his delete operation
    // The rebase log should adjust the position to account for User2's insert
    m_InterfaceUser1->SetActiveWorkBook();
    //DrawCell(m_InterfaceUser1, "Rebase Test User1 Final", 16, 1, 30, 10);
    m_InterfaceUser1->Undo();
    
    DrawCell(m_InterfaceUser1, "Undo DeleteRow 3,1", 1, 1, 20, 5);
    DrawCell(m_InterfaceUser2, "Undo DeleteRow 3,1", 1, 1, 20, 5);
    
    //DrawCell(m_InterfaceUser1, "Rebase Test User1 Final", 16, 1, 30, 10);
    
    // Verify synchronization after User1 undo
    AssertRebase("User1 Undo InsertRow 3,1","A19",wVariant121);
    
    // Test 3: User1 inserts column, User2 inserts column before it, User1 undos
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->UndoInsertCol(5, 2);
    tVariant wVariantBefore=90;
    m_InterfaceUser1->UndoCellValue("G12", wVariantBefore);
    
    DrawCell(m_InterfaceUser1, "G12=90", 1, 1, 20, 8);
    DrawCell(m_InterfaceUser2, "G12=90", 1, 1, 20, 8);
    
    // Verify synchronization after User1 insert column
    AssertRebase("UndoCellValye G12 90","G12",wVariantBefore);
    
    m_InterfaceUser1->SetActiveWorkBook();
    tVariant wVariantCol = 88;
    m_InterfaceUser1->UndoCellValue("G12", wVariantCol);
    AssertRebase("User1 InsertCol","G12",wVariantCol);
    
    // User2: Insert column at position 2 (before User1's insert at 5)
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->UndoInsertCol(2, 1);
    
    // Verify synchronization after User2 insert column
    AssertRebase("User2 InsertCol","H12",wVariantCol);
    
    // User1: Undo his insert column operation
    // The rebase log should adjust the position to account for User2's insert
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->Undo();
    
    // Force synchronization by accessing cells like DrawCell does
    // This ensures the Undo is completely applied on both interfaces
    DrawCell(m_InterfaceUser1, "USER1", 10, 1, 20, 10);
    DrawCell(m_InterfaceUser2, "USER2", 10, 1, 20, 10);
    
    // Verify synchronization after User1 undo insert column
    // AssertRebase will now read the correct value after DrawCell has forced synchronization
    AssertRebase("Rebase After User1 Undo InsertCol","H12", wVariantBefore);
    
    
    // Test 4: User1 deletes column, User2 inserts column, User1 undos
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->UndoDeleteCol(1, 1);
    DrawCell(m_InterfaceUser1, "USER1", 10, 1, 20, 10);
    // Verify synchronization after User1 delete column
    m_InterfaceUser1->SetActiveWorkBook();
    AssertRebase("User1 DeleteCol","G12",wVariantBefore);
   
    
    // User2: Insert column at position 0 (before User1's delete at 1)
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->UndoInsertCol(0, 1);
    DrawCell(m_InterfaceUser1, "USER1", 10, 1, 20, 10);
    // Verify synchronization after User2 insert column
    AssertRebase("User2 InsertCol","H12",wVariantBefore);
      
    // User1: Undo his delete column operation
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->Undo();
    
    DrawCell(m_InterfaceUser1, "USER1", 10, 1, 20, 10);
    
    // Verify synchronization after User1 undo delete column
    AssertRebase("User1 Undo DeleteCol","G12",wVariantBefore);
    DrawCell(m_InterfaceUser1, "Rebase Test User1 Final", 1, 1, 15, 10);
}

void TestSkInterface::TestSkInterfaceRebaseMerge() {
    // Test rebase of merge when multiple users operate on the same workbook
    // The rebase is needed when:
    // - User1 does an operation (e.g., merge cells A1:A3)
    // - User2 does an operation (e.g., merge cells B1:B3)
    // - User1 does an Undo of his operation
    // In this case, User1's operation must be rebased to account for User2's operation
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->UndoApplyMerge("B2:D3");
    
    m_InterfaceUser2->SetActiveWorkBook();
    tRange* wRange=m_InterfaceUser2->Range("B2:D3");
    CPPUNIT_ASSERT_MESSAGE("Merge B2:D3 nullptr", wRange!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("Merge B2:D3 nullptr", wRange->IsMerged());
    
    m_InterfaceUser2->UndoInsertCol(2, 3);
    
    m_InterfaceUser1->SetActiveWorkBook();
    //cout << m_InterfaceUser1->ActiveSheet()->Debug();

    wRange=m_InterfaceUser1->Range("E2:G3");
    CPPUNIT_ASSERT_MESSAGE("Merge B2:D3 nullptr", wRange!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("Merge B2:D3 nullptr", wRange->IsMerged());
    
    m_InterfaceUser1->Undo();
    
    wRange=m_InterfaceUser1->Range("E2:G3");
    CPPUNIT_ASSERT_MESSAGE("Merge B2:D3 nullptr", wRange==nullptr);
}

void TestSkInterface::DrawSheet(tString sSheet, tInterfaceWeb* sInterface,tString sOperation, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
    sInterface->SetActiveWorkBook();
    sInterface->ActiveSheet(sSheet);
    DrawCell(sInterface,"--->"+sSheet+"-->"+sOperation ,sRowBegin, sColBegin, sRowEnd, sColEnd);
}

void TestSkInterface::TestSkInterfaceRebaseMultipleSheet() {
    // Test rebase of multiple sheets when multiple users operate on the same workbook
    // The rebase is needed when:
    // - User1 does an operation (e.g., insert row at position 10)
    // - User2 does an operation (e.g., insert row at position 5)
    // - User1 does an Undo of his operation

    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->ActiveSheet("Sheet1");
    m_InterfaceUser1->UndoCellValue("A1", "=1");
    m_InterfaceUser1->UndoCellValue("A2", "=2");
    m_InterfaceUser1->UndoCellValue("A3", "=3");
    
    m_InterfaceUser1->UndoAddSheet("Sheet2");
    m_InterfaceUser1->ActiveSheet("Sheet2");
    m_InterfaceUser1->UndoCellValue("A1", "=Sheet1!A1+1");
    m_InterfaceUser1->UndoCellValue("A2", "=Sheet1!A2+2");
    m_InterfaceUser1->UndoCellValue("A3", "=Sheet1!A3+3");

    m_InterfaceUser1->UndoAddSheet("Sheet3");
    m_InterfaceUser1->UndoCellValue("A1", "=Sheet2!A1+1");
    m_InterfaceUser1->UndoCellValue("A2", "=Sheet2!A2+2");
    m_InterfaceUser1->UndoCellValue("A3", 123);
    m_InterfaceUser1->UndoCellValue("A3", "=Sheet1!A2+30");
    m_InterfaceUser1->UndoCellValue("A3", "=Sheet2!A3+3");

    
    DrawSheet("Sheet1",m_InterfaceUser1,"Init",1,1,4,4);
    DrawSheet("Sheet2",m_InterfaceUser1,"Init",1,1,4,4);
    DrawSheet("Sheet3",m_InterfaceUser1,"Init",1,1,4,4);
    /// Delete ROW 1 ================================================================
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->ActiveSheet("Sheet1");
    m_InterfaceUser2->UndoDeleteRow(1, 1);
   
    DrawSheet("Sheet1",m_InterfaceUser1,"Delete Row",1,1,4,4);
    DrawSheet("Sheet2",m_InterfaceUser1,"Delete Row",1,1,4,4);
    DrawSheet("Sheet3",m_InterfaceUser1,"Delete Row",1,1,4,4);
    
   
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->ActiveSheet("Sheet3");
    tCell* wCell=m_InterfaceUser1->Cell("A1");
    CPPUNIT_ASSERT_MESSAGE("Cell A1 nullptr", wCell!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("Cell A1 Value", wCell->Value().IsError());
    wCell=m_InterfaceUser1->Cell("A2");
    CPPUNIT_ASSERT_MESSAGE("Cell A2 nullptr", wCell!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("Cell A2 Value", wCell->Value().Int() == 6);
    wCell=m_InterfaceUser1->Cell("A3");
    CPPUNIT_ASSERT_MESSAGE("Cell A3 nullptr", wCell!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("Cell A3 Value", wCell->Value().Int() == 9);
    

    // Undo Sheet1 A3
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->Undo();
    
  
    DrawSheet("Sheet3",m_InterfaceUser1,"Undo SHeet3 A3=Sheet2!A3+3 ",1,1,4,4);
    
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->ActiveSheet("Sheet3");
    wCell=m_InterfaceUser2->Cell("A3");
    //cout << endl << wCell->FormulaStr() << endl;
    CPPUNIT_ASSERT_MESSAGE("Cell A3 nullptr", wCell!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("Cell A3 Value", wCell->FormulaStr() == "Sheet1!A2+30");
    CPPUNIT_ASSERT_MESSAGE("Cell A3 Value", wCell->Value().Int() == 33);
    
    // Undo delete Row
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->Undo();
  
    DrawSheet("Sheet1",m_InterfaceUser1,"Undo Delete Row",1,1,4,4);
    DrawSheet("Sheet2",m_InterfaceUser1,"Undo Delete Row",1,1,4,4);
    DrawSheet("Sheet3",m_InterfaceUser1,"Undo Delete Row",1,1,4,4);
    
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->ActiveSheet("Sheet3");
    wCell=m_InterfaceUser2->Cell("A3");
    CPPUNIT_ASSERT_MESSAGE("Cell A3 nullptr", wCell!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("Cell A3 Value", wCell->FormulaStr() == "Sheet1!A3+30");
    CPPUNIT_ASSERT_MESSAGE("Cell A3 Value", wCell->Value().Int() == 33);
}

void TestSkInterface::TestSkInterfaceRebaseInsertDeleteRectRowCol() {
    // Test rebase of positions when multiple users operate with Rect, Row and Col operations
    // The rebase is needed when:
    // - User1 does an operation (e.g., insert row by rect)
    // - User2 does an operation (e.g., insert row at position)
    // - User1 does an Undo of his operation
    // In this case, User1's operation must be rebased to account for User2's operation
    
    m_InterfaceUser1->SetActiveWorkBook();
    
    // Fill some cells to have data to work with
    for (tInt wRow = 1; wRow < 20; wRow++) {
        for (tInt wCol = 1; wCol < 10; wCol++) {
            tVariant wVariant = wRow * 10 + wCol;
            tCell* wCell = m_InterfaceUser1->EnsureCell(wRow, wCol);
            m_InterfaceUser1->UndoCellValue(wCell->StrRef(), wVariant);
        }
    }
    
    tVariant wVariant = 99;
    // Put a value in a cell at E10 (row 9, col 4, 0-based)
    m_InterfaceUser1->UndoCellValue("E10", wVariant+1);
    m_InterfaceUser1->UndoCellValue("E10", wVariant);
    
    DrawCell(m_InterfaceUser1, "setValue", 1, 1, 20, 10);
    DrawCell(m_InterfaceUser2, "setValue", 1, 1, 20, 10);
    
    // Verify both workbooks are synchronized after User1's operation
    AssertRebase("UndoCellValue","E10",wVariant);
    
    // Test 1: User1 inserts rows by rect, User2 inserts row at position, User1 undos
    m_InterfaceUser1->SetActiveWorkBook();
    tRect wInsertRect1(5, 1, 7, 10); // Insert rows after row 5-7
    m_InterfaceUser1->UndoInsertRowByRect(wInsertRect1);
    
    DrawCell(m_InterfaceUser1, "InsertRowByRect 5-7", 1, 1, 25, 10);
    DrawCell(m_InterfaceUser2, "InsertRowByRect 5-7", 1, 1, 25, 10);
    
    // After inserting 3 rows after row 7, E10 becomes E13
    AssertRebase("User1 InsertRowByRect","E13",wVariant);
    
    // User2: Insert row at position 8 (before User1's insert)
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->UndoInsertRow(8, 2);
    
    DrawCell(m_InterfaceUser1, "User2 InsertRow 8,2", 1, 1, 25, 10);
    DrawCell(m_InterfaceUser2, "User2 InsertRow 8,2", 1, 1, 25, 10);
    
    // After User2 inserts 2 rows at position 8, E13 becomes E15
    AssertRebase("User2 InsertRow","E15",wVariant);
    
    // User1: Undo his insert row by rect operation
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->Undo();
    
    DrawCell(m_InterfaceUser1, "Undo InsertRowByRect", 1, 1, 25, 10);
    DrawCell(m_InterfaceUser2, "Undo InsertRowByRect", 1, 1, 25, 10);
    
    // After undo, E15 should become E12 (original E10 + 2 rows from User2)
    AssertRebase("User1 Undo InsertRowByRect","E12",wVariant);
    
    // Test 2: User1 inserts columns by rect, User2 inserts column, User1 undos
    m_InterfaceUser1->SetActiveWorkBook();
    tRect wInsertColRect1(1, 3, 20, 5); // Insert columns after col 3-5
    m_InterfaceUser1->UndoInsertColByRect(wInsertColRect1);
    
    DrawCell(m_InterfaceUser1, "InsertColByRect 3-5", 1, 1, 25, 15);
    DrawCell(m_InterfaceUser2, "InsertColByRect 3-5", 1, 1, 25, 15);
    
    // After inserting 3 columns after col 5, E12 becomes H12
    AssertRebase("User1 InsertColByRect","H12",wVariant);
    
    // User2: Insert column at position 4
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->UndoInsertCol(4, 1);
    
    DrawCell(m_InterfaceUser1, "User2 InsertCol 4,1", 1, 1, 25, 15);
    DrawCell(m_InterfaceUser2, "User2 InsertCol 4,1", 1, 1, 25, 15);
    
    // After User2 inserts 1 column at position 4, H12 becomes I12
    AssertRebase("User2 InsertCol","I12",wVariant);
    
    
    // User1: Undo his insert column by rect operation
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->Undo();
    
    DrawCell(m_InterfaceUser1, "Undo InsertColByRect", 1, 1, 25, 15);
    DrawCell(m_InterfaceUser2, "Undo InsertColByRect", 1, 1, 25, 15);

    // After undo, I12 should become F12 (original E12 + 1 column from User2)
    AssertRebase("User1 Undo InsertColByRect","F12",wVariant);
    
    // Test 3: User1 deletes rows by rect, User2 inserts row, User1 undos
    m_InterfaceUser1->SetActiveWorkBook();
    tRect wDeleteRect1(8, 1, 10, 10); // Delete rows 8-10
    m_InterfaceUser1->UndoDeleteRowByRect(wDeleteRect1);
    
    DrawCell(m_InterfaceUser1, "DeleteRowByRect 8-10", 1, 1, 25, 15);
    DrawCell(m_InterfaceUser2, "DeleteRowByRect 8-10", 1, 1, 25, 15);
    
    // After deleting rows 8-10, F12 becomes F9
    AssertRebase("User1 DeleteRowByRect","F9",wVariant);
    
    // User2: Insert row at position 5
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->UndoInsertRow(5, 1);
    
    DrawCell(m_InterfaceUser1, "User2 InsertRow 5,1", 1, 1, 13, 10);
    DrawCell(m_InterfaceUser2, "User2 InsertRow 5,1", 1, 1, 13, 10);
    
    // After User2 inserts 1 row at position 5, F9 becomes F10
    AssertRebase("User2 InsertRow","F10",wVariant);
    
    // User1: Undo his delete row by rect operation
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->Undo();
    
    DrawCell(m_InterfaceUser1, "Undo DeleteRowByRect", 1, 1, 13, 10);
    DrawCell(m_InterfaceUser2, "Undo DeleteRowByRect", 1, 1, 13, 10);
    
    // After undo, F10 should become F13 (original F12 + 1 row from User2)
    AssertRebase("User1 Undo DeleteRowByRect","F13",wVariant);
    
    // Test 4: User1 deletes columns by rect, User2 inserts column, User1 undos
    m_InterfaceUser1->SetActiveWorkBook();
    tRect wDeleteColRect1(1, 2, 20, 4); // Delete columns 2-4
    m_InterfaceUser1->UndoDeleteColByRect(wDeleteColRect1);
    
    DrawCell(m_InterfaceUser1, "DeleteColByRect 2-4", 1, 1, 13, 10);
    DrawCell(m_InterfaceUser2, "DeleteColByRect 2-4", 1, 1, 13, 10);
    
    // After deleting columns 2-4, F13 becomes C13
    AssertRebase("User1 DeleteColByRect","C13",wVariant);
    
    // User2: Insert column at position 1
    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->UndoInsertCol(1, 1);
    
    DrawCell(m_InterfaceUser1, "User2 InsertCol 1,1", 1, 1, 25, 15);
    DrawCell(m_InterfaceUser2, "User2 InsertCol 1,1", 1, 1, 25, 15);
    
    // After User2 inserts 1 column at position 1, C13 becomes D13
    AssertRebase("User2 InsertCol","D13",wVariant);
    
    m_InterfaceUser1->SetActiveWorkBook();
    tWorkBook* wWorkBook1=m_InterfaceUser1->ActiveWorkBook();
    //cout << "User 1 " << wWorkBook1->Uri() <<  endl;
    //cout << wWorkBook1->UndoRebaseLog().DebugOperations() << endl;
    m_InterfaceUser2->SetActiveWorkBook();
    tWorkBook* wWorkBook2=m_InterfaceUser2->ActiveWorkBook();
    //cout << "User 2 " << wWorkBook2->Uri() << endl;
    //cout << wWorkBook2->UndoRebaseLog().DebugOperations() << endl;
    
    // User1: Undo his delete column by rect operation
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->Undo();
    
    DrawCell(m_InterfaceUser1, "Undo DeleteColByRect", 1, 1, 25, 15);
    DrawCell(m_InterfaceUser2, "Undo DeleteColByRect", 1, 1, 25, 15);
    
    // After undo, D13 should become F14 (original F13 + 1 column from User2)
    AssertRebase("User1 Undo DeleteColByRect","G13",wVariant);
    
    DrawCell(m_InterfaceUser1, "Rebase Test Rect Row Col Final", 1, 1, 25, 15);
}

void TestSkInterface::TestSkInterfaceRebaseCellValueUndoAfterInsertRowByRect() {
    // User1 writes 123 in B2, User2 inserts rows 2-3 by rect, User1 undoes the cell value.
    // The undo must target the rebased cell (B4), and both clients must stay synchronized.

    m_InterfaceUser1->SetActiveWorkBook();

    tVariant wVariant123(123);
    m_InterfaceUser1->UndoCellValue("B2", wVariant123);

    DrawCell(m_InterfaceUser1, "User1 set B2", 1, 1, 8, 5);
    DrawCell(m_InterfaceUser2, "User1 set B2", 1, 1, 8, 5);
    AssertRebase("User1 UndoCellValue B2", "B2", wVariant123);

    m_InterfaceUser2->SetActiveWorkBook();
    tRect wInsertRect(2, 1, 3, 10);
    m_InterfaceUser2->UndoInsertRowByRect(wInsertRect);

    DrawCell(m_InterfaceUser1, "User2 InsertRowByRect 2-3", 1, 1, 10, 5);
    DrawCell(m_InterfaceUser2, "User2 InsertRowByRect 2-3", 1, 1, 10, 5);

    // Two rows inserted at 2-3 shift the original B2 content down to B4.
    AssertRebase("User2 InsertRowByRect", "B4", wVariant123);

    m_InterfaceUser1->SetActiveWorkBook();
    tUndoSpreadSheet* wUndoBeforeUndo =
        dynamic_cast<tUndoSpreadSheet*>(m_InterfaceUser1->LastUndo());
    CPPUNIT_ASSERT_MESSAGE("User1 last undo before Undo()", wUndoBeforeUndo != nullptr);
    // Do not call Rebase() here — tInterfaceWeb::Undo() rebases once before applying undo.

    CPPUNIT_ASSERT_MESSAGE("User1 Undo()", m_InterfaceUser1->Undo());

    DrawCell(m_InterfaceUser1, "User1 Undo CellValue", 1, 1, 10, 5);
    DrawCell(m_InterfaceUser2, "User1 Undo CellValue", 1, 1, 10, 5);

    auto wAssertNo123 = [&](tString sRef) {
        m_InterfaceUser1->SetActiveWorkBook();
        tVariant wValueUser1 = m_InterfaceUser1->CellValue(sRef);
        m_InterfaceUser2->SetActiveWorkBook();
        tVariant wValueUser2 = m_InterfaceUser2->CellValue(sRef);
        CPPUNIT_ASSERT_MESSAGE("Rebase User1 Undo CellValue sync " + sRef,
            wValueUser1 == wValueUser2);
        if (wValueUser1.Type() == tVariantType::t_int) {
            CPPUNIT_ASSERT_MESSAGE("Rebase User1 Undo CellValue no 123 at " + sRef,
                wValueUser1.Int() != 123);
        }
    };

    wAssertNo123("B4");
    wAssertNo123("B2");
}

void TestSkInterface::TestSkInterfaceRebaseMoveUndoAfterInsertRowByRect() {
    // User1 moves B9:E15 -> B19:E25, User2 inserts rows 2-3, User1 undoes the move.
    // Rebase must target dest B21:E27 and source B11:E17 (row shift only, columns unchanged).

    m_InterfaceUser1->SetActiveWorkBook();

    tVariant wMarker(123);
    m_InterfaceUser1->UndoCellValue("B9", wMarker);

    CPPUNIT_ASSERT_MESSAGE("User1 UndoMove B9:E15 -> B19:E25",
        m_InterfaceUser1->UndoMove("B9:E15", "B19:E25"));

    DrawCell(m_InterfaceUser1, "User1 Move", 1, 1, 25, 10);
    DrawCell(m_InterfaceUser2, "User1 Move", 1, 1, 25, 10);
    AssertRebase("User1 Move dest B19", "B19", wMarker);

    m_InterfaceUser2->SetActiveWorkBook();
    m_InterfaceUser2->UndoInsertRowByRect(tRect(2, 1, 3, 10));

    DrawCell(m_InterfaceUser1, "User2 InsertRowByRect 2-3", 1, 1, 25, 10);
    DrawCell(m_InterfaceUser2, "User2 InsertRowByRect 2-3", 1, 1, 25, 10);
    AssertRebase("User2 InsertRowByRect shifted dest", "B21", wMarker);

    m_InterfaceUser1->SetActiveWorkBook();
    tUndoSpreadSheet* wUndoBeforeUndo =
        dynamic_cast<tUndoSpreadSheet*>(m_InterfaceUser1->LastUndo());
    CPPUNIT_ASSERT_MESSAGE("User1 last undo is move", wUndoBeforeUndo != nullptr);
    CPPUNIT_ASSERT_EQUAL(tString("tUndoMove"), wUndoBeforeUndo->ClassName());
    // Do not call Rebase() here — tInterfaceWeb::Undo() rebases once before applying undo.

    tWorkBook* wWorkBook = m_InterfaceUser1->ActiveWorkBook();
    const tAllocatorRef wSheetAlloc = m_InterfaceUser1->ActiveSheet()->AllocatorRef();
    tRebasePlan wPlan = wWorkBook->UndoRebaseLog().BuildPlan(
        0, wWorkBook->UndoRebaseLog().LatestSequence(), wSheetAlloc);
    tSelect wDestSelect;
    CPPUNIT_ASSERT(wDestSelect.Parse("B19:E25"));
    CPPUNIT_ASSERT(wDestSelect.Rebase(wPlan));
    CPPUNIT_ASSERT_EQUAL(tString("B21:E27"), wDestSelect.StrRef());
    tSelect wSourceSelect;
    CPPUNIT_ASSERT(wSourceSelect.Parse("B9:E15"));
    CPPUNIT_ASSERT(wSourceSelect.Rebase(wPlan));
    CPPUNIT_ASSERT_EQUAL(tString("B11:E17"), wSourceSelect.StrRef());

    CPPUNIT_ASSERT_MESSAGE("User1 Undo Move", m_InterfaceUser1->Undo());

    DrawCell(m_InterfaceUser1, "User1 Undo Move", 1, 1, 25, 10);
    DrawCell(m_InterfaceUser2, "User1 Undo Move", 1, 1, 25, 10);

    AssertRebase("User1 Undo Move restored source", "B11", wMarker);

    auto wAssertNoMarker = [&](tString sRef) {
        m_InterfaceUser1->SetActiveWorkBook();
        tVariant wValueUser1 = m_InterfaceUser1->CellValue(sRef);
        m_InterfaceUser2->SetActiveWorkBook();
        tVariant wValueUser2 = m_InterfaceUser2->CellValue(sRef);
        CPPUNIT_ASSERT_MESSAGE("Rebase User1 Undo Move sync " + sRef,
            wValueUser1 == wValueUser2);
        if (wValueUser1.Type() == tVariantType::t_int) {
            CPPUNIT_ASSERT_MESSAGE("Rebase User1 Undo Move no marker at " + sRef,
                wValueUser1.Int() != 123);
        }
    };

    wAssertNoMarker("B21");
    wAssertNoMarker("B19");
}

void TestSkInterface::TestSkInterfaceRedoInsertColRowByRect() {
    // Regression (Client mode). tInterfaceWeb::Redo() used to skip registering the
    // redone structural op in the UndoRebaseLog, while Do()/Undo() registered theirs.
    // After redoing an Insert Col by rect, redoing the Insert Row by rect built its
    // rebase plan against a stale log, Rebase() returned false and Redo() called
    // ClearUndoRedo() — wiping the whole undo/redo stack so every later Redo/Undo
    // silently did nothing. Reproduces the reported sequence exactly:
    // Insert Col by rect, Insert Row by rect, Undo, Undo, Redo, Redo.

    m_InterfaceUser1->SetActiveWorkBook();

    // Fill a small grid so the structural operations have data to shift.
    for (tInt wRow = 1; wRow <= 12; wRow++) {
        for (tInt wCol = 1; wCol <= 8; wCol++) {
            tVariant wVariant = wRow * 100 + wCol;
            tCell* wCell = m_InterfaceUser1->EnsureCell(wRow, wCol);
            m_InterfaceUser1->UndoCellValue(wCell->StrRef(), wVariant);
        }
    }

    // Snapshot (formula-independent) of the working area for round-trip comparisons.
    auto wSnapshot = [&]() -> tString {
        m_InterfaceUser1->SetActiveWorkBook();
        tStringStream wStream;
        for (tIndex wRow = 1; wRow <= 14; ++wRow) {
            for (tIndex wCol = 1; wCol <= 10; ++wCol) {
                wStream << Base10ToAlpha(wCol) << wRow << "="
                        << m_InterfaceUser1->CellValue(wRow, wCol) << ";";
            }
        }
        return wStream.str();
    };

    const tString wBefore = wSnapshot();

    // D7:E7 -> tRect(Top, Left, Bottom, Right).
    const tRect wRect(7, 4, 7, 5);

    // Do: Insert Col by rect, then Insert Row by rect.
    CPPUNIT_ASSERT_MESSAGE("Do InsertColByRect", m_InterfaceUser1->UndoInsertColByRect(wRect));
    CPPUNIT_ASSERT_MESSAGE("Do InsertRowByRect", m_InterfaceUser1->UndoInsertRowByRect(wRect));
    const tString wAfterDo = wSnapshot();
    CPPUNIT_ASSERT_MESSAGE("Inserts changed the grid", wAfterDo != wBefore);

    // Undo both (row first, then col).
    CPPUNIT_ASSERT_MESSAGE("Undo InsertRowByRect", m_InterfaceUser1->Undo());
    CPPUNIT_ASSERT_MESSAGE("Undo InsertColByRect", m_InterfaceUser1->Undo());
    CPPUNIT_ASSERT_MESSAGE("Undo restored the grid", wSnapshot() == wBefore);

    // Redo both (col first, then row). The second Redo is the regression: before the
    // fix it returned false and cleared the stack; it must now reapply the row insert.
    CPPUNIT_ASSERT_MESSAGE("Redo InsertColByRect", m_InterfaceUser1->Redo());
    CPPUNIT_ASSERT_MESSAGE("Redo InsertRowByRect", m_InterfaceUser1->Redo());
    CPPUNIT_ASSERT_MESSAGE("Redo reapplied both inserts", wSnapshot() == wAfterDo);

    // The undo/redo stack must still be alive (ClearUndoRedo would have emptied it).
    CPPUNIT_ASSERT_MESSAGE("Undo still works after Redo", m_InterfaceUser1->Undo());
    CPPUNIT_ASSERT_MESSAGE("Undo still works after Redo (2)", m_InterfaceUser1->Undo());
    CPPUNIT_ASSERT_MESSAGE("Undo restored the grid after full redo cycle", wSnapshot() == wBefore);
}

void TestSkInterface::TestSkInterfaceRedoDeleteColRowByRect() {
    // Same regression as TestSkInterfaceRedoInsertColRowByRect but for Delete by rect.
    // The Redo registration / doesrow / opid fixes are generic (Insert AND Delete,
    // Row AND Col by rect), so a Delete Col by rect redo followed by a Delete Row by
    // rect redo must both succeed and keep the undo/redo stack alive.
    // Uses the reported rectangle D7:F10.

    m_InterfaceUser1->SetActiveWorkBook();

    // Fill a grid large enough to survive the deleted rectangle.
    for (tInt wRow = 1; wRow <= 14; wRow++) {
        for (tInt wCol = 1; wCol <= 10; wCol++) {
            tVariant wVariant = wRow * 100 + wCol;
            tCell* wCell = m_InterfaceUser1->EnsureCell(wRow, wCol);
            m_InterfaceUser1->UndoCellValue(wCell->StrRef(), wVariant);
        }
    }

    auto wSnapshot = [&]() -> tString {
        m_InterfaceUser1->SetActiveWorkBook();
        tStringStream wStream;
        for (tIndex wRow = 1; wRow <= 16; ++wRow) {
            for (tIndex wCol = 1; wCol <= 12; ++wCol) {
                wStream << Base10ToAlpha(wCol) << wRow << "="
                        << m_InterfaceUser1->CellValue(wRow, wCol) << ";";
            }
        }
        return wStream.str();
    };

    const tString wBefore = wSnapshot();

    // D7:F10 -> tRect(Top, Left, Bottom, Right).
    const tRect wRect(7, 4, 10, 6);

    // Do: Delete Col by rect, then Delete Row by rect.
    CPPUNIT_ASSERT_MESSAGE("Do DeleteColByRect", m_InterfaceUser1->UndoDeleteColByRect(wRect));
    CPPUNIT_ASSERT_MESSAGE("Do DeleteRowByRect", m_InterfaceUser1->UndoDeleteRowByRect(wRect));
    const tString wAfterDo = wSnapshot();
    CPPUNIT_ASSERT_MESSAGE("Deletes changed the grid", wAfterDo != wBefore);

    // Undo both (row first, then col).
    CPPUNIT_ASSERT_MESSAGE("Undo DeleteRowByRect", m_InterfaceUser1->Undo());
    CPPUNIT_ASSERT_MESSAGE("Undo DeleteColByRect", m_InterfaceUser1->Undo());
    CPPUNIT_ASSERT_MESSAGE("Undo restored the grid", wSnapshot() == wBefore);

    // Redo both (col first, then row). The second Redo is the regression check.
    CPPUNIT_ASSERT_MESSAGE("Redo DeleteColByRect", m_InterfaceUser1->Redo());
    CPPUNIT_ASSERT_MESSAGE("Redo DeleteRowByRect", m_InterfaceUser1->Redo());
    CPPUNIT_ASSERT_MESSAGE("Redo reapplied both deletes", wSnapshot() == wAfterDo);

    // The undo/redo stack must still be alive after the redo cycle.
    CPPUNIT_ASSERT_MESSAGE("Undo still works after Redo", m_InterfaceUser1->Undo());
    CPPUNIT_ASSERT_MESSAGE("Undo still works after Redo (2)", m_InterfaceUser1->Undo());
    CPPUNIT_ASSERT_MESSAGE("Undo restored the grid after full redo cycle", wSnapshot() == wBefore);
}

void TestSkInterface::TestSkInterfaceData() {
    // Test RangeData operations in multi-user context
    // Verify that RangeData operations are synchronized between users
    
    m_InterfaceUser1->SetActiveWorkBook();
    
    // Test 1: User1 inserts RangeData, verify User2 sees it
    // filtervalue must be a tVariant JSON object with "t" (type) and "v" (value) keys
    tString wJsonData = "{\"columns\":[{\"index\":1,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"s\",\"v\":\"Dupont\"},\"order\":\"Ascending\"},{\"index\":2,\"name\":\"Age\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"s\",\"v\":\"Jean\"},\"order\":\"Ascending\"}]}";
    
    tBool wResult = m_InterfaceUser1->UndoInsertRangeData("TEST_DATA", "A1:B2", wJsonData);
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Insert should succeed", wResult == true);
    
    // Verify the named range exists on User1
    tRange* wRange = m_InterfaceUser1->FindRangeNamed("TEST_DATA");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range should exist on User1", wRange != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range should be named", wRange->IsNamed());
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range reference User1", wRange->StrRef(true) == "Sheet1!A1:B2");
    
    // Verify RangeData exists on User1
    tRangeData* wRangeData = m_InterfaceUser1->RangeData("TEST_DATA");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: RangeData should exist on User1", wRangeData != nullptr);
    
    // Verify synchronization: User2 should also see the RangeData
    m_InterfaceUser2->SetActiveWorkBook();
    wRange = m_InterfaceUser2->FindRangeNamed("TEST_DATA");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range should exist on User2 (synchronization)", wRange != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range reference User2", wRange->StrRef(true) == "Sheet1!A1:B2");
    
    wRangeData = m_InterfaceUser2->RangeData("TEST_DATA");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: RangeData should exist on User2 (synchronization)", wRangeData != nullptr);
    
    // Test 2: Undo/Redo functionality
    m_InterfaceUser1->SetActiveWorkBook();
    tString wJsonDataUndo = "{\"columns\":[{\"index\":1,\"name\":\"Column1\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"}]}";
    
    wResult = m_InterfaceUser1->UndoInsertRangeData("TEST_UNDO", "C1:C3", wJsonDataUndo);
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Insert TEST_UNDO should succeed", wResult == true);
    
    // Verify the named range exists before undo
    wRange = m_InterfaceUser1->FindRangeNamed("TEST_UNDO");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range TEST_UNDO should exist before undo", wRange != nullptr);
    
    wRangeData = m_InterfaceUser1->RangeData("TEST_UNDO");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: RangeData TEST_UNDO should exist before undo", wRangeData != nullptr);
    
    // Undo the operation
    m_InterfaceUser1->Undo();
    
    // Verify the named range no longer exists on User1
    wRange = m_InterfaceUser1->FindRangeNamed("TEST_UNDO");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range TEST_UNDO should not exist after undo", wRange == nullptr);
    
    // Verify synchronization: User2 should also see the undo
    m_InterfaceUser2->SetActiveWorkBook();
    wRange = m_InterfaceUser2->FindRangeNamed("TEST_UNDO");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range TEST_UNDO should not exist on User2 after undo", wRange == nullptr);
    
    // Redo the operation
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->Redo();
    
    // Verify the named range exists again on User1
    wRange = m_InterfaceUser1->FindRangeNamed("TEST_UNDO");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range TEST_UNDO should exist after redo", wRange != nullptr);
    
    // Verify synchronization: User2 should also see the redo
    m_InterfaceUser2->SetActiveWorkBook();
    wRange = m_InterfaceUser2->FindRangeNamed("TEST_UNDO");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range TEST_UNDO should exist on User2 after redo", wRange != nullptr);
    
    // Test 3: Multiple columns with different types
    m_InterfaceUser1->SetActiveWorkBook();
    tString wJsonDataColumns = "{\"columns\":["
        "{\"index\":1,\"name\":\"Name\",\"type\":\"string\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Ascending\"},"
        "{\"index\":2,\"name\":\"Age\",\"type\":\"int\",\"filterop\":\"None\",\"filtervalue\":{\"t\":\"n\",\"v\":null},\"order\":\"Descending\"},"
        "{\"index\":3,\"name\":\"Salary\",\"type\":\"double\",\"filterop\":\"GreaterThan\",\"filtervalue\":{\"t\":\"d\",\"v\":1000},\"order\":\"Ascending\"}"
        "]}";
    
    wResult = m_InterfaceUser1->UndoInsertRangeData("TEST_COLUMNS", "D1:F5", wJsonDataColumns);
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Insert TEST_COLUMNS should succeed", wResult == true);
    
    // Verify the named range exists on User1
    wRange = m_InterfaceUser1->FindRangeNamed("TEST_COLUMNS");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range TEST_COLUMNS should exist", wRange != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range TEST_COLUMNS reference", wRange->StrRef(true) == "Sheet1!D1:F5");
    
    // Verify synchronization: User2 should also see the RangeData
    m_InterfaceUser2->SetActiveWorkBook();
    wRange = m_InterfaceUser2->FindRangeNamed("TEST_COLUMNS");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range TEST_COLUMNS should exist on User2", wRange != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range TEST_COLUMNS reference User2", wRange->StrRef(true) == "Sheet1!D1:F5");
    
    // Test deletion
    m_InterfaceUser1->SetActiveWorkBook();
    m_InterfaceUser1->UndoDeleteRangeNamed("TEST_COLUMNS");
    wRange = m_InterfaceUser1->FindRangeNamed("TEST_COLUMNS");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range TEST_COLUMNS should not exist after delete", wRange == nullptr);
    
    // Verify synchronization: User2 should also see the deletion
    m_InterfaceUser2->SetActiveWorkBook();
    wRange = m_InterfaceUser2->FindRangeNamed("TEST_COLUMNS");
    CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceData: Range TEST_COLUMNS should not exist on User2 after delete", wRange == nullptr);
    
    m_InterfaceUser1->Undo();
}


void TestSkInterface::setUp() {
    std::filesystem::remove_all("./Spreadsheet");

    m_Application = tApplication::Instance();
     // Fill
    m_NbCol=8;
    m_NbRow=8;

    InitInterface();
};

void TestSkInterface::tearDown() {
    DoneInterface();
    
}

#endif

//=============================================================================
// TestSkFormulaNamed
//=============================================================================
#include "../include/TestSkFormulaNamed.hpp"

TestSkFormulaNamed::TestSkFormulaNamed() : CPPUNIT_NS::TestFixture(), m_Application(nullptr), m_Api(nullptr) {
    m_Application = nullptr;
    m_Api = nullptr;
}

void TestSkFormulaNamed::InsertFormulaNamed() {
    tBool wResult = m_Api->UndoInsertFormulaNamed("TEST1", "1+2");
    CPPUNIT_ASSERT_MESSAGE("TestInsertFormulaNamed 1", wResult);
    
    tWorkBook* wWorkBook=tSpreadSheetContainer::Instance()->ActiveWorkBook();
    tSheet* wSheetNamedFormula=wWorkBook->SheetNamedFormula();
    
    tCell* wCellFormula1=wSheetNamedFormula->Cell(1,1);
    CPPUNIT_ASSERT_MESSAGE("TestInsertFormulaNamed $$!A1 TEST1 !=nullptr", wCellFormula1!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestInsertFormulaNamed $$!A1 TEST1 =3 ",wCellFormula1->Value()==3);
    CPPUNIT_ASSERT_MESSAGE("TestInsertFormulaNamed $$!A1 TEST1 =1+2 ",wCellFormula1->FormulaStr()=="1+2");
    
    
    //cout << endl << wCellFormula1->Value() << ":" << wCellFormula1->FormulaStr() << endl;
    // Call TEST1
    wResult=m_Api->UndoCellValue("A1","=TEST1");
    tCell* wCellA1=m_Api->Cell("A1");
    //cout << endl << wCellA1->Value() << ":" << wCellA1->FormulaStr() << endl;
    
//    cout << wCellFormula1->Debug() << endl;
    
    CPPUNIT_ASSERT_MESSAGE("TestInsertFormulaNamed TEST1 !=nullptr", wCellA1!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestInsertFormulaNamed TEST1 =3 ",wCellA1->Value()==3);
    CPPUNIT_ASSERT_MESSAGE("TestInsertFormulaNamed TEST1 =1+2 ",wCellA1->FormulaStr()=="TEST1");

    //CPPUNIT_ASSERT_MESSAGE("TestInsertFormulaNamed 1", m_Api->FormulaNamed("TEST1") == "=1+2");
    wResult = m_Api->UndoInsertFormulaNamed("TEST2", "Sheet1!A1*2");
      // Call TEST1
    wResult=m_Api->UndoCellValue("A2","=TEST2");
    //cout << endl << m_Api->Cell("A2")->Value() << ":" << m_Api->Cell("A2")->FormulaStr() << endl;
    
    m_Api->UndoDeleteFormulaNamed("TEST1");
    
    wCellA1=m_Api->Cell("A1");
   //cout << endl << wCellA1->Value() << ":" << wCellA1->FormulaStr() << endl;
 
    m_Api->Undo();
    wCellA1=m_Api->Cell("A1");
    //cout << endl << wCellA1->Value() << ":" << wCellA1->FormulaStr() << endl;
}

void TestSkFormulaNamed::JsonFormulaNamed() {
    const tString wName = "JSONTEST";
    const tString wFormula = "5+7";
    tBool wResult = m_Api->UndoInsertFormulaNamed(wName, wFormula);
    CPPUNIT_ASSERT_MESSAGE("JsonFormulaNamed: InsertFormulaNamed", wResult);

    tWorkBook* wWorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
    CPPUNIT_ASSERT_MESSAGE("JsonFormulaNamed: ActiveWorkBook", wWorkBook != nullptr);
    tString wFormulaStr = wWorkBook->RangeNamedContainer()->FormulaNamedStr(wName);
    CPPUNIT_ASSERT_MESSAGE("JsonFormulaNamed: formula before export", wFormulaStr == wFormula);

    tString wUri = wWorkBook->Uri();
    tString wJson = m_Api->WriteJson(wUri);
    //cout << endl << wJson << endl;
    CPPUNIT_ASSERT_MESSAGE("JsonFormulaNamed: WriteJson non-empty", !wJson.empty());
    CPPUNIT_ASSERT_MESSAGE("JsonFormulaNamed: JSON contains name", wJson.find(wName) != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("JsonFormulaNamed: JSON contains formula", wJson.find(wFormula) != tString::npos);
    
    // New
    tearDown();
    setUp();

    wResult = m_Api->ReadJson(wJson);
    CPPUNIT_ASSERT_MESSAGE("JsonFormulaNamed: ReadJson", wResult);

    wWorkBook = tSpreadSheetContainer::Instance()->ActiveWorkBook();
    CPPUNIT_ASSERT_MESSAGE("JsonFormulaNamed: ActiveWorkBook after ReadJson", wWorkBook != nullptr);
    wFormulaStr = wWorkBook->RangeNamedContainer()->FormulaNamedStr(wName);
    CPPUNIT_ASSERT_MESSAGE("JsonFormulaNamed: formula after round-trip", wFormulaStr == wFormula);

    tFormulaNamed* wFormulaNamed = wWorkBook->RangeNamedContainer()->FormulaNamed(wName);
    CPPUNIT_ASSERT_MESSAGE("JsonFormulaNamed: FormulaNamed non-null after round-trip", wFormulaNamed != nullptr);
    if (wFormulaNamed != nullptr && wFormulaNamed->Cell() != nullptr) {
        CPPUNIT_ASSERT_MESSAGE("JsonFormulaNamed: value after round-trip", wFormulaNamed->Cell()->Value() == 12);
    }
}

void TestSkFormulaNamed::CallerRelativeRowNamedFormula() {
    // Excel loan templates: NuméroPaiement = ROW()-LigneEnTête must use the calling row.
    tBool wResult = m_Api->UndoInsertFormulaNamed("HDRROW", "ROW($A$16)");
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: HDRROW insert", wResult);
    wResult = m_Api->UndoInsertFormulaNamed("PAYNO", "ROW()-HDRROW");
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: PAYNO insert", wResult);
    wResult = m_Api->UndoInsertFormulaNamed("PAYNO2", "PAYNO");
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: PAYNO2 insert", wResult);
    wResult = m_Api->UndoInsertFormulaNamed("COLNO", "COLUMN()");
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: COLNO insert", wResult);

    wResult = m_Api->UndoCellValue("B17", "=PAYNO");
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: B17 formula", wResult);
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: B17 PAYNO is 1", m_Api->Cell("B17")->Value() == 1);

    wResult = m_Api->UndoCellValue("B18", "=PAYNO");
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: B18 formula", wResult);
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: B18 PAYNO is 2", m_Api->Cell("B18")->Value() == 2);

    wResult = m_Api->UndoCellValue("C17", "=PAYNO2");
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: C17 formula", wResult);
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: nested PAYNO2 on C17 is 1", m_Api->Cell("C17")->Value() == 1);

    wResult = m_Api->UndoCellValue("C20", "=PAYNO2");
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: C20 formula", wResult);
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: nested PAYNO2 on C20 is 4", m_Api->Cell("C20")->Value() == 4);

    wResult = m_Api->UndoCellValue("D5", "=HDRROW");
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: D5 formula", wResult);
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: ROW($A$16) is 16 on every row", m_Api->Cell("D5")->Value() == 16);

    wResult = m_Api->UndoCellValue("A1", "=COLNO");
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: A1 formula", wResult);
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: COLNO on A1 is 1", m_Api->Cell("A1")->Value() == 1);

    wResult = m_Api->UndoCellValue("C1", "=COLNO");
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: C1 formula", wResult);
    CPPUNIT_ASSERT_MESSAGE("CallerRelative: COLNO on C1 is 3", m_Api->Cell("C1")->Value() == 3);
}

void TestSkFormulaNamed::setUp() {
    std::filesystem::remove_all("./Spreadsheet");
    m_Application = tApplication::Instance();
    m_Application->Locale("fr");
    m_Api = new tApi;
    m_Api->IsUndoActif(true);
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
}

void TestSkFormulaNamed::tearDown() {
    delete m_Api;
}

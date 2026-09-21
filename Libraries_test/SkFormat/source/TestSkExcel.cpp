//==============================================================================
// TestSkExcel
//==============================================================================
#include "../include/TestSkExcel.hpp"
#include "../include/TestSkFormatTeardownHelpers.hpp"

// We can send it to the API of a feature
TestSkExcel::TestSkExcel() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
    m_Application = nullptr;
    m_Api = nullptr;
}

void TestSkExcel::TestSkFillFormat() {
    m_Application->Locale("fr");
    tString wJsonFormat=m_Api->JsonFormatString();
    
    tBool wOk=m_Api->UndoCellFormat("A1", "format-string:\"#,##0.00  €\";");
    if (!wOk) {
        tFormatCssApi* wFormatApiCss=dynamic_cast<SkFormat::tFormatCssApi*>(m_FormatApi);
        cout << wFormatApiCss->Error() << endl;
    }
    m_Api->UndoCellValue("A1", 12345.67);
    //cout << m_Api->CellFormat("A1") << endl;
    
    tString wResult=m_Api->CellFormatString("A1");
    // fr locale: space thousands sep + comma decimal (not '.' like SP/IT).
    CPPUNIT_ASSERT_MESSAGE("TestSkFillFormat Accounting ", "12 345,67 €" == wResult);
    
    
    wOk=m_Api->UndoCellFormat("A2","format-string:\"dd/mm/yyyy hh:mm:ss\";");
    tVariant wVariant;
    wVariant.Parse("01/12/2025 11:30");
    m_Api->UndoCellValue("A2", wVariant);
    wResult=m_Api->CellFormatString("A2");
    CPPUNIT_ASSERT_MESSAGE("TestSkFillFormat Date ", "01/12/2025 11:30:00" == wResult);
}

tString TestSkExcel::DebugCell(tString sRef) {
    tCell* wCell=m_Api->Cell(sRef);
    tStringStream wStream;
    wStream <<  wCell->StrRef(true) << "=" <<  wCell->FormulaStr()  << ":" << m_Api->CellFormat(sRef);
    return(wStream.str());
}

void TestSkExcel::TestSkInterfaceBudget() {
#ifdef SKER_FILE_DIR
    tString wFileName = tString(SKER_FILE_DIR) + "/Budget-familial.sker";
#else
    tString wFileName;
#endif
    tFile wFile=tFile(wFileName);
    if (wFile.Exist()) {
        tString wJson = wFile.LoadString();
        
        m_Api->ReadJson(wJson);
        
        m_Api->ActiveSheet("Budget mensuel");
     
        m_Api->UndoCellValue("B4",1);
        m_Api->UndoCellValue("B5","=B4+1");
        m_Api->Copy("B5");

        const tString wCellStyle =
            R"(color:#28415F;background-color:#DCE6F2;font:"Trebuchet MS",serif 12pt;text-align:center;vertical-align:middle;)";
        const tString wCellFormat =
            R"(format-string:"#,##0  €" 0;border-top:solid 1px #95B3D7;)";
        
        tString wTest1 = R"(Budget mensuel!B6=:)" + wCellStyle + wCellFormat;
        CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceBudget Copy Paste before Copy", wTest1 == DebugCell("B6"));
        m_Api->UndoPaste("B6:B14");
        
        tString wTest2 = R"(Budget mensuel!B6=B5+1:)" + wCellStyle + wCellFormat;
        CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceBudget Copy Paste after Copy", wTest2 == DebugCell("B6"));

        
        m_Api->Undo();
      
        CPPUNIT_ASSERT_MESSAGE("TestSkInterfaceBudget Copy Paste Undo", wTest1 == DebugCell("B6"));
        
    }
}

namespace {

void RegisterBudgetWorkBookCellClasses(tApi* sApi) {
    if (tClassFactory::Instance()->Get("SkCellClassSparkline") == nullptr) {
        sApi->RegisterClassAttribute("SkCellClassSparkline", "Sparkline", "Javascript");
    }
    if (tClassFactory::Instance()->Get("SkCellClassImage") == nullptr) {
        sApi->RegisterClassAttribute("SkCellClassImage", "Image", "Javascript");
    }
    if (tClassFactory::Instance()->Get("SkCellClassTextBox") == nullptr) {
        sApi->RegisterClassAttribute("SkCellClassTextBox", "TextBox", "Javascript");
    }
    if (tClassFactory::Instance()->Get("SkCellClassLineChart") == nullptr) {
        sApi->RegisterClassAttribute("SkCellClassLineChart", "LineChart", "Javascript");
    }
    if (tClassFactory::Instance()->Get("SkCellClassPieChart") == nullptr) {
        sApi->RegisterClassAttribute("SkCellClassPieChart", "PieChart", "Javascript");
    }
}

} // namespace

void TestSkExcel::setUp() {
    std::filesystem::remove_all("./Spreadsheet");
    
    m_Application = tApplication::Instance();
    m_FormatRoot = tFormatRoot::Instance();
    m_Api = new SkSpreadSheet::tApi;
    m_Api->IsUndoActif(true);
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
    RegisterBudgetWorkBookCellClasses(m_Api);
    m_FormatApi=new SkFormat::tFormatCssApi();
    m_Api->FormatApi((tFormatApi*)m_FormatApi);
}

void TestSkExcel::tearDown() {
    delete(m_Api);
    m_Api = nullptr;
    TestSkFormatTeardown::AssertFormatPoolEmptyAfterApiDelete(m_FormatApi, "TestSkExcel");
    delete(m_FormatApi);
    m_FormatApi = nullptr;

    DoneFormatRoot();
}

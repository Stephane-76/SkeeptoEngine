//==============================================================================
// TestSkCopySpreadSheet
//==============================================================================
#include "../include/TestSkCopySpreadSheet.hpp"
#include "../include/TestSkFormatTeardownHelpers.hpp"

#include <SkFormatCssApi.hpp>

#include <filesystem>

using namespace SkRoot;
using namespace SkSpreadSheet;
using namespace SkFormat;

TestSkCopySpreadSheet::TestSkCopySpreadSheet()
    : CPPUNIT_NS::TestFixture(), m_Application(nullptr), m_FormatRoot(nullptr), m_Api(nullptr), m_FormatApi(nullptr) {
}

void TestSkCopySpreadSheet::setUp() {
    std::filesystem::remove_all("./Spreadsheet");

    m_Application = tApplication::Instance();
    m_FormatRoot = tFormatRoot::Instance();
    m_Api = new SkSpreadSheet::tApi;
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
    m_FormatApi = new SkFormat::tFormatCssApi();
    m_Api->FormatApi((tFormatApi*)m_FormatApi);
}

void TestSkCopySpreadSheet::tearDown() {
    delete(m_Api);
    m_Api = nullptr;
    TestSkFormatTeardown::AssertFormatPoolEmptyAfterApiDelete(m_FormatApi, "TestSkCopySpreadSheet");
    delete(m_FormatApi);
    m_FormatApi = nullptr;

    DoneFormatRoot();
}

void TestSkCopySpreadSheet::TestCopy() {
#ifdef checkfo
    SkFormat::tFormatCssApi* wFormatApi=dynamic_cast<SkFormat::tFormatCssApi*>(m_FormatApi);
#endif

    m_Api->UndoCellValue("A1", 123);
    m_Api->UndoCellValue("B2", 456);

    tString wFormatStr = "color:aliceblue;";
    m_Api->UndoCellFormat("A1", wFormatStr);
    //cout << endl;
    tString wFormat=m_Api->CellFormat("A1");
    //cout << "A1=" << wFormat << endl;
   
    m_Api->Copy("A1");
    
    m_Api->UndoPaste("B1");
#ifdef checkfo
    wFormatApi->Check();
#endif
 
    wFormat=m_Api->CellFormat("B1");
    //cout << "B1=" << wFormat << endl;
    CPPUNIT_ASSERT(wFormat=="color:aliceblue;");
    m_Api->Undo();
#ifdef checkfo
    wFormatApi->Check();
#endif
    
    wFormat=m_Api->CellFormat("B1");
    CPPUNIT_ASSERT(wFormat=="");
    //cout << "Undo B1=" << wFormat << endl;

    tString wFormatStrC1 = "color:red;";
    m_Api->UndoCellFormat("C1", wFormatStrC1);
    wFormat=m_Api->CellFormat("C1");
    CPPUNIT_ASSERT(wFormat=="color:red;");
    
     m_Api->UndoPaste("C1");
    wFormat=m_Api->CellFormat("C1");
    CPPUNIT_ASSERT(wFormat=="color:aliceblue;");
    
    m_Api->Undo();
    wFormat=m_Api->CellFormat("C1");
    CPPUNIT_ASSERT(wFormat=="color:red;");
    
    m_Api->Undo();
    wFormat=m_Api->CellFormat("C1");
    CPPUNIT_ASSERT(wFormat=="");
}

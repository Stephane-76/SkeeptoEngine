//==============================================================================
// TestSkSpreadSheetFormat Test For SpreadSheet
// le 17/11/2023
//==============================================================================
#include "../include/TestSkFormatSpreadSheetClass.hpp"
#include "../include/TestSkFormatTeardownHelpers.hpp"

#include <SkFormatCssApi.hpp>

#define _drawdebug

// We can send it to the API of a feature
TestSkFormatSpreadSheetClass::TestSkFormatSpreadSheetClass()  : CPPUNIT_NS::TestFixture(), m_Application(nullptr),m_FormatRoot(nullptr){
	m_Application = nullptr;
	m_Api = nullptr;
}


void TestSkFormatSpreadSheetClass::SetValueMoney(tString sRef,t_UnitMoney sUnitMoney) {
    tClassUnit wClassUnit(sUnitMoney);
    tVariant wValue; // IsEmpty
    tCellClass wCellClass(wValue);
    tCellClassUnit wCellClassUnit(wValue,wClassUnit);
    tVariant wUnitValue(&wCellClassUnit);
    
    m_Api->UndoCellClass(sRef, &wUnitValue);
}

void TestSkFormatSpreadSheetClass::SetValueMoney(tString sRef,t_UnitMoney sUnitMoney,tDouble sValue) {
    tClassUnit wClassUnit(sUnitMoney);
    tVariant wValue=sValue;
    tCellClass wCellClass(wValue);
    tCellClassUnit wCellClassUnit(wValue,wClassUnit);
    tVariant wUnitValue(&wCellClassUnit);
    
    m_Api->UndoCellClass(sRef, &wUnitValue);
}
void TestSkFormatSpreadSheetClass::SetValueLength(tString sRef,t_UnitLength sUnitLength,tDouble sValue) {
    tClassUnit wClassUnit(sUnitLength);
    tVariant wValue=sValue;
    tCellClass wCellClass(wValue);
    tCellClassUnit wCellClassUnit(wValue,wClassUnit);
    tVariant wUnitValue(&wCellClassUnit);
    
    m_Api->UndoCellClass(sRef, &wUnitValue);
}

void  TestSkFormatSpreadSheetClass::DebugUnit(tString sRef) {
    return; //Drop
    tCell* wCell=m_Api->Cell(sRef);
    cout <<  sRef;
    if (wCell!=nullptr) {
        if (wCell->FormulaStr()!="") cout << ":(" << wCell->FormulaStr() << ")";
#ifdef _DEBUG
        if (wCell->Value().IsClass()) {
            cout << "=" << wCell->Value().Class()->Debug();
        } else {
            cout << "=" << wCell->Value();
        }
#else
        cout << "=" << wCell->Value();
#endif
    }
    cout << endl;
}


void TestSkFormatSpreadSheetClass::TestSkClass() {
    m_Api->UndoCellFormat("A1", "color:red;");
    
    SetValueMoney("A1", t_UnitMoney::eur, 12);
    DebugUnit("A1");
    CPPUNIT_ASSERT_MESSAGE("TestSkClass ", m_Api->CellFormat("A1")=="color:red;");
    
    m_Api->Undo();
    m_Api->Redo();
    
    SetValueMoney("B1", t_UnitMoney::eur, 12);
    DebugUnit("B1");
    
    
    m_Api->UndoCellValue("A2", "=A1/B1");
    DebugUnit("A2");
    // Test
    m_Api->UndoCellValue("B1", 12);
    DebugUnit("B1");
    
    m_Api->UndoCellValue("B2", -12356.79);
    m_Api->UndoCellFormat("B2", "format-string:\"#,##0.00 $;(#,##0.00) $\" \"eur\" 3;" );
    
    tString wFormat=m_Api->CellFormat("B2");
    //cout << endl << wFormat << endl;
    
    tString wB2Str=m_Api->ActiveWorkBook()->CellFormatString(m_Api->Cell("B2"));
    //cout << endl << wB2Str << endl;
    tString wJsonView=m_Api->JsonView(1, 1, tUnitMetrics::pixels , 100, 200,0,0, true);
    //cout << wJsonView << endl;
}

void TestSkFormatSpreadSheetClass::setUp() {
    std::filesystem::remove_all("./Spreadsheet");
    
    m_Application = tApplication::Instance();
    m_FormatRoot = tFormatRoot::Instance();
    m_Api = new SkSpreadSheet::tApi;
    m_Api->IsUndoActif(true);
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
    m_FormatApi=new SkFormat::tFormatCssApi();
    m_Api->FormatApi((tFormatApi*)m_FormatApi);
};

void TestSkFormatSpreadSheetClass::tearDown() {
    delete(m_Api);
    m_Api = nullptr;
    TestSkFormatTeardown::AssertFormatPoolEmptyAfterApiDelete(m_FormatApi, "TestSkFormatSpreadSheetClass");
    delete(m_FormatApi);
    m_FormatApi = nullptr;

	DoneFormatRoot();
}

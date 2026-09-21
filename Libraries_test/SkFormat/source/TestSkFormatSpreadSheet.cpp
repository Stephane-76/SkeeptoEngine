//==============================================================================
// TestSkSpreadSheetFormat Test For SpreadSheet
// le 17/11/2023
//==============================================================================
#include "../include/TestSkFormatSpreadSheet.hpp"
#include "../include/TestSkJsonViewHelpers.hpp"
#include "../include/TestSkFormatTeardownHelpers.hpp"

#include <SkFormatCssApi.hpp>

#define _drawdebug

using namespace TestSkJsonView;

// We can send it to the API of a feature
TestSkFormatSpreadSheet::TestSkFormatSpreadSheet()  : CPPUNIT_NS::TestFixture(), m_Application(nullptr),m_FormatRoot(nullptr){
	m_Application = nullptr;
	m_Api = nullptr;
}

void TestSkFormatSpreadSheet::UndoOperation() {
	return; // Drop
	tUndo* wUndo = m_Api->LastUndo();
	if (wUndo != nullptr) {
		cout << "Undo ->" << wUndo->OperationName() << endl;
	}
}

void TestSkFormatSpreadSheet::RedoOperation() {
    return; // Drop
    tUndo* wUndo = m_Api->LastRedo();
    if (wUndo != nullptr) {
        cout << "Redo ->" << wUndo->OperationName() << endl;
    }
}
void TestSkFormatSpreadSheet::Debug() {
}


void  TestSkFormatSpreadSheet::Fill() {
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


void TestSkFormatSpreadSheet::DrawCell(tString sOperation, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
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

void TestSkFormatSpreadSheet::ApplyWidth(tFormatCss* sFormatCssPool) {
	sFormatCssPool->Width(tUnitCss(10, tUnitMetrics::pixels));
}
void TestSkFormatSpreadSheet::ApplyColor(tFormatCss* sFormatCssPool) {
	sFormatCssPool->Color().ColorHex("0xFFFEFFFE");
}
void TestSkFormatSpreadSheet::ApplyBackgroundColor(tFormatCss* sFormatCssPool) {
	sFormatCssPool->BackgroundColor().Color(tColor(t_Color::AliceBlue));
}

void TestSkFormatSpreadSheet::ApplyBorder(tFormatCss* sFormatCssPool) {
	tBorderRectCss wBorderRect;
	wBorderRect.Left().BorderStyle(tBorderStyle::solid);
	wBorderRect.Left().Width(tUnitCss(1, tUnitMetrics::pixels));
	wBorderRect.Left().Color().ColorHex("0xFFFFFFFE");

	wBorderRect.TopLeftRadius().Unit(tUnitCss(50, tUnitMetrics::percent));

	m_FormatRoot->BorderRect(wBorderRect);
}

void TestSkFormatSpreadSheet::ApplyMargin(tFormatCss* sFormatCssPool) {
	tUnitRectCss wMargin;
	wMargin.All(tUnitCss(1, tUnitMetrics::pixels));
	m_FormatRoot->Margin(wMargin);
}
void TestSkFormatSpreadSheet::ApplyPadding(tFormatCss* sFormatCssPool) {
	tUnitRectCss wPadding;
	wPadding.Top(tUnitCss(2, tUnitMetrics::pixels));
	wPadding.Bottom(tUnitCss(2, tUnitMetrics::pixels));
	m_FormatRoot->Padding(wPadding);
}

void TestSkFormatSpreadSheet::ApplyFont(tFormatCss* sFormatCssPool) {
	tFontCss wFont;
	wFont.Name("Arial");
	wFont.Family(tFontFamily::serif);
	wFont.Size(tUnitCss(8, tUnitMetrics::points));
	wFont.ObliqueDegrees(90);
	wFont.StretchPercent(50);
	wFont.LineHeight(tUnitCss(3, tUnitMetrics::number));
	//wFont.Style(tFontStyle::italic);
	m_FormatRoot->Font(wFont);
}
void TestSkFormatSpreadSheet::ApplyText(tFormatCss* sFormatCssPool) {
	tTextCss wText;
	wText.TextAlign(tTextAlign::center);
	wText.VerticalTextAlign(tVerticalTextAlign::middle);
	wText.ExcelFormatString("###.###,00");
	m_FormatRoot->Text(wText);
}

tFormatRef TestSkFormatSpreadSheet::ApplyCellFormat(tString sValue) {
    SkFormat::tFormatApi* wFormatApi=static_cast<SkFormat::tFormatApi*>(m_FormatApi);
    tFormatRef wRef=wFormatApi->ApplyCellFormat(sValue);
    return(wRef);
}

tBool CompareStr(tString sStr1,tString sStr2) {
    tBool wOk=true;
    cout << ">" << sStr1 <<  "<" << endl;
    cout << ">" << sStr2 << "<" << endl;
    cout << endl;
    for(tInt wInd=0; wInd < sStr1.length(); wInd++) {
        cout << sStr1[wInd] << ":";
        if (wInd < sStr2.length()) {
            cout << sStr2[wInd];
            if (sStr2[wInd] != sStr2[wInd]) {
                cout << "....";
                wOk=false;
            }
        } else {
            wOk=false;
        }
        cout << endl;
    }
    return(wOk);
}

void TestSkFormatSpreadSheet::TestSkFormatCellAlloc() {
    
    //tColorCss wColor;
    //wColor.ColorHex("000000");
    //cout << wColor.Str() << endl;
    tFormatCssApi* wCssFormatApi=dynamic_cast<tFormatCssApi*>(m_FormatApi);
    
    tFormatRef wCss=wCssFormatApi->ApplyCellFormat("font:\"Avenir\",serif 12pt;");
    
    tCell* wCellA1 = m_Api->EnsureCell("A1");
    tCell* wCellA2 = m_Api->EnsureCell("A2");
    tCell* wCellA3 = m_Api->EnsureCell("A3");
    wCellA3->Css(wCss);
    tFormatCss* wFormatCssPool = m_FormatRoot->AllocFormatRef();
    ApplyBackgroundColor(wFormatCssPool);
    ApplyColor(wFormatCssPool);
    ApplyMargin(wFormatCssPool);
    ApplyPadding(wFormatCssPool);
    ApplyFont(wFormatCssPool);
    ApplyText(wFormatCssPool);
    ApplyBorder(wFormatCssPool);
    tFormatRef wFormatRefA1 = m_FormatRoot->ApplyCell();
    
    wCellA1->Css(wFormatRefA1);
    tFormatCss* wFormatA1=m_FormatRoot->FormatRef(wCellA1->Css());
#ifdef checkfo
    m_FormatRoot->Check();
#endif
    
#ifdef checksp
    m_Api->Check();
#endif
    
    wFormatCssPool = m_FormatRoot->AllocFormatRef();
    ApplyBackgroundColor(wFormatCssPool);
    ApplyColor(wFormatCssPool);
    ApplyMargin(wFormatCssPool);
    ApplyPadding(wFormatCssPool);
    ApplyFont(wFormatCssPool);
    ApplyText(wFormatCssPool);
    ApplyBorder(wFormatCssPool);
    tFormatRef wFormatRefA2 = m_FormatRoot->ApplyCell();
    m_FormatRoot->IncCell(wFormatRefA2);
    
    wCellA2->Css(wFormatRefA2);
    tFormatCss* wFormatA2 = m_FormatRoot->FormatRef(wCellA2->Css());
    
    //cout << wFormatA1->CssStr(m_FormatRoot) << endl;
        
    tString wFormatStr = "color:#FEFFFE;background-color:aliceblue;margin:1px;padding-top:2px;padding-bottom:2px;font:\"Arial\",serif 8pt;font-style:oblique 90deg;font-stretch: 50%;line-height:3;text-align:center;vertical-align:middle;format-string:\"###,###.00\" 0;border-left:solid 1px #FFFFFE;border-top-left-radius:50%;";
    

    // If not Ok Show Difference
    if (wFormatA1->Str(m_FormatRoot) != wFormatStr) {
        CompareStr(wFormatA1->Str(m_FormatRoot), wFormatStr);
    }

#ifdef _DEBUG
    tFormatCssApi* wFormatApi=dynamic_cast<tFormatCssApi*>(m_Api->FormatApi());
#ifdef drawdebug
    cout << wFormatApi->Debug() << endl;
#endif
#endif
    CPPUNIT_ASSERT_MESSAGE("TestSetFormat A2 =", wFormatA2->Str(m_FormatRoot) == wFormatStr);
    
    CPPUNIT_ASSERT_MESSAGE("TestSetFormat A1,A2 same instance", wCellA1->Css() == wCellA2->Css());
    
    m_FormatRoot->DeleteCell(wFormatRefA2);
    
    CPPUNIT_ASSERT_MESSAGE("TestSetFormat A1 =", wFormatA1->Str(m_FormatRoot) == wFormatStr);
    
#ifdef drawdebug
    cout << endl;
    cout << "A1 Format Alloc=" << wCellA1->Css() << ":" << wFormatA1->CssStr(m_FormatRoot) << endl;
    cout << "A2 Format Alloc=" << wCellA2->Css() << ":" << wFormatA2->CssStr(m_FormatRoot) << endl;
#endif
     
#ifdef checkfo
    m_FormatRoot->Check();
#endif
#ifdef checksp
    m_Api->Check();
#endif
    // Save SpreadSheet width Format =======================================
    tString wSaveJson=m_Api->WriteJson("wwww.skeema.fr/w1");
    tFile wFile("SpreadSheet.json");
    wFile.SaveString(wSaveJson);
    
#ifdef _DEBUG
    m_FormatRoot->DebugFormat();
#endif
    m_FormatRoot->DeleteCell(wCellA1->Css());
    m_FormatRoot->DeleteCell(wCellA2->Css());
    m_FormatRoot->DeleteCell(wCellA3->Css());
    
    wCellA1->Css(0);
    wCellA2->Css(0);
    wCellA3->Css(0);
    
#ifdef drawdebug
    cout << wFormatApi->Debug() << endl;
#endif
    m_FormatRoot->Clear();
}

void TestSkFormatSpreadSheet::TestSkFormatApi() {
#ifdef drawdebug
    cout << endl;
#endif
    tFormatRef wRef=ApplyCellFormat("color:red;");
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApi 1", wRef == 1);
    wRef=ApplyCellFormat("color:black;");
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApi 2 ", wRef == 2);
    tFormatRef wRef1=ApplyCellFormat("color:white; background-color:black;");
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApi 3", wRef1 == 3);
    wRef=ApplyCellFormat("color:red;");
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApi 4", wRef == 1);
    
    wRef=ApplyCellFormat("color:blue;");
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApi 5", wRef == 4);
    m_FormatRoot->DeleteFormatRef(1);
    wRef=ApplyCellFormat("color:blue;");
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApi 6", wRef == 4);
    // Recup First
    wRef=ApplyCellFormat("color:lightblue;");
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApi 7", wRef == 1);
    
    tFormatRef wRef2=ApplyCellFormat("border : 1px black solid;font: \"Arial\", serif italic 12pt bold ultra-condensed small-caps;border:solid 1px black;");
    m_FormatRoot->BeginMerge();
    m_FormatRoot->Merge(wRef1);
    m_FormatRoot->Merge(wRef2);
    tFormatRef wMerge=m_FormatRoot->ApplyMerge();
#ifdef drawdebug
    cout << "------------------------------------------" << endl;
#endif
    tFormatCss* wFormatCss=m_FormatRoot->Format("5");
    
    tString wFormatStr="font:\"Arial\",serif 12pt small_caps;font-weight:bold;font-style:italic;font-stretch:ultra-condensed;border:solid 1px black;";
    tString wResult=wFormatCss->Str(m_FormatRoot,false);
    
    if (wFormatStr!=wResult) {
        cout << endl << wResult << endl;
        cout << wFormatStr << endl;
        //CompareStr(wFormatStr, wResult);
    }
    
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApi 8", wFormatStr == wResult);

#ifdef drawdebug
    cout << wFormatCss->CssStr(m_FormatRoot,false) << endl;
#endif
    
    tStringStream wStream;
    wStream << wMerge;
    tFormatCss* wFormatMergeCss=m_FormatRoot->Format(wStream.str());
#ifdef drawdebug
    cout << "------------------------------------------" << endl;
    cout << wFormatMergeCss->CssStr(m_FormatRoot,false) << endl;
#endif
    
    wFormatStr="color:white;background-color:black;font:\"Arial\",serif 12pt small_caps;font-weight:bold;font-style:italic;font-stretch:ultra-condensed;border:solid 1px black;";
    wResult=wFormatMergeCss->Str(m_FormatRoot,false);
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApi 9", wFormatStr == wResult);
    

    m_FormatRoot->Clear();
}


void TestSkFormatSpreadSheet::TestSkFormatApiText() {
    m_Api->UndoCellFormat("A1","color:#000000; background-color:#000000;text-rotate:90;");
    tString wFormatA1=m_Api->CellFormat("A1");
    //cout << wFormatA1 << endl;
    
    m_Api->UndoCellFormat("A1","border:1px solid #000000;");
    
    wFormatA1=m_Api->CellFormat("A1");
    tString wResult="color:black;background-color:black;text-rotate:90;border:solid 1px black;";
    //cout << endl << wFormatA1 << endl;
    CPPUNIT_ASSERT_MESSAGE("Test #000000 -> Black", wFormatA1==wResult);
    
    // Tect Decoration
    if (!m_Api->UndoCellFormat("A1:A3","font:\"Courier New\",serif 33pt;text-wrap:wrap;")) {
        cout << "Error " << endl;
    };
#ifdef checkfo
    m_FormatRoot->Check();
#endif
    if (!m_Api->UndoCellFormat("A1:A2","font:\"Courier New\",serif 31pt;")) {
        cout << "Error " << endl;
    };
#ifdef checkfo
    m_FormatRoot->Check();
#endif
    
    //cout << m_Api->CellFormat("A2") << endl;
    // test
    wResult="font:\"Courier New\",serif 31pt;text-wrap:wrap;";
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApiText ", m_Api->CellFormat("A2")==wResult);
}


void TestSkFormatSpreadSheet::TestSkFormatRazCell() {
#ifdef drawdebug
    cout << endl;
#endif

#ifdef _DEBUG
    tFormatCssApi* wFormatApi=dynamic_cast<tFormatCssApi*>(m_Api->FormatApi());
#endif
    
    if (!m_Api->UndoCellFormat("A1:A5","color:red;background-color:black;")) {
        cout << "Error " << endl;
    };
    
    
    m_Api->UndoCellValue("A1", 12);
    //tCell* wCell=m_Api->Cell("A1");
    //tString wFormat=m_Api->CellFormat("A1");
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatRazCell ", m_Api->CellFormat("A1")=="color:red;background-color:black;");
    //cout << wFormat << endl;
    m_Api->Undo();

    //m_Api->Redo();
    
    
#ifdef checkfo
    m_Api->CheckFormat();
#endif

#ifdef drawdebug
     cout << wFormatApi->Debug() << endl;
#endif
    m_Api->Undo();

#ifdef checkfo
    m_Api->CheckFormat();
#endif

#ifdef drawdebug
     cout << wFormatApi->Debug() << endl;
#endif
    m_Api->Redo();
    
#ifdef checkfo
    m_Api->CheckFormat();
#endif

#ifdef drawdebug
     cout << wFormatApi->Debug() << endl;
#endif
    
    m_Api->UndoCellFormat("A2:B6", "color:black;");
#ifdef drawdebug
    cout << "A2 Format ->" << m_Api->CellFormat("A2") << endl;
#endif
#ifdef drawdebug
     cout << wFormatApi->Debug() << endl;
#endif
    
#ifdef checkfo
    m_Api->CheckFormat();
#endif

    m_Api->UndoCellFormat("A2:B2", "border : 1px solid black;");
    
#ifdef drawdebug
     cout << "A2 Format ->" << m_Api->CellFormat("A2") << endl;
     cout << wFormatApi->Debug() << endl;
#endif
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoRaz("A1:B10");
#ifdef drawdebug
    cout << "A2 Format ->" << m_Api->CellFormat("A2") << endl;
#endif
#ifdef checkfo
    m_Api->CheckFormat();
#endif

    m_Api->Undo();
    
    return;
#ifdef drawdebug
    cout << "A2 Format ->" << m_Api->CellFormat("A2") << endl;
    cout << "A1 Format ->" << m_Api->CellFormat("A1") << endl;
#endif
#ifdef checkfo
    m_Api->CheckFormat();
#endif

#ifdef drawdebug
     cout << wFormatApi->Debug() << endl;
#endif
    m_Api->Undo();
#ifdef drawdebug
    cout << "A1 Format ->" << m_Api->CellFormat("A1") << endl;
    cout << "A2 Format ->" << m_Api->CellFormat("A2") << endl;
#endif
#ifdef checkfo
    m_Api->CheckFormat();
#endif

    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
#ifdef drawdebug
    cout << wFormatApi->Debug() << endl;
#endif
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellFormat("B25", "font:\"Arial\",serif 12pt;");
    tString wResult=m_Api->CellFormat("B25");
    //cout << endl << wResult << endl;
    
    m_Api->UndoCellFormat("B25", "font-style:italic;font-weight:bold;");
    //m_Api->UndoCellFormat("B25", "font-weight:bold;");
    wResult=m_Api->CellFormat("B25");
    //cout << endl << wResult << endl;
    tFormatMergeCss* wFormatMerge=m_FormatRoot->FormatMerge();
    StringBuffer wStringBuffer;
    Writer<StringBuffer> wWriter(wStringBuffer);
    tVariant wVariant(12);
    wWriter.StartObject();
    wFormatMerge->JsonJavaScript(&wWriter,&wVariant,true);
    wWriter.EndObject();
    wResult=wStringBuffer.GetString();
    //cout << wResult << endl;
    
#ifdef drawdebug
     cout << wFormatApi->Debug() << endl;
    cout << "A1 Format ->" << m_Api->CellFormat("A1") << endl;
    cout << "A2 Format ->" << m_Api->CellFormat("A2") << endl;
#endif
}

void TestSkFormatSpreadSheet::TestSkFormatDeleteColRow() {
#ifdef drawdebug
    cout << endl;
#endif
    m_Api->UndoCellFormat("A1:M10","color:red;");
    m_Api->UndoCellFormat("B2:J10","color:Red;");
    m_Api->UndoCellFormat("B5:B23","color:black;");
    m_Api->UndoCellBorder("A1:B2",5,"dashed black 3px;");
    
    m_Api->UndoDeleteCol(1,2);
    //cout << m_Api->CellFormat("C4") << endl;
    m_Api->Undo();
    //cout << m_Api->CellFormat("C4") << endl;
    
    
    m_Api->UndoDeleteCol(1,2);
    m_Api->UndoDeleteCol(1,2);
    m_Api->UndoDeleteCol(1,2);
    m_Api->UndoDeleteCol(1,2);
    m_Api->UndoDeleteCol(1,2);
    m_Api->UndoDeleteCol(1,2);
    m_Api->UndoDeleteCol(1,2); // BUG WASM ???????
    
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    
    //cout << m_Api->CellFormat("M10") << endl;
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatDeleteColRow 1", m_Api->CellFormat("M10")=="color:red;");
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatDeleteColRow 2", m_Api->CellFormat("J10")=="color:red;");
    
    m_Api->Redo();
    m_Api->Redo();
    m_Api->Redo();
    m_Api->Redo();
    m_Api->Redo();
    m_Api->Redo();
    m_Api->Redo();
    
   
#ifdef _DEBUG
    tFormatCssApi* wFormatApi=dynamic_cast<tFormatCssApi*>(m_Api->FormatApi());
#endif
    
    m_Api->UndoCellFormat("A1:G10","color:red;");
    
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellFormat("A2:B8", "border : 1px solid black;margin : 1px;");
    
    m_Api->UndoCellFormat("A2:F8", "padding-left : 12px;");
    m_Api->UndoCellFormat("A2:Z9", "font:\"Avenir\",serif 12pt;");
    
    m_Api->Undo();
    m_Api->Redo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    // Delete Row
    m_Api->UndoDeleteRow(1,4);
   
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
    
    // Delete Col
    m_Api->UndoDeleteCol(1,2);
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkFormatSpreadSheet::TestSkFormatInsertRowCol() {
    // Border seams: row insert strips top/bottom; when origin has border-bottom also strip left/right.
    // Column insert strips left/right; when origin has border-right also strip top/bottom.
    // No format duplication on first row or first column.
    // Run before other inserts here so row/column addresses match this sheet fixture.
    {
        const tString wRowBorderCss =
            "background-color:tan;border-top:solid 2px navy;border-bottom:solid 2px navy;border-left:solid 2px navy;border-right:solid 2px navy;";
        m_Api->UndoCellFormat("T31:V31", wRowBorderCss);
        // UndoInsertRow registers on undo stack; InsertRow() alone does not.
        CPPUNIT_ASSERT_MESSAGE("UndoInsertRow 32", m_Api->UndoInsertRow(32, 1));
        const tString wT32 = m_Api->CellFormat("T32");
        CPPUNIT_ASSERT_MESSAGE("InsertRow border dup keeps tan", wT32.find("tan") != tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertRow border dup strips border-top", wT32.find("border-top") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertRow border dup strips border-bottom", wT32.find("border-bottom") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertRow border dup strips border-left when origin has border-bottom", wT32.find("border-left") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertRow border dup strips border-right when origin has border-bottom", wT32.find("border-right") == tString::npos);
        m_Api->Undo();
        CPPUNIT_ASSERT_MESSAGE("Undo InsertRow border T32 cleared", m_Api->CellFormat("T32").find("tan") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("Undo InsertRow border template T31 kept", m_Api->CellFormat("T31").find("tan") != tString::npos);
        CPPUNIT_ASSERT_MESSAGE("Undo InsertRow border-top back on template", m_Api->CellFormat("T31").find("border-top") != tString::npos);
    }
    {
        const tString wColBorderCss =
            "background-color:wheat;border-top:solid 2px maroon;border-bottom:solid 2px maroon;border-left:solid 2px maroon;border-right:solid 2px maroon;";
        m_Api->UndoCellFormat("M35:M37", wColBorderCss);
        CPPUNIT_ASSERT_MESSAGE("UndoInsertCol 14", m_Api->UndoInsertCol(14, 1));
        const tString wN35 = m_Api->CellFormat("N35");
        CPPUNIT_ASSERT_MESSAGE("InsertCol border dup keeps wheat", wN35.find("wheat") != tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertCol border dup strips border-left", wN35.find("border-left") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertCol border dup strips border-right", wN35.find("border-right") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertCol border dup strips border-top when origin has border-right", wN35.find("border-top") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertCol border dup strips border-bottom when origin has border-right", wN35.find("border-bottom") == tString::npos);
        m_Api->Undo();
        CPPUNIT_ASSERT_MESSAGE("Undo InsertCol border N35 cleared", m_Api->CellFormat("N35").find("wheat") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("Undo InsertCol border template M35 kept", m_Api->CellFormat("M35").find("wheat") != tString::npos);
    }
    {
        const tString wCssRectRow =
            "background-color:khaki;border-top:solid 1px black;border-bottom:solid 1px black;border-left:solid 1px black;border-right:solid 1px black;";
        m_Api->UndoCellFormat("T44:V44", wCssRectRow);
        tRect wRowStrip(45, 20, 45, 22);
        m_Api->UndoInsertRowByRect(wRowStrip);
        const tString wT45 = m_Api->CellFormat("T45");
        CPPUNIT_ASSERT_MESSAGE("InsertRowByRect border dup strips border-top", wT45.find("border-top") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertRowByRect border dup strips border-bottom", wT45.find("border-bottom") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertRowByRect border dup strips border-left when origin has border-bottom", wT45.find("border-left") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertRowByRect border dup strips border-right when origin has border-bottom", wT45.find("border-right") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertRowByRect border dup keeps khaki", wT45.find("khaki") != tString::npos);
        m_Api->Undo();
        CPPUNIT_ASSERT_MESSAGE("Undo InsertRowByRect T45 khaki cleared", m_Api->CellFormat("T45").find("khaki") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("Undo InsertRowByRect template T44 khaki kept", m_Api->CellFormat("T44").find("khaki") != tString::npos);
    }
    {
        const tString wCssRectCol =
            "background-color:lavender;border-top:solid 1px teal;border-bottom:solid 1px teal;border-left:solid 1px teal;border-right:solid 1px teal;";
        m_Api->UndoCellFormat("M52:M54", wCssRectCol);
        tRect wColStrip(52, 14, 54, 14);
        m_Api->UndoInsertColByRect(wColStrip);
        const tString wN52 = m_Api->CellFormat("N52");
        CPPUNIT_ASSERT_MESSAGE("InsertColByRect border dup strips border-left", wN52.find("border-left") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertColByRect border dup strips border-right", wN52.find("border-right") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertColByRect border dup strips border-top when origin has border-right", wN52.find("border-top") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertColByRect border dup strips border-bottom when origin has border-right", wN52.find("border-bottom") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertColByRect border dup keeps lavender", wN52.find("lavender") != tString::npos);
        m_Api->Undo();
        CPPUNIT_ASSERT_MESSAGE("Undo InsertColByRect N52 lavender cleared", m_Api->CellFormat("N52").find("lavender") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("Undo InsertColByRect template M52 lavender kept", m_Api->CellFormat("M52").find("lavender") != tString::npos);
    }

    // Full-sheet paths (DoInsertRow / DoInsertCol): duplicate cell Css from row above / column left.
    m_Api->UndoCellFormat("B5:D5", "background-color:coral;");
    CPPUNIT_ASSERT_MESSAGE("UndoInsertRow 6", m_Api->UndoInsertRow(6, 1));
    CPPUNIT_ASSERT_MESSAGE("InsertRow B6 copies coral from row 5",
                           m_Api->CellFormat("B6").find("background-color:coral") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("InsertRow C6 copies coral",
                           m_Api->CellFormat("C6").find("background-color:coral") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("InsertRow D6 copies coral",
                           m_Api->CellFormat("D6").find("background-color:coral") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("InsertRow template B5 still coral",
                           m_Api->CellFormat("B5").find("background-color:coral") != tString::npos);
    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Undo InsertRow B6 coral cleared",
                           m_Api->CellFormat("B6").find("background-color:coral") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("Undo InsertRow B5 coral kept",
                           m_Api->CellFormat("B5").find("background-color:coral") != tString::npos);

    // Insert at row 1: no format duplication; formatted row shifts down (A2:C2 -> A3:C3).
    m_Api->UndoCellFormat("A2:C2", "background-color:peachpuff;");
    CPPUNIT_ASSERT_MESSAGE("UndoInsertRow 1", m_Api->UndoInsertRow(1, 1));
    CPPUNIT_ASSERT_MESSAGE("InsertRow 1 A1 no peachpuff copy",
                           m_Api->CellFormat("A1").find("background-color:peachpuff") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("InsertRow 1 B1 no peachpuff copy",
                           m_Api->CellFormat("B1").find("background-color:peachpuff") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("InsertRow 1 shifted template A3 still peachpuff",
                           m_Api->CellFormat("A3").find("background-color:peachpuff") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("InsertRow 1 shifted template C3 still peachpuff",
                           m_Api->CellFormat("C3").find("background-color:peachpuff") != tString::npos);
    m_Api->Undo();

    m_Api->UndoCellFormat("D20:D22", "background-color:skyblue;");
    CPPUNIT_ASSERT_MESSAGE("UndoInsertCol 5", m_Api->UndoInsertCol(5, 1));
    CPPUNIT_ASSERT_MESSAGE("InsertCol E20 copies skyblue from column D",
                           m_Api->CellFormat("E20").find("background-color:skyblue") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("InsertCol E22 copies skyblue",
                           m_Api->CellFormat("E22").find("background-color:skyblue") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("InsertCol template D20 still skyblue",
                           m_Api->CellFormat("D20").find("background-color:skyblue") != tString::npos);
    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Undo InsertCol E20 skyblue cleared",
                           m_Api->CellFormat("E20").find("background-color:skyblue") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("Undo InsertCol D20 skyblue kept",
                           m_Api->CellFormat("D20").find("background-color:skyblue") != tString::npos);

    // Insert at column 1: no format duplication; formatted column shifts right (B20:B22 -> C20:C22).
    m_Api->UndoCellFormat("B20:B22", "background-color:mistyrose;");
    CPPUNIT_ASSERT_MESSAGE("UndoInsertCol 1", m_Api->UndoInsertCol(1, 1));
    CPPUNIT_ASSERT_MESSAGE("InsertCol 1 A20 no mistyrose copy",
                           m_Api->CellFormat("A20").find("background-color:mistyrose") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("InsertCol 1 shifted template C20 still mistyrose",
                           m_Api->CellFormat("C20").find("background-color:mistyrose") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("InsertCol 1 shifted template C22 still mistyrose",
                           m_Api->CellFormat("C22").find("background-color:mistyrose") != tString::npos);
    m_Api->Undo();

    // Rect paths (DoInsertRowByRect / DoInsertColByRect): duplicate cells only inside the strip.
    m_Api->UndoCellFormat("B15:D15", "background-color:goldenrod;");
    {
        tRect wRowStrip(16, 2, 16, 4);
        m_Api->UndoInsertRowByRect(wRowStrip);
        CPPUNIT_ASSERT_MESSAGE("InsertRowByRect B16 goldenrod",
                               m_Api->CellFormat("B16").find("background-color:goldenrod") != tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertRowByRect D16 goldenrod",
                               m_Api->CellFormat("D16").find("background-color:goldenrod") != tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertRowByRect F16 outside strip — no goldenrod",
                               m_Api->CellFormat("F16").find("background-color:goldenrod") == tString::npos);
        m_Api->Undo();
        CPPUNIT_ASSERT_MESSAGE("Undo InsertRowByRect B16 goldenrod cleared",
                               m_Api->CellFormat("B16").find("background-color:goldenrod") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("Undo InsertRowByRect B15 goldenrod kept",
                               m_Api->CellFormat("B15").find("background-color:goldenrod") != tString::npos);
    }

    m_Api->UndoCellFormat("G25:G27", "background-color:plum;");
    {
        tRect wColStrip(25, 8, 27, 8);
        m_Api->UndoInsertColByRect(wColStrip);
        CPPUNIT_ASSERT_MESSAGE("InsertColByRect H25 plum",
                               m_Api->CellFormat("H25").find("background-color:plum") != tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertColByRect H27 plum",
                               m_Api->CellFormat("H27").find("background-color:plum") != tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertColByRect template G25 still plum",
                               m_Api->CellFormat("G25").find("background-color:plum") != tString::npos);
        CPPUNIT_ASSERT_MESSAGE("InsertColByRect H20 outside row band — no plum",
                               m_Api->CellFormat("H20").find("background-color:plum") == tString::npos);
        m_Api->Undo();
        CPPUNIT_ASSERT_MESSAGE("Undo InsertColByRect H25 plum cleared",
                               m_Api->CellFormat("H25").find("background-color:plum") == tString::npos);
        CPPUNIT_ASSERT_MESSAGE("Undo InsertColByRect G25 plum kept",
                               m_Api->CellFormat("G25").find("background-color:plum") != tString::npos);
    }

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkFormatSpreadSheet::TestSkFormatCopyPaste() {
    m_Api->UndoCellFormat("A1", "color:red;");
    m_Api->Copy("A1");
    m_Api->UndoPaste("B1");
    
    m_Api->Undo();
    
    m_Api->UndoCellFormat("A1", "color:red;");
    m_Api->UndoCellFormat("B1", "color:blue;");
    m_Api->Copy("A1:B1");
    
    m_Api->UndoPaste("A1:B1");
    m_Api->UndoPaste("A1:D4");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
    m_Api->UndoCellFormat("A1:G10","color:red;");

    
    m_Api->UndoCellFormat("A2:B2","color:red;");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellFormat("A2:B8", "border : 1px solid black;margin : 1px;");
    
    m_Api->UndoCellFormat("A2:F8", "padding-left : 12px; border : 3px solid black;margin : 2px;");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
    //cout << "Cell A2->" <<  m_Api->CellFormat("A2") << endl;
    
    // With Cell
    m_Api->Copy("A2");
    
    m_Api->UndoPaste("B2");
    //m_Api->UndoPaste("B3:H3");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
    // With Range
    m_Api->Copy("A2:G2");
    
    m_Api->UndoPaste("B2:H2");
    //m_Api->UndoPaste("B3:H3");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();

#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
    
    tApplication::Instance()->ClearUndoRedo();
    
#ifdef checkfo
    m_Api->CheckFormat();
#endif
#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkFormatSpreadSheet::TestSkFormatRecover() {
    m_Api->UndoCellFormat("H2:I5","background-color:yellow;");
    m_Api->UndoCellFormat("G4:L4","background-color:red;");
  
    m_Api->Undo();
    m_Api->Undo();
    
    m_Api->UndoCellFormat("A1;A1;A1;A1","background-color:yellow;");
    m_Api->Undo();
    m_Api->Redo();
    m_Api->Undo();
    
    m_Api->UndoCellFormat("A1:Z10","background-color:yellow;");
    
    m_Api->UndoCellFormat("B2:C7;B2","background-color:red;");
    
    
    tString wFormat=m_Api->CellFormat("B2");
    //cout << wFormat << endl;
    
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatRecover 1", m_Api->CellFormat("B2")=="background-color:red;");

    m_Api->Undo();
    
    wFormat=m_Api->CellFormat("B2");
    //cout << wFormat << endl;
    
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatRecover 2 ", m_Api->CellFormat("B2")=="background-color:yellow;");

    tApplication::Instance()->ClearUndoRedo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkFormatSpreadSheet::TestSkFormatDeleteSheet() {
    tSheet* wSheet1 = m_Api->ActiveSheet();

    tSheet* wSheet2 = m_Api->AddSheet("Sheet2");
    m_Api->ActiveSheet(wSheet2->Name());

    m_Api->UndoCellFormat("A1:G10", "color:red;");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellFormat("A2:B8", "border : 1px solid black;margin : 1px;");
    m_Api->UndoCellFormat("A2:F8", "padding-left : 12px;");
#ifdef checkfo
    m_Api->CheckFormat();
#endif

    const tString kFormatA2Merged = "color:red;margin:1px;padding-left:12px;border:solid 1px black;";

    CPPUNIT_ASSERT_MESSAGE("DeleteSheet format A1 before delete",
                           m_Api->CellFormat("A1") == "color:red;");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet format A2 before delete",
                           m_Api->CellFormat("A2") == kFormatA2Merged);

    m_Api->UndoCellValue("E1", 10);
    m_Api->UndoCellValue("E2", 20);
    m_Api->UndoCellValue("F1", 30);

    m_Api->ActiveSheet(wSheet1->Name());
    m_Api->UndoCellValue("A1", "=Sheet2!E1+Sheet2!E2");
    m_Api->UndoCellValue("A2", "=Sheet2!E1+Sheet2!E2+Sheet2!F1");
    m_Api->ActiveSheet()->Calculate(new tTempoRect(1, 1, 10, 10));

    CPPUNIT_ASSERT_MESSAGE("DeleteSheet A1 formula before delete",
                           m_Api->Formula(1, 1) == "Sheet2!E1+Sheet2!E2");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet A2 formula before delete",
                           m_Api->Formula(2, 1) == "Sheet2!E1+Sheet2!E2+Sheet2!F1");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet A1 value before delete",
                           m_Api->CellValue(1, 1).Str() == "30");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet A2 value before delete",
                           m_Api->CellValue(2, 1).Str() == "60");

    tWorkBook* wWorkBook = m_Api->ActiveWorkBook();
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet sheets before delete",
                           wWorkBook->JsonSheets() == "{\"list\":[\"Sheet1\",\"Sheet2\"]}");

    CPPUNIT_ASSERT_MESSAGE("UndoDeleteSheet Sheet2", m_Api->UndoDeleteSheet("Sheet2"));

    CPPUNIT_ASSERT_MESSAGE("DeleteSheet sheets after Do",
                           wWorkBook->JsonSheets() == "{\"list\":[\"Sheet1\"]}");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet A1 formula after Do",
                           m_Api->Formula(1, 1) == "#REF!+#REF!");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet A2 formula after Do",
                           m_Api->Formula(2, 1) == "#REF!+#REF!+#REF!");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet A1 value after Do",
                           m_Api->CellValue(1, 1).Str() == "#REF!");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet A2 value after Do",
                           m_Api->CellValue(2, 1).Str() == "#REF!");

#ifdef checkfo
    m_Api->CheckFormat();
#endif

    m_Api->Undo();

    CPPUNIT_ASSERT_MESSAGE("DeleteSheet sheets after Undo",
                           wWorkBook->JsonSheets() == "{\"list\":[\"Sheet1\",\"Sheet2\"]}");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet A1 formula after Undo",
                           m_Api->Formula(1, 1) == "Sheet2!E1+Sheet2!E2");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet A2 formula after Undo",
                           m_Api->Formula(2, 1) == "Sheet2!E1+Sheet2!E2+Sheet2!F1");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet A1 value after Undo",
                           m_Api->CellValue(1, 1).Str() == "30");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet A2 value after Undo",
                           m_Api->CellValue(2, 1).Str() == "60");

    m_Api->ActiveSheet(wSheet2->Name());
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet format A1 after Undo",
                           m_Api->CellFormat("A1") == "color:red;");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet format A2 after Undo",
                           m_Api->CellFormat("A2") == kFormatA2Merged);
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet E1 value after Undo",
                           m_Api->CellValue(1, 5).Str() == "10");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet E2 value after Undo",
                           m_Api->CellValue(2, 5).Str() == "20");
    CPPUNIT_ASSERT_MESSAGE("DeleteSheet F1 value after Undo",
                           m_Api->CellValue(1, 6).Str() == "30");

#ifdef checkfo
    m_Api->CheckFormat();
#endif

    // Direct delete without undo stack
    m_Application->ClearUndoRedo();
    wWorkBook->DeleteSheet("Sheet2", true);
#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkFormatSpreadSheet::TestSkFormatApplySheet() {
    tRect wRect=m_Api->GetSheetSelect();
    tString wSelect=wRect.StrRef();
    m_Api->UndoCellFormat(wSelect,"color:red;");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellBorder(wSelect,tBorderAll, "1px solid black;");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellBorder(wSelect,tBorderLeft, "");

    tString wFormat=m_Api->CellFormat("A2");
    //cout << endl << wFormat << endl;
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApplySheet m_Api->CellFormat('A2')", m_Api->CellFormat("A2") == wFormat);
    
    //"color:red;border-top:solid 1px black;border-right:solid 1px black;border-bottom:solid 1px black;"
    

#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkFormatSpreadSheet::TestSkFormatApplyCol() {
    tRect wRect=m_Api->GetColSelect(2,8);
    tString wSelect=wRect.StrRef();
    m_Api->UndoCellFormat(wSelect,"color:red;");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellFormat("A2", "border : 1px solid black;margin : 1px;");
    
    tCell* wCellA2=m_Api->Cell("A2");
    tFormatRef wCssA2=wCellA2->Css();
    
    
    tColRow* wColRowB=m_Api->ActiveSheet()->Col(2);
    tFormatRef wCssColB=wColRowB->Css();
    
    
    tFormatCssApi* wFormatApi=dynamic_cast<tFormatCssApi*>(m_FormatApi);
    
    
    //cout << endl << "Cell A2=" << m_Api->CellFormat("A2") << endl;
    //cout << "Col B=" << wFormatApi->CellFormat(wCssA2,false);
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApplyCol m_Api->CellFormat('A2')", m_Api->CellFormat("A2") == "margin:1px;border:solid 1px black;");
 
    tVectorFormatRef wVector;
    wVector.push_back(wCssColB);
    wVector.push_back(wCssA2);
    tString wFormatStr=wFormatApi->CellFormat(&wVector,false);
    //cout << endl << wFormatStr << endl;
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApplyCol A2 wFormatApi->CellFormat", wFormatStr == "color:red;margin:1px;border:solid 1px black;");


    tString wFormatApiStr=m_Api->CellFormat("B2");
    //cout << wFormatApiStr << endl;
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApplyCol B2 format", wFormatApiStr == "color:red;");
    
    
    m_Api->UndoCellFormat("B2", "border : 1px solid black;margin : 1px;");
    wFormatApiStr=m_Api->CellFormat("B2");
    //cout << endl << wFormatApiStr << endl;
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApplyCol B2 format with col", wFormatApiStr == "color:red;margin:1px;border:solid 1px black;");
    
    m_Api->UndoCellFormat("B2", "color:black;");
    wFormatApiStr=m_Api->CellFormat("B2");
    //cout << endl << wFormatApiStr << endl;
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApplyCol B2 format with col color black", wFormatApiStr == "color:black;margin:1px;border:solid 1px black;");
    
    
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellBorder(wSelect,tBorderAll, "1px solid black;");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellBorder(wSelect,tBorderLeft, "");

    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();

#ifdef checkfo
    m_Api->CheckFormat();
#endif

    m_Api->Redo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
    // Delete Col
    m_Api->UndoDeleteCol(1,4);
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
    m_Api->UndoCellBorder(wSelect,tBorderAll, "1px solid black;");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellBorder(wSelect,tBorderLeft, "");

    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();

#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
    m_Api->Undo();
    /*
    m_Api->Undo();
    */
#ifdef checkfo
    m_Api->CheckFormat();
#endif
     
}

void TestSkFormatSpreadSheet::TestSkFormatApplyRow() {
    tRect wRect=m_Api->GetRowSelect(2,4);
    tString wSelect=wRect.StrRef();
    m_Api->UndoCellFormat(wSelect,"color:red;");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
    m_Api->UndoCellBorder(wSelect,tBorderAll, "1px solid black;");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellBorder(wSelect,tBorderLeft, "");
    
    tString wFormat=m_Api->CellFormat("A2");
    //cout << endl << wFormat << endl;
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatApplyRow m_Api->CellFormat('A2')", m_Api->CellFormat("A2") == "color:red;border-top:solid 1px black;border-right:solid 1px black;border-bottom:solid 1px black;");
    
    // Delete Row
    m_Api->UndoDeleteRow(1,2);
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
    m_Api->Undo();

#ifdef checkfo
    m_Api->CheckFormat();
#endif

    //m_Api->Undo();
    
#ifdef checkfo
    //m_Api->CheckFormat();
#endif
}

void TestSkFormatSpreadSheet::TestSkFormatBorder() {
    //m_Api->UndoCellFormat("A1:G10","color:red;");
    //m_Api->UndoCellFormat("A1:G10","font:\"Avenir\",serif 12pt;");
    //m_Api->UndoCellBorder("A1:G10",tBorderAll, "solid 1px black;");
    //m_Api->UndoCellBorder("A1:G10",tBorderAll, "");
    
     m_Api->UndoCellBorder("A1:C2;B2:B3",tBorderAll, "solid 1px black;");
     
    m_Api->UndoCellFormat("H2", "font:\"Avenir\",serif 12pt;");
    m_Api->UndoCellBorder("H2",tBorderAll, "solid 1px black;");
    tString wFormatStr=m_Api->CellFormat("H2");
    // Cell-local storage: tBorderAll on a single cell stores all four sides on that cell.
    tString wResultFormat="font:\"Avenir\",serif 12pt;border-left:solid 1px black;border-top:solid 1px black;border-right:solid 1px black;border-bottom:solid 1px black;";
    
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatBorder  (1) H2 format", wFormatStr ==wResultFormat);
    
    //cout << endl << m_Api->CellFormat("H2") << endl;
    m_Api->UndoCellBorder("H2",1, "");
    
    wResultFormat="font:\"Avenir\",serif 12pt;";
    wFormatStr=m_Api->CellFormat("H2");
    
    //cout << endl << m_Api->CellFormat("H2") << endl;
    
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatBorder  (2) H2 format", wFormatStr ==wResultFormat);
    
    m_Api->UndoCellBorder("A1:C2;B2:B3",tBorderAll, "solid 1px black;");
    m_Api->UndoCellBorder("A1:C2;B2:B3",tBorderAll, "solid 1px black;");
    
    m_Api->UndoCellBorder("B7:H10;B10:B12",tBorderAll, "solid 1px black;");
    m_Api->UndoCellBorder("B7:H10;B10:B12",tBorderAll, "solid 1px blue;");
    
    m_Api->UndoCellBorder("B7:H10;B10:B12",tBorderAll, "");
   
    
    m_Api->UndoCellBorder("B7:B8",tBorderAll, "solid 1px black;");
    m_Api->UndoCellBorder("B7:B8",tBorderAll, "solid 3.5px blue;");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellBorder("A1",tBorderAll, "solid 1px black;");
    
    //cout << endl  << "A1 Format="<< m_Api->CellFormat("A1") << ";" << endl;
    
    m_Api->UndoCellBorder("A1",tBorderBottom, "");
    
    //cout << endl  << "A1 Format="<< m_Api->CellFormat("A1") << ";" << endl;
 
    m_Api->UndoCellBorder("A1",tBorderAll, "");
    
    tString wJson=m_Api->JsonView(1,1,tUnitMetrics::pixels,224,1240,0,0,true);
    
    //cout << wJson << endl;
    
    tRect wRect=m_Api->GetSheetSelect();
    tString wSelect=wRect.StrRef();
    m_Api->UndoCellFormat(wSelect,"color:red;");
    
    
    wRect=m_Api->GetRowSelect(1,4);
    wSelect=wRect.StrRef();
    m_Api->UndoCellFormat(wSelect,"background-color:yellow;");
    
    
    wRect=m_Api->GetColSelect(1,4);
    wSelect=wRect.StrRef();
    m_Api->UndoCellFormat(wSelect,"padding: 3px;");
    
    
    wFormatStr=m_Api->CellFormat("A2");

    m_Api->UndoCellBorder("A1:F20;F1:H10;A1",tBorderAll, "solid 1px black;");
    
    wFormatStr=m_Api->CellFormat("A1");
    //cout << endl <<  wFormatStr << endl;
    // Cell-local storage: tBorderAll on A1:F20 stores all four sides on each cell.
    wResultFormat="color:red;background-color:yellow;padding:3px;border-left:solid 1px black;border-top:solid 1px black;border-right:solid 1px black;border-bottom:solid 1px black;";
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatBorder (3) A1 format", wFormatStr ==wResultFormat);
    
    
    m_Api->UndoCellBorder("A1:F20;F1:H10",tBorderAll, "");

    wFormatStr=m_Api->CellFormat("A1");
    //cout << endl <<  wFormatStr << endl;
    // Erasing tBorderAll clears borders on each cell of the range only.
    tString wResultFormat2="color:red;background-color:yellow;padding:3px;";
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatBorder (4) A1 format", wFormatStr ==wResultFormat2);

    m_Api->Undo();
    
    wJson=m_Api->JsonView(1,1,tUnitMetrics::pixels,224,124,0,0,false);
    
    //cout << wJson << endl;

#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
    
    wFormatStr=m_Api->CellFormat("A1");
    //cout << endl <<  wFormatStr << endl;
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatBorder  (5) A1 format", wFormatStr ==wResultFormat);
        
    m_Api->Undo();
    
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Redo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    // Error
    m_Api->UndoRaz("B7:H10;B5:B12;D4:D13;F5:F13");
    m_Api->Undo();
    
    m_Api->UndoCellBorder("B7:H10;B5:B12;D4:D13;F5:F13",tBorderAll, "solid 1px black;");
    
    wFormatStr=m_Api->CellFormat("B7");
    //cout << endl << wFormatStr << endl;

    m_Api->UndoCellBorder("B7:H10;B5:B12;D4:D13;F5:F13;H5:H12",tBorderAll, "solid 1px black;");
    // Error
    m_Api->UndoCellBorder("B7:H10;B5:B12;D4:D13;F5:F13;H5:H12",tBorderAll, "solid 3.5px blue;");
    
    m_Api->UndoCellBorder("B7",tBorderAll, "solid 1px black;");
    m_Api->UndoCellBorder("B7",tBorderAll, "solid 3px blue;");
    m_Api->UndoCellBorder("B7",tBorderLeft, "solid 1px black;");
    m_Api->UndoCellBorder("B7",tBorderLeft, "solid 3px blue;");
    m_Api->UndoCellBorder("B7",tBorderRight, "solid 1px black;");
    m_Api->UndoCellBorder("B7",tBorderRight, "solid 3px blue;");

    m_Api->UndoCellFormat("A1:G10","color:red;");
    //m_Api->UndoCellFormat("A1:G10","background-color:yellow;");
    wFormatStr=m_Api->CellFormat("B7");
    //cout << endl << wFormatStr << endl;


    m_Api->UndoCellFormat("A1:G10","font:\"Avenir\",serif 12pt;");
    // Cell-local storage: B7 carries top, left, right and bottom on the same cell.
    wResultFormat="color:red;padding:3px;font:\"Avenir\",serif 12pt;border-left:solid 3px blue;border-top:solid 3px blue;border-right:solid 3px blue;border-bottom:solid 3px blue;";
    wFormatStr=m_Api->CellFormat("B7");
    
    CPPUNIT_ASSERT_MESSAGE("TestSkFormatBorder (6) B7 format", wFormatStr ==wResultFormat);
    
    //cout << wFormatStr << endl;
    
    wFormatStr=m_Api->CellFormat("G2");
    //cout << wFormatStr << endl;

    m_Api->UndoCellBorder("A1:G10",tBorderAll, "");
    wFormatStr=m_Api->CellFormat("B7");
    //cout << wFormatStr << endl;

    m_Api->Undo();
    m_Api->Redo();
    // Stress Redo Undo =====================================================
    for(tInt wInd=0;wInd<40;wInd++) {
        UndoOperation();
        m_Api->Undo();
    }
    for(tInt wInd=0;wInd<40;wInd++) {
        RedoOperation();
        m_Api->Redo();
    }
    
    tApplication::Instance()->ClearUndoRedo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void  TestSkFormatSpreadSheet::TestSkFormatBorderUndoRedo() {
#ifdef drawdebug
    cout << endl << "Background Do B2:D5 -----------------------" << endl;
#endif
    
    m_Api->UndoCellFormat("B2:D5;B2:B30;","background-color:yellow;");
#ifdef drawdebug
    cout << endl << "Do A3:E6 -----------------------" << endl;
#endif
    m_Api->UndoCellBorder("A3:E6;A3:A10;A3",tBorderAll,"dashed black 3px;");
    
#ifdef drawdebug
    tCell* wCellB3=m_Api->Cell("B3");
    tFormatCss* wFormatB3 = m_FormatRoot->FormatRef(wCellB3->Css());
    cout <<  "B3-->" << wFormatB3->CssStr(m_FormatRoot) << endl;
#endif
    m_Api->Undo();
    
#ifdef drawdebug
    wCellB3=m_Api->Cell("B3");
    wFormatB3 = m_FormatRoot->FormatRef(wCellB3->Css());
    cout <<  "B3-->" << wFormatB3->CssStr(m_FormatRoot) << endl;
#endif

    
#ifdef drawdebug
    cout << endl << "Do C2:D4------------------------" << endl;
#endif
    m_Api->UndoCellBorder("C2:D4",tBorderAll,"dashed black 3px;");
#ifdef drawdebug
    tCell* wCellD3=m_Api->Cell("D3");
    tFormatCss* wFormatD3 = m_FormatRoot->FormatRef(wCellD3->Css());
    cout <<  "D3-->" << wFormatD3->CssStr(m_FormatRoot) << endl;
#endif
    
#ifdef drawdebug
    cout << endl << "Do D3:E4------------------------" << endl;
#endif
    m_Api->UndoCellBorder("D3:E4",tBorderAll,"dashed black 3px;");
    
#ifdef drawdebug
    wCellD3=m_Api->Cell("D3");
    wFormatD3 = m_FormatRoot->FormatRef(wCellD3->Css());
    cout <<  "D3-->" << wFormatD3->CssStr(m_FormatRoot) << endl;
#endif
    m_Api->Undo();
    
#ifdef drawdebug
    wCellD3=m_Api->Cell("D3");
    wFormatD3 = m_FormatRoot->FormatRef(wCellD3->Css());
    if (wFormatD3!=nullptr)
        cout <<  "D3-->" << wFormatD3->CssStr(m_FormatRoot) << endl;
#endif


    m_Api->UndoCellBorder("A3:D5",tBorderAll,"dashed black 3px;");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellBorder("A3:D5",tBorderLeft,"");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->UndoCellBorder("A3:D5",tBorderRight,"");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
    
    m_Api->UndoCellBorder("D5:E7;J6:O9",tBorderAll,"solid yellow 3px;");
    Debug();
    
    

    /*
    m_Api->UndoCellFormat("C2","background-color:yellow;");
    Debug();
    m_Api->UndoCellFormat("C2","background-color:yellow;");
    Debug();
     */
   
    
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
    Debug();
    m_Api->UndoCellBorder("C3",tBorderAll,"dashed black 3px;");
    Debug();
#ifdef drawdebug
    tCell* wCellC3=m_Api->Cell("C3");
    tFormatCss* wFormatC3 = m_FormatRoot->FormatRef(wCellC3->Css());
    cout <<  "C3-->" << wFormatC3->CssStr(m_FormatRoot) << endl;
#endif
    m_Api->UndoCellBorder("C2:C3",tBorderAll,"dashed black 3px;");
    
#ifdef drawdebug
    tCell* wCellC2=m_Api->Cell("C2");
    tFormatCss* wFormatC2 = m_FormatRoot->FormatRef(wCellC2->Css());
    cout << "C2-->" << wFormatC2->CssStr(m_FormatRoot) << endl;
    wFormatC3 = m_FormatRoot->FormatRef(wCellC3->Css());
    cout << "C3-->" << wFormatC3->CssStr(m_FormatRoot) << endl;
#endif
    Debug();
    
    m_Api->UndoCellFormat("D3:F20","background-color:yellow;");
    Debug();
    m_Api->UndoCellFormat("E7:H10;B16:L17","background-color:lightblue;");
    Debug();
    m_Api->UndoCellBorder("C2:E12;J18:O22",tBorderAll,"dashed black 3px;");
    Debug();
    m_Api->UndoCellBorder("D5:E7;J6:O9",tBorderAll,"solid yellow 3px;");
    Debug();
    m_Api->UndoCellBorder("A20:G20",tBorderAll,"solid red 2px;");
    Debug();
    m_Api->UndoCellBorder("A12:Z22",tBorderRight,"solid yellow 3px;");
    Debug();
    m_Api->UndoCellBorder("A1:A1",tBorderTop,"solid red 3px;");
    Debug();
    m_Api->UndoApplyMerge("D5:E7");
    Debug();
    m_Api->UndoCellFormat("D5","background-color:lightblue;font:\"Arial\",serif 32pt;");
    Debug();
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
    
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    m_Api->Undo();
    
    
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif    
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->Undo();
#ifdef checkfo
    m_Api->CheckFormat();
#endif

    m_Api->Redo();
    m_Api->Redo();
    m_Api->Redo();
    m_Api->Redo();
    m_Api->Redo();
    m_Api->Redo();
    m_Api->Redo();
    m_Api->Redo();

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkFormatSpreadSheet::TestSkFormatRead() {
   tFile wFile("SpreadSheet.json");
   tString wSaveJson=wFile.LoadString();
   m_Api->ReadJson(wSaveJson);
#ifdef checkfo
    m_Api->CheckFormat();
#endif
    m_Api->DeleteSheet("Sheet1");
#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

tString    wStaticSaveFile;

void TestSkFormatSpreadSheet::TestSaveWorkBook() {
    m_Api->NewWorkBook("C");
    m_Api->NewWorkBook("A");
    m_Api->NewWorkBook("B");
    
    m_Api->AddWorkBook("B");
    
    m_Api->ActiveWorkBook("C");
    m_Api->UndoCellFormat("A1:G10","color:red;");
    
    //cout << m_Api->CellFormat("A2") << endl;
    m_Api->ActiveWorkBook("A");
    //cout << m_Api->CellFormat("A2") << endl;
    m_Api->CellValue("A3","Coucou");
    m_Api->UndoCellFormat("A1:G10","color:blue;");
    m_Api->UndoApplyMerge("A2:A3");
    m_Api->UndoCellFormat("A1:G10","background-color:yellow;font:\"Courier New\",serif 32pt;");

    m_Api->UndoCellBorder("A2:G4",tBorderAll,"dashed black 3px;");
    
    tString wTestResult="color:blue;background-color:yellow;font:\"Courier New\",serif 32pt;border-left:dashed 3px black;border-top:dashed 3px black;border-right:dashed 3px black;border-bottom:dashed 3px black;";
    
    CPPUNIT_ASSERT_MESSAGE("TestSaveWorkBook A", m_Api->CellFormat("A2")==wTestResult);
    
   // cout << m_Api->CellFormat("A2") << endl;
    
    wStaticSaveFile=m_Api->WriteJson("A");
    //cout << wJson;
    
    
#ifdef checksp
    m_Api->Check();
#endif
#ifdef checkfo
    m_Api->CheckFormat();
#endif
}
void TestSkFormatSpreadSheet::TestLoadWorkBook() {
    
    
    m_Api->ReadJson(wStaticSaveFile);
#ifdef checksp
    m_Api->Check();
#endif
#ifdef checkfo
    m_Api->CheckFormat();
#endif

    //cout << m_Api->CellFormat("A2") << endl;
    tString wTestResult="color:blue;background-color:yellow;font:\"Courier New\",serif 32pt;border-left:dashed 3px black;border-top:dashed 3px black;border-right:dashed 3px black;border-bottom:dashed 3px black;";
    
    CPPUNIT_ASSERT_MESSAGE("TestSaveWorkBook A", m_Api->CellFormat("A2")==wTestResult);
}

void TestSkFormatSpreadSheet::TestJsonView() {
    // Left
    m_Api->CellValue("A1", "A1");
    
    m_Api->CellValue("D1","D1");
    
    // Right
    m_Api->CellValue("H1", "H1");
    
    m_Api->UndoCellFormat("A1:H1","vertical-align:top;");
    m_Api->UndoCellFormat("A1:H1","text-align:right;");
    
    
    tString wResult=m_Api->JsonView(1, 3, tUnitMetrics::pixels, 12, 300,0,0, true);
    
    //cout << endl << wResult << endl;
    
    Document wDocument;
    wDocument.Parse(wResult.c_str());
    
    Value& wRows=wDocument["rows"];
    CPPUNIT_ASSERT_MESSAGE("TestJsonView rows", wRows.IsArray());
    
    Value& wRow=wRows[0];
    Value& wCells=wRow["cells"];
    CPPUNIT_ASSERT_MESSAGE("TestJsonView cells", wCells.IsArray());
    
    Value& wCell=wCells[0];
    CPPUNIT_ASSERT_MESSAGE("TestJsonView cell align horizontal", CellHasMember(wDocument, wCell, "f_ah"));
    CPPUNIT_ASSERT_MESSAGE("TestJsonView cell align horizontal", CellMemberInt(wDocument, wCell, "f_ah")==3);
   
    CPPUNIT_ASSERT_MESSAGE("TestJsonView cell align vertical", CellHasMember(wDocument, wCell, "f_av"));
    CPPUNIT_ASSERT_MESSAGE("TestJsonView cell align vertical", CellMemberInt(wDocument, wCell, "f_av")==8);
}

// Reproduces the reported bug: applying "All Borders" on A1:C2 then merging
// A1:C2 should still display the right and bottom sides of the merged range.
// These sides live on the D column and row 3 respectively (Excel adjacency
// storage), and tJsonView::EmitMergedBorders must project them onto the
// anchor as f_bor / f_bob.
void TestSkFormatSpreadSheet::TestJsonViewMergedBorders() {
    // 1. Apply All Borders on A1:C2
    m_Api->UndoCellBorder("A1:C2", tBorderAll, "solid 1px black;");

    // Sanity check cell-local storage: right/bottom live on C1/C2, not on neighbors.
    tString wC1Css = m_Api->CellFormat("C1");
    tString wC2Css = m_Api->CellFormat("C2");
    CPPUNIT_ASSERT_MESSAGE(("TestJsonViewMergedBorders C1 border-right expected, got: " + wC1Css).c_str(),
                           wC1Css.find("border-right") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE(("TestJsonViewMergedBorders C2 border-bottom expected, got: " + wC2Css).c_str(),
                           wC2Css.find("border-bottom") != tString::npos);

    // 2. Merge A1:C2
    m_Api->UndoApplyMerge("A1:C2");

    // 3. Ask tJsonView for a window large enough to cover rows 1..3 and cols A..D
    tString wResult = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 200, 400, 0, 0, true);

    Document wDocument;
    wDocument.Parse(wResult.c_str());
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders JSON parse", !wDocument.HasParseError());
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders has rows", wDocument.HasMember("rows") && wDocument["rows"].IsArray());

    // Locate the merged anchor cell (c_r=1, c_c=1) in the emitted rows.
    const Value* wAnchor = nullptr;
    const Value& wRows = wDocument["rows"];
    for (SizeType wR = 0; wR < wRows.Size() && wAnchor == nullptr; wR++) {
        if (!wRows[wR].HasMember("cells")) continue;
        const Value& wCells = wRows[wR]["cells"];
        if (!wCells.IsArray()) continue;
        for (SizeType wC = 0; wC < wCells.Size(); wC++) {
            const Value& wCell = wCells[wC];
            if (wCell.HasMember("c_r") && wCell.HasMember("c_c") &&
                wCell["c_r"].GetInt() == 1 && wCell["c_c"].GetInt() == 1) {
                wAnchor = &wCell;
                break;
            }
        }
    }
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders anchor A1 emitted", wAnchor != nullptr);

    // Anchor must carry the merged dimensions and its own top/left borders.
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders anchor c_w present", wAnchor->HasMember("c_w"));
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders anchor c_h present", wAnchor->HasMember("c_h"));
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders anchor f_bol present",
                           CellHasMember(wDocument, *wAnchor, "f_bol"));
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders anchor f_bot present",
                           CellHasMember(wDocument, *wAnchor, "f_bot"));

    // f_bor / f_bob come from edge cells of the merge (owner-model JsonView).
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders anchor f_bor (right) present after merge",
                           CellHasMember(wDocument, *wAnchor, "f_bor"));
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders anchor f_bob (bottom) present after merge",
                           CellHasMember(wDocument, *wAnchor, "f_bob"));

    // 4. Re-run with sCss=false (canvas mode, what the WASM front-end really
    // uses). The f_bor / f_bob values must be canvas objects (StartObject
    // with keys c, w, s) — not CSS strings.
    tString wResultCanvas = m_Api->JsonView(1, 1, tUnitMetrics::pixels, 200, 400, 0, 0, false);
    Document wDocCanvas;
    wDocCanvas.Parse(wResultCanvas.c_str());
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders canvas JSON parse", !wDocCanvas.HasParseError());

    const Value* wAnchorC = nullptr;
    const Value& wRowsC = wDocCanvas["rows"];
    for (SizeType wR = 0; wR < wRowsC.Size() && wAnchorC == nullptr; wR++) {
        if (!wRowsC[wR].HasMember("cells")) continue;
        const Value& wCellsC = wRowsC[wR]["cells"];
        if (!wCellsC.IsArray()) continue;
        for (SizeType wC = 0; wC < wCellsC.Size(); wC++) {
            const Value& wCellC = wCellsC[wC];
            if (wCellC.HasMember("c_r") && wCellC.HasMember("c_c") &&
                wCellC["c_r"].GetInt() == 1 && wCellC["c_c"].GetInt() == 1) {
                wAnchorC = &wCellC;
                break;
            }
        }
    }
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders canvas anchor A1 emitted", wAnchorC != nullptr);
    const Value* wFbor = CellMemberValue(wDocCanvas, *wAnchorC, "f_bor");
    const Value* wFbob = CellMemberValue(wDocCanvas, *wAnchorC, "f_bob");
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders canvas anchor f_bor present", wFbor != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders canvas anchor f_bob present", wFbob != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders canvas f_bor is an object", wFbor->IsObject());
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders canvas f_bob is an object", wFbob->IsObject());
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders canvas f_bor has color", wFbor->HasMember("c"));
    CPPUNIT_ASSERT_MESSAGE("TestJsonViewMergedBorders canvas f_bor has width", wFbor->HasMember("w"));
}

void TestSkFormatSpreadSheet::TestBorderApplyClearBlockCheckfo() {
    // User scenario: 4x4 block D5:G8, apply then clear all borders (no merge).
    m_Api->UndoCellBorder("D5:G8", tBorderAll, "solid 1px black;");
    CPPUNIT_ASSERT_MESSAGE("TestBorderApplyClearBlockCheckfo D5 bordered",
        m_Api->CellFormat("D5").find("border-left") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestBorderApplyClearBlockCheckfo G8 bordered",
        m_Api->CellFormat("G8").find("border-right") != tString::npos);

    CPPUNIT_ASSERT(m_Api->UndoCellBorder("D5:G8", tBorderAll, ""));

    CPPUNIT_ASSERT_MESSAGE("TestBorderApplyClearBlockCheckfo D5 cleared",
        m_Api->Cell("D5") == nullptr ||
        m_Api->CellFormat("D5").find("border") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestBorderApplyClearBlockCheckfo E6 cleared",
        m_Api->Cell("E6") == nullptr ||
        m_Api->CellFormat("E6").find("border") == tString::npos);

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkFormatSpreadSheet::TestAdjacentCells() {
    m_Api->UndoCellValue("E15", tVariant("ALLEZ"));
    m_Api->UndoCellFormat("E15", "background-color:yellow;");
    m_Api->UndoCellValue("D16", tVariant("Stephane"));
    m_Api->UndoCellFormat("D16", "background-color:orange;");
    m_Api->UndoCellBorder("D15", tBorderAll, "solid 1px black;");

    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells D15 all sides",
        m_Api->CellFormat("D15").find("border-right") != tString::npos &&
        m_Api->CellFormat("D15").find("border-bottom") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells E15 yellow before raz",
        m_Api->CellFormat("E15").find("background-color:yellow") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells E15 no neighbor seam before raz",
        m_Api->CellFormat("E15").find("border-left") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells D16 orange before raz",
        m_Api->CellFormat("D16").find("background-color:orange") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells D16 no neighbor seam before raz",
        m_Api->CellFormat("D16").find("border-top") == tString::npos);

#ifdef checkfo
    m_Api->CheckFormat();
#endif

    CPPUNIT_ASSERT(m_Api->UndoRaz("D15"));

    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells D15 cleared", m_Api->Cell("D15") == nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells E15 kept",
        m_Api->Cell("E15") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells E15 yellow after raz",
        m_Api->CellFormat("E15").find("background-color:yellow") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells E15 still no border after raz",
        m_Api->CellFormat("E15").find("border-left") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells D16 kept",
        m_Api->Cell("D16") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells D16 orange after raz",
        m_Api->CellFormat("D16").find("background-color:orange") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells D16 still no border after raz",
        m_Api->CellFormat("D16").find("border-top") == tString::npos);

#ifdef checkfo
    m_Api->CheckFormat();
#endif

    CPPUNIT_ASSERT(m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells undo D15",
        m_Api->CellFormat("D15").find("border-left") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells undo E15 yellow",
        m_Api->CellFormat("E15").find("background-color:yellow") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestAdjacentCells undo D16 orange",
        m_Api->CellFormat("D16").find("background-color:orange") != tString::npos);

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkFormatSpreadSheet::TestCopy() {
    
    m_Api->UndoCellFormat("A2:H2","background-color:blue;");
    m_Api->UndoCellFormat("A1:H1","vertical-align:top;");
    m_Api->UndoCellFormat("A1:H1","text-align:right;");
    m_Api->UndoCellFormat("B1:D1","color:red;");
    m_Api->Copy("A1:H1");
    
   // cout << endl <<  m_Application->Clipboard()->Text() << endl;
    
    if (m_Api->UndoPaste("A2:H2")) {
#ifdef drawdebug
        tCell* wCellC2=m_Api->Cell("C2");
        tFormatCss* wFormatC2 = m_FormatRoot->FormatRef(wCellC2->Css());
        cout << "C2-->" << wFormatC2->CssStr(m_FormatRoot) << endl;
#endif
        m_Api->Undo();
    }
}

void TestSkFormatSpreadSheet::TestSkPressure() {
    m_NbRow=10;
    m_NbCol=10;
    Fill();
    
    m_Api->UndoCellFormat("B2:C2","background-color:yellow;");
    m_Api->UndoCellFormat("B2:D5;B2:B30;","background-color:yellow;");
    m_Api->UndoCellBorder("A2:J8;B4:C5",tBorderAll, "1px solid black;");
    m_Api->UndoCellFormat("A3:C7","font: \"Arial\", serif italic 12pt bold ultra-condensed small-caps;");
    
    m_Api->UndoDeleteRow(1,3);
    
    m_Api->Undo();
    
    tString wResultFormat=m_Api->CellFormat("C5");
    //cout << wResultFormat << endl;
    DrawCell("Before Delete Col", 1, 1, m_NbRow, m_NbCol);
    tCell* wCell=m_Api->Cell(m_NbRow,m_NbCol);
    //cout <<endl << wCell->StrRef()  << ":" << wCell->FormulaStr() << "=" << wCell->Value() << endl;
    for(tIndex wCol=1;wCol<=m_NbCol-2;wCol++) {
        m_Api->UndoDeleteCol(1,1);
        DrawCell("Delete ", 1, 1, m_NbRow, m_NbCol);
    }
    
    for(tIndex wCol=1;wCol<=m_NbCol-2;wCol++) {
        //cout << tApplication::Instance()->DebugUndo() << endl;
        m_Api->Undo();
        DrawCell("Undo ", 1, 1, m_NbRow, m_NbCol);
    }
  
    
    CPPUNIT_ASSERT_MESSAGE("TestSkPressure Delete Col J10", wCell->Value().Int() == 3240);
    CPPUNIT_ASSERT_MESSAGE("TestSkPressure Delete Col J10", wCell->FormulaStr() == "SUM(A10:I10)");

    DrawCell("Before Delete Row", 1, 1, m_NbRow, m_NbCol);
    
    //cout <<endl << wCell->StrRef()  << ":" << wCell->FormulaStr() << "=" << wCell->Value() << endl;
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
    
    m_Api->Cell(m_NbRow,m_NbCol);
    
    
    //cout <<endl << wCell->StrRef()  << ":" << wCell->FormulaStr() << "=" << wCell->Value() << endl;
    
    wCell = m_Api->Cell(m_NbRow,m_NbCol);
    
    CPPUNIT_ASSERT_MESSAGE("TestSkPressure Delete Row J10", wCell->Value().Int() == 3240);
    CPPUNIT_ASSERT_MESSAGE("TestSkPressure Delete Row J10", wCell->FormulaStr() == "SUM(A10:I10)");
    CPPUNIT_ASSERT_MESSAGE("TestSkPressure Delete Row J10 Format", m_Api->CellFormat("C5")==wResultFormat);
}


void TestSkFormatSpreadSheet::setUp() {
    std::filesystem::remove_all("./Spreadsheet");
    
    m_Application = tApplication::Instance();
    m_FormatRoot = tFormatRoot::Instance();
    m_Api = new SkSpreadSheet::tApi;
    m_Api->IsUndoActif(true);
    m_Api->NewWorkBook("wwww.skeema.fr/w1");
    m_FormatApi=new SkFormat::tFormatCssApi();
    m_Api->FormatApi((tFormatApi*)m_FormatApi);
};

void TestSkFormatSpreadSheet::tearDown() {
    delete(m_Api);
    m_Api = nullptr;
    TestSkFormatTeardown::AssertFormatPoolEmptyAfterApiDelete(m_FormatApi, "TestSkFormatSpreadSheet");
    delete(m_FormatApi);
    m_FormatApi = nullptr;

	DoneFormatRoot();
}

//==============================================================================
// TestSkCopyPaste
//==============================================================================
#include "../include/TestSkCopyPaste.hpp"
#include <SkFormatRoot.hpp>
#include <SkFormatCssApi.hpp>
#include <SkMessage.hpp>
#include <SkUndoRedoSp.hpp>
#include <SkCopyPaste.hpp>
#ifdef TestMultiUser
#include <SkInterfaceWeb.hpp>
#include <SkCollaborationLimits.hpp>
#endif
#include <SkClipboard.hpp>

using namespace SkFormat;

namespace {

static const char* kInterfaceWebTestWorkBook = "wwww.skeema.fr/w-interface-web-mono";

static bool CellIntValueEq(tCell* sCell, tInt sExpected) {
    if (sCell == nullptr) {
        return false;
    }
    const tVariant& wV = sCell->Value();
    if (wV.IsInt()) {
        return wV.Int() == sExpected;
    }
    if (wV.IsDouble()) {
        return wV.Double() == static_cast<tDouble>(sExpected);
    }
    return false;
}

} // namespace
TestSkCopyPaste::TestSkCopyPaste() :CPPUNIT_NS::TestFixture(), m_Application(nullptr), m_FormatApi(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
    m_NbRow = 8;
    m_NbCol = 10;
}

void TestSkCopyPaste::DrawCell(tString sOperation,tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd) {
    return; // drop
    cout << endl;
	cout << sOperation << endl;
	for (tInt wRow = sRowBegin; wRow <= sRowEnd; wRow++) {
		for (tInt wCol = sColBegin; wCol <= sColEnd; wCol++) {
			tVariant wVariant = m_Api->CellValue(wRow, wCol);
			tString wFormula = m_Api->Formula(wRow, wCol);
			cout << Base10ToAlpha(wCol) << wRow << "=" << wFormula << ";" << wVariant << "\t";
		}
		cout << endl;
	}
}

void TestSkCopyPaste::Fill() {
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

void TestSkCopyPaste::UndoOperation() {
	return; // Drop 
	tUndo* wUndo = m_Api->LastUndo();
	if (wUndo != nullptr) {
		cout << "Undo ->" << wUndo->OperationName() << endl;
	}
}

void TestSkCopyPaste::TestCopy() {
    
    //1 Simple Copy
    m_Api->UndoCellValue("A1", tVariant(123));
    if (!m_Api->Copy("A1")) {
        CPPUNIT_ASSERT_MESSAGE("TestCopy Copy A1 (return fale)", false);
    }

    m_Api->UndoCellValue("B1", tVariant("Coucou !"));
    tCell* wCellB1= m_Api->Cell("B1");
    
    m_Api->UndoCellValue("C1", tVariant(("=B1")));
    
    CPPUNIT_ASSERT_MESSAGE("TestCopy C1 (width formula)", wCellB1->Value()==tVariant("Coucou !"));

    if (!m_Api->UndoPaste("B1")) {
        CPPUNIT_ASSERT_MESSAGE("TestCopy Paste B1 (return fale)", false);
    }
    CPPUNIT_ASSERT_MESSAGE("TestCopy B1", wCellB1->Value()==tVariant(123));
    if (!m_Api->UndoPaste("C1")) {
        CPPUNIT_ASSERT_MESSAGE("TestCopy Paste C1 (return fale)", false);
    };
    CPPUNIT_ASSERT_MESSAGE("TestCopy C1 (width formula) after copy", wCellB1->Value()==tVariant(123));
    
    
	// 2  copy paste simple (with null after undo copy)
    if (!m_Api->UndoPaste("C8")) {
        CPPUNIT_ASSERT_MESSAGE("TestCopy Paste C8 (return fale)", false);
    };
	tCell* wCellA1 = m_Api->Cell("A1");
    CPPUNIT_ASSERT_MESSAGE("TestCopy A1", wCellA1 != nullptr);

	tCell* wCellC8 = m_Api->Cell("C8");

	tVariant wVariant = wCellC8->Value();
	CPPUNIT_ASSERT_MESSAGE("TestCopy C8=123", (wVariant.Type() == tVariantType::t_int) &&
												(wVariant.Int()==123));

	UndoOperation();
	m_Api->Undo();

	wCellC8 = m_Api->Cell("C8");
	CPPUNIT_ASSERT_MESSAGE("TestCopy C8=nullptr", wCellC8 == nullptr);

	m_Api->Redo();
	m_Api->UndoCellValue("B2", "=A1+12");

	tCell* wCellB2 = m_Api->Cell("B2");
	wVariant = wCellB2->Value();
	CPPUNIT_ASSERT_MESSAGE("TestCopy B2 C8=135", wVariant.Int() == 135);

	m_Api->Copy("B2");

	m_Api->UndoPaste("D9");
	tCell* wCellD9 = m_Api->Cell("D9");
    CPPUNIT_ASSERT_MESSAGE("TestCopy D9", wCellD9 != nullptr);
	//cout << wCellD9->StrRef() << ":" << wCellD9->FormulaStr() << "=" << wCellD9->Value();

	//DrawCell("Final", 1, 1, 10, 10);
	CPPUNIT_ASSERT_MESSAGE("TestCopy B2  C9=135", wVariant.Int() == 135);
    
}

void TestSkCopyPaste::TestCopyRelativeAbsolute() {
	// 1 copy paste simple (with null after undo copy)
	m_Api->UndoCellValue("A1", tVariant(123));
	m_Api->UndoCellValue("B1", tVariant(321));
	m_Api->UndoCellValue("A2", "=$A$1");
	tCell* wCellA2 = m_Api->Cell("A2");
    
    CPPUNIT_ASSERT_MESSAGE("TestCopyRelativeAbsolute A2", wCellA2 != nullptr);
	//cout << endl;
	//cout << wCellA2->StrRef() << ":" << wCellA2->FormulaStr(true) << "=" << wCellA2->Value();

	DrawCell("Final", 1, 1, 10, 10);

	m_Api->Copy("A2");

	m_Api->UndoPaste("B2");
	tCell* wCellB2 = m_Api->Cell("B2");
    CPPUNIT_ASSERT_MESSAGE("TestCopyRelativeAbsolute B2", wCellB2 != nullptr);

	CPPUNIT_ASSERT_MESSAGE("TestCopy B2=123", m_Api->Cell("B2")->Value().Int() == 123);

	
	m_Api->UndoCellValue("A3", "=A$1");
	m_Api->Copy("A3");

	m_Api->UndoPaste("B3");
    
    DrawCell("UndoPaste", 1, 1, 10, 10);
    
	tCell* wCellB3 = m_Api->Cell("B3");
    CPPUNIT_ASSERT_MESSAGE("TestCopyRelativeAbsolute B3", wCellB3 != nullptr);

	CPPUNIT_ASSERT_MESSAGE("TestCopy B3=135", m_Api->Cell("B3")->Value().Int() == 321);

}

void TestSkCopyPaste::TestCopySimpleRange() {
    m_Api->UndoCellValue("A1:A20", tVariant(123));
    
    m_Api->Copy("A1:A20");
    m_Api->UndoPaste("B1");
    
    CPPUNIT_ASSERT_MESSAGE("TestPaste B3=K3", m_Api->Cell("B2")->Value().Int() == 123);
   
    
    m_Api->Undo();
    
    CPPUNIT_ASSERT_MESSAGE("TestPaste B3=K3", m_Api->Cell("B2") == nullptr);
}


void TestSkCopyPaste::TestCopyMultiple() {
    m_Api->UndoCellValue("A2:J2", tVariant(123));
    
    DrawCell("Fill Sheet1", 1, 1, 8, 10);
    
    m_Api->Copy("A2:J2");
    
    m_Api->UndoPaste("B3:K3");
    DrawCell("Paste A2:J2 to B3:K3", 1, 1, 8, 10);
    
    CPPUNIT_ASSERT_MESSAGE("TestPaste B3=K3", m_Api->Cell("K3")->Value().Int() == 123);
   
    m_Api->Undo();
    DrawCell("Undo Paste", 1, 1, 12, 11);
    CPPUNIT_ASSERT_MESSAGE("TestPaste B3=K3", m_Api->Cell("K3")==nullptr);

    m_Api->Redo();
    CPPUNIT_ASSERT_MESSAGE("TestPaste Redo B3=K3", m_Api->Cell("K3")->Value().Int() == 123);
 

    m_Api->Copy("A2");
    
    m_Api->UndoCellValue("A5", tVariant(236));
    
    m_Api->UndoPaste("A5:B7");
    DrawCell("Undo Paste", 1, 1, 12, 11);
    CPPUNIT_ASSERT_MESSAGE("TestPaste A5:B7", m_Api->Cell("B7")->Value().Int() == 123);
    
    m_Api->Undo();
    DrawCell("Undo Paste", 1, 1, 12, 11);
    CPPUNIT_ASSERT_MESSAGE("TestPaste undo A5:B7", m_Api->Cell("B7") == nullptr);
    
    CPPUNIT_ASSERT_MESSAGE("TestPaste A5:B7", m_Api->Cell("A5")->Value().Int() == 236);
   
}

void TestSkCopyPaste::TestCopyBorder() {
    m_Api->UndoCellBorder("B2", tBorderAll, "solid 1px black;");
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorder source B2 right",
        m_Api->CellFormat("B2").find("border-right") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorder source B2 bottom",
        m_Api->CellFormat("B2").find("border-bottom") != tString::npos);

    m_Api->Copy("B2");
    m_Api->UndoPaste("D5");

    tString wDest = m_Api->CellFormat("D5");
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorder D5 left",
        wDest.find("border-left") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorder D5 top",
        wDest.find("border-top") != tString::npos);

    tString wRight = m_Api->CellFormat("D5");
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorder D5 right on cell",
        wRight.find("border-right") != tString::npos);

    tString wBottom = m_Api->CellFormat("D5");
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorder D5 bottom on cell",
        wBottom.find("border-bottom") != tString::npos);
}

void TestSkCopyPaste::TestCopyBorderNeighborBackground() {
    m_Api->UndoCellValue("D4", tVariant(1));
    m_Api->UndoCellValue("E4", tVariant(2));
    m_Api->UndoCellFormat("D5:H5", "background-color:green;");
    m_Api->UndoCellBorder("D4:E4", tBorderAll, "solid 1px black;");

    tString wBeforeCopy = m_Api->CellFormat("E5");
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderNeighborBackground E5 green before copy",
        wBeforeCopy.find("background-color:green") != tString::npos);

    m_Api->Copy("D4:E4");

    tString wAfterCopy = m_Api->CellFormat("E5");
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderNeighborBackground E5 green after copy",
        wAfterCopy.find("background-color:green") != tString::npos);

    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderNeighborBackground H5 green before paste",
        m_Api->CellFormat("H5").find("background-color:green") != tString::npos);

    m_Api->UndoPaste("G4");

    tString wNeighborPaste = m_Api->CellFormat("H5");
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderNeighborBackground H5 green after paste",
        wNeighborPaste.find("background-color:green") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderNeighborBackground H4 bottom after paste",
        m_Api->CellFormat("H4").find("border-bottom") != tString::npos);
}

void TestSkCopyPaste::TestBorderOwnerWriteReadJson() {
    const tString wUri = "wwww.skeema.fr/w1";

    m_Api->UndoCellFormat("D5:H5", "background-color:#9FC5E8;");
    m_Api->UndoCellBorder("D4:E4", tBorderAll, "solid 1px black;");

    m_Api->Copy("D4:E4");
    CPPUNIT_ASSERT(m_Api->UndoPaste("G4"));

    tString wNeighbor = m_Api->CellFormat("H5");
    CPPUNIT_ASSERT(wNeighbor.find("background-color:#9FC5E8") != tString::npos);
    CPPUNIT_ASSERT(m_Api->CellFormat("H4").find("border-bottom") != tString::npos);

    const tString wJson = m_Api->WriteJson(wUri);
    m_Api->ActiveWorkBook()->Clear();
    CPPUNIT_ASSERT(m_Api->ReadJson(wJson));
    m_Api->ActiveWorkBook()->RecalculateAll();

#ifdef checkfo
    m_Api->CheckFormat();
#endif

    wNeighbor = m_Api->CellFormat("H5");
    CPPUNIT_ASSERT(wNeighbor.find("background-color:#9FC5E8") != tString::npos);
    CPPUNIT_ASSERT(m_Api->CellFormat("H4").find("border-bottom") != tString::npos);
}

static tBool CellFormatContains(const tString& sCss, const tString& sNeedle) {
    return sCss.find(sNeedle) != tString::npos;
}

void TestSkCopyPaste::TestReadJsonEmptyClearsFormatPool() {
    m_Api->UndoCellBorder("B2", tBorderAll, "solid 1px black;");
    CPPUNIT_ASSERT(m_Api->CellFormat("B2").find("border-left") != tString::npos);

    const tString wEmptyJson =
        "{\"uri\":\"wwww.skeema.fr/w1\",\"sizecol\":10,\"sizerow\":5,"
        "\"sheets\":[{\"name\":\"Sheet1\",\"rows\":[],\"cols\":[],\"cells\":[],\"merged\":[]}],"
        "\"namedranges\":[],\"f\":{\"formats\":[]}}";

    CPPUNIT_ASSERT(m_Api->ReadJson(wEmptyJson));
    CPPUNIT_ASSERT(m_Api->Cell("B2") == nullptr);

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkCopyPaste::TestUndoMoveAdjacentBorderedCellsE6ToE7() {
    // User scenario: two stacked bordered cells, drag E6 onto E7.
    m_Api->UndoCellBorder("E6", tBorderAll, "solid 1px black;");
    m_Api->UndoCellBorder("E7", tBorderAll, "solid 1px black;");

    CPPUNIT_ASSERT(m_Api->UndoMove("E6", "E7"));

    CPPUNIT_ASSERT_MESSAGE("after move E6 cleared",
        m_Api->Cell("E6") == nullptr || m_Api->CellFormat("E6").find("border") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("after move E7 border-left",
        m_Api->CellFormat("E7").find("border-left") != tString::npos);

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkCopyPaste::TestUndoMoveTableOntoOrangeBlock() {
    // User scenario: table C5:F11 moved onto orange block H3:L14 -> F5:I11 (partial overlap).
    m_Api->UndoCellFormat("H3:L14", "background-color:#F6B26B;");
    m_Api->UndoCellValue("C5:F11", tVariant(1));
    m_Api->UndoCellFormat("C5:F5", "background-color:#9FC5E8;");
    m_Api->UndoCellFormat("C6:F11", "background-color:#FFF2CC;");
    m_Api->UndoCellBorder("C5:F11", tBorderAll, "solid 1px black;");

    CPPUNIT_ASSERT(m_Api->UndoMove("C5:F11", "F5:I11"));

    // Dest block must show table fill, not orange.
    CPPUNIT_ASSERT_MESSAGE("after move H6 no orange",
        !CellFormatContains(m_Api->CellFormat("H6"), "F6B26B"));
    CPPUNIT_ASSERT_MESSAGE("after move I8 no orange",
        !CellFormatContains(m_Api->CellFormat("I8"), "F6B26B"));
    CPPUNIT_ASSERT_MESSAGE("after move H6 table fill",
        CellFormatContains(m_Api->CellFormat("H6"), "FFF2CC") ||
        CellFormatContains(m_Api->CellFormat("H6"), "9FC5E8"));
    // Source block cleared.
    CPPUNIT_ASSERT_MESSAGE("after move C5 no table fill ghost",
        !CellFormatContains(m_Api->CellFormat("C5"), "9FC5E8"));
    // Orange outside dest footprint must keep its fill (may gain a border seam).
    CPPUNIT_ASSERT_MESSAGE("after move J6 orange preserved",
        m_Api->Cell("J6") == nullptr ||
        CellFormatContains(m_Api->CellFormat("J6"), "F6B26B"));

    CPPUNIT_ASSERT(m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE("after undo table back C5",
        CellFormatContains(m_Api->CellFormat("C5"), "9FC5E8"));
    CPPUNIT_ASSERT_MESSAGE("after undo orange H3",
        CellFormatContains(m_Api->CellFormat("H3"), "F6B26B"));
    // G5 was empty before move (dest-only); I5 received source F5 paste.
    CPPUNIT_ASSERT_MESSAGE("after undo dest G5 cleared",
        m_Api->Cell("G5") == nullptr ||
        (!CellFormatContains(m_Api->CellFormat("G5"), "9FC5E8") &&
         !CellFormatContains(m_Api->CellFormat("G5"), "FFF2CC")));
    CPPUNIT_ASSERT_MESSAGE("after undo dest I5 cleared",
        m_Api->Cell("I5") == nullptr ||
        (!CellFormatContains(m_Api->CellFormat("I5"), "9FC5E8") &&
         !CellFormatContains(m_Api->CellFormat("I5"), "FFF2CC")));
    CPPUNIT_ASSERT_MESSAGE("after undo overlap H6 orange restored",
        CellFormatContains(m_Api->CellFormat("H6"), "F6B26B"));
    CPPUNIT_ASSERT_MESSAGE("after undo orange J6 preserved",
        m_Api->Cell("J6") == nullptr ||
        CellFormatContains(m_Api->CellFormat("J6"), "F6B26B"));
    CPPUNIT_ASSERT_MESSAGE("after undo no table ghost K9",
        !CellFormatContains(m_Api->CellFormat("K9"), "FFF2CC"));

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkCopyPaste::TestUndoMoveTableValuesOntoOrangeBlock() {
    // User scenario: table C5:F11 with values moved onto orange H3:L14 -> G5:J11.
    // Undo must restore orange values (including K5:K11), not only background fill.
    m_Api->UndoCellFormat("H3:L14", "background-color:#F6B26B;");
    for (tIndex wRow = 3; wRow <= 14; wRow++) {
        for (const tChar* wCol : { "H", "I", "J", "K", "L" }) {
            tStringStream wRef;
            wRef << wCol << wRow;
            m_Api->UndoCellValue(wRef.str(), tVariant(static_cast<tInt>(wRow - 2)));
        }
    }
    m_Api->UndoCellValue("C5:F11", tVariant(1));
    m_Api->UndoCellFormat("C5:F5", "background-color:#9FC5E8;");
    m_Api->UndoCellFormat("C6:F11", "background-color:#FFF2CC;");
    m_Api->UndoCellBorder("C5:F11", tBorderAll, "solid 1px black;");
    m_Api->UndoCellValue("C6", tVariant("Stéphane"));
    m_Api->UndoCellValue("D5", tVariant("2026"));
    m_Api->UndoCellValue("E5", tVariant("2025"));
    m_Api->UndoCellValue("F5", tVariant("2024"));

    CPPUNIT_ASSERT(CellIntValueEq(m_Api->Cell("K6"), 4));
    CPPUNIT_ASSERT(CellFormatContains(m_Api->CellFormat("K6"), "F6B26B"));

    CPPUNIT_ASSERT(m_Api->UndoMove("C5:F11", "G5:J11"));

    CPPUNIT_ASSERT(CellIntValueEq(m_Api->Cell("H6"), 4) == false);
    CPPUNIT_ASSERT(CellFormatContains(m_Api->CellFormat("G6"), "9FC5E8") ||
        CellFormatContains(m_Api->CellFormat("G6"), "FFF2CC"));

    CPPUNIT_ASSERT(m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE("after undo table name C6",
        m_Api->Cell("C6") != nullptr &&
        m_Api->Cell("C6")->Value().Str() == tString("Stéphane"));
    CPPUNIT_ASSERT_MESSAGE("after undo overlap H6 orange value",
        CellIntValueEq(m_Api->Cell("H6"), 4));
    CPPUNIT_ASSERT_MESSAGE("after undo overlap H6 orange fill",
        CellFormatContains(m_Api->CellFormat("H6"), "F6B26B"));
    CPPUNIT_ASSERT_MESSAGE("after undo dest neighbor K6 orange value",
        CellIntValueEq(m_Api->Cell("K6"), 4));
    CPPUNIT_ASSERT_MESSAGE("after undo dest neighbor K6 orange fill",
        CellFormatContains(m_Api->CellFormat("K6"), "F6B26B"));
    for (tIndex wRow = 5; wRow <= 11; wRow++) {
        tStringStream wRef;
        wRef << "K" << wRow;
        CPPUNIT_ASSERT_MESSAGE(
            (tString("after undo K col row ") + std::to_string(wRow)).c_str(),
            CellIntValueEq(m_Api->Cell(wRef.str()), static_cast<tInt>(wRow - 2)));
    }

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkCopyPaste::TestBorderOwnerModelMoveUndo() {
    // Phase 1 owner-model: borders live on the cell; move+undo must not need seam neighbors.
    m_Api->UndoCellValue("C5:E7", tVariant(1));
    m_Api->UndoCellBorder("C5:E7", tBorderAll, "solid 1px black;");

    auto wHasBorder = [&](const tString& sRef, const tString& sSide) {
        return m_Api->CellFormat(sRef).find(sSide) != tString::npos;
    };
    CPPUNIT_ASSERT(wHasBorder("C5", "border-top"));
    CPPUNIT_ASSERT(wHasBorder("E5", "border-right"));
    CPPUNIT_ASSERT(wHasBorder("C7", "border-bottom"));
    CPPUNIT_ASSERT(wHasBorder("E7", "border-right"));
    CPPUNIT_ASSERT(!wHasBorder("F5", "border-left"));

    CPPUNIT_ASSERT(m_Api->UndoMove("C5:E7", "G8:I10"));

    CPPUNIT_ASSERT(wHasBorder("G8", "border-top"));
    CPPUNIT_ASSERT(wHasBorder("I8", "border-right"));
    CPPUNIT_ASSERT(!wHasBorder("C5", "border-top"));

    CPPUNIT_ASSERT(m_Api->Undo());

    CPPUNIT_ASSERT(wHasBorder("C5", "border-top"));
    CPPUNIT_ASSERT(wHasBorder("E5", "border-right"));
    CPPUNIT_ASSERT(!wHasBorder("G8", "border-top"));

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkCopyPaste::TestCopyBorderRangePasteUndo() {
    m_Api->UndoCellValue("D4:E6", tVariant(1));
    m_Api->UndoCellFormat("D4:E6", "background-color:#9FC5E8;");
    m_Api->UndoCellBorder("D4:E6", tBorderAll, "solid 1px black;");

    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderRangePasteUndo source D4",
        m_Api->CellFormat("D4").find("border-left") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderRangePasteUndo source E4",
        m_Api->CellFormat("E4").find("border-left") != tString::npos);

    m_Api->Copy("D4:E6");
    CPPUNIT_ASSERT(m_Api->UndoPaste("G8"));

    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderRangePasteUndo paste G8 left",
        m_Api->CellFormat("G8").find("border-left") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderRangePasteUndo paste H8 left",
        m_Api->CellFormat("H8").find("border-left") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderRangePasteUndo outside H8 right",
        m_Api->CellFormat("H8").find("border-right") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderRangePasteUndo outside G10 bottom",
        m_Api->CellFormat("G10").find("border-bottom") != tString::npos);

    CPPUNIT_ASSERT(m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderRangePasteUndo undo source D4",
        m_Api->CellFormat("D4").find("border-left") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderRangePasteUndo undo source E6 bottom",
        m_Api->CellFormat("E6").find("border-bottom") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderRangePasteUndo undo paste G8 cleared",
        m_Api->Cell("G8") == nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderRangePasteUndo undo outside H8 cleared",
        m_Api->Cell("H8") == nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderRangePasteUndo undo outside G10 cleared",
        m_Api->Cell("G10") == nullptr);
}

void TestSkCopyPaste::TestCopyBorderSingleCellPasteUndo() {
    // C12: empty destination with blue border (user scenario: paste then undo must restore it).
    m_Api->UndoCellBorder("C12", tBorderAll, "solid 2px blue;");
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderSingleCellPasteUndo C12 blue before",
        m_Api->CellFormat("C12").find("blue") != tString::npos);

    m_Api->UndoCellValue("C7", tVariant("fdfdfddfdfdf"));
    m_Api->UndoCellBorder("C7", tBorderAll, "solid 1px black;");

    m_Api->Copy("C7");
    CPPUNIT_ASSERT(m_Api->UndoPaste("C12"));

    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderSingleCellPasteUndo C12 black after paste",
        m_Api->CellFormat("C12").find("black") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderSingleCellPasteUndo C12 right after paste",
        m_Api->CellFormat("C12").find("border-right") != tString::npos);

    CPPUNIT_ASSERT(m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderSingleCellPasteUndo C12 blue after undo",
        m_Api->CellFormat("C12").find("blue") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderSingleCellPasteUndo C12 no black after undo",
        m_Api->CellFormat("C12").find("black") == tString::npos);

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkCopyPaste::TestCopyBorderGridPastePreservesOuter() {
    // Empty bordered grid (user scenario): paste data inside must not destroy outer seams.
    m_Api->UndoCellBorder("C4:I17", tBorderAll, "solid 1px black;");
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderGridPastePreservesOuter I8 before paste",
        m_Api->CellFormat("I8").find("border-right") != tString::npos);

    m_Api->UndoCellValue("M4:Q13", tVariant(1));
    m_Api->UndoCellFormat("M4:Q13", "background-color:#9FC5E8;");
    m_Api->UndoCellFormat("N5:Q13", "background-color:#FFF2CC;");
    m_Api->UndoCellBorder("M4:Q13", tBorderAll, "solid 1px black;");
    m_Api->UndoCellValue("N5", tVariant("2026"));
    m_Api->UndoCellValue("O5", tVariant("2025"));
    m_Api->UndoCellValue("P5", tVariant("2024"));
    m_Api->UndoCellValue("M6", tVariant("Stéphane"));
    m_Api->UndoCellValue("N6", tVariant("10 €"));

    m_Api->Copy("M4:Q13");
    CPPUNIT_ASSERT(m_Api->UndoPaste("D4:H13"));

    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderGridPastePreservesOuter I8 outer right",
        m_Api->CellFormat("I8").find("border-right") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderGridPastePreservesOuter I13 outer right",
        m_Api->CellFormat("I13").find("border-right") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderGridPastePreservesOuter C8 left column",
        m_Api->CellFormat("C8").find("border-left") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestCopyBorderGridPastePreservesOuter H13 bottom",
        m_Api->CellFormat("H13").find("border-bottom") != tString::npos);

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkCopyPaste::TestCopyGridPastePreservesOuterFillAfterLocalDo() {
    // Green bordered block with inner paste (local PassCss path): col+1/+2 and bottom-right
    // neighbors must keep fill; undo must still restore.
    m_Api->UndoCellFormat("B4:J15", "background-color:#93C47D;");
    m_Api->UndoCellBorder("B4:J15", tBorderAll, "solid 2px black;");

    CPPUNIT_ASSERT_MESSAGE("before paste I7 green",
        CellFormatContains(m_Api->CellFormat("I7"), "93C47D"));
    CPPUNIT_ASSERT_MESSAGE("before paste J15 green",
        CellFormatContains(m_Api->CellFormat("J15"), "93C47D"));

    m_Api->UndoCellValue("M7:Q13", tVariant(1));
    m_Api->UndoCellFormat("M7:Q13", "background-color:#9FC5E8;");
    m_Api->UndoCellFormat("N8:Q13", "background-color:#FFF2CC;");
    m_Api->UndoCellBorder("M7:Q13", tBorderAll, "solid 1px black;");
    m_Api->UndoCellValue("N7", tVariant("2026"));
    m_Api->UndoCellValue("O7", tVariant("2025"));
    m_Api->UndoCellValue("P7", tVariant("2024"));
    m_Api->UndoCellValue("M8", tVariant("Stéphane"));
    m_Api->UndoCellValue("N8", tVariant("10 €"));

    m_Api->Copy("M7:Q13");
    CPPUNIT_ASSERT(m_Api->UndoPaste("D7:G13"));

    CPPUNIT_ASSERT_MESSAGE("after paste I7 green",
        CellFormatContains(m_Api->CellFormat("I7"), "93C47D"));
    CPPUNIT_ASSERT_MESSAGE("after paste I14 green",
        CellFormatContains(m_Api->CellFormat("I14"), "93C47D"));
    CPPUNIT_ASSERT_MESSAGE("after paste J15 green",
        CellFormatContains(m_Api->CellFormat("J15"), "93C47D"));
    CPPUNIT_ASSERT_MESSAGE("after paste J15 border",
        m_Api->CellFormat("J15").find("border") != tString::npos);

    CPPUNIT_ASSERT(m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE("after undo I7 green",
        CellFormatContains(m_Api->CellFormat("I7"), "93C47D"));
    CPPUNIT_ASSERT_MESSAGE("after undo J15 green",
        CellFormatContains(m_Api->CellFormat("J15"), "93C47D"));

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkCopyPaste::TestUndoMoveBorderGhostSeamsAfterMove() {
    // User scenario: table B3:F9 moved down-right — no ghost seams at old G column / row 10.
    m_Api->UndoCellValue("B3:F9", tVariant(1));
    m_Api->UndoCellFormat("B3:C9", "background-color:#9FC5E8;");
    m_Api->UndoCellFormat("D3:F3", "background-color:#6FA8DC;");
    m_Api->UndoCellFormat("D4:F9", "background-color:#FFF2CC;");
    m_Api->UndoCellBorder("B3:F9", tBorderAll, "solid 2px black;");

    CPPUNIT_ASSERT_MESSAGE("before move F3 outer border-right",
        m_Api->CellFormat("F3").find("border-right") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("before move B9 outer border-bottom",
        m_Api->CellFormat("B9").find("border-bottom") != tString::npos);

    CPPUNIT_ASSERT(m_Api->UndoMove("B3:F9", "G6:K12"));

    CPPUNIT_ASSERT_MESSAGE("after move G3 no ghost border-left",
        m_Api->CellFormat("G3").find("border-left") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("after move G5 no ghost border-left",
        m_Api->CellFormat("G5").find("border-left") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("after move B10 no ghost border-top",
        m_Api->CellFormat("B10").find("border-top") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("after move F10 no ghost border-top",
        m_Api->CellFormat("F10").find("border-top") == tString::npos);

    CPPUNIT_ASSERT(m_Api->Undo());

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkCopyPaste::TestUndoMoveUpLeftClearsSourceFillGhosts() {
    // User scenario: colored table moved up-left — no fill ghosts on source bottom/right bands.
    m_Api->UndoCellValue("I11:L17", tVariant(1));
    m_Api->UndoCellFormat("I11:J17", "background-color:#9FC5E8;");
    m_Api->UndoCellFormat("K11:K11", "background-color:#6FA8DC;");
    m_Api->UndoCellFormat("K12:L17", "background-color:#FFF2CC;");
    m_Api->UndoCellBorder("I11:L17", tBorderAll, "solid 1px black;");

    CPPUNIT_ASSERT_MESSAGE("before move J11 border-left (I|J seam)",
        m_Api->CellFormat("J11").find("border-left") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("before move L11 border-right (outer, or on M11)",
        m_Api->CellFormat("L11").find("border-right") != tString::npos ||
        m_Api->CellFormat("M11").find("border-left") != tString::npos);

    CPPUNIT_ASSERT(m_Api->UndoMove("I11:L17", "E11:H17"));

    CPPUNIT_ASSERT_MESSAGE("after move source top band I10 no border",
        m_Api->CellFormat("I10").find("border") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("after move source top band L10 no border",
        m_Api->CellFormat("L10").find("border") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("after move source I11 no fill ghost",
        !CellFormatContains(m_Api->CellFormat("I11"), "9FC5E8") &&
        !CellFormatContains(m_Api->CellFormat("I11"), "FFF2CC"));
    CPPUNIT_ASSERT_MESSAGE("after move bottom band I18 no yellow fill",
        !CellFormatContains(m_Api->CellFormat("I18"), "FFF2CC"));
    CPPUNIT_ASSERT_MESSAGE("after move bottom band L18 no yellow fill",
        !CellFormatContains(m_Api->CellFormat("L18"), "FFF2CC"));
    CPPUNIT_ASSERT_MESSAGE("after move right band M14 no yellow fill",
        !CellFormatContains(m_Api->CellFormat("M14"), "FFF2CC"));
    CPPUNIT_ASSERT_MESSAGE("after move dest E11 has fill",
        CellFormatContains(m_Api->CellFormat("E11"), "9FC5E8"));
    CPPUNIT_ASSERT_MESSAGE("after move F11 border-left (E|F seam)",
        m_Api->CellFormat("F11").find("border-left") != tString::npos);
    for (tIndex wRow = 12; wRow <= 17; wRow++) {
        tStringStream wStream;
        wStream << "F" << wRow;
        CPPUNIT_ASSERT_MESSAGE(
            (tString("after move ") + wStream.str() + " border-left").c_str(),
            m_Api->CellFormat(wStream.str()).find("border-left") != tString::npos);
    }

    CPPUNIT_ASSERT(m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE("after undo dest top seam E10 cleared",
        m_Api->CellFormat("E10").find("border") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("after undo dest left seam D11 cleared",
        m_Api->CellFormat("D11").find("border") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("after undo source I11 fill restored",
        CellFormatContains(m_Api->CellFormat("I11"), "9FC5E8"));

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkCopyPaste::TestUndoMoveUpLeftPreservesOuterBorders() {
    // User scenario: J7:M13 -> I6:L12 — outer top/right/bottom borders must survive ghost cleanup.
    m_Api->UndoCellValue("J7:M13", tVariant(1));
    m_Api->UndoCellFormat("J7:K13", "background-color:#9FC5E8;");
    m_Api->UndoCellFormat("L7:L7", "background-color:#6FA8DC;");
    m_Api->UndoCellFormat("L8:M13", "background-color:#FFF2CC;");
    m_Api->UndoCellBorder("J7:M13", tBorderAll, "solid 1px black;");

    CPPUNIT_ASSERT(m_Api->UndoMove("J7:M13", "I6:L12"));

    auto wHasBorder = [&](const tString& sRef, const tString& sSide) {
        return m_Api->CellFormat(sRef).find(sSide) != tString::npos;
    };
    CPPUNIT_ASSERT_MESSAGE("after move top seam I6 or H6",
        wHasBorder("I6", "border-top") || wHasBorder("H6", "border-bottom"));
    CPPUNIT_ASSERT_MESSAGE("after move right seam L6 or M6",
        wHasBorder("L6", "border-right") || wHasBorder("M6", "border-left"));
    CPPUNIT_ASSERT_MESSAGE("after move bottom seam I12 or I13",
        wHasBorder("I12", "border-bottom") || wHasBorder("I13", "border-top"));
    CPPUNIT_ASSERT_MESSAGE("after move internal F6 border-left",
        wHasBorder("J6", "border-left"));
    CPPUNIT_ASSERT_MESSAGE("after move source M7 no yellow fill",
        !CellFormatContains(m_Api->CellFormat("M7"), "FFF2CC"));

    CPPUNIT_ASSERT(m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE("after undo dest top seam row 5 cleared",
        m_Api->CellFormat("I5").find("border") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("after undo dest left seam H6 cleared",
        m_Api->CellFormat("H6").find("border") == tString::npos);
    CPPUNIT_ASSERT_MESSAGE("after undo table back J7",
        CellFormatContains(m_Api->CellFormat("J7"), "9FC5E8"));

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkCopyPaste::TestUndoMoveOverlapUpLeftPreservesOuterBorders() {
    // User scenario: H8:K14 -> F6:I12 with source/dest overlap — outer borders must remain.
    m_Api->UndoCellValue("H8:K14", tVariant(1));
    m_Api->UndoCellFormat("H8:I14", "background-color:#9FC5E8;");
    m_Api->UndoCellFormat("J8:J8", "background-color:#6FA8DC;");
    m_Api->UndoCellFormat("J9:K14", "background-color:#FFF2CC;");
    m_Api->UndoCellBorder("H8:K14", tBorderAll, "solid 1px black;");

    CPPUNIT_ASSERT(m_Api->UndoMove("H8:K14", "F6:I12"));

    auto wHasBorder = [&](const tString& sRef, const tString& sSide) {
        return m_Api->CellFormat(sRef).find(sSide) != tString::npos;
    };
    for (const tChar* wCol : { "F", "G", "H", "I" }) {
        tStringStream wTop;
        wTop << wCol << "6";
        CPPUNIT_ASSERT_MESSAGE(
            (tString("after overlap move ") + wTop.str() + " border-top").c_str(),
            wHasBorder(wTop.str(), "border-top") ||
            wHasBorder(tString(wCol) + "5", "border-bottom"));
    }
    for (tIndex wRow = 6; wRow <= 12; wRow++) {
        tStringStream wRight;
        wRight << "I" << wRow;
        tStringStream wNeighbor;
        wNeighbor << "J" << wRow;
        CPPUNIT_ASSERT_MESSAGE(
            (tString("after overlap move ") + wRight.str() + " border-right").c_str(),
            wHasBorder(wRight.str(), "border-right") ||
            wHasBorder(wNeighbor.str(), "border-left"));
    }
    CPPUNIT_ASSERT_MESSAGE("after overlap move F6 border-left",
        wHasBorder("F6", "border-left") || wHasBorder("E6", "border-right"));
    CPPUNIT_ASSERT_MESSAGE("after overlap move F12 border-bottom",
        wHasBorder("F12", "border-bottom") || wHasBorder("F13", "border-top"));
    CPPUNIT_ASSERT_MESSAGE("after overlap move J9 no yellow fill ghost",
        !CellFormatContains(m_Api->CellFormat("J9"), "FFF2CC"));
    CPPUNIT_ASSERT_MESSAGE("after overlap move J10 no yellow fill ghost",
        !CellFormatContains(m_Api->CellFormat("J10"), "FFF2CC"));

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkCopyPaste::TestUndoMove() {
    m_Api->UndoCellValue("D4:E5", tVariant(1));
    m_Api->UndoCellBorder("D4:E5", tBorderAll, "solid 1px black;");

    CPPUNIT_ASSERT(m_Api->UndoMove("D4:E5", "G8:H9"));

    CPPUNIT_ASSERT_MESSAGE("TestUndoMove source D4 cleared",
        m_Api->Cell("D4") == nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMove dest G8",
        m_Api->Cell("G8") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMove dest G8 border",
        m_Api->CellFormat("G8").find("border-left") != tString::npos);

    CPPUNIT_ASSERT(m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE("TestUndoMove undo source D4 restored",
        m_Api->Cell("D4") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMove undo dest G8 cleared",
        m_Api->Cell("G8") == nullptr);
}

void TestSkCopyPaste::TestUndoCut() {
    m_Api->UndoCellValue("D4:E5", tVariant(1));
    m_Api->UndoCellFormat("D4:E5", "background-color:#9FC5E8;");
    m_Api->UndoCellBorder("D4:E5", tBorderAll, "solid 1px black;");

    CPPUNIT_ASSERT(m_Api->UndoCut("D4:E5"));
    CPPUNIT_ASSERT(m_Api->Cell("D4") == nullptr);
    CPPUNIT_ASSERT(CopyJsonHasCellPayload(tApplication::Instance()->Clipboard()->Text()));

    CPPUNIT_ASSERT(m_Api->UndoPaste("G8:H9"));
    CPPUNIT_ASSERT(m_Api->Cell("G8") != nullptr);

    tUndoSpreadSheet* wUndo = dynamic_cast<tUndoSpreadSheet*>(m_Api->LastUndo());
    CPPUNIT_ASSERT(wUndo != nullptr);
    CPPUNIT_ASSERT(wUndo->ClassName() == "tUndoMove");

    CPPUNIT_ASSERT(m_Api->Undo());
    CPPUNIT_ASSERT(m_Api->Cell("G8") == nullptr);
    CPPUNIT_ASSERT(m_Api->Cell("D4") != nullptr);
}

void TestSkCopyPaste::TestUndoCutCollaborativeUndoJson() {
    m_Api->UndoCellValue("D4:E5", tVariant(77));
    m_Api->UndoCellFormat("D4:E5", "background-color:#9FC5E8;");
    CPPUNIT_ASSERT(m_Api->UndoCut("D4:E5"));
    CPPUNIT_ASSERT(m_Api->UndoPaste("G8:H9"));
    CPPUNIT_ASSERT(m_Api->Cell("G8") != nullptr);
    CPPUNIT_ASSERT(m_Api->Cell("D4") == nullptr);

    tUndoSpreadSheet* wUndo = dynamic_cast<tUndoSpreadSheet*>(m_Api->LastUndo());
    CPPUNIT_ASSERT(wUndo != nullptr);
    CPPUNIT_ASSERT(wUndo->ClassName() == "tUndoMove");

    // Simulate remote client: Undo message is rebased + JSON, then applied via ReadJson/Undo.
    wUndo->IsUndo(true);
    CPPUNIT_ASSERT_MESSAGE("TestUndoCutCollaborativeUndoJson rebase", wUndo->Rebase());
    wUndo->IsJson(true);
#ifdef TestMultiUser
    tMessage wMessage("", "wwww.skeema.fr/w1", "Undo");
#else
    tMessage wMessage("", "", "", "wwww.skeema.fr/w1", "Undo");
#endif
    wMessage.WriteJson(wUndo);
    wUndo->IsJson(false);
    tUndoSpreadSheet* wRemoteUndo = wMessage.ReadJson(wMessage.Json());
    CPPUNIT_ASSERT_MESSAGE("TestUndoCutCollaborativeUndoJson remote undo parse", wRemoteUndo != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestUndoCutCollaborativeUndoJson remote undo apply", wRemoteUndo->Undo());
    delete(wRemoteUndo);

    CPPUNIT_ASSERT_MESSAGE("TestUndoCutCollaborativeUndoJson undo dest G8 cleared",
        m_Api->Cell("G8") == nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestUndoCutCollaborativeUndoJson undo source D4 restored",
        m_Api->Cell("D4") != nullptr);
    CPPUNIT_ASSERT(CellIntValueEq(m_Api->Cell("D4"), 77));
}

void TestSkCopyPaste::TestUndoMoveEmptySourcePreservesDestFormat() {
    // Empty source (K2, outside Fill grid) moved onto formatted J6 must not strip dest fill;
    // undo must leave the destination formatted (user drag-move scenario).
    m_Api->UndoCellFormat("H3:J10", "background-color:#F6B26B;");
    CPPUNIT_ASSERT(m_Api->Cell("K2") == nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveEmptySourcePreservesDestFormat J6 fill before",
        CellFormatContains(m_Api->CellFormat("J6"), "F6B26B"));

    CPPUNIT_ASSERT(m_Api->UndoMove("K2", "J6"));

    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveEmptySourcePreservesDestFormat J6 fill after move",
        CellFormatContains(m_Api->CellFormat("J6"), "F6B26B"));

    CPPUNIT_ASSERT(m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveEmptySourcePreservesDestFormat J6 fill after undo",
        CellFormatContains(m_Api->CellFormat("J6"), "F6B26B"));
}

void TestSkCopyPaste::TestUndoMoveBorderShiftLeft() {
    m_Api->UndoCellValue("B9:E15", tVariant(1));
    m_Api->UndoCellFormat("B9:E9", "background-color:#9FC5E8;");
    m_Api->UndoCellFormat("B10:B15", "background-color:#9FC5E8;");
    m_Api->UndoCellBorder("B9:E15", tBorderAll, "solid 1px black;");

    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveBorderShiftLeft source E10 internal left",
        m_Api->CellFormat("E10").find("border-left") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveBorderShiftLeft source E15 bottom",
        m_Api->CellFormat("E15").find("border-bottom") != tString::npos);

    CPPUNIT_ASSERT(m_Api->UndoMove("B9:E15", "A9:D15"));

    // Simulate remote client: Undo message is rebased + JSON, then applied via ReadJson/Undo.
    tUndoSpreadSheet* wUndo = dynamic_cast<tUndoSpreadSheet*>(m_Api->LastUndo());
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveBorderShiftLeft last undo", wUndo != nullptr);
    wUndo->IsUndo(true);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveBorderShiftLeft rebase", wUndo->Rebase());
    wUndo->IsJson(true);
#ifdef TestMultiUser
    tMessage wMessage("", "wwww.skeema.fr/w1", "Undo");
#else
    tMessage wMessage("", "", "", "wwww.skeema.fr/w1", "Undo");
#endif
    wMessage.WriteJson(wUndo);
    wUndo->IsJson(false);
    tUndoSpreadSheet* wRemoteUndo = wMessage.ReadJson(wMessage.Json());
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveBorderShiftLeft remote undo parse", wRemoteUndo != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveBorderShiftLeft remote undo apply", wRemoteUndo->Undo());
    delete(wRemoteUndo);

    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveBorderShiftLeft undo E10 internal left",
        m_Api->CellFormat("E10").find("border-left") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveBorderShiftLeft undo E15 internal left",
        m_Api->CellFormat("E15").find("border-left") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveBorderShiftLeft undo E15 bottom",
        m_Api->CellFormat("E15").find("border-bottom") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveBorderShiftLeft undo E15 outer right",
        m_Api->CellFormat("E15").find("border-right") != tString::npos);

#ifdef checkfo
    m_Api->CheckFormat();
#endif
}

void TestSkCopyPaste::TestUndoMoveShiftDown() {
    m_Api->UndoCellValue("A10", tVariant(10));
    m_Api->UndoCellValue("A11", tVariant(11));
    m_Api->UndoCellValue("A12", tVariant(12));

    CPPUNIT_ASSERT(m_Api->UndoMove("A10:A12", "A11:A13"));

    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveShiftDown A10 cleared",
        m_Api->Cell("A10") == nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveShiftDown A11",
        m_Api->Cell("A11") != nullptr && m_Api->Cell("A11")->Value().Int() == 10);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveShiftDown A12",
        m_Api->Cell("A12") != nullptr && m_Api->Cell("A12")->Value().Int() == 11);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveShiftDown A13",
        m_Api->Cell("A13") != nullptr && m_Api->Cell("A13")->Value().Int() == 12);

    CPPUNIT_ASSERT(m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveShiftDown undo A10",
        m_Api->Cell("A10") != nullptr && m_Api->Cell("A10")->Value().Int() == 10);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveShiftDown undo A11",
        m_Api->Cell("A11") != nullptr && m_Api->Cell("A11")->Value().Int() == 11);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveShiftDown undo A12",
        m_Api->Cell("A12") != nullptr && m_Api->Cell("A12")->Value().Int() == 12);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveShiftDown undo A13 cleared",
        m_Api->Cell("A13") == nullptr);
}

void TestSkCopyPaste::TestUndoMoveFormulaRefs() {
    m_Api->UndoCellValue("A10", tVariant(10));
    m_Api->UndoCellValue("A11", tVariant(11));
    m_Api->UndoCellValue("A12", tVariant(12));
    m_Api->UndoCellValue("C10", tVariant("=SUM(A10:A12)"));

    CPPUNIT_ASSERT(m_Api->UndoMove("A10:A12", "A11:A13"));

    tCell* wCell = m_Api->Cell("C10");
    CPPUNIT_ASSERT(wCell != nullptr);
    const tString wFormulaAfterMove = wCell->FormulaStr();
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveFormulaRefs move updates range start",
        wFormulaAfterMove.find("A11") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveFormulaRefs move updates range end",
        wFormulaAfterMove.find("A13") != tString::npos);

    CPPUNIT_ASSERT(m_Api->Undo());

    wCell = m_Api->Cell("C10");
    CPPUNIT_ASSERT(wCell != nullptr);
    const tString wFormulaAfterUndo = wCell->FormulaStr();
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveFormulaRefs undo restores range start",
        wFormulaAfterUndo.find("A10") != tString::npos);
    CPPUNIT_ASSERT_MESSAGE("TestUndoMoveFormulaRefs undo restores range end",
        wFormulaAfterUndo.find("A12") != tString::npos);
}

void TestSkCopyPaste::TestUndoMoveRebaseAfterInsertRowByRect() {
    m_Api->UndoCellValue("B9", tVariant(123));
    CPPUNIT_ASSERT(m_Api->UndoMove("B9:E15", "B19:E25"));
    CPPUNIT_ASSERT_MESSAGE("marker at B19 after move", CellIntValueEq(m_Api->Cell("B19"), 123));

    tSheet* wSheet = m_Api->ActiveSheet();
    tSaveSelectErase wSave;
    wSheet->DoInsertRowByRect(&wSave, tRect(2, 1, 3, 10));
    wSheet->WorkBook()->UndoRebaseLog().RegisterInsertRect(
        wSheet->AllocatorRef(), tRect(2, 1, 3, 10), 0, 0, true);

    CPPUNIT_ASSERT_MESSAGE("marker shifted to B21 after insert", CellIntValueEq(m_Api->Cell("B21"), 123));

    tUndoSpreadSheet* wUndo = dynamic_cast<tUndoSpreadSheet*>(m_Api->LastUndo());
    CPPUNIT_ASSERT(wUndo != nullptr);
    CPPUNIT_ASSERT(wUndo->Rebase());
    CPPUNIT_ASSERT_MESSAGE("rebased dest ref", wUndo->RefRebase() == "B21:E27");

    wUndo->IsUndo(true);
    wUndo->IsJson(true);
#ifdef TestMultiUser
    tMessage wMsg("", "wwww.skeema.fr/w1", "Undo");
#else
    tMessage wMsg("", "", "", "wwww.skeema.fr/w1", "Undo");
#endif
    wMsg.WriteJson(wUndo);
    wUndo->IsJson(false);

    CPPUNIT_ASSERT(m_Api->Undo());

    CPPUNIT_ASSERT_MESSAGE("marker restored at B11", CellIntValueEq(m_Api->Cell("B11"), 123));
    CPPUNIT_ASSERT_MESSAGE("dest must not still hold marker after undo",
        !CellIntValueEq(m_Api->Cell("B21"), 123));
}

#ifdef TestMultiUser
// WASM mono-user: Client() + MultiUserActive(false) must Undo() locally (not PopUndoToRedo only).
void TestSkCopyPaste::TestUndoPasteInterfaceWebMonoUser() {
    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    wInterface.UriWorkBook(kInterfaceWebTestWorkBook);
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);
    wInterface.NewWorkBook(kInterfaceWebTestWorkBook);

    wInterface.UndoCellValue("A1", tVariant(42));
    wInterface.Copy("A1");
    CPPUNIT_ASSERT(wInterface.UndoPaste("B1"));
    CPPUNIT_ASSERT(wInterface.Cell("B1") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("paste value at B1",
        CellIntValueEq(wInterface.Cell("B1"), 42));

    CPPUNIT_ASSERT(wInterface.Undo());
    CPPUNIT_ASSERT_MESSAGE("TestUndoPasteInterfaceWebMonoUser B1 cleared",
        wInterface.Cell("B1") == nullptr);

    wInterface.FormatApi(nullptr);
    delete(wFormatApi);
}

void TestSkCopyPaste::TestUndoPasteCollaborationPayloadTooLarge() {
    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(true);
    wInterface.User("u@test.fr", "Test", "User");
    wInterface.UriWorkBook(kInterfaceWebTestWorkBook);
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);
    wInterface.NewWorkBook(kInterfaceWebTestWorkBook);

    tString wPadding(kCollaborationPasteCopyMaxBytes + 1024, 'x');
    tStringStream wCopyJson;
    wCopyJson << "{\"select\":[{\"r\":\"A1:A1\"}],\"cells\":[{\"c\":\"A1\",\"v\":{\"t\":\"s\",\"v\":\""
              << wPadding << "\"}}]}";
    tApplication::Instance()->Clipboard()->Text(wCopyJson.str());

    CPPUNIT_ASSERT_MESSAGE("TestUndoPasteCollaborationPayloadTooLarge paste must be rejected",
        !wInterface.UndoPaste("B1"));
    CPPUNIT_ASSERT(wInterface.Cell("B1") == nullptr);
    CPPUNIT_ASSERT(wInterface.Error().find("collaboratif") != tString::npos);

    // Do not leave a huge clipboard for the next test (tUndoPaste reads it on construction).
    tApplication::Instance()->Clipboard()->Text("");
    wInterface.FormatApi(nullptr);
    delete(wFormatApi);
}

void TestSkCopyPaste::TestUndoMoveInterfaceWebMonoUser() {
    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    wInterface.UriWorkBook(kInterfaceWebTestWorkBook);
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);
    wInterface.NewWorkBook(kInterfaceWebTestWorkBook);

    tApplication::Instance()->Clipboard()->Text("");
    CPPUNIT_ASSERT(wInterface.UndoCellValue("D4", tVariant(99)));
    CPPUNIT_ASSERT_MESSAGE("D4 value before move",
        CellIntValueEq(wInterface.Cell("D4"), 99));
    CPPUNIT_ASSERT(wInterface.UndoMove("D4", "G8"));
    CPPUNIT_ASSERT(wInterface.Cell("D4") == nullptr);
    CPPUNIT_ASSERT(wInterface.Cell("G8") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("move value at G8",
        CellIntValueEq(wInterface.Cell("G8"), 99));

    CPPUNIT_ASSERT(wInterface.Undo());
    CPPUNIT_ASSERT(wInterface.Cell("G8") == nullptr);
    CPPUNIT_ASSERT(wInterface.Cell("D4") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("undo move restores value at D4",
        CellIntValueEq(wInterface.Cell("D4"), 99));

    wInterface.FormatApi(nullptr);
    delete(wFormatApi);
}

void TestSkCopyPaste::TestUndoCutInterfaceWebMonoUser() {
    tInterfaceWeb wInterface;
    wInterface.Client(true);
    wInterface.MultiUserActive(false);
    wInterface.User("u@test.fr", "Test", "User");
    wInterface.UriWorkBook(kInterfaceWebTestWorkBook);
    tFormatCssApi* wFormatApi = new tFormatCssApi();
    wInterface.FormatApi(wFormatApi);
    wInterface.NewWorkBook(kInterfaceWebTestWorkBook);

    CPPUNIT_ASSERT(wInterface.UndoCellValue("D4", tVariant(3)));
    wInterface.UndoCellFormat("D4", "background-color:#9FC5E8;");
    CPPUNIT_ASSERT(wInterface.UndoCut("D4"));
    CPPUNIT_ASSERT(wInterface.Cell("D4") == nullptr);
    CPPUNIT_ASSERT(wInterface.UndoPaste("G8"));
    CPPUNIT_ASSERT(wInterface.Cell("G8") != nullptr);

    tUndoSpreadSheet* wUndo = dynamic_cast<tUndoSpreadSheet*>(wInterface.LastUndo());
    CPPUNIT_ASSERT(wUndo != nullptr);
    CPPUNIT_ASSERT(wUndo->ClassName() == "tUndoMove");

    CPPUNIT_ASSERT(wInterface.Undo());
    CPPUNIT_ASSERT(wInterface.Cell("G8") == nullptr);
    CPPUNIT_ASSERT(wInterface.Cell("D4") != nullptr);
    CPPUNIT_ASSERT(CellIntValueEq(wInterface.Cell("D4"), 3));

    wInterface.FormatApi(nullptr);
    delete(wFormatApi);
}
#endif

void TestSkCopyPaste::setUp() {
	std::filesystem::remove_all("./Spreadsheet");

	m_Application = tApplication::Instance();
	m_Application->Locale("us");
	tFormatRoot::Instance();
	m_Api = new tApi;
	m_Api->IsUndoActif(true);
	m_Api->NewWorkBook("wwww.skeema.fr/w1");
	m_FormatApi = new tFormatCssApi();
	m_Api->FormatApi(m_FormatApi);
};

void TestSkCopyPaste::tearDown() {
	delete(m_Api);
	if (m_FormatApi != nullptr) {
		delete(m_FormatApi);
		m_FormatApi = nullptr;
	}
	DoneFormatRoot();
}

//==============================================================================
// TestSkRange
//==============================================================================

#include "../include/TestSkRange.hpp"
#include "../include/SkSpreadSheet.hpp"

// We can send it to the API of a feature 
TestSkRange::TestSkRange() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
};

void TestSkRange::DebugRange() {
	return; // Drop 
	cout << "Debug Range" << endl;
    tVectorRange* wVectorRange; // = m_Api->ActiveSheet()->ColRowCellRange()->VectorRange();
	for (auto wRange : *wVectorRange) {
		wRange->Debug();
	}
}


void  TestSkRange::DebugCell(tString sRef) {
    return; // Drop
    tCell* wCell=m_Api->Cell(sRef);
    cout <<  sRef;
    if (wCell!=nullptr) {
        if (wCell->FormulaStr()!="") cout << ":(" << wCell->FormulaStr() << ")";
        cout << "=" << wCell->Value();
    }
    cout << endl;
}


void TestSkRange::TestRange() {
	m_Api->EnsureRange(1, 1, 2, 2);
	DebugRange();
#ifdef checksp
	m_Api->Check();
#endif
	
	tRange* wRange = m_Api->FindRange(1, 1, 2, 2);
	CPPUNIT_ASSERT_MESSAGE("Ensure(1,1,2,2) A1:B2", wRange->StrRef()=="A1:B2");
	tColRow::tContainerRange::tResult* wVectorRange = m_Api->FindRangesCovered(1, 1);
	tAllocatorRef wRefRange = (*wVectorRange)[0];;
	tRange* wRangeResult = m_Api->ActiveSheet()->ColRowCellRange()->Range(wRefRange);

	
	CPPUNIT_ASSERT_MESSAGE("Find Recover 1,1 A1 ", wRange == wRangeResult);

	m_Api->EnsureRange(10, 10, 20, 20);
	wRange = m_Api->FindRange(10, 10, 20, 20);
	CPPUNIT_ASSERT_MESSAGE("Ensure(10,10,20,20) J10:T20", wRange->StrRef() == "J10:T20");

	wVectorRange = m_Api->FindRangesCovered(20, 20);
	wRefRange = (*wVectorRange)[0];;
	wRangeResult = m_Api->ActiveSheet()->ColRowCellRange()->Range(wRefRange);
	CPPUNIT_ASSERT_MESSAGE("Find Recover 1,1 T20 ", wRange == wRangeResult);
#ifdef checksp
	m_Api->Check();
#endif

	m_Api->DeleteRange(10, 10, 20, 20);
	wVectorRange = m_Api->FindRangesCovered(20, 20);
	CPPUNIT_ASSERT_MESSAGE("Find Recover 1,1 T20 ", wVectorRange->size()==0);
#ifdef checksp
	m_Api->Check();
#endif

	// Test for index 
	for (tInt wRow = 0; wRow < 10; wRow++) {
		for (tInt wCol = 0; wCol < 10; wCol++) {
			tRange* wRange = m_Api->EnsureRange(wRow, wCol, wRow+10,wCol+2);
			tRange* wRangeSearch = m_Api->FindRange(wRow, wCol, wRow+10,wCol+2);
			tStringStream wStream;
			wStream << "Test Range not equal " << wRange->StrRef() << "=" << wRangeSearch->StrRef();
            //cout << wStream.str() << endl;
			CPPUNIT_ASSERT_MESSAGE(wStream.str(), wRangeSearch == wRange);
		}
	}
	 
	/* 
	wVectorRange = m_Api->FindRangesCovered(5, 5);
	for (auto wRange : (*wVectorRange)) {
		cout << wRange->StrRef() << endl;
	}
	*/
#ifdef checksp
	m_Api->Check();
#endif

}

void TestSkRange::TestRectIsValid() {
    {
        tRect wDefault;
        CPPUNIT_ASSERT_MESSAGE("Default tRect (-1 corners) is invalid", !wDefault.IsValid());
    }
    {
        tRect wOk(1, 1, 5, 5);
        CPPUNIT_ASSERT_MESSAGE("Ordinary tRect is valid", wOk.IsValid());
    }
    {
        tRect wSingle(3, 7, 3, 7);
        CPPUNIT_ASSERT_MESSAGE("1x1 tRect is valid", wSingle.IsValid());
    }
    {
        tRect wInvertedRows(5, 1, 1, 5);
        CPPUNIT_ASSERT_MESSAGE("Top > Bottom is invalid", !wInvertedRows.IsValid());
    }
    {
        tRect wInvertedCols(1, 5, 5, 1);
        CPPUNIT_ASSERT_MESSAGE("Left > Right is invalid", !wInvertedCols.IsValid());
    }
    {
        tRect wNegative(-1, 0, 2, 2);
        CPPUNIT_ASSERT_MESSAGE("Negative corner makes tRect invalid", !wNegative.IsValid());
    }

    {
        tTempoRect wDefault;
        CPPUNIT_ASSERT_MESSAGE("Default tTempoRect is invalid", !wDefault.IsValid());
    }
    {
        tTempoRect wOk(1, 1, 5, 5);
        CPPUNIT_ASSERT_MESSAGE("Ordinary tTempoRect is valid", wOk.IsValid());
    }
    {
        tTempoRect wInverted(5, 1, 1, 5);
        CPPUNIT_ASSERT_MESSAGE("Inverted rows on tTempoRect is invalid", !wInverted.IsValid());
    }
}

void TestSkRange::TestFormulaRange() {
	tBool wResult=m_Api->UndoCellValue("A1","=SUM(A2:BZ100)");
	CPPUNIT_ASSERT_MESSAGE("TestFormulaRange ", wResult);

#ifdef checksp
	m_Api->Check();
#endif
	m_Api->Undo();
#ifdef checksp
	m_Api->Check();
#endif
    m_Api->Redo();
#ifdef checksp
	m_Api->Check();
#endif

}

void TestSkRange::TestRangeNamed() {
	m_Api->UndoInsertRangeNamed("TEST1", "B2:G4");
	tRange* wRange = m_Api->FindRangeNamed("TEST1");
	CPPUNIT_ASSERT_MESSAGE("TestRangeNamed 1", wRange != nullptr);

	m_Api->UndoDeleteRow(0, 10);
	
#ifdef checksp
	m_Api->Check();
#endif
	m_Api->Undo();

#ifdef checksp
	m_Api->Check();
#endif
	wRange = m_Api->FindRangeNamed("TEST1");
	CPPUNIT_ASSERT_MESSAGE("TestRangeNamed 1", wRange != nullptr);

	m_Api->UndoDeleteRangeNamed("TEST1");
	wRange = m_Api->FindRangeNamed("TEST1");
	CPPUNIT_ASSERT_MESSAGE("TestRangeNamed 2", wRange == nullptr);
 
    // Check function Range Named
    m_Api->UndoCellValue("A1",1);
    m_Api->UndoCellValue("A2",2);
    m_Api->UndoCellValue("A3",3);
    
    m_Api->UndoInsertRangeNamed("SUMRange", "A1:A3");
    
    tBool wOk=m_Api->UndoCellValue("A4","=SUM(SUMRange)");
    
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamed SUM(SUMRange)", wOk);
    
    tVariant wResult=m_Api->CellValue("A4");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamed SUM(SUMRange)=6", wResult==6);
}

void TestSkRange::TestRangeNamedCalcul() {
    tBool wOk=m_Api->UndoInsertRangeNamed("OneCell", "$A$1");
    CPPUNIT_ASSERT_MESSAGE("TTestSkRange::TestRangeNamedCalcul $A$1", wOk);
    //cout << endl << m_Api->JsonRangeNamed() << endl;
    
    m_Api->UndoCellValue("A1","Ca Marche");
    
    m_Api->UndoCellValue("A2","=OneCell+\" Stéphane\"");

    tCell* wCell=m_Api->Cell("A2");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamed Calcul OneCell Str", wCell->Value().String() == "Ca Marche Stéphane");
    //cout << wCell->Value().String() << endl;
    //cout << wCell->Debug() << endl;
}

void TestSkRange::TestRangeNamedWidthRecover() {
	m_Api->UndoInsertRangeNamed("TEST1", "B2:G4");
	tRange* wRange = m_Api->FindRangeNamed("TEST1");
	CPPUNIT_ASSERT_MESSAGE("TestRangeNamedWidthRecover 1", wRange != nullptr);
	m_Api->UndoInsertRangeNamed("TEST2", "B2:G5");

	m_Api->UndoDeleteRow(4, 2);
#ifdef checksp
	m_Api->Check();
#endif

	m_Api->Undo();

#ifdef checksp
	m_Api->Check();
#endif
	wRange = m_Api->FindRangeNamed("TEST1");
	CPPUNIT_ASSERT_MESSAGE("TestRangeNamed 1", wRange->Name() == "TEST1");
	wRange = m_Api->FindRangeNamed("TEST2");
	CPPUNIT_ASSERT_MESSAGE("TestRangeNamed 1", wRange->Name() == "TEST2");
}

void TestSkRange::TestRangeNamedMultiArea() {
    // Insert a multi-area named range on a single sheet.
    tBool wOk = m_Api->UndoInsertRangeNamed("MULTI", "A1:B2;D5:E6");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea insert", wOk);

    // FindRangeNamed returns the first area for back-compat with the
    // formula engine; StrRef must match the leading rectangle.
    tRange* wFirst = m_Api->FindRangeNamed("MULTI");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea first range", wFirst != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea first ref",
                           wFirst->StrRef() == "A1:B2");

    // Reach into the container to verify all areas are registered under the
    // same name (same sheet, two range allocator refs).
    tRangeNamedContainer* wContainer = m_Api->ActiveWorkBook()->RangeNamedContainer();
    std::vector<tRange*> wRanges = wContainer->Ranges("MULTI");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea 2 areas",
                           wRanges.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea area 0 ref",
                           wRanges[0] != nullptr && wRanges[0]->StrRef() == "A1:B2");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea area 1 ref",
                           wRanges[1] != nullptr && wRanges[1]->StrRef() == "D5:E6");

    // Reverse lookup: each area should point back to "MULTI". The test fixture
    // only creates a single active sheet, so both ranges share it.
    tSheet* wSheet = m_Api->ActiveSheet();
    tString wName0 = m_Api->ActiveWorkBook()->FindRangeNamed(wRanges[0]->AllocatorRef(),
                                                             wSheet);
    tString wName1 = m_Api->ActiveWorkBook()->FindRangeNamed(wRanges[1]->AllocatorRef(),
                                                             wSheet);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea reverse area 0", wName0 == "MULTI");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea reverse area 1", wName1 == "MULTI");

    // JSON round-trip preserves every area ("r":"A1:B2;D5:E6").
    tString wSave = m_Api->WriteJson("wwww.skeema.fr/w1");
    tBool wLoad = m_Api->ReadJson(wSave);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea ReadJson", wLoad);
    wRanges = m_Api->ActiveWorkBook()->RangeNamedContainer()->Ranges("MULTI");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea round-trip 2 areas",
                           wRanges.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea round-trip area 0",
                           wRanges[0] != nullptr && wRanges[0]->StrRef() == "A1:B2");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea round-trip area 1",
                           wRanges[1] != nullptr && wRanges[1]->StrRef() == "D5:E6");

    // DeleteRangeNamed must remove every area and clean up the reverse map.
    wOk = m_Api->UndoDeleteRangeNamed("MULTI");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea delete", wOk);
    wRanges = m_Api->ActiveWorkBook()->RangeNamedContainer()->Ranges("MULTI");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea delete -> empty",
                           wRanges.empty());
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea FindRangeNamed after delete",
                           m_Api->FindRangeNamed("MULTI") == nullptr);

    // Undo brings back the full multi-area selection.
    m_Api->Undo();
    wRanges = m_Api->ActiveWorkBook()->RangeNamedContainer()->Ranges("MULTI");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea undo -> 2 areas",
                           wRanges.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea undo area 0",
                           wRanges[0] != nullptr && wRanges[0]->StrRef() == "A1:B2");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea undo area 1",
                           wRanges[1] != nullptr && wRanges[1]->StrRef() == "D5:E6");

    // Formula evaluation: SUM(MULTI) must aggregate both areas, the same way
    // Excel does when passing a multi-area reference to an aggregation
    // function. Fill A1:B2 with 1..4 and D5:E6 with 5..8 so the expected
    // sum is 36.
    m_Api->UndoCellValue("A1", 1);
    m_Api->UndoCellValue("B1", 2);
    m_Api->UndoCellValue("A2", 3);
    m_Api->UndoCellValue("B2", 4);
    m_Api->UndoCellValue("D5", 5);
    m_Api->UndoCellValue("E5", 6);
    m_Api->UndoCellValue("D6", 7);
    m_Api->UndoCellValue("E6", 8);

    wOk = m_Api->UndoCellValue("H1", "=SUM(MULTI)");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea SUM(MULTI) compile", wOk);
    tVariant wSum = m_Api->CellValue("H1");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea SUM(MULTI)=36", wSum == 36);

    // Rendering must preserve the multi-area name (no expansion to the raw
    // rectangles). FormulaStr() returns the display without leading '='.
    tCell* wCellH1 = m_Api->Cell("H1");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea H1 cell", wCellH1 != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea render keeps MULTI",
                           wCellH1->FormulaStr() == "SUM(MULTI)");

    // JSON round-trip exercises recompile + Str(): value and rendering must
    // both survive the serialization cycle.
    wSave = m_Api->WriteJson("wwww.skeema.fr/w1");
    wLoad = m_Api->ReadJson(wSave);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea formula ReadJson", wLoad);
    tVariant wSumAfter = m_Api->CellValue("H1");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea SUM(MULTI)=36 after roundtrip",
                           wSumAfter == 36);
    tCell* wCellH1After = m_Api->Cell("H1");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea H1 after roundtrip",
                           wCellH1After != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea render keeps MULTI after roundtrip",
                           wCellH1After->FormulaStr() == "SUM(MULTI)");

    // Other aggregation functions must walk the same expansion path driven by
    // ShouldExpandMultiAreaNamedRange(). AVERAGE and COUNT expose two things
    // we care about: (1) they consume N range arguments just like SUM, and
    // (2) their rendering must keep the name "MULTI" through a save/load
    // cycle. 8 values summing to 36, so AVERAGE=4.5 and COUNT=8.
    wOk = m_Api->UndoCellValue("H2", "=AVERAGE(MULTI)");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea AVERAGE(MULTI) compile", wOk);
    tVariant wAvg = m_Api->CellValue("H2");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea AVERAGE(MULTI)=4.5", wAvg == 4.5);
    tCell* wCellH2 = m_Api->Cell("H2");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea H2 cell", wCellH2 != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea render keeps MULTI for AVERAGE",
                           wCellH2->FormulaStr() == "AVERAGE(MULTI)");

    wOk = m_Api->UndoCellValue("H3", "=COUNT(MULTI)");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea COUNT(MULTI) compile", wOk);
    tVariant wCnt = m_Api->CellValue("H3");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea COUNT(MULTI)=8", wCnt == 8);
    tCell* wCellH3 = m_Api->Cell("H3");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea H3 cell", wCellH3 != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea render keeps MULTI for COUNT",
                           wCellH3->FormulaStr() == "COUNT(MULTI)");

    // Cross-context: a byref function (ROWS) must NOT expand the multi-area
    // name into several arguments — ROWS expects a single reference. The
    // compiler falls back to the first area (A1:B2 -> 2 rows) instead of
    // flooding the function with extra refs that would break its arity.
    // Rendering keeps the user-facing name.
    wOk = m_Api->UndoCellValue("H4", "=ROWS(MULTI)");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea ROWS(MULTI) compile", wOk);
    tVariant wRows = m_Api->CellValue("H4");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea ROWS(MULTI)=2 (first area)", wRows == 2);
    tCell* wCellH4 = m_Api->Cell("H4");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea H4 cell", wCellH4 != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea render keeps MULTI for ROWS",
                           wCellH4->FormulaStr() == "ROWS(MULTI)");

    // Cross-context: a binary operator is outside any function so the
    // compiler is in the same fallback (first area) — nothing gets expanded
    // and the stack stays balanced. We only assert the formula compiles and
    // is rendered back with the name; the numeric value depends on the
    // engine's range+scalar semantics (#VALUE!-like in Excel) so we avoid
    // over-specifying it here.
    wOk = m_Api->UndoCellValue("H5", "=MULTI+1");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea MULTI+1 compile", wOk);
    tCell* wCellH5 = m_Api->Cell("H5");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea H5 cell", wCellH5 != nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea render keeps MULTI for binary op",
                           wCellH5->FormulaStr() == "MULTI+1");

    // Roundtrip the whole sheet: every cross-context cell must survive a
    // save/load cycle keeping both its value and its formula rendering.
    wSave = m_Api->WriteJson("wwww.skeema.fr/w1");
    wLoad = m_Api->ReadJson(wSave);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea cross-context ReadJson", wLoad);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea AVERAGE after roundtrip",
                           m_Api->CellValue("H2") == 4.5);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea COUNT after roundtrip",
                           m_Api->CellValue("H3") == 8);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea ROWS after roundtrip",
                           m_Api->CellValue("H4") == 2);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea AVERAGE render after roundtrip",
                           m_Api->Cell("H2")->FormulaStr() == "AVERAGE(MULTI)");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea COUNT render after roundtrip",
                           m_Api->Cell("H3")->FormulaStr() == "COUNT(MULTI)");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea ROWS render after roundtrip",
                           m_Api->Cell("H4")->FormulaStr() == "ROWS(MULTI)");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea MULTI+1 render after roundtrip",
                           m_Api->Cell("H5")->FormulaStr() == "MULTI+1");

    // MIN / MAX / PRODUCT / COUNTA exercise the same expansion path but with
    // different reductions. Values span 1..8 across the two areas so:
    //   MIN     = 1
    //   MAX     = 8
    //   PRODUCT = 8! = 40320
    //   COUNTA  = 8
    wOk = m_Api->UndoCellValue("H6", "=MIN(MULTI)");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea MIN(MULTI) compile", wOk);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea MIN(MULTI)=1",
                           m_Api->CellValue("H6") == 1);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea MIN render",
                           m_Api->Cell("H6")->FormulaStr() == "MIN(MULTI)");

    wOk = m_Api->UndoCellValue("H7", "=MAX(MULTI)");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea MAX(MULTI) compile", wOk);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea MAX(MULTI)=8",
                           m_Api->CellValue("H7") == 8);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea MAX render",
                           m_Api->Cell("H7")->FormulaStr() == "MAX(MULTI)");

    wOk = m_Api->UndoCellValue("H8", "=PRODUCT(MULTI)");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea PRODUCT(MULTI) compile", wOk);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea PRODUCT(MULTI)=40320",
                           m_Api->CellValue("H8") == 40320);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea PRODUCT render",
                           m_Api->Cell("H8")->FormulaStr() == "PRODUCT(MULTI)");

    wOk = m_Api->UndoCellValue("H9", "=COUNTA(MULTI)");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea COUNTA(MULTI) compile", wOk);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea COUNTA(MULTI)=8",
                           m_Api->CellValue("H9") == 8);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea COUNTA render",
                           m_Api->Cell("H9")->FormulaStr() == "COUNTA(MULTI)");

    // Mixed-arity: the multi-area name must behave as N separate arguments
    // alongside other literal or range arguments. SUM(MULTI, 100) = 36 + 100.
    // Mixed-arity formulas: the rendered separator follows the current
    // locale (';' in FR, ',' in en-US). We retrieve it from the application
    // so the test does not hardcode either variant.
    tString wSep(1, tApplication::Instance()->Locale()->Arg());

    wOk = m_Api->UndoCellValue("H10", "=SUM(MULTI" + wSep + "100)");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea SUM(MULTI,100) compile", wOk);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea SUM(MULTI,100)=136",
                           m_Api->CellValue("H10") == 136);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea SUM(MULTI,100) render",
                           m_Api->Cell("H10")->FormulaStr() == "SUM(MULTI" + wSep + "100)");

    // Also with a leading literal so the expansion does not happen at the
    // head of the argument list.
    wOk = m_Api->UndoCellValue("H11", "=SUM(10" + wSep + "MULTI)");
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea SUM(10,MULTI) compile", wOk);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea SUM(10,MULTI)=46",
                           m_Api->CellValue("H11") == 46);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea SUM(10,MULTI) render",
                           m_Api->Cell("H11")->FormulaStr() == "SUM(10" + wSep + "MULTI)");

    // Final roundtrip covering the new cells.
    wSave = m_Api->WriteJson("wwww.skeema.fr/w1");
    wLoad = m_Api->ReadJson(wSave);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea minmax ReadJson", wLoad);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea MIN after roundtrip",
                           m_Api->CellValue("H6") == 1);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea MAX after roundtrip",
                           m_Api->CellValue("H7") == 8);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea PRODUCT after roundtrip",
                           m_Api->CellValue("H8") == 40320);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea COUNTA after roundtrip",
                           m_Api->CellValue("H9") == 8);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea SUM(MULTI,100) after roundtrip",
                           m_Api->CellValue("H10") == 136);
    CPPUNIT_ASSERT_MESSAGE("TestRangeNamedMultiArea SUM(10,MULTI) after roundtrip",
                           m_Api->CellValue("H11") == 46);
}

void TestSkRange::TestRangeMerged() {
    // Create
    m_Api->UndoApplyMerge("B2:D3");
    tSheet* wSheet=m_Api->ActiveSheet();
    tRange* wRange=wSheet->Range(2, 2, 3,4);
    
    CPPUNIT_ASSERT_MESSAGE("TestMerged not nullptr", wRange!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestMerged B2:D3", wRange->StrRef()=="B2:D3");
    CPPUNIT_ASSERT_MESSAGE("TestMerged IsMerged", wRange->IsMerged());
  
    // Delete
    m_Api->UndoApplyMerge("B2:D3");
    wRange=wSheet->Range(2, 2, 3,4);
    CPPUNIT_ASSERT_MESSAGE("TestMerged nullptr", wRange==nullptr);
 }

// Two named ranges that share a common area (A1:A2). Verifies that delete /
// undo / redelete on either name leaves the other one fully usable, both for
// lookup (Ranges) and for formula evaluation (SUM).
//
// Layout:
//   A1=1  B1=2  C1=10
//   A2=3  B2=4  C2=20
//
// Test1 = A1:A2;B1:B2  -> SUM = 1+3+2+4 = 10
// Test2 = A1:A2;C1:C2  -> SUM = 1+3+10+20 = 34
void TestSkRange::TestRangeNamedMultiAreaOverlap() {
    m_Api->UndoCellValue("A1", 1);
    m_Api->UndoCellValue("B1", 2);
    m_Api->UndoCellValue("C1", 10);
    m_Api->UndoCellValue("A2", 3);
    m_Api->UndoCellValue("B2", 4);
    m_Api->UndoCellValue("C2", 20);

    tBool wOk = m_Api->UndoInsertRangeNamed("Test1", "A1:A2;B1:B2");
    CPPUNIT_ASSERT_MESSAGE("Overlap insert Test1", wOk);
    wOk = m_Api->UndoInsertRangeNamed("Test2", "A1:A2;C1:C2");
    CPPUNIT_ASSERT_MESSAGE("Overlap insert Test2", wOk);

    // Both names are independently registered with both their areas.
    auto wContainer = m_Api->ActiveWorkBook()->RangeNamedContainer();
    std::vector<tRange*> wR1 = wContainer->Ranges("Test1");
    std::vector<tRange*> wR2 = wContainer->Ranges("Test2");
    CPPUNIT_ASSERT_MESSAGE("Overlap Test1 has 2 areas", wR1.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Overlap Test2 has 2 areas", wR2.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Overlap Test1[0]=A1:A2", wR1[0]->StrRef() == "A1:A2");
    CPPUNIT_ASSERT_MESSAGE("Overlap Test1[1]=B1:B2", wR1[1]->StrRef() == "B1:B2");
    CPPUNIT_ASSERT_MESSAGE("Overlap Test2[0]=A1:A2", wR2[0]->StrRef() == "A1:A2");
    CPPUNIT_ASSERT_MESSAGE("Overlap Test2[1]=C1:C2", wR2[1]->StrRef() == "C1:C2");
    CPPUNIT_ASSERT_MESSAGE("Overlap shared A1:A2 ptr", wR1[0] == wR2[0]);

    // Both formulas evaluate to their expected aggregation.
    wOk = m_Api->UndoCellValue("E1", "=SUM(Test1)");
    CPPUNIT_ASSERT_MESSAGE("Overlap SUM(Test1) compile", wOk);
    wOk = m_Api->UndoCellValue("E2", "=SUM(Test2)");
    CPPUNIT_ASSERT_MESSAGE("Overlap SUM(Test2) compile", wOk);
    CPPUNIT_ASSERT_MESSAGE("Overlap SUM(Test1)=10", m_Api->CellValue("E1") == 10);
    CPPUNIT_ASSERT_MESSAGE("Overlap SUM(Test2)=34", m_Api->CellValue("E2") == 34);

    // Delete Test1: Test2 must survive intact, including the shared A1:A2.
    wOk = m_Api->UndoDeleteRangeNamed("Test1");
    CPPUNIT_ASSERT_MESSAGE("Overlap delete Test1", wOk);
    CPPUNIT_ASSERT_MESSAGE("Overlap Test1 gone",
                           m_Api->FindRangeNamed("Test1") == nullptr);
    wR2 = wContainer->Ranges("Test2");
    CPPUNIT_ASSERT_MESSAGE("Overlap Test2 still has 2 areas after Test1 delete",
                           wR2.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Overlap Test2[0] still A1:A2 after delete",
                           wR2[0]->StrRef() == "A1:A2");
    // The shared A1:A2 must still carry the named flag (Test2 owns it).
    CPPUNIT_ASSERT_MESSAGE("Overlap A1:A2 still IsNamed after Test1 delete",
                           wR2[0]->IsNamed());
    // SUM(Test2) recomputed must still yield the right value (the formula's
    // VectorRef references the shared range objects).
    CPPUNIT_ASSERT_MESSAGE("Overlap SUM(Test2)=34 after Test1 delete",
                           m_Api->CellValue("E2") == 34);

    // Undo brings back Test1 with both areas, and Test2 is still healthy.
    m_Api->Undo();
    wR1 = wContainer->Ranges("Test1");
    wR2 = wContainer->Ranges("Test2");
    CPPUNIT_ASSERT_MESSAGE("Overlap undo restores Test1 (2 areas)", wR1.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Overlap undo Test1[0]=A1:A2",
                           wR1[0]->StrRef() == "A1:A2");
    CPPUNIT_ASSERT_MESSAGE("Overlap undo Test1[1]=B1:B2",
                           wR1[1]->StrRef() == "B1:B2");
    CPPUNIT_ASSERT_MESSAGE("Overlap undo Test2 still 2 areas", wR2.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Overlap undo shared A1:A2 ptr", wR1[0] == wR2[0]);
    CPPUNIT_ASSERT_MESSAGE("Overlap undo SUM(Test1)=10",
                           m_Api->CellValue("E1") == 10);
    CPPUNIT_ASSERT_MESSAGE("Overlap undo SUM(Test2)=34",
                           m_Api->CellValue("E2") == 34);

    // Reverse order: delete Test2, Test1 must keep functioning.
    wOk = m_Api->UndoDeleteRangeNamed("Test2");
    CPPUNIT_ASSERT_MESSAGE("Overlap delete Test2", wOk);
    CPPUNIT_ASSERT_MESSAGE("Overlap Test2 gone",
                           m_Api->FindRangeNamed("Test2") == nullptr);
    wR1 = wContainer->Ranges("Test1");
    CPPUNIT_ASSERT_MESSAGE("Overlap Test1 still 2 areas after Test2 delete",
                           wR1.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Overlap A1:A2 still IsNamed after Test2 delete",
                           wR1[0]->IsNamed());
    CPPUNIT_ASSERT_MESSAGE("Overlap SUM(Test1)=10 after Test2 delete",
                           m_Api->CellValue("E1") == 10);

    // Undo Test2 back, then delete BOTH and check the shared A1:A2 finally
    // loses its named flag.
    m_Api->Undo();
    wOk = m_Api->UndoDeleteRangeNamed("Test1");
    CPPUNIT_ASSERT_MESSAGE("Overlap re-delete Test1", wOk);
    wOk = m_Api->UndoDeleteRangeNamed("Test2");
    CPPUNIT_ASSERT_MESSAGE("Overlap re-delete Test2", wOk);
    tSheet* wSheet = m_Api->ActiveSheet();
    tRange* wA1A2 = wSheet->Range(1, 1, 2, 1);
    CPPUNIT_ASSERT_MESSAGE("Overlap A1:A2 alive", wA1A2 != nullptr);
    CPPUNIT_ASSERT_MESSAGE("Overlap A1:A2 not named once both deleted",
                           !wA1A2->IsNamed());

    // JSON round-trip with overlapping names re-registered: Test1 + Test2
    // both come back with their full area lists.
    m_Api->Undo();
    m_Api->Undo();
    wR1 = wContainer->Ranges("Test1");
    wR2 = wContainer->Ranges("Test2");
    CPPUNIT_ASSERT_MESSAGE("Overlap undo all restores Test1", wR1.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Overlap undo all restores Test2", wR2.size() == 2);

    tString wSave = m_Api->WriteJson("wwww.skeema.fr/w1");
    tBool wLoad = m_Api->ReadJson(wSave);
    CPPUNIT_ASSERT_MESSAGE("Overlap ReadJson", wLoad);
    auto wContainer2 = m_Api->ActiveWorkBook()->RangeNamedContainer();
    wR1 = wContainer2->Ranges("Test1");
    wR2 = wContainer2->Ranges("Test2");
    CPPUNIT_ASSERT_MESSAGE("Overlap JSON Test1 still 2 areas", wR1.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Overlap JSON Test2 still 2 areas", wR2.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Overlap JSON Test1[0]=A1:A2",
                           wR1[0]->StrRef() == "A1:A2");
    CPPUNIT_ASSERT_MESSAGE("Overlap JSON Test2[0]=A1:A2",
                           wR2[0]->StrRef() == "A1:A2");
    CPPUNIT_ASSERT_MESSAGE("Overlap JSON shared ptr restored",
                           wR1[0] == wR2[0]);
    CPPUNIT_ASSERT_MESSAGE("Overlap JSON SUM(Test1)=10",
                           m_Api->CellValue("E1") == 10);
    CPPUNIT_ASSERT_MESSAGE("Overlap JSON SUM(Test2)=34",
                           m_Api->CellValue("E2") == 34);
}

// Exercise the atomic UndoUpdateRangeNamed path covering every meaningful
// scenario: pure rename, selection change, rename + reselect, conflict,
// overlapping names, undo/redo and JSON roundtrip.
//
// Layout:
//   A1=1  B1=2  C1=10  D1=100
//   A2=3  B2=4  C2=20  D2=200
void TestSkRange::TestRangeNamedUpdate() {
    m_Api->UndoCellValue("A1", 1);
    m_Api->UndoCellValue("B1", 2);
    m_Api->UndoCellValue("C1", 10);
    m_Api->UndoCellValue("D1", 100);
    m_Api->UndoCellValue("A2", 3);
    m_Api->UndoCellValue("B2", 4);
    m_Api->UndoCellValue("C2", 20);
    m_Api->UndoCellValue("D2", 200);

    // -- Scenario 1: pure rename (Alpha -> AlphaRenamed), same selection.
    //
    // We capture the tRange* BEFORE the rename and check it is still alive
    // (same pointer -> same tAllocatorRef) AFTER. That is the core promise:
    // keeping the allocator refs lets every formula that embeds the range in
    // its VectorRef keep evaluating, and tFormula::ClassOrRangeStr picks up
    // the new name through m_MapRef automatically.
    tBool wOk = m_Api->UndoInsertRangeNamed("Alpha", "A1:A2;B1:B2");
    CPPUNIT_ASSERT_MESSAGE("Update insert Alpha", wOk);

    auto wContainer = m_Api->ActiveWorkBook()->RangeNamedContainer();
    std::vector<tRange*> wAlphaBefore = wContainer->Ranges("Alpha");
    CPPUNIT_ASSERT_MESSAGE("Update Alpha has 2 areas", wAlphaBefore.size() == 2);
    tRange* wA1A2Before = wAlphaBefore[0];
    tRange* wB1B2Before = wAlphaBefore[1];

    wOk = m_Api->UndoCellValue("E1", "=SUM(Alpha)");
    CPPUNIT_ASSERT_MESSAGE("Update SUM(Alpha) compile", wOk);
    CPPUNIT_ASSERT_MESSAGE("Update SUM(Alpha)=10", m_Api->CellValue("E1") == 10);
    CPPUNIT_ASSERT_MESSAGE("Update E1 renders Alpha",
                           m_Api->Cell("E1")->FormulaStr() == "SUM(Alpha)");

    wOk = m_Api->UndoUpdateRangeNamed("Alpha", "AlphaRenamed", "A1:A2;B1:B2");
    CPPUNIT_ASSERT_MESSAGE("Update rename Alpha -> AlphaRenamed", wOk);

    // Old name is gone, new name owns the same two tRange objects.
    CPPUNIT_ASSERT_MESSAGE("Update old Alpha gone",
                           m_Api->FindRangeNamed("Alpha") == nullptr);
    std::vector<tRange*> wAlphaAfter = wContainer->Ranges("AlphaRenamed");
    CPPUNIT_ASSERT_MESSAGE("Update AlphaRenamed has 2 areas",
                           wAlphaAfter.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Update AlphaRenamed[0] keeps allocatorRef",
                           wAlphaAfter[0] == wA1A2Before);
    CPPUNIT_ASSERT_MESSAGE("Update AlphaRenamed[1] keeps allocatorRef",
                           wAlphaAfter[1] == wB1B2Before);

    // Formula keeps evaluating AND re-renders with the new name.
    CPPUNIT_ASSERT_MESSAGE("Update SUM=10 after rename",
                           m_Api->CellValue("E1") == 10);
    CPPUNIT_ASSERT_MESSAGE("Update E1 renders new name",
                           m_Api->Cell("E1")->FormulaStr() == "SUM(AlphaRenamed)");

    // Undo brings the old name back (and formula re-renders Alpha again).
    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Update undo restores Alpha",
                           wContainer->Ranges("Alpha").size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Update undo drops AlphaRenamed",
                           m_Api->FindRangeNamed("AlphaRenamed") == nullptr);
    CPPUNIT_ASSERT_MESSAGE("Update undo keeps allocatorRef A1:A2",
                           wContainer->Ranges("Alpha")[0] == wA1A2Before);
    CPPUNIT_ASSERT_MESSAGE("Update undo keeps allocatorRef B1:B2",
                           wContainer->Ranges("Alpha")[1] == wB1B2Before);
    CPPUNIT_ASSERT_MESSAGE("Update undo SUM=10",
                           m_Api->CellValue("E1") == 10);
    CPPUNIT_ASSERT_MESSAGE("Update undo E1 renders Alpha",
                           m_Api->Cell("E1")->FormulaStr() == "SUM(Alpha)");

    // Redo re-applies the rename cleanly.
    m_Api->Redo();
    CPPUNIT_ASSERT_MESSAGE("Update redo AlphaRenamed present",
                           wContainer->Ranges("AlphaRenamed").size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Update redo E1 renders new name",
                           m_Api->Cell("E1")->FormulaStr() == "SUM(AlphaRenamed)");
    // Undo back to initial state for the rest of the test.
    m_Api->Undo();

    // -- Scenario 2: keep the name, change the selection (add an area).
    //
    // Adding a new area to a named range only affects formulas compiled
    // AFTER the change (LemonInterface expands the name at compile time).
    // Pre-existing formulas keep their VectorRef untouched -> their value
    // stays the same. This is the same semantics InsertRangeNamed exposes
    // today, so the atomic update mirrors it on purpose.
    wOk = m_Api->UndoUpdateRangeNamed("Alpha", "Alpha",
                                      "A1:A2;B1:B2;C1:C2");
    CPPUNIT_ASSERT_MESSAGE("Update reselect Alpha add C1:C2", wOk);
    std::vector<tRange*> wAlphaGrown = wContainer->Ranges("Alpha");
    CPPUNIT_ASSERT_MESSAGE("Update Alpha now 3 areas", wAlphaGrown.size() == 3);
    CPPUNIT_ASSERT_MESSAGE("Update Alpha[0] kept",
                           wAlphaGrown[0] == wA1A2Before);
    CPPUNIT_ASSERT_MESSAGE("Update Alpha[1] kept",
                           wAlphaGrown[1] == wB1B2Before);
    CPPUNIT_ASSERT_MESSAGE("Update Alpha[2]=C1:C2",
                           wAlphaGrown[2] != nullptr
                           && wAlphaGrown[2]->StrRef() == "C1:C2");
    // Pre-existing formula keeps its compiled shape.
    CPPUNIT_ASSERT_MESSAGE("Update pre-existing SUM(Alpha) still=10",
                           m_Api->CellValue("E1") == 10);
    // A brand-new formula picks up all three areas since LemonInterface is
    // run now: 1+3+2+4+10+20=40.
    wOk = m_Api->UndoCellValue("F1", "=SUM(Alpha)");
    CPPUNIT_ASSERT_MESSAGE("Update new SUM(Alpha) compile after reselect", wOk);
    CPPUNIT_ASSERT_MESSAGE("Update new SUM(Alpha)=40 after reselect",
                           m_Api->CellValue("F1") == 40);
    // Both formulas render with the same name.
    CPPUNIT_ASSERT_MESSAGE("Update E1 still renders Alpha",
                           m_Api->Cell("E1")->FormulaStr() == "SUM(Alpha)");
    CPPUNIT_ASSERT_MESSAGE("Update F1 renders Alpha",
                           m_Api->Cell("F1")->FormulaStr() == "SUM(Alpha)");

    // Undo the F1 creation and the reselect. Alpha is back to 2 areas.
    m_Api->Undo();
    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Update reselect undo 2 areas",
                           wContainer->Ranges("Alpha").size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Update reselect undo SUM(E1)=10",
                           m_Api->CellValue("E1") == 10);

    // -- Scenario 3: remove an area. The vanishing range should lose its
    // named flag (it is not shared) while the remaining one keeps it.
    wOk = m_Api->UndoUpdateRangeNamed("Alpha", "Alpha", "A1:A2");
    CPPUNIT_ASSERT_MESSAGE("Update shrink Alpha to A1:A2", wOk);
    std::vector<tRange*> wShrunk = wContainer->Ranges("Alpha");
    CPPUNIT_ASSERT_MESSAGE("Update Alpha now 1 area", wShrunk.size() == 1);
    CPPUNIT_ASSERT_MESSAGE("Update Alpha[0] kept pointer",
                           wShrunk[0] == wA1A2Before);
    // B1:B2 still exists as a tRange* but is no longer "named".
    CPPUNIT_ASSERT_MESSAGE("Update A1:A2 still named",
                           wA1A2Before->IsNamed());
    CPPUNIT_ASSERT_MESSAGE("Update B1:B2 no longer named",
                           !wB1B2Before->IsNamed());
    // Pre-existing formula's VectorRef still references A1:A2 + B1:B2 so
    // the value is unchanged (=10). A new formula would see only A1:A2.
    CPPUNIT_ASSERT_MESSAGE("Update shrink E1 still=10",
                           m_Api->CellValue("E1") == 10);
    wOk = m_Api->UndoCellValue("F2", "=SUM(Alpha)");
    CPPUNIT_ASSERT_MESSAGE("Update shrink SUM compile", wOk);
    CPPUNIT_ASSERT_MESSAGE("Update shrink new SUM(Alpha)=4 (A1+A2)",
                           m_Api->CellValue("F2") == 4);
    m_Api->Undo();
    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Update shrink undo restores B1:B2 flag",
                           wB1B2Before->IsNamed());

    // -- Scenario 4: rename + reselect in one atomic step. tAllocatorRef is
    // preserved on A1:A2/B1:B2 (kept) and the rename propagates to the
    // existing formula through m_MapRef -> FormulaStr.
    wOk = m_Api->UndoUpdateRangeNamed("Alpha", "AlphaFull",
                                      "A1:A2;B1:B2;D1:D2");
    CPPUNIT_ASSERT_MESSAGE("Update combined rename+reselect", wOk);
    CPPUNIT_ASSERT_MESSAGE("Update Alpha gone",
                           m_Api->FindRangeNamed("Alpha") == nullptr);
    std::vector<tRange*> wFull = wContainer->Ranges("AlphaFull");
    CPPUNIT_ASSERT_MESSAGE("Update AlphaFull 3 areas", wFull.size() == 3);
    CPPUNIT_ASSERT_MESSAGE("Update AlphaFull[0] kept",
                           wFull[0] == wA1A2Before);
    CPPUNIT_ASSERT_MESSAGE("Update AlphaFull[1] kept",
                           wFull[1] == wB1B2Before);
    CPPUNIT_ASSERT_MESSAGE("Update AlphaFull[2]=D1:D2",
                           wFull[2] != nullptr
                           && wFull[2]->StrRef() == "D1:D2");
    // Pre-existing formula keeps its 2-area compiled form but re-renders
    // with the new name picked up from m_MapRef: the multi-area match
    // cannot match 3 areas against a VectorRef of 2, but the single-area
    // fallback in ClassOrRangeStr still renders AlphaFull for the kept
    // slots. Either way value stays 10.
    CPPUNIT_ASSERT_MESSAGE("Update combined pre-existing SUM=10",
                           m_Api->CellValue("E1") == 10);
    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Update combined undo restores Alpha",
                           wContainer->Ranges("Alpha").size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Update combined undo SUM=10",
                           m_Api->CellValue("E1") == 10);

    // -- Scenario 5: failure cases. Unknown name and conflicting target
    // must leave the state untouched.
    wOk = m_Api->UndoUpdateRangeNamed("Nope", "Other", "A1:A2");
    CPPUNIT_ASSERT_MESSAGE("Update fails on unknown source", !wOk);
    m_Api->UndoInsertRangeNamed("Beta", "D1:D2");
    wOk = m_Api->UndoUpdateRangeNamed("Alpha", "Beta", "A1:A2;B1:B2");
    CPPUNIT_ASSERT_MESSAGE("Update fails on conflicting target", !wOk);
    CPPUNIT_ASSERT_MESSAGE("Update failure keeps Alpha",
                           wContainer->Ranges("Alpha").size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Update failure keeps Beta",
                           wContainer->Ranges("Beta").size() == 1);
    m_Api->UndoDeleteRangeNamed("Beta");

    // -- Scenario 6: overlap awareness. Alpha and Gamma share A1:A2. When we
    // rename Gamma, Alpha (including the shared A1:A2) must stay intact.
    wOk = m_Api->UndoInsertRangeNamed("Gamma", "A1:A2;C1:C2");
    CPPUNIT_ASSERT_MESSAGE("Update insert overlap Gamma", wOk);
    CPPUNIT_ASSERT_MESSAGE("Update Gamma+Alpha share A1:A2",
                           wContainer->Ranges("Gamma")[0]
                           == wContainer->Ranges("Alpha")[0]);
    wOk = m_Api->UndoCellValue("E2", "=SUM(Gamma)");
    CPPUNIT_ASSERT_MESSAGE("Update SUM(Gamma) compile", wOk);
    CPPUNIT_ASSERT_MESSAGE("Update SUM(Gamma)=34",
                           m_Api->CellValue("E2") == 34);

    wOk = m_Api->UndoUpdateRangeNamed("Gamma", "GammaRenamed",
                                      "A1:A2;C1:C2");
    CPPUNIT_ASSERT_MESSAGE("Update rename Gamma overlap", wOk);
    // Alpha survives untouched.
    std::vector<tRange*> wAlphaKept = wContainer->Ranges("Alpha");
    CPPUNIT_ASSERT_MESSAGE("Update overlap Alpha still 2 areas",
                           wAlphaKept.size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Update overlap Alpha kept pointer",
                           wAlphaKept[0] == wA1A2Before);
    CPPUNIT_ASSERT_MESSAGE("Update overlap Alpha A1:A2 still named",
                           wAlphaKept[0]->IsNamed());
    CPPUNIT_ASSERT_MESSAGE("Update overlap SUM(Alpha)=10",
                           m_Api->CellValue("E1") == 10);
    CPPUNIT_ASSERT_MESSAGE("Update overlap SUM(GammaRenamed)=34",
                           m_Api->CellValue("E2") == 34);
    // Both formulas re-render the right name thanks to the overlap-aware
    // ClassOrRangeStr that scans AllNames() for a full-sequence match.
    CPPUNIT_ASSERT_MESSAGE("Update overlap E1 renders Alpha",
                           m_Api->Cell("E1")->FormulaStr() == "SUM(Alpha)");
    CPPUNIT_ASSERT_MESSAGE("Update overlap E2 renders GammaRenamed",
                           m_Api->Cell("E2")->FormulaStr()
                           == "SUM(GammaRenamed)");

    // -- Scenario 7: JSON roundtrip after an update must preserve the new
    // name, the value, and the formula rendering.
    tString wSave = m_Api->WriteJson("wwww.skeema.fr/w1");
    tBool wLoad = m_Api->ReadJson(wSave);
    CPPUNIT_ASSERT_MESSAGE("Update JSON ReadJson", wLoad);
    auto wContainer2 = m_Api->ActiveWorkBook()->RangeNamedContainer();
    CPPUNIT_ASSERT_MESSAGE("Update JSON Alpha restored",
                           wContainer2->Ranges("Alpha").size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Update JSON GammaRenamed restored",
                           wContainer2->Ranges("GammaRenamed").size() == 2);
    CPPUNIT_ASSERT_MESSAGE("Update JSON Gamma gone",
                           m_Api->FindRangeNamed("Gamma") == nullptr);
    CPPUNIT_ASSERT_MESSAGE("Update JSON SUM(Alpha)=10",
                           m_Api->CellValue("E1") == 10);
    CPPUNIT_ASSERT_MESSAGE("Update JSON SUM(GammaRenamed)=34",
                           m_Api->CellValue("E2") == 34);
    CPPUNIT_ASSERT_MESSAGE("Update JSON E1 renders Alpha",
                           m_Api->Cell("E1")->FormulaStr() == "SUM(Alpha)");
    CPPUNIT_ASSERT_MESSAGE("Update JSON E2 renders GammaRenamed",
                           m_Api->Cell("E2")->FormulaStr()
                           == "SUM(GammaRenamed)");

    // -- Scenario 8: single-cell named range rename.
    //
    // Regression for a UI-reported bug: create Test=L1:L1, write =Test into
    // another cell, rename Test to NewKey. Because the 1-cell path in
    // tLemonInterface::PushRef pushes the underlying tCell* (not the tRange*)
    // and relies on tCell::IsNamed() so tFormula::ClassOrRangeStr can
    // promote it back to the range when rendering, the rename MUST
    // reinstall that cell-level flag — otherwise the formula re-renders
    // the raw coordinates ("=L1") instead of the new name ("=NewKey").
    m_Api->UndoCellValue("L1", 42);
    wOk = m_Api->UndoInsertRangeNamed("Test", "L1:L1");
    CPPUNIT_ASSERT_MESSAGE("Update single-cell insert Test", wOk);

    tCell* wL1 = m_Api->Cell("L1");
    CPPUNIT_ASSERT_MESSAGE("Update single-cell L1 flagged named after insert",
                           wL1->IsNamed());

    wOk = m_Api->UndoCellValue("M1", "=Test");
    CPPUNIT_ASSERT_MESSAGE("Update single-cell =Test compile", wOk);
    CPPUNIT_ASSERT_MESSAGE("Update single-cell =Test value",
                           m_Api->CellValue("M1") == 42);
    CPPUNIT_ASSERT_MESSAGE("Update single-cell =Test renders name",
                           m_Api->Cell("M1")->FormulaStr() == "Test");

    wOk = m_Api->UndoUpdateRangeNamed("Test", "NewKey", "L1:L1");
    CPPUNIT_ASSERT_MESSAGE("Update single-cell rename Test -> NewKey", wOk);

    // The heart of the bug: the cell-level IsNamed() flag must survive the
    // delete+reinsert cycle that tUndoUpdateRangeNamed uses internally.
    CPPUNIT_ASSERT_MESSAGE("Update single-cell L1 still flagged named",
                           m_Api->Cell("L1")->IsNamed());
    CPPUNIT_ASSERT_MESSAGE("Update single-cell =Test still evaluates",
                           m_Api->CellValue("M1") == 42);
    CPPUNIT_ASSERT_MESSAGE("Update single-cell renders new name, not L1",
                           m_Api->Cell("M1")->FormulaStr() == "NewKey");

    m_Api->Undo();
    CPPUNIT_ASSERT_MESSAGE("Update single-cell undo restores Test",
                           m_Api->FindRangeNamed("Test") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("Update single-cell undo renders Test again",
                           m_Api->Cell("M1")->FormulaStr() == "Test");
}

// Verify the rename-gating policy wired to IsRenameAllowed():
//   - Default (solo, local) => every rename is accepted.
//   - Client mode + multi-user active => renames are refused but pure
//     reselects (same name, new selection) and no-op renames stay legal.
//   - Going back to "alone" re-enables renames without losing any state.
void TestSkRange::TestRangeNamedRenameGating() {
    m_Api->UndoCellValue("A1", 1);
    m_Api->UndoCellValue("A2", 2);
    m_Api->UndoCellValue("B1", 3);

    // Scenario 1: solo mode. Default flags leave IsRenameAllowed() == true,
    // so both sheet and named-range renames must succeed.
    CPPUNIT_ASSERT_MESSAGE("Gating solo rename allowed",
                           m_Api->IsRenameAllowed());

    tBool wOk = m_Api->UndoInsertRangeNamed("Solo", "A1:A2");
    CPPUNIT_ASSERT_MESSAGE("Gating insert Solo", wOk);
    wOk = m_Api->UndoUpdateRangeNamed("Solo", "SoloRenamed", "A1:A2");
    CPPUNIT_ASSERT_MESSAGE("Gating solo rename accepted", wOk);
    CPPUNIT_ASSERT_MESSAGE("Gating solo rename applied",
                           m_Api->FindRangeNamed("SoloRenamed") != nullptr);

    wOk = m_Api->UndoAddSheet("Helper");
    CPPUNIT_ASSERT_MESSAGE("Gating add Helper sheet", wOk);
    wOk = m_Api->UndoRenameSheet("Helper", "HelperRenamed");
    CPPUNIT_ASSERT_MESSAGE("Gating solo sheet rename accepted", wOk);
    CPPUNIT_ASSERT_MESSAGE("Gating solo sheet renamed",
                           m_Api->ActiveWorkBook()->Sheet("HelperRenamed")
                           != nullptr);

    // Scenario 2: simulated collaborative session with at least one remote
    // peer. Client() switches the API into "network" mode and
    // MultiUserActive(true) tells IsRenameAllowed() that a rewrite would
    // leave stale references on peers. Both renames must be refused.
    m_Api->Client(true);
    m_Api->MultiUserActive(true);
    CPPUNIT_ASSERT_MESSAGE("Gating multi-user forbids rename",
                           !m_Api->IsRenameAllowed());

    wOk = m_Api->UndoUpdateRangeNamed("SoloRenamed", "Forbidden", "A1:A2");
    CPPUNIT_ASSERT_MESSAGE("Gating multi-user rename refused", !wOk);
    CPPUNIT_ASSERT_MESSAGE("Gating multi-user range kept old name",
                           m_Api->FindRangeNamed("SoloRenamed") != nullptr);
    CPPUNIT_ASSERT_MESSAGE("Gating multi-user new name not created",
                           m_Api->FindRangeNamed("Forbidden") == nullptr);

    // Pure reselect (same name, different selection) must still be allowed
    // because the identifier visible to peers does not change.
    wOk = m_Api->UndoUpdateRangeNamed("SoloRenamed", "SoloRenamed",
                                      "A1:A2;B1:B1");
    CPPUNIT_ASSERT_MESSAGE("Gating multi-user reselect allowed", wOk);
    auto wRanges = m_Api->ActiveWorkBook()
                       ->RangeNamedContainer()->Ranges("SoloRenamed");
    CPPUNIT_ASSERT_MESSAGE("Gating reselect expanded to 2 areas",
                           wRanges.size() == 2);

    // Sheet rename is refused too.
    wOk = m_Api->UndoRenameSheet("HelperRenamed", "ShouldNotApply");
    CPPUNIT_ASSERT_MESSAGE("Gating multi-user sheet rename refused", !wOk);
    CPPUNIT_ASSERT_MESSAGE("Gating multi-user sheet kept old name",
                           m_Api->ActiveWorkBook()->Sheet("HelperRenamed")
                           != nullptr);

    // No-op rename (same name on both sides) should still be accepted since
    // there is nothing to propagate.
    wOk = m_Api->UndoRenameSheet("HelperRenamed", "HelperRenamed");
    CPPUNIT_ASSERT_MESSAGE("Gating multi-user no-op sheet rename accepted",
                           wOk);

    // Scenario 3: last peer leaves, we are alone again: renames re-enabled.
    m_Api->MultiUserActive(false);
    CPPUNIT_ASSERT_MESSAGE("Gating alone rename re-enabled",
                           m_Api->IsRenameAllowed());
    wOk = m_Api->UndoUpdateRangeNamed("SoloRenamed", "SoloFinal",
                                      "A1:A2;B1:B1");
    CPPUNIT_ASSERT_MESSAGE("Gating alone rename accepted again", wOk);
    CPPUNIT_ASSERT_MESSAGE("Gating alone rename applied",
                           m_Api->FindRangeNamed("SoloFinal") != nullptr);
}

void TestSkRange::TestRangeMergedWithContainer() {
    // Create
    m_Api->UndoApplyMerge("B2:D3");
    tSheet* wSheet=m_Api->ActiveSheet();
    tRange* wRange=wSheet->Range(2, 2, 3,4);
    
    m_Api->UndoCellValue("A1","=SUM(B2:D3)");
    
    CPPUNIT_ASSERT_MESSAGE("TestMerged not nullptr", wRange!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestMerged B2:D3", wRange->StrRef()=="B2:D3");
    CPPUNIT_ASSERT_MESSAGE("TestMerged IsMerged", wRange->IsMerged());
    
    // SetMerged to false
    m_Api->UndoApplyMerge("B2:D3");
    wRange=wSheet->Range(2, 2, 3,4);
    CPPUNIT_ASSERT_MESSAGE("TestMerged not nullptr", wRange!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestMerged not Merged", !wRange->IsMerged());
    
    // SetMerged to  true
    m_Api->UndoCellValue("A1",123);
    m_Api->UndoApplyMerge("B2:D3");
    wRange=wSheet->Range(2, 2, 3,4);
    CPPUNIT_ASSERT_MESSAGE("TestMerged not nullptr", wRange!=nullptr);
    CPPUNIT_ASSERT_MESSAGE("TestMerged B2:D3", wRange->StrRef()=="B2:D3");
    CPPUNIT_ASSERT_MESSAGE("TestMerged IsMerged", wRange->IsMerged());

    // Erase range
    m_Api->UndoApplyMerge("B2:D3");
    wRange=wSheet->Range(2, 2, 3,4);
    CPPUNIT_ASSERT_MESSAGE("TestMerged nullptr", wRange==nullptr);
    
    m_Api->UndoApplyMerge("B1:D5");
    
    m_Api->UndoApplyMerge("F2:G4");
   
    tVectorRange wVectorRange;
    tRect wRect(1,2,4,5); // B1:B5
    m_Api->FindRangesCovered(wRect, &wVectorRange);
    /*
    for(auto wRange : wVectorRange) {
        cout << "wRange " << wRange->StrRef() << endl;
    }
    cout << endl;
    */
}

void TestSkRange::TestRangeWithUTF8() {
    m_Api->UndoCellValue("B2", "Coucou !");
    // Single-cell named range: multi-cell B2:B3 makes & coerce a range to text with engine-specific rules;
    // scalar context must match the expected "CETTE ANNÉE (Coucou !)" string.
    m_Api->UndoInsertRangeNamed("AnnéeSélectionnée", "B2:B2");
    
    m_Api->UndoCellValue("A1","=\"CETTE ANNÉE (\"&AnnéeSélectionnée&\")\"");
    DebugCell("A1");
    tCell* wCellA1=m_Api->Cell("A1");
    tString wResult=wCellA1->Value().Str();
    // Accept NFC or NFD UTF-8: accented letters may be normalized differently.
    // Use char literals (not u8) so they match tString (std::string<char>), not char8_t[] in C++20.
    const tString wExpectedNfc = "CETTE ANNÉE (Coucou !)";
    const tString wExpectedNfd = "CETTE ANN\u0045\u0301E (Coucou !)";
    CPPUNIT_ASSERT_MESSAGE("TestRangeWithUTF8()", wResult == wExpectedNfc || wResult == wExpectedNfd);
}

void  TestSkRange::TestFormulaWithOutRange() {
    m_Api->UndoCellValue("A1","=\"CETTE ANNÉE (\"&AnnéeSélectionnée&\")\"");
    DebugCell("A1");
    tCell* wCellA1=m_Api->Cell("A1");
    tString wResult=wCellA1->Value().Str();
    CPPUNIT_ASSERT_MESSAGE("TestRangeWithUTF8()", wResult=="#NAME?");
    
   
}

void TestSkRange::setUp() {
	std::filesystem::remove_all("./Spreadsheet");

	m_Application = tApplication::Instance();
	m_Api = new tApi;
	m_Api->NewWorkBook("wwww.skeema.fr/w1");
};

void TestSkRange::tearDown() {
	delete(m_Api);
}

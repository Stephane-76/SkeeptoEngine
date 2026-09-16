//==============================================================================
// TestSkCopyPaste
// le 08/11/2023 
//==============================================================================
#ifndef TestSkCopyPaste_hpp
#define TestSkCopyPaste_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

#define TestAllCopyPaste

class TestSkCopyPaste : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
    CPPUNIT_TEST_SUITE(TestSkCopyPaste);
#ifndef TestAllCopyPaste
#ifdef TestMultiUser
	CPPUNIT_TEST(TestUndoMoveInterfaceWebMonoUser);
#endif
#endif
#ifdef TestAllCopyPaste
	CPPUNIT_TEST(TestCopy);
    CPPUNIT_TEST(TestCopySimpleRange);
	CPPUNIT_TEST(TestCopyRelativeAbsolute);
	CPPUNIT_TEST(TestCopyMultiple);
	CPPUNIT_TEST(TestCopyBorder);
	CPPUNIT_TEST(TestCopyBorderNeighborBackground);
	CPPUNIT_TEST(TestBorderOwnerWriteReadJson);
	CPPUNIT_TEST(TestReadJsonEmptyClearsFormatPool);
	CPPUNIT_TEST(TestUndoMoveAdjacentBorderedCellsE6ToE7);
	CPPUNIT_TEST(TestUndoMoveTableOntoOrangeBlock);
	CPPUNIT_TEST(TestUndoMoveTableValuesOntoOrangeBlock);
	CPPUNIT_TEST(TestBorderOwnerModelMoveUndo);
	CPPUNIT_TEST(TestCopyBorderRangePasteUndo);
	CPPUNIT_TEST(TestCopyBorderSingleCellPasteUndo);
	CPPUNIT_TEST(TestCopyBorderGridPastePreservesOuter);
	CPPUNIT_TEST(TestCopyGridPastePreservesOuterFillAfterLocalDo);
	CPPUNIT_TEST(TestUndoMoveBorderGhostSeamsAfterMove);
	CPPUNIT_TEST(TestUndoMoveUpLeftClearsSourceFillGhosts);
	CPPUNIT_TEST(TestUndoMoveUpLeftPreservesOuterBorders);
	CPPUNIT_TEST(TestUndoMoveOverlapUpLeftPreservesOuterBorders);
	CPPUNIT_TEST(TestUndoMove);
	CPPUNIT_TEST(TestUndoCut);
	CPPUNIT_TEST(TestUndoCutCollaborativeUndoJson);
	CPPUNIT_TEST(TestUndoMoveEmptySourcePreservesDestFormat);
	CPPUNIT_TEST(TestUndoMoveBorderShiftLeft);
	CPPUNIT_TEST(TestUndoMoveShiftDown);
	CPPUNIT_TEST(TestUndoMoveFormulaRefs);
	CPPUNIT_TEST(TestUndoMoveRebaseAfterInsertRowByRect);
#ifdef TestMultiUser
	// tInterfaceWeb + tTestDispatcher: in-process client/server simulation (required for Client() Do/Undo).
	CPPUNIT_TEST(TestUndoPasteInterfaceWebMonoUser);
	CPPUNIT_TEST(TestUndoPasteCollaborationPayloadTooLarge);
	CPPUNIT_TEST(TestUndoMoveInterfaceWebMonoUser);
	CPPUNIT_TEST(TestUndoCutInterfaceWebMonoUser);
#endif
#endif
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;
	tFormatApi* m_FormatApi;
    // For Fill
    tInt m_NbCol;
    tInt m_NbRow;
public:
	TestSkCopyPaste();
private:
	void DrawCell(tString sOperation,tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
    void Fill();
    void UndoOperation();

	void TestCopy();
    void TestCopySimpleRange();
	void TestCopyRelativeAbsolute();
	void TestCopyMultiple();
	void TestCopyBorder();
	void TestCopyBorderNeighborBackground();
	void TestBorderOwnerWriteReadJson();
	void TestReadJsonEmptyClearsFormatPool();
	void TestUndoMoveAdjacentBorderedCellsE6ToE7();
	void TestUndoMoveTableOntoOrangeBlock();
	void TestUndoMoveTableValuesOntoOrangeBlock();
	void TestBorderOwnerModelMoveUndo();
	void TestCopyBorderRangePasteUndo();
	void TestCopyBorderSingleCellPasteUndo();
	void TestCopyBorderGridPastePreservesOuter();
	void TestCopyGridPastePreservesOuterFillAfterLocalDo();
	void TestUndoMoveBorderGhostSeamsAfterMove();
	void TestUndoMoveUpLeftClearsSourceFillGhosts();
	void TestUndoMoveUpLeftPreservesOuterBorders();
	void TestUndoMoveOverlapUpLeftPreservesOuterBorders();
	void TestUndoMove();
	void TestUndoCut();
	void TestUndoCutCollaborativeUndoJson();
	void TestUndoMoveEmptySourcePreservesDestFormat();
	void TestUndoMoveBorderShiftLeft();
	void TestUndoMoveShiftDown();
	void TestUndoMoveFormulaRefs();
	void TestUndoMoveRebaseAfterInsertRowByRect();
#ifdef TestMultiUser
	void TestUndoPasteInterfaceWebMonoUser();
	void TestUndoPasteCollaborationPayloadTooLarge();
	void TestUndoMoveInterfaceWebMonoUser();
	void TestUndoCutInterfaceWebMonoUser();
#endif
public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkCopyPaste */

//==============================================================================
// TestSkSheet
// Test la librairie SparseArray
//==============================================================================

#include "../include/TestSkTree.hpp"
#include "../include/SkSpreadSheet.hpp"

#define _printdebug
// We can send it to the API of a feature
TestSkTree::TestSkTree() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
	m_Api = nullptr;
}

tString TestSkTree::DebugColRow(tColRowCellRange* sColRowCellRange,tIndex sIndex,tBool sIsRow) {
    tStringStream wStream;
    tColRow* wColRow=sColRowCellRange->ColRowByIndex(sIndex, sIsRow);
    if (wColRow==nullptr) return("nullptr");
    wStream << "Index=" << wColRow->Index();
    tColRow* wColRowParent=sColRowCellRange->ColRowByAllocatorRef(wColRow->ParentRef(), sIsRow);
    if (wColRowParent!=nullptr) wStream << " Parent=" << wColRowParent->Index();
    tColRow::tVectorColRow*  wChildren=wColRow->VectorChildren();
    if (!wChildren->empty()) {
        wStream << " {";
        tBool wIsFirst=false;
        for (auto wChildRef : *wChildren) {
            tColRow* wChild=sColRowCellRange->ColRowByAllocatorRef(wChildRef, sIsRow);
            if (wChild==nullptr) continue;
            if (wIsFirst) wStream << ",";
            wStream << wChild->Index();
            wIsFirst=true;
        }
        wStream << "}";
    }
    return(wStream.str());
}

void TestSkTree::AssertColRow(tColRowCellRange* sColRowCellRange,tIndex sIndex,tBool sIsRow,
                              const tString& sExpected,const tString& sMessage) {
    tString wResult=DebugColRow(sColRowCellRange, sIndex, sIsRow);
    CPPUNIT_ASSERT_MESSAGE(sMessage + " got=[" + wResult + "] expected=[" + sExpected + "]",
                           wResult == sExpected);
}

void TestSkTree::AssertTreeLinks(tColRowCellRange* sColRowCellRange,tBool sIsRow,
                                 tIndex sBegin,tIndex sEnd,const tString& sMessage) {
    for (tIndex wIndex=sBegin; wIndex<=sEnd; wIndex++) {
        tColRow* wColRow=sColRowCellRange->ColRowByIndex(wIndex, sIsRow);
        if (wColRow==nullptr) continue;
        tAllocatorRef wSelfRef=sIsRow
            ? sColRowCellRange->RowAllocatorRef(wIndex)
            : sColRowCellRange->ColAllocatorRef(wIndex);
        tStringStream wAt;
        wAt << " at " << wIndex;
        if (wColRow->ParentRef()!=0) {
            tColRow* wParent=sColRowCellRange->ColRowByAllocatorRef(wColRow->ParentRef(), sIsRow);
            CPPUNIT_ASSERT_MESSAGE(sMessage + " dangling ParentRef" + wAt.str(),
                                   wParent!=nullptr);
            CPPUNIT_ASSERT_MESSAGE(sMessage + " child not in parent list" + wAt.str(),
                                   wParent->IsChildrenExist(wSelfRef));
        }
        for (auto wChildRef : *wColRow->VectorChildren()) {
            tColRow* wChild=sColRowCellRange->ColRowByAllocatorRef(wChildRef, sIsRow);
            CPPUNIT_ASSERT_MESSAGE(sMessage + " dangling child ref under" + wAt.str(),
                                   wChild!=nullptr);
            CPPUNIT_ASSERT_MESSAGE(sMessage + " child ParentRef mismatch under" + wAt.str(),
                                   wChild->ParentRef()==wSelfRef);
        }
    }
}

void TestSkTree::DebugTree(tBool sIsRow,tString sTitle,tIndex sBegin,tIndex sEnd) {
#ifdef printdebug
    cout << sTitle << endl;
    tSheet* wSheet=m_Api->ActiveSheet();
    tColRowCellRange* wColRowCellRange=wSheet->ColRowCellRange();
    for(tIndex wIndex=sBegin;wIndex<=sEnd;wIndex++) {
        tColRow* wColRow=  wColRowCellRange->ColRowByIndex(wIndex,sIsRow);
        cout << wIndex << ":";
        if (wColRow!=nullptr) {
            tInt wDeep=wColRow->Deep(wSheet->ColRowCellRange(),sIsRow);
            for(tInt wInd=0; wInd<wDeep;wInd++) cout << ".";
            if (wColRow->Open()) {
                cout << "O";
            } else {
                cout << "C";
            }
            if (wColRow->ParentRef()!=0) {
                tColRow* wParent= wColRowCellRange->ColRowByAllocatorRef(wColRow->ParentRef(), sIsRow);
                cout << ":" << wParent->Index();
            }
        } else {
            cout << " nullptr";
        }
        
        /*
        if (wColRow!=nullptr) {
            cout << wColRow->Debug();
        } else {
            cout << "Index=" << wIndex << ":" << "nullptr";
        }
         */
        cout << endl;
    }
#endif
}


void TestSkTree::TestTreeRow() {
    tString wTitle="Tree Row";
    tSheet* wSheet=m_Api->ActiveSheet();
    tColRowCellRange* wColRowCellRange=wSheet->ColRowCellRange();
    
    m_Api->UndoChangeTreeRow(true, 2, 10);
    
    tString wResult=DebugColRow(wColRowCellRange,2,true);
    
    CPPUNIT_ASSERT_MESSAGE(wTitle+" true,2,10 ", wResult  == "Index=2 {3,4,5,6,7,8,9,10,11}");
    
    

    DebugTree(true,"UndoChangeTreeRow(true, 2, 10)",2,11);
    m_Api->UndoChangeTreeRow(false, 3, 1);
    DebugTree(true,"UndoChangeTreeRow(true, 3, 10)",2,11);

    
    m_Api->Undo();
    DebugTree(true,"Undo",1,11);

    m_Api->Undo();
    tColRow* wColRow=wSheet->Row(2);
    CPPUNIT_ASSERT_MESSAGE(wTitle+" Undo true,2,10 ", wColRow==nullptr);
   
    m_Api->Redo();
    wResult=DebugColRow(wColRowCellRange,2,true);
    
    CPPUNIT_ASSERT_MESSAGE(wTitle+" redo true,2,10 ", wResult  == "Index=2 {3,4,5,6,7,8,9,10,11}");
    
    m_Api->UndoChangeTreeRow(true, 4, 10);
    
    wResult=DebugColRow(wColRowCellRange,2,true);
    CPPUNIT_ASSERT_MESSAGE(wTitle+" true,4,10 ", wResult  == "Index=2 {3,4}");
 
    
    wResult=DebugColRow(wColRowCellRange,4,true);
    CPPUNIT_ASSERT_MESSAGE(wTitle+" true,4,10 ", wResult  == "Index=4 Parent=2 {5,6,7,8,9,10,11}");
    
    
    DebugTree(true,"UndoChangeTreeRow(true, 2, 10)",2,12);
    
    m_Api->Undo();
    wResult=DebugColRow(wColRowCellRange,2,true);
    
    CPPUNIT_ASSERT_MESSAGE(wTitle+" Undo true 4,10 ", wResult  == "Index=2 {3,4,5,6,7,8,9,10,11}");
    
    DebugTree(true,"Undo",2,12);
    m_Api->Redo();
    AssertColRow(wColRowCellRange, 2, true, "Index=2 {3,4}", wTitle+" after redo nest");
    AssertColRow(wColRowCellRange, 4, true, "Index=4 Parent=2 {5,6,7,8,9,10,11}", wTitle+" after redo nest child");
    AssertTreeLinks(wColRowCellRange, true, 2, 11, wTitle+" links after nest");
    
// Test Delete =================================================================
    DebugTree(true,"Redo",2,12);
    // Delete nested parent 4 and its children 5..11
    m_Api->UndoDeleteRow(4, 8);
    DebugTree(true,"UndoDeleteRow(4, 8)",2,11);
    AssertColRow(wColRowCellRange, 2, true, "Index=2 {3}", wTitle+" after delete nested block");
    AssertTreeLinks(wColRowCellRange, true, 1, 5, wTitle+" links after delete nested block");

    m_Api->Undo();
    AssertColRow(wColRowCellRange, 2, true, "Index=2 {3,4}", wTitle+" undo delete nested block");
    AssertColRow(wColRowCellRange, 4, true, "Index=4 Parent=2 {5,6,7,8,9,10,11}", wTitle+" undo delete nested child");
    AssertTreeLinks(wColRowCellRange, true, 2, 11, wTitle+" links after undo delete nested");
    
    DebugTree(true,"Undo",2,11);
    
    m_Api->UndoDeleteRow(2, 15);
    DebugTree(true,"UndoDeleteRow(2, 15)",2,11);
    CPPUNIT_ASSERT_MESSAGE(wTitle+" after delete root tree Row(2) gone or vacant",
                           wSheet->Row(2)==nullptr || wSheet->Row(2)->ParentRef()==0);
    
    m_Api->Undo();
    AssertColRow(wColRowCellRange, 2, true, "Index=2 {3,4}", wTitle+" undo delete root tree");
    AssertColRow(wColRowCellRange, 4, true, "Index=4 Parent=2 {5,6,7,8,9,10,11}", wTitle+" undo delete root nested");
    AssertTreeLinks(wColRowCellRange, true, 2, 11, wTitle+" links after undo delete root");
    
    DebugTree(true,"Undo",2,11);
// Test Insert ================================================================
    m_Api->UndoInsertRow(6, 1);
    DebugTree(true,"UndoInsertRow(6, 1)",1,17);
    // New row 6 inherits parent of former row 6 (=4); children of 4 grow by one
    AssertColRow(wColRowCellRange, 2, true, "Index=2 {3,4}", wTitle+" after insert keep root");
    AssertColRow(wColRowCellRange, 4, true, "Index=4 Parent=2 {5,6,7,8,9,10,11,12}", wTitle+" after insert under 4");
    AssertColRow(wColRowCellRange, 6, true, "Index=6 Parent=4", wTitle+" inserted row parent");
    AssertTreeLinks(wColRowCellRange, true, 2, 12, wTitle+" links after insert");

    m_Api->Undo();
    AssertColRow(wColRowCellRange, 4, true, "Index=4 Parent=2 {5,6,7,8,9,10,11}", wTitle+" undo insert");
    AssertTreeLinks(wColRowCellRange, true, 2, 11, wTitle+" links after undo insert");
    
    DebugTree(true,"Undo",0,12);
    m_Api->UndoChangeTreeRow(false, 6, 10);
    DebugTree(true,"UndoChangeTreeRow(false, 6, 10)",2,10);
    // Outdent: all children of 4 move under 2
    AssertColRow(wColRowCellRange, 2, true, "Index=2 {3,4,5,6,7,8,9,10,11}", wTitle+" after tree left");
    AssertColRow(wColRowCellRange, 4, true, "Index=4 Parent=2", wTitle+" after tree left node 4 empty");
    AssertTreeLinks(wColRowCellRange, true, 2, 11, wTitle+" links after tree left");
    m_Api->Undo();
    AssertColRow(wColRowCellRange, 2, true, "Index=2 {3,4}", wTitle+" undo tree left");
    AssertColRow(wColRowCellRange, 4, true, "Index=4 Parent=2 {5,6,7,8,9,10,11}", wTitle+" undo tree left nested");
    DebugTree(true,"Undo",2,10);
}



void TestSkTree::TestTreeCol() {
    tString wTitle="Tree Col";
    
    m_Api->UndoChangeTreeCol(true, 2, 10);
    tSheet* wSheet=m_Api->ActiveSheet();
    tColRowCellRange* wColRowCellRange=wSheet->ColRowCellRange();
    
    tString wResult=DebugColRow(wColRowCellRange,2,false);
    
    CPPUNIT_ASSERT_MESSAGE(wTitle+" true,2,10 ", wResult  == "Index=2 {3,4,5,6,7,8,9,10,11}");
    
    
    DebugTree(false,"UndoChangeTreeRow(true, 2, 10)",2,10);
    m_Api->UndoChangeTreeCol(false, 3, 1);
    DebugTree(false,"UndoChangeTreeRow(true, 3, 10)",2,10);
    
    m_Api->Undo();
    DebugTree(false,"Undo",2,10);

    m_Api->Undo();
    tColRow* wColRow=wSheet->Col(2);
    CPPUNIT_ASSERT_MESSAGE(wTitle+" Undo true,2,10 ", wColRow==nullptr);
   
    m_Api->Redo();
    wResult=DebugColRow(wColRowCellRange,2,false);
    CPPUNIT_ASSERT_MESSAGE(wTitle+" redo true,2,10 ", wResult  == "Index=2 {3,4,5,6,7,8,9,10,11}");
    
    m_Api->UndoChangeTreeCol(true, 4, 10);
    wResult=DebugColRow(wColRowCellRange,2,false);

    CPPUNIT_ASSERT_MESSAGE(wTitle+" true,4,10 ", wResult  == "Index=2 {3,4}");
 
    
    wResult=DebugColRow(wColRowCellRange,4,false);
    CPPUNIT_ASSERT_MESSAGE(wTitle+" true,4,10 ", wResult  == "Index=4 Parent=2 {5,6,7,8,9,10,11}");
        
    DebugTree(false,"UndoChangeTreeRow(true, 2, 10)",2,12);
    
    m_Api->Undo();
    wResult=DebugColRow(wColRowCellRange,2,false);
    CPPUNIT_ASSERT_MESSAGE(wTitle+" Undo true 4,10 ", wResult  == "Index=2 {3,4,5,6,7,8,9,10,11}");
    
    DebugTree(false,"Undo",2,12);
    m_Api->Redo();
    AssertColRow(wColRowCellRange, 2, false, "Index=2 {3,4}", wTitle+" after redo nest");
    AssertColRow(wColRowCellRange, 4, false, "Index=4 Parent=2 {5,6,7,8,9,10,11}", wTitle+" after redo nest child");
    AssertTreeLinks(wColRowCellRange, false, 2, 11, wTitle+" links after nest");
    
    DebugTree(false,"Redo",2,12);
    
// Test Delete =================================================================
    DebugTree(false,"Redo",2,12);
    // Delete two middle children of 4 (cols 6 and 7)
    m_Api->UndoDeleteCol(6,2);
    DebugTree(false,"UndoDeleteCol(6, 2)",2,12);
    AssertColRow(wColRowCellRange, 2, false, "Index=2 {3,4}", wTitle+" after delete keep root");
    AssertColRow(wColRowCellRange, 4, false, "Index=4 Parent=2 {5,6,7,8,9}", wTitle+" after delete two children");
    AssertTreeLinks(wColRowCellRange, false, 2, 9, wTitle+" links after delete children");

    m_Api->Undo();
    AssertColRow(wColRowCellRange, 4, false, "Index=4 Parent=2 {5,6,7,8,9,10,11}", wTitle+" undo delete children");
    AssertTreeLinks(wColRowCellRange, false, 2, 11, wTitle+" links after undo delete children");
    
    DebugTree(false,"Undo",2,11);
// Test Insert ================================================================
    m_Api->UndoInsertCol(6, 1);
    DebugTree(false,"UndoInsertCol(6, 1)",1,17);
    AssertColRow(wColRowCellRange, 2, false, "Index=2 {3,4}", wTitle+" after insert keep root");
    AssertColRow(wColRowCellRange, 4, false, "Index=4 Parent=2 {5,6,7,8,9,10,11,12}", wTitle+" after insert under 4");
    AssertColRow(wColRowCellRange, 6, false, "Index=6 Parent=4", wTitle+" inserted col parent");
    AssertTreeLinks(wColRowCellRange, false, 2, 12, wTitle+" links after insert");

    m_Api->Undo();
    AssertColRow(wColRowCellRange, 4, false, "Index=4 Parent=2 {5,6,7,8,9,10,11}", wTitle+" undo insert");
    AssertTreeLinks(wColRowCellRange, false, 2, 11, wTitle+" links after undo insert");
    DebugTree(false,"Undo",0,11);
    
    m_Api->UndoChangeTreeCol(false, 6, 10);
    DebugTree(false,"UndoChangeTreeCol(false, 6, 10)",2,10);
    AssertColRow(wColRowCellRange, 2, false, "Index=2 {3,4,5,6,7,8,9,10,11}", wTitle+" after tree left");
    AssertColRow(wColRowCellRange, 4, false, "Index=4 Parent=2", wTitle+" after tree left node 4 empty");
    AssertTreeLinks(wColRowCellRange, false, 2, 11, wTitle+" links after tree left");
    m_Api->Undo();
    AssertColRow(wColRowCellRange, 2, false, "Index=2 {3,4}", wTitle+" undo tree left");
    AssertColRow(wColRowCellRange, 4, false, "Index=4 Parent=2 {5,6,7,8,9,10,11}", wTitle+" undo tree left nested");
    DebugTree(false,"Undo",2,10);
    
    // Delete parent node 4 alone: children reparent to 2
    m_Api->UndoDeleteCol(4, 1);
    AssertColRow(wColRowCellRange, 2, false, "Index=2 {3,4,5,6,7,8,9,10}", wTitle+" after delete parent 4 reparent");
    AssertTreeLinks(wColRowCellRange, false, 2, 10, wTitle+" links after delete parent 4");
    m_Api->Undo();
    AssertColRow(wColRowCellRange, 2, false, "Index=2 {3,4}", wTitle+" undo delete parent 4");
    AssertColRow(wColRowCellRange, 4, false, "Index=4 Parent=2 {5,6,7,8,9,10,11}", wTitle+" undo delete parent 4 nested");
    DebugTree(false,"Undo",1,10);
}


void TestSkTree::TestTreeInsertDeleteRow() {
    tString wTitle="Tree InsertDelete Row";
    tSheet* wSheet=m_Api->ActiveSheet();
    tColRowCellRange* wGrid=wSheet->ColRowCellRange();

    // 2 parent of {3,4,5,6}
    CPPUNIT_ASSERT(m_Api->UndoChangeTreeRow(true, 2, 5));
    AssertColRow(wGrid, 2, true, "Index=2 {3,4,5,6}", wTitle+" build");
    AssertTreeLinks(wGrid, true, 2, 6, wTitle+" links build");

    // Insert inside the group: new row becomes sibling under 2
    CPPUNIT_ASSERT(m_Api->UndoInsertRow(4, 1));
    AssertColRow(wGrid, 2, true, "Index=2 {3,4,5,6,7}", wTitle+" insert sibling");
    AssertColRow(wGrid, 4, true, "Index=4 Parent=2", wTitle+" inserted row");
    AssertColRow(wGrid, 5, true, "Index=5 Parent=2", wTitle+" shifted former 4");
    AssertTreeLinks(wGrid, true, 2, 7, wTitle+" links after insert");

    CPPUNIT_ASSERT(m_Api->Undo());
    AssertColRow(wGrid, 2, true, "Index=2 {3,4,5,6}", wTitle+" undo insert");

    // Delete parent only: children reparent to root and shift up
    CPPUNIT_ASSERT(m_Api->UndoDeleteRow(2, 1));
    AssertColRow(wGrid, 2, true, "Index=2", wTitle+" after delete parent former 3 is root");
    AssertColRow(wGrid, 3, true, "Index=3", wTitle+" after delete parent former 4 is root");
    AssertTreeLinks(wGrid, true, 1, 6, wTitle+" links after delete parent");

    CPPUNIT_ASSERT(m_Api->Undo());
    AssertColRow(wGrid, 2, true, "Index=2 {3,4,5,6}", wTitle+" undo delete parent");
    AssertTreeLinks(wGrid, true, 2, 6, wTitle+" links after undo delete parent");

    // Delete a middle child: parent children list shrinks and renums
    CPPUNIT_ASSERT(m_Api->UndoDeleteRow(4, 1));
    AssertColRow(wGrid, 2, true, "Index=2 {3,4,5}", wTitle+" after delete middle child");
    AssertColRow(wGrid, 4, true, "Index=4 Parent=2", wTitle+" former 5 shifted");
    AssertTreeLinks(wGrid, true, 2, 5, wTitle+" links after delete middle");
}

void TestSkTree::TestTreeInsertDeleteCol() {
    tString wTitle="Tree InsertDelete Col";
    tSheet* wSheet=m_Api->ActiveSheet();
    tColRowCellRange* wGrid=wSheet->ColRowCellRange();

    CPPUNIT_ASSERT(m_Api->UndoChangeTreeCol(true, 2, 5));
    AssertColRow(wGrid, 2, false, "Index=2 {3,4,5,6}", wTitle+" build");
    AssertTreeLinks(wGrid, false, 2, 6, wTitle+" links build");

    CPPUNIT_ASSERT(m_Api->UndoInsertCol(4, 1));
    AssertColRow(wGrid, 2, false, "Index=2 {3,4,5,6,7}", wTitle+" insert sibling");
    AssertColRow(wGrid, 4, false, "Index=4 Parent=2", wTitle+" inserted col");
    AssertTreeLinks(wGrid, false, 2, 7, wTitle+" links after insert");

    CPPUNIT_ASSERT(m_Api->Undo());
    AssertColRow(wGrid, 2, false, "Index=2 {3,4,5,6}", wTitle+" undo insert");

    CPPUNIT_ASSERT(m_Api->UndoDeleteCol(2, 1));
    AssertColRow(wGrid, 2, false, "Index=2", wTitle+" after delete parent former 3 is root");
    AssertTreeLinks(wGrid, false, 1, 6, wTitle+" links after delete parent");

    CPPUNIT_ASSERT(m_Api->Undo());
    AssertColRow(wGrid, 2, false, "Index=2 {3,4,5,6}", wTitle+" undo delete parent");

    CPPUNIT_ASSERT(m_Api->UndoDeleteCol(4, 1));
    AssertColRow(wGrid, 2, false, "Index=2 {3,4,5}", wTitle+" after delete middle child");
    AssertTreeLinks(wGrid, false, 2, 5, wTitle+" links after delete middle");
}

void TestSkTree::TestTreeChangeUndoNoHang() {
    tString wTitle="Tree ChangeUndo NoHang";
    tSheet* wSheet=m_Api->ActiveSheet();
    tColRowCellRange* wGrid=wSheet->ColRowCellRange();

    CPPUNIT_ASSERT(m_Api->UndoChangeTreeRow(true, 2, 6));
    AssertColRow(wGrid, 2, true, "Index=2 {3,4,5,6,7}", wTitle+" build");

    // Selection includes existing parent 2 → skip it, nest 4..7 under 3.
    CPPUNIT_ASSERT(m_Api->UndoChangeTreeRow(true, 2, 6));
    AssertColRow(wGrid, 2, true, "Index=2 {3}", wTitle+" skip-parent");
    AssertColRow(wGrid, 3, true, "Index=3 Parent=2 {4,5,6,7}", wTitle+" nested under 3");
    AssertTreeLinks(wGrid, true, 2, 7, wTitle+" links after nest");

    // Ungroup a root parent: promote its children to root.
    CPPUNIT_ASSERT(m_Api->Undo()); // back to 2→{3..7}
    CPPUNIT_ASSERT(m_Api->UndoChangeTreeRow(false, 2, 1));
    AssertColRow(wGrid, 2, true, "Index=2", wTitle+" root parent ungrouped");
    AssertColRow(wGrid, 3, true, "Index=3", wTitle+" former child is root");
    AssertTreeLinks(wGrid, true, 2, 7, wTitle+" links after ungroup");

    for (tIndex wIndex=2; wIndex<=7; wIndex++) {
        tColRow* wColRow=wGrid->Row(wIndex);
        if (wColRow==nullptr) continue;
        (void)wColRow->Deep(wGrid, true);
        (void)wColRow->IsInClosedPath(wGrid, true);
        (void)wColRow->SearchNextOpen(wGrid, true);
    }
    tString wView=m_Api->JsonView(1, 1, tUnitMetrics::pixels, 300, 300, -10, -2, true);
    CPPUNIT_ASSERT_MESSAGE(wTitle+" JsonView not empty", !wView.empty());
}

void TestSkTree::TestTreeOpenCloseRow1() {
    tString wTitle="Tree OpenClose Row1";
    tSheet* wSheet=m_Api->ActiveSheet();
    tColRowCellRange* wGrid=wSheet->ColRowCellRange();

    // Nest rows 2..8 under row 1 (selection size 8 = parent + 7 children).
    CPPUNIT_ASSERT(m_Api->UndoChangeTreeRow(true, 1, 8));
    AssertColRow(wGrid, 1, true, "Index=1 {2,3,4,5,6,7,8}", wTitle+" build");
    tColRow* wRow1=wGrid->Row(1);
    CPPUNIT_ASSERT(wRow1 != nullptr && wRow1->Open());

    // Close
    CPPUNIT_ASSERT(m_Api->UndoOpenCloseTreeRow(1));
    CPPUNIT_ASSERT_MESSAGE(wTitle+" closed", !wRow1->Open());
    tIndex wNextClosed=wRow1->SearchNextOpen(wGrid, true);
    CPPUNIT_ASSERT_MESSAGE(wTitle+" skip subtree when closed", wNextClosed == 9);

    // Re-open
    CPPUNIT_ASSERT(m_Api->UndoOpenCloseTreeRow(1));
    CPPUNIT_ASSERT_MESSAGE(wTitle+" reopened", wRow1->Open());
    tIndex wNextOpen=wRow1->SearchNextOpen(wGrid, true);
    CPPUNIT_ASSERT_MESSAGE(wTitle+" next is child 2 when open", wNextOpen == 2);

    // Idempotent second Do with absolute target would keep open; toggle Undo/Redo cycle.
    CPPUNIT_ASSERT(m_Api->Undo());
    CPPUNIT_ASSERT_MESSAGE(wTitle+" undo reopen → closed", !wRow1->Open());
    CPPUNIT_ASSERT(m_Api->Redo());
    CPPUNIT_ASSERT_MESSAGE(wTitle+" redo → open", wRow1->Open());

    // Simulate JSON parent "c" without child "p": clear ParentRefs, keep m_Children.
    CPPUNIT_ASSERT(m_Api->UndoOpenCloseTreeRow(1)); // close again
    for (tIndex wIndex = 2; wIndex <= 8; wIndex++) {
        tColRow* wChild = wGrid->Row(wIndex);
        CPPUNIT_ASSERT(wChild != nullptr);
        wChild->ParentRef(0);
    }
    CPPUNIT_ASSERT_MESSAGE(wTitle+" children list still present", !wRow1->VectorChildren()->empty());
    // OpenClose must repair ParentRef via SyncTreeParentRefsFromChildren before Renum.
    CPPUNIT_ASSERT(m_Api->UndoOpenCloseTreeRow(1));
    CPPUNIT_ASSERT_MESSAGE(wTitle+" reopen after missing ParentRef", wRow1->Open());
    AssertColRow(wGrid, 1, true, "Index=1 {2,3,4,5,6,7,8}", wTitle+" links repaired");
    AssertTreeLinks(wGrid, true, 1, 8, wTitle+" links after ParentRef repair");
}

void TestSkTree::TestJsonViewCol() {
    m_Api->CellValue("A1", "A1");
    m_Api->CellValue("D1","D1");
    
    m_Api->CellValue("H1", "H1");
    
    tString wResult=m_Api->JsonColByPixel(1, 20);
    //cout << endl << "ColByPixel=" << wResult << endl;;
    //cout << "Row A=" << m_Api->ActiveSheet()->Row(1)->Size() << endl;
    
    for(int i=1;i<100;i++) {
        wResult=m_Api->JsonColByPixel(1, i);
        //cout << "ColByPixel(" << i << ")=" << wResult << endl;;
    }

    wResult=m_Api->JsonView(1, 3, tUnitMetrics::pixels, 12, 300,-10,-2, true);
    
    //cout << endl << wResult << endl;
    
    Document wDocument;
    wDocument.Parse(wResult.c_str());
    
    Value& wName=wDocument["sheet"];
    //cout << wName.GetString() << endl;
    CPPUNIT_ASSERT_MESSAGE("TestJsonView sheet", tString(wName.GetString())  == "Sheet1");
    
    Value& wRows=wDocument["rows"];
    CPPUNIT_ASSERT_MESSAGE("TestJsonView rows", wRows.IsArray());
    
    Value& wRow=wRows[0];
    Value& wCells=wRow["cells"];
    CPPUNIT_ASSERT_MESSAGE("TestJsonView cells", wCells.IsArray());
    
}

void TestSkTree::TestJsonViewRow() {
    m_Api->CellValue("A6", "A7");
    m_Api->CellValue("D7","D7");
    
    m_Api->CellValue("H7", "H7");
    
    
    m_Api->UndoChangeTreeRow(true, 2, 10);
    m_Api->UndoChangeTreeRow(true, 4, 8);
    m_Api->UndoChangeTreeRow(true, 5, 3);
    m_Api->UndoChangeTreeRow(true, 8, 3);
    
    m_Api->UndoOpenCloseTreeRow(5);
    m_Api->UndoOpenCloseTreeRow(8);
    DebugTree(true,"TestJsonViewRow",1,12);
    
    tSheet* wSheet=m_Api->ActiveSheet();
    tColRow* wColRow=wSheet->Row(10);
    for(int i=0; i<5;i++) {
        tIndex wIndex=wColRow->SearchPrecOpen(wSheet->ColRowCellRange(), true);
#ifdef printdebug
        cout << "Result=" <<  wIndex << endl;
#endif
        wColRow=wSheet->Row(wIndex);
    }
    for(int i=0; i<5;i++) {
        tIndex wIndex=wColRow->SearchNextOpen(wSheet->ColRowCellRange(), true);
#ifdef printdebug
        cout << "Result=" <<  wIndex << endl;
#endif
        wColRow=wSheet->Row(wIndex);
    }
    
    tString wResultJson=m_Api->JsonView(1, 1, tUnitMetrics::pixels, 300, 300,-10,-2, true);
#ifdef printdebug
    cout << endl << "View=" << wResultJson << endl;;
#endif
    tString wResult=m_Api->JsonRowByPixel(1, 20);
#ifdef printdebug
    cout << endl << "ColByPixel=" << wResult << endl;
#endif
    
    // Size ==================================================================
#ifdef printdebug
    cout << endl << "Metrics 1" << m_Api->SumPixelWidth(1, 1) <<  endl;
    cout  << "Metrics 1..10" << m_Api->SumPixelWidth(1, 10) <<  endl;
#endif
    // Cursor ================================================================
    tPoint wPoint(5,10);
    for(tInt wInd=0;wInd<5; wInd++) {
        tRect wMoveRect=m_Api->MoveCell(wPoint, tKey::t_Down, 0,tRect(0,0,0,0));
#ifdef printdebug
        cout << "Down " << wMoveRect.StrRef() << endl;
#endif
        wPoint.Row(wMoveRect.Row()); wPoint.Col(wMoveRect.Col());
    }
  
    for(tInt wInd=0;wInd<5; wInd++) {
        tRect wMoveRect=m_Api->MoveCell(wPoint, tKey::t_Up, 0,tRect(0,0,0,0));
#ifdef printdebug
        cout << "Up " << wMoveRect.StrRef() << endl;
#endif
        wPoint.Row(wMoveRect.Row()); wPoint.Col(wMoveRect.Col());
    }
    
    
    //cout << "Row A=" << m_Api->ActiveSheet()->Row(1)->Size() << endl;
    
    for(int i=1;i<100;i++) {
        wResult=m_Api->JsonRowByPixel(1, i);
        //cout << "RowByPixel(" << i << ")=" << wResult << endl;;
    }
}
void TestSkTree::TestWeb() {
    //m_Api->UndoChangeTreeCol(true, 2, 150);
    //m_Api->UndoChangeTreeCol(true, 4, 7);
    //m_Api->UndoChangeTreeCol(true, 15, 5);
    //m_Api->UndoChangeTreeCol(true, 16, 2);
    //m_Api->UndoChangeTreeCol(true, 20, 20);
    
    m_Api->UndoChangeTreeCol(true, 2, 15);
    m_Api->UndoChangeTreeCol(true, 4, 7);
    m_Api->UndoChangeTreeCol(true, 15, 2);
    DebugTree(false,"TestWeb )",2,16);
    m_Api->UndoDeleteCol(7, 1);
    DebugTree(false,"TestWeb )",2,16);
    
    m_Api->Undo();
    DebugTree(false,"TestWeb )",2,16);
}

void TestSkTree::setUp() {
	std::filesystem::remove_all("./Spreadsheet");

	m_Application = tApplication::Instance();
	m_Api = new tApi;
	m_Api->IsUndoActif(true);
	m_Api->NewWorkBook("wwww.skeema.fr/w1");
};

void TestSkTree::tearDown() {
	delete(m_Api);
}

//
//  Untitled.hpp
//  SkSpreadSheet_test
//
//  Created by stephane allez on 16/06/2025.
//

#ifndef TestSkInterface_hpp
#define TestSkInterface_hpp

#include <stdio.h>


#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/Exception.h>
#include <SkApplication.hpp>
#include <SkApi.hpp>
#include <SkInterfaceWeb.hpp>
#include <SkUndoRedoRebaseFormula.hpp>
using namespace SkRoot;
using namespace SkSpreadSheet;

#ifdef TestMultiUser

#define TestAllInterface

class TestSkInterface : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkInterface);
#ifndef TestAllInterface
    CPPUNIT_TEST(TestSkInterfaceRebaseMultipleSheet);
#endif
#ifdef TestAllInterface
    CPPUNIT_TEST(TestSkInterfaceData);
    CPPUNIT_TEST(TestSkInterfaceRebaseFormula);
    CPPUNIT_TEST(TestSkInterfaceValue);
    CPPUNIT_TEST(TestSkInsertDeleteColRowWithCoveredRangeUndo);
    CPPUNIT_TEST(TestSkInterfaceRaz);
    CPPUNIT_TEST(TestSkInterfaceCopy);
    CPPUNIT_TEST(TestSkInterfaceClasss);
    CPPUNIT_TEST(TestSkInterfaceNameRange);
    CPPUNIT_TEST(TestSkInterfaceSheet);
    CPPUNIT_TEST(TestSkInterfaceConditionalFormat);
    CPPUNIT_TEST(TestSkInterfaceRebaseInsertDeleteColRow);
#ifndef __EMSCRIPTEN__MEMORY__
    CPPUNIT_TEST(TestSkInterfaceRebaseInsertDeleteRectRowCol);
    CPPUNIT_TEST(TestSkInterfaceRebaseCellValueUndoAfterInsertRowByRect);
    CPPUNIT_TEST(TestSkInterfaceRebaseMoveUndoAfterInsertRowByRect);
    CPPUNIT_TEST(TestSkInterfaceRedoInsertColRowByRect);
    CPPUNIT_TEST(TestSkInterfaceRedoDeleteColRowByRect);
#endif
    CPPUNIT_TEST(TestSkInterfaceRebaseMerge);
    CPPUNIT_TEST(TestSkInterfaceRebaseMultipleSheet);
    CPPUNIT_TEST(TestSkInterfaceData);
#endif
    CPPUNIT_TEST_SUITE_END();

private:
    tApplication* m_Application;
    tInterfaceWeb* m_Interface;
    
    
   // Number of columns and rows
   tInt m_NbCol;
   tInt m_NbRow;

    
    // User Str
    const tString m_UserServerStr="Server";
    const tString m_User1Str="User1";
    const tString m_User2Str="User2";
    const tString m_WorkBookUri="www.skeema.fr/";
    // User
    tTestUser* m_UserServer;
    tTestUser* m_User1;
    tTestUser* m_User2;


    // Interface
    tInterfaceWeb* m_InterfaceServer;
    tInterfaceWeb* m_InterfaceUser1;
    tInterfaceWeb* m_InterfaceUser2;

    void DrawCell(tInterfaceWeb* sInterface,tString sOperation, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);

    
    void DebugRange(tInterfaceWeb* sInterface);
    void Fill(tInterfaceWeb* sInterface);
    
    void CreateGenericClass(tInterfaceWeb* sInterface);
    
    void InitInterface();
    void DoneInterface();
    
    void AssertRebase(tString sLabel,tString sRef,tVariant& sValue);
    
    void DrawSheet(tString sSheet, tInterfaceWeb* sInterface,tString sOperation, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
    
    void TestSkInterfaceRebaseFormula();
    void TestSkInterfaceValue();
    void TestSkInterfaceInsertDeleteColRow();
    void TestSkInterfaceRebaseInsertDeleteRectRowCol();
    void TestSkInterfaceRebaseCellValueUndoAfterInsertRowByRect();
    void TestSkInterfaceRebaseMoveUndoAfterInsertRowByRect();
    void TestSkInterfaceRedoInsertColRowByRect();
    void TestSkInterfaceRedoDeleteColRowByRect();
    void TestSkInsertDeleteColRowWithCoveredRangeUndo();
    void TestSkInterfaceRaz();
    void TestSkInterfaceCopy();
    void TestSkInterfaceClasss();
    void TestSkInterfaceNameRange();
    void TestSkInterfaceSheet();
    void TestSkInterfaceConditionalFormat();
    void TestSkInterfaceRebaseInsertDeleteColRow();
    void TestSkInterfaceRebaseMerge();
    void TestSkInterfaceRebaseMultipleSheet();
    void TestSkInterfaceData();
public:
    virtual void setUp();
    virtual void tearDown();
};
#endif

#endif /* TestSkInterface_hpp */


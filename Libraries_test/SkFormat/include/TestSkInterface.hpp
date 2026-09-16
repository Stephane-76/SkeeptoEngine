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

#include <SkFormatApi.hpp>
#include <SkFormatRoot.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;
using namespace SkFormat;

#ifdef TestMultiUser

#define TestAllInterface

class TestSkInterface : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkInterface);
#ifndef TestAllInterface
    CPPUNIT_TEST(TestSkInterfaceRebaseMultipleSheet);
#endif
#ifdef TestAllInterface
    CPPUNIT_TEST(TestSkInterfaceUndoJson);
    CPPUNIT_TEST(TestSkInterfaceFormatString);
    CPPUNIT_TEST(TestSkInterfaceDeleteRow);
    CPPUNIT_TEST(TestSkInterfaceRaz);
    CPPUNIT_TEST(TestSkInterfaceCopy);
    CPPUNIT_TEST(TestSkInterfaceRebaseConditionalFormat);
    CPPUNIT_TEST(TestSkInterfaceBudget);
    CPPUNIT_TEST(TestSkInterfaceBudgetCopyPaste);
    CPPUNIT_TEST(TestSkInterfaceBudgetInsertCopyPaste);
    CPPUNIT_TEST(TestSkInterfaceDeleteSheet);
#endif
    CPPUNIT_TEST_SUITE_END();

private:
    tApplication* m_Application;
    tInterfaceWeb* m_Interface;

    SkFormat::tFormatRoot* m_FormatRoot;
    tVirtualClass* m_FormatApi; // Api
    
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

    
    void InitInterface();
    void DoneInterface();
 
    
    void LoadDocument(tInterfaceWeb* sInterfaceWeb,rapidjson::Document& wDocument);
        
    
    void ListWorkBook();
    
    void DrawCell(tInterfaceWeb* sInterface,tString sOperation, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);

    void Fill(tInterfaceWeb* sInterface);
    
    tString DebugCell(tInterfaceWeb* sInterface, tString sRef);
    
    void TestSkInterfaceUndoJson();
    void TestSkInterfaceFormatString();
    void TestSkInterfaceDeleteRow();
    void TestSkInterfaceRaz();
    void TestSkInterfaceCopy();
    void TestSkInterfaceRebaseConditionalFormat();
    void TestSkInterfaceBudget();
    void TestSkInterfaceBudgetCopyPaste();
    void TestSkInterfaceBudgetInsertCopyPaste();
    void TestSkInterfaceDeleteSheet();
public:
    virtual void setUp();
    virtual void tearDown();
};
#endif /* Test Multi User*/
#endif /* TestSkInterface_hpp */


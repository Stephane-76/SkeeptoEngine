//==============================================================================
// TestSkSpreadSheetFormat Test For SpreadSheet
// le 17/11/2023
//==============================================================================
#ifndef TestSkFormatString_hpp
#define TestSkFormatString_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

#include <SkFormatRoot.hpp>


using namespace SkRoot;
using namespace SkSpreadSheet;
using namespace SkFormat;

class TestSkFormatString : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkFormatString);
   	CPPUNIT_TEST(TestSkFormatNumber);
    CPPUNIT_TEST(TestSkFormatDate);
    CPPUNIT_TEST(TestSkFormatExcelInterface);
    CPPUNIT_TEST(TestSkFormatPrecision);
    CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	SkFormat::tFormatRoot* m_FormatRoot;
    SkSpreadSheet::tApi* m_Api;
    
    tVirtualClass* m_FormatApi; // Conflict with tSpreadSheet
    
    tBool ApplyFormat(tString sRef, tString sValue);

public:
    TestSkFormatString();
    
    void TestSkFormatNumber();
    void TestSkFormatDate();
    void TestSkFormatExcelInterface();
    void TestSkFormatPrecision();

private:
	void UndoOperation();
public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkSpreadSheetFormat */

//==============================================================================
// TestSkWorkBook
// le 15/09/2021
//==============================================================================

#ifndef TestSkWorkBook_hpp
#define TestSkWorkBook_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkWorkBook : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkWorkBook);
	CPPUNIT_TEST(TestTools);
	CPPUNIT_TEST(TestSheet);
    CPPUNIT_TEST(TestAddDeleteWorkBook);
    CPPUNIT_TEST(TestJsonRangeNamed);
    CPPUNIT_TEST(TestInfo);
    CPPUNIT_TEST(TestPath);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi*		   m_Api;
public:
	TestSkWorkBook();
private:
    tString WorkBooksArray(tString sJson);
    
	void TestTools();
	void TestSheet();
    
    void TestAddDeleteWorkBook();
    
    void TestJsonRangeNamed();
    void TestInfo();
    void TestPath();
public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkWorkBook */

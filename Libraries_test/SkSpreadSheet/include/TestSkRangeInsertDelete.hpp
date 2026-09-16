//=============================================================================  
// TestSkRangeInsertDelete
// author : Stéphane Allez
//=============================================================================  
#ifndef TestSkRangeInsertDelete_hpp
#define TestSkRangeInsertDelete_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkRangeInsertDelete : public CPPUNIT_NS::TestFixture {
    // Sequence of tests
    CPPUNIT_TEST_SUITE(TestSkRangeInsertDelete);
    CPPUNIT_TEST(TestInsertRowByRect);
	CPPUNIT_TEST_SUITE_END();
private:
    tApplication* m_Application; // Pointer of static application
    tApi* m_Api;
    // For Fill
    tInt m_NbCol;
    tInt m_NbRow;

    void DrawCell(tString sTitle, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
    void DebugRange();

    void Fill();
public:
    TestSkRangeInsertDelete();
public:
    void TestInsertRowByRect();
    
    
    void setUp();
    void tearDown();
};
#endif

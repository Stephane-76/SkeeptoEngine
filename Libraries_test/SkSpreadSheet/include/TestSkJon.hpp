//==============================================================================
// TestSkJson
// le 31/10/2021 
//==============================================================================
#ifndef TestSkJson_hpp
#define TestSkJson_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkJson : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkJson);
	CPPUNIT_TEST(Write);
	CPPUNIT_TEST(Read);
    CPPUNIT_TEST(ReadMultiSheet);
    CPPUNIT_TEST(Excel);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;

	// For Fill
	tInt m_NbCol;
	tInt m_NbRow;
public:
	TestSkJson();
private:
	void DrawCell(tString sTitle,tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);

	void Fill();

	void Write();
	void Read();
    
    void ReadMultiSheet();
    
    void Excel();

public:
	void setUp();
	void tearDown();
};


#endif  /* TestSkJon */

//==============================================================================
// TestSkRangeNamed
// le 31/10/2021 
//==============================================================================
#ifndef TestSkRangeNamed_hpp
#define TestSkRangeNamed_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkRangeNamed : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkRangeNamed);
	CPPUNIT_TEST(Name);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;

	// For Fill
	tInt m_NbCol;
	tInt m_NbRow;
public:
	TestSkRangeNamed();
private:
	void DrawCell(tString sTitle, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);

	void Fill();

	void Name();

	void TestFormula(tCell* sCell, tString sFormula);

public:
	void setUp();
	void tearDown();
};


#endif  /* TestSkJon */

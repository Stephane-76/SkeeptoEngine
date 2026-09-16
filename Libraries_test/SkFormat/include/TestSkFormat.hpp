//==============================================================================
// TestSkFormat
// le 15/09/2021
//==============================================================================

#ifndef TestSkFormat_hpp
#define TestSkFormat_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkFormatRoot.hpp>


using namespace SkRoot;
using namespace SkFormat;

class TestSkFormat : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkFormat);

	CPPUNIT_TEST(TestFormat);

	CPPUNIT_TEST_SUITE_END();
private:	
	tApplication* m_Application;
	tFormatRoot* m_FormatRoot;
public:
	TestSkFormat();
private:
#ifdef _DEBUG
	void Debug();
#endif
	void TestFormat();
public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkFormat */

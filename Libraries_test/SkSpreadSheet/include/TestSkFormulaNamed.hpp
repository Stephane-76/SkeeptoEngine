//=============================================================================
// TestSkFormulaNamed
//=============================================================================
#ifndef TestSkFormulaNamed_hpp
#define TestSkFormulaNamed_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;


class TestSkFormulaNamed : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkFormulaNamed);
	CPPUNIT_TEST(InsertFormulaNamed);
	CPPUNIT_TEST(JsonFormulaNamed);
	CPPUNIT_TEST(CallerRelativeRowNamedFormula);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;
public:
	TestSkFormulaNamed();
	void InsertFormulaNamed();
	void JsonFormulaNamed();
	void CallerRelativeRowNamedFormula();
	

	void setUp();
	void tearDown();
};

#endif

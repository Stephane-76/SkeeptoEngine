//=============================================================================
// TestSkFunctionFinancial.hpp
//=============================================================================
#ifndef TestSkFunctionFinancial_hpp
#define TestSkFunctionFinancial_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkSpreadSheet;

class TestSkFunctionFinancial : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkFunctionFinancial);
    CPPUNIT_TEST(TestFunctionFinancial);
    CPPUNIT_TEST_SUITE_END();

public:
    TestSkFunctionFinancial();
    ~TestSkFunctionFinancial() override;
    
    void TestFunctionFinancial();
    
private:
    tApplication* m_Application; // Pointer of static application
    tApi* m_Api;

public:
	void setUp() override;
	void tearDown() override;
};

#endif // TESTSKFUNCTIONFINANCIAL_HPP

//==============================================================================
// TestSkRootVariant
//==============================================================================

#ifndef TestSkRootVariant_hpp
#define TestSkRootVariant_hpp
#include <math.h> // Round
#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>


using namespace SkRoot;

class tRootVariant : public tVariantClass {
	private:
		tString  m_Unit;
	public:
		tRootVariant() : tVariantClass(), m_Unit() {}
		tRootVariant(tVariant sValue,tString sString) : tVariantClass(sValue), m_Unit(sString) {}

		tRootVariant(const tRootVariant& sVariantTestClass) : tVariantClass(sVariantTestClass) {
			m_Unit = sVariantTestClass.m_Unit;
		}
		 ~tRootVariant() {
		}
		virtual tVirtualClass* Clone() {
			return(new tRootVariant(*this));
		}
		tString GetUnit() {
			return(m_Unit);
		}
		
		virtual tVariant Operator_plus(tBool sLeft, const tVariant& sVariant) {
			return(tVariantClass::Operator_plus(sLeft, sVariant));
		}
		virtual tVariant Operator_minus(tBool sLeft, const tVariant& sVariant) {
			return(tVariantClass::Operator_minus(sLeft, sVariant));
		}
		virtual tVariant Operator_multiply(tBool sLeft, const tVariant& sVariant) {
			return(tVariantClass::Operator_multiply(sLeft, sVariant));
		}
		virtual tVariant Operator_divide(tBool sLeft, const tVariant& sVariant)  {
			return(tVariantClass::Operator_divide(sLeft, sVariant));
		}
		
};

class TestSkRootVariant : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkRootVariant);

	CPPUNIT_TEST(TestVariant);
	CPPUNIT_TEST(TestVariantClass);
	CPPUNIT_TEST(TestParse);
	CPPUNIT_TEST(TestAmpersand);

	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application;
public:
	TestSkRootVariant();
private:
	void TestVariant();
	void TestVariantClass();

	void TestParse();
	void TestAmpersand();

	void Error(tString sMessage);
	void Warning(tString sMessage);
public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkRoot */

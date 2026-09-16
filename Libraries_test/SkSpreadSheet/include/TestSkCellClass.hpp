//=============================================================================
// TestSkCellClasss
// le 31/10/2021 
//=============================================================================
#ifndef TestSkCellClass_hpp
#define TestSkCellClass_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkCellClass : public CPPUNIT_NS::TestFixture {
	// Sequence of tests
	CPPUNIT_TEST_SUITE(TestSkCellClass);
	CPPUNIT_TEST(TestInstance);
    CPPUNIT_TEST(TestCopy);
    CPPUNIT_TEST(TestCalculate);
	CPPUNIT_TEST_SUITE_END();
private:
	tApplication* m_Application; // Pointer of static application
	tApi* m_Api;
public:
	TestSkCellClass();
private:
    
    void SetValueMoney(tString sRef,t_UnitMoney sUnitMoney);
    void SetValueMoney(tString sRef,t_UnitMoney sUnitMoney,tDouble sValue);
    void SetValueLength(tString sRef,t_UnitLength sUnitLength,tDouble sValue);
    
    void DebugUnit(tString sRef);
    tString Str(tString sRef);
    
	void Fill();
	void DrawCell(tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
	void UndoOperation();
	
	void TestInstance();
    void TestCopy();
    void TestCalculate();

public:
	void setUp();
	void tearDown();
};


#endif    /* TestSkCell */

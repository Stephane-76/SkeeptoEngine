//=============================================================================
// TestSkCell.hpp
//
// CppUnit fixture for SkSpreadSheet::tCell (formulas, calculation graph, undo,
// row/column bounds, dates, messaging).
//
// Link TestSkCell.cpp with CppUnit and the same object libraries as other SkSpreadSheet tests.
//=============================================================================
#ifndef TestSkCell_hpp
#define TestSkCell_hpp

#include <cppunit/extensions/HelperMacros.h>

#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

/// CppUnit fixture: inherits CPPUNIT_NS::TestFixture ( CPPUNIT_TEST_SUITE registers tests ).
class TestSkCell : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkCell);
    CPPUNIT_TEST(TestFormula);
    CPPUNIT_TEST(TestCalculate);
    CPPUNIT_TEST(TestBlockedAcyclicSubgraphResolve);
    CPPUNIT_TEST(TestCell);
    CPPUNIT_TEST(TestUndoCell);
    CPPUNIT_TEST(TestLastColRow);
    CPPUNIT_TEST(TestDate);
    CPPUNIT_TEST(TestPostMessage);
    CPPUNIT_TEST_SUITE_END();

private:
    tApplication* m_Application;
    tApi* m_Api;

public:
    TestSkCell();

private:
    void DrawCell(tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
    void UndoOperation();

    void TestFormula();
    void TestCalculate();
    /// Mutual formulas A1=1+B1/2, B1=A1/2 (fixed point); ResolveBlockedAcyclicSubgraph must not tag #RECURSIVE.
    void TestBlockedAcyclicSubgraphResolve();
    void TestCell();
    void TestUndoCell();
    void TestLastColRow();
    void TestDate();
    void TestPostMessage();

public:
    void setUp();
    void tearDown();
};

#endif // TestSkCell_hpp

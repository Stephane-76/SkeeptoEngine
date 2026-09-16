// TestSkRaz.hpp
//
//  Created on: 28/11/2025
//      Author: Stéphane Allez
//

#ifndef TestSkRaz_hpp
#define TestSkRaz_hpp

#include <cppunit/extensions/HelperMacros.h>
#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;

class TestSkRaz : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(TestSkRaz);
    CPPUNIT_TEST(TestUndoRaz);
    CPPUNIT_TEST(TestUndoRazFormulaCell);
    CPPUNIT_TEST(TestUndoRazErrorCell);
    CPPUNIT_TEST(TestUndoRazValueCellInsideRange);
    CPPUNIT_TEST_SUITE_END();
public:
    TestSkRaz();
    void setUp();
    void tearDown();
private:
    void TestUndoRaz();
    // Regression: when a cell containing a formula is cleared (Raz/Delete),
    // dependents must be recalculated. Reproduces the user-reported bug
    // where deleting a function cell did not trigger recalc of dependents.
    void TestUndoRazFormulaCell();
    // Regression: when a cell in error (e.g. #DIV/0!) is cleared, dependents
    // must be recalculated so that the stale error is cleared from them.
    void TestUndoRazErrorCell();
    // Regression: single-cell Raz must recalc dependents even when the
    // razed cell is a plain value referenced by a range (e.g. SUM(G9:G10)).
    // Reproduces the user-reported case G10=23 / G12=SUM(G9:G10).
    void TestUndoRazValueCellInsideRange();
    tApplication* m_Application;
    tApi* m_Api;
};

#endif

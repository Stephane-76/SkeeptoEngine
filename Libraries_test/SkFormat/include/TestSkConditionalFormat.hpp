//=============================================================================
// TestSkConditionalFormat.hpp
// Stéphane Allez *
//  Created on: 21 sept. 2025
//=============================================================================

#ifndef TestSkConditionalFormat_hpp
#define TestSkConditionalFormat_hpp

#include <cppunit/extensions/HelperMacros.h>
#include <SkFormatCss.hpp>
#include <SkFormatCssApi.hpp>
#include <SkApplication.hpp>
#include <SkApi.hpp>

using namespace SkRoot;
using namespace SkSpreadSheet;
using namespace SkFormat;

class TestSkConditionalFormat : public CPPUNIT_NS::TestFixture {
    // Sequence of tests
    CPPUNIT_TEST_SUITE(TestSkConditionalFormat);
    CPPUNIT_TEST(TestFormatWidthFormula);
    CPPUNIT_TEST(TestColorScales2Color);
    CPPUNIT_TEST(TestColorScales3Color);
    CPPUNIT_TEST(TestColorScalesTemperature);
    CPPUNIT_TEST(TestColorScalesDetailed);
    CPPUNIT_TEST(TestColorScalesDebugColors);
    CPPUNIT_TEST(TestColorScalesJsonViewValidation);
    CPPUNIT_TEST(TestColorScalesNegativeValues);
    CPPUNIT_TEST(TestIconSetsArrows);
    CPPUNIT_TEST(TestIconSetsFlags);
    CPPUNIT_TEST(TestIconSets5Arrows);
    CPPUNIT_TEST(TestIconSets5Flags);
    CPPUNIT_TEST(TestIconSets5Ratings);
    CPPUNIT_TEST(TestIconSetsWithoutThresholds);
    CPPUNIT_TEST(TestDataBarsBasic);
    CPPUNIT_TEST(TestDataBarsGradient);
    CPPUNIT_TEST(TestDataBarsCustomRange);
    CPPUNIT_TEST(TestDataBarsNegativeValues);
    CPPUNIT_TEST(TestJsonConditionalFormat);
    CPPUNIT_TEST(TestUndoDeleteConditionalFormat);
    CPPUNIT_TEST(TestHighlightCellsRulesJsonViewRendering);
    CPPUNIT_TEST(TestJsonViewCfViewportClip);
    CPPUNIT_TEST_SUITE_END();
private:
    tApplication* m_Application; // Pointer of static application
    SkFormat::tFormatRoot* m_FormatRoot;
    tFormatApi* m_FormatApi;
    tApi* m_Api;
public:
    TestSkConditionalFormat();
private:
   void  TestFormatWidthFormula();
   void  TestColorScales2Color();
   void  TestColorScales3Color();
   void  TestColorScalesTemperature();
   void  TestColorScalesDetailed();
   void  TestColorScalesDebugColors();
   void  TestColorScalesJsonViewValidation();
   void  TestColorScalesNegativeValues();
   void  TestIconSetsArrows();
   void  TestIconSetsFlags();
   void  TestIconSets5Arrows();
   void  TestIconSets5Flags();
   void  TestIconSets5Ratings();
   void  TestIconSetsWithoutThresholds();
   void  TestDataBarsBasic();
   void  TestDataBarsGradient();
   void  TestDataBarsCustomRange();
   void  TestDataBarsNegativeValues();
   void  TestJsonConditionalFormat();
   void  TestUndoDeleteConditionalFormat();
   void  TestHighlightCellsRulesJsonViewRendering();
   void  TestJsonViewCfViewportClip();
public:
   void setUp() override;
   void tearDown() override;
};

#endif // TestSkConditionalFormat_hpp

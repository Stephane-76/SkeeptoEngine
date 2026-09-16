//==============================================================================
// TestSkRoot
// le 15/09/2021
//==============================================================================



#include "../include/TestSkInterface.hpp"
#include "../include/TestSkWorkBook.hpp"
#include "../include/TestSkCell.hpp"
#include "../include/TestSkCellClassUnit.hpp"
#include "../include/TestSkFunction.hpp"
#include "../include/TestSkExcel.hpp"
#include "../include/TestSkCellClass.hpp"
#include "../include/TestSkCellClassAttribute.hpp"
#include "../include/TestSkRaz.hpp"
#include "../include/TestSkCopyPaste.hpp"
#include "../include/TestSkSheet.hpp"
#include "../include/TestSkRange.hpp"
#include "../include/TestSkFormula.hpp"
#include "../include/TestSkFormulaNamed.hpp"
#include "../include/TestSkDynamicRange.hpp"
#include "../include/TestSkJon.hpp"
#include "../include/TestSkInsertDeleteColRow.hpp"
#include "../include/TestSkInsertDeleteColRowByRect.hpp"
#include "../include/TestSkNamedRange.hpp"
#include "../include/TestSkRangeData.hpp"
#include "../include/TestSkTree.hpp"
#include "../include/TestSkJsonPayload.hpp"
#include "../include/TestSkConditionalFormat.hpp"
#include "../include/TestSkFunctionFinancial.hpp"
#include "../include/TestSkWasm.hpp"
#include "../include/TestSkMatrix.hpp"
#include "../include/TestSkFindCell.hpp"
#include "../include/TestSkFillSeries.hpp"


#include <SkException.hpp>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TextTestProgressListener.h>

#include <cppunit/BriefTestProgressListener.h>

// Documentation
//http://cppunit.sourceforge.net/doc/cvs/class_test_runner.html
// Full suite: uncomment TestAll below.
// Isolate: comment TestAll and register a single suite in the #ifndef TestAll block.
#define TestAll

int main(int argc, char* argv[]) {
#ifndef TestAll
#ifdef TestMultiUser
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkInterface);
#endif
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkRange);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkFunction);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkMatrix);
#endif
#ifdef TestAll
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkFunction);

    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkFunctionFinancial);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkFormula);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkDynamicRange);

    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkCellClass);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkCellClassUnit);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkRange);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkWorkBook);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkSheet);
   
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkRangeNamed);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkFormulaNamed);

    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkInsertDeleteColRow);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkInsertDeleteColRowByRect);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkConditionalFormat);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkMatrix);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkFindCell);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkFillSeries);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkJsonPayload);
//#ifndef __EMSCRIPTEN__MEMORY__
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkCellClassAttribute);
//#endif
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkRaz);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkCopyPaste);
    
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkJson);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkTree);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkConditionalFormat);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkRangeData);

    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkExcel);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkWasm);
#ifdef TestMultiUser
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkInterface);
#endif
#endif
    std::string testPath = (argc > 1) ? std::string(argv[1]) : "";
    // Create the event manager and test controller
    CppUnit::TestResult controller;

    // Add a listener that colllects test result
    CppUnit::TestResultCollector result;
    controller.addListener(&result);

    // Add a listener that print dots as test run.
    CPPUNIT_NS::BriefTestProgressListener progress;
    controller.addListener(&progress);

    // Add a listener that print dots as test run.
    //CppUnit::TexTestProgressListener progress;
    //controller.addListener(&progress);

    // Add the top suite to the test runner
    CppUnit::TestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());
    try   {
        std::cout << "Running " << testPath << " .."<< endl;
        runner.run(controller, testPath);

        std::cerr << std::endl;

        // Print test in a compiler compatible format.
        CppUnit::CompilerOutputter outputter(&result, std::cerr);
        outputter.write();
    }
    catch (std::invalid_argument& e)  // Test path not resolved
    {
        std::cerr << std::endl
            << "ERROR: " << e.what()
            << std::endl;
        return 0;
    }
    catch (SkRoot::tExceptionInternalError& e)  // Catch SkRoot internal exceptions
    {
        std::cerr << std::endl
            << "SKER EXCEPTION Internal Error: " << e.what()
            << std::endl;
    }
    catch (const std::exception& e)  // Catch all standard exceptions
    {
        std::cerr << std::endl
            << "STANDARD EXCEPTION: " << e.what()
            << std::endl;
    }
    catch (...)  // Catch all other exceptions
    {
        std::cerr << std::endl
            << "UNKNOWN EXCEPTION occurred!"
            << std::endl;
    }
#ifdef __EMSCRIPTEN__
    tApplication::Instance()->Clear();
#ifdef _debugleak
    DebugMemory();
#endif
  
    
#endif
    return result.wasSuccessful() ? 0 : 1;
}

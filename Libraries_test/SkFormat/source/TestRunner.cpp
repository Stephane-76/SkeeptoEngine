//==============================================================================
// TestSkRoot
// le 15/09/2021
//==============================================================================
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TextTestProgressListener.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TestListener.h>

#include <cppunit/BriefTestProgressListener.h>
#include <exception>
#include <iostream>
#include <csignal>
#include <cassert>

#include "../include/TestSkFormat.hpp"
#include "../include/TestSkCopySpreadSheet.hpp"
#include "../include/TestSkFormatSpreadSheet.hpp"
#include "../include/TestSkInterface.hpp"
#include "../include/TestSkFormatSpreadSheetClass.hpp"
#include "../include/TestSkFormatString.hpp"
#include "../include/TestSkFormatWeb.hpp"
#include "../include/TestSkExcel.hpp"
#include "../include/TestSkConditionalFormat.hpp"
#include "../include/TestSkInsertRowCol.hpp"
#include "../include/TestSkRazFormat.hpp"


int main(int argc, char* argv[]) {
    // Set up exception handlers for debugging in Xcode
   
    
    // Uncomment the line below to test if Xcode stops on debugger trigger
    // triggerDebugger();
    
    // Uncomment the line below to run only the problematic test
    // testPath = "TestSkConditionalFormat::TestIconSets5Arrows";
    // Full suite: uncomment the line below (TestAll must not be passed via CMake -DTestAll).
#define TestAll

#ifndef TestAll
#ifdef TestMultiUser
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkInterface);
#endif
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkInsertRowCol);
#endif

#ifdef TestAll
    // Registers the fixture into the 'registry'
#ifdef TestMultiUser
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkInterface);
#endif
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkFormatString);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkFormatWeb);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkFormat);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkCopySpreadSheet);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkFormatSpreadSheet);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkFormatSpreadSheetClass);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkConditionalFormat);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkInsertRowCol);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkRazFormat);
    CPPUNIT_TEST_SUITE_REGISTRATION(TestSkExcel);
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
    catch (tExceptionInternalError& e)  // Catch all standard exceptions
    {
        std::cerr << std::endl
            << "SKER EXEPTION Internal Error: " << e.what()
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
#ifdef ___EMSCRIPTEN__
#ifdef _debugleak
    DebugMemory();
#endif
#endif

    return result.wasSuccessful() ? 0 : 1;
}

//=============================================================================
// Shared format-pool assertions after SpreadSheet API teardown in unit tests.
//=============================================================================
#ifndef TestSkFormatTeardownHelpers_hpp
#define TestSkFormatTeardownHelpers_hpp

#include <cppunit/TestAssert.h>

#include <SkClass.hpp>
#include <SkFormatCssApi.hpp>
#include <SkFormatRoot.hpp>

#include <sstream>
#include <string>

namespace TestSkFormatTeardown {

inline void AssertFormatPoolEmptyAfterApiDelete(SkRoot::tVirtualClass* sFormatApi, const char* sContext) {
    SkFormat::tFormatCssApi* wFormatApiCss =
        dynamic_cast<SkFormat::tFormatCssApi*>(sFormatApi);
    CPPUNIT_ASSERT_MESSAGE(
        (std::string(sContext) + ": FormatApi must be tFormatCssApi").c_str(),
        wFormatApiCss != nullptr);

#ifdef checkfo
    wFormatApiCss->Check();
#endif

    const SkRoot::tFormatRef wCount = wFormatApiCss->Count();
    if (wCount != 0) {
        std::ostringstream wMsg;
        wMsg << sContext << ": format pool should be empty after delete(m_Api), got "
             << wCount;
        CPPUNIT_FAIL(wMsg.str().c_str());
    }
}

inline void AssertFormatRootEmptyAfterTeardown(SkFormat::tFormatRoot* sFormatRoot, const char* sContext) {
    CPPUNIT_ASSERT_MESSAGE(
        (std::string(sContext) + ": FormatRoot must not be null").c_str(),
        sFormatRoot != nullptr);

#ifdef checkfo
    sFormatRoot->Check();
#endif

    const SkRoot::tFormatRef wCount = sFormatRoot->Count();
    if (wCount != 0) {
        std::ostringstream wMsg;
        wMsg << sContext << ": format pool should be empty after teardown, got "
             << wCount;
        CPPUNIT_FAIL(wMsg.str().c_str());
    }
}

} // namespace TestSkFormatTeardown

#endif /* TestSkFormatTeardownHelpers_hpp */

#ifndef SkExcelApplyChartsSelfTest_hpp
#define SkExcelApplyChartsSelfTest_hpp

#include <SkTypes.hpp>

namespace SkExcel {

using SkRoot::tInt;
using SkRoot::tString;

tString DefaultApplyChartsTestXlsxPath();

//! Returns 0 on PASS, 1 on FAIL, 2 on SKIP (no charts in workbook).
tInt RunApplyChartsSelfTest(const tString& sXlsxPath);

} // namespace SkExcel

#endif

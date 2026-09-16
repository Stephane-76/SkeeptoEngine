#ifndef SkExcelApplyTextBoxesSelfTest_hpp
#define SkExcelApplyTextBoxesSelfTest_hpp

#include <SkTypes.hpp>

namespace SkExcel {

using SkRoot::tInt;
using SkRoot::tString;

/// Default xlsx used when /t:apply-textboxes runs without /f:.
tString DefaultApplyTextBoxesTestXlsxPath();

/// Import sXlsxPath and assert floating SkCellClassTextBox in WriteJson.
/// Returns 0 on pass or skip (no text boxes), non-zero on failure.
tInt RunApplyTextBoxesSelfTest(const tString& sXlsxPath);

} // namespace SkExcel

#endif

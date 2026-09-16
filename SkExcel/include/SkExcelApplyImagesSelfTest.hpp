#ifndef SkExcelApplyImagesSelfTest_hpp
#define SkExcelApplyImagesSelfTest_hpp

#include <SkTypes.hpp>

namespace SkExcel {

using SkRoot::tInt;
using SkRoot::tString;

/// Default xlsx used when /t:apply-images runs without /f:.
tString DefaultApplyImagesTestXlsxPath();

/// Import sXlsxPath and assert floating SkCellClassImage + data:image/ in WriteJson.
/// Returns 0 on pass or skip (no embeddable images), non-zero on failure.
tInt RunApplyImagesSelfTest(const tString& sXlsxPath);

} // namespace SkExcel

#endif

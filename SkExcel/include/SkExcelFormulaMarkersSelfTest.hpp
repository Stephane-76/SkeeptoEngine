#ifndef SkExcelFormulaMarkersSelfTest_hpp
#define SkExcelFormulaMarkersSelfTest_hpp

#include <SkTypes.hpp>

namespace SkExcel {

using SkRoot::tInt;

/// Self-test for the OOXML formula-marker transform (TransFormFormulaSyntax):
///   - strips _xlfn. / _xlfn._xlws. / _xlpm. / _xll. prefixes,
///   - rewrites ANCHORARRAY(D14) to D14# (Excel spilled-range operator),
///   - preserves literal array constants like {"POS","TEAM"} (does NOT drop their braces),
///   - leaves markers inside quoted string literals untouched.
/// Returns 0 on pass, non-zero on failure. Needs no input file.
tInt RunFormulaMarkersSelfTest();

} // namespace SkExcel

#endif

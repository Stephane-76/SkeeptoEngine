//=============================================================================
// SkRangeRefTransform — qualify / strip sheet prefixes on A1 refs via Lexer
// Component Floating Object
//=============================================================================
#ifndef SkRangeRefTransform_hpp
#define SkRangeRefTransform_hpp

#include "SkTools.hpp"

namespace SkSpreadSheet {

    class tWorkBook;

    /// @brief Prefix unqualified refs with sSheet (Sheet!A1 or 'My Sheet'!A1).
    tString QualifyRefsForSheet(tString sSheet, tString sText);

    /// @brief Remove sSheet! from refs that point to sSheet; leave other sheets unchanged.
    tString StripTargetSheetFromRefs(tString sSheet, tString sText);

    /// @brief Collect cell/range/table/name refs from a formula string (lexer-based; tolerates syntax errors).
    /// @param sWorkBook Optional workbook for resolving coordinates (nullptr => text/kind only).
    /// @param sHostSheet Sheet hosting the formula cell (default sheet for unqualified refs).
    /// @param sHostRow 1-based row of the formula cell (#This Row, relative refs).
    /// @param sHostCol 1-based column of the formula cell.
    /// @param sText Formula text (with or without leading '=').
    /// @return JSON: { syntaxError, errorLine, errorColumn, refs:[{ kind, text, sheet, top, left, bottom, right, resolved }] }.
    tString CollectFormulaRefsJson(tWorkBook* sWorkBook, tString sHostSheet, tIndex sHostRow, tIndex sHostCol,
                                   tString sText);

} // namespace SkSpreadSheet

#endif

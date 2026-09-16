//=============================================================================
// SkFillSeries
// Excel-like fill-handle series engine (auto-fill), C++ port of the former
// JavaScript SkFillSeries.js.
//
// Given a set of seed cell values (in forward order) it extrapolates the next
// values: arithmetic number sequences, month names, weekday names (localized
// through tLocale of the active application locale), "prefix + number" text
// patterns, and, as a fallback, a cyclic repeat of the seeds.
//
// String / date helpers delegate to tClassString and tClassDate (same parse
// path as tVariant for dates via FormatDateShort / FormatDateLong).
//=============================================================================
#ifndef SkFillSeries_hpp
#define SkFillSeries_hpp

#include <SkTypes.hpp>
using namespace SkRoot;

namespace SkSpreadSheet {

    //! Excel-like auto-fill series engine.
    class tFillSeries {
    public:
        /// @brief      Extend a seed series forward by @p sCount values.
        /// @param[in]  sSeeds  Seed cell input strings in forward (fill) order.
        /// @param[in]  sCount  Number of values to generate after the seeds.
        /// @return     Generated values (size == sCount). Uses the active
        ///             application locale (tApplication::Instance()->Locale())
        ///             for month / weekday detection.
        static tVectorString Extend(const tVectorString& sSeeds, tSize sCount);

        /// @brief      Shift the relative A1 references of a formula string.
        /// @param[in]  sFormula   Formula string (with or without leading '=').
        /// @param[in]  sDeltaRow  Row offset (may be negative).
        /// @param[in]  sDeltaCol  Column offset (may be negative).
        /// @return     Formula string with relative references shifted; the
        ///             leading '=' (if any) is preserved. Absolute ($) parts,
        ///             function names and numeric literals are left untouched.
        static tString ShiftFormula(const tString& sFormula, tInt sDeltaRow, tInt sDeltaCol);
    };

}

#endif /* SkFillSeries_hpp */

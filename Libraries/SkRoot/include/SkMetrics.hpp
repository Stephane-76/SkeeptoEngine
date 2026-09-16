//=============================================================================
// SkMetrics
//=============================================================================
#ifndef SkMetrics_hpp
#define SkMetrics_hpp

#include "SkTypes.hpp"
#include "SkClass.hpp"

namespace SkRoot {
    enum class tUnitMetrics : tByte {
        none=0,		   // Not uses
        pixels,		   // pixels = 1 / 96 inches 
        inches,        // inches = 2.54 centimeters 
        points,        // points = 1 / 12 picas 
        picas,         // picas = 12 * 96 / 72 pixels
        centimeters,   // centimeters = 10 millimeters
        millimeters,   // millimeters = 72 / 2.54 points
        percent,	   // CSS percent
        em,            // Font relative
        number         // by example line height
    };
// Bug LINK ????????????????????????
#ifdef __linux__
namespace SkMetrics {
    /// @brief Convert value from one unit to another
    /// @param[in] sValue tDouble Value to convert
    /// @param[in] sFrom tUnitMetrics Source unit
    /// @param[in] sTo tUnitMetrics Target unit
    /// @return tDouble Converted value
    tDouble Convert(const tDouble sValue, const tUnitMetrics sFrom, const tUnitMetrics sTo);

    /// @brief Get short name of unit
    /// @param[in] sUnit tUnitMetrics Unit
    /// @return tString Short name of unit
    tString UnitShortName(const tUnitMetrics sUnit);

    /// @brief Get the DPI used for pixel <-> physical-unit conversions.
    /// Callers that mix Excel-style px-at-96 DPI with SkSpreadSheet's
    /// rendering DPI need this to compute compensation factors.
    tInt GetDPI();
}
#else
    class SkMetrics : public tClass {
    public:
        /// @brief Convert value from one unit to another
        /// @param[in] sValue tDouble Value to convert
        /// @param[in] sFrom tUnitMetrics Source unit
        /// @param[in] sTo tUnitMetrics Target unit
        /// @return tDouble Converted value
        static tDouble Convert(const tDouble sValue, const tUnitMetrics sFrom, const tUnitMetrics sTo);

        /// @brief Get short name of unit
        /// @param[in] sUnit tUnitMetrics Unit
        /// @return tString Short name of unit
        static tString UnitShortName(const tUnitMetrics sUnit);

        /// @brief Get the DPI used for pixel <-> physical-unit conversions.
        /// Callers that mix Excel-style px-at-96 DPI with SkSpreadSheet's
        /// rendering DPI need this to compute compensation factors.
        static tInt GetDPI();
    };
#endif // End __linux__
} // end of SkRoot
#endif // End SkMetrics_hpp

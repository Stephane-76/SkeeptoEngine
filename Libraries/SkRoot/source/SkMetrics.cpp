//=============================================================================
// Sker  Metrics
//=============================================================================
#include "../include/SkMetrics.hpp"

namespace SkRoot {

static const tInt m_DPI = 96;

namespace {


// Conversion factor table goes through "points" as the pivot unit
// (1 inch = 72 pt). This avoids the previous chain implementation that:
//   - looped indirectly through pixels for purely metric conversions
//     (mm -> cm), pulling in a useless dependency on m_DPI;
//   - recursed forever when the target was a non-length unit such as
//     percent / em / number / none, because the chain is circular.
//
// Pixels & picas are still DPI-resolved at call time because m_DPI is
// a runtime configuration knob, but we never need DPI when both source
// and target are in {in, cm, mm, pt, pc}.
inline tDouble ToPoints(tDouble sValue, tUnitMetrics sUnit) {
    switch (sUnit) {
        case tUnitMetrics::inches:      return sValue * 72.0;
        case tUnitMetrics::centimeters: return sValue * 72.0 / 2.54;
        case tUnitMetrics::millimeters: return sValue * 72.0 / 25.4;
        case tUnitMetrics::points:      return sValue;
        case tUnitMetrics::picas:       return sValue * 12.0;
        case tUnitMetrics::pixels:      return sValue * 72.0 / static_cast<tDouble>(m_DPI);
        // Non-length units (percent / em / number / none) carry no
        // length semantics; we pass the value through untouched.
        default:                        return sValue;
    }
}

inline tDouble FromPoints(tDouble sPoints, tUnitMetrics sUnit) {
    switch (sUnit) {
        case tUnitMetrics::inches:      return sPoints / 72.0;
        case tUnitMetrics::centimeters: return sPoints * 2.54 / 72.0;
        case tUnitMetrics::millimeters: return sPoints * 25.4 / 72.0;
        case tUnitMetrics::points:      return sPoints;
        case tUnitMetrics::picas:       return sPoints / 12.0;
        case tUnitMetrics::pixels:      return sPoints * static_cast<tDouble>(m_DPI) / 72.0;
        default:                        return sPoints;
    }
}

inline tDouble ConvertImpl(tDouble sValue, tUnitMetrics sFrom, tUnitMetrics sTo) {
    if (sFrom == sTo) return sValue;
    return FromPoints(ToPoints(sValue, sFrom), sTo);
}

inline tString UnitShortNameImpl(tUnitMetrics sUnit) {
    switch (sUnit) {
        case tUnitMetrics::inches:      return tString("in");
        case tUnitMetrics::centimeters: return tString("cm");
        case tUnitMetrics::millimeters: return tString("mm");
        case tUnitMetrics::points:      return tString("pt");
        case tUnitMetrics::picas:       return tString("pc");
        case tUnitMetrics::pixels:      return tString("px");
        case tUnitMetrics::percent:     return tString("%");
        case tUnitMetrics::em:          return tString("em");
        case tUnitMetrics::number:      return tString("");
        case tUnitMetrics::none:        return tString("");
    }
    return tString("");
}

} // anonymous namespace

#ifdef __linux__
namespace SkMetrics {

    tDouble Convert(const tDouble sValue, const tUnitMetrics sFrom, const tUnitMetrics sTo) {
        return ConvertImpl(sValue, sFrom, sTo);
    }

    tString UnitShortName(const tUnitMetrics sUnit) {
        return UnitShortNameImpl(sUnit);
    }

    tInt GetDPI() { return m_DPI; }

} // End of namespace
#else
    tDouble SkMetrics::Convert(const tDouble sValue, const tUnitMetrics sFrom, const tUnitMetrics sTo) {
        return ConvertImpl(sValue, sFrom, sTo);
    }

    tString SkMetrics::UnitShortName(const tUnitMetrics sUnit) {
        return UnitShortNameImpl(sUnit);
    }

    tInt SkMetrics::GetDPI() { return m_DPI; }
#endif

} //end of name SkRoot


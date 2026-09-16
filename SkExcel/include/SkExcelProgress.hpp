//=============================================================================
// SkExcelProgress.hpp
//
// Cross-boundary progress reporting for Excel <-> SpreadSheet conversion.
//
// The xlsx import/export runs as a single blocking call from JS (Node worker in
// Electron, or the browser). To surface a real percentage we call out to a JS
// hook periodically from inside the conversion loops. Under Emscripten this
// invokes globalThis.__skExcelProgress(pct) synchronously; the host relays it.
// On native builds it is a no-op.
//
// The implementation lives in a single translation unit (SkExcel2SpreadSheet.cpp)
// so the EM_JS symbol is defined exactly once.
//=============================================================================
#ifndef SKEXCELPROGRESS_HPP
#define SKEXCELPROGRESS_HPP

namespace SkExcel {

// Report conversion progress as an integer percentage in [0, 100].
// Safe to call frequently; the JS side throttles duplicate values.
void SkExcelReportProgress(int sPercent);

} // namespace SkExcel

#endif // SKEXCELPROGRESS_HPP

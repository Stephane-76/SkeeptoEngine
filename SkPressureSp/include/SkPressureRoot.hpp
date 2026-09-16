//=============================================================================
// SkPressureRoot 
// Test Pressure
//=============================================================================
#ifndef SkPressureRoot_hpp
#define SkPressureRoot_hpp

// Windows process-memory APIs must be parsed before Lemon's token macros
// (SkLemonSpreadSheet.h #define BOOL 36), which otherwise turn psapi.h's
// `BOOL WINAPI GetWsChanges` into `36 WINAPI GetWsChanges` (C2059).
#ifdef _M_X64
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
// Lemon (SkLemonSpreadSheet.h) #define BOOL 36. If that header was already
// included (e.g. SkMain.cpp), psapi.h would parse `BOOL WINAPI` as `36 WINAPI`.
#ifdef BOOL
#undef BOOL
#endif
#include <psapi.h>
#endif

#include <SkApi.hpp>
#include <SkSpreadSheet.hpp>

#include <ctime>
#include <iostream>


struct separate_thousands : std::numpunct<char> {
    char_type do_thousands_sep() const override { return '.'; }  // separate with point
    string_type do_grouping() const override { return "\3"; } // groups of 3 digit
};
using namespace SkRoot;
using namespace SkSpreadSheet;

#ifdef _DEBUG
const tInt wNbRow = 10;
const tInt wModulo = 1;
const tInt wNbCol = 4;
#else

#ifdef __EMSCRIPTEN__
const tInt wNbRow = 1048576;
const tInt wModulo = 8;
const tInt wNbCol = 11; //10
#else
const tInt wNbRow = 1048576;
const tInt wModulo = 20;
const tInt wNbCol = 11;
#endif
#endif

extern tInt wDynamicNbRow;
extern tInt wDynamicNbCol;
extern tInt wDynamicModulo;

extern tBool wJsonTest;

extern tBool wDeleteRow;

enum class SkColorConsole { bluelow = 1, green, blue, red, purple, brow, normal, lightgreen,lightblue,lightred };
void SetConsoleColor(SkColorConsole sColor);

void DrawCell(tApi* sApi, tInt sRowBegin, tInt sColBegin, tInt sRowEnd, tInt sColEnd);
void PrintSize();
void StreamLocale();
void MemoryUses();

/// @brief      Format a tApplication timer sample as seconds with 3 decimals.
///             TimerElapsed() returns clock ticks, not milliseconds: CLOCKS_PER_SEC is
///             1000000 on macOS and Emscripten, so the old " ms" suffix printed microseconds.
/// @param[in]  sTicks clock_t as returned by tApplication::TimerElapsed()
/// @return     tString e.g. "16.854"
tString ElapsedSec(clock_t sTicks);

/// @brief      ElapsedSec() on the current tApplication timer.
/// @return     tString
tString ElapsedSec();
#endif

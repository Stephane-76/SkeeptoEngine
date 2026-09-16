//=============================================================================
// Skeema Types
/**
* @page SkType
* @par
* @par SkType.hpp Definition of the base types of the Sker library
*/
//=============================================================================
#ifndef SkType_hpp
#define SkType_hpp

#include <vector>
#include <set>
#include <map>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <locale>
#include <memory>
#include <assert.h> 
#include <ctime>
#include <functional>
#include <limits>
#include <filesystem>
#include <thread>         // std::thread
#include <mutex>

// Release debug special test
//#define _DEBUGSK

#ifdef _WIN32
	#include <windows.h>
	// Problem with limit::maxnumeric_limits<T>::max()
	#undef max
	#undef min
#else
	#include <unistd.h>
#endif

#ifdef __EMSCRIPTEN__
    // wasm: SK_RELEASE comes from CMake (SK_COMPIL=RELEASE). Do not use NDEBUG:
    // Windows compilWasm.cmd passes CMAKE_BUILD_TYPE=Release even for debug wasm.
    #if defined(SK_RELEASE)
        #define _RELEASE
        // Release debug special test in release mode
        //#define _DEBUGSK
    #else
        #define _DEBUGSK
        #define _DEBUGLeak
    #endif
    #define __EMSCRIPTEN__MEMORY__
	#define SkInline inline
#else
// Native (Windows/MSVC): Release is NDEBUG (CMake /DNDEBUG) and/or !_DEBUG.
// Do not rely on NDEBUG alone: some CMakeLists overwrite CMAKE_CXX_FLAGS_RELEASE
// and drop /DNDEBUG. MSVC Debug CRT always defines _DEBUG (/MDd /MTd).
    #if defined(NDEBUG) || (defined(_MSC_VER) && !defined(_DEBUG))
        #define _RELEASE
        // Release debug special test in release mode
        //#define _DEBUGSK
    #else
        #define _DEBUGSK
        // Memory leak
        #define _DEBUGLeak
    #endif
    // `inline` is required for ODR on header definitions. Debug /Od already
    // skips inlining; do not leave SkInline empty or free functions in headers
    // produce LNK4006 in the static library.
    #define SkInline inline
#endif


using namespace std;

namespace SkRoot {

// Define Sker Min Max ========================================================
#define SkMin(a,b) ((a) < (b) ? (a) : (b))
#define SkMax(a,b) ((a) > (b) ? (a) : (b))

    // Define Maxint ===============================================================
    template<typename T>
    constexpr T max_value {std::numeric_limits<T>::max()};

    const int SkMaxInt=SkRoot::max_value<int>;

	// For memory alignment
	#define SkAlign 8 // very important for circular memory 
	//=========================================================================
	//! tInt 4 bytes int
	typedef int tInt;		   
	typedef unsigned int tUInt; 

	//! tSize 4 bytes unsigned int
	typedef size_t tSize;
    typedef vector<tSize> tVectorSize;
    typedef stack<tSize> tStackSize;
	//! tShort 2 bytes
	typedef short int tShort; // 2 bytes
	typedef unsigned short int tUShort; // 2 bytes

	//! tByte 1 bytes unsigned
	typedef char tByte; // 1 bytes unsigned
	typedef unsigned char tUByte; // 1 bytes unsigned

	//! tLong 4 bytes
	typedef long tLong;
	
	//! tLongLong 8 bytes
	typedef long long tLongLong;
	
	//! tBool 1 bytes
	typedef bool tBool;
	
	//! tFloat 4 bytes
	typedef float tFloat;
	//! tDouble 8 bytes
	typedef double tDouble;

	//! tChar Utf8 char 1 byte
	typedef char tChar;

	//! tString Utf8 String 40 bytes
	typedef std::basic_string<tChar> tString;

	//! tStringVector (vector of string)
	typedef vector<tString> tVectorString;
	typedef tVectorString::iterator tIteratorVectorString;

	//! t16Char Utf16 char 2 bytes
	typedef char16_t t16Char;
	//! t16String Utf16String 40 bytes
	typedef std::basic_string<t16Char> t16String;

	//! t32Char Utf32 char 4 bytes
	typedef char32_t t32Char;
	//! t16String Utf16String 40 bytes
	typedef std::basic_string<t32Char> t32String;

	//!tDate time
	typedef std::time_t tDate; 
	//========================================================================
	//! tStringStream Stream string Utf8
	typedef std::basic_stringstream<tChar> tStringStream;

	//! tString16Stream Stream string Utf16
	typedef std::basic_stringstream<t16Char> t16StringStream;

	//! tString32Stream Stream string Utf32
	typedef std::basic_stringstream<t32Char> t32StringStream;

	//! vector of pointer
	typedef std::vector<void*> tVectorPointer;
	typedef tVectorPointer::iterator tIteratorVectorPointer;

	//! vector of tInt
	typedef std::vector<tInt> tVectorInt;
	typedef tVectorInt::iterator tIteratorVectorInt;

    typedef std::stack<tInt> tStackInt;
	//! For allocator memory and Db ==========================================

#if defined(__EMSCRIPTEN__) || defined(__CLOPINETTEAPPLE__)
    typedef unsigned int tAllocatorRef; // 32bits
#else
    typedef unsigned int tAllocatorRef; // 64bits
#endif
	typedef vector<tAllocatorRef> tVectorAllocatorRef;

    // Index (Spreadsheet col row index)
    typedef int  tIndex;
    typedef tByte tIndexShort;

	//! Color for Skia
	typedef uint32_t tColor;

#ifdef _WIN32
	const char cSlash='\\';
#else
	const char cSlash = '/';
#endif


}; // end of namespace ========================================================
#endif

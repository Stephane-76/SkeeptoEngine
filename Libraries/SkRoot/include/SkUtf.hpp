//=============================================================================
// SkUtf
/**
 * @page SkUtf
 * @par
 * @par Utf 8 16 Management 
 */
//=============================================================================
#ifndef SkUtf_hpp
#define SkUtf_hpp

#include "SkTypes.hpp"

namespace SkRoot {
    /// @brief Convert UTF-16 string to UTF-8
    /// @param[in] sUtf16 t16String
    /// @return tString
    tString Utf16ToUtf8(const t16String sUtf16);

    /// @brief Convert UTF-8 string to UTF-16
    /// @param[in] sUtf8 tString
    /// @return t16String
    t16String Utf8ToUtf16(const tString sUtf8);
    
    /// @brief Count UTF-8 characters in a string (not bytes)
    /// @param[in] sText tString UTF-8 encoded string
    /// @return tSize Number of UTF-8 characters
    tSize CountUtf8Characters(const tString& sText);
    
    /// @brief Count UTF-16 characters in a string (not code units)
    /// @param[in] sText t16String UTF-16 encoded string
    /// @return tSize Number of UTF-16 characters (surrogate pairs count as 1)
    tSize CountUtf16Characters(const t16String& sText);
}

#endif

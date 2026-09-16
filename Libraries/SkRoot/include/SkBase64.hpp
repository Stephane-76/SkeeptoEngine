//=============================================================================
// SkBase64.hpp — RFC 4648 Base64 encode/decode (no external deps)
//=============================================================================
#ifndef SKBASE64_HPP
#define SKBASE64_HPP

#include "SkTypes.hpp"
#include <vector>

namespace SkRoot {

class SkBase64 {
public:
    /// Encode binary data to Base64 (no line breaks).
    static tString Encode(const void* sData, tSize sSize);
    static tString Encode(const std::vector<char>& sData);

    /// Decode Base64 to bytes (ignores whitespace). Returns empty on invalid input.
    static std::vector<char> Decode(const tString& sEncoded);

    /// Guess MIME type from a file path extension (xl/media/image1.png).
    static tString MimeTypeFromImagePath(const tString& sPath);

    /// Build a data: URL suitable for HTML img src.
    static tString DataUrl(const tString& sMimeType, const void* sData, tSize sSize);
};

} // namespace SkRoot

#endif

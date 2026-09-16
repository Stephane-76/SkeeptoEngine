//=============================================================================
// SkBase64.cpp
//=============================================================================
#include "../include/SkBase64.hpp"

#include <cctype>

namespace SkRoot {

namespace {

static const char kEncodeTable[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static tInt DecodeChar(tByte sChar) {
    if (sChar >= 'A' && sChar <= 'Z') {
        return sChar - 'A';
    }
    if (sChar >= 'a' && sChar <= 'z') {
        return sChar - 'a' + 26;
    }
    if (sChar >= '0' && sChar <= '9') {
        return sChar - '0' + 52;
    }
    if (sChar == '+') {
        return 62;
    }
    if (sChar == '/') {
        return 63;
    }
    return -1;
}

static tString ToLowerExt(tString sExt) {
    for (tChar& wCh : sExt) {
        if (wCh >= 'A' && wCh <= 'Z') {
            wCh = static_cast<tChar>(wCh - 'A' + 'a');
        }
    }
    return sExt;
}

} // namespace

tString SkBase64::Encode(const void* sData, tSize sSize) {
    if (sData == nullptr || sSize == 0) {
        return "";
    }
    const auto* wBytes = static_cast<const unsigned char*>(sData);
    tString wOut;
    wOut.reserve(((sSize + 2) / 3) * 4);

    tSize wIndex = 0;
    while (wIndex + 2 < sSize) {
        // tByte is char (signed): bytes >= 0x80 must not sign-extend into tUInt.
        const tUInt wBlock =
            (static_cast<tUInt>(wBytes[wIndex]) << 16) |
            (static_cast<tUInt>(wBytes[wIndex + 1]) << 8) |
            static_cast<tUInt>(wBytes[wIndex + 2]);
        wOut.push_back(kEncodeTable[(wBlock >> 18) & 0x3F]);
        wOut.push_back(kEncodeTable[(wBlock >> 12) & 0x3F]);
        wOut.push_back(kEncodeTable[(wBlock >> 6) & 0x3F]);
        wOut.push_back(kEncodeTable[wBlock & 0x3F]);
        wIndex += 3;
    }

    const tSize wRemain = sSize - wIndex;
    if (wRemain == 1) {
        const tUInt wBlock = static_cast<tUInt>(wBytes[wIndex]) << 16; // unsigned char*
        wOut.push_back(kEncodeTable[(wBlock >> 18) & 0x3F]);
        wOut.push_back(kEncodeTable[(wBlock >> 12) & 0x3F]);
        wOut.push_back('=');
        wOut.push_back('=');
    } else if (wRemain == 2) {
        const tUInt wBlock =
            (static_cast<tUInt>(wBytes[wIndex]) << 16) |
            (static_cast<tUInt>(wBytes[wIndex + 1]) << 8); // unsigned char*
        wOut.push_back(kEncodeTable[(wBlock >> 18) & 0x3F]);
        wOut.push_back(kEncodeTable[(wBlock >> 12) & 0x3F]);
        wOut.push_back(kEncodeTable[(wBlock >> 6) & 0x3F]);
        wOut.push_back('=');
    }
    return wOut;
}

tString SkBase64::Encode(const std::vector<char>& sData) {
    return Encode(sData.data(), sData.size());
}

std::vector<char> SkBase64::Decode(const tString& sEncoded) {
    std::vector<char> wOut;
    if (sEncoded.empty()) {
        return wOut;
    }

    tInt wBuffer = 0;
    tInt wBits = 0;
    for (tChar wRaw : sEncoded) {
        if (std::isspace(static_cast<unsigned char>(wRaw)) != 0) {
            continue;
        }
        if (wRaw == '=') {
            break;
        }
        const tInt wVal = DecodeChar(static_cast<tByte>(wRaw));
        if (wVal < 0) {
            return {};
        }
        wBuffer = (wBuffer << 6) | wVal;
        wBits += 6;
        if (wBits >= 8) {
            wBits -= 8;
            wOut.push_back(static_cast<char>((wBuffer >> wBits) & 0xFF));
        }
    }
    return wOut;
}

tString SkBase64::MimeTypeFromImagePath(const tString& sPath) {
    const tSize wDot = sPath.find_last_of('.');
    if (wDot == tString::npos) {
        return "application/octet-stream";
    }
    const tString wExt = ToLowerExt(sPath.substr(wDot + 1));
    if (wExt == "png") {
        return "image/png";
    }
    if (wExt == "jpg" || wExt == "jpeg") {
        return "image/jpeg";
    }
    if (wExt == "gif") {
        return "image/gif";
    }
    if (wExt == "bmp") {
        return "image/bmp";
    }
    if (wExt == "webp") {
        return "image/webp";
    }
    if (wExt == "emf") {
        return "image/emf";
    }
    if (wExt == "wmf") {
        return "image/wmf";
    }
    if (wExt == "tif" || wExt == "tiff") {
        return "image/tiff";
    }
    return "application/octet-stream";
}

tString SkBase64::DataUrl(const tString& sMimeType, const void* sData, tSize sSize) {
    const tString wMime = sMimeType.empty() ? "application/octet-stream" : sMimeType;
    return "data:" + wMime + ";base64," + Encode(sData, sSize);
}

} // namespace SkRoot

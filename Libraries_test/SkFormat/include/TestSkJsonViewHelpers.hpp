//=============================================================================
// JsonView test helpers — resolve f_i through formats[] (same as JS hydrate).
//=============================================================================
#ifndef TestSkJsonViewHelpers_hpp
#define TestSkJsonViewHelpers_hpp

#include <rapidjson/document.h>

namespace TestSkJsonView {

inline const rapidjson::Value* CellFormatEntry(
    const rapidjson::Document& sDoc,
    const rapidjson::Value& sCell) {
    if (sCell.HasMember("f_i") && sDoc.HasMember("formats")) {
        const rapidjson::Value& wFormats = sDoc["formats"];
        if (wFormats.IsArray()) {
            const rapidjson::SizeType wIdx = sCell["f_i"].GetUint();
            if (wIdx < wFormats.Size() && wFormats[wIdx].IsObject()) {
                return &wFormats[wIdx];
            }
        }
    }
    return nullptr;
}

inline bool CellHasMember(
    const rapidjson::Document& sDoc,
    const rapidjson::Value& sCell,
    const char* sKey) {
    if (sCell.HasMember(sKey)) {
        return true;
    }
    const rapidjson::Value* wFmt = CellFormatEntry(sDoc, sCell);
    return wFmt != nullptr && wFmt->HasMember(sKey);
}

inline int CellMemberInt(
    const rapidjson::Document& sDoc,
    const rapidjson::Value& sCell,
    const char* sKey) {
    if (sCell.HasMember(sKey)) {
        return sCell[sKey].GetInt();
    }
    const rapidjson::Value* wFmt = CellFormatEntry(sDoc, sCell);
    if (wFmt != nullptr && wFmt->HasMember(sKey)) {
        return (*wFmt)[sKey].GetInt();
    }
    return 0;
}

inline const rapidjson::Value* CellMemberValue(
    const rapidjson::Document& sDoc,
    const rapidjson::Value& sCell,
    const char* sKey) {
    if (sCell.HasMember(sKey)) {
        return &sCell[sKey];
    }
    const rapidjson::Value* wFmt = CellFormatEntry(sDoc, sCell);
    if (wFmt != nullptr && wFmt->HasMember(sKey)) {
        return &(*wFmt)[sKey];
    }
    return nullptr;
}

} // namespace TestSkJsonView

#endif /* TestSkJsonViewHelpers_hpp */

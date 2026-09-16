#include "../include/SkJsonSelect.hpp"
#include <rapidjson/error/en.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <iostream>

namespace SkSpreadSheet {

tBool tJsonSelect::ReadJson(tString sJson) {
    Document wDocument;
    rapidjson::ParseResult wParseResult = wDocument.Parse(sJson.c_str());
    
    if (!wParseResult) {
        // Calculate context window for error reporting
        const tInt wDiff = 30;
        size_t wErrorOffset = wParseResult.Offset();
        size_t wStartPos = (wErrorOffset > wDiff) ? wErrorOffset - wDiff : 0;
        size_t wEndPos = std::min(wErrorOffset + wDiff, sJson.length());
        
        std::cerr << "Parsing error: "
                  << rapidjson::GetParseError_En(wParseResult.Code())
                  << " at offset " << wErrorOffset
                  << "\nContext: ..." << sJson.substr(wStartPos, wEndPos - wStartPos) << "..."
                  << std::endl;
        return false;
    }
    
    return ReadJson(wDocument);
}

tBool tJsonSelect::ReadJson(const Value& sDocument) {
    if (!sDocument.IsObject()) {
        return false;
    }
    
    // Read sheet name
    if (sDocument.HasMember("sheet") && sDocument["sheet"].IsString()) {
        m_Sheet = sDocument["sheet"].GetString();
    }
    
    // Read toprow
    if (sDocument.HasMember("toprow") && sDocument["toprow"].IsInt()) {
        m_TopRow = sDocument["toprow"].GetInt();
    }
    
    // Read topcol
    if (sDocument.HasMember("topcol") && sDocument["topcol"].IsInt()) {
        m_TopCol = sDocument["topcol"].GetInt();
    }
    
    // Read cursor
    if (sDocument.HasMember("cursor") && sDocument["cursor"].IsObject()) {
        if (!ReadCursor(sDocument["cursor"])) {
            return false;
        }
    }
    
    // Read selections array
    if (sDocument.HasMember("selections") && sDocument["selections"].IsArray()) {
        if (!ReadSelections(sDocument["selections"])) {
            return false;
        }
    }
    
    // Read selectrows array
    if (sDocument.HasMember("selectrows") && sDocument["selectrows"].IsArray()) {
        if (!ReadSelectRows(sDocument["selectrows"])) {
            return false;
        }
    }
    
    // Read selectcols array
    if (sDocument.HasMember("selectcols") && sDocument["selectcols"].IsArray()) {
        if (!ReadSelectCols(sDocument["selectcols"])) {
            return false;
        }
    }
    
    return true;
}

tBool tJsonSelect::ReadCursor(const Value& sValue) {
    if (!sValue.IsObject()) {
        return false;
    }
    
    if (sValue.HasMember("m_Row") && sValue["m_Row"].IsInt()) {
        m_Cursor.m_Row = sValue["m_Row"].GetInt();
    }
    
    if (sValue.HasMember("m_Col") && sValue["m_Col"].IsInt()) {
        m_Cursor.m_Col = sValue["m_Col"].GetInt();
    }
    
    return true;
}

tBool tJsonSelect::ReadSelections(const Value& sValue) {
    if (!sValue.IsArray()) {
        return false;
    }
    
    m_Selections.clear();
    m_Selections.reserve(sValue.Size());
    
    for (const auto& wSelection : sValue.GetArray()) {
        if (!wSelection.IsObject()) {
            continue;
        }
        
        tJsonSelection wJsonSelection;
        
        if (wSelection.HasMember("m_Row") && wSelection["m_Row"].IsInt()) {
            wJsonSelection.m_Row = wSelection["m_Row"].GetInt();
        }
        
        if (wSelection.HasMember("m_Col") && wSelection["m_Col"].IsInt()) {
            wJsonSelection.m_Col = wSelection["m_Col"].GetInt();
        }
        
        if (wSelection.HasMember("m_Bottom") && wSelection["m_Bottom"].IsInt()) {
            wJsonSelection.m_Bottom = wSelection["m_Bottom"].GetInt();
        }
        
        if (wSelection.HasMember("m_Right") && wSelection["m_Right"].IsInt()) {
            wJsonSelection.m_Right = wSelection["m_Right"].GetInt();
        }
        
        m_Selections.push_back(wJsonSelection);
    }
    
    return true;
}

tBool tJsonSelect::ReadSelectRows(const Value& sValue) {
    if (!sValue.IsArray()) {
        return false;
    }
    
    m_SelectRows.clear();
    m_SelectRows.reserve(sValue.Size());
    
    for (const auto& wSelectRow : sValue.GetArray()) {
        if (!wSelectRow.IsObject()) {
            continue;
        }
        
        tJsonSelectRow wJsonSelectRow;
        
        if (wSelectRow.HasMember("m_Anchor") && wSelectRow["m_Anchor"].IsInt()) {
            wJsonSelectRow.m_Anchor = wSelectRow["m_Anchor"].GetInt();
        }
        
        if (wSelectRow.HasMember("m_Begin") && wSelectRow["m_Begin"].IsInt()) {
            wJsonSelectRow.m_Begin = wSelectRow["m_Begin"].GetInt();
        }
        
        if (wSelectRow.HasMember("m_End") && wSelectRow["m_End"].IsInt()) {
            wJsonSelectRow.m_End = wSelectRow["m_End"].GetInt();
        }
        
        m_SelectRows.push_back(wJsonSelectRow);
    }
    
    return true;
}

tBool tJsonSelect::ReadSelectCols(const Value& sValue) {
    if (!sValue.IsArray()) {
        return false;
    }
    
    m_SelectCols.clear();
    m_SelectCols.reserve(sValue.Size());
    
    for (const auto& wSelectCol : sValue.GetArray()) {
        if (!wSelectCol.IsObject()) {
            continue;
        }
        
        tJsonSelectCol wJsonSelectCol;
        
        if (wSelectCol.HasMember("m_Anchor") && wSelectCol["m_Anchor"].IsInt()) {
            wJsonSelectCol.m_Anchor = wSelectCol["m_Anchor"].GetInt();
        }
        
        if (wSelectCol.HasMember("m_Begin") && wSelectCol["m_Begin"].IsInt()) {
            wJsonSelectCol.m_Begin = wSelectCol["m_Begin"].GetInt();
        }
        
        if (wSelectCol.HasMember("m_End") && wSelectCol["m_End"].IsInt()) {
            wJsonSelectCol.m_End = wSelectCol["m_End"].GetInt();
        }
        
        m_SelectCols.push_back(wJsonSelectCol);
    }
    
    return true;
}

tBool tJsonSelect::Rebase(const tRebasePlan& sRebasePlan) {
    // Rebase toprow
    auto wRebasedTopRow = sRebasePlan.RebaseRow(m_TopRow);
    if (wRebasedTopRow.has_value()) {
        // If the original top row still exists, keep the rebased value
        m_TopRow = wRebasedTopRow.value();
    }
    
    // Rebase topcol
    auto wRebasedTopCol = sRebasePlan.RebaseCol(m_TopCol);
    if (wRebasedTopCol.has_value()) {
        // If the original top column still exists, keep the rebased value
        m_TopCol = wRebasedTopCol.value();
    }
    
    // Rebase cursor
    tPoint wCursorPoint(m_Cursor.m_Row, m_Cursor.m_Col);
    auto wRebasedCursor = sRebasePlan.RebasePoint(wCursorPoint);
    if (wRebasedCursor.has_value()) {
        // Cursor cell still exists, use rebased coordinates
        m_Cursor.m_Row = wRebasedCursor.value().Row();
        m_Cursor.m_Col = wRebasedCursor.value().Col();
    } else {
        // Cursor cell was deleted. Fallback to a consistent location so that
        // UI cursor stays synchronized with the current selection after rebase.
        if (!m_Selections.empty()) {
            // Use the first selection rectangle top‑left as new cursor
            m_Cursor.m_Row = m_Selections.front().m_Row;
            m_Cursor.m_Col = m_Selections.front().m_Col;
        } else if (!m_SelectRows.empty()) {
            // Use the first selected row and keep column aligned with current viewport
            m_Cursor.m_Row = m_SelectRows.front().m_Begin;
            m_Cursor.m_Col = m_TopCol;
        } else if (!m_SelectCols.empty()) {
            // Use the first selected column and keep row aligned with current viewport
            m_Cursor.m_Row = m_TopRow;
            m_Cursor.m_Col = m_SelectCols.front().m_Begin;
        } else {
            // No selection information, reset to origin
            m_Cursor.m_Row = 0;
            m_Cursor.m_Col = 0;
        }
    }
    
    // Rebase selections
    std::vector<tJsonSelection> wRebasedSelections;
    wRebasedSelections.reserve(m_Selections.size());
    
    for (const auto& wSelection : m_Selections) {
        tRect wRect(wSelection.m_Row, wSelection.m_Col, wSelection.m_Bottom, wSelection.m_Right);
        auto wRebasedRect = sRebasePlan.RebaseRect(wRect);
        
        if (!wRebasedRect.has_value()) {
            // Selection was deleted, skip it
            continue;
        }
        
        tJsonSelection wRebasedSelection;
        wRebasedSelection.m_Row = wRebasedRect.value().Top();
        wRebasedSelection.m_Col = wRebasedRect.value().Left();
        wRebasedSelection.m_Bottom = wRebasedRect.value().Bottom();
        wRebasedSelection.m_Right = wRebasedRect.value().Right();
        
        wRebasedSelections.push_back(wRebasedSelection);
    }
    
    m_Selections = std::move(wRebasedSelections);
    
    // Rebase selectrows
    std::vector<tJsonSelectRow> wRebasedSelectRows;
    wRebasedSelectRows.reserve(m_SelectRows.size());
    
    for (const auto& wSelectRow : m_SelectRows) {
        // Rebase anchor
        auto wRebasedAnchor = sRebasePlan.RebaseRow(wSelectRow.m_Anchor);
        if (!wRebasedAnchor.has_value()) {
            continue;
        }
        
        // Rebase begin
        auto wRebasedBegin = sRebasePlan.RebaseRow(wSelectRow.m_Begin);
        if (!wRebasedBegin.has_value()) {
            continue;
        }
        
        // Rebase end
        auto wRebasedEnd = sRebasePlan.RebaseRow(wSelectRow.m_End);
        if (!wRebasedEnd.has_value()) {
            continue;
        }
        
        tJsonSelectRow wRebasedSelectRow;
        wRebasedSelectRow.m_Anchor = wRebasedAnchor.value();
        wRebasedSelectRow.m_Begin = wRebasedBegin.value();
        wRebasedSelectRow.m_End = wRebasedEnd.value();
        
        wRebasedSelectRows.push_back(wRebasedSelectRow);
    }
    
    m_SelectRows = std::move(wRebasedSelectRows);
    
    // Rebase selectcols
    std::vector<tJsonSelectCol> wRebasedSelectCols;
    wRebasedSelectCols.reserve(m_SelectCols.size());
    
    for (const auto& wSelectCol : m_SelectCols) {
        // Rebase anchor
        auto wRebasedAnchor = sRebasePlan.RebaseCol(wSelectCol.m_Anchor);
        if (!wRebasedAnchor.has_value()) {
            continue;
        }
        
        // Rebase begin
        auto wRebasedBegin = sRebasePlan.RebaseCol(wSelectCol.m_Begin);
        if (!wRebasedBegin.has_value()) {
            continue;
        }
        
        // Rebase end
        auto wRebasedEnd = sRebasePlan.RebaseCol(wSelectCol.m_End);
        if (!wRebasedEnd.has_value()) {
            continue;
        }
        
        tJsonSelectCol wRebasedSelectCol;
        wRebasedSelectCol.m_Anchor = wRebasedAnchor.value();
        wRebasedSelectCol.m_Begin = wRebasedBegin.value();
        wRebasedSelectCol.m_End = wRebasedEnd.value();
        
        wRebasedSelectCols.push_back(wRebasedSelectCol);
    }
    
    m_SelectCols = std::move(wRebasedSelectCols);
    
    return true;
}

tString tJsonSelect::WriteJson() const {
    StringBuffer wStringBuffer;
    Writer<StringBuffer> wWriter(wStringBuffer);
    WriteJson(&wWriter);
    return wStringBuffer.GetString();
}

void tJsonSelect::WriteJson(Writer<StringBuffer>* sWriter) const {
    sWriter->StartObject();
    
    // Write sheet name
    sWriter->Key("sheet");
    sWriter->String(m_Sheet.c_str());
    
    // Write toprow
    sWriter->Key("toprow");
    sWriter->Int(m_TopRow);
    
    // Write topcol
    sWriter->Key("topcol");
    sWriter->Int(m_TopCol);
    
    // Write cursor
    sWriter->Key("cursor");
    sWriter->StartObject();
    sWriter->Key("m_Row");
    sWriter->Int(m_Cursor.m_Row);
    sWriter->Key("m_Col");
    sWriter->Int(m_Cursor.m_Col);
    sWriter->EndObject();
    
    // Write selections array
    sWriter->Key("selections");
    sWriter->StartArray();
    for (const auto& wSelection : m_Selections) {
        sWriter->StartObject();
        sWriter->Key("m_Row");
        sWriter->Int(wSelection.m_Row);
        sWriter->Key("m_Col");
        sWriter->Int(wSelection.m_Col);
        sWriter->Key("m_Bottom");
        sWriter->Int(wSelection.m_Bottom);
        sWriter->Key("m_Right");
        sWriter->Int(wSelection.m_Right);
        sWriter->EndObject();
    }
    sWriter->EndArray();
    
    // Write selectrows array
    sWriter->Key("selectrows");
    sWriter->StartArray();
    for (const auto& wSelectRow : m_SelectRows) {
        sWriter->StartObject();
        sWriter->Key("m_Anchor");
        sWriter->Int(wSelectRow.m_Anchor);
        sWriter->Key("m_Begin");
        sWriter->Int(wSelectRow.m_Begin);
        sWriter->Key("m_End");
        sWriter->Int(wSelectRow.m_End);
        sWriter->EndObject();
    }
    sWriter->EndArray();
    
    // Write selectcols array
    sWriter->Key("selectcols");
    sWriter->StartArray();
    for (const auto& wSelectCol : m_SelectCols) {
        sWriter->StartObject();
        sWriter->Key("m_Anchor");
        sWriter->Int(wSelectCol.m_Anchor);
        sWriter->Key("m_Begin");
        sWriter->Int(wSelectCol.m_Begin);
        sWriter->Key("m_End");
        sWriter->Int(wSelectCol.m_End);
        sWriter->EndObject();
    }
    sWriter->EndArray();
    
    sWriter->EndObject();
}

} // namespace SkSpreadSheet


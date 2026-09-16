//=============================================================================
// SkJsonSelect.hpp
//=============================================================================
#ifndef SkJsonSelect_hpp
#define SkJsonSelect_hpp

#include <SkTypes.hpp>
#include <vector>
#include <optional>
#include <rapidjson/document.h>
#include "SkTools.hpp"
#include "SkUndoRedoRebase.hpp"

using namespace SkRoot;
using namespace rapidjson;

namespace SkSpreadSheet {

// Structure to represent a selection rectangle
struct tJsonSelection {
    tIndex m_Row {0};
    tIndex m_Col {0};
    tIndex m_Bottom {0};
    tIndex m_Right {0};
    
    tJsonSelection() = default;
    tJsonSelection(tIndex sRow, tIndex sCol, tIndex sBottom, tIndex sRight)
        : m_Row(sRow), m_Col(sCol), m_Bottom(sBottom), m_Right(sRight) {}
};

// Structure to represent a column selection
struct tJsonSelectCol {
    tIndex m_Anchor {0};
    tIndex m_Begin {0};
    tIndex m_End {0};
    
    tJsonSelectCol() = default;
    tJsonSelectCol(tIndex sAnchor, tIndex sBegin, tIndex sEnd)
        : m_Anchor(sAnchor), m_Begin(sBegin), m_End(sEnd) {}
};

// Structure to represent a row selection
struct tJsonSelectRow {
    tIndex m_Anchor {0};
    tIndex m_Begin {0};
    tIndex m_End {0};
    
    tJsonSelectRow() = default;
    tJsonSelectRow(tIndex sAnchor, tIndex sBegin, tIndex sEnd)
        : m_Anchor(sAnchor), m_Begin(sBegin), m_End(sEnd) {}
};

// Structure to represent cursor position
struct tJsonCursor {
    tIndex m_Row {0};
    tIndex m_Col {0};
    
    tJsonCursor() = default;
    tJsonCursor(tIndex sRow, tIndex sCol) : m_Row(sRow), m_Col(sCol) {}
};

// Main class to read and rebase JSON select format
class tJsonSelect {
private:
    tString m_Sheet;
    tIndex m_TopRow {0};
    tIndex m_TopCol {0};
    tJsonCursor m_Cursor;
    std::vector<tJsonSelection> m_Selections;
    std::vector<tJsonSelectRow> m_SelectRows;
    std::vector<tJsonSelectCol> m_SelectCols;
    
    /// @brief      Read cursor from JSON
    /// @param[in]  sValue const rapidjson::Value& cursor object
    /// @return     tBool true if parsing succeeded
    tBool ReadCursor(const Value& sValue);

    /// @brief      Read selections array from JSON
    /// @param[in]  sValue const rapidjson::Value& selections array
    /// @return     tBool true if parsing succeeded
    tBool ReadSelections(const Value& sValue);

    /// @brief      Read selectrows array from JSON
    /// @param[in]  sValue const rapidjson::Value& selectrows array
    /// @return     tBool true if parsing succeeded
    tBool ReadSelectRows(const Value& sValue);

    /// @brief      Read selectcols array from JSON
    /// @param[in]  sValue const rapidjson::Value& selectcols array
    /// @return     tBool true if parsing succeeded
    tBool ReadSelectCols(const Value& sValue);
public:


    /// @brief      Default constructor
    tJsonSelect() = default;

    /// @brief      Read JSON from string
    /// @param[in]  sJson tString containing JSON data
    /// @return     tBool true if parsing succeeded, false otherwise
    tBool ReadJson(tString sJson);

    /// @brief      Read JSON from RapidJSON Document
    /// @param[in]  sDocument const rapidjson::Value& JSON document
    /// @return     tBool true if parsing succeeded, false otherwise
    tBool ReadJson(const Value& sDocument);

    /// @brief      Apply rebase plan to all elements
    /// @param[in]  sRebasePlan const tRebasePlan& rebase plan to apply
    /// @return     tBool true if rebase succeeded, false if any element was deleted
    tBool Rebase(const tRebasePlan& sRebasePlan);

    /// @brief      Write JSON to string
    /// @return     tString JSON representation
    tString WriteJson() const;

    /// @brief      Write JSON to RapidJSON Writer
    /// @param[in]  sWriter Writer<StringBuffer>* JSON writer
    void WriteJson(Writer<StringBuffer>* sWriter) const;

};

} // namespace SkSpreadSheet

#endif // SkJsonSelect_hpp

//
//  SkFormatApi.hpp
//  SkRoot
//
//  Created by stephane allez on 25/12/2023.
//
#ifndef SkFormatApi_hpp
#define SkFormatApi_hpp

#include "SkFormatString.hpp"

// If Debug Check (CMake may already pass -Dcheckfo on the command line)
#ifdef _DEBUGSK
#ifndef checkfo
    #define checkfo
#endif
#endif


namespace SkRoot {

// Binary for mask (multiple combination)
// tBorderAll        : every cell of the range gets its 4 sides (Excel "All Borders")
// tBorderOutside    : only the range perimeter (Excel "Outside Borders")
// tBorderInside     : only the interior grid lines (H + V), excluding the perimeter
// tBorderHorizontal : only the interior horizontal lines
// tBorderVertical   : only the interior vertical lines
const tShort tBorderAll        = 1;
const tShort tBorderLeft       = 2;
const tShort tBorderTop        = 4;
const tShort tBorderRight      = 8;
const tShort tBorderBottom     = 16;
const tShort tBorderOutside    = 32;
const tShort tBorderInside     = 64;
const tShort tBorderHorizontal = 128;
const tShort tBorderVertical   = 256;


// Referance to format
typedef tAllocatorRef tFormatRef;
typedef std::vector<tFormatRef> tVectorFormatRef;


class tFormatApi : public tVirtualClass {
public:
    /// @brief Constructor
    tFormatApi();
    
    /// @brief Destructor
    virtual ~tFormatApi();
    
    /// @brief Clear the format API
    virtual void Clear();
    
    /// @brief Compile format
    /// @param[in] sValue tString
    /// @return tBool
    virtual tBool Compil(tString sValue);

    /// @brief Apply cell format
    /// @param[in] sValue tString
    /// @return tFormatRef
    virtual tFormatRef ApplyCellFormat(tString sValue);
    
    /// @brief return json cell format
    /// @param sFormatRef 
    /// @return tString
    virtual tString Format2Json(tFormatRef sFormatRef);
    
    /// @brief return json cell format
    /// @param sJson tString
    /// @return tString
    virtual tString Json2Format(tString sJson);
    
    /// @brief Begin merge operation
    virtual void BeginMerge();
 
    /// @brief Merge cell format
    /// @param[in] sAllocatorRef tFormatRef
    virtual void Merge(tFormatRef sAllocatorRef);

    /// @brief Apply merged format
    /// @return tFormatRef
    virtual tFormatRef ApplyMerge();
        
    /// @brief Delete cell format
    /// @param[in] sAllocatorRef tFormatRef
    /// @return tBool
    virtual tBool DeleteCellFormat(tFormatRef sAllocatorRef);
    
    /// @brief Increment cell format
    /// @param[in] sAllocatorRef tFormatRef
    virtual void IncCellFormat(tFormatRef sAllocatorRef);

    /// @brief Return cell refcount on a format pool entry.
    /// @param[in] sAllocatorRef tFormatRef
    /// @return tInt
    virtual tInt CellFormatRefCount(tFormatRef sAllocatorRef);
    
    // Border =============================================================
    /// @brief Delete border
    /// @param[in] sFormatRef tFormatRef
    /// @param[in] sBorderMask tShort
    /// @return tFormatRef
    virtual tFormatRef DeleteBorder(tFormatRef sFormatRef, tShort sBorderMask);
      
    /// @brief Return border mask
    /// @param[in] sFormatRef tFormatRef
    /// @return tShort
    virtual tShort BorderMask(tFormatRef sFormatRef);
    
    /// @brief Return cell format
    /// @param[in] sAllocatorRef tFormatRef
    /// @param[in] sReturn tBool
    /// @return tString
    virtual tString CellFormat(tFormatRef sAllocatorRef, tBool sReturn = false);
    
    /// @brief Return merged cell format
    /// @param[in] sVectorFormatRef tVectorFormatRef*
    /// @param[in] sReturn tBool
    /// @return tString
    virtual tString CellFormat(tVectorFormatRef* sVectorFormatRef, tBool sReturn = false);

    /// @brief Write JSON for React
    /// @param[in] sAllocatorRef tFormatRef
    /// @param[in] sVariant tVariant*
    /// @param[in] sCss tBool (CSS or Canvas)
    /// @param[in] sWriter Writer<StringBuffer>*
    virtual void _JsonJavaScript(tFormatRef sAllocatorRef, tVariant* sVariant, tBool sCss, rapidjson::Writer<rapidjson::StringBuffer>* sWriter);

    /// @brief Read JSON for React
    /// @param[in] sVectorFormatRef tVectorFormatRef*
    /// @param[in] sVariant tVariant*
    /// @param[in] sCss tBool (CSS or Canvas)
    /// @param[in] sWriter Writer<StringBuffer>* sWriter);
    virtual void _JsonJavaScript(tVectorFormatRef* sVectorFormatRef, tVariant* sVariant, tBool sCss, rapidjson::Writer<rapidjson::StringBuffer>* sWriter);

    /// @brief Write display value keys only (f_value, f_mo).
    virtual void _JsonJavaScriptVariantDisplay(tVariant* sVariant, tBool sCss, rapidjson::Writer<rapidjson::StringBuffer>* sWriter);

    /// @brief Write f_value/f_mo after merging sheet/col/row/cell format refs (JsonView f_i path).
    virtual void _JsonJavaScriptMergedVariantDisplay(tVectorFormatRef* sVectorFormatRef, tVariant* sVariant, tBool sCss, rapidjson::Writer<rapidjson::StringBuffer>* sWriter);

    /// @brief Write merged style keys only (no f_value) for JsonView format-table dedup.
    virtual void _JsonJavaScriptMergedStyle(tVectorFormatRef* sVectorFormatRef, tBool sCss, rapidjson::Writer<rapidjson::StringBuffer>* sWriter);

    /// @brief Default f_ah on cell for plain strings (JsonView f_i path; mirrors JsonJavaScript).
    ///        Variant-dependent — not stored in formats[] dedup table.
    virtual void _JsonJavaScriptDefaultStringAlign(tVariant* sVariant, rapidjson::Writer<rapidjson::StringBuffer>* sWriter);

    /// @brief Return cell format string
    /// @param[in] sAllocatorRef tFormatRef
    /// @param[in] sVariant tVariant*
    /// @param[in] sReturn tBool
    /// @return tString
    virtual tString CellFormatString(tFormatRef sAllocatorRef, tVariant* sVariant, tBool sReturn = false);
    
    /// @brief Return merged cell format string
    /// @param[in] sVectorFormatRef tVectorFormatRef*
    /// @param[in] sVariant tVariant*
    /// @param[in] sReturn tBool
    /// @return tString
    virtual tString CellFormatString(tVectorFormatRef* sVectorFormatRef, tVariant* sVariant, tBool sReturn = false);
    
    /// @brief Return cell border mask (for ClipRect)
    /// @param[in] sAllocatorRef tFormatRef
    /// @return tShort
    virtual tShort CellBorder(tFormatRef sAllocatorRef);
    
    /// @brief Return merged cell border mask
    /// @param[in] sVectorFormatRef tVectorFormatRef*
    /// @return tShort
    virtual tShort CellBorder(tVectorFormatRef* sVectorFormatRef);

    /// @brief Write a single border side from the merged formats using a custom JSON key.
    ///        Used to project a neighbor's border onto a merged cell's anchor so the
    ///        frontend renders border-right / border-bottom on the merged cell.
    /// @param[in] sVectorFormatRef tVectorFormatRef*
    /// @param[in] sBorderSide one of tBorderLeft, tBorderTop, tBorderRight, tBorderBottom
    /// @param[in] sKey JSON key to emit (e.g. "f_bor", "f_bob")
    /// @param[in] sCss true for CSS string, false for canvas object
    /// @param[in] sWriter Writer<StringBuffer>*
    virtual void JsonJavaScriptBorderSide(tVectorFormatRef* sVectorFormatRef, tShort sBorderSide, tString sKey, tBool sCss, rapidjson::Writer<rapidjson::StringBuffer>* sWriter);

    /// @brief Merged border side as ApplyCell CSS (e.g. "border-left:solid 1px black;").
    virtual tString MergedBorderSideApplyCss(tVectorFormatRef* sVectorFormatRef, tShort sBorderSide);
    
    /// @brief Return cell format string
    /// @param[in] sAllocatorRef tFormatRef
    /// @return tFormatString*
    virtual tFormatString* CellFormatString(tFormatRef sAllocatorRef);
    
    /// @brief Return merged cell format string
    /// @param[in] sVectorFormatRef tVectorFormatRef*
    /// @return tFormatString*
    virtual tFormatString* CellFormatString(tVectorFormatRef* sVectorFormatRef);
    
    /// @brief		Set explanation of the error.
    /// @param[in]  sRow tInt
    /// @param[in]  sCol  tInt
    /// @param[in]	sLexerError tString
    virtual void Error(tString sLexerError,tInt sRow,tInt sCol);

    /// @brief		Returnf  error.
    /// @return		tString
    virtual tString Error();

    /// @brief		Return error Column
    /// @return		tInt
    virtual tInt ErrorColumn();

    /// @brief		Return error Line
    /// @return		tInt
    virtual tInt ErrorLine();

    /// @brief        Return explanation of the error.
    /// @return       tString
    virtual tString ErrorWithDetail();
    
    // Json ===============================================================
    /// @brief Begin writing JSON
    virtual void BeginWriteJson();
 
    /// @brief Add format to JSON
    /// @param[in] sFormatRef tFormatRef
    /// @return tSize
    virtual tSize WriteJsonAddFormat(tFormatRef sFormatRef);
    
    /// @brief Get format from JSON
    /// @param[in] sIndex tSize
    /// @return tString
    virtual tString ReadJsonGetFormat(tSize sIndex);
    
    /// @brief Write JSON
    /// @param[in] sWriter Writer<StringBuffer>*
    void Json(rapidjson::Writer<rapidjson::StringBuffer>* sWriter) override;

    /// @brief Read JSON
    /// @param[in] sValue Value&
    void Json(const rapidjson::Value& sValue) override;

#ifdef checkfo
    /// @brief Reset check
    virtual void ResetCheck();

    /// @brief Increment check
    /// @param[in] sAllocatorRef tFormatRef
    virtual void IncCheck(tFormatRef sAllocatorRef);

    /// @brief Perform check
    virtual void Check();
#endif

#ifdef _DEBUGSK
    /// @brief Debug format
    /// @return tString
    virtual tString Debug() override;
    
    /// @brief Debug format
    /// @param[in] sFormatRef tFormatRef
    /// @return tString
    virtual tString Debug(tFormatRef sFormatRef);
#endif
};

}

#endif /* SkFormatApi_h */

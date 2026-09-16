//=============================================================================
//  SkFormatApi.hpp
//  SkFormat
//
//  Created by stephane allez on 19/12/2023.
//=============================================================================
#ifndef SkFormatCssApi_hpp
#define SkFormatCssApi_hpp

#include <rapidjson/document.h>
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <rapidjson/error/en.h> 

#include <SkApplication.hpp>
#include <SkFormatApi.hpp>
#include "SkFormatRoot.hpp"
#include "SkLemonFormatReserved.hpp"
#include "SkLemonFormat.hpp"
#include "SkLemonFormatInterface.hpp"

#include "SkFormatShare.hpp"

using namespace SkRoot;

namespace SkFormat {

//=============================================================================
//! Low level API with Spreadsheet
class tFormatCssApi : public SkRoot::tFormatApi {
private:
    tFormatRoot*            m_FormatRoot;
    tLemonFormatInterface*  m_LemonInterface;
public:
    /// @brief      Constructor SkApi.
    tFormatCssApi();

    /// @brief      Destructor SkApi.
    ~tFormatCssApi() override;
    
    /// @brief      Celar
    void Clear() override;


    /// @brief      Compil
    /// @param [in] sValue tString
    tBool Compil(tString sValue) override;

    /// @brief      Apply Format
    /// @param [in] sValue tString
    tFormatRef ApplyCellFormat(tString sValue) override;

    /// @brief return json cell format
    /// @param sFormatRef 
    /// @return tString
    tString Format2Json(tFormatRef sFormatRef) override;
    
     /// @brief return json cell format
    /// @param sJson tString
    /// @return tString
    virtual tString Json2Format(tString sJson) override;
    
    /// @brief      Begin Merge
    void BeginMerge() override;
 
    /// @brief      Inc Cell format Compteur
    /// @param [in] sFormatRef tFormatRef
    void Merge(tFormatRef sFormatRef) override;

    /// @brief   Apply Format
    /// @return  tFormatRef
    tFormatRef ApplyMerge() override;
        
    /// @brief      Delete  Format
    /// @param[in] sFormatRef  tFormatRef
    /// @return tBool.
    tBool DeleteCellFormat(tFormatRef sFormatRef) override;
    
    /// @brief      Delete  Format
    /// @param[in] sFormatRef  tFormatRef
    /// @return tBool.
    void IncCellFormat(tFormatRef sFormatRef) override;

    /// @brief Return cell refcount on a format pool entry.
    tInt CellFormatRefCount(tFormatRef sFormatRef) override;
    
    /// @brief      Return foirmat
    /// @param[in] sFormatRef  tFormatRef
    /// @param[in] sItem tSring
    /// @param[in]  sReturn tBool
    /// @return tString
    tString CellFormat(tFormatRef sFormatRef,tBool sReturn=false) override;
    
    /// @brief      Return Format Merged
    /// @param[in] sVectorFormatRef tVectorFormatRef*
    /// @param[in] sItem tSring (color, etc ..)
    /// @param[in]  sReturn tBool
    /// @return tString
    tString CellFormat(tVectorFormatRef* sVectorFormatRef,tBool sReturn=false) override;
  
  
    // Border =============================================================
    /// @brief     Delete Border
    /// @param[in] sFormatRef  tFormatRef
    /// @param[in] sBorderMask  tShort
    /// @return tFormatRef
    tFormatRef DeleteBorder(tFormatRef sFormatRef,tShort sBorderMask) override;
      
    /// @brief     Return Border mask
    /// @param[in] sFormatRef  tFormatRef
    /// @return  tShort
    tShort BorderMask(tFormatRef sFormatRef) override;
    
    // Json ===============================================================
    /// @brief Begin WriteJson
    void BeginWriteJson() override;

    /// @brief WriteJsonAddFormat
    /// @param[in] sFormatRef tFormatRef
    /// @return tSize
    tSize WriteJsonAddFormat(tFormatRef sFormatRef) override;
    
    /// @brief ReadJsonGetFormat
    /// @param[in] sFormatRef tFormatRef
    /// @return tSize
    tString ReadJsonGetFormat(tSize sIndex) override;
    
    /// @brief        Writer Json.
    /// @param[in]    sWriter Writer<StringBuffer>*
    void Json(Writer<StringBuffer>* sWriter) override;

    /// @brief        Reader Json.
    /// @param[in]    sValue Value&
    void Json(const rapidjson::Value& sValue) override;
    
    
    // Json ===============================================================
    /// @brief      Writer Json for react.
    /// @param [in] sAllocatordRef  tFormatRef
    /// @param [in] sVariant tVariant*
    /// @param [in] sCss tBool (Css or Canvas)
    /// @param[in]  sWriter Writer<StringBuffer>*
    void _JsonJavaScript(tFormatRef sAllocatorRef,tVariant* sVariant,tBool sCss, Writer<StringBuffer>* sWriter) override;

    /// @brief      Reader  Json for react.
    /// @param [in] sVectorFormatRef tVectorFormatRef*
    /// @param [in] sVariant tVariant*
    /// @param [in] sCss tBool (Css or Canvas)
    /// @param[in]  sWriter Writer<StringBuffer>*
    void _JsonJavaScript(tVectorFormatRef* sVectorFormatRef,tVariant* sVariant,tBool sCss,Writer<StringBuffer>* sWriter) override;

    void _JsonJavaScriptVariantDisplay(tVariant* sVariant, tBool sCss, Writer<StringBuffer>* sWriter) override;

    void _JsonJavaScriptMergedVariantDisplay(tVectorFormatRef* sVectorFormatRef, tVariant* sVariant, tBool sCss, Writer<StringBuffer>* sWriter) override;

    void _JsonJavaScriptMergedStyle(tVectorFormatRef* sVectorFormatRef, tBool sCss, Writer<StringBuffer>* sWriter) override;

    void _JsonJavaScriptDefaultStringAlign(tVariant* sVariant, Writer<StringBuffer>* sWriter) override;
    
    /// @brief      Return format
    /// @param [in] sAllocatordRef  tFormatRef
    /// @param [in] sVariant tVariant
    /// @param[in]  sReturn tString
    /// @return tString
    tString CellFormatString(tFormatRef sAllocatorRef,tVariant* sVariant,tBool sReturn=false) override;
    
    /// @brief      Return Format Merged
    /// @param [in] sVectorFormatRef tVectorFormatRef*
    /// @param [in] sVariant tVariant
    /// @param[in]  sReturn tString
    tString CellFormatString(tVectorFormatRef* sVectorFormatRef,tVariant* sVariant, tBool sReturn=false) override;
    
    
    /// @brief      Return MaskBorder (For ClipRect)
    /// @param [in] sAllocatordRef  tFormatRef
    /// @return tShort
    tShort CellBorder(tFormatRef sAllocatorRef) override;
    
    /// @brief      Return Format Merged
    /// @param [in] sVectorFormatRef tVectorFormatRef*
    /// @return tShort
    tShort CellBorder(tVectorFormatRef* sVectorFormatRef) override;

    /// @brief      Write one side of the merged border under a custom JSON key.
    /// @param [in] sVectorFormatRef tVectorFormatRef*
    /// @param [in] sBorderSide tBorderLeft/tBorderTop/tBorderRight/tBorderBottom
    /// @param [in] sKey        JSON key (e.g. "f_bor", "f_bob")
    /// @param [in] sCss        CSS (true) or canvas object (false)
    /// @param [in] sWriter     Writer<StringBuffer>*
    void JsonJavaScriptBorderSide(tVectorFormatRef* sVectorFormatRef, tShort sBorderSide, tString sKey, tBool sCss, rapidjson::Writer<rapidjson::StringBuffer>* sWriter) override;

    /// @brief Merged border side as ApplyCell CSS (e.g. "border-left:solid 1px black;").
    tString MergedBorderSideApplyCss(tVectorFormatRef* sVectorFormatRef, tShort sBorderSide) override;
    
    /// @brief      Return FormatString
    /// @param [in] sAllocatordRef  tFormatRef
    /// @return     tFormatString*
    tFormatString* CellFormatString(tFormatRef sAllocatorRef) override;
    
    /// @brief      Return FormatString
    /// @param [in] sVectorFormatRef tVectorFormatRef*
    /// @return     tFormatString*
    tFormatString* CellFormatString(tVectorFormatRef* sVectorFormatRef) override;

    /// @brief		Set explanation of the error.
    /// @param[in]  sRow tInt
    /// @param[in]  sCol  tInt
    /// @param[in]	sLexerError tString
    void Error(tString sLexerError,tInt sRow,tInt sCol) override;

    /// @brief      Return Error
    /// @return     tString
    tString Error() override;

    /// @brief      Return Error Column
    /// @return     tInt
    tInt ErrorColumn() override;    

    /// @brief      Return Error Line
    /// @return     tInt
    tInt ErrorLine() override;      

    /// @brief      Return Error With Detail
    /// @return     tString
    tString ErrorWithDetail() override;
    
    ///  @brief Return number of Format
    ///  @return fFormatRef
    tFormatRef Count();
    
#ifdef checkfo
    /// @brief      Method reset check
    void ResetCheck() override;

    /// @brief      Method increments check
    /// @param [in] sAllocatordRef  tFormatRef
    void IncCheck(tFormatRef sAllocatorRef) override;

    /// @brief      check
    void Check() override;
#endif

#ifdef _DEBUGSK
    /// @brief  Debug Format
    /// @return tString
    tString Debug() override;
    /// @brief Debug format
    /// @param[in] sFormatRef tFormatRef
    /// @return tString
    tString Debug(tFormatRef sFormatRef) override;
#endif

};

tFormatCssApi* FormatApi();

}

#endif /* SkFormatApi_hpp */

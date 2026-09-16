//
//  SkFormatApi.cpp
//  SkRoot
//
//  Created by stephane allez on 25/12/2023.
//
#include <stdio.h>
#include "../include/SkFormatApi.hpp"
#include "../include/SkVariant.hpp"
#include "../include/SkFormatString.hpp"
namespace SkRoot {

    // tFormat Api=================================================================
    tFormatApi::tFormatApi() : tVirtualClass() {}
    tFormatApi::~tFormatApi() {}

    void tFormatApi::Clear() {}

    tBool tFormatApi::Compil(tString sValue) { return(true); }

    tFormatRef tFormatApi::ApplyCellFormat(tString sValue) { return(0); }

    tString tFormatApi::Format2Json(tFormatRef sFormatRef) { return(""); }
    
    tString tFormatApi::Json2Format(tString sJson) { return(""); }

    void tFormatApi::BeginMerge() {}

    void tFormatApi::Merge(tFormatRef sAllocatorRef) {  }

    tFormatRef tFormatApi::ApplyMerge() { return(0); }

    tBool tFormatApi::DeleteCellFormat(tFormatRef sAllocatorRef) { return(false); }

    void tFormatApi::IncCellFormat(tFormatRef sAllocatorRef) { }

    tInt tFormatApi::CellFormatRefCount(tFormatRef sAllocatorRef) { return(0); }

    tFormatRef tFormatApi::DeleteBorder(tFormatRef sFormatRef,tShort sBorderMask) { return(0); }

    tShort tFormatApi::BorderMask(tFormatRef sFormatRef) {return(0); }
  
    tString tFormatApi::CellFormat(tFormatRef sAllocatorRef,tBool sReturn) { return(""); }

    tString tFormatApi::CellFormat(tVectorFormatRef* sVectorFormatRef,tBool sReturn) { return(""); }

    void tFormatApi::_JsonJavaScript(tFormatRef sAllocatorRef,tVariant* sVariant,tBool sCss,Writer<StringBuffer>* sWriter) {}

    void tFormatApi::_JsonJavaScript(tVectorFormatRef* sVectorFormatRef,tVariant* sVariant,tBool sCss,Writer<StringBuffer>* sWriter) {}

    void tFormatApi::_JsonJavaScriptVariantDisplay(tVariant* sVariant, tBool sCss, Writer<StringBuffer>* sWriter) {}

    void tFormatApi::_JsonJavaScriptMergedVariantDisplay(tVectorFormatRef* sVectorFormatRef, tVariant* sVariant, tBool sCss, Writer<StringBuffer>* sWriter) {}

    void tFormatApi::_JsonJavaScriptMergedStyle(tVectorFormatRef* sVectorFormatRef, tBool sCss, Writer<StringBuffer>* sWriter) {}

    void tFormatApi::_JsonJavaScriptDefaultStringAlign(tVariant* sVariant, Writer<StringBuffer>* sWriter) {}

    tString tFormatApi::CellFormatString(tFormatRef sAllocatorRef,tVariant* sVariant,tBool sReturn) {
        if (sVariant!=nullptr) return(sVariant->Str());
        return("");
    }

    tString tFormatApi::CellFormatString(tVectorFormatRef* sVectorFormatRef,tVariant* sVariant, tBool sReturn) {
        if (sVariant!=nullptr) return(sVariant->Str());
        return("");
    }

    tShort tFormatApi::CellBorder(tFormatRef sAllocatorRef) { return(0); }

    tShort tFormatApi::CellBorder(tVectorFormatRef* sVectorFormatRef) { return(0); }

    void tFormatApi::JsonJavaScriptBorderSide(tVectorFormatRef* sVectorFormatRef, tShort sBorderSide, tString sKey, tBool sCss, Writer<StringBuffer>* sWriter) {}

    tString tFormatApi::MergedBorderSideApplyCss(tVectorFormatRef* sVectorFormatRef, tShort sBorderSide) { (void)sVectorFormatRef; (void)sBorderSide; return(""); }

    tFormatString* tFormatApi::CellFormatString(tFormatRef sAllocatorRef) { return(nullptr); }

    tFormatString* tFormatApi::CellFormatString(tVectorFormatRef* sVectorFormatRef) { return(nullptr); }

    tString tFormatApi::ErrorWithDetail() { return(""); }

    void tFormatApi::Error(tString sLexerError,tInt sRow,tInt sCol) {}

    tString tFormatApi::Error() { return(""); }

    tInt tFormatApi::ErrorColumn() { return(0); }

    tInt tFormatApi::ErrorLine() { return(0); }

    void tFormatApi::BeginWriteJson() {}

    tSize tFormatApi::WriteJsonAddFormat(tFormatRef sFormatRef) { return(0); }

    tString tFormatApi::ReadJsonGetFormat(tSize sIndex) { return(""); }

    void tFormatApi::Json(Writer<StringBuffer>* sWriter) {}
    void tFormatApi::Json(const rapidjson::Value& sValue) {};

#ifdef checkfo
    void tFormatApi::ResetCheck() {};
    void tFormatApi::IncCheck(tFormatRef sAllocatorRef) {};

    void tFormatApi::Check() {}

#endif

#ifdef _DEBUGSK
    tString tFormatApi::Debug() { return ("Debug not implemented"); }
    tString tFormatApi::Debug(tFormatRef sFormatRef) { return("Debug not implemented"); }
#endif
} // end of namespace

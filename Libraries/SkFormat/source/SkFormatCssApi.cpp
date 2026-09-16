//
//  SkFormatApi.cpp
//  SkFormat
//
//  Created by stephane allez on 19/12/2023.
//
#include "../include/SkFormatCssApi.hpp"
#include "../include/SkText.hpp"
#include "rapidjson/error/en.h"

#define _debugformatapi

#ifdef _DEBUGSK
// Build flag: _DEBUGSK active
#endif
#ifdef _RELEASE
// Build flag: _RELEASE active
#endif
#ifdef checkfo
// Build flag: checkfo active
#endif

namespace SkFormat {

    tFormatCssApi::tFormatCssApi() : SkRoot::tFormatApi() {
        m_FormatRoot=tFormatRoot::Instance();
        m_LemonInterface=new tLemonFormatInterface();
    }

    tFormatCssApi::~tFormatCssApi() {
        Clear();
        delete(m_LemonInterface);
    }

    void tFormatCssApi::Clear() {
        m_FormatRoot->Clear();
        m_LemonInterface->Clear();
    }
    
    tBool tFormatCssApi::Compil(tString sValue) {
        return(m_LemonInterface->Compil(sValue.c_str()));
    }

    tFormatRef tFormatCssApi::ApplyCellFormat(tString sValue) {
#ifdef debugformatapi
        cout << "tFormatCssApi::ApplyCellFormat(" << sValue << ") ";
#endif
        tString wCompil="ApplyCell{"+sValue+"}";
        
        if (m_LemonInterface->Compil(wCompil.c_str())) {
            tFormatRef wCellAlloc=m_LemonInterface->CellAlloc();
#ifdef debugformatapi
            cout << " Ok " << wCellAlloc << endl;;
#endif
            return(wCellAlloc);
        }
#ifdef debugformatapi
        cout << m_LemonInterface->ErrorWithDetail() << endl;
#endif
        return(0);
    }


    void ShowParseErrorJson(Document* sDocJson,tString sJsonStr) {
        cerr << "JSON parsing error at position " << sDocJson->GetErrorOffset() << endl;
        cerr << "Error: " << rapidjson::GetParseError_En(sDocJson->GetParseError()) << endl;
        
        // Display context around the error
        size_t errorPos = sDocJson->GetErrorOffset();
        size_t start = (errorPos > 20) ? errorPos - 20 : 0;
        size_t length = 40; // Display 40 characters around the error
        if (start + length > sJsonStr.length()) {
            length = sJsonStr.length() - start;
        }
        
        cerr << "Context: ..." << sJsonStr.substr(start, length) << "..." << endl;
        cerr << "         " << string(errorPos - start, ' ') << "^" << endl;
    }


    tString tFormatCssApi::Format2Json(tFormatRef sFormatRef) {
        tFormatCss* wFormatCss=m_FormatRoot->FormatRef(sFormatRef);
        if (wFormatCss!=nullptr) {
            rapidjson::StringBuffer wBuffer;
            rapidjson::Writer<rapidjson::StringBuffer> wWriter(wBuffer);
            wFormatCss->Json(&wWriter,m_FormatRoot);
            return(wBuffer.GetString());
        }
        return("");
    }

    tString tFormatCssApi::Json2Format(tString sString) {
        rapidjson::Document wDocument;
        rapidjson::ParseResult wParseResult = wDocument.Parse(sString.c_str());
        if (wParseResult.IsError()) {
            cerr << "Failed to parse Message JSON" << endl;
            ShowParseErrorJson(&wDocument,sString);
                
            throw tExceptionInternalError("Failed to parse Message JSON");
        }
        tFormatCss* wFormatCss=m_FormatRoot->AllocFormat("#tempo");
        wFormatCss->Json(wDocument, m_FormatRoot);
        //cout << wFormatCss->Debug(m_FormatRoot) << endl;
        tString wReturn=wFormatCss->Str(m_FormatRoot);
        
        m_FormatRoot->DeleteFormat("#tempo");
        
        return(wReturn);
    }

    void tFormatCssApi::BeginMerge() {
        m_FormatRoot->BeginMerge();
    }

    void tFormatCssApi::Merge(tFormatRef sFormatRef) {
        m_FormatRoot->Merge(sFormatRef);
    }

    tFormatRef tFormatCssApi::ApplyMerge() {
        return(m_FormatRoot->ApplyMerge());
    }

    tBool tFormatCssApi::DeleteCellFormat(tFormatRef sFormatRef) {
        if (sFormatRef!=0) {
            m_FormatRoot->DeleteCell(sFormatRef);
            return(true);
        }
        return(false);
    }

    void tFormatCssApi::IncCellFormat(tFormatRef sFormatRef) {
        if (sFormatRef != 0) {
            m_FormatRoot->IncCell(sFormatRef);
        }
    }

    tInt tFormatCssApi::CellFormatRefCount(tFormatRef sFormatRef) {
        if (sFormatRef == 0) {
            return(0);
        }
        tFormatCss* wFormatCss = m_FormatRoot->FormatRef(sFormatRef);
        if (wFormatCss == nullptr) {
            return(0);
        }
        return(wFormatCss->CountCell());
    }

    tString tFormatCssApi::CellFormat(tFormatRef sFormatRef,tBool sReturn) {
        tFormatCss* wFormatCss=m_FormatRoot->FormatRef(sFormatRef);
        if (wFormatCss!=nullptr) {
            return(wFormatCss->Str(m_FormatRoot, sReturn));
        }
        return("");
    };

    tString tFormatCssApi::CellFormat(tVectorFormatRef* sVectorFormatRef,tBool sReturn) {
        BeginMerge();
        for(auto wFormatRef : *sVectorFormatRef) {
            Merge(wFormatRef);
        }
        return(m_FormatRoot->FormatMerge()->Str(sReturn));
    }


    tFormatRef tFormatCssApi::Count() {
        return(m_FormatRoot->Count());
    }

    tFormatRef tFormatCssApi::DeleteBorder(tFormatRef sInstance,tShort sBorderMask) {
        return(m_FormatRoot->DeleteBorder(sInstance, sBorderMask));
    }

    tShort tFormatCssApi::BorderMask(tFormatRef sFormatRef) {
        return(m_FormatRoot->BorderMask(sFormatRef));
    }

    void tFormatCssApi::_JsonJavaScript(tFormatRef sAllocatorRef,tVariant* sVariant,tBool sCss,Writer<StringBuffer>* sWriter) {
        BeginMerge();
        if (sAllocatorRef!=0) {
            Merge(sAllocatorRef);
        }
        m_FormatRoot->FormatMerge()->JsonJavaScript(sWriter,sVariant,sCss);
    }

    void tFormatCssApi::_JsonJavaScript(tVectorFormatRef* sVectorFormatRef,tVariant* sVariant,tBool sCss,Writer<StringBuffer>* sWriter) {
        BeginMerge();
        for(auto wFormatRef : *sVectorFormatRef) {
            Merge(wFormatRef);
        }
        m_FormatRoot->FormatMerge()->JsonJavaScript(sWriter,sVariant,sCss);
    }

    void tFormatCssApi::_JsonJavaScriptVariantDisplay(tVariant* sVariant, tBool sCss, Writer<StringBuffer>* sWriter) {
        BeginMerge();
        m_FormatRoot->FormatMerge()->JsonJavaScriptVariantDisplay(sWriter, sVariant, sCss);
    }

    void tFormatCssApi::_JsonJavaScriptMergedVariantDisplay(
        tVectorFormatRef* sVectorFormatRef,
        tVariant* sVariant,
        tBool sCss,
        Writer<StringBuffer>* sWriter) {
        BeginMerge();
        if (sVectorFormatRef != nullptr) {
            for (auto wFormatRef : *sVectorFormatRef) {
                Merge(wFormatRef);
            }
        }
        m_FormatRoot->FormatMerge()->JsonJavaScriptVariantDisplay(sWriter, sVariant, sCss);
    }

    void tFormatCssApi::_JsonJavaScriptMergedStyle(tVectorFormatRef* sVectorFormatRef, tBool sCss, Writer<StringBuffer>* sWriter) {
        BeginMerge();
        for (auto wFormatRef : *sVectorFormatRef) {
            Merge(wFormatRef);
        }
        m_FormatRoot->FormatMerge()->JsonJavaScriptStyle(sWriter, sCss);
    }

    // JsonView f_i path: default f_ah=left for plain strings without explicit text-align.
    // Same rule as tFormatMergeCss::JsonJavaScript (cannot live in formats[] dedup table
    // because it depends on variant type, not format layers alone).
    void tFormatCssApi::_JsonJavaScriptDefaultStringAlign(tVariant* sVariant, Writer<StringBuffer>* sWriter) {
        if (sVariant != nullptr && sVariant->Type() == tVariantType::t_string) {
            sWriter->Key("f_ah");
            sWriter->Int(tInt(tTextAlign::left));
        }
    }

    void tFormatCssApi::BeginWriteJson() {
        m_FormatRoot->BeginWriteJson();
    }

    tSize tFormatCssApi::WriteJsonAddFormat(tFormatRef sFormatRef) {
        return(m_FormatRoot->WriteJsonAddFormat(sFormatRef));
    }

    tString tFormatCssApi::ReadJsonGetFormat(tSize sIndex) {
        return(m_FormatRoot->ReadJsonGetFormat(sIndex));
    }
    void tFormatCssApi::Json(Writer<StringBuffer>* sWriter) {
        m_FormatRoot->JsonSpreadSheet(sWriter);
    }

    void tFormatCssApi::Json(const rapidjson::Value& sValue) {
        m_FormatRoot->JsonSpreadSheet(sValue);
    };

    tString tFormatCssApi::CellFormatString(tFormatRef sAllocatorRef,tVariant* sVariant,tBool sReturn) {
        BeginMerge();
        if (sVariant!=nullptr) {
            if (sAllocatorRef!=0) {
                Merge(sAllocatorRef);
            }
            tFormatMergeCss* wFormatMergeCss=m_FormatRoot->FormatMerge();
            tFormatString* wFormatString=wFormatMergeCss->FormatString();
            wFormatString->SetDefaultFormat(sVariant);
#ifdef debugformatapi
            tFormatStringRoot* wFormatStringRoot=tApplication::Instance()->FormatStringRoot();
            

            cout << "tFormatCssApi::CellFormatString" << wFormatStringRoot->FormatStringType2LocalFormatString(wFormatString->FormatType()) << " precision=" <<  wFormatString->Decimal() << endl;
#endif
            
            return(sVariant->FormatString(wFormatString));
            }
        return("");
    }

    tString tFormatCssApi::CellFormatString(tVectorFormatRef* sVectorFormatRef,tVariant* sVariant, tBool sReturn) {
        BeginMerge();
        if (sVariant!=nullptr) {
            for(auto wFormatRef : *sVectorFormatRef) {
                Merge(wFormatRef);
            }
            tFormatMergeCss* wFormatMergeCss = m_FormatRoot->FormatMerge();
            tFormatString* wFormatString = wFormatMergeCss->FormatString();
            wFormatString->SetDefaultFormat(sVariant);
            return(sVariant->FormatString(wFormatString)); 
        }
        return("");
    }

    tShort tFormatCssApi::CellBorder(tFormatRef sAllocatorRef) {
        if (sAllocatorRef!=0) {
            BeginMerge();
            if (sAllocatorRef!=0) {
                Merge(sAllocatorRef);
            }
            tFormatMergeCss* wFormatMergeCss=m_FormatRoot->FormatMerge();
            return(wFormatMergeCss->BorderRect()->BorderMask());
        }
        return(0);
    }

    tShort tFormatCssApi::CellBorder(tVectorFormatRef* sVectorFormatRef) {
        BeginMerge();
        for(auto wFormatRef : *sVectorFormatRef) {
            Merge(wFormatRef);
        }
        tFormatMergeCss* wFormatMergeCss=m_FormatRoot->FormatMerge();
        return(wFormatMergeCss->BorderRect()->BorderMask());
    }

    // Merge the formats of a (neighbor) cell and emit a single border side under a
    // custom JSON key. This is used by tJsonView to project the neighbor's border
    // onto a merged cell's anchor so the frontend renders border-right / border-bottom
    // on the merged cell (Excel-compatible behavior).
    namespace {
        tBorderCss* BorderSidePtr(tBorderRectCss* sRect, tShort sBorderSide) {
            switch (sBorderSide) {
                case tBorderLeft:   return &sRect->Left();
                case tBorderTop:    return &sRect->Top();
                case tBorderRight:  return &sRect->Right();
                case tBorderBottom: return &sRect->Bottom();
                default: return nullptr;
            }
        }

        tString BorderSidePropertyName(tShort sBorderSide) {
            switch (sBorderSide) {
                case tBorderLeft:   return "border-left";
                case tBorderTop:    return "border-top";
                case tBorderRight:  return "border-right";
                case tBorderBottom: return "border-bottom";
                default: return "";
            }
        }
    }

    tString tFormatCssApi::MergedBorderSideApplyCss(tVectorFormatRef* sVectorFormatRef, tShort sBorderSide) {
        if (sVectorFormatRef == nullptr || sVectorFormatRef->empty()) {
            return("");
        }
        BeginMerge();
        for (auto wFormatRef : *sVectorFormatRef) {
            Merge(wFormatRef);
        }
        tFormatMergeCss* wFormatMergeCss = m_FormatRoot->FormatMerge();
        tBorderRectCss* wRect = wFormatMergeCss->BorderRect();
        tBorderCss* wSide = BorderSidePtr(wRect, sBorderSide);
        if (wSide == nullptr || wSide->Empty()) {
            return("");
        }
        tString wProp = BorderSidePropertyName(sBorderSide);
        if (wProp.empty()) {
            return("");
        }
        tString wCss = wSide->Str(wProp);
        if (wCss.empty()) {
            return("");
        }
        return wCss + ";";
    }

    void tFormatCssApi::JsonJavaScriptBorderSide(tVectorFormatRef* sVectorFormatRef, tShort sBorderSide, tString sKey, tBool sCss, Writer<StringBuffer>* sWriter) {
        if (sVectorFormatRef == nullptr) return;
        if (sVectorFormatRef->empty()) return;
        BeginMerge();
        for (auto wFormatRef : *sVectorFormatRef) {
            Merge(wFormatRef);
        }
        tFormatMergeCss* wFormatMergeCss = m_FormatRoot->FormatMerge();
        tBorderRectCss* wRect = wFormatMergeCss->BorderRect();
        tBorderCss* wSide = BorderSidePtr(wRect, sBorderSide);
        if (wSide == nullptr) return;
        if (wSide->Empty()) return;
        sWriter->Key(sKey.c_str());
        if (sCss) {
            sWriter->String(wSide->Str().c_str());
        } else {
            wSide->JsonJavaScriptCanvas(sWriter);
        }
    }

    tFormatString* tFormatCssApi::CellFormatString(tFormatRef sAllocatorRef) {
        if (sAllocatorRef!=0) {
            BeginMerge();
            if (sAllocatorRef!=0) {
                Merge(sAllocatorRef);
            }
            tFormatMergeCss* wFormatMergeCss=m_FormatRoot->FormatMerge();
            return(wFormatMergeCss->FormatString());
        }
        return(nullptr);
    }

    tFormatString* tFormatCssApi::CellFormatString(tVectorFormatRef* sVectorFormatRef) {
        BeginMerge();
        for(auto wFormatRef : *sVectorFormatRef) {
            Merge(wFormatRef);
        }
        tFormatMergeCss* wFormatMergeCss=m_FormatRoot->FormatMerge();
        return(wFormatMergeCss->FormatString());
    }
    void tFormatCssApi::Error(tString sLexerError,tInt sRow,tInt sCol) {
        m_LemonInterface->Error(sLexerError,sRow,sCol);
    }

    tString tFormatCssApi::Error() { return(m_LemonInterface->Error()); }

    tInt tFormatCssApi::ErrorColumn() { return(m_LemonInterface->ErrorColumn()); }

    tInt tFormatCssApi::ErrorLine() { return(m_LemonInterface->ErrorLine()); }

    tString tFormatCssApi::ErrorWithDetail() { return(m_LemonInterface->ErrorWithDetail()); }

#ifdef checkfo
    void tFormatCssApi::ResetCheck() {
        m_FormatRoot->ResetCheckCell();
    };

    void tFormatCssApi::IncCheck(tFormatRef sAllocatorRef) {
        tFormatCss* wFormatCss=m_FormatRoot->FormatRef(sAllocatorRef);
        if (wFormatCss->FormatCell()) {
            wFormatCss->IncCountCellCheck();;
        }
        
    };
   
    void tFormatCssApi::Check() {
        m_FormatRoot->CheckCell();
        m_FormatRoot->Check();
    }
#endif
    
#ifdef _DEBUGSK
    tString tFormatCssApi::Debug() { return (m_FormatRoot->Str()); }
    
    tString tFormatCssApi::Debug(tFormatRef sFormatRef) {
        tStringStream wStream;
        tFormatCss* wFormatCss=m_FormatRoot->FormatRef(sFormatRef);
        if (wFormatCss!=nullptr) {
            wStream << wFormatCss->Debug(m_FormatRoot);
        }
        return(wStream.str());
    }
#endif
    tFormatCssApi* FormatApi() { return(new tFormatCssApi()); }

} // namespace end

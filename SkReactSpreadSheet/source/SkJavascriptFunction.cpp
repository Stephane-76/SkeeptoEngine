//=============================================================================
// SkJavascriptFunction.hpp
// Api for React wasm
//=============================================================================
#include "../include/SkJavascriptFunction.hpp"

#ifdef __EMSCRIPTEN__
#include <cstdlib>
#include <emscripten/em_js.h>

#ifdef SK_NODE
EM_JS(char*, sker_js_generic_function, (const char* utf8), {
  var wArg = UTF8ToString(utf8);
  var wResult = Module.GenericFunction(wArg);
  var wLengthBytes = lengthBytesUTF8(wResult) + 1;
  var wStringOnWasmHeap = _malloc(wLengthBytes);
  stringToUTF8(wResult, wStringOnWasmHeap, wLengthBytes);
  return wStringOnWasmHeap;
});
#else
EM_JS(char*, sker_js_generic_function, (const char* utf8), {
  var wArg = UTF8ToString(utf8);
  var wResult;
  if (typeof importScripts === "function" && typeof window === "undefined") {
    wResult = globalThis.GenericFunction(wArg);
  } else {
    wResult = window.GenericFunction(wArg);
  }
  var wLengthBytes = lengthBytesUTF8(wResult) + 1;
  var wStringOnWasmHeap = _malloc(wLengthBytes);
  stringToUTF8(wResult, wStringOnWasmHeap, wLengthBytes);
  return wStringOnWasmHeap;
});
#endif
#endif


namespace SkSpreadSheet {

void _WriteJson(Writer<StringBuffer>* sWriter,tVariant& sValue) {
    switch (sValue.Type()) {
        case tVariantType::t_null: sWriter->Null();  break; // os << "null"; break;
        case tVariantType::t_int:  sWriter->Int(sValue.Int()); break;
        case tVariantType::t_bool: sWriter->Bool(sValue.Bool()); break;
        case tVariantType::t_double: sWriter->Double(sValue.Double()); break;
        case tVariantType::t_string: sWriter->String(sValue.String().c_str()); break;
        case tVariantType::t_error: sWriter->String(sValue.Error().Error().c_str());  break;
        case tVariantType::t_date: {
            tClassDate wDate(sValue.Date());
            sWriter->String(wDate.UsDate().c_str());
            break;
        }
        case tVariantType::t_class: {
            tVirtualClass* wVirtualClass = sValue.Class();
            if (wVirtualClass != nullptr) {
                sWriter->StartObject();
                tString wName = wVirtualClass->ClassName();
                sWriter->Key("n");
                sWriter->String(wName.c_str());
                if (wVirtualClass->IsReactComponent()) {
                    sWriter->Key("co");
                    sWriter->Bool(true);
                }
                sWriter->Key("c");
                if (wVirtualClass->IsJsonJavaScript()) {
                    wVirtualClass->JsonJavaScript(sWriter);
                } else {
                    wVirtualClass->Json(sWriter);
                }
                sWriter->EndObject();
            }
            else {
                sWriter->Null(); break;
            }
            break;
        }
    }
}

    // CallBack for function sum ============================================
tCallBackRangeJavascript::tCallBackRangeJavascript(tColRowCellRange* sColRowCellRange, Writer<StringBuffer>* sWriter,tFunctionJavascript* sParent) : tCallBackRangeFunction(sColRowCellRange),m_Writer(sWriter),m_Parent(sParent) {}


    tBool tCallBackRangeJavascript::CallBack(tAllocatorRef sAllocatorRef) {
        //tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
        //cout << "Cell  " << wCell->StrRef() << "=" << wCell->CalculableValue() << endl;
        (void)m_Parent; // Suppress unused warning - may be used in future
        m_Value = m_ColRowCellRange->Cell(sAllocatorRef)->CalculableValue();
        m_Writer->StartObject();
        m_Value.Json(m_Writer);
        m_Writer->EndObject();
        //_WriteJson(m_Writer,m_Value);
        return(true); // false for Stop
    };

    // Function =============================================================
    tFunctionJavascript::tFunctionJavascript(tString sName,tInt sNbArg, tBool sRef) : tFunction(),m_Name(sName), m_Ref(sRef) {
        (void)sNbArg;
    }



    tString tFunctionJavascript::CallJavascript(tString sArg) {
#ifdef __EMSCRIPTEN__
        char* wResult = sker_js_generic_function(sArg.c_str());
        tString wOut(wResult != nullptr ? wResult : "");
        if (wResult != nullptr) {
            std::free(wResult);
        }
        return wOut;
#else
        (void)sArg; // Not used in non-EMSCRIPTEN build
        tString wResult="{\"t\":\"i\",\"v\":200}";
        return(wResult);
#endif
}

    tStackElem tFunctionJavascript::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        tVariant wResult = CallWithArgs(&wArgs);
        return tStackElem(wResult);
    }

    tVariant tFunctionJavascript::CallWithArgs(std::vector<tStackElem>* sArgVector) {
        tVariant wValueResult;
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        cout << "CALL   -->" << m_Name << endl;
        wWriter.StartObject();
        wWriter.Key("n");
        wWriter.String(m_Name.c_str());
        wWriter.Key("a");
        wWriter.StartArray();
        std::vector<tStackElem>::reverse_iterator wIterator;
        for (wIterator = sArgVector->rbegin(); wIterator != sArgVector->rend(); wIterator++) {
            // Variant, Cell, or Range
            tStackElem wArg = *wIterator;
            switch (wArg.Type()) {
                case tStackType::t_Variant : {
                    wWriter.StartObject();
                    wArg.Variant().Json(&wWriter);
                    wWriter.EndObject();
                    break;
                }
                case tStackType::t_Cell: {
                    tCell* wCell = wArg.Cell();
                    if (wCell != nullptr) {
                        wWriter.StartObject();
                        wCell->CalculableValue().Json(&wWriter);
                        wWriter.EndObject();
                    }
                    break;
                }
                case tStackType::t_Range: {
                    tRange* wRange = wArg.Range();
                    if (m_Ref) {
                        wWriter.String(wRange->StrRef(true).c_str());
                    } else {
                        wWriter.StartArray();
                        tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
                        tCallBackRangeJavascript wCallBackRange(wColRowCellRange,&wWriter,this);
                        wColRowCellRange->VisitorRange(wRange, &wCallBackRange);
                        wWriter.EndArray();
                    }
                    break;
                }
                default : break;
            }
        }
    
        wWriter.EndArray();
        wWriter.EndObject();
        tString wValueArg=wStringBuffer.GetString();
        
        tString wJson=CallJavascript(wValueArg);
        Document wDocument;
        wDocument.Parse(wJson.c_str());
        wValueResult.Json(wDocument);
        
        cout << "Result " << wValueResult << endl;
        

        return(wValueResult);
    };

}; // end of namespace

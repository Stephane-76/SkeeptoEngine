//=============================================================================
// SkUISpreadSheetApi
// Api for User interface
//=============================================================================

#include "rapidjson/error/en.h"

#include "../include/SkInterfaceWeb.hpp"
#include "../include/SkJavascriptFunction.hpp"
#include "../include/SkUISpreadSheetApi.hpp"
#include "../include/SkUndoRedo.hpp"
#include "../include/SkJsonSelect.hpp"
#include <SkRangeRefTransform.hpp>
#include <SkTools.hpp>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <string>
#include <ctime>

#ifdef __EMSCRIPTEN__
#include <iostream>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/em_js.h>
namespace {
// Early init: libc FILE may not be ready; constructor below applies a second, reliable flush setup.
struct EmscriptenLineBufferedStdStreams {
	EmscriptenLineBufferedStdStreams() {
		setvbuf(stdout, nullptr, _IOLBF, BUFSIZ);
		setvbuf(stderr, nullptr, _IOLBF, BUFSIZ);
	}
};
static EmscriptenLineBufferedStdStreams s_emscriptenLineBufferedStdStreams;
} // namespace

// Prefer EM_JS over EM_ASM for host callbacks: EM_ASM uses dynamic wasm-table dispatch
// (invoke_* / getWasmTableEntry) which breaks if .wasm and generated .mjs drift apart.
#ifdef SK_NODE
EM_JS(void, sker_js_post_message, (const char* utf8), {
  var s = UTF8ToString(utf8);
  if (typeof Module !== 'undefined' && typeof Module.PostMessage === 'function') {
    Module.PostMessage(s);
  } else {
    console.warn('Module.PostMessage is not available');
  }
});
EM_JS(void, sker_js_on_cell_change, (const char* utf8), {
  var s = UTF8ToString(utf8);
  if (typeof Module !== 'undefined' && typeof Module.OnCellChange === 'function') {
    Module.OnCellChange(s);
  } else {
    console.warn('Module.OnCellChange is not available');
  }
});
#else
EM_JS(void, sker_js_post_message, (const char* utf8), {
  var s = UTF8ToString(utf8);
  if (typeof importScripts === 'function' && typeof window === 'undefined') {
    if (typeof globalThis !== 'undefined' && typeof globalThis.PostMessage === 'function') {
      globalThis.PostMessage(s);
    } else {
      console.warn('globalThis.PostMessage is not available');
    }
  } else if (typeof window !== 'undefined' && typeof window.PostMessage === 'function') {
    window.PostMessage(s);
  } else {
    console.warn('window.PostMessage is not available');
  }
});
EM_JS(void, sker_js_on_cell_change, (const char* utf8), {
  var s = UTF8ToString(utf8);
  if (typeof importScripts === 'function' && typeof window === 'undefined') {
    if (typeof globalThis !== 'undefined' && typeof globalThis.OnCellChange === 'function') {
      globalThis.OnCellChange(s);
    } else {
      console.warn('globalThis.OnCellChange is not available');
    }
  } else if (typeof window !== 'undefined' && typeof window.OnCellChange === 'function') {
    window.OnCellChange(s);
  } else {
    console.warn('window.OnCellChange is not available');
  }
});
#endif
#endif

#ifdef _DEBUGSK
#pragma message "_DEBUGSK active"
#endif
#ifdef _RELEASE
#pragma message "_RELEASE active"
#endif
#ifdef checksp
#pragma message "checksp"
#endif
#ifdef checkfo
#pragma message "checkfo"
#endif

#define _DebugInterface
// Toggle the verbose JSON dumps emitted by _ReadJson/_WriteJson. The full
// workbook JSON can easily exceed several MB on real spreadsheets and floods
// the Node host stdout, so we keep it gated behind its own switch and leave
// it off by default. Re-enable locally when chasing a serialization bug.
// #define DebugInterfaceJsonDump

namespace SkSpreadSheet {
#ifdef __EMSCRIPTEN__
// Binding code
EMSCRIPTEN_BINDINGS(Module) {
    class_<tUISpreadSheet>("UISpreadSheet")
        .constructor()
        .function("GetActiveWorkBook",&tUISpreadSheet::_GetActiveWorkBook)
        .function("SetActiveWorkBook",&tUISpreadSheet::_SetActiveWorkBook)
    
        .function("NewWorkBook",&tUISpreadSheet::_NewWorkBook)
    
        .function("AddWorkBook",&tUISpreadSheet::_AddWorkBook)
        .function("DeleteWorkBook",&tUISpreadSheet::_DeleteWorkBook)
        .function("RenameWorkBook",&tUISpreadSheet::_RenameWorkBook)
        .function("JsonWorkBook",&tUISpreadSheet::_JsonWorkBook)
        .function("JsonWorkBooks",&tUISpreadSheet::_JsonWorkBooks)
    
        .function("WriteJson",&tUISpreadSheet::_WriteJson)
        .function("ReadJson",&tUISpreadSheet::_ReadJson)
        .function("RecalculateAll",&tUISpreadSheet::_RecalculateAll)
        .function("BeginRecalculateAllCooperative",&tUISpreadSheet::_BeginRecalculateAllCooperative)
        .function("StepRecalculateAllCooperative",&tUISpreadSheet::_StepRecalculateAllCooperative)
        .function("RecalculateAllCooperativeProgress",&tUISpreadSheet::_RecalculateAllCooperativeProgress)
        .function("IsRecalculateAllCooperativeActive",&tUISpreadSheet::_IsRecalculateAllCooperativeActive)
        .function("SetCooperativeCalculateEnabled",&tUISpreadSheet::_SetCooperativeCalculateEnabled)
        .function("CooperativeCalculateEnabled",&tUISpreadSheet::_CooperativeCalculateEnabled)
        
        .function("Undo", &tUISpreadSheet::_Undo)
        .function("Redo", &tUISpreadSheet::_Redo)
        .function("IsUndoActif", &tUISpreadSheet::_IsUndoActif)
        .function("SetIsUndoActif", &tUISpreadSheet::_SetIsUndoActif)

        //User Interface ======================================================
        .function("SetUserInterface", &tUISpreadSheet::_SetUserInterface)
        .function("GetUserInterface", &tUISpreadSheet::_GetUserInterface)
    
        // Sheet ==============================================================
        .function("SetExtraUndo", &tUISpreadSheet::_SetExtraUndo)
        .function("GetExtraUndo", &tUISpreadSheet::_GetExtraUndo)
    
        .function("SetActiveSheet",&tUISpreadSheet::_SetActiveSheet)
        .function("GetActiveSheet",&tUISpreadSheet::_GetActiveSheet)
    
        .function("AddSheet", &tUISpreadSheet::_AddSheet)
        .function("RenameSheet", &tUISpreadSheet::_RenameSheet)
        .function("SwapSheet", emscripten::optional_override([](tUISpreadSheet& self,
                                                                emscripten::val sName1,
                                                                emscripten::val sName2,
                                                                emscripten::val sInsertAfter) -> tBool {
            const tString wName1 = sName1.as<std::string>();
            const tString wName2 = sName2.as<std::string>();
            const tBool wInsertAfter =
                sInsertAfter.isUndefined() || sInsertAfter.isNull()
                    ? false
                    : static_cast<tBool>(sInsertAfter.as<bool>());
            return self._SwapSheet(wName1, wName2, wInsertAfter);
        }))
        .function("DeleteSheet", &tUISpreadSheet::_DeleteSheet)
        .function("SheetsList", &tUISpreadSheet::_SheetsList)
    
        // Cell ===============================================================
        // Value: use emscripten::val for JS strings — const char* is not a registered embind type (UnboundTypeError PKc);
        // std::string-only binding is kept inside the lambda so we delegate to _Value with normal tStrings.
        .function(
            "Value",
            emscripten::optional_override([](tUISpreadSheet& self, emscripten::val sRef, emscripten::val sValue, emscripten::val sSheet) -> tBool {
                const tString wRef = sRef.as<std::string>();
                const tString wVal = sValue.as<std::string>();
                const tString wSh = sSheet.as<std::string>();
                return self._Value(wRef, wVal, wSh);
            }))
        .function(
            "FillSeries",
            emscripten::optional_override([](tUISpreadSheet& self, emscripten::val sSourceRef, emscripten::val sDestRef, emscripten::val sSheet) -> tBool {
                const tString wSourceRef = sSourceRef.as<std::string>();
                const tString wDestRef = sDestRef.as<std::string>();
                const tString wSh = sSheet.as<std::string>();
                return self._FillSeries(wSourceRef, wDestRef, wSh);
            }))
        .function("ValueAttribute", &tUISpreadSheet::_ValueAttribute)
        .function("CellClassAttributes", &tUISpreadSheet::_CellClassAttributes)
        .function("ValueClassCalculable", &tUISpreadSheet::_ValueClassCalculable)
    
        .function("ValueString", &tUISpreadSheet::_ValueString)
        .function("ValueInt", &tUISpreadSheet::_ValueInt)
        .function("ValueDouble", &tUISpreadSheet::_ValueDouble)
        
        .function("GetValue", &tUISpreadSheet::_GetValue)
        .function("GetCalculableScalar", &tUISpreadSheet::_GetCalculableScalar)
        .function("GetValueAttribute", &tUISpreadSheet::_GetValueAttribute)
        .function("GetFormulaAttribute", &tUISpreadSheet::_GetFormulaAttribute)

        .function("GetInputValue", &tUISpreadSheet::_GetInputValue)
        .function(
            "GetFormula",
            emscripten::optional_override([](tUISpreadSheet& self, emscripten::val sRef, emscripten::val sSheet, emscripten::val sUser) -> tString {
                const tString wRef = sRef.as<std::string>();
                const tString wSh = sSheet.as<std::string>();
                const tBool wUser = sUser.isUndefined() || sUser.isNull() ? false : static_cast<tBool>(sUser.as<bool>());
                return self._GetFormula(wRef, wSh, wUser);
            }))
        .function(
            "CellRef",
            emscripten::optional_override([](tUISpreadSheet& self, emscripten::val sRefAnchor, emscripten::val sRef, emscripten::val sSheet) -> tString {
                const tString wAnchor = sRefAnchor.as<std::string>();
                const tString wRef = sRef.as<std::string>();
                const tString wSh = sSheet.isUndefined() || sSheet.isNull() ? tString("") : sSheet.as<std::string>();
                return self._CellRef(wAnchor, wRef, wSh);
            }))

        // Do not use names "Error" / "ErrorLine" here: embind exposes methods on Module and
        // collides with JS Error / emscripten plumbing → invalid wasm table entries (invoke_*).
        .function("CompileError", &tUISpreadSheet::_Error)
        .function("CompileErrorLine", &tUISpreadSheet::_ErrorLine)
        .function("CompileErrorColumn", &tUISpreadSheet::_ErrorColumn)
        .function("CompileErrorWithDetail", &tUISpreadSheet::_ErrorWithDetail)
    
        .function("Raz", &tUISpreadSheet::_Raz)
        .function("RazFormat", &tUISpreadSheet::_RazFormat)
        
        .function("SizeRow", &tUISpreadSheet::_SizeRow)
        .function("SizeCol", &tUISpreadSheet::_SizeCol)

        .function("GetSizeRow", &tUISpreadSheet::_GetSizeRow)
        .function("GetSizeCol", &tUISpreadSheet::_GetSizeCol)
    
        .function("InsertRow", &tUISpreadSheet::_InsertRow)
        .function("InsertRowByRect", &tUISpreadSheet::_InsertRowByRect)
        .function("InsertRowByRectWithLabel", &tUISpreadSheet::_InsertRowByRectWithLabel)
        .function("DeleteRowByRect", &tUISpreadSheet::_DeleteRowByRect)
        .function("DeleteRow", &tUISpreadSheet::_DeleteRow)
    
        .function("InsertCol", &tUISpreadSheet::_InsertCol)
        .function("InsertColByRect", &tUISpreadSheet::_InsertColByRect)
        .function("DeleteCol", &tUISpreadSheet::_DeleteCol)
        .function("DeleteColByRect", &tUISpreadSheet::_DeleteColByRect)
    
        // Copy Paste
        .function("Copy", &tUISpreadSheet::_Copy)
        .function("Cut", &tUISpreadSheet::_Cut)
        .function("Paste", &tUISpreadSheet::_Paste)
        .function("Move", &tUISpreadSheet::_Move)
    
        // Format
        .function("Format", &tUISpreadSheet::_Format)
        .function("Precision", &tUISpreadSheet::_Precision)
        .function("ConditionalFormat", &tUISpreadSheet::_ConditionalFormat)
        .function("DeleteConditionalFormat", &tUISpreadSheet::_DeleteConditionalFormat)
        .function("Border", &tUISpreadSheet::_Border)
    
        .function("GetFormat", &tUISpreadSheet::_GetFormat)
        .function("DebugFormat",&tUISpreadSheet::_DEBUGSKFormat)
        .function("FormatCount",&tUISpreadSheet::_FormatCount)
    
        .function("ApplyFormatString",&tUISpreadSheet::_ApplyFormatString)
        .function("DefaultFormatString",&tUISpreadSheet::_DefaultFormatString)
        .function("FormatValueWithCellFormat",&tUISpreadSheet::_FormatValueWithCellFormat)
        .function("JsonFormatString",&tUISpreadSheet::_JsonFormatString)
        .function("JsonConditionalFormat",&tUISpreadSheet::_JsonConditionalFormat)
    
        // Merge
        .function("Merge", &tUISpreadSheet::_Merge)

        .function("ReturnMerged", &tUISpreadSheet::_ReturnMerged)
        .function("ReturnRangeMerged", &tUISpreadSheet::_ReturnRangeMerged)
        .function("ReturnRangeMergedFusion", &tUISpreadSheet::_ReturnRangeMergedFusion)

        // Named Range
        .function("InsertNamedRange", &tUISpreadSheet::_InsertNamedRange)
        .function("DeleteNamedRange", &tUISpreadSheet::_DeleteNamedRange)
        .function("UpdateNamedRange", &tUISpreadSheet::_UpdateNamedRange)

        // Rename gating (sheet / named range) for collaborative mode
        .function("IsRenameAllowed", &tUISpreadSheet::_IsRenameAllowed)
        .function("SetMultiUserActive", &tUISpreadSheet::_SetMultiUserActive)

        // Named Formula
        .function("InsertFormulaNamed", &tUISpreadSheet::_InsertFormulaNamed)
        .function("DeleteFormulaNamed", &tUISpreadSheet::_DeleteFormulaNamed)

        // Floating objects
        .function("InsertFloatingObject", &tUISpreadSheet::_InsertFloatingObject)
        .function("DeleteFloatingObject", &tUISpreadSheet::_DeleteFloatingObject)
        .function("FloatingObjectLayout", &tUISpreadSheet::_FloatingObjectLayout)
        .function("FloatingObjectBringToFront", &tUISpreadSheet::_FloatingObjectBringToFront)
        .function("FloatingObjectAttribute", &tUISpreadSheet::_FloatingObjectAttribute)
        .function("FloatingObjectAttributes", &tUISpreadSheet::_FloatingObjectAttributes)
        
        // View
        .function("JsonView", &tUISpreadSheet::_JsonView)
        .function("JsonFloatingObjectsForSheet", &tUISpreadSheet::_JsonFloatingObjectsForSheet)
        .function("JsonFloatingObjects", &tUISpreadSheet::_JsonFloatingObjects)
        .function("JsonRightJustify",&tUISpreadSheet::_JsonRightJustify)
        .function("JsonBottomJustify",&tUISpreadSheet::_JsonBottomJustify)
    
        .function("JsonColByPixel",&tUISpreadSheet::_JsonColByPixel)
        .function("JsonRowByPixel",&tUISpreadSheet::_JsonRowByPixel)
    
        
        .function("JsonBottomRight",&tUISpreadSheet::_JsonBottomRight)
        .function("JsonPixelBottomRight",&tUISpreadSheet::_JsonPixelBottomRight)
  
        .function("JsonRangeNamed",&tUISpreadSheet::_JsonRangeNamed)
        .function("JsonFormulaNamed",&tUISpreadSheet::_JsonFormulaNamed)
        .function("JsonPrintParameters",&tUISpreadSheet::_JsonPrintParameters)
        .function("SetJsonPrintParameters",&tUISpreadSheet::_SetJsonPrintParameters)
        .function("JsonRangeData",&tUISpreadSheet::_JsonRangeData)
        .function("JsonFindUniqueValue",&tUISpreadSheet::_JsonFindUniqueValue)
        .function("JsonFindCell",&tUISpreadSheet::_JsonFindCell)
        .function("UndoApplyRangeData",&tUISpreadSheet::_UndoApplyRangeData)
        .function("UndoAddRangeData",&tUISpreadSheet::_UndoAddRangeData)
        // Tree
        .function("OpenCloseTreeRow",&tUISpreadSheet::_OpenCloseTreeRow)
        .function("OpenCloseTreeCol",&tUISpreadSheet::_OpenCloseTreeCol)
    
        .function("ChangeTreeRow",&tUISpreadSheet::_ChangeTreeRow)
        .function("ChangeTreeCol",&tUISpreadSheet::_ChangeTreeCol)
        .function("SplitView",&tUISpreadSheet::_SplitView)
        .function("SplitFreezeCol",&tUISpreadSheet::_SplitFreezeCol)
        .function("SplitFreezeRow",&tUISpreadSheet::_SplitFreezeRow)

        // Class
        .function("RegisterClassAttribute",&tUISpreadSheet::_RegisterClassAttribute)
        .function("AddProperty",&tUISpreadSheet::_AddProperty)
        .function("CellClass",&tUISpreadSheet::_CellClass)
    
    
        .function("JsonCellClass",&tUISpreadSheet::_JsonCellClass)
    
        .function("JsonCellClassByName",&tUISpreadSheet::_JsonCellClassByName)
        .function("ApplyUnit",&tUISpreadSheet::_ApplyUnit)
    
        // Move ===============================================================
        .function("MoveCell", &tUISpreadSheet::_MoveCell)
        .function("MoveToCell", &tUISpreadSheet::_MoveToCell)
   
        .function("SumPixelHeight", &tUISpreadSheet::_SumPixelHeight)
        .function("SumPixelWidth", &tUISpreadSheet::_SumPixelWidth)
    
        // Utils ==============================================================
        .function("Base10toAlpha", &tUISpreadSheet::_Base10toAlpha)
        .function("AlphaToBase10", &tUISpreadSheet::_AlphaToBase10)
        .function("ParseCell", &tUISpreadSheet::_ParseCell)
        .function("ParseRange", &tUISpreadSheet::_ParseRange)
        .function("QualifyRefsForSheet", &tUISpreadSheet::_QualifyRefsForSheet)
        .function("StripTargetSheetFromRefs", &tUISpreadSheet::_StripTargetSheetFromRefs)
        .function("CollectFormulaRefs", &tUISpreadSheet::_CollectFormulaRefs)

        // Internal ===============================================================
        .function("EnsureCell", &tUISpreadSheet::_EnsureCell)
        .function("SetLang", &tUISpreadSheet::_SetLang)
    
         .function("AddFunction",&tUISpreadSheet::_AddFunction)

        .function("Call", &tUISpreadSheet::_Call)
    
        .function("GetMessage",&tUISpreadSheet::GetMessage)
    
        .function("Pressure", &tUISpreadSheet::_Pressure)
        .function("BeginPressureCooperative", &tUISpreadSheet::_BeginPressureCooperative)
        .function("StepPressureCooperative", &tUISpreadSheet::_StepPressureCooperative)
        .function("PressureCooperativeProgress", &tUISpreadSheet::_PressureCooperativeProgress)
        .function("IsPressureCooperativeActive", &tUISpreadSheet::_IsPressureCooperativeActive)
        .function("EndPressureCooperative", &tUISpreadSheet::_EndPressureCooperative)
        ;
}
#endif

    const tString wWorkBookUri="www.skeema.fr/WorkBook";
    //=========================================================================
    tUIExtraUndo::tUIExtraUndo() : tUndoExtra(), m_Json("") {}
    
    tUIExtraUndo::tUIExtraUndo(tString sJson) : tUndoExtra(), m_Json(sJson) {}

    tUIExtraUndo::tUIExtraUndo(const tUIExtraUndo& sUIExtraUndo) : tUndoExtra(sUIExtraUndo), m_Json(sUIExtraUndo.m_Json) {}

    tUIExtraUndo& tUIExtraUndo::operator=(const tUIExtraUndo& sUIExtraUndo) {
        if (this != &sUIExtraUndo) {
            tUndoExtra::operator=(sUIExtraUndo);
            m_Json = sUIExtraUndo.m_Json;
        }
        return *this;
    }

    void tUIExtraUndo::_Json(tString sJson) { m_Json=sJson; }
    tString tUIExtraUndo::_Json() { return(m_Json);}

    static tUISpreadSheet* wStaticUISpreadSheet;

    void OnCellChange(tCell* sCell,tVariant& sValue) {
        wStaticUISpreadSheet->_OnCellChange(sCell,sValue);
    }

    //=========================================================================
	tUISpreadSheet::tUISpreadSheet() : tInterfaceWeb() {
#ifdef __EMSCRIPTEN__
		// After WASM runtime init, FILE* and Module.print are valid. Unbuffered stdio + unitbuf so every cout reaches console.log.
		setvbuf(stdout, nullptr, _IONBF, 0);
		setvbuf(stderr, nullptr, _IONBF, 0);
		std::cout.setf(std::ios::unitbuf);
#endif
        m_Application = tApplication::Instance();
        NewWorkBook(wWorkBookUri);
        UriWorkBook(wWorkBookUri);
        // Excel / .sker formula strings use US-style list separators (comma between args, dot decimal).
        // With Locale("fr"), m_Decimal is ',' so DATE(x,1,1) lexes as DATE(x,1.1) — see SkLexerSpreadSheet::number.
        m_Application->Locale("us");
        // One Format Api for the application
        m_FormatApi=new  SkFormat::tFormatCssApi();
        FormatApi(m_FormatApi);
        // tUISpreadSheet is the skeepto UI: undo must stay on. Python opts out
        // after construction; tApi itself defaults to false for batch clients.
        IsUndoActif(true);
#ifdef __EMSCRIPTEN__
        // PostMessage / OnCellChange target the JS host. Native (Python, tests)
        // stays Client(false) with no cell-change hook.
        tSpreadSheetContainer::SetOnCellChange(&OnCellChange);
        wStaticUISpreadSheet=this;
        Client(true);
		cout << "tUISpreadSheet::tUISpreadSheet(";
#ifdef _DEBUGSK
        cout << "DEBUG";
#else
        cout << "RELEASE";
#endif
        cout << ")" << endl;
#endif
	}

	tUISpreadSheet::~tUISpreadSheet() {
#ifdef DebugInterface
		cout << "tUISpreadSheet::~tUISpreadSheet()" << endl;
#endif
        if (wStaticUISpreadSheet == this) {
            tSpreadSheetContainer::SetOnCellChange(nullptr);
            wStaticUISpreadSheet = nullptr;
        }
        // Delete m_FormatApi
        delete(m_FormatApi);
        m_FormatApi=nullptr;
        FormatApi(m_FormatApi);
 	}

void tUISpreadSheet::_OnCellChange(tCell* sCell,tVariant& sValue) {
    tStringStream wStream;
    wStream << sCell->Sheet()->Name() << "!" << ((tItem*)(sCell))->StrRef() << "=" << sValue.Str();
#ifdef __EMSCRIPTEN__
    sker_js_on_cell_change(wStream.str().c_str());
#endif
    }

    void tUISpreadSheet::Clear() {
        m_FormatApi->Clear();
    }

    tString tUISpreadSheet::_GetActiveWorkBook() {
        tWorkBook* wWorkBook=ActiveWorkBook();
        if (wWorkBook!=nullptr) {
            return(wWorkBook->Uri());
        }
        return("");
    }

    tBool tUISpreadSheet::_SetActiveWorkBook(tString sUri) {
    #ifdef DebugInterface
            cout << "tUISpreadSheet::SetActiveWorkBook(" << sUri << ")";
    #endif
        tBool wOk = ActiveWorkBook(sUri);
        if (wOk) {
            UriWorkBook(sUri);
        }
        return(wOk);
    }

    tBool tUISpreadSheet::_NewWorkBook(tString sUri){
#ifdef DebugInterface
        cout << "tUISpreadSheet::NewWorkBook(" << sUri << ")";
#endif
        tApplication::Instance()->ClearUndoRedo();
        tBool wOk=NewWorkBook(sUri);
        if (wOk) {
            UriWorkBook(sUri);
        }
#ifdef DebugInterface
        if (wOk) { cout << " Ok"; } else { cout << " Not ok.."; }
        cout << endl;
#endif
        return(wOk);
    }


    tBool tUISpreadSheet::_AddWorkBook(tString sUri) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::AddWorkBook(" << sUri << ")";
#endif
        tApplication::Instance()->ClearUndoRedo();
        tBool wOk=AddWorkBook(sUri);
        if (wOk) {
            UriWorkBook(sUri);
        }
#ifdef DebugInterface
        if (wOk) { cout << " Ok"; } else { cout << " Not ok.."; }
        cout << endl;
#endif
        return(wOk);
    }

    tBool tUISpreadSheet::_DeleteWorkBook(tString sUri) {
#ifdef DebugInterface
            cout << "tUISpreadSheet::DeleteWorkBook(" << sUri << ")" << endl;
#endif
        tApplication::Instance()->ClearUndoRedo();
        tBool wOk=DeleteWorkBook(sUri);
#ifdef DebugInterface
        if (wOk) { cout << " Ok"; } else { cout << " Not ok.."; }
        cout << endl;
#endif
        return(wOk);
    }

    tBool tUISpreadSheet::_RenameWorkBook(tString sUri,tString sUriTo) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::RenameWorkBook(" << sUri << " to " << sUriTo << ")" << endl;
#endif
        tApplication::Instance()->ClearUndoRedo();
        tBool wOk=RenameWorkBook(sUri,sUriTo);
        if (wOk && UriWorkBook() == sUri) {
            UriWorkBook(sUriTo);
        }
#ifdef DebugInterface
        if (wOk) { cout << " Ok"; } else { cout << " Not ok.."; }
        cout << endl;
#endif
        return(wOk);
    }
    
    tString  tUISpreadSheet::_JsonWorkBook(tString sUri) {
        return(JsonWorkBook(sUri));
    }

    tString  tUISpreadSheet::_JsonWorkBooks() {
        return(JsonWorkBooks());
    }

    tString tUISpreadSheet::_WriteJson(tString sUri) {
    #ifdef DebugInterface
            cout << "tUISpreadSheet::WriteJson(" << sUri << ")" << endl;

        tSpreadSheetContainer* wSpreadSheetContainer=tSpreadSheetContainer::Instance();
        
        tWorkBook* wWorkBook=wSpreadSheetContainer->ActiveWorkBook();
        cout << "Active WorkBook " << wWorkBook->Uri() << endl;
#endif
        tString wResult=WriteJson(sUri);
    #ifdef DebugInterfaceJsonDump
        cout << wResult << endl;
    #endif
        return(wResult);
    }

    tBool tUISpreadSheet::_ReadJson(tString sJson) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::ReadJson(uri="
             << (ActiveWorkBook() != nullptr ? ActiveWorkBook()->Uri() : tString("<null>"))
             << ", " << sJson.size() << " chars)" << endl;
#endif
#ifdef DebugInterfaceJsonDump
        cout << "tUISpreadSheet::ReadJson payload: " << sJson << endl;
#endif
        tApplication::Instance()->ClearUndoRedo();
        ActiveWorkBook()->Clear();
        tBool wOk=ReadJson(sJson);
#ifdef DebugInterface
        if (wOk) { cout << " Ok"; } else { cout << " Not ok.."; }
        cout << endl;
#endif
        // Full RecalculateAll disabled after load: JsonEnd compiles formulas and restores persisted v/t from .sker.
        // if (tWorkBook* wLoadBook = ActiveWorkBook()) {
        //     wLoadBook->RecalculateAll();
        // }
#ifdef checksp
        Check();
#endif
        return(wOk);
    }

    tBool tUISpreadSheet::_RecalculateAll() {
        tWorkBook* wWorkBook = ActiveWorkBook();
        if (wWorkBook == nullptr) {
            return false;
        }
        wWorkBook->RecalculateAll();
        return true;
    }

    void tUISpreadSheet::_BeginRecalculateAllCooperative() {
        tWorkBook* wWorkBook = ActiveWorkBook();
        if (wWorkBook == nullptr) {
            return;
        }
        wWorkBook->BeginRecalculateAllCooperative();
    }

    tBool tUISpreadSheet::_StepRecalculateAllCooperative(tInt sMaxMs) {
        tWorkBook* wWorkBook = ActiveWorkBook();
        if (wWorkBook == nullptr) {
            return true;
        }
        return wWorkBook->StepRecalculateAllCooperative(sMaxMs);
    }

    tInt tUISpreadSheet::_RecalculateAllCooperativeProgress() {
        tWorkBook* wWorkBook = ActiveWorkBook();
        if (wWorkBook == nullptr) {
            return 100;
        }
        return wWorkBook->RecalculateAllCooperativeProgress();
    }

    tBool tUISpreadSheet::_IsRecalculateAllCooperativeActive() {
        tWorkBook* wWorkBook = ActiveWorkBook();
        if (wWorkBook == nullptr) {
            return false;
        }
        return wWorkBook->IsRecalculateAllCooperativeActive();
    }

    void tUISpreadSheet::_SetCooperativeCalculateEnabled(tBool sEnabled) {
        tWorkBook* wWorkBook = ActiveWorkBook();
        if (wWorkBook == nullptr) {
            return;
        }
        wWorkBook->SetCooperativeCalculateEnabled(sEnabled);
    }

    tBool tUISpreadSheet::_CooperativeCalculateEnabled() {
        tWorkBook* wWorkBook = ActiveWorkBook();
        if (wWorkBook == nullptr) {
            return false;
        }
        return wWorkBook->CooperativeCalculateEnabled();
    }

    // --- Cooperative _Pressure generation ------------------------------------
    // Same non-blocking pattern as the cooperative recalc: JS calls Begin once, then
    // Step repeatedly (yielding to the browser between calls) while polling Progress
    // for the banner %, then End to run the (blocking) JsonEnd finalize. Only the
    // generation phase is cooperative; JsonEnd stays a single call (see _EndPressure...).
    void tUISpreadSheet::_BeginPressureCooperative(tInt sDynamicRow, tInt sDynamicCol, tString sSheet) {
        if (!SetSheet(sSheet)) {
            m_PressureActive = false;
            return;
        }
        m_PressureTotalRows = sDynamicRow;
        m_PressureCol = sDynamicCol;
        m_PressureRow = 2; // rows 2..total are generated in steps; row 1 is seeded below
        m_PressureActive = true;

        cout << "Start timer ensure cells with formula and SUM() " << (sDynamicRow * sDynamicCol)
             << " Cells on " << sDynamicRow << "  rows " << sDynamicCol << " cols." << endl;
        m_PressureGenClock = clock();

        // Defer formula compilation (file-load path): queue formulas as plain string values;
        // JsonEnd compiles + calculates them in one batch on _EndPressureCooperative. No ranges
        // are wired per cell, so generation stays linear.
        tSpreadSheetContainer::Instance()->JsonBegin();
        // Set last cell so the sheet extent is final from the start (keeps generation linear).
        EnsureCell(sDynamicRow, sDynamicCol);
        // Seed row 1.
        for (tInt wCol = 1; wCol <= sDynamicCol; wCol++) {
            if (wCol > 1) {
                tCell* wCell = EnsureCell(1, wCol);
                tStringStream wStream;
                wStream << Base10ToAlpha(wCol - 1) << sDynamicRow - 1 << "+1";
                wCell->Value(wStream.str());
                tSpreadSheetContainer::Instance()->PushJsonCell(wCell);
            }
        }
    }

    tBool tUISpreadSheet::_StepPressureCooperative(tInt sMaxRows) {
        if (!m_PressureActive) return true;
        if (sMaxRows <= 0) sMaxRows = 1;
        const tInt sDynamicRow = m_PressureTotalRows;
        const tInt sDynamicCol = m_PressureCol;
        tInt wRowsDone = 0;
        for (; m_PressureRow <= sDynamicRow && wRowsDone < sMaxRows; m_PressureRow++, wRowsDone++) {
            const tInt wRow = m_PressureRow;
            for (tInt wCol = 1; wCol <= sDynamicCol; wCol++) {
                tCell* wCell = EnsureCell(wRow, wCol);
                tStringStream wStream;
                if (wRow == sDynamicRow) {
                    if (wCol != sDynamicCol) {
                        wStream << "SUM(" << Base10ToAlpha(wCol) << 1 << ":" << Base10ToAlpha(wCol) << wRow - 1 << ")";
                    }
                    else {
                        wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(sDynamicCol - 1) << wRow << ")";
                    }
                }
                else {
                    if (wCol == sDynamicCol) {
                        wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(sDynamicCol - 1) << wRow << ")";
                    }
                    else {
                        wStream << Base10ToAlpha(wCol) << wRow - 1 << "+1";
                    }
                }
                wCell->Value(wStream.str());
                tSpreadSheetContainer::Instance()->PushJsonCell(wCell);
            }
        }
        return (m_PressureRow > sDynamicRow);
    }

    tInt tUISpreadSheet::_PressureCooperativeProgress() {
        if (!m_PressureActive || m_PressureTotalRows <= 1) return 100;
        // rows 2..total => (m_PressureRow - 2) fully-generated rows out of (total - 1).
        tInt wDone = m_PressureRow - 2;
        if (wDone < 0) wDone = 0;
        const tInt wTotal = m_PressureTotalRows - 1;
        if (wTotal <= 0) return 100;
        tInt wPct = (tInt)(((tLongLong)wDone * 100) / (tLongLong)wTotal);
        if (wPct > 100) wPct = 100;
        return wPct;
    }

    tBool tUISpreadSheet::_IsPressureCooperativeActive() {
        return m_PressureActive;
    }

    void tUISpreadSheet::_EndPressureCooperative() {
        if (!m_PressureActive) return;
        cout << "[pressure] generation done in "
             << ((double)(clock() - m_PressureGenClock) * 1000.0 / CLOCKS_PER_SEC) << " ms" << endl;

        // Blocking finalize: batch-compile all queued formulas + cold calculate (the file-load path).
        cout << "Start timer JsonEnd (deferred compile + calculate).. " << endl;
        tApplication::Instance()->TimerStart();
        tSpreadSheetContainer::Instance()->JsonEndProfile(true);
        tSpreadSheetContainer::Instance()->JsonEnd();
        tSpreadSheetContainer::Instance()->JsonEndProfile(false);
        cout << "Elapsed Time "
             << ((double)tApplication::Instance()->TimerElapsed() * 1000.0 / CLOCKS_PER_SEC)
             << " ms" << endl;

        tCell* wCellResult = EnsureCell(m_PressureTotalRows, m_PressureCol);
        tSheet* wSheet = wCellResult->Sheet();
        cout << "Nb Cell=" << wSheet->NbCell() << endl;
        cout << "ColRow allocator  Memory Size=" << wSheet->MemoryColRowSize() << endl;
        cout << "Cell allocator Memory Size=" << wSheet->MemoryCellSize() << endl;
        cout << "Range allocator Memory Size=" << wSheet->MemoryRangeSize() << endl;
        cout << "Total allocator Memory Size " << wSheet->MemoryColRowSize() + wSheet->MemoryCellSize() + wSheet->MemoryRangeSize() << endl;
        cout << "Nb Shared String " << tApplication::Instance()->NbSharedString() << endl;
        cout << "SkSharedFormulaContainer::Size() " << tSpreadSheetContainer::Instance()->NbSharedFormula() << endl;
        cout << "   Result  " << wCellResult->StrRef() << "=" << wCellResult->FormulaStr() << ":" << wCellResult->Value() << endl;

        m_PressureActive = false;
    }

    void tUISpreadSheet::RebaseUIExtraUndo(tSequenceId sSequenceId) {
        tWorkBook* wWorkBook=ActiveWorkBook();
        tSequenceId wCurrentSequence = wWorkBook->UndoRebaseLog().LatestSequence();
        
        tRebasePlan wRebasePlan = wWorkBook->UndoRebaseLog().BuildPlan(
                                                                       sSequenceId,
                                                                       wCurrentSequence,
                                                                       ActiveSheet()->AllocatorRef()
                                                                       );
        tJsonSelect wJsonSelect;
        wJsonSelect.ReadJson(m_UIExtraUndo._Json());
        wJsonSelect.Rebase(wRebasePlan);
        m_UIExtraUndo._Json(wJsonSelect.WriteJson());
    }

    void tUISpreadSheet::_Undo() {
        tUndo* wUndo = LastUndo();
        if (wUndo == nullptr) return;
        tUIExtraUndo* wUIExtraUndo=dynamic_cast<tUIExtraUndo*>(wUndo->Extra());
        if (wUIExtraUndo!=nullptr) {
            m_UIExtraUndo=*wUIExtraUndo;
            tUndoSpreadSheet* wUndoSpreadSheet=dynamic_cast<tUndoSpreadSheet*>(wUndo);
            if (wUndoSpreadSheet!=nullptr) {
                RebaseUIExtraUndo(wUndoSpreadSheet->SequenceId());
            }
        }
       
#ifdef DebugInterface
        cout << "tUISpreadSheet::Undo" << endl;
        cout << "Undo ->" << wUndo->OperationName() << endl;
        cout << "-->" << wUIExtraUndo->_Json() << endl;
#endif
        
        this->Undo();
#ifdef DebugInterface
        /*
        wUndo = LastUndo();
        if (wUndo != nullptr) {
            cout << "UndoAfter ->" << wUndo->OperationName() << endl;
        }
        tUndo* wRedo = LastRedo();
        if (wRedo != nullptr) {
            cout << "Redo ->" << wRedo->OperationName() << endl;
        }
        */
#endif
#ifdef _DEBUGSK
#ifdef checksp
        Check();
#endif
#endif
    };

    void tUISpreadSheet::_Redo() {
        tUndo* wRedo = LastRedo();
        if (wRedo == nullptr) return;
        tUIExtraUndo* wUIExtraUndo=dynamic_cast<tUIExtraUndo*>(wRedo->Extra());
        if (wUIExtraUndo!=nullptr) {
            m_UIExtraUndo=*wUIExtraUndo;
            tUndoSpreadSheet* wUndoSpreadSheet=dynamic_cast<tUndoSpreadSheet*>(wRedo);
            if (wUndoSpreadSheet!=nullptr) {
                RebaseUIExtraUndo(wUndoSpreadSheet->SequenceId());
            }
        }
#ifdef DebugInterface
        cout << "tUISpreadSheet::Redo" << endl;
        cout << "Redo ->" << wRedo->OperationName() << endl;
#endif
        this->Redo();
#ifdef _DEBUGSK
#ifdef checksp
        Check();
#endif
#endif
    }

    tBool tUISpreadSheet::_IsUndoActif() {
        return(IsUndoActif());
    }

    void tUISpreadSheet::_SetIsUndoActif(tBool sIsUndoActif) {
        IsUndoActif(sIsUndoActif);
    }

    void tUISpreadSheet::_SetExtraUndo(tString sJson) {
        m_UIExtraUndo._Json(sJson);
    }

    tString tUISpreadSheet::_GetExtraUndo() {
        return(m_UIExtraUndo._Json());
    }

    void tUISpreadSheet::_SetUserInterface(tString sJson) {
        JsonUser(sJson);
    }

    tString tUISpreadSheet::_GetUserInterface() {
        return(JsonUser());
    }

    tBool tUISpreadSheet::_SetActiveSheet(tString sSheetName) {
        tSheet* wSheet=ActiveSheet(sSheetName);
        return(wSheet!=nullptr);
    }

    tString tUISpreadSheet::_GetActiveSheet() {
        return(ActiveSheet()->Name());
    }

    tBool tUISpreadSheet::_AddSheet(tString sName,tString sLeft) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::AddSheet :"  << sName;
        if (sLeft!="") cout << " Letf:" << sLeft;
        cout << endl;
#endif
        if (UndoAddSheet(sName,sLeft)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
    }

    tBool tUISpreadSheet::_RenameSheet(tString sName,tString sNewName) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::RenameSheet :"  << sName << " to " << sNewName << endl;;
#endif
        if (UndoRenameSheet(sName, sNewName)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
    }

    tBool tUISpreadSheet::_SwapSheet(tString sName1,tString sName2,tBool sInsertAfter) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::SwapSheet :"  << sName1 << "," << sName2
             << (sInsertAfter ? " insertAfter" : " swap") << endl;;
#endif
        if (sInsertAfter) {
            if (UndoSwapSheet(sName1, sName2, true)) {
                FixUndoExtra();
                return(true);
            }
            return(false);
        }
        if (sName2.empty() || sName1 == sName2) {
            return(false);
        }
        if (UndoSwapSheet(sName1, sName2, false)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
    }

    tBool tUISpreadSheet::_DeleteSheet(tString sName) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::DeleteSheet :"  << sName << endl;;
#endif
        if (UndoDeleteSheet(sName)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
    }

    tString tUISpreadSheet::_SheetsList() {
        return(JsonSheets());
    }

    void tUISpreadSheet::FixUndoExtra() {
        tUndo* wUndo=LastUndo();
        if (wUndo!=nullptr)
            wUndo->Extra(new tUIExtraUndo(m_UIExtraUndo));
    }
    

    tBool tUISpreadSheet::SetSheet(tString sSheet) {
        // Use Acttive Sheet ?
        if (sSheet=="") return(true);
        tWorkBook* wWorkBook=ActiveWorkBook();
        tSheet* wSheet=wWorkBook->ActiveSheet(sSheet);
        if (wSheet==nullptr) return(false);
        return(true);
    }

	tBool tUISpreadSheet::_Value(tString sRef, tString sValue, tString sSheet) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::Value(" << sSheet << ":" << sRef << "," << sValue << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);

		tVariant wVariant;
		wVariant.Parse(sValue);
        if (UndoCellValue(sRef, wVariant)) {
#ifdef checksp
           Check();
#endif
           FixUndoExtra();
           return(true);
        };
		return(false);
	}

    tBool tUISpreadSheet::_FillSeries(tString sSourceRef, tString sDestRef, tString sSheet) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::FillSeries(" << sSheet << ":" << sSourceRef
             << " -> " << sDestRef << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        if (UndoFillSeries(sSourceRef, sDestRef)) {
#ifdef checksp
            Check();
#endif
            FixUndoExtra();
            return(true);
        }
        return(false);
    }

    tBool tUISpreadSheet::_ValueAttribute(tString sRef,tString sName, tString sValue, tString sSheet) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::ValueAttribute(" << sSheet << ":" << sRef << "," << sName << "," << sValue << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        tVariant wVariant;
        wVariant.Parse(sValue);
    
#ifdef DebugInterface
        cout << "Value -->" << wVariant << endl;
#endif
        if (UndoCellAttribute(sRef,sName, wVariant)) {
            FixUndoExtra();
            return(true);
        };
        return(false);

}

    namespace {
        // ModelClass "value" property type (SkCellClassCheck → bool, SkCellClassCalendar → date).
        tVariantType CalculableValueModelType(tCell* sCell) {
            if (sCell == nullptr || !sCell->Value().IsClass()) {
                return(tVariantType::t_null);
            }
            tCellClassAttribute* wAttr =
                dynamic_cast<tCellClassAttribute*>(sCell->Value().Class());
            if (wAttr == nullptr) {
                return(tVariantType::t_null);
            }
            tModelClass* wModel = wAttr->ModelClass();
            if (wModel == nullptr) {
                wModel = tClassFactory::Instance()->Get(wAttr->ClassName());
            }
            if (wModel == nullptr) {
                return(tVariantType::t_null);
            }
            tModelProperty* wProp = wModel->Property("value");
            if (wProp == nullptr) {
                return(tVariantType::t_null);
            }
            return(wProp->Type());
        }

        tBool ParseCalculableBoolWire(tString sValue, tVariant& oOut) {
            tString wLower;
            wLower.reserve(sValue.size());
            for (tChar wCh : sValue) {
                wLower.push_back(static_cast<tChar>(std::tolower(static_cast<unsigned char>(wCh))));
            }
            if (wLower == "true" || wLower == "1") {
                oOut.SetBool(true);
                return(true);
            }
            if (wLower == "false" || wLower == "0") {
                oOut.SetBool(false);
                return(true);
            }
            return(false);
        }

        tString VariantWireString(const tVariant& sVariant) {
            tStringStream wStream;
            switch (sVariant.Type()) {
            case tVariantType::t_null:
                wStream << "Null";
                break;
            case tVariantType::t_int:
                wStream << sVariant.Int();
                break;
            case tVariantType::t_bool:
                wStream << sVariant.Bool();
                break;
            case tVariantType::t_double:
                wStream << sVariant.Double();
                break;
            case tVariantType::t_string:
                wStream << sVariant.String();
                break;
            case tVariantType::t_error:
                wStream << sVariant.Error().Error();
                break;
            case tVariantType::t_date: {
                tClassDate wDate(sVariant.Date());
                wStream << wDate.UsDate();
                break;
            }
            case tVariantType::t_class: {
                tVirtualClass* wClass = sVariant.Class();
                tCellClassAttribute* wCellClassAttribute =
                    dynamic_cast<tCellClassAttribute*>(wClass);
                if (wCellClassAttribute != nullptr) {
                    wStream << "{" << wCellClassAttribute->ModelClass()->ClassName() << "}";
                } else if (wClass != nullptr) {
                    wStream << "{" << wClass->ClassName() << "}";
                }
                break;
            }
            default:
                wStream << "Unknow";
                break;
            }
            return wStream.str();
        }
    }

    tBool tUISpreadSheet::_ValueClassCalculable(tString sRef, tString sValue, tString sSheet) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::ValueClassCalculable(" << sSheet << ":" << sRef << "," << sValue << ")" << endl;
#endif
        if (!SetSheet(sSheet)) return(false);
        tVariant wVariant;
        const tVariantType wModelValueType = CalculableValueModelType(Cell(sRef));
        if (sValue.empty()) {
            wVariant.Clear();
        } else if (wModelValueType == tVariantType::t_bool) {
            if (!ParseCalculableBoolWire(sValue, wVariant)) {
                wVariant.Parse(sValue);
            }
        } else if (wModelValueType == tVariantType::t_string) {
            // Keep wire text as t_string (ComboBox labels); Parse() would turn "1" into t_int → date display.
            wVariant.SetString(sValue);
        } else if (wModelValueType == tVariantType::t_date) {
            // Same load path as tVariant::Json case t_date: UsDate(v) → SetDate (tVariantType::t_date).
            auto wTryUsDate = [&](tString sCandidate) -> tBool {
                tClassDate wUsDate;
                wUsDate.UsDate(sCandidate);
                tInt wYear = 0;
                tInt wMonth = 0;
                tInt wDay = 0;
                wUsDate.YearMonthDay(wYear, wMonth, wDay);
                if (wYear >= 1900 && wYear <= 2100 && wMonth >= 1 && wMonth <= 12 && wDay >= 1 && wDay <= 31) {
                    wVariant.SetDate(wUsDate.Value());
                    return(true);
                }
                return(false);
            };

            if (!wTryUsDate(sValue)) {
                tString wNormalized = sValue;
                std::replace(wNormalized.begin(), wNormalized.end(), '/', '-');
                if (!wTryUsDate(wNormalized)) {
                    wVariant.Parse(sValue);
                    if (wVariant.Type() == tVariantType::t_string && wNormalized != sValue) {
                        wVariant.Parse(wNormalized);
                    }
                }
            }
        } else {
            wVariant.Parse(sValue);
        }
#ifdef DebugInterface
        cout << "ValueClassCalculable -->" << wVariant << endl;
#endif
        if (UndoCellClassCalculable(sRef, wVariant)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
    }

	tBool tUISpreadSheet::_ValueString(tString sRef, tString sValue, tString sSheet) {
#ifdef DebugInterface
		cout << "SetString(" << sSheet << ":" << sRef << "=" << sValue << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
		tVariant wVariant = sValue;
        if (UndoCellValue(sRef, wVariant)) {
            FixUndoExtra();
            return(true);
        }
		return(false);
	}


	tBool tUISpreadSheet::_ValueInt(tString sRef, tInt sValue, tString sSheet) {
#ifdef DebugInterface
		cout << "SetInt(" << sSheet << ":" << sRef << "=" << sValue << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
		tVariant wVariant = sValue;
        if (UndoCellValue(sRef, wVariant)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
	}

	tBool tUISpreadSheet::_ValueDouble(tString sRef, tDouble sValue, tString sSheet) {
#ifdef DebugInterface
		cout << "SetDouble(" << sSheet << ":" << sRef << "=" << sValue << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
		tVariant wVariant = sValue;
        if (UndoCellValue(sRef, wVariant)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
	}


	tString tUISpreadSheet::_GetValue(tString sRef, tString sSheet) {
#ifdef DebugInterface
        cout << "GetValue(" << sSheet << ":" << sRef << ")" << endl;
#endif
        if (!SetSheet(sSheet)) {
            return("");
        }
        const tString wResult = VariantWireString(CellValue(sRef));
#ifdef DebugInterface
        cout << wResult << endl;
#endif
        return wResult;
	}

    tString tUISpreadSheet::_GetCalculableScalar(tString sRef, tString sSheet) {
#ifdef DebugInterface
        cout << "GetCalculableScalar(" << sSheet << ":" << sRef << ")" << endl;
#endif
        if (!SetSheet(sSheet)) {
            return("");
        }
        const tString wResult = VariantWireString(CellCalculableScalarValue(sRef));
#ifdef DebugInterface
        cout << wResult << endl;
#endif
        return wResult;
    }

    tString tUISpreadSheet::_GetInputValue(tString sRef, tString sSheet) {
        // Must activate sheet before reading cell input (same order as _GetValue).
        if (!SetSheet(sSheet)) return("");
        tString wResult = CellInputString(sRef);
#ifdef DebugInterface
        cout << "GetInputValue(" << sSheet << ":" << sRef  << ")" << "=" << wResult << endl;
#endif
        return(wResult);
    }

    tString tUISpreadSheet::_GetFormula(tString sRef, tString sSheet, tBool sUser) {
        if (!SetSheet(sSheet)) return("");
        return Formula(sRef, nullptr, sUser);
    }

    tString tUISpreadSheet::_CellRef(tString sRefAnchor, tString sRef, tString sSheet) {
        if (!SetSheet(sSheet)) return("");
        return CellRef(sRefAnchor, sRef, nullptr);
    }

    tString tUISpreadSheet::_Error() {
        return Error();
    }

    tInt tUISpreadSheet::_ErrorLine() {
        return ErrorLine();
    }

    tInt tUISpreadSheet::_ErrorColumn() {
        return ErrorColumn();
    }

    tString tUISpreadSheet::_ErrorWithDetail() {
        return ErrorWithDetail();
    }

    tString tUISpreadSheet::_GetFormulaAttribute(tString sRef, tString sName, tString sSheet) {
#ifdef DebugInterface
        cout << "GetFormulaAttribute(" << sSheet << ":" << sRef << "," << sName << ")" << endl;
#endif
        if (!SetSheet(sSheet)) {
            return("");
        }
        tCellAttribute* wCellAttribute = CellAttribute(sRef, sName);
        if (wCellAttribute != nullptr && wCellAttribute->Formula() != nullptr) {
            return wCellAttribute->FormulaStr();
        }
        return "";
    }

    tString tUISpreadSheet::_GetValueAttribute(tString sRef,tString sName, tString sSheet) {
#ifdef DebugInterface
        cout << "GetValueAttribute(" << sSheet << ":" << sRef << "," << sName << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return("");
       tStringStream wStream;
        tCellAttribute* wCellAttribute=CellAttribute(sRef,sName);
        if (wCellAttribute!=nullptr) {
            tVariant wVariant = wCellAttribute->Value();
            switch (wVariant.Type()) {
                case tVariantType::t_null: wStream << "Null"; break;
                case tVariantType::t_int: wStream << wVariant.Int(); break;
                case tVariantType::t_bool: wStream << wVariant.Bool(); break;
                case tVariantType::t_double: wStream << wVariant.Double(); break;
                case tVariantType::t_string: wStream << wVariant.String(); break;
                case tVariantType::t_error: wStream << wVariant.Error().Error(); break;
                case tVariantType::t_date: {
                    tClassDate wDate(wVariant.Date());
                    wStream << wDate.UsDate();
                    break;
                }
                case tVariantType::t_class: {
                    tVirtualClass* wClass=wVariant.Class();
                    tCellClassAttribute* wCellClassAttribute=dynamic_cast<tCellClassAttribute*>(wClass);
                    if (wCellClassAttribute!=nullptr) {
                        wStream <<  "{" << wCellClassAttribute->ModelClass()->ClassName() << "}";
                    } else {
                        wStream <<  "{" << wClass->ClassName() << "}";
                    }
                    break;
                }
                default:
                    wStream << "Unknow";  break;
            }
        } else {
            wStream << "#Unkwon attribute !";
        }
    #ifdef DebugInterface
        cout << wStream.str() << endl;
    #endif
        return(wStream.str());
    }

    tBool tUISpreadSheet::_Raz(tString sRef, tBool sKeepFormat, tString sSheet) {
#ifdef DebugInterface
            cout << "tUISpreadSheet::Raz(" << sSheet << ":" << sRef << " keepFormat=" << sKeepFormat << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);

        if (UndoRaz(sRef, sKeepFormat)) {
            FixUndoExtra();
            return(true);
        };
        return(false);
    }

    tBool tUISpreadSheet::_RazFormat(tString sRef, tString sSheet) {
#ifdef DebugInterface
            cout << "tUISpreadSheet::RazFormat(" << sSheet << ":" << sRef << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);

        if (UndoRazFormat(sRef)) {
            FixUndoExtra();
            return(true);
        };
        return(false);
    }

	tBool tUISpreadSheet::_SizeRow(tInt sBegin ,tInt sEnd ,tDouble sSize, tString sSheet) {
#ifdef DebugInterface
        cout << "Resize Row(" << sSheet << ":" << sBegin << " Size=" << sSize << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        tBool wOk=false;
        if (sSize==-1) {
            wOk=UndoSizeRow(sBegin,sEnd,sSize);
        } else {
            wOk=UndoSizeRow(sBegin,sEnd, SkMetrics::Convert(sSize, tUnitMetrics::pixels, tUnitMetrics::millimeters));
        }
#ifdef DebugInterface
        cout << "  Size after :" << _GetSizeRow(sBegin, sSheet) << endl;
#endif
        return(wOk);
	}

	tDouble tUISpreadSheet::_GetSizeRow(tInt sIndex, tString sSheet) {
        // Change Sheet ?
        if (ActiveWorkBook()==nullptr) return(0);
        if (!SetSheet(sSheet)) return(0);
        tSheet* wSheet=ActiveWorkBook()->ActiveSheet();
        if (wSheet==nullptr) {
#ifdef DebugInterface
            cerr << "GetSizeRow wSheet==nullptr" << endl;
#endif
            return(0);
        }
        tColRowCellRange* wColRowCellRange=wSheet->ColRowCellRange();
        if (wColRowCellRange==nullptr) { cout << "wColRowCellRange==nullptr" << endl; return(0); }
        // If is In Closed path return 0
        tDouble wSize = SizeRow(sIndex);
		return(SkMetrics::Convert(wSize, tUnitMetrics::millimeters, tUnitMetrics::pixels));
	}

	tBool tUISpreadSheet::_SizeCol(tInt sBegin,tInt sEnd, tDouble sSize, tString sSheet) {
#ifdef DebugInterface
        cout << "Resize Col(" << sSheet << ":" << Base10ToAlpha(sBegin) << " Size=" << sSize << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        tBool wOk=false;
        if (sSize==-1) {
            wOk=UndoSizeCol(sBegin,sEnd,sSize);
        } else {
            wOk=UndoSizeCol(sBegin,sEnd, SkMetrics::Convert(sSize, tUnitMetrics::pixels, tUnitMetrics::millimeters));
        }
        return(wOk);
	}

	tDouble tUISpreadSheet::_GetSizeCol(tInt sIndex, tString sSheet) {
        // Change Sheet ?
        if (ActiveWorkBook()==nullptr) return(0);
        if (!SetSheet(sSheet)) return(0);
        tSheet* wSheet=ActiveWorkBook()->ActiveSheet();
        if (wSheet==nullptr) {
#ifdef DebugInterface
            cerr << "GetSizeCol wSheet==nullptr" << endl;
#endif
            return(0);
        }
        tColRowCellRange* wColRowCellRange=wSheet->ColRowCellRange();
        if (wColRowCellRange==nullptr) { cout << "wColRowCellRange==nullptr" << endl; return(0); }
        // If is In Closed path return 0
        tDouble wSize = SizeCol(sIndex);
		return(SkMetrics::Convert(wSize, tUnitMetrics::millimeters, tUnitMetrics::pixels));
	}


    tBool tUISpreadSheet::_InsertRow(tInt sBegin,tInt sEnd, tString sSheet) {
#ifdef DebugInterface
        cout << "Insert Row(" << sSheet << ":" << sBegin << "," << sEnd << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        tIndex wSize=sEnd-sBegin+1;
        if (wSize<1) return(false);
        if (UndoInsertRow(sBegin, wSize)) {
            FixUndoExtra();
            return(true);
        };
        return(false);
    }

    tBool tUISpreadSheet::_InsertRowByRect(tString sRef, tString sSheet) {
#ifdef DebugInterface
        cout << "Insert Row By Rect(" << sSheet << ":" << sRef << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        tIndex wTop, wLeft, wBottom, wRight;
        if (!ParseRange(sRef, wTop, wLeft, wBottom, wRight)) return(false);
        tRect wRect(wTop, wLeft, wBottom, wRight);
        if (UndoInsertRowByRect(wRect)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
    }

    tBool tUISpreadSheet::_InsertRowByRectWithLabel(tString sRef, tString sLabelRef,
                                                    tString sLabelValue, tString sSheet) {
#ifdef DebugInterface
        cout << "Insert Row By Rect With Label(" << sSheet << ":" << sRef
             << " label=" << sLabelRef << ":" << sLabelValue << ")" << endl;
#endif
        if (!SetSheet(sSheet)) return(false);
        tIndex wTop, wLeft, wBottom, wRight;
        if (!ParseRange(sRef, wTop, wLeft, wBottom, wRight)) return(false);
        tRect wRect(wTop, wLeft, wBottom, wRight);
        if (UndoInsertRowByRectWithLabel(wRect, sLabelRef, sLabelValue)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
    }

    tBool tUISpreadSheet::_DeleteRow(tInt sBegin,tInt sEnd, tString sSheet) {
#ifdef DebugInterface
        cout << "Delete Row(" << sSheet << ":" << sBegin;
        if (sBegin!=sEnd) {
            cout << " to " <<  sEnd << endl;
        }
        cout << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        tIndex wSize=sEnd-sBegin+1;
        if (wSize<1) return(false);
        if (UndoDeleteRow(sBegin, wSize)) {
            FixUndoExtra();
            return(true);
        };
        return(false);
    }

    tBool tUISpreadSheet::_DeleteRowByRect(tString sRef, tString sSheet) {
#ifdef DebugInterface
        cout << "Delete Row By Rect(" << sSheet << ":" << sRef << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        tIndex wTop, wLeft, wBottom, wRight;
        if (!ParseRange(sRef, wTop, wLeft, wBottom, wRight)) return(false);
        tRect wRect(wTop, wLeft, wBottom, wRight);
        if (UndoDeleteRowByRect(wRect)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
    }
        
    tBool tUISpreadSheet::_InsertCol(tInt sBegin,tInt sEnd, tString sSheet) {
#ifdef DebugInterface
        cout << "Insert Col(" << sSheet << ":" << sBegin << "," << sEnd << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        tIndex wSize=sEnd-sBegin+1;
        if (wSize<1) return(false);
        if (UndoInsertCol(sBegin, wSize)) {
            FixUndoExtra();
            return(true);
        };
        return(false);
    }
    
    tBool tUISpreadSheet::_InsertColByRect(tString sRef, tString sSheet) {
#ifdef DebugInterface
        cout << "Insert Col By Rect(" << sSheet << ":" << sRef << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        tIndex wTop, wLeft, wBottom, wRight;
        if (!ParseRange(sRef, wTop, wLeft, wBottom, wRight)) return(false);
        tRect wRect(wTop, wLeft, wBottom, wRight);
        if (UndoInsertColByRect(wRect)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
    }
    tBool tUISpreadSheet::_DeleteCol(tInt sBegin,tInt sEnd, tString sSheet)  {
#ifdef DebugInterface
        cout << "Delete Col(" << sSheet << ":" << Base10ToAlpha(sBegin);
        if (sBegin!=sEnd) {
            cout << " to " << Base10ToAlpha(sEnd) << endl;
        }
        cout << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        tIndex wSize=sEnd-sBegin+1;
        if (wSize<1) return(false);
        if (UndoDeleteCol(sBegin, wSize)) {
            FixUndoExtra();
            return(true);
        };
        return(false);
    }

    tBool tUISpreadSheet::_DeleteColByRect(tString sRef, tString sSheet)  {
#ifdef DebugInterface
        cout << "Delete Col By Rect(" << sSheet << ":" << sRef << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        tIndex wTop, wLeft, wBottom, wRight;
        if (!ParseRange(sRef, wTop, wLeft, wBottom, wRight)) return(false);
        tRect wRect(wTop, wLeft, wBottom, wRight);
        if (UndoDeleteColByRect(wRect)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
    }

    tBool tUISpreadSheet::_Copy(tString sRef, tString sSheet) {
#ifdef DebugInterface
        cout << "Copy(" << sSheet << ":" << sRef << ")" <<  endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        if (Copy(sRef)) {
            FixUndoExtra();
            return(true);
        };
        return(false);
    }

    tBool tUISpreadSheet::_Cut(tString sRef, tString sSheet) {
#ifdef DebugInterface
        cout << "Cut(" << sSheet << ":" << sRef << ")" << endl;
#endif
        if (!SetSheet(sSheet)) {
            return(false);
        }
        if (UndoCut(sRef)) {
#ifdef checksp
            Check();
#endif
            FixUndoExtra();
            return(true);
        }
        return(false);
    }
            
    tBool tUISpreadSheet::_Paste(tString sRef, tString sSheet) {
#ifdef DebugInterface
        tApplication* wApplication=tApplication::Instance();
        tString wCopy=wApplication->Clipboard()->Text();
        cout << "Paste(" << sSheet << ":" << sRef << ")=" << wCopy << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);

       if (UndoPaste(sRef)) {
#ifdef checksp
           Check();
#endif
           FixUndoExtra();
           return(true);
       };
        
       return(false);
    }

    tBool tUISpreadSheet::_Move(tString sSourceRef, tString sDestRef, tString sSheet) {
#ifdef DebugInterface
        cout << "Move(" << sSheet << ":" << sSourceRef << " -> " << sDestRef << ")" << endl;
#endif
        if (!SetSheet(sSheet)) {
            return(false);
        }
        if (UndoMove(sSourceRef, sDestRef)) {
#ifdef checksp
            Check();
#endif
            FixUndoExtra();
            return(true);
        }
        return(false);
    }

    tString tUISpreadSheet::_DEBUGSKFormat() {
        tString wDebug;
#ifdef _DEBUGSK
        if (FormatApi() != nullptr) {
            wDebug = FormatApi()->Debug();
        }
#endif
        cout << "Debug Format (FormatApi / FormatRoot) ----------------------" << endl;
        cout << "Count=" << _FormatCount() << endl;
        cout << wDebug << endl;
        cout << "-----------------------------------" << endl;
        return wDebug;
    }

    tInt tUISpreadSheet::_FormatCount() {
        if (m_FormatApi == nullptr) {
            return 0;
        }
        return static_cast<tInt>(m_FormatApi->Count());
    }

    tBool tUISpreadSheet::_Format(tString sRef, tString sValue, tString sSheet) {
#ifdef DebugInterface
        cout << "Format(" << sRef << "," << sValue << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);

        if (UndoCellFormat(sRef,sValue)) {
#ifdef checksp
           Check();
#endif
            FixUndoExtra();
            return(true);
        };
#ifdef DebugInterface
        cout << "Error Format(" << sRef << "," << sValue << ")" << endl;
#endif

        return(false);
    }
    tBool tUISpreadSheet::_Precision(tString sRef, tBool sInc, tString sSheet) {
#ifdef DebugInterface
        cout << "SetPrecision(" << sSheet << ":" << sRef << "," << sInc << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);

        if (UndoCellPrecision(sRef,sInc)) {
#ifdef checksp
           Check();
#endif
            FixUndoExtra();
            return(true);
        };
        return(false);
    }

    tBool tUISpreadSheet::_ConditionalFormat(tString sType,tString sRef,tString sParam1,tString sParam2,tString sParam3,tString sParam4,tString sParam5,tString sParam6,tString sParam7,tString sParam8,tString sParam9,tString sParam10, tString  sSheet) {
#ifdef DebugInterface
        cout << "_ConditionalFormat(" << sSheet << ":" << sRef << ")" << endl;
        cout << "  Type: " << sType << endl;
        cout << "  Param1: " << sParam1 << endl;
        cout << "  Param2: " << sParam2 << endl;
        cout << "  Param3: " << sParam3 << endl;
        cout << "  Param4: " << sParam4 << endl;
        cout << "  Param5: " << sParam5 << endl;
        cout << "  Param6: " << sParam6 << endl;
        cout << "  Param7: " << sParam7 << endl;
        cout << "  Param8: " << sParam8 << endl;
        cout << "  Param9: " << sParam9 << endl;
        cout << "  Param10: " << sParam10 << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        
        if (UndoConditionalFormat(sType,sRef,sParam1,sParam2,sParam3,sParam4,sParam5,sParam6,sParam7,sParam8,sParam9,sParam10)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
    }

    tBool tUISpreadSheet::_DeleteConditionalFormat(tString sType,tString sRef, tString sSheet) {
#ifdef DebugInterface
        cout << "DeleteConditionalFormat(" << sSheet << ":" << sRef << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        if (UndoDeleteConditionalFormat(sType,sRef)) {
            FixUndoExtra();
            return(true);
        }
        return(false);
    }

    tBool tUISpreadSheet::_Border(tString sRef,tInt sBorder,tString sValue, tString sSheet) {
#ifdef DebugInterface
        cout << "Border(" << sSheet << ":" << sRef << ","  << sBorder << "," << sValue << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);

        if (UndoCellBorder(sRef,sBorder,sValue)) {
#ifdef checksp
           Check();
#endif
            FixUndoExtra();
            return(true);
        };
        return(false);
    }

    tString tUISpreadSheet::_GetFormat(tString sRef, tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) return("");
        return(CellFormat(sRef));
    }

    tString tUISpreadSheet::_DefaultFormatString(tString sFormatString) {
        return(tApplication::Instance()->FormatStringRoot()->DefaultFormatString(sFormatString));
    }

    tString tUISpreadSheet::_FormatValueWithCellFormat(tString sRef, tString sValue, tString sSheet) {
        if (!SetSheet(sSheet)) {
            return("");
        }
        tDouble wNumeric = 0.0;
        try {
            wNumeric = std::stod(sValue);
        } catch (...) {
            return "";
        }
        return FormatValueWithCellFormat(sRef, wNumeric, ActiveSheet());
    }

    tBool tUISpreadSheet::_ApplyFormatString(tString sRef,tString sValue,tString sSheet) {
#ifdef DebugInterface
        cout << "ApplyFormatString(" << sSheet << ":" << sRef << "," << sValue << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);

        return(UndoCellFormat(sRef, "format-string :"+ sValue+";"));
    }

    tString tUISpreadSheet::_JsonFormatString() {
    #ifdef DebugInterface
        cout << "JsonFormatString()" << endl;
    #endif
        return(JsonFormatString());
    }

    tString tUISpreadSheet::_JsonConditionalFormat(tString sSheet) {
    #ifdef DebugInterface
        cout << "JsonConditionalFormat(" << sSheet << ")" << endl;
    #endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return("");
        return(JsonConditionalFormats());
    }

    tBool tUISpreadSheet::_Merge(tString sRef, tString sSheet) {
#ifdef DebugInterface
        cout << "Merge(" << sSheet << ":" << sRef << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        return(UndoApplyMerge(sRef));
    }

    tString tUISpreadSheet::_ReturnMerged(tInt sRow, tInt sCol, tString sSheet) {
#ifdef DebugInterface
        cout << "ReturnMerged(" << sSheet << ":" << sRow << "," << sCol << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return("");

        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        tRange* wRange=ActiveSheet()->MergedRange(sRow,sCol);
        if (wRange!=nullptr) {
            wWriter.StartObject();
            wWriter.Key("r_l"); wWriter.Int64(wRange->Left()->Index());
            wWriter.Key("r_t"); wWriter.Int64(wRange->Top()->Index());
            wWriter.Key("r_r"); wWriter.Int64(wRange->Right()->Index());
            wWriter.Key("r_b"); wWriter.Int64(wRange->Bottom()->Index());
            wWriter.EndObject();
        }
        return(wStringBuffer.GetString());
    }

    tString tUISpreadSheet::_ReturnRangeMerged(tString sRef, tString sSheet) {
#ifdef DebugInterface
        cout << "ReturnRangeMerged(" << sSheet << ":" << sRef << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return("");
        tString wResult="";
        tIndex wTop, wLeft, wBottom, wRight;

        //cout << "  Internal Parse " << sRef << endl;
        if (ParseRange(sRef, wTop, wLeft, wBottom, wRight)) {
            StringBuffer wStringBuffer;
            Writer<StringBuffer> wWriter(wStringBuffer);
            wWriter.StartObject();
            wWriter.Key("rs");
            wWriter.StartArray();
            tVectorRange wVectorRange;
            tRect wRect(wTop,wLeft,wBottom,wRight);
            FindRangesCovered(wRect, &wVectorRange);
            //cout << "--------------" << endl;
            for(auto wRange : wVectorRange) {
#ifdef DebugInterface
                //cout << "    Internal Range " << wRange->tItem::StrRef() << endl;
#endif
                wWriter.StartObject();
                wWriter.Key("r_l"); wWriter.Int64(wRange->Left()->Index());
                wWriter.Key("r_t"); wWriter.Int64(wRange->Top()->Index());
                wWriter.Key("r_r"); wWriter.Int64(wRange->Right()->Index());
                wWriter.Key("r_b"); wWriter.Int64(wRange->Bottom()->Index());
                wWriter.EndObject();
            }
            wWriter.EndArray();
            wWriter.EndObject();
            wResult=wStringBuffer.GetString();
        }
        return(wResult);
    }

    tString tUISpreadSheet::_ReturnRangeMergedFusion(tString sRef, tString sSheet) {
        tString wResult="";
        tIndex wTop, wLeft, wBottom, wRight;
        
        // Change Sheet ?
        if (!SetSheet(sSheet)) return("");
        
#ifdef DebugInterface
        cout << "_ReturnRangeMergedFusion(" << sSheet << ":" << sRef << ")" << endl;
#endif
        //cout << "  Internal Parse " << sRef << endl;
        if (ParseRange(sRef, wTop, wLeft, wBottom, wRight)) {
            tVectorRange wVectorRange;
            tRect wRect(wTop,wLeft,wBottom,wRight);
            tRect wRectSave;
            //cout << "  Internal while Rect " << wRect.StrRef() << endl;
            // Loop to take all nested ranges
            while(!(wRect==wRectSave))  {
                wRectSave=wRect;
                FindRangesCovered(wRect, &wVectorRange);
                //cout << "--------------" << endl;
                for(auto wRange : wVectorRange) {
                    cout << "    Internal Range " << wRange->tItem::StrRef() << endl;
                    if (wRange->Top()->Index()<wTop) wTop=wRange->Top()->Index();
                    if (wRange->Left()->Index()<wLeft) wLeft=wRange->Left()->Index();
                    if (wRange->Bottom()->Index()>wBottom) wBottom=wRange->Bottom()->Index();
                    if (wRange->Right()->Index()>wRight) wRight=wRange->Right()->Index();
                }
                wRect.Top(wTop);
                wRect.Left(wLeft);
                wRect.Bottom(wBottom);
                wRect.Right(wRight);
            }

            StringBuffer wStringBuffer;
            Writer<StringBuffer> wWriter(wStringBuffer);
            wWriter.StartObject();
            wWriter.Key("r_l"); wWriter.Int64(wLeft);
            wWriter.Key("r_t"); wWriter.Int64(wTop);
            wWriter.Key("r_r"); wWriter.Int64(wRight);
            wWriter.Key("r_b"); wWriter.Int64(wBottom);
            wWriter.EndObject();
            wResult=wStringBuffer.GetString();
        }
        return(wResult);
    }

    tBool tUISpreadSheet::_InsertNamedRange(tString sName, tString sRef, tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) {
            return(false);
        }
        return(UndoInsertRangeNamed(sName, sRef));
    }

    tBool tUISpreadSheet::_DeleteNamedRange(tString sName, tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        return(UndoDeleteRangeNamed(sName));
    }

    tBool tUISpreadSheet::_UpdateNamedRange(tString sOldName, tString sNewName,
                                            tString sNewRef, tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        return(UndoUpdateRangeNamed(sOldName, sNewName, sNewRef));
    }

    tBool tUISpreadSheet::_IsRenameAllowed() {
        return(IsRenameAllowed());
    }

    void tUISpreadSheet::_SetMultiUserActive(tBool sActive) {
        MultiUserActive(sActive);
    }

    tBool tUISpreadSheet::_InsertFormulaNamed(tString sName, tString sFormula, tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) {
            return(false);
        }
        return(UndoInsertFormulaNamed(sName, sFormula));
    }

    tBool tUISpreadSheet::_DeleteFormulaNamed(tString sName, tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        return(UndoDeleteFormulaNamed(sName));
    }

    tBool tUISpreadSheet::_InsertFloatingObject(tString sName, tString sClassName, tString sTargetSheetName, tString sSheet,
                                               tDouble sDiffX, tDouble sDiffY, tDouble sWidth, tDouble sHeight,
                                               tDouble sOpacity, tString sAnchorCellRef) {
        if (!SetSheet(sSheet)) {
            return (false);
        }
        if (UndoInsertFloatingObject(sName, sClassName, sTargetSheetName, nullptr, sDiffX, sDiffY, sWidth, sHeight,
                                       sOpacity, sAnchorCellRef)) {
            FixUndoExtra();
            return (true);
        }
        return (false);
    }

    tBool tUISpreadSheet::_DeleteFloatingObject(tString sName, tString sSheet) {
        if (!SetSheet(sSheet)) {
            return (false);
        }
        if (UndoDeleteFloatingObject(sName)) {
            FixUndoExtra();
            return (true);
        }
        return (false);
    }

    tBool tUISpreadSheet::_FloatingObjectLayout(tString sName, tDouble sDiffX, tDouble sDiffY, tDouble sWidth, tDouble sHeight,
                                                tDouble sOpacity, tString sAnchorCellRef, tString sSheet) {
        if (!SetSheet(sSheet)) {
            return (false);
        }
        if (UndoFloatingObjectLayout(sName, sDiffX, sDiffY, sWidth, sHeight, sOpacity, sAnchorCellRef)) {
            FixUndoExtra();
            return (true);
        }
        return (false);
    }

    tBool tUISpreadSheet::_FloatingObjectBringToFront(tString sName, tString sSheet) {
        if (!SetSheet(sSheet)) {
            return (false);
        }
        if (UndoFloatingObjectBringToFront(sName)) {
            FixUndoExtra();
            return (true);
        }
        return (false);
    }

    tBool tUISpreadSheet::_FloatingObjectAttribute(tString sName, tString sAttribute, tString sValue, tString sSheet) {
        if (!SetSheet(sSheet)) {
            return (false);
        }
        tVariant wVariant;
        wVariant.Parse(sValue);
        if (UndoFloatingObjectAttribute(sName, sAttribute, wVariant)) {
            FixUndoExtra();
            return (true);
        }
        return (false);
    }

    namespace {
        tBool ParseAttributeWireBatch(tString sAttributesJson, tVectorCellAttributeWire* sBatch) {
            if (sBatch == nullptr) {
                return (false);
            }
            sBatch->clear();
            Document wDocument;
            wDocument.Parse(sAttributesJson.c_str());
            if (wDocument.HasParseError() || !wDocument.IsArray()) {
                return (false);
            }
            sBatch->reserve(wDocument.Size());
            for (rapidjson::SizeType wI = 0; wI < wDocument.Size(); ++wI) {
                const Value& wItem = wDocument[wI];
                if (!wItem.IsObject() || !wItem.HasMember("n") || !wItem["n"].IsString()) {
                    continue;
                }
                tCellAttributeWire wWire;
                wWire.Name = wItem["n"].GetString();
                tString wValue;
                if (wItem.HasMember("v")) {
                    const Value& wValueJson = wItem["v"];
                    if (wValueJson.IsString()) {
                        wValue = wValueJson.GetString();
                    } else if (wValueJson.IsBool()) {
                        wValue = wValueJson.GetBool() ? "true" : "false";
                    } else if (wValueJson.IsInt()) {
                        tStringStream wStream;
                        wStream << wValueJson.GetInt();
                        wValue = wStream.str();
                    } else if (wValueJson.IsDouble()) {
                        tStringStream wStream;
                        wStream << wValueJson.GetDouble();
                        wValue = wStream.str();
                    } else if (wValueJson.IsNull()) {
                        wValue = "";
                    } else {
                        return (false);
                    }
                }
                wWire.Value.Parse(wValue);
                sBatch->push_back(std::move(wWire));
            }
            return (!sBatch->empty());
        }
    }

    tBool tUISpreadSheet::_FloatingObjectAttributes(tString sName, tString sAttributesJson, tString sSheet) {
        if (!SetSheet(sSheet)) {
            return (false);
        }
        tVectorCellAttributeWire wBatch;
        if (!ParseAttributeWireBatch(sAttributesJson, &wBatch)) {
            return (false);
        }
        if (UndoFloatingObjectAttributes(sName, wBatch)) {
            FixUndoExtra();
            return (true);
        }
        return (false);
    }

    tBool tUISpreadSheet::_CellClassAttributes(tString sRef, tString sAttributesJson, tString sSheet) {
        if (!SetSheet(sSheet)) {
            return (false);
        }
        tVectorCellAttributeWire wBatch;
        if (!ParseAttributeWireBatch(sAttributesJson, &wBatch)) {
            return (false);
        }
        if (UndoCellClassAttributes(sRef, wBatch)) {
            FixUndoExtra();
            return (true);
        }
        return (false);
    }

   
    tString tUISpreadSheet::_JsonView(tInt sRow, tInt sCol, tDouble sViewHeight, tDouble sViewWidth,tDouble sDiffY, tDouble sDiffX, tString sSheet, tBool sCss) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::_JsonView("<< sSheet << ":" << sRow << "," << sCol << "," << sViewHeight  << "," << sViewWidth << "," << sDiffY << "," << sDiffX << ", sCss=" << (sCss ? "true" : "false") << ")" << endl;
#endif
        // Change Sheet ?   
        if (!SetSheet(sSheet)) return("");
        return(JsonView(sRow, sCol, tUnitMetrics::pixels, sViewHeight, sViewWidth,sDiffY,sDiffX,sCss));
	};

    tString tUISpreadSheet::_JsonFloatingObjectsForSheet(tString sTargetSheetName, tString sSheet) {
        if (!SetSheet(sSheet)) {
            return ("{\"objects\":[]}");
        }
        return (JsonFloatingObjectsForSheet(sTargetSheetName));
    }

    tString tUISpreadSheet::_JsonFloatingObjects(tString sSheet) {
        if (!SetSheet(sSheet)) {
            return ("{\"objects\":[]}");
        }
        return (JsonFloatingObjects());
    }

    tString tUISpreadSheet::_JsonRightJustify(tIndex sCol,tDouble sViewWidth, tString sSheet) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::_JsonRightJustify("<< sSheet << ":" << sCol << "," << sViewWidth << ")" << endl;
#endif
        // Change Sheet ?   
        if (!SetSheet(sSheet)) return("");
        return(JsonRightJustify(sCol, tUnitMetrics::pixels, sViewWidth));
    }

    tString tUISpreadSheet::_JsonBottomJustify(tIndex sRow,tDouble sViewHeight, tString sSheet) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::_JsonBottomJustify("<< sSheet << ":" << sRow << "," << sViewHeight << ")" << endl;
#endif
        // Change Sheet ?   
        if (!SetSheet(sSheet)) return("");
        return(JsonBottomJustify(sRow, tUnitMetrics::pixels, sViewHeight));
    }

    // Class ==============================================================
    tBool tUISpreadSheet::_RegisterClassAttribute(tString sClassName,tString sLabel,tString sFamily) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::RegisterClass("<< sClassName  << ", " << sLabel << "," << sFamily <<")" << endl;
#endif
        return(tApi::RegisterClassAttribute(sClassName,sLabel,sFamily));
    }
    
    tBool tUISpreadSheet::_AddProperty(tString sName, tString sType, tString sLabel, tSize sOrder, tString sDefaultValue, tString sKind) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::AddProperty on "<< m_LastModelClassAttribute->ClassName() <<" ...." << endl;
#endif
        return(tApi::AddProperty(sName, sType, sLabel, sOrder, sDefaultValue, sKind));
    }

    tBool tUISpreadSheet::_CellClass(tString sRef, tString sClassName) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::UndoCellClass("<< sRef << "," << sClassName <<")" << endl;
#endif
        
        return(UndoCellClass(sRef, sClassName));
    }

    tString tUISpreadSheet::_JsonCellClass() {
        return(tClassFactory::Instance()->Json());
    }

    tString tUISpreadSheet::_JsonCellClassByName(tString sClassName) {
        return(tClassFactory::Instance()->Json(sClassName));
    }

    tBool tUISpreadSheet::_ApplyUnit(tString sRef,tString sFamily,tString sUnit, tString sSheet) {
    #ifdef DebugInterface
        cout << "tUISpreadSheet::UndoApplyUnit("<< sRef << "," << sFamily  << ":" << sUnit <<")" << endl;
    #endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        
        return(UndoApplyUnit(sRef, sFamily, sUnit));
    }

    tString tUISpreadSheet::_MoveCell(tInt sRow, tInt sCol, tByte sKey,tByte sMeta, tInt sTop,tInt sLeft, tInt sBottom, tInt sRight, tString sSheet) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::MoveCell("<< Base10ToAlpha(sCol) << sRow << ", Key:" << sKey << ", Meta:" <<  sMeta << "," << Base10ToAlpha(sLeft) << sTop << ":" << Base10ToAlpha(sRight) << sBottom <<  ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return("");
        
        tRect wRect(MoveCell(tPoint(sRow,sCol), sKey, sMeta,tRect(sTop,sLeft,sBottom,sRight)));
        
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        wWriter.Key("r_l"); wWriter.Int64(wRect.Col());
        wWriter.Key("r_t"); wWriter.Int64(wRect.Row());
        wWriter.Key("r_r"); wWriter.Int64(wRect.Right());
        wWriter.Key("r_b"); wWriter.Int64(wRect.Bottom());
        wWriter.EndObject();
        return(wStringBuffer.GetString());
    }

    tString tUISpreadSheet::_MoveToCell(tInt sRow,tInt sCol,tInt sDirection, tString sSheet) {
        tPoint wPoint(MoveToCell(tPoint(sRow,sCol), sDirection));
#ifdef DebugInterface
        cout << "tUISpreadSheet::MoveToCell("<< wPoint.StrRef() << ")" << endl;
#endif
        // Change Sheet ?
        if (!SetSheet(sSheet)) return("");
        
        tStringStream wStream;
        wStream << "{" << "\"r\":" << wPoint.Row() << ",\"c\":" << wPoint.Col() << "}";
        return(wStream.str());
    }

    tString tUISpreadSheet::_JsonBottomRight(tString sSheet) {
        // Delegates to tApi::BottomRight(nullptr): LastRow/LastCol from ColRowCellRange — identical to native tests
        // that call tApi::BottomRight(tSheet*). Large LastCol (e.g. near Excel grid max) is engine state, not this wrapper.
        // Change Sheet ?
        if (!SetSheet(sSheet)) return("");

        tPoint wPoint=BottomRight();
        tStringStream wStream;
        wStream << "{\"r\":" << wPoint.Row() << ",\"c\":" << wPoint.Col() << "}";
        return wStream.str();
    }

    tString tUISpreadSheet::_JsonPixelBottomRight(tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) return("");
        
        tPoint wPoint=BottomRight();
        tDouble wHeight=SumPixelHeight(1, wPoint.Row());
        tDouble wWidth=SumPixelWidth(1, wPoint.Col());
#ifdef DebugInterface
        //cout << "tUISpreadSheet::BottomRightPixel(" << sSheet << ":" << std::fixed << std::setprecision(3) << wHeight << "," << wWidth << ")" << endl;
#endif
        tStringStream wStream;
        wStream << "{" << "\"r\":" << std::fixed << std::setprecision(3) << wHeight << ",\"c\":" << wWidth << "}";
        return(wStream.str());
    }
    
    tString tUISpreadSheet::_JsonRangeNamed() {
        return(JsonRangeNamed());
    }
    
    tString tUISpreadSheet::_JsonFormulaNamed() {
        return(JsonformulaNamed());
    }

    tString tUISpreadSheet::_JsonPrintParameters(tString sSheet) {
        if (!SetSheet(sSheet)) {
            return("");
        }
        return(JsonPrintParameters(nullptr));
    }

    tBool tUISpreadSheet::_SetJsonPrintParameters(tString sJson, tString sSheet) {
        if (!SetSheet(sSheet)) {
            return(false);
        }
        tSheet* wSheet = ActiveSheet();
        if (wSheet == nullptr) {
            return(false);
        }
        tPrintParameters wProbe;
        if (!wProbe.JsonParse(sJson)) {
            return(false);
        }
        return(JsonPrintParameters(sJson, wSheet));
    }
        
    tString tUISpreadSheet::_JsonRangeData() {
        return(JsonRangeData());
    }

    tString tUISpreadSheet::_JsonFindUniqueValue(tString sRef, tString sSheet) {
        if (!SetSheet(sSheet)) {
            return("");
        }
        tIndex wTop = 0;
        tIndex wLeft = 0;
        tIndex wBottom = 0;
        tIndex wRight = 0;
        if (!ParseRange(sRef, wTop, wLeft, wBottom, wRight)) {
            return("");
        }
        return(JsonFindUniqueValue(tRect(wTop, wLeft, wBottom, wRight)));
    }

    tString tUISpreadSheet::_JsonFindCell(tString sSearch, tBool sMatchCase, tBool sMatchEntireCell, tString sSheet) {
        if (!sSheet.empty()) {
            if (!SetSheet(sSheet)) {
                return "";
            }
            return JsonFindCell(sSearch, sMatchCase, sMatchEntireCell);
        }
        (void)SetSheet(sSheet);
        return JsonFindCellWorkBook(sSearch, sMatchCase, sMatchEntireCell);
    }

    tBool tUISpreadSheet::_UndoApplyRangeData(tString sName, tString sJsonData, tString sSheet) {
        if (!SetSheet(sSheet)) {
            return(false);
        }
        return(UndoApplyRangeData(sName, sJsonData));
    }

    tBool tUISpreadSheet::_UndoAddRangeData(tString sName, tString sRef, tString sJsonData, tString sSheet) {
        if (!SetSheet(sSheet)) {
            return(false);
        }
        return(UndoAddRangeData(sName, sRef, sJsonData));
    }
    

    tBool tUISpreadSheet::_OpenCloseTreeRow(tIndex sRow, tString sSheet) {
        if (!SetSheet(sSheet)) return(false);
        if (Client() && !SetActiveWorkBook()) return(false);
        tSheet* wSheet = ActiveSheet();
        if (wSheet == nullptr) return(false);
        return(UndoOpenCloseTreeRow(sRow, wSheet));
    }

    tBool tUISpreadSheet::_OpenCloseTreeCol(tIndex sRow, tString sSheet) {
        if (!SetSheet(sSheet)) return(false);
        if (Client() && !SetActiveWorkBook()) return(false);
        tSheet* wSheet = ActiveSheet();
        if (wSheet == nullptr) return(false);
        return(UndoOpenCloseTreeCol(sRow, wSheet));
    }

    tBool tUISpreadSheet::_ChangeTreeRow(tBool sRight,tIndex sRow,tIndex sSize, tString sSheet) {
        if (!SetSheet(sSheet)) return(false);
        if (Client() && !SetActiveWorkBook()) return(false);
        tSheet* wSheet = ActiveSheet();
        if (wSheet == nullptr) {
            std::fprintf(stdout, "[SkSpreadSheet] ChangeTreeRow: ActiveSheet is null\n");
            return(false);
        }
        return(UndoChangeTreeRow(sRight, sRow, sSize, wSheet));
    }

    tBool tUISpreadSheet::_ChangeTreeCol(tBool sRight,tIndex sCol,tIndex sSize, tString sSheet) {
        if (!SetSheet(sSheet)) return(false);
        if (Client() && !SetActiveWorkBook()) return(false);
        tSheet* wSheet = ActiveSheet();
        if (wSheet == nullptr) {
            std::fprintf(stdout, "[SkSpreadSheet] ChangeTreeCol: ActiveSheet is null\n");
            return(false);
        }
        return(UndoChangeTreeCol(sRight, sCol, sSize, wSheet));
    }

    tBool tUISpreadSheet::_SplitView(tByte sCde, tIndex sPosition, tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);

        return(UndoSplitView(sCde, sPosition));
    }

    tIndex tUISpreadSheet::_SplitFreezeCol(tString sSheet) {
        if (!SetSheet(sSheet)) {
            return(static_cast<tIndex>(-1));
        }
        tSheet* const wSheet = ActiveSheet();
        if (wSheet == nullptr) {
            return(static_cast<tIndex>(-1));
        }
        return(wSheet->SplitV());
    }

    tIndex tUISpreadSheet::_SplitFreezeRow(tString sSheet) {
        if (!SetSheet(sSheet)) {
            return(static_cast<tIndex>(-1));
        }
        tSheet* const wSheet = ActiveSheet();
        if (wSheet == nullptr) {
            return(static_cast<tIndex>(-1));
        }
        return(wSheet->SplitH());
    }
    // Class ==============================================================
    tDouble tUISpreadSheet::_SumPixelHeight(tIndex sRowStart,tIndex sRowEnd, tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(0);
        
        tDouble wHeight=SumPixelHeight(sRowStart,sRowEnd);
#ifdef DebugInterface_ // Too Many call 
        cout << "tUISpreadSheet::_SumPixelHeight("  << sRowStart << "," << sRowEnd << ")=" << std::fixed << std::setprecision(3) << wHeight << endl;
#endif
        return(wHeight);
    }

    tDouble tUISpreadSheet::_SumPixelWidth(tIndex sColStart,tIndex sColEnd, tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(0);
        
        tDouble wWidth=SumPixelWidth(sColStart,sColEnd);
#ifdef DebugInterface_
        cout << "tUISpreadSheet::_SumPixelWidth("  << Base10ToAlpha(sColStart) << "," <<  Base10ToAlpha(sColEnd) << ")=" << std::fixed << std::setprecision(3) << wWidth << endl;
#endif
        return(wWidth);
    }


    tString tUISpreadSheet::_JsonColByPixel(tIndex sColStart,tDouble sPixel, tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) return("");
        
        return(JsonColByPixel(sColStart, sPixel));
    }

    tString tUISpreadSheet::_JsonRowByPixel(tIndex sRowStart,tDouble sPixel, tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) return("");
        return(JsonRowByPixel(sRowStart, sPixel));
    }

	tString tUISpreadSheet::_Base10toAlpha(tInt sValue) {
		return(Base10ToAlpha(sValue));
	}
	tInt tUISpreadSheet::_AlphaToBase10(tString sValue) {
		return(AlphaToBase10(sValue));
	}

    tString tUISpreadSheet::_ParseCell(tString sRef) {
        tIndex wRow = 0;
        tIndex wCol = 0;
        if (!ParseCell(sRef, wRow, wCol)) {
            return "{\"ok\":false}";
        }
        return std::string("{\"ok\":true,\"row\":") + std::to_string(wRow)
            + ",\"col\":" + std::to_string(wCol) + "}";
    }

    tString tUISpreadSheet::_ParseRange(tString sRef) {
        tIndex wTop = 0;
        tIndex wLeft = 0;
        tIndex wBottom = 0;
        tIndex wRight = 0;
        if (!ParseRange(sRef, wTop, wLeft, wBottom, wRight)) {
            return "{\"ok\":false}";
        }
        return std::string("{\"ok\":true,\"top\":") + std::to_string(wTop)
            + ",\"left\":" + std::to_string(wLeft)
            + ",\"bottom\":" + std::to_string(wBottom)
            + ",\"right\":" + std::to_string(wRight) + "}";
    }

    tString tUISpreadSheet::_QualifyRefsForSheet(tString sSheet, tString sText) {
        return QualifyRefsForSheet(sSheet, sText);
    }

    tString tUISpreadSheet::_StripTargetSheetFromRefs(tString sSheet, tString sText) {
        return StripTargetSheetFromRefs(sSheet, sText);
    }

    tString tUISpreadSheet::_CollectFormulaRefs(tString sText, tString sSheet, tIndex sRow, tIndex sCol) {
        return CollectFormulaRefsJson(ActiveWorkBook(), sSheet, sRow, sCol, sText);
    }

    tIndex tUISpreadSheet::_MaxCol() {
        return(Cst_MaxCol);
    }
    tIndex tUISpreadSheet::_MaxRow() {
        return(Cst_MaxRow);
    }

    tBool tUISpreadSheet::_EnsureCell(tString sRef, tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) return(false);
        
        if (Cell(sRef)==nullptr) {
            EnsureCell(sRef);
            return(true);
        }
        return(false);
    }

    void tUISpreadSheet::_SetLang(tString sLang) {
        tApplication::Instance()->Locale()->Lang(sLang);
    }

    tBool tUISpreadSheet::_AddFunction(tString sName,tString sLabel,tString sFamily, tInt sNbArg) {
#ifdef DebugInterface
        cout << "tUISpreadSheet::AddFunction("<< sName << "," << sNbArg << ")" << endl;
#endif
        tFunctionDictionary* wDictionary=m_SpreadSheetContainer->FunctionDictionary();
        return(wDictionary->AddFunctionRef(sName,sLabel,sFamily,sNbArg, new tFunctionJavascript(sName,sNbArg)));
        
    }
	void tUISpreadSheet::_Pressure(tInt sDynamicRow, tInt sDynamicCol, tString sSheet) {
        // Change Sheet ?
        if (!SetSheet(sSheet)) return;
		tCell* wCell;
		cout << "Start timer ensure cells with formula and SUM() " << (sDynamicRow * sDynamicCol) << " Cells on " << sDynamicRow << "  rows " << sDynamicCol << " cols." << endl;
		tApplication::Instance()->TimerStart();
		// Emulate the file-load path: defer formula compilation. JsonBegin + PushJsonCell queue the
		// formulas as plain string values; JsonEnd (below) compiles them all in one batch AFTER the
		// whole grid exists. No ranges are wired per cell, so EnsureRow's AttachFullColumnRangesToNewRow
		// scans empty column containers -> generation stays linear (avoids the O(n^2) seen with
		// per-cell CompilCell).
		tSpreadSheetContainer::Instance()->JsonBegin();
        // Set Last Cell
        tCell* wCellExtend = EnsureCell(sDynamicRow, sDynamicCol);

		for (tInt wCol = 1; wCol <= sDynamicCol; wCol++) {
			if (wCol > 1) {
				wCell = EnsureCell(1, wCol);
				tStringStream wStream;
				wStream << Base10ToAlpha(wCol - 1) << sDynamicRow - 1 << "+1";
				tString wFormulaStr = wStream.str();
				wCell->Value(wFormulaStr);
				tSpreadSheetContainer::Instance()->PushJsonCell(wCell);
			}
        }
		

		// Per-batch timing: log CPU time for each block of rows so we can see whether
		// the per-row cost stays flat (linear) or grows batch after batch (O(n^2)).
		// Split the cost between EnsureCell (sparse-array growth) and CompilCell
		// (formula parse + dependency/range wiring) to localize which side scales.
		const tInt wBatchRows = 1000;
		clock_t wBatchClock = clock();
		clock_t wGenClock = clock();
		clock_t wEnsureAccum = 0;   // CPU ticks spent in EnsureCell for this batch
		clock_t wCompilAccum = 0;   // CPU ticks spent in CompilCell for this batch
		// Split the "ensure" cost by whether the call also creates a new row.
		// wCol == 1 is the first cell of a fresh row: EnsureCell -> EnsureRow (row
		// allocator + AttachFullColumnRangesToNewRow). wCol > 1 only creates the cell
		// (row already exists). If the sudden step around ~20k rows is in row
		// creation, wFirstColAccum jumps; if it is in plain cell allocation, the
		// per-cell wOtherColAccum jumps too.
		clock_t wFirstColAccum = 0; // ensure ticks for wCol == 1 (row-creating call)
		clock_t wOtherColAccum = 0; // ensure ticks for wCol  > 1 (cell-only call)
		for (tInt wRow = 2; wRow <= sDynamicRow; wRow++) {
			for (tInt wCol = 1; wCol <= sDynamicCol; wCol++) {
				clock_t wT0 = clock();
				wCell = EnsureCell(wRow, wCol);
				clock_t wT1 = clock();
				wEnsureAccum += (wT1 - wT0);
				if (wCol == 1) wFirstColAccum += (wT1 - wT0);
				else           wOtherColAccum += (wT1 - wT0);
				tStringStream wStream;
				if (wRow == sDynamicRow) {
					if (wCol != sDynamicCol) {
						wStream << "SUM(" << Base10ToAlpha(wCol) << 1 << ":" << Base10ToAlpha(wCol) << wRow - 1 << ")";
					}
					else {
						wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(sDynamicCol - 1) << wRow << ")";
					}
				}
				else {
					if (wCol == sDynamicCol) {
						wStream << "SUM(" << Base10ToAlpha(1) << wRow << ":" << Base10ToAlpha(sDynamicCol - 1) << wRow << ")";
						//wStream << "1";
					}
					else {
						wStream << Base10ToAlpha(wCol) << wRow - 1 << "+1";
					}
				}
				tString wFormulaStr = wStream.str();
				clock_t wT2 = clock();
				// Deferred: queue the formula string; JsonEnd compiles it later (see JsonBegin above).
				// "compil" below now measures only the enqueue cost (near 0); the real compile time
				// moves to the JsonEnd timer.
				wCell->Value(wFormulaStr);
				tSpreadSheetContainer::Instance()->PushJsonCell(wCell);
				clock_t wT3 = clock();
				wCompilAccum += (wT3 - wT2);
			}
			if (wRow % wBatchRows == 0) {
				const double wBatchMs = (double)(clock() - wBatchClock) * 1000.0 / CLOCKS_PER_SEC;
				const double wTotalMs = (double)(clock() - wGenClock) * 1000.0 / CLOCKS_PER_SEC;
				const double wEnsureMs = (double)wEnsureAccum * 1000.0 / CLOCKS_PER_SEC;
				const double wCompilMs = (double)wCompilAccum * 1000.0 / CLOCKS_PER_SEC;
				const double wFirstColMs = (double)wFirstColAccum * 1000.0 / CLOCKS_PER_SEC;
				const double wOtherColMs = (double)wOtherColAccum * 1000.0 / CLOCKS_PER_SEC;
				cout << "[pressure] rows " << (wRow - wBatchRows + 1) << ".." << wRow
				     << " | " << (wRow * sDynamicCol) << " cells total"
				     << " | last " << wBatchRows << " rows: " << wBatchMs << " ms"
				     << " (ensure: " << wEnsureMs << " ms [row-create: " << wFirstColMs
				     << " ms, cell-only: " << wOtherColMs << " ms], compil: " << wCompilMs << " ms)"
				     << " | cumulative: " << wTotalMs << " ms" << endl;
				wBatchClock = clock();
				wEnsureAccum = 0;
				wCompilAccum = 0;
				wFirstColAccum = 0;
				wOtherColAccum = 0;
			}
		}
		cout << "[pressure] generation done in "
		     << ((double)(clock() - wGenClock) * 1000.0 / CLOCKS_PER_SEC) << " ms" << endl;

		// Compile all queued formulas in one batch + calculate — the exact file-load path.
		// No cached values here (fresh generation), so JsonEnd runs a full EndCalculate; this
		// cleanly isolates the cold-calc cost from the now-linear generation above.
		cout << "Start timer JsonEnd (deferred compile + calculate).. " << endl;
		tApplication::Instance()->TimerStart();
		tSpreadSheetContainer::Instance()->JsonEndProfile(true);
		tSpreadSheetContainer::Instance()->JsonEnd();
		tSpreadSheetContainer::Instance()->JsonEndProfile(false);
		// TimerElapsed() returns raw clock_t CPU ticks, NOT milliseconds. Convert to ms
		// with CLOCKS_PER_SEC (1e6 under emscripten -> the raw value was microseconds,
		// i.e. inflated ~1000x when printed as "ms"). Same conversion as the batch timers.
		cout << "Elapsed Time "
		     << ((double)tApplication::Instance()->TimerElapsed() * 1000.0 / CLOCKS_PER_SEC)
		     << " ms" << endl;

		tSheet* wSheet = wCell->Sheet();
		cout << "Nb Cell=" << wSheet->NbCell() << endl;
		cout << "ColRow allocator  Memory Size=" << wSheet->MemoryColRowSize() << endl;
		cout << "Cell allocator Memory Size=" << wSheet->MemoryCellSize() << endl;
		cout << "Range allocator Memory Size=" << wSheet->MemoryRangeSize() << endl;
		cout << "Total allocator Memory Size " << wSheet->MemoryColRowSize() + wSheet->MemoryCellSize() + wSheet->MemoryRangeSize() << endl;
		cout << "Nb Shared String " << tApplication::Instance()->NbSharedString() << endl;
		cout << "SkSharedFormulaContainer::Size() " << tSpreadSheetContainer::Instance()->NbSharedFormula() << endl;

		tCell* wCellResult = EnsureCell(sDynamicRow, sDynamicCol);
		cout << "   Result  " << wCellResult->StrRef() << "=" << wCellResult->FormulaStr() << ":" << wCellResult->Value() << endl;
	}


    tBool tUISpreadSheet::PostMessage(tString sMessage) {
        tStringStream wStream;
        wStream << sMessage;
#ifdef DebugInterface
        cout << "tUISpreadSheet::PostMessage(" << wStream.str() << ")"  << endl;
#endif
#ifdef __EMSCRIPTEN__
        sker_js_post_message(wStream.str().c_str());
#endif
        return(true);
    }
    
    tBool tUISpreadSheet::GetMessage(tString sMessage) {
        return(tInterfaceWeb::GetMessage(sMessage));
    }
    // Déclaration de la map statique en dehors de la méthode
    static std::map<tString, std::function<tString(tUISpreadSheet*, const Document&)>> wFunctionMap;

    tString tUISpreadSheet::_Call(tString sFunctionName, tString sJsonParams) {
#ifdef DebugInterface
    cout << "tUISpreadSheet::_Call(" << sFunctionName << ", " << sJsonParams << ")" << endl;
#endif

    Document wDocument;
    wDocument.Parse(sJsonParams.c_str());

    if (wDocument.HasParseError()) {
        tString errorStr = "{\"error\":\"Invalid JSON parameters\"}";
        cerr << "Error Parsing Json " << sFunctionName << ":" << sJsonParams << endl;
        ShowParseErrorJson(&wDocument,sJsonParams);
        return errorStr;
    }

    // Initialize the function map if it's empty
    if (wFunctionMap.empty()) {
        wFunctionMap = {
            // WorkBook operations
            {"GetActiveWorkBook", [](tUISpreadSheet* self, const Document&) -> tString {
                tString result = self->_GetActiveWorkBook();
                return "{\"result\":\"" + result + "\"}";
            }},
            {"SetActiveWorkBook", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString uri = params["uri"].GetString();
                tBool result = self->_SetActiveWorkBook(uri);
                return JsonBoolResult(result);
            }},
            // User Interface ====================================================
            {"SetUserInterface", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString json = params["json"].GetString();
                self->_SetUserInterface(json);
                return JsonStringResult("true");
            }},
            {"GetUserInterface", [](tUISpreadSheet* self, const Document&) -> tString {
                tString result = self->_GetUserInterface();
                return JsonStringResult(result);
            }},
            {"NewWorkBook", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString uri = params["uri"].GetString();
                tBool result = self->_NewWorkBook(uri);
                return JsonBoolResult(result);
            }},
            {"AddWorkBook", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString uri = params["uri"].GetString();
                tBool result = self->_AddWorkBook(uri);
                return JsonBoolResult(result);
            }},
            {"DeleteWorkBook", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString uri = params["uri"].GetString();
                tBool result = self->_DeleteWorkBook(uri);
                return JsonBoolResult(result);
            }},
            {"RenameWorkBook", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString uri = params["uri"].GetString();
                tString uriTo = params["uriTo"].GetString();
                tBool result = self->_RenameWorkBook(uri, uriTo);
                return JsonBoolResult(result);
            }},
            {"JsonWorkBook", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString uri = params["uri"].GetString();
                tString result = self->_JsonWorkBook(uri);
                return JsonObjectResult(result);
            }},
            {"JsonWorkBooks", [](tUISpreadSheet* self, const Document&) -> tString {
                tString result = self->_JsonWorkBooks();
                return JsonObjectResult(result);
            }},
            {"WriteJson", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString uri = params["uri"].GetString();
                tString result = self->_WriteJson(uri);
                // WriteJson already returns JSON object text — embed directly (not as escaped string).
                if (result.empty()) {
                    return "{\"result\":null}";
                }
                return tString("{\"result\":") + result + tString("}");
            }},
            {"ReadJson", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString json = params["json"].GetString();
                tBool result = self->_ReadJson(json);
                return JsonBoolResult(result);
            }},
            {"RecalculateAll", [](tUISpreadSheet* self, const Document&) -> tString {
                tBool result = self->_RecalculateAll();
                return JsonBoolResult(result);
            }},
            
            // Sheet operations
            {"SetActiveSheet", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tBool result = self->_SetActiveSheet(name);
                return JsonBoolResult(result);
            }},
            {"GetActiveSheet", [](tUISpreadSheet* self, const Document&) -> tString {
                tString result = self->_GetActiveSheet();
                return JsonStringResult(result);
            }},
            {"AddSheet", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString left = params.HasMember("left") ? params["left"].GetString() : "";
                tBool result = self->_AddSheet(name, left);
                return JsonBoolResult(result);
            }},
            {"RenameSheet", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString newName = params["newName"].GetString();
                tBool result = self->_RenameSheet(name, newName);
                return JsonBoolResult(result);
            }},
            {"SwapSheet", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name1 = params["name1"].GetString();
                tString name2 = params["name2"].GetString();
                tBool insertAfter =
                    params.HasMember("insertAfter") && params["insertAfter"].GetBool();
                tBool result = self->_SwapSheet(name1, name2, insertAfter);
                return JsonBoolResult(result);
            }},
            {"DeleteSheet", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tBool result = self->_DeleteSheet(name);
                return JsonBoolResult(result);
            }},
            {"SheetsList", [](tUISpreadSheet* self, const Document&) -> tString {
                tString result = self->_SheetsList();
                return JsonObjectResult(result);
            }},
            
            // Cell operations
            {"Value", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString value = params["value"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_Value(ref, value, sheet);
                return JsonBoolResult(result);
            }},
            {"ValueString", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString value = params["value"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_ValueString(ref, value, sheet);
                return JsonBoolResult(result);
            }},
            {"ValueInt", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tInt value = params["value"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_ValueInt(ref, value, sheet);
                return JsonBoolResult(result);
            }},
            {"ValueDouble", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tDouble value = params["value"].GetDouble();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_ValueDouble(ref, value, sheet);
                return JsonBoolResult(result);
            }},
            {"GetValue", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_GetValue(ref, sheet);
                return JsonStringResult(result);
            }},
            {"GetCalculableScalar", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_GetCalculableScalar(ref, sheet);
                return JsonStringResult(result);
            }},
            {"GetInputValue", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_GetInputValue(ref, sheet);
                return JsonStringResult(result);
            }},
            {"GetFormula", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool user = params.HasMember("user") && params["user"].IsBool() ? params["user"].GetBool() : false;
                tString result = self->_GetFormula(ref, sheet, user);
                return JsonStringResult(result);
            }},
            {"CellRef", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString anchor = params.HasMember("anchor") ? params["anchor"].GetString() : "";
                tString ref = params.HasMember("ref") ? params["ref"].GetString() : "";
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_CellRef(anchor, ref, sheet);
                return JsonStringResult(result);
            }},
            {"CompileError", [](tUISpreadSheet* self, const Document& params) -> tString {
                (void)params;
                return JsonObjectResult(self->_Error());
            }},
            {"CompileErrorLine", [](tUISpreadSheet* self, const Document& params) -> tString {
                (void)params;
                return JsonDoubleResult(std::to_string(self->_ErrorLine()));
            }},
            {"CompileErrorColumn", [](tUISpreadSheet* self, const Document& params) -> tString {
                (void)params;
                return JsonDoubleResult(std::to_string(self->_ErrorColumn()));
            }},
            {"CompileErrorWithDetail", [](tUISpreadSheet* self, const Document& params) -> tString {
                (void)params;
                return JsonObjectResult(self->_ErrorWithDetail());
            }},
            {"GetValueAttribute", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString name = params["name"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_GetValueAttribute(ref, name, sheet);
                return JsonStringResult(result);
            }},
            {"GetFormulaAttribute", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString name = params["name"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_GetFormulaAttribute(ref, name, sheet);
                return JsonStringResult(result);
            }},
            
            // Format operations
            {"Format", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString value = params["value"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_Format(ref, value, sheet);
                return JsonBoolResult(result);
            }},
            // Border operations
            {"Border", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tInt border = params["border"].GetInt();
                tString value = params["value"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_Border(ref, border, value, sheet);
                return JsonBoolResult(result);
            }},
            // Precision operations
            {"Precision", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tBool inc = false;
                if (params.HasMember("inc") && params["inc"].IsBool()) {
                    inc = params["inc"].GetBool();
                } else if (params.HasMember("precision") && params["precision"].IsBool()) {
                    inc = params["precision"].GetBool();
                }
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_Precision(ref, inc, sheet);
                return JsonBoolResult(result);
            }},
            {"ConditionalFormat", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString type = params["type"].GetString();
                tString ref = params["ref"].GetString();
                tString param1 = params["param1"].GetString();
                tString param2 = params["param2"].GetString();
                tString param3 = params["param3"].GetString();
                tString param4 = params["param4"].GetString();
                tString param5 = params["param5"].GetString();
                tString param6 = params["param6"].GetString();
                tString param7 = params["param7"].GetString();
                tString param8 = params["param8"].GetString();
                tString param9 = params["param9"].GetString();
                tString param10 = params["param10"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_ConditionalFormat(type,ref, param1, param2, param3, param4, param5, param6, param7, param8, param9, param10, sheet);
                return JsonBoolResult(result);
            }},
            {"DeleteConditionalFormat", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString type = params["type"].GetString();
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_DeleteConditionalFormat(type,ref, sheet);
                return JsonBoolResult(result);
            }},
            {"GetFormat", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_GetFormat(ref, sheet);
                return JsonStringResult(result);
            }},
            {"JsonFormatString", [](tUISpreadSheet* self, const Document&) -> tString {
                tString result = self->_JsonFormatString();
                return result;
            }},
            {"JsonConditionalFormat", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_JsonConditionalFormat(sheet);
                return JsonObjectResult(result);
            }},
            {"DefaultFormatString", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString formatString = params["formatString"].GetString();
                tString result = self->_DefaultFormatString(formatString);
                return JsonStringResult(result);
            }},
            {"FormatValueWithCellFormat", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString value = params["value"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_FormatValueWithCellFormat(ref, value, sheet);
                return JsonStringResult(result);
            }},
            {"ApplyFormatString", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString value = params["value"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_ApplyFormatString(ref, value, sheet);
                return JsonBoolResult(result);
            }},

            // Merge operations
            {"Merge", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_Merge(ref, sheet);
                return JsonBoolResult(result);
            }},
            {"ReturnMerged", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt row = params["row"].GetInt();
                tInt col = params["col"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_ReturnMerged(row, col, sheet);
                return JsonObjectResult(result);
            }},
            {"ReturnRangeMerged", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_ReturnRangeMerged(ref, sheet);
                return JsonObjectResult(result);
            }},
            {"ReturnRangeMergedFusion", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_ReturnRangeMergedFusion(ref, sheet);
                return JsonObjectResult(result);
            }},

            // Named Range operations
            {"InsertNamedRange", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_InsertNamedRange(name, ref, sheet);
                return JsonBoolResult(result);
            }},
            {"DeleteNamedRange", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_DeleteNamedRange(name, sheet);
                return JsonBoolResult(result);
            }},
            {"UpdateNamedRange", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString oldName = params["oldname"].GetString();
                tString newName = params["newname"].GetString();
                tString newRef = params["newref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_UpdateNamedRange(oldName, newName, newRef, sheet);
                return JsonBoolResult(result);
            }},

            // Rename gating (see tApi::IsRenameAllowed). The UI uses these to
            // disable "rename sheet" / "rename named range" buttons when
            // peers are connected, and to forward the multi-user state from
            // the WebSocket / presence layer.
            {"IsRenameAllowed", [](tUISpreadSheet* self, const Document&) -> tString {
                tBool result = self->_IsRenameAllowed();
                return JsonBoolResult(result);
            }},
            {"SetMultiUserActive", [](tUISpreadSheet* self, const Document& params) -> tString {
                tBool active = params["active"].GetBool();
                self->_SetMultiUserActive(active);
                return JsonBoolResult(true);
            }},

            // Named Formula operations
            {"InsertFormulaNamed", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString formula = params["formula"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_InsertFormulaNamed(name, formula, sheet);
                return JsonBoolResult(result);
            }},
            {"DeleteFormulaNamed", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_DeleteFormulaNamed(name, sheet);
                return JsonBoolResult(result);
            }},

            // Floating object operations
            {"InsertFloatingObject", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString className = params["className"].GetString();
                tString targetSheetName = params["targetSheetName"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tDouble diffX = params.HasMember("diffX") ? params["diffX"].GetDouble() : 0.0;
                tDouble diffY = params.HasMember("diffY") ? params["diffY"].GetDouble() : 0.0;
                tDouble width = params.HasMember("width") ? params["width"].GetDouble() : 0.0;
                tDouble height = params.HasMember("height") ? params["height"].GetDouble() : 0.0;
                tDouble opacity = params.HasMember("opacity") ? params["opacity"].GetDouble() : 0.0;
                tString anchorCellRef = params.HasMember("anchorCellRef") ? params["anchorCellRef"].GetString() : "";
                tBool result = self->_InsertFloatingObject(name, className, targetSheetName, sheet, diffX, diffY, width,
                                                           height, opacity, anchorCellRef);
                return JsonBoolResult(result);
            }},
            {"DeleteFloatingObject", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_DeleteFloatingObject(name, sheet);
                return JsonBoolResult(result);
            }},
            {"FloatingObjectLayout", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tDouble diffX = params["diffX"].GetDouble();
                tDouble diffY = params["diffY"].GetDouble();
                tDouble width = params["width"].GetDouble();
                tDouble height = params["height"].GetDouble();
                tDouble opacity = params["opacity"].GetDouble();
                tString anchorCellRef = params.HasMember("anchorCellRef") ? params["anchorCellRef"].GetString() : "";
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_FloatingObjectLayout(name, diffX, diffY, width, height, opacity, anchorCellRef, sheet);
                return JsonBoolResult(result);
            }},
            {"FloatingObjectBringToFront", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_FloatingObjectBringToFront(name, sheet);
                return JsonBoolResult(result);
            }},
            {"FloatingObjectAttribute", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString attribute = params["attribute"].GetString();
                tString value = params["value"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_FloatingObjectAttribute(name, attribute, value, sheet);
                return JsonBoolResult(result);
            }},
            {"FloatingObjectAttributes", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString attributesJson = params["attributesJson"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_FloatingObjectAttributes(name, attributesJson, sheet);
                return JsonBoolResult(result);
            }},

            // View operations
            {"JsonView", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt row = params["row"].GetInt();
                tInt col = params["col"].GetInt();
                tDouble viewHeight = params["viewHeight"].GetDouble();
                tDouble viewWidth = params["viewWidth"].GetDouble();
                tDouble diffY = params["diffY"].GetDouble();
                tDouble diffX = params["diffX"].GetDouble();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool sCss = params.HasMember("sCss") ? params["sCss"].GetBool() : false;
                tString result = self->_JsonView(row, col, viewHeight, viewWidth, diffY, diffX, sheet, sCss);
#ifdef DebugInterface
                /* Test in debug mode
                Document doc;
                tString wResult=JsonObjectResult(result);
                doc.Parse(wResult.c_str());
                
                if (doc.HasParseError()) {
                    ShowParseErrorJson(&doc, result);
                };
                */
#endif
                
                return(result);
            }},
            {"JsonFloatingObjectsForSheet", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString targetSheetName = params["targetSheetName"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_JsonFloatingObjectsForSheet(targetSheetName, sheet);
                return JsonObjectResult(result);
            }},
            {"JsonFloatingObjects", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_JsonFloatingObjects(sheet);
                return JsonObjectResult(result);
            }},
            {"JsonRightJustify", [](tUISpreadSheet* self, const Document& params) -> tString {
                tIndex col = params["col"].GetInt();
                tDouble viewWidth = params["viewWidth"].GetDouble();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_JsonRightJustify(col, viewWidth, sheet);
                return JsonObjectResult(result);
            }},
            {"JsonBottomJustify", [](tUISpreadSheet* self, const Document& params) -> tString {
                tIndex row = params["row"].GetInt();
                tDouble viewHeight = params["viewHeight"].GetDouble();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_JsonBottomJustify(row, viewHeight, sheet);
                return JsonObjectResult(result);
            }},
            {"JsonColByPixel", [](tUISpreadSheet* self, const Document& params) -> tString {
                tIndex colStart = params["colStart"].GetInt();
                tDouble pixel = params["pixel"].GetDouble();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_JsonColByPixel(colStart, pixel, sheet);
                return JsonObjectResult(result);
            }},
            {"JsonRowByPixel", [](tUISpreadSheet* self, const Document& params) -> tString {
                tIndex rowStart = params["rowStart"].GetInt();
                tDouble pixel = params["pixel"].GetDouble();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_JsonRowByPixel(rowStart, pixel, sheet);
                return JsonObjectResult(result);
            }},
            {"JsonBottomRight", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_JsonBottomRight(sheet);
                return JsonObjectResult(result);
            }},
            {"JsonPixelBottomRight", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_JsonPixelBottomRight(sheet);
                return JsonObjectResult(result);
            }},

            // Tree operations
            {"OpenCloseTreeRow", [](tUISpreadSheet* self, const Document& params) -> tString {
                tIndex row = params["row"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_OpenCloseTreeRow(row, sheet);
                return JsonBoolResult(result);
            }},
            {"OpenCloseTreeCol", [](tUISpreadSheet* self, const Document& params) -> tString {
                tIndex row = params["row"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_OpenCloseTreeCol(row, sheet);
                return JsonBoolResult(result);
            }},
            {"ChangeTreeRow", [](tUISpreadSheet* self, const Document& params) -> tString {
                tBool right = params["right"].GetBool();
                tIndex row = params["row"].GetInt();
                tIndex size = params["size"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_ChangeTreeRow(right, row, size, sheet);
                return JsonBoolResult(result);
            }},
            {"ChangeTreeCol", [](tUISpreadSheet* self, const Document& params) -> tString {
                tBool right = params["right"].GetBool();
                tIndex col = params["col"].GetInt();
                tIndex size = params["size"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_ChangeTreeCol(right, col, size, sheet);
                return JsonBoolResult(result);
            }},
            {"SplitView", [](tUISpreadSheet* self, const Document& params) -> tString {
                tByte cde = 0;
                const auto& jc = params["cde"];
                if (jc.IsUint()) {
                    cde = static_cast<tByte>(jc.GetUint());
                } else if (jc.IsInt() && (jc.GetInt() >= 0)) {
                    cde = static_cast<tByte>(jc.GetInt());
                }
                tIndex position = 0;
                if (params.HasMember("position") && params["position"].IsInt()) {
                    position = params["position"].GetInt();
                }
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_SplitView(cde, position, sheet);
                return JsonBoolResult(result);
            }},
            {"SplitFreezeCol", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                const tIndex result = self->_SplitFreezeCol(sheet);
                return tString("{\"result\":") + std::to_string(static_cast<long long>(result)) + tString("}");
            }},
            {"SplitFreezeRow", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                const tIndex result = self->_SplitFreezeRow(sheet);
                return tString("{\"result\":") + std::to_string(static_cast<long long>(result)) + tString("}");
            }},

            // Class operations
            {"RegisterClassAttribute", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString className = params["className"].GetString();
                tString label = params["label"].GetString();
                tString family = params["family"].GetString();
                tBool result = self->_RegisterClassAttribute(className, label, family);
                return JsonBoolResult(result);
            }},
            {"AddProperty", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString type = params["type"].GetString();
                tString label = params["label"].GetString();
                tSize order = params["order"].GetInt();
                tString defaultValue = params["defaultValue"].GetString();
                tString kind = params.HasMember("kind") && params["kind"].IsString()
                    ? params["kind"].GetString()
                    : tString("");
                tBool result = self->_AddProperty(name, type, label, order, defaultValue, kind);
                return JsonBoolResult(result);
            }},
            {"CellClass", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString className = params["className"].GetString();
                tBool result = self->_CellClass(ref, className);
                return JsonBoolResult(result);
            }},
            {"JsonCellClass", [](tUISpreadSheet* self, const Document&) -> tString {
                tString result = self->_JsonCellClass();
                return JsonStringResult(result);
            }},
            {"JsonCellClassByName", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString className = params["className"].GetString();
                tString result = self->_JsonCellClassByName(className);
                return JsonStringResult(result);
            }},
            {"ApplyUnit", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString family = params["family"].GetString();
                tString unit = params["unit"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_ApplyUnit(ref, family, unit, sheet);
                return JsonBoolResult(result);
            }},

            // Utility operations
            {"Base10toAlpha", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt value = params["value"].GetInt();
                tString result = self->_Base10toAlpha(value);
                return JsonStringResult(result);
            }},
            {"AlphaToBase10", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString value = params["value"].GetString();
                tInt result = self->_AlphaToBase10(value);
                return JsonStringResult(std::to_string(result));
            }},
            {"ParseCell", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                return JsonObjectResult(self->_ParseCell(ref));
            }},
            {"ParseRange", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                return JsonObjectResult(self->_ParseRange(ref));
            }},
            {"QualifyRefsForSheet", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString sheet = params["sheet"].GetString();
                tString text = params["text"].GetString();
                tString result = self->_QualifyRefsForSheet(sheet, text);
                return JsonStringResult(result);
            }},
            {"StripTargetSheetFromRefs", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString sheet = params["sheet"].GetString();
                tString text = params["text"].GetString();
                tString result = self->_StripTargetSheetFromRefs(sheet, text);
                return JsonStringResult(result);
            }},
            {"CollectFormulaRefs", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString formula = params["formula"].GetString();
                tString sheet = params["sheet"].GetString();
                tIndex row = params["row"].GetInt();
                tIndex col = params["col"].GetInt();
                tString result = self->_CollectFormulaRefs(formula, sheet, row, col);
                return JsonObjectResult(result);
            }},
            {"MaxCol", [](tUISpreadSheet* self, const Document&) -> tString {
                tIndex result = self->_MaxCol();
                return JsonStringResult(std::to_string(result));
            }},
            {"MaxRow", [](tUISpreadSheet* self, const Document&) -> tString {
                tIndex result = self->_MaxRow();
                return JsonStringResult(std::to_string(result));
            }},
            {"EnsureCell", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_EnsureCell(ref, sheet);
                return JsonBoolResult(result);
            }},
            {"SetLang", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString lang = params["lang"].GetString();
                self->_SetLang(lang);
                return JsonBoolResult(true);
            }},
            {"AddFunction", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString label = params["label"].GetString();
                tString family = params["family"].GetString();
                tInt nbArg = params["nbArg"].GetInt();
                tBool result = self->_AddFunction(name, label, family, nbArg);
                return JsonBoolResult(result);
            }},
            {"Undo", [](tUISpreadSheet* self, const Document&) -> tString {
                self->_Undo();
                return JsonStringResult("true");
            }},
            {"Redo", [](tUISpreadSheet* self, const Document&) -> tString {
                self->_Redo();
                return JsonStringResult("true");
            }},
            {"IsUndoActif", [](tUISpreadSheet* self, const Document&) -> tString {
                return JsonBoolResult(self->_IsUndoActif());
            }},
            {"SetIsUndoActif", [](tUISpreadSheet* self, const Document& params) -> tString {
                tBool active = params.HasMember("active") && params["active"].GetBool();
                self->_SetIsUndoActif(active);
                return JsonBoolResult(true);
            }},
            {"SetExtraUndo", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString json = params["json"].GetString();
                self->_SetExtraUndo(json);
                return JsonStringResult("true");
            }},
            {"GetExtraUndo", [](tUISpreadSheet* self, const Document&) -> tString {
                tString result = self->_GetExtraUndo();
                return JsonStringResult(result);
            }},
            {"DebugFormat", [](tUISpreadSheet* self, const Document&) -> tString {
                tString wDebug = self->_DEBUGSKFormat();
                return JsonStringResult(StringForJson(wDebug));
            }},
            {"Pressure", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt dynamicRow = params["dynamicRow"].GetInt();
                tInt dynamicCol = params["dynamicCol"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                self->_Pressure(dynamicRow, dynamicCol, sheet);
                return JsonStringResult("true");
            }},
            {"MoveCell", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt row = params["row"].GetInt();
                tInt col = params["col"].GetInt();
                tByte key = params["key"].GetInt();
                tByte meta = params["meta"].GetInt();
                tInt top = params["top"].GetInt();
                tInt left = params["left"].GetInt();
                tInt bottom = params["bottom"].GetInt();
                tInt right = params["right"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_MoveCell(row, col, key, meta, top, left, bottom, right, sheet);
                return JsonObjectResult(result);
            }},
            {"MoveToCell", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt row = params["row"].GetInt();
                tInt col = params["col"].GetInt();
                tInt direction = params["direction"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tString result = self->_MoveToCell(row, col, direction, sheet);
                return JsonObjectResult(result);
            }},
            {"SumPixelHeight", [](tUISpreadSheet* self, const Document& params) -> tString {
                tIndex rowStart = params["rowStart"].GetInt();
                tIndex rowEnd = params["rowEnd"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tDouble result = self->_SumPixelHeight(rowStart, rowEnd, sheet);
                return JsonDoubleResult(std::to_string(result));
            }},
            {"SumPixelWidth", [](tUISpreadSheet* self, const Document& params) -> tString {
                tIndex colStart = params["colStart"].GetInt();
                tIndex colEnd = params["colEnd"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tDouble result = self->_SumPixelWidth(colStart, colEnd, sheet);
                return JsonDoubleResult(std::to_string(result));
            }},
            {"GetSizeRow", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt index = params["index"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tDouble result = self->_GetSizeRow(index, sheet);
                return JsonDoubleResult(std::to_string(result));
            }},
            {"GetSizeCol", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt index = params["index"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tDouble result = self->_GetSizeCol(index, sheet);
                return JsonDoubleResult(std::to_string(result));
            }},
            {"ValueAttribute", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString name = params["name"].GetString();
                tString value = params["value"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_ValueAttribute(ref, name, value, sheet);
                return JsonBoolResult(result);
            }},
            {"CellClassAttributes", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString attributesJson = params["attributesJson"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_CellClassAttributes(ref, attributesJson, sheet);
                return JsonBoolResult(result);
            }},
            {"ValueClassCalculable", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString value = params["value"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_ValueClassCalculable(ref, value, sheet);
                return JsonBoolResult(result);
            }},
            {"SizeRow", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt begin = params["begin"].GetInt();
                tInt end = params["end"].GetInt();
                tDouble size = params["size"].GetDouble();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_SizeRow(begin, end, size, sheet);
                return JsonBoolResult(result);
            }},
            {"SizeCol", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt begin = params["begin"].GetInt();
                tInt end = params["end"].GetInt();
                tDouble size = params["size"].GetDouble();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_SizeCol(begin, end, size, sheet);
                return JsonBoolResult(result);
            }},
            {"Pressure", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt dynamicRow = params["dynamicRow"].GetInt();
                tInt dynamicCol = params["dynamicCol"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                self->_Pressure(dynamicRow, dynamicCol, sheet);
                return JsonStringResult("true");
            }},
            {"Raz", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tBool keepFormat = params.HasMember("keepFormat") ? params["keepFormat"].GetBool() : false;
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_Raz(ref, keepFormat, sheet);
                return JsonBoolResult(result);
            }},
            {"RazFormat", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_RazFormat(ref, sheet);
                return JsonBoolResult(result);
            }},
            {"DeleteCol", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt begin = params["begin"].GetInt();
                tInt end = params["end"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_DeleteCol(begin, end, sheet);
                return JsonBoolResult(result);
            }},
            {"DeleteColByRect", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_DeleteColByRect(ref, sheet);
                return JsonBoolResult(result);
            }},
            {"DeleteRow", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt begin = params["begin"].GetInt();
                tInt end = params["end"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_DeleteRow(begin, end, sheet);
                return JsonBoolResult(result);
            }},
            {"DeleteRowByRect", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_DeleteRowByRect(ref, sheet);
                return JsonBoolResult(result);
            }},
            {"InsertCol", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt begin = params["begin"].GetInt();
                tInt end = params["end"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_InsertCol(begin, end, sheet);
                return JsonBoolResult(result);
            }},
            {"InsertRow", [](tUISpreadSheet* self, const Document& params) -> tString {
                tInt begin = params["begin"].GetInt();
                tInt end = params["end"].GetInt();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_InsertRow(begin, end, sheet);
                return JsonBoolResult(result);
            }},
            {"InsertRowByRect", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_InsertRowByRect(ref, sheet);
                return JsonBoolResult(result);
            }},
            {"InsertRowByRectWithLabel", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString labelRef = params["labelRef"].GetString();
                tString labelValue = params.HasMember("labelValue")
                    ? params["labelValue"].GetString()
                    : "Total";
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_InsertRowByRectWithLabel(ref, labelRef, labelValue, sheet);
                return JsonBoolResult(result);
            }},
            {"InsertColByRect", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_InsertColByRect(ref, sheet);
                return JsonBoolResult(result);
            }},
            
            // Copy Paste operations
            {"Copy", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_Copy(ref, sheet);
                return JsonBoolResult(result);
            }},
            {"Cut", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_Cut(ref, sheet);
                return JsonBoolResult(result);
            }},
            {"Paste", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_Paste(ref, sheet);
                return JsonBoolResult(result);
            }},
            {"Move", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString sourceRef = params["sourceRef"].GetString();
                tString destRef = params["destRef"].GetString();
                tString sheet = params.HasMember("sheet") ? params["sheet"].GetString() : "";
                tBool result = self->_Move(sourceRef, destRef, sheet);
                return JsonBoolResult(result);
            }},
            
            // Additional missing functions
            {"JsonRangeNamed", [](tUISpreadSheet* self, const Document&) -> tString {
                tString result = self->JsonRangeNamed();
                return JsonObjectResult(result);
            }},
            {"JsonRangeData", [](tUISpreadSheet* self, const Document&) -> tString {
                tString result = self->_JsonRangeData();
                return JsonObjectResult(result);
            }},
            {"JsonFindUniqueValue", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString ref = params["ref"].GetString();
                tString sheet = params.HasMember("sheet") && params["sheet"].IsString()
                                    ? params["sheet"].GetString()
                                    : "";
                tString result = self->_JsonFindUniqueValue(ref, sheet);
                return JsonObjectResult(result);
            }},
            {"JsonFindCell", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString search = params["search"].GetString();
                tBool wMatchCase = params.HasMember("matchCase") && params["matchCase"].IsBool()
                                       ? params["matchCase"].GetBool()
                                       : false;
                tBool wMatchEntireCell =
                    params.HasMember("matchEntireCell") && params["matchEntireCell"].IsBool()
                        ? params["matchEntireCell"].GetBool()
                        : false;
                tString sheet = params.HasMember("sheet") && params["sheet"].IsString()
                                    ? params["sheet"].GetString()
                                    : "";
                tString result = self->_JsonFindCell(search, wMatchCase, wMatchEntireCell, sheet);
                return JsonObjectResult(result);
            }},
            {"UndoApplyRangeData", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString jsonData = params["jsonData"].GetString();
                tString sheet = params.HasMember("sheet") && params["sheet"].IsString()
                                    ? params["sheet"].GetString()
                                    : "";
                tBool ok = self->_UndoApplyRangeData(name, jsonData, sheet);
                return JsonBoolResult(ok);
            }},
            {"UndoAddRangeData", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString name = params["name"].GetString();
                tString ref = params["ref"].GetString();
                tString jsonData = params.HasMember("jsonData") && params["jsonData"].IsString()
                                       ? params["jsonData"].GetString()
                                       : "";
                tString sheet = params.HasMember("sheet") && params["sheet"].IsString()
                                    ? params["sheet"].GetString()
                                    : "";
                tBool ok = self->_UndoAddRangeData(name, ref, jsonData, sheet);
                return JsonBoolResult(ok);
            }},
            {"JsonPrintParameters", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString sheet = params.HasMember("sheet") && params["sheet"].IsString() ? params["sheet"].GetString() : "";
                tString result = self->_JsonPrintParameters(sheet);
                return JsonObjectResult(result);
            }},
            {"SetJsonPrintParameters", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString json = params["json"].GetString();
                tString sheet = params.HasMember("sheet") && params["sheet"].IsString() ? params["sheet"].GetString() : "";
                tBool ok = self->_SetJsonPrintParameters(json, sheet);
                return JsonBoolResult(ok);
            }},
            {"GetMessage", [](tUISpreadSheet* self, const Document& params) -> tString {
                tString message = params["message"].GetString();
                tBool result = self->GetMessage(message);
                return JsonBoolResult(result);
            }},
            {"GetClipboard", [](tUISpreadSheet* self, const Document& params) -> tString {
                (void)self;
                (void)params;
                tString text = tApplication::Instance()->Clipboard()->Text();
                return JsonStringResult(text);
            }},
            {"SetClipboard", [](tUISpreadSheet* self, const Document& params) -> tString {
                (void)self;
                tString text = params["text"].GetString();
                tApplication::Instance()->Clipboard()->Text(text);
                return JsonBoolResult(true);
            }},
        };
    }

    auto it = wFunctionMap.find(sFunctionName);
    if (it == wFunctionMap.end()) {
        tString errorStr = "{\"error\":\"Unknown function\"}";
        return errorStr;
    }

    try {
        tString result = it->second(this, wDocument);
        return result;
    } catch (const std::exception& e) {
        tString errorStr = "{\"error\":\"Exception occurred\"}";
        return errorStr;
    }
}

}; // end of namespace

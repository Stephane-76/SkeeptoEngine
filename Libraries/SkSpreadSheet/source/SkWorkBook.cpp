//=============================================================================
// SkSpreadSheet WorkBook
//=============================================================================
#include "../include/SkWorkBook.hpp"
#include "../include/SkSpreadSheet.hpp"
#include "../include/SkUndoRedoSp.hpp"
#include "../include/SkTools.hpp"
#include "../include/SkCell.hpp"
#include "../include/SkRangeNamed.hpp"
#include "../include/SkRangeData.hpp"
#include "../include/SkCellClassAttribute.hpp"
#include "../include/SkCellClass.hpp"
#include "../include/SkCalculationPath.hpp"
#include <SkLocale.hpp>
#include <set>
#define _debugformat
#define _debugsheet
#define _debugrangenamed
#define _debugcalculate
#define _debugspill
#define debugprune

namespace SkSpreadSheet {

    static void PushJsonViewFormatLayer(tVectorFormatRef& sVector, tFormatRef sRef) {
        if (sRef != 0) {
            sVector.push_back(sRef);
        }
    }

    /** True when the cell layer itself sets left/center/right/justify. */
    static tBool FormatHasExplicitHAlign(tWorkBook* sBook, tFormatRef sRef) {
        if (sBook == nullptr || sRef == 0) {
            return false;
        }
        const tString wCss = sBook->CellFormat(sRef);
        return wCss.find("text-align:left") != tString::npos
            || wCss.find("text-align:center") != tString::npos
            || wCss.find("text-align:right") != tString::npos
            || wCss.find("text-align:justify") != tString::npos;
    }

    /**
     * Sheet → col → row → table → cell → CF.
     * sGeneralHAlign (text-align:none) sits before the cell so Excel General
     * does not inherit column/row text-align (Amort.xlsx C3 vs column C center).
     */
    static void AppendJsonViewFormatLayers(
        tVectorFormatRef& sVector,
        tFormatRef sSheet,
        tFormatRef sCol,
        tFormatRef sRow,
        tFormatRef sTable,
        tFormatRef sTable2,
        tFormatRef sTable3,
        tFormatRef sGeneralHAlign,
        tFormatRef sCell,
        tFormatRef sItemCf) {
        PushJsonViewFormatLayer(sVector, sSheet);
        PushJsonViewFormatLayer(sVector, sCol);
        PushJsonViewFormatLayer(sVector, sRow);
        PushJsonViewFormatLayer(sVector, sTable);
        PushJsonViewFormatLayer(sVector, sTable2);
        PushJsonViewFormatLayer(sVector, sTable3);
        PushJsonViewFormatLayer(sVector, sGeneralHAlign);
        PushJsonViewFormatLayer(sVector, sCell);
        PushJsonViewFormatLayer(sVector, sItemCf);
    }

    static void AppendJsonViewFormatLayers(
        tVectorFormatRef& sVector,
        tFormatRef sSheet,
        tFormatRef sCol,
        tFormatRef sRow,
        const tVectorFormatRef& sTableLayers,
        tFormatRef sGeneralHAlign,
        tFormatRef sCell,
        tFormatRef sItemCf) {
        const tFormatRef wTable = sTableLayers.empty() ? 0 : sTableLayers[0];
        const tFormatRef wTable2 = sTableLayers.size() > 1 ? sTableLayers[1] : 0;
        const tFormatRef wTable3 = sTableLayers.size() > 2 ? sTableLayers[2] : 0;
        AppendJsonViewFormatLayers(
            sVector, sSheet, sCol, sRow, wTable, wTable2, wTable3,
            sGeneralHAlign, sCell, sItemCf);
    }

    /** f_value must use merged sheet/col/row/cell formats (currency, decimals, …).
     *  Table overlays sit under direct cell formatting so user fills stay visible
     *  (Excel: cell xfs beats ListObject banding). CF stays last. */
    static void EmitJsonViewVariantWithLayers(
        tFormatApi* sApi,
        tFormatRef sSheet,
        tFormatRef sCol,
        tFormatRef sRow,
        const tVectorFormatRef& sTableLayers,
        tFormatRef sGeneralHAlign,
        tFormatRef sCell,
        tFormatRef sItemCf,
        tVariant* sVariant,
        tBool sCss,
        Writer<StringBuffer>* sWriter) {
        if (sApi == nullptr) {
            return;
        }
        tVectorFormatRef wVector;
        AppendJsonViewFormatLayers(
            wVector, sSheet, sCol, sRow, sTableLayers, sGeneralHAlign, sCell, sItemCf);
        if (wVector.empty()) {
            sApi->_JsonJavaScriptVariantDisplay(sVariant, sCss, sWriter);
        } else {
            sApi->_JsonJavaScriptMergedVariantDisplay(&wVector, sVariant, sCss, sWriter);
        }
    }

    class tCallBackResetCellPath : public tSparseArrayCallBack<tAllocatorRef> {
        tColRowCellRange* m_ColRowCellRange;
    public:
        explicit tCallBackResetCellPath(tColRowCellRange* sColRowCellRange)
            : tSparseArrayCallBack<tAllocatorRef>(), m_ColRowCellRange(sColRowCellRange) {}
        tBool CallBack(tAllocatorRef sAllocatorRef) override {
            tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
            if (wCell != nullptr && wCell->Path() != 0) {
                wCell->Path(0);
            }
            return(true);
        }
    };

    void ResetAllCellPathsOnSheet(SkSpreadSheet::tSheet* sSheet) {
        using namespace SkSpreadSheet;
        if (sSheet == nullptr) {
            return;
        }
        tColRowCellRange* wColRowCellRange = sSheet->ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return;
        }
        tCallBackResetCellPath wCallBack(wColRowCellRange);
        wColRowCellRange->CallBackAllCell(&wCallBack);
    }

    class tCallBackCollectCellClassNames : public tSparseArrayCallBack<tAllocatorRef> {
        tColRowCellRange* m_ColRowCellRange;
        std::set<tString>* m_ClassNames;
    public:
        tCallBackCollectCellClassNames(tColRowCellRange* sColRowCellRange, std::set<tString>* sClassNames)
            : tSparseArrayCallBack<tAllocatorRef>(), m_ColRowCellRange(sColRowCellRange), m_ClassNames(sClassNames) {}

        tBool CallBack(tAllocatorRef sAllocatorRef) override {
            tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
            if (wCell == nullptr) {
                return true;
            }
            tCellClassAttribute* wAttr = wCell->ClassAttribute();
            if (wAttr == nullptr) {
                return true;
            }
            tString wClassName = wAttr->ClassName();
            if (wClassName.empty() && wAttr->ModelClass() != nullptr) {
                wClassName = wAttr->ModelClass()->ClassName();
            }
            if (!wClassName.empty()) {
                m_ClassNames->insert(wClassName);
            }
            return true;
        }
    };

    static void CollectUsedCellClassNames(tWorkBook* sWorkBook, std::set<tString>& oClassNames) {
        if (sWorkBook == nullptr) {
            return;
        }
        tVectorAllocatorRef* wVectorSheet = sWorkBook->VectorSheet();
        if (wVectorSheet != nullptr) {
            for (auto wSheetRef : *wVectorSheet) {
                tSheet* wSheet = sWorkBook->SheetByAllocator(wSheetRef);
                if (wSheet == nullptr) {
                    continue;
                }
                tCallBackCollectCellClassNames wCallBack(wSheet->ColRowCellRange(), &oClassNames);
                wSheet->ColRowCellRange()->CallBackAllCell(&wCallBack);
            }
        }
        if (tSheet* wAnchor = sWorkBook->SheetClassAnchor(); wAnchor != nullptr) {
            tCallBackCollectCellClassNames wCallBack(wAnchor->ColRowCellRange(), &oClassNames);
            wAnchor->ColRowCellRange()->CallBackAllCell(&wCallBack);
        }
    }

    static void JsonWorkBookCellClassModels(Writer<StringBuffer>* sWriter, const std::set<tString>& sClassNames) {
        tClassFactory* wFactory = tClassFactory::Instance();
        sWriter->Key(kJsonKeyModels);
        sWriter->StartArray();
        for (const tString& wClassName : sClassNames) {
            tModelClass* wModel = wFactory->Get(wClassName);
            if (wModel == nullptr) {
                continue;
            }
            tCellModelClass* wCellModel = dynamic_cast<tCellModelClass*>(wModel);
            if (wCellModel != nullptr) {
                wCellModel->Json(sWriter);
            } else {
                wModel->Json(sWriter);
            }
        }
        sWriter->EndArray();
    }
} // namespace

namespace SkSpreadSheet {

	const tDouble StaticCellHeight = 20; // Pixels;
	const tDouble StaticCellWidth = 64;

    // Identification ==============================================================
	tWorkBookInfo::tWorkBookInfo() : tClass() ,
		m_AuthorName(),
		m_AuthorEmail(),
		m_DateCreation(tClassDate::Now()),
		m_DateModification(tClassDate::Now()),
		m_VersionMajor(1),
		m_VersionMinor(0),
		m_VersionPatch(0),
		m_Comment() {
        }

	tWorkBookInfo::~tWorkBookInfo() {
	}

	void tWorkBookInfo::Json(Writer<StringBuffer>* sWriter) {
		sWriter->Key("author"); sWriter->String(m_AuthorName.c_str());
		sWriter->Key("email"); sWriter->String(m_AuthorEmail.c_str());
		sWriter->Key("date-creation"); sWriter->String(tClassDate(m_DateCreation).UsDate().c_str());
		sWriter->Key("date-modification"); sWriter->String(tClassDate(m_DateModification).UsDate().c_str());
		sWriter->Key("v-major"); sWriter->Int(m_VersionMajor);
		sWriter->Key("v-minor"); sWriter->Int(m_VersionMinor);
		sWriter->Key("v-patch"); sWriter->Int(m_VersionPatch);
		sWriter->Key("comment"); sWriter->String(m_Comment.c_str());
 	}

	void tWorkBookInfo::Json(const Value& sValue) {
        if (sValue.HasMember("author")) {
            m_AuthorName = sValue["author"].GetString();
        }
        if (sValue.HasMember("email")) {
            m_AuthorEmail = sValue["email"].GetString();
        }

        if (sValue.HasMember("date-creation") && sValue["date-creation"].IsString()) {
            tString wDateStr = sValue["date-creation"].GetString();
            if (!wDateStr.empty()) {
                tClassDate wClassDate;
                wClassDate.UsDate(wDateStr);
                // Verify that the date was parsed correctly
                tString wParsedDate = wClassDate.UsDate();
                if (!wParsedDate.empty() && wParsedDate != "00-00-0000" && wParsedDate.find("-") != tString::npos) {
                    // Extract date part (without time) for comparison
                    tString wParsedDateOnly = wParsedDate.substr(0, wParsedDate.find(" "));
                    tString wInputDateOnly = wDateStr.substr(0, wDateStr.find(" "));
                    // Accept if dates match (with or without time component)
                    if (wParsedDateOnly == wInputDateOnly || wParsedDate == wDateStr) {
                        // Check if parsing failed: if input was NOT "01-01-1900" but parsed result IS "01-01-1900"
                        // This indicates parsing failure and we should use Now() instead
                        if (wInputDateOnly != "01-01-1900" && wParsedDateOnly == "01-01-1900") {
                            // Parsing failed (got default value), use current date
                            cerr << "Warning: Date parsing failed - input: '" << wDateStr << "' -> parsed as default: '" << wParsedDate << "', using Now()" << endl;
                            m_DateCreation = tClassDate::Now();
                        } else {
                            // Valid date (either matches input, or input was intentionally "01-01-1900")
                            m_DateCreation = wClassDate.Value();
                        }
                    } else {
                        // Parsing mismatch, use current date
                        cerr << "Warning: Date parsing mismatch - input: '" << wDateStr << "' -> parsed as: '" << wParsedDate << "', using Now()" << endl;
                        m_DateCreation = tClassDate::Now();
                    }
                } else {
                    // Invalid date format, use current date
                    cerr << "Warning: Failed to parse date-creation: '" << wDateStr << "' -> parsed as: '" << wParsedDate << "', using Now()" << endl;
                    m_DateCreation = tClassDate::Now();
                }
            }
        }
        if (sValue.HasMember("date-modification") && sValue["date-modification"].IsString()) {
            tString wDateStr = sValue["date-modification"].GetString();
            if (!wDateStr.empty()) {
                tClassDate wClassDate;
                wClassDate.UsDate(wDateStr);
                // Verify that the date was parsed correctly
                tString wParsedDate = wClassDate.UsDate();
                if (!wParsedDate.empty() && wParsedDate != "00-00-0000" && wParsedDate.find("-") != tString::npos) {
                    // Extract date part (without time) for comparison
                    tString wParsedDateOnly = wParsedDate.substr(0, wParsedDate.find(" "));
                    tString wInputDateOnly = wDateStr.substr(0, wDateStr.find(" "));
                    // Accept if dates match (with or without time component)
                    if (wParsedDateOnly == wInputDateOnly || wParsedDate == wDateStr) {
                        // Check if parsing failed: if input was NOT "01-01-1900" but parsed result IS "01-01-1900"
                        // This indicates parsing failure and we should use Now() instead
                        if (wInputDateOnly != "01-01-1900" && wParsedDateOnly == "01-01-1900") {
                            // Parsing failed (got default value), use current date
                            cerr << "Warning: Date parsing failed - input: '" << wDateStr << "' -> parsed as default: '" << wParsedDate << "', using Now()" << endl;
                            m_DateModification = tClassDate::Now();
                        } else {
                            // Valid date (either matches input, or input was intentionally "01-01-1900")
                            m_DateModification = wClassDate.Value();
                        }
                    } else {
                        // Parsing mismatch, use current date
                        cerr << "Warning: Date parsing mismatch - input: '" << wDateStr << "' -> parsed as: '" << wParsedDate << "', using Now()" << endl;
                        m_DateModification = tClassDate::Now();
                    }
                } else {
                    // Invalid date format, use current date
                    cerr << "Warning: Failed to parse date-modification: '" << wDateStr << "' -> parsed as: '" << wParsedDate << "', using Now()" << endl;
                    m_DateModification = tClassDate::Now();
                }
            }
        }

        if (sValue.HasMember("v-major")) {
            m_VersionMajor = sValue["v-major"].GetInt();
        }
        if (sValue.HasMember("v-minor")) {
            m_VersionMinor = sValue["v-minor"].GetInt();
        }
        if (sValue.HasMember("v-patch")) {
            m_VersionPatch = sValue["v-patch"].GetInt();
        }
        if (sValue.HasMember("comment")) {
            m_Comment = sValue["comment"].GetString();
        }
	}

    // WorkBook ================================================================
	tWorkBook::tWorkBook() : SkSpAncestor() ,
		//! Id of allocator
		m_AllocatorRef(0),
        m_WorkBookInfo(),
		m_ActiveSheet(0),
		m_Uri(),
		m_DefaultSizeCol(SkMetrics::Convert(StaticCellWidth,tUnitMetrics::pixels,tUnitMetrics::millimeters)),
		m_DefaultSizeRow(SkMetrics::Convert(StaticCellHeight, tUnitMetrics::pixels, tUnitMetrics::millimeters)),
		m_DefaultFontName("Calibri"),
		m_DefaultFontSize(11.0),
		m_UndoRebaseLog(),
        m_SheetNamedFormulaRef(0),
        m_SheetClassAnchor(0),
        m_TableStyleContainer(this),
        m_JsonViewFormatTableActive(false),
        m_JsonViewGeneralHAlignRef(0),
        m_CooperativeRecalcPath(nullptr),
        m_CooperativeRecalcActive(false),
        m_CooperativeRecalcTotal(0),
        m_CooperativeCalculateEnabled(false)
    {
    m_RangeNamedContainer.Set(this);
    m_FloatingObjectContainer.Set(this);
    m_LemonInterface = nullptr;
}

	tWorkBook::~tWorkBook() {
		Clear();
	};

	// Rebase ===================================================================
	tUndoRebaseLog& tWorkBook::UndoRebaseLog() {
		return(m_UndoRebaseLog);
	}

    void tWorkBook::ClearUndoRedo() {
        tApplication::Instance()->ClearUndoRedo();
        m_UndoRebaseLog.Clear();
    }
    
		void tWorkBook::Clear() {
        CancelRecalculateAllCooperative();
        // Release table-style pool refs before sheet teardown (overlays are not on cells).
        // Order matters: Clear() → ReleaseFormats so Real can drop to 0 before cell DecCell.
        m_TableStyleContainer.Clear();
        if (m_JsonViewGeneralHAlignRef != 0) {
            DeleteCellFormat(m_JsonViewGeneralHAlignRef);
            m_JsonViewGeneralHAlignRef = 0;
        }
        // When system Delete ALL another Sheet must be null
        // m_VectorSheet.clear() do that;
        tVectorAllocatorRef wVectorSheet=m_VectorSheet;
        m_VectorSheet.clear();
        
        // Clear All Sheet (ReleaseAllCellFormats inside sheet Clear)
        for (auto wSheetRef : wVectorSheet) {
            m_SheetAllocator(wSheetRef)->Clear();
		}
        // Clear Sheet named formula
        if (m_SheetNamedFormulaRef!=0) {
            m_SheetAllocator(m_SheetNamedFormulaRef)->Clear();
        }
        m_SheetNamedFormulaRef=0;
        // Clear Named Range
        m_RangeNamedContainer.Clear();
        m_FloatingObjectContainer.Clear();
        
        // Clear Sheet Class Anchor
        if (m_SheetClassAnchor!=0) {
            m_SheetAllocator(m_SheetClassAnchor)->Clear();
        }
        m_SheetClassAnchor=0;
        m_ActiveSheet = 0;
        m_OffListSheets.clear();
        m_UndoRebaseLog.Clear();
        m_PrintParameters = tPrintParameters();
	}

	void tWorkBook::Set(tAllocatorRef sAllocatorRef, tString sUri) {
		AllocatorRef(sAllocatorRef);
		m_Uri = sUri;
	}

	void tWorkBook::WorkBookInfo(tString sJson) {
        Document wDocument;
        wDocument.Parse(sJson.c_str());
        m_WorkBookInfo.Json(wDocument);
	}

    tString tWorkBook::WorkBookInfo() {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        m_WorkBookInfo.Json(&wWriter);
        wWriter.EndObject();
        return(wStringBuffer.GetString());
    }

	tAllocatorRef  tWorkBook::AllocatorRef() { return(m_AllocatorRef); }

	void tWorkBook::AllocatorRef(tAllocatorRef sAllocatorRef) { m_AllocatorRef = sAllocatorRef; };




	// Private ================================================================
	tVectorAllocatorRef::iterator tWorkBook::GetIteratorSheet(tString sSheetName) {
#ifdef debugsheet
            cout << "Search Sheet :" << sSheetName << endl;
#endif
		tVectorAllocatorRef::iterator wIterator;
		for (wIterator = m_VectorSheet.begin(); wIterator != m_VectorSheet.end(); wIterator++) {
            tSheet* wSheet=m_SheetAllocator(*wIterator);
            if (wSheet!=nullptr) {
#ifdef debugsheet
                cout << " " << wSheet->Name() << endl;
#endif
                if (wSheet->Name() == sSheetName) return(wIterator);
            }
		}
		return(m_VectorSheet.end());
	}

	void tWorkBook::NormalizeSheet(tSheet** sSheet) {
		if (*sSheet == nullptr) *sSheet = m_SheetAllocator(m_ActiveSheet);
	}

	// public ================================================================
	// Uri
	tString tWorkBook::Uri() { return(m_Uri); }

    void tWorkBook::Uri(tString sUri) { m_Uri=sUri; }


    // Interface Api
    void tWorkBook::FormatApi(tFormatApi* sFormatApi) { tSpreadSheetContainer::Instance()->FormatApi(sFormatApi); }
    tFormatApi* tWorkBook::FormatApi() { return(tSpreadSheetContainer::Instance()->FormatApi()); }

    tFormatRef tWorkBook::EnsureJsonViewGeneralHAlign() {
        if (m_JsonViewGeneralHAlignRef == 0 && FormatApi() != nullptr) {
            m_JsonViewGeneralHAlignRef = FormatApi()->ApplyCellFormat("text-align:none;");
#ifdef checkfo
            // Seed Check so CheckCell() between JsonView and CheckFormat sees Real==Check.
            // CheckFormat ResetCheck + IncCheck will recount this owner next.
            if (m_JsonViewGeneralHAlignRef != 0) {
                FormatApi()->IncCheck(m_JsonViewGeneralHAlignRef);
            }
#endif
        }
        return m_JsonViewGeneralHAlignRef;
    }

    tTableStyleContainer* tWorkBook::TableStyleContainer() {
        return &m_TableStyleContainer;
    }

    const tTableStyleContainer* tWorkBook::TableStyleContainer() const {
        return &m_TableStyleContainer;
    }

    tFormatRef tWorkBook::TableStyleOverlayFormat(tSheet* sSheet, tIndex sRow, tIndex sCol) {
        return m_TableStyleContainer.OverlayForCell(sSheet, sRow, sCol);
    }

    tString tWorkBook::TableStyleOverlayCss(tSheet* sSheet, tIndex sRow, tIndex sCol) {
        return m_TableStyleContainer.OverlayCssString(sSheet, sRow, sCol);
    }

    tString tWorkBook::CellFormatString(tCell* sCell) {
#ifdef debugformat
        cout << "tWorkBook::CellFormatString Cell(" << sCell->StrRef() << ")" << sCell->Value() << endl;
#endif
        tFormatApi* wFormatApi=FormatApi();
        tString wFormatStringStr="";
        if (wFormatApi!=nullptr) {
            wFormatStringStr=wFormatApi->CellFormatString(sCell->Css(),&sCell->Value());
#ifdef debugformat
            cout << "tWorkBook::CellFormatString =" << wFormatStringStr << endl;
#endif
            // Intercept Unit --> future to do
            //tCellClassUnit* wCellClassUnit;
            if (sCell->Class()!=nullptr) {
                //wCellClassUnit=dynamic_cast<tCellClassUnit*>(sCell->Class());
                return(wFormatStringStr);
            }
            // Intercept Money
            tFormatString* wFormatString=wFormatApi->CellFormatString(sCell->Css());
            if (wFormatString!=nullptr) {
                t_UnitMoney wMoney=wFormatString->Money();
                if (wMoney!=t_UnitMoney::None)
                    wFormatStringStr+=" "+UnitMoneySymbol(wMoney);
            }
        }
#ifdef debugformat
        cout << "tWorkBook::CellFormatString return:" << wFormatStringStr << endl;
#endif
        return(wFormatStringStr);
    }

    tString tWorkBook::CellInputString(tCell* sCell) {
        tFormatApi* wFormatApi=FormatApi();
        tVariant wVariant(sCell->CalculableValue());
        tFormatString wFormatResolved;
        if ((wFormatApi!=nullptr) && (sCell->Css()!=0)) {
            tFormatString* wFormatString=wFormatApi->CellFormatString(sCell->Css());
            if (wFormatString!=nullptr) {
                wFormatResolved = *wFormatString;
            }
        }
        // InputString resolves date masks (General/excelnumber → locale date).
        return(wVariant.InputString(&wFormatResolved));
    }

    tShort tWorkBook::CellBorder(tFormatRef sAllocatorRef) {
        return(FormatApi()->CellBorder(sAllocatorRef));
    }

    tBool tWorkBook::ApplyCellFormat(tCell* sCell,tString sFormat) {
        tFormatApi* wFormatApi=FormatApi();
#ifdef debugformat
        cout << "tWorkBook::ApplyCellFormat(" << sCell->StrRef() << "=" << sFormat << ") " << sCell->Css() << endl;
#endif
        if (wFormatApi!=nullptr) {
            if (sCell->Css()==0) {
                tFormatRef wNew=wFormatApi->ApplyCellFormat(sFormat);
                // Error compil
                if (wNew==0) {
#ifdef debugformat
                    cout << "tWorkBook::ApplyCellFormat error:" << sFormat << endl;
#endif
                    return(false);
                }
                sCell->Css(wNew);
            } else {
                // Make a new format with sFormat
                tFormatRef wNew=wFormatApi->ApplyCellFormat(sFormat);
                // Error ======================================================
                if (wNew==0) {
#ifdef debugformat
                    cout << "tWorkBook::ApplyCellFormat error:" << sFormat << endl;
#endif
                    //sCell->Css(0); // d'ont change all value
                    return(false);
                }
                // Don't Modify
                if (wNew==sCell->Css()) {
                    return(true);
                }
                // Get old value to merge
                tFormatRef wOld=sCell->Css();
                wFormatApi->BeginMerge();
                wFormatApi->Merge(wOld);
                wFormatApi->Merge(wNew);
                tFormatRef wApply=wFormatApi->ApplyMerge();
                sCell->Css(wApply);
                // Raz instance of new format
                wFormatApi->DeleteCellFormat(wNew);
            }
        }
        return(true);
    }

    tBool tWorkBook::ApplyColRowFormat(tColRow* sColRow,tString sFormat) {
#ifdef debugformat
        cout << "tWorkBook::ApplyColRowFormat(" << sColRow->Index() << "=" << sFormat << ") " << sColRow->Css() << endl;
#endif
        tFormatApi* wFormatApi=FormatApi();
        if (wFormatApi!=nullptr) {
            if (sColRow->Css()==0) {
                tFormatRef wNew=wFormatApi->ApplyCellFormat(sFormat);
                if (wNew==0) {
#ifdef debugformat
                    cout << "tWorkBook::ApplyColRowFormat error:" << sFormat << endl;
#endif
                    return(false);
                }
                sColRow->Css(wNew);
            } else {
                // Make a new format with sFormat
                tFormatRef wNew=wFormatApi->ApplyCellFormat(sFormat);
                // Error ======================================================
                if (wNew==0) {
#ifdef debugformat
                    cout << "tWorkBook::ApplyColRowFormat error:" << sFormat << endl;
#endif
                    return(false);
                }
                
                // Get Old value for merge
                tFormatRef wOld=sColRow->Css();
                wFormatApi->BeginMerge();
                wFormatApi->Merge(wOld);
                wFormatApi->Merge(wNew);
                tFormatRef wApply=wFormatApi->ApplyMerge();
                sColRow->Css(wApply);
                // Raz instance of new format
                wFormatApi->DeleteCellFormat(wNew);
                return(true);
            }
        }
        return(false);
    }

    tBool tWorkBook::ApplySheetFormat(tSheet* sSheet,tString sFormat) {
        tFormatApi* wFormatApi=FormatApi();
#ifdef debugformat
        cout << "tWorkBook::ApplySheetFormat(" << sSheet->Name() << "=" << sFormat << ") " << sSheet->Css() << endl;
#endif
        if (wFormatApi!=nullptr) {
            if (sSheet->Css()==0) {
                tFormatRef wNew=wFormatApi->ApplyCellFormat(sFormat);
                if (wNew==0) {
#ifdef debugformat
                    cout << "tWorkBook::ApplySheetFormat error:" << sFormat << endl;
#endif
                    return(false);
                }
                sSheet->Css(wNew);
                return(true);
            } else {
                // Make a new format with sFormat
                tFormatRef wNew=wFormatApi->ApplyCellFormat(sFormat);
                // Error ======================================================
                if (wNew==0) {
#ifdef debugformat
                    cout << "tWorkBook::ApplySheetFormat error:" << sFormat << endl;
#endif
                    return(false);
                }
                // Get Old value for merge
                tFormatRef wOld=sSheet->Css();
                wFormatApi->BeginMerge();
                wFormatApi->Merge(wOld);
                wFormatApi->Merge(wNew);
                tFormatRef wApply=wFormatApi->ApplyMerge();
                sSheet->Css(wApply);
                // Raz instance of new format
                wFormatApi->DeleteCellFormat(wNew);
                return(true);
            }
        }
        return(false);
    }

    void tWorkBook::DeleteCellFormat(tFormatRef sFormatRef) {
        if (FormatApi()!=nullptr) {
            //cout << "Delete CellFormat :" << CellFormat(sFormatRef) << endl;
            FormatApi()->DeleteCellFormat(sFormatRef);
        }
    }

    void tWorkBook::IncCellFormat(tFormatRef sFormatRef) {
        if (FormatApi()!=nullptr) {
            //cout << "Inc CellFormat :" << CellFormat(sFormatRef) << endl;
            FormatApi()->IncCellFormat(sFormatRef);
        }
    }

    tInt tWorkBook::CellFormatRefCount(tFormatRef sFormatRef) {
        if (FormatApi() != nullptr) {
            return(FormatApi()->CellFormatRefCount(sFormatRef));
        }
        return(0);
    }

    tString  tWorkBook::CellFormat(tFormatRef sFormatRef) {
        if (FormatApi()!=nullptr) {
            return(FormatApi()->CellFormat(sFormatRef));
        }
        return("");
    }

    tString  tWorkBook::CellFormat(tVectorFormatRef* sVector) {
        if (FormatApi()!=nullptr) {
            return(FormatApi()->CellFormat(sVector));
        }
        return("");
    }


    tFormatRef tWorkBook::DeleteBorder(tFormatRef sFormatRef,tShort sBorderType) {
        tFormatRef wReturn=0;
        switch (sBorderType) {
            case tBorderAll :
            case tBorderTop :
            case tBorderLeft :
            case tBorderBottom :
            case tBorderRight : {
                wReturn=FormatApi()->DeleteBorder(sFormatRef,sBorderType);
                break;
            }
            default : {
                //cout << endl << FormatApi()->Debug() << endl;
                wReturn=FormatApi()->DeleteBorder(sFormatRef,sBorderType);
                break;
            }
        }
        return(wReturn);
    }

    tShort tWorkBook::BorderMask(tFormatRef sFormatRef) {
        return(FormatApi()->BorderMask(sFormatRef));
    }

    bool tWorkBook::tJsonViewStyleKey::operator<(const tJsonViewStyleKey& sOther) const {
        if (m_Sheet != sOther.m_Sheet) return m_Sheet < sOther.m_Sheet;
        if (m_Col != sOther.m_Col) return m_Col < sOther.m_Col;
        if (m_Row != sOther.m_Row) return m_Row < sOther.m_Row;
        if (m_Table != sOther.m_Table) return m_Table < sOther.m_Table;
        if (m_Table2 != sOther.m_Table2) return m_Table2 < sOther.m_Table2;
        if (m_Table3 != sOther.m_Table3) return m_Table3 < sOther.m_Table3;
        if (m_Cell != sOther.m_Cell) return m_Cell < sOther.m_Cell;
        return m_ItemCf < sOther.m_ItemCf;
    }

    void tWorkBook::BeginJsonViewFormatTable() {
        m_JsonViewFormatTable.clear();
        m_JsonViewFormatIndex.clear();
        m_JsonViewFormatTableActive = true;
    }

    tSize tWorkBook::AcquireJsonViewStyleIndex(
        tFormatRef sSheet,
        tFormatRef sCol,
        tFormatRef sRow,
        tFormatRef sTable,
        tFormatRef sTable2,
        tFormatRef sTable3,
        tFormatRef sCell,
        tFormatRef sItemCf,
        tBool sCss) {
        tJsonViewStyleKey wKey{sSheet, sCol, sRow, sTable, sTable2, sTable3, sCell, sItemCf};
        const auto wFound = m_JsonViewFormatIndex.find(wKey);
        if (wFound != m_JsonViewFormatIndex.end()) {
            return wFound->second;
        }
        tVectorFormatRef wVector;
        const tFormatRef wGeneralHAlign =
            (sCell != 0 && !FormatHasExplicitHAlign(this, sCell))
                ? EnsureJsonViewGeneralHAlign()
                : 0;
        AppendJsonViewFormatLayers(
            wVector, sSheet, sCol, sRow, sTable, sTable2, sTable3,
            wGeneralHAlign, sCell, sItemCf);
        StringBuffer wStyleBuffer;
        Writer<StringBuffer> wStyleWriter(wStyleBuffer);
        wStyleWriter.StartObject();
        FormatApi()->_JsonJavaScriptMergedStyle(&wVector, sCss, &wStyleWriter);
        wStyleWriter.EndObject();
        const tSize wIndex = m_JsonViewFormatTable.size();
        m_JsonViewFormatTable.push_back(wStyleBuffer.GetString());
        m_JsonViewFormatIndex[wKey] = wIndex;
        return wIndex;
    }

    void tWorkBook::WriteJsonViewFormatTable(Writer<StringBuffer>* sWriter) {
        if (!m_JsonViewFormatTableActive || m_JsonViewFormatTable.empty()) {
            m_JsonViewFormatTableActive = false;
            m_JsonViewFormatTable.clear();
            m_JsonViewFormatIndex.clear();
            return;
        }
        sWriter->Key("formats");
        sWriter->StartArray();
        for (const tString& wFormatJson : m_JsonViewFormatTable) {
            sWriter->RawValue(
                wFormatJson.c_str(),
                static_cast<SizeType>(wFormatJson.size()),
                kObjectType);
        }
        sWriter->EndArray();
        m_JsonViewFormatTableActive = false;
        m_JsonViewFormatTable.clear();
        m_JsonViewFormatIndex.clear();
    }

    void tWorkBook::JsonFormatJavaScript(tSheet* sSheet,tIndex sRow, tIndex sCol,tBool sCss,tCellConditionalFormat* sCellConditionalFormat,Writer<StringBuffer>* sWriter) {
        if (FormatApi()==nullptr) {
            return;
        }
        tFormatRef wFormatSheet=sSheet->Css();
        tFormatRef wFormatCol=0;
        tFormatRef wFormatRow=0;
        tVectorFormatRef wTableLayers;
        m_TableStyleContainer.AppendOverlayFormats(sSheet, sRow, sCol, wTableLayers);
        const tFormatRef wFormatTable = wTableLayers.empty() ? 0 : wTableLayers.front();
        const tFormatRef wFormatTable2 = wTableLayers.size() > 1 ? wTableLayers[1] : 0;
        const tFormatRef wFormatTable3 = wTableLayers.size() > 2 ? wTableLayers[2] : 0;
        tFormatRef wFormatCell=0;

        tColRow* wCol=sSheet->Col(sCol);
        if (wCol!=nullptr) wFormatCol=wCol->Css();

        tColRow* wRow=sSheet->Row(sRow);
        if (wRow!=nullptr) wFormatRow=wRow->Css();

        tCell* wCell = sSheet->Cell(sRow, sCol);
        tVariant* wVariant=nullptr;
        if (wCell!=nullptr) {
            wFormatCell=wCell->Css();
            if (wCell->Value().IsClass()) {
                wVariant=wCell->Value().Class()->Value();
            } else {
                wVariant=&wCell->Value();
            }
        }

        tFormatRef wFormatItemCF=0;
        const tBool wHasPerCellCf =
            (sCellConditionalFormat != nullptr && !sCellConditionalFormat->IsEmpty());
        tBool wNeedsCfVisualArray = false;
        if (wHasPerCellCf) {
            tColRowCellRange* wColRowCellRange=sSheet->ColRowCellRange();
            for (auto wItemCFIndex : sCellConditionalFormat->VectorItemCF()) {
                tItemCF* wItemCF=wColRowCellRange->ItemCF(wItemCFIndex);
                if (wItemCF == nullptr) {
                    continue;
                }
                switch (wItemCF->Type()) {
                    case tConditionalFormatType::t_None:
                        break;
                    case tConditionalFormatType::t_CustomFormulas:
                    case tConditionalFormatType::t_ColorScales:
                    case tConditionalFormatType::t_HighlightCellsRules:
                        wFormatItemCF=wItemCF->Css();
                        break;
                    case tConditionalFormatType::t_DataBars:
                    case tConditionalFormatType::t_IconSets:
                        wNeedsCfVisualArray = true;
                        break;
                    default:
                        break;
                }
            }
        }

        if (wNeedsCfVisualArray) {
            sWriter->Key("c_cf");
            sWriter->StartArray();
            tColRowCellRange* wColRowCellRange=sSheet->ColRowCellRange();
            for (auto wItemCFIndex : sCellConditionalFormat->VectorItemCF()) {
                tItemCF* wItemCF=wColRowCellRange->ItemCF(wItemCFIndex);
                if (wItemCF == nullptr) {
                    continue;
                }
                switch (wItemCF->Type()) {
                    case tConditionalFormatType::t_DataBars:
                        wItemCF->JsonDataBars(sWriter);
                        break;
                    case tConditionalFormatType::t_IconSets:
                        wItemCF->JsonIconSets(sWriter);
                        break;
                    default:
                        break;
                }
            }
            sWriter->EndArray();
        }

        const tBool wHasLayerFormat =
            (wFormatSheet != 0) || (wFormatCol != 0) || (wFormatRow != 0) ||
            (wFormatTable != 0) || (wFormatCell != 0) || (wFormatItemCF != 0);

        if (!wHasLayerFormat && !wNeedsCfVisualArray) {
            tVectorFormatRef wEmptyTable;
            EmitJsonViewVariantWithLayers(
                FormatApi(), 0, 0, 0, wEmptyTable, 0, 0, 0, wVariant, sCss, sWriter);
            FormatApi()->_JsonJavaScriptDefaultStringAlign(wVariant, sWriter);
            return;
        }

        const tFormatRef wGeneralHAlign =
            (wFormatCell != 0 && !FormatHasExplicitHAlign(this, wFormatCell))
                ? EnsureJsonViewGeneralHAlign()
                : 0;

        if (wNeedsCfVisualArray || !m_JsonViewFormatTableActive) {
            if ((wFormatSheet==0) && (wFormatCol==0) && (wFormatRow==0)
                && wTableLayers.empty() && (wFormatItemCF==0) && (wGeneralHAlign==0)) {
                FormatApi()->_JsonJavaScript(wFormatCell, wVariant, sCss, sWriter);
            } else {
                tVectorFormatRef wVector;
                AppendJsonViewFormatLayers(
                    wVector, wFormatSheet, wFormatCol, wFormatRow, wTableLayers,
                    wGeneralHAlign, wFormatCell, wFormatItemCF);
                FormatApi()->_JsonJavaScript(&wVector, wVariant, sCss, sWriter);
            }
            return;
        }

        EmitJsonViewVariantWithLayers(
            FormatApi(),
            wFormatSheet,
            wFormatCol,
            wFormatRow,
            wTableLayers,
            wGeneralHAlign,
            wFormatCell,
            wFormatItemCF,
            wVariant,
            sCss,
            sWriter);
        if (wHasLayerFormat) {
            const tSize wStyleIndex = AcquireJsonViewStyleIndex(
                wFormatSheet, wFormatCol, wFormatRow,
                wFormatTable, wFormatTable2, wFormatTable3,
                wFormatCell, wFormatItemCF, sCss);
            sWriter->Key("f_i");
            sWriter->Uint(static_cast<unsigned>(wStyleIndex));
        }
    }

	// Interface Sheet
	tSheet* tWorkBook::AddSheet(tString sSheetName,tString sSheetLeft) {
		tVectorAllocatorRef::iterator wIterator = GetIteratorSheet(sSheetName);
		if (wIterator != m_VectorSheet.end()) {
			m_ActiveSheet = *wIterator;
			return(m_SheetAllocator(m_ActiveSheet));
		}
		tAllocatorRef wId;
		tSheet* wSheet;
		tie(wId, wSheet) = m_SheetAllocator.Alloc();
		wSheet->Set(wId, AllocatorRef(), sSheetName);
        AddSheetInList(wId, sSheetLeft);
		m_ActiveSheet = wId;
#ifdef checksp
    Check();
#endif
		return(wSheet);
	}

	void tWorkBook::AddSheetInList(tAllocatorRef sSheetAllocator, tString sSheetLeft) {
		if (sSheetLeft == "") {
            m_VectorSheet.push_back(sSheetAllocator);
		}
		else {
            tVectorAllocatorRef::iterator wIterator;
			for (wIterator = m_VectorSheet.begin(); wIterator != m_VectorSheet.end(); wIterator++) {
				if (m_SheetAllocator(*wIterator)->Name() == sSheetLeft) {
					break;
				}
			}
			m_VectorSheet.insert(wIterator, sSheetAllocator);
		}
	}

    void tWorkBook::SwapSheets(tString sName1,tString sName2) {
        tVectorAllocatorRef::iterator wIterator1 = m_VectorSheet.end();
        tVectorAllocatorRef::iterator wIterator2 = m_VectorSheet.end();
        for (tVectorAllocatorRef::iterator wIterator = m_VectorSheet.begin();
             wIterator != m_VectorSheet.end();
             ++wIterator) {
            if (m_SheetAllocator(*wIterator)->Name() == sName1) {
                wIterator1 = wIterator;
            }
            if (m_SheetAllocator(*wIterator)->Name() == sName2) {
                wIterator2 = wIterator;
            }
        }
        if (wIterator1 != m_VectorSheet.end() && wIterator2 != m_VectorSheet.end()) {
            std::swap(*wIterator1, *wIterator2);
        }
    }

    tString tWorkBook::SheetLeftOf(tString sSheetName) {
        for (tVectorAllocatorRef::iterator wIterator = m_VectorSheet.begin();
             wIterator != m_VectorSheet.end();
             ++wIterator) {
            if (m_SheetAllocator(*wIterator)->Name() != sSheetName) {
                continue;
            }
            if (wIterator == m_VectorSheet.begin()) {
                return("");
            }
            tVectorAllocatorRef::iterator wPrev = wIterator;
            --wPrev;
            return(m_SheetAllocator(*wPrev)->Name());
        }
        return("");
    }

    void tWorkBook::MoveSheet(tString sSheetName, tString sInsertAfterName) {
        tVectorAllocatorRef::iterator wMoveIt = m_VectorSheet.end();
        for (tVectorAllocatorRef::iterator wIterator = m_VectorSheet.begin();
             wIterator != m_VectorSheet.end();
             ++wIterator) {
            if (m_SheetAllocator(*wIterator)->Name() == sSheetName) {
                wMoveIt = wIterator;
                break;
            }
        }
        if (wMoveIt == m_VectorSheet.end()) {
            return;
        }
        const tAllocatorRef wSheetRef = *wMoveIt;
        m_VectorSheet.erase(wMoveIt);

        if (sInsertAfterName.empty()) {
            m_VectorSheet.insert(m_VectorSheet.begin(), wSheetRef);
            return;
        }
        tVectorAllocatorRef::iterator wInsertIt = m_VectorSheet.end();
        for (tVectorAllocatorRef::iterator wIterator = m_VectorSheet.begin();
             wIterator != m_VectorSheet.end();
             ++wIterator) {
            if (m_SheetAllocator(*wIterator)->Name() == sInsertAfterName) {
                wInsertIt = wIterator;
                break;
            }
        }
        if (wInsertIt == m_VectorSheet.end()) {
            m_VectorSheet.push_back(wSheetRef);
            return;
        }
        ++wInsertIt;
        m_VectorSheet.insert(wInsertIt, wSheetRef);
    }


	tBool tWorkBook::DeleteSheet(tString sSheetName, tBool sErase) {
		tVectorAllocatorRef::iterator wIterator = GetIteratorSheet(sSheetName);
		if (wIterator != m_VectorSheet.end()) {
			tAllocatorRef wDeletedRef = *wIterator;
			if (sErase) {
                m_SheetAllocator.Delete(*wIterator);
                m_OffListSheets.erase(sSheetName);
            } else {
                m_OffListSheets[sSheetName] = wDeletedRef;
            }
			m_VectorSheet.erase(wIterator);
			// Do not leave m_ActiveSheet pointing at a freed sheet slot (undefined behavior; often traps in WASM).
			if (m_VectorSheet.empty()) {
				m_ActiveSheet = 0;
			} else if (m_ActiveSheet == wDeletedRef) {
				m_ActiveSheet = m_VectorSheet.front();
			}
			return(true);
		}
		return(false);
	}

	tBool tWorkBook::DeleteSheet(tAllocatorRef sAllocatorRef) {
		return(m_SheetAllocator.Delete(sAllocatorRef));
	}


	tSheet* tWorkBook::Sheet(tString sSheetName) {
        if (sSheetName == CstSheetClassAnchor && m_SheetClassAnchor != 0) {
            return (m_SheetAllocator(m_SheetClassAnchor));
        }
        if (sSheetName == CstSheetNamed && m_SheetNamedFormulaRef != 0) {
            return (m_SheetAllocator(m_SheetNamedFormulaRef));
        }
		tVectorAllocatorRef::iterator wIterator = GetIteratorSheet(sSheetName);
		if (wIterator != m_VectorSheet.end()) {
			return(m_SheetAllocator(*wIterator));
		}
		return(nullptr);
	}

	tSheet* tWorkBook::SheetByAllocator(tAllocatorRef sAllocatorRef) {
		return(m_SheetAllocator(sAllocatorRef));
	}

    tSheet* tWorkBook::TakeOffListSheet(tString sSheetName, tIndex sIndex) {
        auto wIterator = m_OffListSheets.find(sSheetName);
        if (wIterator == m_OffListSheets.end()) {
            return(nullptr);
        }
        const tAllocatorRef wSheetRef = wIterator->second;
        m_OffListSheets.erase(wIterator);
        if (m_SheetAllocator.IsNullptr(wSheetRef)) {
            return(nullptr);
        }
        if (sIndex > m_VectorSheet.size()) {
            sIndex = static_cast<tIndex>(m_VectorSheet.size());
        }
        m_VectorSheet.insert(m_VectorSheet.begin() + sIndex, wSheetRef);
        return(m_SheetAllocator(wSheetRef));
    }

	tSheet* tWorkBook::ActiveSheet(tString sSheetName) {
        // Internal _$$ sheets: resolve via Sheet() but keep the user-facing active sheet unchanged.
        if (IsSystemSheetName(sSheetName)) {
            if (sSheetName == CstSheetClassAnchor && m_SheetClassAnchor == 0) {
                SheetClassAnchor();
            }
            if (sSheetName == CstSheetNamed && m_SheetNamedFormulaRef == 0) {
                SheetNamedFormula();
            }
            return (Sheet(sSheetName));
        }
		tVectorAllocatorRef::iterator wIterator = GetIteratorSheet(sSheetName);
		if (wIterator != m_VectorSheet.end()) {
			m_ActiveSheet = (*wIterator);
			return(m_SheetAllocator(*wIterator));
		}
		return(nullptr);
	}

	tSheet* tWorkBook::ActiveSheet() { return(m_SheetAllocator(m_ActiveSheet)); }

    void tWorkBook::RecalculateSheet(tSheet* sSheet,tContainerPath* sContainerPath) {
#ifdef  debugcalculate
        tIndex wLastRow = sSheet->LastRow();
        tIndex wLastCol = sSheet->LastCol();
        cout << "Recalculate All " << sSheet->Name() << "->A0:" << Base10ToAlpha(wLastCol) << wLastRow << endl;
#endif
        // Diagnostic for WASM "memory access out of bounds" on Budget.sker (Apr 2026): the trap loses any
        // stack info under Node, so emit the sheet boundary on stderr to localize the crashing sheet.
#ifdef diagcalc
        std::cerr << "[diag] RecalculateSheet START name=\"" << sSheet->Name() << "\" lastRow=" << sSheet->LastRow()
                  << " lastCol=" << sSheet->LastCol() << std::endl;
        std::cerr.flush();
#endif
        // Sparse formula scan: AddRect on the full sheet bbox triggers FindRanges recovery
        // for every empty cell in the grid (catastrophic on large sparse sheets like Ref).
        tColRowCellRange* wColRowCellRange = sSheet->ColRowCellRange();
        if (wColRowCellRange != nullptr) {
            sContainerPath->AddAllFormulaCells(wColRowCellRange);
        }
#ifdef diagcalc
        std::cerr << "[diag] RecalculateSheet END   name=\"" << sSheet->Name() << "\"" << std::endl;
        std::cerr.flush();
#endif
    }

	void tWorkBook::RecalculateAll() {
		tContainerPath wContainerPath;
		PrepareRecalculateAll(&wContainerPath);
        wContainerPath.EndCalculate();
	}

    void tWorkBook::CancelRecalculateAllCooperative() {
        if (m_CooperativeRecalcPath != nullptr) {
            delete m_CooperativeRecalcPath;
            m_CooperativeRecalcPath = nullptr;
        }
        m_CooperativeRecalcActive = false;
        m_CooperativeRecalcTotal = 0;
    }

    void tWorkBook::PrepareRecalculateAll(tContainerPath* sContainerPath) {
        if (sContainerPath == nullptr) {
            return;
        }
		// Stale tCell::Path() from a previous tContainerPath (e.g. JsonEnd Reduce incomplete) blocks Add(); clear first.
		ResetAllCellCalculationPathsToZero();
        ClearPersistedSpillSlavesForRecalc();
        {
            auto wInvalidate = [](tSheet* sSheet) {
                if (sSheet != nullptr && sSheet->ColRowCellRange() != nullptr) {
                    sSheet->ColRowCellRange()->InvalidateExtentCache();
                }
            };
            if (m_SheetNamedFormulaRef != 0) {
                wInvalidate(m_SheetAllocator(m_SheetNamedFormulaRef));
            }
            for (auto wSheetRef : m_VectorSheet) {
                wInvalidate(m_SheetAllocator(wSheetRef));
            }
            if (m_SheetClassAnchor != 0) {
                wInvalidate(m_SheetAllocator(m_SheetClassAnchor));
            }
        }
		sContainerPath->BeginCalculate();

		if (m_SheetNamedFormulaRef != 0) {
			tSheet* wSheetNamedFormula = m_SheetAllocator(m_SheetNamedFormulaRef);
			RecalculateSheet(wSheetNamedFormula, sContainerPath);
		}
		for (auto wSheetRef : m_VectorSheet) {
			tSheet* wSheet = m_SheetAllocator(wSheetRef);
			if (wSheet != nullptr) {
				RecalculateSheet(wSheet, sContainerPath);
			}
		}
    }

    void tWorkBook::BeginRecalculateAllCooperative() {
        CancelRecalculateAllCooperative();
        m_CooperativeRecalcPath = new tContainerPath();
        PrepareRecalculateAll(m_CooperativeRecalcPath);
        m_CooperativeRecalcTotal = m_CooperativeRecalcPath->CountPathsInList();
        m_CooperativeRecalcActive = true;
    }

    tBool tWorkBook::StepRecalculateAllCooperative(tInt sMaxMs) {
        if (!m_CooperativeRecalcActive || m_CooperativeRecalcPath == nullptr) {
            return true;
        }
        const tBool wDone = m_CooperativeRecalcPath->ReduceStep(sMaxMs);
        if (wDone) {
            CancelRecalculateAllCooperative();
        }
        return wDone;
    }

    tInt tWorkBook::RecalculateAllCooperativeProgress() {
        if (!m_CooperativeRecalcActive || m_CooperativeRecalcPath == nullptr) {
            return 100;
        }
        if (m_CooperativeRecalcTotal <= 0) {
            return 0;
        }
        const tInt wRemaining = m_CooperativeRecalcPath->CountPathsInList();
        const tInt wDone = m_CooperativeRecalcTotal - wRemaining;
        if (wRemaining <= 0) {
            return 100;
        }
        if (wDone <= 0) {
            return 0;
        }
        tInt wPct = (wDone * 100) / m_CooperativeRecalcTotal;
        // Kahn leftover (range-covered SUMIF cycles) stays on the list while values
        // iterate in place — without this the bar freezes (~88%) during that pass.
        const tInt wBlockedPct = m_CooperativeRecalcPath->BlockedSubgraphProgressPercent();
        if (wBlockedPct > 0) {
            const tInt wTail = (wRemaining * wBlockedPct) / 100;
            const tInt wAdj = wDone + wTail;
            wPct = (wAdj * 100) / m_CooperativeRecalcTotal;
            if (wPct > 99) {
                wPct = 99;
            }
        }
        return wPct;
    }

    tBool tWorkBook::IsRecalculateAllCooperativeActive() const {
        return m_CooperativeRecalcActive;
    }

    void tWorkBook::SetCooperativeCalculateEnabled(tBool sEnabled) {
        m_CooperativeCalculateEnabled = sEnabled;
        if (!sEnabled) {
            return;
        }
        // A new cooperative edit must not reuse a stale full-workbook session.
        CancelRecalculateAllCooperative();
    }

    tBool tWorkBook::CooperativeCalculateEnabled() const {
        return m_CooperativeCalculateEnabled;
    }

    void tWorkBook::BeginCooperativeCalculateFromSaveSelect(
        tSaveSelect* sSaveSelect,
        tColRowCellRange* sColRowCellRange,
        tVolatile sVolatile) {
        if (sSaveSelect == nullptr || sColRowCellRange == nullptr) {
            return;
        }
        CancelRecalculateAllCooperative();
        m_CooperativeRecalcPath = new tContainerPath();
        sSaveSelect->PopulateCalculationGraph(m_CooperativeRecalcPath, sColRowCellRange, sVolatile);
        m_CooperativeRecalcTotal = m_CooperativeRecalcPath->CountPathsInList();
        m_CooperativeRecalcActive = (m_CooperativeRecalcTotal > 0);
    }

	void tWorkBook::ResetAllCellCalculationPathsToZero() {
		for (auto wSheetRef : m_VectorSheet) {
			ResetAllCellPathsOnSheet(m_SheetAllocator(wSheetRef));
		}
		if (m_SheetNamedFormulaRef != 0) {
			ResetAllCellPathsOnSheet(m_SheetAllocator(m_SheetNamedFormulaRef));
		}
	}

    void tWorkBook::RecalculateSheetNamedFormula() {
        ResetAllCellCalculationPathsToZero();
        tContainerPath wContainerPath;
        wContainerPath.BeginCalculate();
        tSheet* wSheetNamedFormula = m_SheetAllocator(m_SheetNamedFormulaRef);
        RecalculateSheet(wSheetNamedFormula, &wContainerPath);
        wContainerPath.EndCalculate();
        //PruneArrayFormulaSpillsOnSheet(wSheetNamedFormula);
    }

    void tWorkBook::RelinkJsonPersistedSpillSlaves() {
        struct tCallBackRelinkSpillSlaves : tSparseArrayCallBack<tAllocatorRef> {
            tColRowCellRange* m_ColRowCellRange;
            explicit tCallBackRelinkSpillSlaves(tColRowCellRange* sColRowCellRange)
                : m_ColRowCellRange(sColRowCellRange) {}
            tBool CallBack(tAllocatorRef sAllocatorRef) override {
                tCell* wOrigin = m_ColRowCellRange->Cell(sAllocatorRef);
                if (wOrigin == nullptr || wOrigin->Formula() == nullptr) {
                    return true;
                }
                tTempoRect wOut = wOrigin->ArrayFormulaOutputRect();
                const tIndex wTop = wOut.Top();
                const tIndex wBottom = wOut.Bottom();
                const tIndex wLeft = wOut.Left();
                const tIndex wRight = wOut.Right();
                if (wTop > wBottom || wLeft > wRight) {
                    return true;
                }
                for (tIndex wRow = wTop; wRow <= wBottom; ++wRow) {
                    for (tIndex wCol = wLeft; wCol <= wRight; ++wCol) {
                        if (wRow == wOrigin->RowIndex() && wCol == wOrigin->ColIndex()) {
                            continue;
                        }
                        tCell* wSlave = m_ColRowCellRange->Cell(wRow, wCol);
                        if (wSlave == nullptr || wSlave->Formula() != nullptr) {
                            continue;
                        }
                        if (!wSlave->Value().IsExcelNull()) {
                            wSlave->SetMatExtend();
                        }
                    }
                }
                return true;
            }
        };
        auto wRelinkSheet = [&](tSheet* sSheet) {
            if (sSheet == nullptr) {
                return;
            }
            tCallBackRelinkSpillSlaves wCallBack(sSheet->ColRowCellRange());
            sSheet->ColRowCellRange()->CallBackAllCell(&wCallBack);
        };
        for (auto wSheetRef : m_VectorSheet) {
            wRelinkSheet(m_SheetAllocator(wSheetRef));
        }
        if (m_SheetNamedFormulaRef != 0) {
            wRelinkSheet(m_SheetAllocator(m_SheetNamedFormulaRef));
        }
        if (m_SheetClassAnchor != 0) {
            wRelinkSheet(m_SheetAllocator(m_SheetClassAnchor));
        }
    }

    void tWorkBook::ClearPersistedSpillSlavesForRecalc() {
        struct tCallBackClearMatExtendValues : tSparseArrayCallBack<tAllocatorRef> {
            tColRowCellRange* m_ColRowCellRange;
            explicit tCallBackClearMatExtendValues(tColRowCellRange* sColRowCellRange)
                : m_ColRowCellRange(sColRowCellRange) {}
            tBool CallBack(tAllocatorRef sAllocatorRef) override {
                tCell* wCell = m_ColRowCellRange->Cell(sAllocatorRef);
                if (wCell == nullptr || wCell->Formula() != nullptr) {
                    return true;
                }
                if (wCell->IsMatExtend()) {
                    wCell->RemoveMatExtend();
                    wCell->Value(tVariant());
                }
                return true;
            }
        };
        struct tCallBackClearSpillSlaves : tSparseArrayCallBack<tAllocatorRef> {
            tColRowCellRange* m_ColRowCellRange;
            explicit tCallBackClearSpillSlaves(tColRowCellRange* sColRowCellRange)
                : m_ColRowCellRange(sColRowCellRange) {}
            tBool CallBack(tAllocatorRef sAllocatorRef) override {
                tCell* wOrigin = m_ColRowCellRange->Cell(sAllocatorRef);
                if (wOrigin == nullptr || wOrigin->Formula() == nullptr) {
                    return true;
                }
                tIndex wTop = 0;
                tIndex wBottom = 0;
                tIndex wLeft = 0;
                tIndex wRight = 0;
                if (tRange* wSpill = wOrigin->SpillRange(); wSpill != nullptr) {
                    wTop = wSpill->TopIndex();
                    wBottom = wSpill->BottomIndex();
                    wLeft = wSpill->LeftIndex();
                    wRight = wSpill->RightIndex();
                } else {
                    tTempoRect wOut = wOrigin->ArrayFormulaOutputRect();
                    if (!wOut.IsValid()) {
                        return true;
                    }
                    wTop = wOut.Top();
                    wBottom = wOut.Bottom();
                    wLeft = wOut.Left();
                    wRight = wOut.Right();
                }
                if (wTop > wBottom || wLeft > wRight) {
                    return true;
                }
                for (tIndex wRow = wTop; wRow <= wBottom; ++wRow) {
                    for (tIndex wCol = wLeft; wCol <= wRight; ++wCol) {
                        if (wRow == wOrigin->RowIndex() && wCol == wOrigin->ColIndex()) {
                            continue;
                        }
                        tCell* wSlave = m_ColRowCellRange->Cell(wRow, wCol);
                        if (wSlave == nullptr || wSlave->Formula() != nullptr) {
                            continue;
                        }
                        wSlave->RemoveMatExtend();
                        wSlave->Value(tVariant());
                    }
                }
                return true;
            }
        };
        auto wClearSheet = [&](tSheet* sSheet) {
            if (sSheet == nullptr) {
                return;
            }
            tColRowCellRange* wCr = sSheet->ColRowCellRange();
            tCallBackClearMatExtendValues wClearMatExtend(wCr);
            wCr->CallBackAllCell(&wClearMatExtend);
            tCallBackClearSpillSlaves wClearSpill(wCr);
            wCr->CallBackAllCell(&wClearSpill);
        };
        for (auto wSheetRef : m_VectorSheet) {
            wClearSheet(m_SheetAllocator(wSheetRef));
        }
        if (m_SheetNamedFormulaRef != 0) {
            wClearSheet(m_SheetAllocator(m_SheetNamedFormulaRef));
        }
        if (m_SheetClassAnchor != 0) {
            wClearSheet(m_SheetAllocator(m_SheetClassAnchor));
        }
    }

	tVectorAllocatorRef* tWorkBook::VectorSheet() { return(&m_VectorSheet); }

	tSheet* tWorkBook::Sheet(tInt sIndex) {
        if ((sIndex>=0) && (sIndex<m_VectorSheet.size())) {
            return(m_SheetAllocator(m_VectorSheet[sIndex]));
        }
        return(nullptr);
    }

   tVectorSheet tWorkBook::VectorPtSheet() {
       tVectorSheet wResult;
        tVectorAllocatorRef* wVectorSheet = VectorSheet();
        for (auto wIndex : *wVectorSheet) {
            tSheet* wSheet = SheetByAllocator(wIndex);
            wResult.push_back(wSheet);
        }
        return(wResult);
    }

	// Interface Cell ====================================================
	tBool tWorkBook::CompilCell(tCell* sCell, const tChar* sValue) {
#ifdef _DEBUGSK
        if (sCell->StrRef()=="B15") {
        }
#endif
        if (m_LemonInterface==nullptr) {
            m_LemonInterface = tSpreadSheetContainer::Instance()->LemonInterface();
        }
        // if Volatile delete in colrange
        tBool wBeforeCellVolatile=false;
        const tFormula* wFormula=sCell->Formula();
        if (wFormula!=nullptr) 
        if (!wFormula->BitSetVolatile().Empty()) {
            wBeforeCellVolatile=true;
        }
        
        tBool wResult = m_LemonInterface->Compil(this,sCell, sValue);
        if (wResult) {
            wFormula=sCell->Formula();
            if (wFormula!=nullptr) {
                if (!wFormula->BitSetVolatile().Empty()) {
                    if (!wBeforeCellVolatile) {
                        sCell->ColRowCellRange()->AddVolatile(sCell);
                    }
                } else {
                    if (wBeforeCellVolatile) {
                        sCell->ColRowCellRange()->DeleteVolatile(sCell);
                    }
                }
            } else {
                // Formula became nullptr (formula was cleared), remove from volatile if it was volatile
                if (wBeforeCellVolatile) {
                    sCell->ColRowCellRange()->DeleteVolatile(sCell);
                }
            }
            //sCell->ColRowCellRange()->AddVolatile(wAllocatorRef);
        }
        // Set Path to 0 for calculate
		sCell->Path(0);
		return(wResult);
	}

	tBool tWorkBook::CellValue(tCell* sCell, tVariant& sVariant, tBool sCalculate) {
        tString wHeaderError;
        if (!tRangeData::ValidateHeaderCellValue(this, sCell->Sheet(), sCell->RowIndex(), sCell->ColIndex(),
                                                 sVariant, &wHeaderError)) {
            if (m_LemonInterface == nullptr) {
                m_LemonInterface = tSpreadSheetContainer::Instance()->LemonInterface();
            }
            if (m_LemonInterface != nullptr) {
                m_LemonInterface->Error(wHeaderError, 0, 0);
            }
            return false;
        }
        // If Variant is a formula
        tString wFormula=sVariant.Formula();
        if (wFormula!="") {
            tBool wResult = CompilCell(sCell, wFormula.c_str());
            if (wResult) {
                sCell->Value(tVariant()); //RAZ
                if (sCalculate) sCell->Calculation();
            }
            return(wResult);
        }
        // If old formula clear
        if (sCell->Formula()!=nullptr) {
            sCell->ClearFormula();
        }
        // If Variant is a value
        sCell->Value(sVariant);
        tSheet* wSheet = sCell->Sheet();
        tString wTableName;
        tRange* wTableRange = nullptr;
        tie(wTableName, wTableRange) = wSheet->FindRangeDataCovered(sCell->RowIndex(), sCell->ColIndex());
        if (wTableRange != nullptr && wTableRange->IsData() && sCell->RowIndex() == wTableRange->TopIndex()) {
            tRangeData* wRangeData = RangeData(wTableName);
            if (wRangeData != nullptr && wRangeData->HasHeaders()) {
                wRangeData->CaptureHeaderLabelFromCell(wSheet, wTableRange, sCell->ColIndex(), sVariant.Str());
            }
        }
        // If Calculate is true calculate the cell
		if (sCalculate) sCell->Calculation();
		return(true);
	}
 
    tLemonInterface* tWorkBook::LemonInterface() {
        return(m_LemonInterface);
    }
 

	// Named Range ============================================================
	tRangeNamedContainer* tWorkBook::RangeNamedContainer() {
		return(&m_RangeNamedContainer);
	}

    tFloatingObjectContainer* tWorkBook::FloatingObjectContainer() {
        return (&m_FloatingObjectContainer);
    }
	
	
	tRange* tWorkBook::InsertRangeNamed(tString sName, tTempoRect sRect, tSheet* sSheet) {
		tRange* wRange=sSheet->EnsureRange(sRect.Row(), sRect.Left(), sRect.Bottom(), sRect.Right());
		wRange->SetNamed();
        // Symmetric with tRangeNamedContainer::DeleteRangeNamedByName: for a
        // single-cell named range, the compile path in
        // tLemonInterface::PushRef pushes the cell (not the range) and
        // relies on tCell::IsNamed() so tFormula::ClassOrRangeStr can
        // promote the cell reference back to the range and render the
        // name. DeleteRangeNamedByName clears that flag on the cell; if we
        // do not re-set it here, a subsequent InsertRangeNamed after a
        // rename leaves the formula rendering raw coordinates ("L1")
        // instead of the new name.
        if (wRange->IsCell()) {
            wRange->EnsureCell()->SetNamed();
        }
        m_RangeNamedContainer.InsertRangeNamed(sName, sSheet->AllocatorRef(),wRange->AllocatorRef());
		return(wRange);
	}
    

  
    tBool tWorkBook::InsertNamedFormula(tString sName,tString sFormula) {
        // Compile and calculate immediately so named array formulas have MatrixRange() for consumers (Excel-like).
        return(m_RangeNamedContainer.ApplyFormulaNamed(sName,sFormula,true,0));
    }

    tFormulaNamed* tWorkBook::FindFormulaNamed(tString sName) {
        return(m_RangeNamedContainer.FormulaNamed(sName));
    }

    tFormulaNamed* tWorkBook::FindFormulaNamedByCell(tCell* sCell) {
        return(m_RangeNamedContainer.FormulaNamedByCell(sCell));
    }

    tFormulaNamed* tWorkBook::FindFormulaNamedBySpillRange(tRange* sRange) {
        return(m_RangeNamedContainer.FormulaNamedBySpillRange(sRange));
    }

    void tWorkBook::PushNamedFormulaCaller(tCell* sCaller) {
        m_NamedFormulaCallerStack.push_back(sCaller);
    }

    void tWorkBook::PopNamedFormulaCaller() {
        if (!m_NamedFormulaCallerStack.empty()) {
            m_NamedFormulaCallerStack.pop_back();
        }
    }

    tCell* tWorkBook::NamedFormulaCaller() const {
        if (m_NamedFormulaCallerStack.empty()) {
            return nullptr;
        }
        return m_NamedFormulaCallerStack.back();
    }

    void tWorkBook::ClearNamedCallerEvalCache() {
        m_NamedCallerEvalCache.clear();
    }

    tBool tWorkBook::TryNamedCallerEval(const tFormulaNamed* sFn, tIndex sRow, tIndex sCol,
                                        tVariant& oOut) const {
        tNamedCallerEvalKey wKey;
        wKey.m_Fn = sFn;
        wKey.m_Row = sRow;
        wKey.m_Col = sCol;
        auto wIt = m_NamedCallerEvalCache.find(wKey);
        if (wIt == m_NamedCallerEvalCache.end()) {
            return false;
        }
        oOut = wIt->second;
        return true;
    }

    void tWorkBook::StoreNamedCallerEval(const tFormulaNamed* sFn, tIndex sRow, tIndex sCol,
                                         const tVariant& sValue) {
        tNamedCallerEvalKey wKey;
        wKey.m_Fn = sFn;
        wKey.m_Row = sRow;
        wKey.m_Col = sCol;
        m_NamedCallerEvalCache[wKey] = sValue;
    }

    tBool tWorkBook::TryNamedFormulaSpillBufferAt(tIndex sRow, tIndex sCol, tVariant& out,
                                                  tIndex sOperandRangeTop, tIndex sOperandRangeLeft,
                                                  tIndex sOperandRangeHeight, tIndex sOperandRangeWidth) {
        return m_RangeNamedContainer.TryFormulaNamedSpillBufferAt(
            sRow, sCol, out, sOperandRangeTop, sOperandRangeLeft, sOperandRangeHeight, sOperandRangeWidth);
    }

    tRange* tWorkBook::InsertRangeData(tString sName, tRangeData sRangeData, tTempoRect sRect, tSheet* sSheet) {
        tRange* wRange=InsertRangeNamed(sName,sRect,sSheet);
        wRange->SetData();
        sRangeData.SyncFromRange(sSheet, wRange);
        m_RangeNamedContainer.InsertRangeData(sName,sRangeData,sSheet->AllocatorRef(),wRange->AllocatorRef());
        // sFormula already applied in InsertRangeNamed to range cell when non-empty
        return(wRange);
    }

    tRange* tWorkBook::ApplyRangeData(tString sName,tRangeData sRangeData) {
        return(m_RangeNamedContainer.ApplyRangeData(sName,sRangeData));
    }

    void tWorkBook::ReplaceRangeDataMetadata(tString sName, tRangeData sRangeData) {
        m_RangeNamedContainer.ReplaceRangeDataMetadata(sName, sRangeData);
    }

    void tWorkBook::SyncRangeDataAfterColumnInsert(tRange* sRange, tIndex sInsertCol, tIndex sWidth) {
        if (sRange == nullptr || !sRange->IsData() || sWidth <= 0) {
            return;
        }
        tSheet* wSheet = sRange->Sheet();
        if (wSheet == nullptr) {
            return;
        }
        const tString wName = FindRangeNamed(sRange->AllocatorRef(), wSheet);
        if (wName.empty()) {
            return;
        }
        tRangeData* wExisting = RangeData(wName);
        if (wExisting == nullptr) {
            return;
        }
        tRangeData wUpdated(wExisting);
        wUpdated.ShiftSheetCols(sInsertCol, sWidth);
        wUpdated.SyncFromRange(wSheet, sRange);
        ReplaceRangeDataMetadata(wName, wUpdated);
    }

    namespace {

    tBool CellBlankForCalculatedColumnFill(tCell* sCell) {
        if (sCell == nullptr) {
            return true;
        }
        if (sCell->Formula() != nullptr) {
            return false;
        }
        const tVariant& wValue = sCell->Value();
        if (wValue.IsNull() || wValue.IsExcelNull()) {
            return true;
        }
        return wValue.IsString() && wValue.String().empty();
    }

    tString FormulaTextForCalculatedColumnFill(tSheet* sSheet, tIndex sCol,
                                               tIndex sDataTop, tIndex sDataBottom,
                                               tIndex sInsertedFirst, tIndex sInsertedLast,
                                               const tString& sCalculatedColumnFormula) {
        // Prefer an already-compiled data cell (locale-correct structured refs).
        tString wFromCell;
        tString wAbove;
        for (tIndex wRow = sDataTop; wRow <= sDataBottom; ++wRow) {
            if (wRow >= sInsertedFirst && wRow <= sInsertedLast) {
                continue;
            }
            tCell* wSrc = sSheet->Cell(wRow, sCol);
            if (wSrc == nullptr || wSrc->Formula() == nullptr) {
                continue;
            }
            const tString wWire = wSrc->FormulaWire();
            if (wWire.empty()) {
                continue;
            }
            wFromCell = wWire;
            if (wRow < sInsertedFirst) {
                wAbove = wWire;
            }
        }
        if (!wAbove.empty()) {
            return wAbove.front() == '=' ? wAbove : ("=" + wAbove);
        }
        if (!wFromCell.empty()) {
            return wFromCell.front() == '=' ? wFromCell : ("=" + wFromCell);
        }
        // Fallback: OOXML / JSON calculatedColumnFormula template.
        if (sCalculatedColumnFormula.empty()) {
            return "";
        }
        return sCalculatedColumnFormula.front() == '='
            ? sCalculatedColumnFormula
            : ("=" + sCalculatedColumnFormula);
    }

    } // namespace

    void tWorkBook::ApplyCalculatedColumnFormulasToInsertedRows(tSheet* sSheet,
                                                                tIndex sFirstRow, tIndex sLastRow,
                                                                tIndex sLeftCol, tIndex sRightCol) {
        if (sSheet == nullptr || sFirstRow == 0 || sLastRow < sFirstRow) {
            return;
        }
        tContainerPath wContainerPath;
        tBool wBegan = false;
        const std::vector<tString> wNames = m_RangeNamedContainer.AllNames();
        for (const tString& wName : wNames) {
            tRange* wRange = FindRangeNamed(wName);
            if (wRange == nullptr || !wRange->IsData() || wRange->Sheet() != sSheet) {
                continue;
            }
            tRangeData* wRangeData = RangeData(wName);
            if (wRangeData == nullptr || wRangeData->IsEmpty()) {
                continue;
            }
            tIndex wDataTop = wRange->TopIndex();
            if (wRangeData->HasHeaders()) {
                ++wDataTop;
            }
            const tIndex wDataBottom = wRange->BottomIndex();
            if (wDataTop > wDataBottom) {
                continue;
            }
            tIndex wRowFrom = sFirstRow > wDataTop ? sFirstRow : wDataTop;
            tIndex wRowTo = sLastRow < wDataBottom ? sLastRow : wDataBottom;
            if (wRowFrom > wRowTo) {
                continue;
            }
            tIndex wLeft = wRange->LeftIndex();
            tIndex wRight = wRange->RightIndex();
            if (sLeftCol > 0 && sLeftCol > wLeft) {
                wLeft = sLeftCol;
            }
            if (sRightCol > 0 && sRightCol < wRight) {
                wRight = sRightCol;
            }
            if (wLeft > wRight) {
                continue;
            }
            for (const tColumnData& wCol : wRangeData->Columns()) {
                const tIndex wColIdx = wCol.SheetCol();
                if (wColIdx < wLeft || wColIdx > wRight) {
                    continue;
                }
                const tString wFormula = FormulaTextForCalculatedColumnFill(
                    sSheet, wColIdx, wDataTop, wDataBottom, sFirstRow, sLastRow,
                    wCol.CalculatedColumnFormula());
                if (wFormula.empty()) {
                    continue;
                }
                for (tIndex wRow = wRowFrom; wRow <= wRowTo; ++wRow) {
                    tCell* wCell = sSheet->Cell(wRow, wColIdx);
                    if (!CellBlankForCalculatedColumnFill(wCell)) {
                        continue;
                    }
                    wCell = sSheet->EnsureCell(wRow, wColIdx);
                    if (wCell == nullptr) {
                        continue;
                    }
                    tVariant wValue(wFormula);
                    tBool wOk = false;
                    {
                        // calculatedColumnFormula / FormulaWire are US A1 (OOXML / collab).
                        tLocalePush wUs("us");
                        wOk = CellValue(wCell, wValue, false);
                    }
                    if (!wOk) {
                        continue;
                    }
                    if (!wBegan) {
                        wContainerPath.BeginCalculate();
                        wBegan = true;
                    }
                    wContainerPath.Add(wCell);
                }
            }
        }
        if (wBegan) {
            wContainerPath.EndCalculate();
        }
    }
 
	tBool tWorkBook::DeleteRangeNamed(tString sName) {
		// Multi-area aware: clear the named/data flags on every area, then
		// drop the whole name from the container. Previous single-area code
		// only acted on the first range and only erased the map entry when
		// that range was empty; we now always erase to keep forward/reverse
		// maps in sync.
		//
		// Overlap-aware: if another name still references the same range
		// (e.g. Test1=A1:A2;B1:B2 and Test2=A1:A2;C1:C2), keep its flags
		// intact — only the entries the dying name uniquely owns are reset.
		std::vector<tRange*> wRanges = m_RangeNamedContainer.Ranges(sName);
		if (wRanges.empty()) {
			return(false);
		}
		for (tRange* wRange : wRanges) {
			if (wRange == nullptr) continue;
			tAllocatorRef wSheetRef = wRange->Sheet()->AllocatorRef();
			tAllocatorRef wRangeRef = wRange->AllocatorRef();
			if (m_RangeNamedContainer.IsRangeSharedByOtherName(sName, wSheetRef, wRangeRef)) {
				continue;
			}
			wRange->RemoveNamed();
			wRange->RemoveData();
		}
		return(m_RangeNamedContainer.DeleteRangeNamedByName(sName));
	}

	tString tWorkBook::FindRangeNamed(tAllocatorRef sAllocatorRef, tSheet* sSheet) {
		return(m_RangeNamedContainer.RangeByRef(sSheet->AllocatorRef(), sAllocatorRef));
	}

	tRange* tWorkBook::FindRangeNamed(tString sName) {
		return(m_RangeNamedContainer.Range(sName));
	}
    
    tRangeData* tWorkBook::RangeData(tString sName) {
        return(m_RangeNamedContainer.RangeData(sName));
    }

    tColumnData* tWorkBook::RangeDataColumn(tString sName, tString sColumnName) {
        tRangeData* wRangeData = RangeData(sName);
        tRange* wRange = FindRangeNamed(sName);
        if (wRangeData != nullptr && wRange != nullptr) {
            return(wRangeData->FindColumnByName(wRange->Sheet(), wRange, sColumnName));
        }
        return(nullptr);
    }
 
    tSheet* tWorkBook::SheetNamedFormula() {
        if (m_SheetNamedFormulaRef==0) {
            tSheet* wSheet;
            tie(m_SheetNamedFormulaRef, wSheet) = m_SheetAllocator.Alloc();
            wSheet->Set(m_SheetNamedFormulaRef, AllocatorRef(),CstSheetNamed);
            return(wSheet);
        } else {
            return(m_SheetAllocator(m_SheetNamedFormulaRef));
        }
    }

    tSheet* tWorkBook::SheetClassAnchor() {
        if (m_SheetClassAnchor==0) {
            tSheet* wSheet;
            tie(m_SheetClassAnchor, wSheet) = m_SheetAllocator.Alloc();
            wSheet->Set(m_SheetClassAnchor, AllocatorRef(), CstSheetClassAnchor);
            return(wSheet);
        }
        return(m_SheetAllocator(m_SheetClassAnchor));
    }

    void tWorkBook::JsonSheets(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        sWriter->Key("list");
        sWriter->StartArray();
        for (auto wSheetRef : m_VectorSheet) {
            sWriter->String(m_SheetAllocator(wSheetRef)->Name().c_str());
        }
        sWriter->EndArray();
        sWriter->EndObject();
    }

    void tWorkBook::JsonInfo(Writer<StringBuffer>* sWriter) {
        m_WorkBookInfo.Json(sWriter);
    }

    tString tWorkBook::JsonSheets() {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        JsonSheets(&wWriter);
        return(wStringBuffer.GetString());
    }


	// Json ===================================================================
	void tWorkBook::Json(Writer<StringBuffer>* sWriter) {
        tSpreadSheetContainer::Instance()->JsonBegin();
        if (FormatApi()!=nullptr) {
            // Init save Format
            FormatApi()->BeginWriteJson();
        }
        
		sWriter->StartObject();
		sWriter->Key("id"); sWriter->Int64(AllocatorRef());
		sWriter->Key("uri"); sWriter->String(m_Uri.c_str());

        // Identification ==============================================================
        sWriter->Key("info");
        sWriter->StartObject();
        m_WorkBookInfo.Json(sWriter);
        sWriter->EndObject();

        std::set<tString> wUsedCellClassNames;
        CollectUsedCellClassNames(this, wUsedCellClassNames);
        if (!wUsedCellClassNames.empty()) {
            JsonWorkBookCellClassModels(sWriter, wUsedCellClassNames);
        }

        sWriter->SetMaxDecimalPlaces(3); // limit double precision to 3 decimals
        
		sWriter->Key("sizecol"); sWriter->Double(m_DefaultSizeCol);
		sWriter->Key("sizerow"); sWriter->Double(m_DefaultSizeRow);
		sWriter->Key("defaultfontname"); sWriter->String(m_DefaultFontName.c_str());
		sWriter->Key("defaultfontsize"); sWriter->Double(m_DefaultFontSize);
        // Reset to default behavior
		sWriter->SetMaxDecimalPlaces(rapidjson::Writer<rapidjson::StringBuffer>::kDefaultMaxDecimalPlaces); // reset to default behavior

		sWriter->Key(kJsonKeySheetPrintParameters);
		m_PrintParameters.JsonWrite(sWriter, tPrintJsonScope::WorkBook);

        // Host cell classes live in _$$A sheets[]; sync registry rows before serializing sheets.
        m_FloatingObjectContainer.EnsureAllHostCellClasses();
   
		sWriter->Key("sheets");
		sWriter->StartArray();
		for (auto wSheetIndex : m_VectorSheet) {
            sWriter->StartObject();
			tSheet* wSheet = m_SheetAllocator(wSheetIndex);
            wSheet->Json(sWriter);
            sWriter->EndObject();
		}
        if (m_SheetClassAnchor != 0) {
            sWriter->StartObject();
            m_SheetAllocator(m_SheetClassAnchor)->Json(sWriter);
            sWriter->EndObject();
        }
        sWriter->EndArray();
        
        // Write Named Range
        m_RangeNamedContainer.Json(sWriter);
        m_RangeNamedContainer.JsonFormulaNamed(sWriter);
        m_FloatingObjectContainer.JsonFloatingObjects(sWriter);
        
        // Write Shared String
        tSpreadSheetContainer::Instance()->JsonShared(sWriter);
        
        // Save Format at end of Json
        if (FormatApi()!=nullptr) {
            // Save Format
            sWriter->Key("f");
            FormatApi()->Json(sWriter);
        }
        
		sWriter->EndObject();
	}

	void tWorkBook::Json(const Value& sValue) {
		m_Uri = sValue["uri"].GetString();

        InstallCellClassModelStubHandler();

        // Identification ==============================================================
        if (sValue.HasMember("info")) {
            m_WorkBookInfo.Json(sValue["info"]);
        }

        if (sValue.HasMember(kJsonKeyModels)) {
            RegisterCellClassModelsFromJson(sValue[kJsonKeyModels]);
        }
        RegisterMissingCellClassModelsFromWorkbookJson(sValue);

        // Load Shared String
        tSpreadSheetContainer::Instance()->JsonShared(sValue);
    
        // Sheets ==============================================================
		m_DefaultSizeCol = sValue["sizecol"].GetDouble();
		m_DefaultSizeRow = sValue["sizerow"].GetDouble();
		if (sValue.HasMember("defaultfontname") && sValue["defaultfontname"].IsString()) {
			m_DefaultFontName = sValue["defaultfontname"].GetString();
		} else {
			m_DefaultFontName = "Calibri";
		}
		if (sValue.HasMember("defaultfontsize") && sValue["defaultfontsize"].IsNumber()) {
			m_DefaultFontSize = sValue["defaultfontsize"].GetDouble();
		} else {
			m_DefaultFontSize = 11.0;
		}

		if (sValue.HasMember(kJsonKeySheetPrintParameters) && sValue[kJsonKeySheetPrintParameters].IsObject()) {
			if (m_PrintParameters.JsonRead(sValue[kJsonKeySheetPrintParameters])) {
				m_PrintParameters.ClearSheetFields();
			}
		}
        
        // Fist operation : Load Format
        if (sValue.HasMember("f")) {
            const Value& wFormats= sValue["f"];
            if (FormatApi()!=nullptr) {
                FormatApi()->Json(wFormats);
            }
        }
        const Value& wSheets = sValue["sheets"];
		assert(wSheets.IsArray());
		for (SizeType wIndex = 0; wIndex < wSheets.Size(); wIndex++) {
            tString wSheetName = wSheets[wIndex]["name"].GetString();
            tSheet* wSheet = nullptr;
            if (wSheetName == CstSheetClassAnchor) {
                wSheet = SheetClassAnchor();
            } else if (wSheetName == CstSheetNamed) {
                wSheet = SheetNamedFormula();
            } else {
                wSheet = AddSheet(wSheetName);
            }
            if (wIndex == 0 && wSheet != nullptr) {
                ActiveSheet(wSheet->Name());
            }
        }
        // Read Named Range before for formula
        m_RangeNamedContainer.Json(sValue);
        m_RangeNamedContainer.JsonFormulaNamed(sValue);
        
        // Load sheet content; ActiveSheet() per entry must not leave _$$A active at the end.
        tString wActiveSheetName;
        if (tSheet* wPrevActive = ActiveSheet(); wPrevActive != nullptr) {
            wActiveSheetName = wPrevActive->Name();
        }
		for (SizeType wIndex = 0; wIndex < wSheets.Size(); wIndex++) {
            tString wSheetName = wSheets[wIndex]["name"].GetString();
            tSheet* wSheet = Sheet(wSheetName);
            if (wSheet == nullptr && wSheetName == CstSheetClassAnchor) {
                wSheet = SheetClassAnchor();
            } else if (wSheet == nullptr && wSheetName == CstSheetNamed) {
                wSheet = SheetNamedFormula();
            }
            if (wSheet == nullptr) {
                continue;
            }
            ActiveSheet(wSheetName);
            // Load change Name
            wSheet->Json(wSheets[wIndex]);
 		}
            
        if (!wActiveSheetName.empty()) {
            ActiveSheet(wActiveSheetName);
        }

        m_FloatingObjectContainer.JsonFloatingObjects(sValue);
        
#ifdef _DEBUGSK
#ifdef debugrangenamed
        cout << Debug() << endl;
#endif
#endif
	}
 
	tString tWorkBook::WriteJson() {
		StringBuffer wStringBuffer;
		Writer<StringBuffer> wWriter(wStringBuffer);
		Json(&wWriter);
		return(wStringBuffer.GetString());
	}

	void tWorkBook::ReadJson(tString sJson) {
        // Set Us Lang
        tLocale* wLocale=tApplication::Instance()->Locale();
        tString wLang=wLocale->Lang();
        wLocale->Lang("us");
 
		Document wDocument;
		wDocument.Parse(sJson.c_str());
		Json(wDocument);
  
        // Recup Lang
        wLocale->Lang(wLang);
	};

	tString tWorkBook::JsonRangeNamed() {
        tExtension wFilter;
        wFilter.Set(t_Named);
		StringBuffer wStringBuffer;
		Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
		m_RangeNamedContainer.Json(&wWriter, wFilter);
        wWriter.EndObject();
		return(wStringBuffer.GetString());
	}

    tString tWorkBook::JsonFormulaNamed() {
    	StringBuffer wStringBuffer;
		Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        m_RangeNamedContainer.JsonFormulaNamed(&wWriter);
        wWriter.EndObject();
		return(wStringBuffer.GetString());
    }

    tString tWorkBook::JsonRangeData() {
        tExtension wFilter;
        wFilter.Set(t_Data);
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        m_RangeNamedContainer.Json(&wWriter, wFilter);
        wWriter.EndObject();
        return wStringBuffer.GetString();
    }
    
    tString tWorkBook::JsonConditionalFormats() {
        tString wJsonConditionalFormat;
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        for (auto wSheetIndex : m_VectorSheet) {
            tSheet* wSheet = m_SheetAllocator(wSheetIndex);
            if (wSheet != nullptr) {
                wWriter.Key(kJsonKeySheet);
                wWriter.String(wSheet->Name().c_str());
                wWriter.Key(kJsonKeyConditionalFormats);
                tConditionalFormatContainer* wConditionalFormatContainer = wSheet->ColRowCellRange()->ConditionalFormatContainer();
                if (wConditionalFormatContainer != nullptr) {
                    wConditionalFormatContainer->Json(&wWriter);
                }
            }
        }
        wWriter.EndObject();
        return(wStringBuffer.GetString());
    }

	void tWorkBook::GetRangeNamedsBySheet(tAllocatorRef sSheetAllocatorRef, tVectorAllocatorRef& sRangesAllocatorRef) {
		m_RangeNamedContainer.GetRangeNamedsBySheet(sSheetAllocatorRef, sRangesAllocatorRef);
	}

	tDouble tWorkBook::DefaultSizeCol() { return(m_DefaultSizeCol); }
	void tWorkBook::DefaultSizeCol(tDouble sValue) { m_DefaultSizeCol = sValue; }

	tDouble tWorkBook::DefaultSizeRow() { return(m_DefaultSizeRow); }
	void tWorkBook::DefaultSizeRow(tDouble sValue) { m_DefaultSizeRow = sValue; }

	tString tWorkBook::DefaultFontName() const { return m_DefaultFontName; }
	void tWorkBook::DefaultFontName(tString sValue) { m_DefaultFontName = std::move(sValue); }

	tDouble tWorkBook::DefaultFontSize() const { return m_DefaultFontSize; }
	void tWorkBook::DefaultFontSize(tDouble sValue) { m_DefaultFontSize = sValue; }

	tPrintParameters& tWorkBook::PrintParameters() { return m_PrintParameters; }
	const tPrintParameters& tWorkBook::PrintParameters() const { return m_PrintParameters; }

	void tWorkBook::AdoptWorkBookPrintFieldsIfDefault(const tPrintParameters& sSrc)
	{
		if (m_PrintParameters.WorkBookFieldsAreDefault()) {
			m_PrintParameters.CopyWorkBookFieldsFrom(sSrc);
		}
	}

	tBool tWorkBook::operator < (tWorkBook& sWorkBook) {
		return(m_Uri < sWorkBook.m_Uri);
	}

	tBool tWorkBook::operator == (tWorkBook& sWorkBook) {
		return(m_Uri == sWorkBook.m_Uri);
	}

#ifdef checksp
	void tWorkBook::Check() {
        for (auto wSheetIndex : m_VectorSheet) {
			tSheet* wSheet = m_SheetAllocator(wSheetIndex);
			wSheet->Check();
		}
	}
#endif
#ifdef checkfo
    void tWorkBook::CheckFormat() {
        m_TableStyleContainer.CheckFormat();
        // Workbook-owned JsonView overlay (ApplyCellFormat, not on a cell).
        if (m_JsonViewGeneralHAlignRef != 0 && FormatApi() != nullptr) {
            FormatApi()->IncCheck(m_JsonViewGeneralHAlignRef);
        }
        for (auto wSheetIndex : m_VectorSheet) {
            tSheet* wSheet = m_SheetAllocator(wSheetIndex);
            wSheet->CheckFormat();
        }
    }
#endif
#ifdef _DEBUGSK
    tString tWorkBook::Debug() {
        tStringStream wStream;
        wStream << "WorkBook " << m_Uri << "---------------" << endl;
        for (auto wSheetIndex : m_VectorSheet) {
            tSheet* wSheet = m_SheetAllocator(wSheetIndex);
            wStream << wSheet->Name() << endl;
        }
        wStream << m_RangeNamedContainer.Debug();
        return(wStream.str());
    }

    tString tWorkBook::DebugFormatCell(tCell* sCell) {
        tFormatApi* wFormatApi=FormatApi();
        tStringStream wStream;
        
        if (wFormatApi!=nullptr) {
            wStream << "Format " << sCell->StrRef() << " ";
            if (sCell->Css()!=0) {
                wStream << wFormatApi->Debug(sCell->Css());
            } else {
                wStream << "..";
            }
        }
        return(wStream.str());
    }
#endif


} // End of namespace

#include "../include/SkTools.hpp"
#include "../include/SkSelect.hpp"
#include "../include/SkWorkBook.hpp"

#include "../include/SkUndoRedoJsonCallBack.hpp"
#include "../include/SkSpreadSheet.hpp"

namespace SkSpreadSheet {

    tUndoRedoJsonCallBack::tUndoRedoJsonCallBack() {
    }

    tUndoRedoJsonCallBack::~tUndoRedoJsonCallBack() {
    }


    tBool tUndoRedoJsonCallBack::CallBackCell(tTempoPoint* sPoint) {
        tCell* wCell = m_Sheet->Cell(sPoint->Row(), sPoint->Col());
        if (wCell != nullptr) {
            if (!wCell->IsEmpty()) {
    #ifdef debugcopypaste
                cout << "." << wCell->StrRef();
                tString wFormulaStr=wCell->FormulaStr();
                if (wFormulaStr!="") cout << "=" << wFormulaStr;
                cout << wCell->Value() << endl;
    #endif
                m_Writer->StartObject();
                wCell->Json(m_Writer, false);
                m_Writer->EndObject();
            }
        }
        return(true);
    }

    tBool tUndoRedoJsonCallBack::CallBackRange(tTempoRect* sRect) {
        for (tIndex wRow = sRect->Row(); wRow <= sRect->Bottom(); wRow++) {
            for (tIndex wCol = sRect->Col(); wCol <= sRect->Right(); wCol++) {
                tTempoPoint wPoint(wRow, wCol);
                if (!CallBackCell(&wPoint)) return(false);
            }
        }
        return(true);
    }

    tString tUndoRedoJsonCallBack::WriteCell(tString sRef, tSheet* sSheet) {
        tBool wResult = true;
        tString wResultStr="";
        m_Sheet = sSheet;
        // Get Format Api
        tFormatApi* wFormatApi =m_Sheet->WorkBook()->FormatApi();
        if (wFormatApi!=nullptr) {
            // Init save Format
            wFormatApi->BeginWriteJson();
        }
       
        // Important Select use temporary memory
        // We have to parse again every time
        tSelect wSelect;
        if (wSelect.Parse(sRef)) {
            StringBuffer wJsonBuffer;
            m_Writer=new Writer<StringBuffer>(wJsonBuffer);
            // Write Select for Copy Multiple
            m_Writer->StartObject();
            m_Writer->Key("selects");
            m_Writer->StartArray();  
            // For each select (Cell or Rect)
            for(auto wElem : *wSelect.VectorSelect()) {
                m_Writer->StartObject();
                m_Writer->Key("select");
                tTempoRect* wTempoRect=dynamic_cast<tTempoRect*>(wElem);
                if (wTempoRect!=nullptr) {
                    wTempoRect->Json(m_Writer);
                } else {
                    wElem->Json(m_Writer);
                }
                m_Writer->Key("cells");
                m_Writer->StartArray();
                if (wTempoRect!=nullptr) {
                    wTempoRect->CallBack(this);
                } else {
                    wElem->Json(m_Writer);
                    wElem->CallBack(this);
                }
                m_Writer->EndArray();
                m_Writer->EndObject();
            }
            m_Writer->EndArray();
            
            if (wFormatApi!=nullptr) {
                // save Format
                m_Writer->Key("f");
                wFormatApi->Json(m_Writer);
            }
        
            m_Writer->EndObject();
         
        
            if (wResult) {
    #ifdef debugcopypaste
                tString wJson = wJsonBuffer.GetString();
                cout << wJson << endl;
    #endif
                wResultStr=wJsonBuffer.GetString();
            }
            delete(m_Writer);
        };
        return(wResultStr);
    }

    tBool tUndoRedoJsonCallBack::ReadCell(tString sJson) {
        tBool wResult = false;
        
        // Parse JSON document
        Document wDocument;
        rapidjson::ParseResult wParseResult = wDocument.Parse(sJson.c_str());
        if (!wParseResult) {
            return(false);
        }
        
        // Get Format Api
        tFormatApi* wFormatApi = m_Sheet->WorkBook()->FormatApi();
        if (wFormatApi != nullptr) {
            // Load formats if present
            if (wDocument.HasMember("f")) {
                const rapidjson::Value& wFormats = wDocument["f"];
                wFormatApi->Json(wFormats);
            }
        }
        
        // Get ColRowCellRange for cell operations
        tColRowCellRange* wColRowCellRange = m_Sheet->ColRowCellRange();
        
        // Process selects array
        if (wDocument.HasMember("selects")) {
            const rapidjson::Value& wSelects = wDocument["selects"];
            if (wSelects.IsArray()) {
                for (SizeType wSelectIndex = 0; wSelectIndex < wSelects.Size(); wSelectIndex++) {
                    const rapidjson::Value& wSelect = wSelects[wSelectIndex];
                    
                    // Check if wSelect is an object and has cells member
                    if (wSelect.IsObject() && wSelect.HasMember("cells")) {
                        const rapidjson::Value& wCells = wSelect["cells"];
                        if (wCells.IsArray()) {
                            for (SizeType wCellIndex = 0; wCellIndex < wCells.Size(); wCellIndex++) {
                                const rapidjson::Value& wJsonCell = wCells[wCellIndex];
                                
                                // Check if cell data is valid
                                if (wJsonCell.IsObject() && wJsonCell.HasMember("c")) {
                                    tTempoPoint wPoint(wJsonCell["c"].GetString());
                                    tIndex wRow = wPoint.Row();
                                    tIndex wCol = wPoint.Col();
                                    
                                    // Ensure cell exists and apply JSON data
                                    tCell* wCell = wColRowCellRange->EnsureCell(wRow, wCol);
                                    if (wCell != nullptr) {
                                        wCell->Json(wJsonCell);
                                        wResult = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        return(wResult);
    }
    tString tUndoRedoJsonCallBack::WriteRange(tString sRef, tSheet* sSheet) {
        tBool wResult = true;
        tString wResultStr="";
        m_Sheet = sSheet;
      
        tSelect wSelect;
        if (wSelect.Parse(sRef)) {
            StringBuffer wJsonBuffer;
            m_Writer=new Writer<StringBuffer>(wJsonBuffer);
            // Write Select for Copy Multiple
            m_Writer->StartObject();
            m_Writer->Key("selects");
            m_Writer->StartArray();
            // For each select (Cell or Rect)
            for(auto wElem : *wSelect.VectorSelect()) {
                tTempoRect* wTempoRect=dynamic_cast<tTempoRect*>(wElem);
                if (wTempoRect!=nullptr) {
                    m_Writer->StartObject();
                    m_Writer->Key("select");
                    wTempoRect->Json(m_Writer);
                    m_Writer->Key("ranges");
                    m_Writer->StartArray();
                    tVectorRange wVectorRange;
                    m_Sheet->ColRowCellRange()->FindRanges(tRect(*wTempoRect),&wVectorRange);
                    for(auto wRange : wVectorRange) {
                        if (wRange->IsMerged() || wRange->IsNamed()) {
                            m_Writer->StartObject();
                            wRange->Json(m_Writer);
                            m_Writer->EndObject();
                        }
                    }
                    m_Writer->EndArray();
                    m_Writer->EndObject();
                }
            }
            m_Writer->EndArray();
            m_Writer->EndObject();
            if (wResult) {
#ifdef debugcopypaste
                tString wJson = wJsonBuffer.GetString();
                cout << wJson << endl;
#endif
                wResultStr=wJsonBuffer.GetString();
            }
            delete(m_Writer);
        }
    
        return(wResultStr);
    }


    tBool tUndoRedoJsonCallBack::ReadRange(tString sJson) {
        tBool wResult = false;
        
        // Parse JSON document
        Document wDocument;
        rapidjson::ParseResult wParseResult = wDocument.Parse(sJson.c_str());
        if (!wParseResult) {
            return(false);
        }
        
        // Get Format Api
        tFormatApi* wFormatApi = m_Sheet->WorkBook()->FormatApi();
        if (wFormatApi != nullptr) {
            // Load formats if present
            if (wDocument.HasMember("f")) {
                const rapidjson::Value& wFormats = wDocument["f"];
                wFormatApi->Json(wFormats);
            }
        }
        
        // Get ColRowCellRange for cell operations
        tColRowCellRange* wColRowCellRange = m_Sheet->ColRowCellRange();
        
        // Process selects array
        if (wDocument.HasMember("selects")) {
            const rapidjson::Value& wSelects = wDocument["selects"];
            if (wSelects.IsArray()) {
                for (SizeType wSelectIndex = 0; wSelectIndex < wSelects.Size(); wSelectIndex++) {
                    const rapidjson::Value& wSelect = wSelects[wSelectIndex];
                    
                    // Check if wSelect is an object and has cells member
                    if (wSelect.IsObject() && wSelect.HasMember("ranges")) {
                        const rapidjson::Value& wRanges = wSelect["ranges"];
                        if (wRanges.IsArray()) {
                            for (SizeType wRangeIndex = 0; wRangeIndex < wRanges.Size(); wRangeIndex++) {
                                const rapidjson::Value& wJsonRange = wRanges[wRangeIndex];
                                if (wJsonRange.HasMember("r")) {
                                    tTempoRect wTempoRect=tTempoRect(wJsonRange["r"].GetString());
                                    tString wType = wJsonRange["t"].GetString();
                                    if (wType == "m") {
                                        tRange* wRange=wColRowCellRange->EnsureRange(wTempoRect.Left(),wTempoRect.Top(),wTempoRect.Right(),wTempoRect.Bottom());
                                        wRange->SetMerged();
                                    }
                                    else if (wType == "n") {
                                        tString wName=wJsonRange["n"].GetString();  
                                        /*tRange* wRange=*/m_Sheet->WorkBook()->InsertRangeNamed(wName,wTempoRect,m_Sheet);
                                    }
                                }
                            } // Loop on ranges
                        } // If ranges is an array
                    } // Loop on selects
                } // Is Select is an object
            }
        } // Is Selects is Has Member Selects
        return(wResult);
    }

} // end of namespace

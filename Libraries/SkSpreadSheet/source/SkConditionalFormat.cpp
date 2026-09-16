//=============================================================================
// SkConditionalFormat.cpp
// Stéphane Allez *
//  Created on: 21 sept. 2025
//=============================================================================

#include "../include/SkConditionalFormat.hpp"

#include "../include/SkSpreadSheet.hpp"
#include "../include/SkLemonInterface.hpp"
#include "../include/SkUndoRedoSaveSp.hpp"
#include "../include/SkCellClassUnit.hpp"
#include <iomanip>
#include <algorithm>
#include <limits>
#include <vector>

namespace SkSpreadSheet {




#define _debugconditionalformat
#define _debugcell

    /// Unwrap tCellUnit magnitude for CF aggregation only; keeps CalculableValue() as full class for formula unit propagation.
    static tVariant CfNumericScanVariant(const tCell* sCell) {
        return tCell::CalculableScalarFromVariant(sCell->CalculableValue());
    }

    //=========================================================================
    //! Utility function to convert ConditionalFormatType to string
    //=========================================================================
    const char* ConditionalFormatTypeToString(tConditionalFormatType type) {
        switch (type) {
            case tConditionalFormatType::t_None:
                return "None";
            case tConditionalFormatType::t_HighlightCellsRules:
                return "HighlightCellsRules";
            case tConditionalFormatType::t_DataBars:
                return "DataBars";
            case tConditionalFormatType::t_ColorScales:
                return "ColorScales";
            case tConditionalFormatType::t_IconSets:
                return "IconSets";
            case tConditionalFormatType::t_CustomFormulas:
                return "CustomFormulas";
            default:
                return "Unknown";
        }
    }

    //=========================================================================
    //! Utility function to convert string to ConditionalFormatType
    //=========================================================================
    tConditionalFormatType StringToConditionalFormatType(const char* typeString) {
        if (typeString == nullptr) {
            return tConditionalFormatType::t_None;
        }
        
        if (strcmp(typeString, "None") == 0) {
            return tConditionalFormatType::t_None;
        } else if (strcmp(typeString, "HighlightCellsRules") == 0) {
            return tConditionalFormatType::t_HighlightCellsRules;
        } else if (strcmp(typeString, "DataBars") == 0) {
            return tConditionalFormatType::t_DataBars;
        } else if (strcmp(typeString, "ColorScales") == 0) {
            return tConditionalFormatType::t_ColorScales;
        } else if (strcmp(typeString, "IconSets") == 0) {
            return tConditionalFormatType::t_IconSets;
        } else if (strcmp(typeString, "CustomFormulas") == 0) {
            return tConditionalFormatType::t_CustomFormulas;
        } else {
            return tConditionalFormatType::t_None; // Default fallback
        }
    }

    //=========================================================================
    //! Utility function to convert IconType to string
    //=========================================================================
    const char* IconTypeToString(tIconType type) {
        switch (type) {
            case tIconType::t_None:
                return "None";
            case tIconType::t_Flags:
                return "Flags";
            case tIconType::t_Arrows:
                return "Arrows";
            case tIconType::t_Shapes:
                return "Shapes";
            case tIconType::t_Indicators:
                return "Indicators";
            case tIconType::t_Ratings:
                return "Ratings";
            default:
                return "Unknown";
        }
    }

    //=========================================================================
    //! Utility function to convert string to IconType
    //=========================================================================
    tIconType StringToIconType(const char* typeString) {
        if (typeString == nullptr) {
            return tIconType::t_None;
        }
        
        if (strcmp(typeString, "None") == 0) {
            return tIconType::t_None;
        } else if (strcmp(typeString, "Flags") == 0) {
            return tIconType::t_Flags;
        } else if (strcmp(typeString, "Arrows") == 0) {
            return tIconType::t_Arrows;
        } else if (strcmp(typeString, "Shapes") == 0) {
            return tIconType::t_Shapes;
        } else if (strcmp(typeString, "Indicators") == 0) {
            return tIconType::t_Indicators;
        } else if (strcmp(typeString, "Ratings") == 0) {
            return tIconType::t_Ratings;
        } else {
            return tIconType::t_None; // Default fallback
        }
    }


    tString  KeyStyleRef(tConditionalFormatType sType,tString sRef)  {
        tStringStream wKey;
        wKey << sRef << ".";
        switch (sType) {
            case tConditionalFormatType::t_None: break;
            case tConditionalFormatType::t_HighlightCellsRules:  wKey << "HC"; break;
            case tConditionalFormatType::t_CustomFormulas:  wKey << "CF"; break;
            case tConditionalFormatType::t_DataBars: wKey << "DB"; break;
            case tConditionalFormatType::t_ColorScales:  wKey << "CS"; break;
            case tConditionalFormatType::t_IconSets: wKey << "IS"; break;
            default:
                break;
        }
        return(wKey.str());
    }

    //=========================================================================
    //! Set Extension for Conditional Format
    //=========================================================================
    void SetConditionalFormat(tConditionalFormatType sType,tItem* sItem) {
        switch (sType) {
            case tConditionalFormatType::t_HighlightCellsRules:
                sItem->SetCFHR();
                break;
            case tConditionalFormatType::t_DataBars:
                sItem->SetCFDB();
                break;
            case tConditionalFormatType::t_ColorScales:
                sItem->SetCFCS();
                break;
            case tConditionalFormatType::t_IconSets:
                sItem->SetCFIS();
                break;
            case tConditionalFormatType::t_CustomFormulas:
                sItem->SetCFCF();
                break;
            default:
                break;
        }
    }

    void RemoveConditionalFormat(tConditionalFormatType sType,tItem* sItem) {
        switch (sType) {
            case tConditionalFormatType::t_HighlightCellsRules:
                sItem->RemoveCFHR();
                break;
            case tConditionalFormatType::t_DataBars:
                sItem->RemoveCFDB();
                break;
            case tConditionalFormatType::t_ColorScales:
                sItem->RemoveCFCS();
                break;
            case tConditionalFormatType::t_IconSets:
                sItem->RemoveCFIS();
                break;
            case tConditionalFormatType::t_CustomFormulas:
                sItem->RemoveCFCF();
                break;
            default:
                break;
        }
    }


    // =========================================================================
    //! ColorScale utility functions ==========================================
    //=========================================================================
    //! Convert hex color string to RGB components
    //=========================================================================
    struct tRGB {
        tInt R, G, B;
        tRGB() : R(0), G(0), B(0) {}
        tRGB(tInt r, tInt g, tInt b) : R(r), G(g), B(b) {}
    };

    tRGB HexToRGB(const tString& hexColor) {
        // Accept both "#RRGGBB" (CSS-style) and "RRGGBB" (Excel/.sker style — ColorScales
        // params come straight from the OOXML file as bare hex strings, e.g. "F8696B").
        // Without the bare-hex branch, every ColorScales cell evaluated to tRGB(0,0,0) and
        // f_bc rendered as "black" in the JsonView (regression seen on Conditional.sker).
        tRGB rgb;
        tSize wOffset = 0;
        if (!hexColor.empty() && hexColor[0] == '#') wOffset = 1;
        if (hexColor.length() >= wOffset + 6) {
            try {
                rgb.R = std::stoi(hexColor.substr(wOffset    , 2), nullptr, 16);
                rgb.G = std::stoi(hexColor.substr(wOffset + 2, 2), nullptr, 16);
                rgb.B = std::stoi(hexColor.substr(wOffset + 4, 2), nullptr, 16);
            } catch (const std::exception&) {
                // Leave rgb at (0,0,0) on malformed input rather than crashing during render.
                rgb = tRGB();
            }
        }
        return rgb;
    }

    tString RGBToHex(const tRGB& rgb) {
        tStringStream stream;
        stream << "#" << std::hex << std::uppercase 
               << std::setfill('0') << std::setw(2) << rgb.R
               << std::setfill('0') << std::setw(2) << rgb.G
               << std::setfill('0') << std::setw(2) << rgb.B;
        return stream.str();
    }

    //=========================================================================
    //! Linear interpolation between two RGB colors
    //=========================================================================
    tRGB InterpolateColor(const tRGB& color1, const tRGB& color2, tDouble factor) {
        // Clamp factor between 0 and 1
        factor = std::max(0.0, std::min(1.0, factor));
        
        tRGB result;
        result.R = static_cast<tInt>(color1.R + (color2.R - color1.R) * factor);
        result.G = static_cast<tInt>(color1.G + (color2.G - color1.G) * factor);
        result.B = static_cast<tInt>(color1.B + (color2.B - color1.B) * factor);
        
        return result;
    }

    //=========================================================================
    //! Calculate percentage position of value between min and max
    //=========================================================================
    tDouble CalculatePercentage(tDouble value, tDouble minValue, tDouble maxValue) {
        if (maxValue == minValue) return 0.5; // Middle if all values are the same
        return (value - minValue) / (maxValue - minValue);
    }


    // =========================================================================
    //! Conditional Key =======================================================
    tConditionalRanges::tConditionalRanges() : tClass(), m_VectorRange(), m_Type(tConditionalFormatType::t_None) {}

    tConditionalRanges::tConditionalRanges(const tConditionalRanges& sConditionalKey) : tClass(sConditionalKey), m_VectorRange(sConditionalKey.m_VectorRange), m_Type(sConditionalKey.m_Type) {}

    tConditionalRanges::~tConditionalRanges() {
        m_VectorRange.clear();
    }


    void tConditionalRanges::SetKeyAndEnsureRange(tConditionalFormatType sType,tString sKey,tSheet* sSheet) {
        // IconSets keys may carry "@3Symbols2" suffix for duplicate ranges in Excel.
        tString wRangeKey = sKey;
        const tSize wAt = sKey.find('@');
        if (wAt != tString::npos) {
            wRangeKey = sKey.substr(0, wAt);
        }
        tSelect wTempoSelect;
        m_Type=sType;
        tBool wOk=wTempoSelect.Parse(wRangeKey);
        if (wOk) {
            m_VectorRange.clear();
            for (auto wItem : *wTempoSelect.VectorSelect()) {
                tTempoRect* wTempoRect = dynamic_cast<tTempoRect*>(wItem);
                if (wTempoRect != nullptr) {
                    tRange* wRange = sSheet->EnsureRange(wTempoRect->Top(),wTempoRect->Left(),wTempoRect->Bottom(),wTempoRect->Right());
                    SetConditionalFormat(sType,wRange);
                    m_VectorRange.push_back(wRange);
                } else {
                    // We don't put cells. But only ranges for better key management.
                    tTempoPoint* wTempoPoint = dynamic_cast<tTempoPoint*>(wItem);
                    if (wTempoPoint != nullptr) {
                        tPoint wPoint(*wTempoPoint);
                        tTempoRect wTempoRect(wPoint.Row(),wPoint.Col(),wPoint.Row(),wPoint.Col());
                        tRange* wRange = sSheet->EnsureRange(wTempoRect.Top(),wTempoRect.Left(),wTempoRect.Bottom(),wTempoRect.Right());
                        SetConditionalFormat(sType,wRange);
  
                        m_VectorRange.push_back(wRange);
                    }
                }
            }
            sort(m_VectorRange.begin(), m_VectorRange.end(), tComparatorRange());
        } else {
            tStringStream wStream;
            wStream << "tConditionalRanges::Keys " << sKey   <<  " error parsing !";
            cerr << wStream.str() << endl;
            throw(tExceptionInternalError(wStream.str()));
        }
    }

    tString tConditionalRanges::Ref() const {
        tStringStream wStream;
        tBool wFirst=true;
        // For the key Take First Range ========================================
       
        for (auto wItem : m_VectorRange) {
            if (!wFirst) { wStream << ";";  } else { wFirst=false; };
            tRange* wRange = wItem->Range();
            if (wRange != nullptr) {
                wStream << wRange->StrRef();
            }
        }
        return(wStream.str());
    }

    tString tConditionalRanges::Key() const {
        return(KeyStyleRef(m_Type,Ref()));
    }

    tVectorRange* tConditionalRanges::VectorRange() {
        return(&m_VectorRange);
    }

    tBool tConditionalRanges::IntersectRect(tRect* sRect) {
        for (auto wItem : m_VectorRange) {
            tRange* wRange = wItem->Range();
            if (wRange != nullptr) {
                if (wRange->IntersectRect(sRect)) return(true);
            }
        }
        return(false);
    }

    tCell* tConditionalRanges::FirstCell() {
        for (auto wItem : m_VectorRange) {
            tRange* wRange = wItem->Range();
            if (wRange != nullptr) {
                tCell* wCell=wRange->Sheet()->EnsureCell(wRange->TopIndex(),wRange->LeftIndex());
                return(wCell);
            }
        }
        return(nullptr);
    }

    tBool tConditionalRanges::RemoveRange(tRange* sRange) {
        for (auto it = m_VectorRange.begin(); it != m_VectorRange.end(); ++it) {
            if (*it == sRange) {
                m_VectorRange.erase(it);
                return(true);
                break;
            }
        }
        return(false);
    }

    tConditionalRanges::operator tString() const {
        return(Ref());
    }

    // =========================================================================
    //! Item Conditionnal Format for each cell 
    tItemCF::tItemCF() : tClass(),
        m_Type(tConditionalFormatType::t_None),
        m_Css(0),
        m_Percent(0.0),
        m_IconType(tIconType::t_None),
        m_IconString(""),
        m_IconIndex(-1),
        m_IconTotal(0),
        m_Color(""),
        m_Direction(""),
        m_Style(""),
        m_MinValue(0.0),
        m_MaxValue(100.0),
        m_Param6(""),
        m_Param7("") {}

    tItemCF::tItemCF(tConditionalFormatType sType, tFormatRef sCss, tDouble sPercent, tIconType sIconType, tString sIconString, 
                     tString sColor, tString sDirection, tString sStyle, tDouble sMinValue, tDouble sMaxValue, 
                     tString sParam6, tString sParam7) : tClass(),
        m_Type(sType), 
        m_Css(sCss),
        m_Percent(sPercent),
        m_IconType(sIconType), 
        m_IconString(sIconString),
        m_IconIndex(-1),
        m_IconTotal(0),
        m_Color(sColor),
        m_Direction(sDirection),
        m_Style(sStyle),
        m_MinValue(sMinValue),
        m_MaxValue(sMaxValue),
        m_Param6(sParam6),
        m_Param7(sParam7) {}

    void tItemCF::Set(tConditionalFormatType sType, tFormatRef sCss, tDouble sPercent, tIconType sIconType, tString sIconString,
                      tString sColor, tString sDirection, tString sStyle, tDouble sMinValue, tDouble sMaxValue,
                      tString sParam6, tString sParam7) {
        m_Type = sType;
        m_Css = sCss;
        m_Percent = sPercent;
        m_IconType = sIconType;
        m_IconString = sIconString;
        m_IconIndex = -1;
        m_IconTotal = 0;
        m_Color = sColor;
        m_Direction = sDirection;
        m_Style = sStyle;
        m_MinValue = sMinValue;
        m_MaxValue = sMaxValue;
        m_Param6 = sParam6;
        m_Param7 = sParam7;
    }
       
    void tItemCF::Clear() {}

    tFormatRef tItemCF::Css() { return(m_Css); }
    
    tDouble tItemCF::Percent() { return(m_Percent); }

    tConditionalFormatType tItemCF::Type() { return(m_Type); }
    tIconType tItemCF::IconType() { return(m_IconType); }

    tString tItemCF::IconString() { return(m_IconString); }

    tInt tItemCF::IconIndex() { return(m_IconIndex); }
    void tItemCF::SetIconIndex(tInt sIndex) { m_IconIndex = sIndex; }

    tInt tItemCF::IconTotal() { return(m_IconTotal); }
    void tItemCF::SetIconTotal(tInt sTotal) { m_IconTotal = sTotal; }

    // DataBars specific getters
    tString tItemCF::Color() { return(m_Color); }
    tString tItemCF::Direction() { return(m_Direction); }
    tString tItemCF::Style() { return(m_Style); }
    tDouble tItemCF::MinValue() { return(m_MinValue); }
    tDouble tItemCF::MaxValue() { return(m_MaxValue); }
    tString tItemCF::Param6() { return(m_Param6); }
    tString tItemCF::Param7() { return(m_Param7); }

    tInt tItemCF::ReturnSortIndex() { 
        switch (m_Type) {
            case tConditionalFormatType::t_HighlightCellsRules:
                return(4);
            case tConditionalFormatType::t_DataBars:
                return(2);
            case tConditionalFormatType::t_ColorScales:
                return(1);
            case tConditionalFormatType::t_IconSets:
                return(0);
            case tConditionalFormatType::t_CustomFormulas:
                return(3);
            default: return(-1);
        }
    }
    
    void tItemCF::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        
        sWriter->Key("type");
        sWriter->String(ConditionalFormatTypeToString(m_Type));
        
        sWriter->Key("css");
        sWriter->Int64(m_Css);
        
        sWriter->Key("percent");
        sWriter->Double(m_Percent);
        
        sWriter->Key("iconType");
        sWriter->String(IconTypeToString(m_IconType));
        
        sWriter->Key("iconString");
        sWriter->String(m_IconString.c_str());
        
        // DataBars specific parameters
        sWriter->Key("color");
        sWriter->String(m_Color.c_str());
        
        sWriter->Key("direction");
        sWriter->String(m_Direction.c_str());
        
        sWriter->Key("style");
        sWriter->String(m_Style.c_str());
        
        sWriter->Key("minValue");
        sWriter->Double(m_MinValue);
        
        sWriter->Key("maxValue");
        sWriter->Double(m_MaxValue);
        
        sWriter->Key("colorNegative");
        sWriter->String(m_Param6.c_str());
        
        sWriter->EndObject();
    }

    void tItemCF::Json(const Value& sValue) {
        if (sValue.HasMember("type") && sValue["type"].IsString()) {
            m_Type = StringToConditionalFormatType(sValue["type"].GetString());
        }
        
        if (sValue.HasMember("css") && sValue["css"].IsInt64()) {
            m_Css = sValue["css"].GetInt();
        }
        
        if (sValue.HasMember("percent") && sValue["percent"].IsDouble()) {
            m_Percent = sValue["percent"].GetDouble();
        }
        
        if (sValue.HasMember("iconType") && sValue["iconType"].IsString()) {
            m_IconType = StringToIconType(sValue["iconType"].GetString());
        }
        
        if (sValue.HasMember("iconString") && sValue["iconString"].IsString()) {
            m_IconString = sValue["iconString"].GetString();
        }
        
        // DataBars specific parameters
        if (sValue.HasMember("color") && sValue["color"].IsString()) {
            m_Color = sValue["color"].GetString();
        }
        
        if (sValue.HasMember("direction") && sValue["direction"].IsString()) {
            m_Direction = sValue["direction"].GetString();
        }
        
        if (sValue.HasMember("style") && sValue["style"].IsString()) {
            m_Style = sValue["style"].GetString();
        }
        
        if (sValue.HasMember("minValue") && sValue["minValue"].IsDouble()) {
            m_MinValue = sValue["minValue"].GetDouble();
        }
        
        if (sValue.HasMember("maxValue") && sValue["maxValue"].IsDouble()) {
            m_MaxValue = sValue["maxValue"].GetDouble();
        }
        
        if (sValue.HasMember("colorNegative") && sValue["colorNegative"].IsString()) {
            m_Param6 = sValue["colorNegative"].GetString();
        }
        
        if (sValue.HasMember("param7") && sValue["param7"].IsString()) {
            m_Param7 = sValue["param7"].GetString();
        }
    }

    tString tItemCF::Json() {
        StringBuffer wBuffer;
        Writer<StringBuffer> wWriter(wBuffer);
        Json(&wWriter);
        return wBuffer.GetString();
    }

    // Specialized JSON methods for each conditional format type
    void tItemCF::JsonHighlightCellsRules(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        
        sWriter->Key(kJsonKeyType);
        sWriter->String("HighlightCellsRules");
        
        sWriter->Key(kJsonKeyFormat);
        sWriter->Int(m_Css);
        
        sWriter->Key(kJsonKeyPercent);
        sWriter->Double(m_Percent);
        
        sWriter->EndObject();
    }

    void tItemCF::JsonHighlightCellsRules(const Value& sValue) {
        if (sValue.HasMember(kJsonKeyFormat) && sValue[kJsonKeyFormat].IsInt64()) {
            m_Css = sValue[kJsonKeyFormat].GetInt();
        }
        if (sValue.HasMember(kJsonKeyPercent) && sValue[kJsonKeyPercent].IsDouble()) {
            m_Percent = sValue[kJsonKeyPercent].GetDouble();
        }
    }

    void tItemCF::JsonDataBars(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        
        sWriter->Key(kJsonKeyType);
        sWriter->String("DataBars");
        
        sWriter->Key(kJsonKeyFormat);
        sWriter->Int(m_Css);
        
        sWriter->Key(kJsonKeyPercent);
        sWriter->Double(m_Percent);
        
        sWriter->Key(kJsonKeyColor);
        sWriter->String(m_Color.c_str());
        
        sWriter->Key(kJsonKeyDirection);
        sWriter->String(m_Direction.c_str());
        
        sWriter->Key(kJsonKeyStyle);
        sWriter->String(m_Style.c_str());
        
        sWriter->Key(kJsonKeyMinValue);
        sWriter->Double(m_MinValue);
        
        sWriter->Key(kJsonKeyMaxValue);
        sWriter->Double(m_MaxValue);
        
        sWriter->Key(kJsonKeyParam6);
        sWriter->String(m_Param6.c_str());
        
        sWriter->Key(kJsonKeyParam7);
        sWriter->String(m_Param7.c_str());
        
        sWriter->EndObject();
    }

    void tItemCF::JsonDataBars(const Value& sValue) {
        if (sValue.HasMember(kJsonKeyFormat) && sValue[kJsonKeyFormat].IsInt()) {
            m_Css = sValue[kJsonKeyFormat].GetInt();
        }
        if (sValue.HasMember(kJsonKeyPercent) && sValue[kJsonKeyPercent].IsDouble()) {
            m_Percent = sValue[kJsonKeyPercent].GetDouble();
        }
        if (sValue.HasMember(kJsonKeyColor) && sValue[kJsonKeyColor].IsString()) {
            m_Color = sValue[kJsonKeyColor].GetString();
        }
        if (sValue.HasMember(kJsonKeyDirection) && sValue[kJsonKeyDirection].IsString()) {
            m_Direction = sValue[kJsonKeyDirection].GetString();
        }
        if (sValue.HasMember(kJsonKeyStyle) && sValue[kJsonKeyStyle].IsString()) {
            m_Style = sValue[kJsonKeyStyle].GetString();
        }
        if (sValue.HasMember(kJsonKeyMinValue) && sValue[kJsonKeyMinValue].IsDouble()) {
            m_MinValue = sValue[kJsonKeyMinValue].GetDouble();
        }
        if (sValue.HasMember(kJsonKeyMaxValue) && sValue[kJsonKeyMaxValue].IsDouble()) {
            m_MaxValue = sValue[kJsonKeyMaxValue].GetDouble();
        }
        if (sValue.HasMember(kJsonKeyParam6) && sValue[kJsonKeyParam6].IsString()) {
            m_Param6 = sValue[kJsonKeyParam6].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam7) && sValue[kJsonKeyParam7].IsString()) {
            m_Param7 = sValue[kJsonKeyParam7].GetString();
        }
    }

    void tItemCF::JsonColorScales(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        
        sWriter->Key(kJsonKeyType);
        sWriter->String("ColorScales");
        
        sWriter->Key(kJsonKeyFormat);
        sWriter->Int(m_Css);
        
        sWriter->Key(kJsonKeyPercent);
        sWriter->Double(m_Percent);
        
        sWriter->Key(kJsonKeyColor);
        sWriter->String(m_Color.c_str());
        
        sWriter->Key(kJsonKeyDirection);
        sWriter->String(m_Direction.c_str());
        
        sWriter->Key(kJsonKeyStyle);
        sWriter->String(m_Style.c_str());
        
        sWriter->Key(kJsonKeyMinValue);
        sWriter->Double(m_MinValue);
        
        sWriter->Key(kJsonKeyMaxValue);
        sWriter->Double(m_MaxValue);
        
        sWriter->EndObject();
    }

    void tItemCF::JsonColorScales(const Value& sValue) {
        if (sValue.HasMember(kJsonKeyFormat) && sValue[kJsonKeyFormat].IsInt()) {
            m_Css = sValue[kJsonKeyFormat].GetInt();
        }
        if (sValue.HasMember(kJsonKeyPercent) && sValue[kJsonKeyPercent].IsDouble()) {
            m_Percent = sValue[kJsonKeyPercent].GetDouble();
        }
        if (sValue.HasMember(kJsonKeyColor) && sValue[kJsonKeyColor].IsString()) {
            m_Color = sValue[kJsonKeyColor].GetString();
        }
        if (sValue.HasMember(kJsonKeyDirection) && sValue[kJsonKeyDirection].IsString()) {
            m_Direction = sValue[kJsonKeyDirection].GetString();
        }
        if (sValue.HasMember(kJsonKeyStyle) && sValue[kJsonKeyStyle].IsString()) {
            m_Style = sValue[kJsonKeyStyle].GetString();
        }
        if (sValue.HasMember(kJsonKeyMinValue) && sValue[kJsonKeyMinValue].IsDouble()) {
            m_MinValue = sValue[kJsonKeyMinValue].GetDouble();
        }
        if (sValue.HasMember(kJsonKeyMaxValue) && sValue[kJsonKeyMaxValue].IsDouble()) {
            m_MaxValue = sValue[kJsonKeyMaxValue].GetDouble();
        }
    }

    void tItemCF::JsonIconSets(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        
        sWriter->Key(kJsonKeyType);
        sWriter->String("IconSets");
        
        sWriter->Key(kJsonKeyFormat);
        sWriter->Int(m_Css);
        
        sWriter->Key(kJsonKeyPercent);
        sWriter->Double(m_Percent);
        
        sWriter->Key(kJsonKeyIconSetType);
        sWriter->String(IconTypeToString(m_IconType));
        
        sWriter->Key(kJsonKeyIconString);
        sWriter->String(m_IconString.c_str());

        // 0-based icon position. Lets the renderer (drawIconSets in SkUtility.js)
        // pick a deterministic tier (low/mid/high) without parsing the OOXML
        // iconString, which is non-unique for several built-in IconSets.
        sWriter->Key("iconIndex");
        sWriter->Int(m_IconIndex);
        // Total icons in the parent set (3 or 5). Combined with iconIndex it
        // gives the renderer a normalized tier (e.g. 3Arrows index=2 → high,
        // 5Arrows index=2 → mid) so the right Material Symbols glyph is chosen.
        sWriter->Key("iconTotal");
        sWriter->Int(m_IconTotal);
        
        sWriter->Key(kJsonKeyParam6);
        sWriter->String(m_Param6.c_str());
        if (m_Param6 != "") {
            sWriter->Key("excelIconSet");
            sWriter->String(m_Param6.c_str());
        }
        
        sWriter->Key(kJsonKeyParam7);
        sWriter->String(m_Param7.c_str());
        
        sWriter->EndObject();
    }

    void tItemCF::JsonIconSets(const Value& sValue) {
        if (sValue.HasMember(kJsonKeyFormat) && sValue[kJsonKeyFormat].IsInt()) {
            m_Css = sValue[kJsonKeyFormat].GetInt();
        }
        if (sValue.HasMember(kJsonKeyPercent) && sValue[kJsonKeyPercent].IsDouble()) {
            m_Percent = sValue[kJsonKeyPercent].GetDouble();
        }
        if (sValue.HasMember(kJsonKeyIconSetType) && sValue[kJsonKeyIconSetType].IsString()) {
            m_IconType = StringToIconType(sValue[kJsonKeyIconSetType].GetString());
        }
        if (sValue.HasMember(kJsonKeyIconString) && sValue[kJsonKeyIconString].IsString()) {
            m_IconString = sValue[kJsonKeyIconString].GetString();
        }
        if (sValue.HasMember("iconIndex") && sValue["iconIndex"].IsInt()) {
            m_IconIndex = sValue["iconIndex"].GetInt();
        }
        if (sValue.HasMember("iconTotal") && sValue["iconTotal"].IsInt()) {
            m_IconTotal = sValue["iconTotal"].GetInt();
        }
        if (sValue.HasMember(kJsonKeyParam6) && sValue[kJsonKeyParam6].IsString()) {
            m_Param6 = sValue[kJsonKeyParam6].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam7) && sValue[kJsonKeyParam7].IsString()) {
            m_Param7 = sValue[kJsonKeyParam7].GetString();
        }
    }

    // =========================================================================
    //! Conditional Format ====================================================
    tConditionalFormat::tConditionalFormat() : tClass(),tInterfaceCompil(),
        m_Container(nullptr), 
        m_Sheet(nullptr),
        m_WorkBook(nullptr),
        m_FormatApi(nullptr), 
        m_Type(tConditionalFormatType::t_None),
        m_IconType(tIconType::t_None),
        m_Param1(""),
        m_Param2(""),
        m_Param3(""),
        m_Param4(""),
        m_Param5(""),
        m_Param6(""),
        m_Param7(""),
        m_Param8(""),
        m_Param9(""),
        m_Param10(""),
        m_KeyString(""),
        m_Sum(0),
        m_CalculatedMinValue(0.0),
        m_CalculatedMaxValue(0.0),
        m_CalculatedMidValue(0.0),
        m_HasCalculatedValues(false) {}


    tConditionalFormat::tConditionalFormat(const tConditionalFormat& sConditionalFormat) : tClass(), tInterfaceCompil(),
        m_Container(sConditionalFormat.m_Container),
        m_Sheet(sConditionalFormat.m_Sheet),
        m_WorkBook(sConditionalFormat.m_WorkBook),
        m_FormatApi(sConditionalFormat.m_FormatApi),
        m_Type(sConditionalFormat.m_Type),
        m_IconType(sConditionalFormat.m_IconType),
        m_Param1(sConditionalFormat.m_Param1),
        m_Param2(sConditionalFormat.m_Param2),
        m_Param3(sConditionalFormat.m_Param3),
        m_Param4(sConditionalFormat.m_Param4),
        m_Param5(sConditionalFormat.m_Param5),
        m_Param6(sConditionalFormat.m_Param6),
        m_Param7(sConditionalFormat.m_Param7),

        m_KeyString(sConditionalFormat.m_KeyString),
        m_Sum(0),
        m_CalculatedMinValue(0.0),
        m_CalculatedMaxValue(0.0),
        m_CalculatedMidValue(0.0),
        m_HasCalculatedValues(false) {}
   
    tConditionalFormat::tConditionalFormat(tConditionalFormatContainer* sConditionalFormatContainer, tConditionalFormatType sConditionalFormatType,tString sRef, tSheet* sSheet) : tClass(), tInterfaceCompil(),
        m_Container(sConditionalFormatContainer), 
        m_Sheet(sSheet), 
        m_Type(sConditionalFormatType), 
        m_IconType(tIconType::t_None),
        m_Param1(""),
        m_Param2(""),
        m_Param3(""),
        m_Param4(""),
        m_Param5(""),
        m_Param6(""),
        m_Param7(""),
        m_Param8(""),
        m_Param9(""),
        m_Param10(""),
        m_KeyString(""),
        m_Sum(0),
        m_CalculatedMinValue(0.0),
        m_CalculatedMaxValue(0.0),
        m_CalculatedMidValue(0.0),
        m_HasCalculatedValues(false) {
            m_ConditionalRanges.SetKeyAndEnsureRange(m_Type, sRef, sSheet);
            m_WorkBook=sSheet->WorkBook();
            m_FormatApi=m_WorkBook->FormatApi();
    }

    tConditionalFormat::~tConditionalFormat() {
        ClearVectorRefAndDeleteDependant();
    }

    tConditionalFormatType tConditionalFormat::Type() { return(m_Type); }

    tIconType tConditionalFormat::IconType() { return(m_IconType); }
    void tConditionalFormat::IconType(tIconType sIconType) { m_IconType = sIconType; }

    tString tConditionalFormat::Param1() { return(m_Param1()); }
    tString tConditionalFormat::Param2() { return(m_Param2()); }
    tString tConditionalFormat::Param3() { return(m_Param3()); }
    tString tConditionalFormat::Param4() { return(m_Param4()); }
    tString tConditionalFormat::Param5() { return(m_Param5()); }
    tString tConditionalFormat::Param6() { return(m_Param6()); }
    tString tConditionalFormat::Param7() { return(m_Param7()); }
    tString tConditionalFormat::Param8() { return(m_Param8()); }
    tString tConditionalFormat::Param9() { return(m_Param9()); }
    tString tConditionalFormat::Param10() { return(m_Param10()); }

    void tConditionalFormat::SetParam1(tString sParam1) { m_Param1 = sParam1; }
    void tConditionalFormat::SetParam2(tString sParam2) { m_Param2 = sParam2; }
    void tConditionalFormat::SetParam3(tString sParam3) { m_Param3 = sParam3; }
    void tConditionalFormat::SetParam4(tString sParam4) { m_Param4 = sParam4; }
    void tConditionalFormat::SetParam5(tString sParam5) { m_Param5 = sParam5; }
    void tConditionalFormat::SetParam6(tString sParam6) { m_Param6 = sParam6; }
    void tConditionalFormat::SetParam7(tString sParam7) { m_Param7 = sParam7; }
    void tConditionalFormat::SetParam8(tString sParam8) { m_Param8 = sParam8; }
    void tConditionalFormat::SetParam9(tString sParam9) { m_Param9 = sParam9; }
    void tConditionalFormat::SetParam10(tString sParam10) { m_Param10 = sParam10; }

    tString tConditionalFormat::Key() {
        // Search ============================================================
        if (m_KeyString != "") {
            return(m_KeyString);
        }
        return(m_ConditionalRanges.Key());
    }

    tString tConditionalFormat::Ref() {
        return(m_ConditionalRanges.Ref());
    }

    void tConditionalFormat::SetKeyAndEnsureRange(tString sKey,tSheet* sSheet) {
        m_ConditionalRanges.SetKeyAndEnsureRange(m_Type,sKey,sSheet);
    }

    tConditionalRanges* tConditionalFormat::ConditionalRange() {
        return(&m_ConditionalRanges);
    }

    void tConditionalFormat::ClearVectorRefAndDeleteDependant() {
        m_VectorRef.clear();
        m_SharedFormula.Clear();
    }

    tBool tConditionalFormat::Compil(tString sFormula,tString sFormatTrue,tString sFormatFalse) {
#ifdef debugconditionalformat
        cout << "compil " << sFormula << ";" << sFormatTrue << ";" << sFormatFalse << endl;
#endif

        tLemonInterface* wLemonInterface = tSpreadSheetContainer::Instance()->LemonInterface();
        wLemonInterface->Compil(m_WorkBook,this, sFormula.c_str());
        if (wLemonInterface->CompilError()!=tErrorFormula::t_None) {
            cerr << wLemonInterface->Error() << ":" << wLemonInterface->ErrorWithDetail() << endl;
            return(false);
        } else {
            m_SharedFormula = *wLemonInterface->Formula();
            m_Param2 = sFormatTrue;
            m_Param3 = sFormatFalse;
            m_Param1 = sFormula;
        }
#ifdef debugconditionalformat
        cout << "Ok" << endl;
#endif
        return(true);
    }

    void tConditionalFormat::CallBackCell(tCell* sCell,tPass sPass) {
    
    #ifdef debugcell
        tString wStrPass;
        switch (sPass) {
            case tPass::t_PassApply: wStrPass="Apply"; break;
            case tPass::t_PassSum: wStrPass="Sum"; break;
            case tPass::t_PassFree: wStrPass="Free"; break;
            case tPass::t_PassInit: wStrPass="Init"; break;
            case tPass::t_PassCalculate: wStrPass="Calculate"; break;
        }
        m_Sheet->ColRowCellRange()->DebugCell("Before Pass "+wStrPass,sCell->StrRef());
    #endif

        switch (m_Type) {
            case tConditionalFormatType::t_CustomFormulas:
            case tConditionalFormatType::t_HighlightCellsRules: {
                if (sPass == tPass::t_PassApply) {
#ifdef debugconditionalformat
                    cout << "Apply -> tConditionalFormat::CallBackCell(" << sCell->StrRef() << ") Css=" << sCell->Css() << endl;
#endif
                    if (m_FormatApi!=nullptr) {
                        // Must default to 0: if formula is missing or result is not bool, we must not pass an uninitialized ref into tItemCF (would corrupt DeleteCellFormat bookkeeping).
                        tFormatRef wCss = 0;
#ifdef debugconditionalformat
                        cout << "Execute " << sCell->StrRef() << ":" << m_SharedFormula.Formula()->Str(sCell) << ":" << sCell->Value() << "->";
#endif
                        if (m_SharedFormula.Formula()!=nullptr) {
                            // Cross-cell refs in a CF formula (e.g. "%>$B$2")
                            // are stored on m_VectorRef (this CF object), not
                            // on sCell. tCell::InternalCalculation reads
                            // sCell->m_VectorRef when resolving Cell/Range
                            // bytecodes, so we must temporarily lend our refs
                            // to the cell. We do *not* PushRef them on the
                            // cell — that would wire B2 -> sCell into the
                            // dependency graph and break recalculation paths
                            // (CF is not a calculation dependency, just a
                            // visual rule). Swap restores the cell state at
                            // the end (RAII via tSwapGuard).
                            struct tSwapGuard {
                                tCell* m_Cell;
                                tVectorItem* m_Foreign;
                                std::vector<tBool> m_ForeignAddDependent;
                                tSwapGuard(tCell* sCellArg, tVectorItem* sForeignArg)
                                    : m_Cell(sCellArg), m_Foreign(sForeignArg),
                                      m_ForeignAddDependent(sForeignArg->size(), false) {
                                    m_Cell->SwapVectorRef(*m_Foreign, m_ForeignAddDependent);
                                }
                                ~tSwapGuard() {
                                    m_Cell->SwapVectorRef(*m_Foreign, m_ForeignAddDependent);
                                }
                            } wSwapGuard(sCell, &m_VectorRef);
                            tVariant wVariant=sCell->InternalCalculation(m_SharedFormula.Formula());
                            if (wVariant.IsBool()) {
                                // Normalize the trailing ';': the parser rejects ";;" so we
                                // must not blindly append one when the user-supplied CSS
                                // (or the Excel importer) already ends with one. Skip
                                // empty branches entirely so we don't feed ";" alone.
                                auto wNormalize = [](const tString& sCss) -> tString {
                                    if (sCss.empty()) return tString();
                                    if (!sCss.empty() && sCss.back() == ';') return sCss;
                                    return sCss + ";";
                                };
                                if (wVariant.Bool()) {
    #ifdef debugconditionalformat
                                    cout << "->" << m_Param2();
    #endif
                                    // For HighlightCellsRules/CustomFormulas: Param2 = format if true
                                    tString wCssTrue = wNormalize(m_Param2());
                                    if (!wCssTrue.empty()) {
                                        wCss=m_FormatApi->ApplyCellFormat(wCssTrue);
                                        if (wCss==0) {
                                            cerr << " tConditionalFormat::Apply(" << m_Param2() << ") Bad Format " << endl;
                                        }
                                    }
                                } else {
    #ifdef debugconditionalformat
                                    cout << "->" << m_Param3();
    #endif
                                    // For HighlightCellsRules/CustomFormulas: Param3 = format if false
                                    tString wCssFalse = wNormalize(m_Param3());
                                    if (!wCssFalse.empty()) {
                                        wCss=m_FormatApi->ApplyCellFormat(wCssFalse);
                                        if (wCss==0) {
                                            cerr << " tConditionalFormat::Apply(" << m_Param3() << ") Bad Format " << endl;
                                        }
                                    }
                                }
                            }
                        }
                        // Only IconSets can have icons, others use t_None
                        if (wCss != 0) {
                            tIconType wIconType = (m_Type == tConditionalFormatType::t_IconSets) ? m_IconType : tIconType::t_None;
                            tAllocatorRef wItemCFRef = m_Sheet->ColRowCellRange()->AllocItemCF();
                            tItemCF* wItemCf = m_Sheet->ColRowCellRange()->ItemCF(wItemCFRef);
                            if (wItemCf != nullptr) {
                                wItemCf->Set(m_Type,wCss,0.0, wIconType,"", "", "", "", 0.0, 100.0, "", "");
                                SetConditionalFormat(m_Type,sCell);
                                m_Container->AddItemCF(sCell, wItemCFRef);
#ifdef debugconditionalformat
                                cout << "Css=" << wItemCf->Css() << endl;
#endif
                            }
                        }
                    }
                    
                }
                break;
            }
            case tConditionalFormatType::t_DataBars:
                if (sPass== tPass::t_PassSum) {
                    tVariant wVariant=CfNumericScanVariant(sCell);
                    tDouble wValue = 0.0;
                    
                    if (wVariant.IsDouble()) {
                        wValue = wVariant.Double();
                    }
                    else if (wVariant.IsInt()) {
                        wValue = wVariant.Int();
                    }
                    else {
                        return; // Skip non-numeric values
                    }
                    
                    // Calculate min/max values from actual cell values if Param4/Param5 are empty
                    if (m_Param4() == "" || m_Param5() == "") {
                        if (!m_HasCalculatedValues) {
                            // First cell: initialize min/max
                            m_CalculatedMinValue = wValue;
                            m_CalculatedMaxValue = wValue;
                            m_HasCalculatedValues = true;
                        } else {
                            // Update min/max
                            if (wValue < m_CalculatedMinValue) {
                                m_CalculatedMinValue = wValue;
                            }
                            if (wValue > m_CalculatedMaxValue) {
                                m_CalculatedMaxValue = wValue;
                            }
                        }
                    }
                }
                else if (sPass== tPass::t_PassApply) {
                    // For DataBars: Param1=color, Param3=style,
                    //               Param4=min_value, Param5=max_value, Param6=negative color
              
                    // Calculate the percentage for this cell
                    tDouble wPercent = CalculateDataBarPercent(sCell);
                    
                    // Only IconSets can have icons, others use t_None
                    tIconType wIconType = (m_Type == tConditionalFormatType::t_IconSets) ? m_IconType : tIconType::t_None;
                    
                    // Use calculated min/max if Param4/Param5 are empty
                    tDouble wMinValue = 0.0;
                    tDouble wMaxValue = 100.0;
                    
                    if (m_Param4() != "") {
                        wMinValue = tClassString(m_Param4()).ToDouble();
                    } else if (m_HasCalculatedValues) {
                        wMinValue = m_CalculatedMinValue;
                    }
                    
                    if (m_Param5() != "") {
                        wMaxValue = tClassString(m_Param5()).ToDouble();
                    } else if (m_HasCalculatedValues) {
                        wMaxValue = m_CalculatedMaxValue;
                    }
                    
                    tAllocatorRef wItemCFRef = m_Sheet->ColRowCellRange()->AllocItemCF();
                    tItemCF* wItemCf = m_Sheet->ColRowCellRange()->ItemCF(wItemCFRef);
                    if (wItemCf != nullptr) {
                        wItemCf->Set(m_Type, 0, wPercent, wIconType, "",
                                      m_Param1(), "", m_Param3(),
                                      wMinValue, wMaxValue,
                                      m_Param6(), m_Param7());
                        SetConditionalFormat(m_Type,sCell);
                        m_Container->AddItemCF(sCell, wItemCFRef);
                    }
                }
                break;
            case tConditionalFormatType::t_ColorScales:
                if (sPass== tPass::t_PassSum) {
                    tVariant wVariant=CfNumericScanVariant(sCell);
                    tDouble wValue = 0.0;
                    
                    if (wVariant.IsDouble()) {
                        wValue = wVariant.Double();
                    }
                    else if (wVariant.IsInt()) {
                        wValue = wVariant.Int();
                    }
                    else {
                        return; // Skip non-numeric values
                    }
                    
                    // Calculate min/max/mid values from actual cell values
                    if (!m_HasCalculatedValues) {
                        // First cell: initialize min/max
                        m_CalculatedMinValue = wValue;
                        m_CalculatedMaxValue = wValue;
                        m_HasCalculatedValues = true;
                    } else {
                        // Update min/max
                        if (wValue < m_CalculatedMinValue) {
                            m_CalculatedMinValue = wValue;
                        }
                        if (wValue > m_CalculatedMaxValue) {
                            m_CalculatedMaxValue = wValue;
                        }
                    }
                    
                    // Calculate mid value (average of min and max)
                    m_CalculatedMidValue = (m_CalculatedMinValue + m_CalculatedMaxValue) / 2.0;
                }
                else if (sPass== tPass::t_PassApply) {
                    // Calculate color based on cell value and ColorScales parameters
                    // For ColorScales: Param1=min_color, Param2=mid_color, Param3=max_color, 
                    //                  Param4=min_value, Param5=mid_value, Param6=max_value, Param7=additional
                    if (m_FormatApi == nullptr) {
                        break;
                    }
                    tString wColorHex = CalculateColorScaleColor(sCell);
                    if (wColorHex != "") {
                        tStringStream wStream;
                        wStream << "background-color:" << wColorHex << ";";
#ifdef debugconditionalformat
                        cout << sCell->StrRef() << "=" << sCell->Value() << "-->" <<  wStream.str() << endl;
#endif
                        tFormatRef wCss = m_FormatApi->ApplyCellFormat(wStream.str());
                        
                        if (wCss != 0) {
                            // Only IconSets can have icons, others use t_None
                            tIconType wIconType = (m_Type == tConditionalFormatType::t_IconSets) ? m_IconType : tIconType::t_None;
                            tAllocatorRef wItemCFRef = m_Sheet->ColRowCellRange()->AllocItemCF();
                            tItemCF* wItemCf = m_Sheet->ColRowCellRange()->ItemCF(wItemCFRef);
                            if (wItemCf != nullptr) {
                                wItemCf->Set(m_Type, wCss, 0.0, wIconType, "", "", "", "", 0.0, 100.0, "", "");
                                SetConditionalFormat(m_Type,sCell);
                                m_Container->AddItemCF(sCell, wItemCFRef);
                            }
                        }
                    }
                }
                break;
            case tConditionalFormatType::t_IconSets:
                if (sPass== tPass::t_PassSum) {
                    tVariant wVariant=CfNumericScanVariant(sCell);
                    tDouble wValue = 0.0;
                    
                    if (wVariant.IsDouble()) {
                        wValue = wVariant.Double();
                    }
                    else if (wVariant.IsInt()) {
                        wValue = wVariant.Int();
                    }
                    else {
                        return; // Skip non-numeric values
                    }
                    
                    // Calculate min/max values from actual cell values for automatic thresholds
                    if (!m_HasCalculatedValues) {
                        // First cell: initialize min/max
                        m_CalculatedMinValue = wValue;
                        m_CalculatedMaxValue = wValue;
                        m_HasCalculatedValues = true;
                    } else {
                        // Update min/max
                        if (wValue < m_CalculatedMinValue) {
                            m_CalculatedMinValue = wValue;
                        }
                        if (wValue > m_CalculatedMaxValue) {
                            m_CalculatedMaxValue = wValue;
                        }
                    }
                    
                    // Calculate mid value (average of min and max)
                    m_CalculatedMidValue = (m_CalculatedMinValue + m_CalculatedMaxValue) / 2.0;
                }
                else if (sPass== tPass::t_PassApply) {
                    // For IconSets: Param1=icon1, Param2=icon2, Param3=icon3, 
                    //               Param4=threshold1, Param5=threshold2, Param6=threshold3, Param7=additional
                    // Paired Excel rules (e.g. 3Signs + 3Symbols2): higher Param10 wins — replace any
                    // IconSets already on the cell from a lower-priority rule applied earlier.
                    if (m_Container != nullptr && m_Container->CellHasIconSetItemCF(sCell)) {
                        m_Container->RemoveIconSetItemCF(sCell);
                    }
                    tInt wIconIndex = -1;
                    tInt wIconTotal = 0;
                    tString wIconSet = CalculateIconSetWithIndex(sCell, wIconIndex, wIconTotal);
#ifdef debugconditionalformat
                    cout << sCell->StrRef() << ":" << wIconSet << " idx=" << wIconIndex << "/" << wIconTotal << endl;
#endif
                    if (wIconSet != "") {
                        // IconSets can have icons, so use m_IconType
                        tAllocatorRef wItemCFRef = m_Sheet->ColRowCellRange()->AllocItemCF();
                        tItemCF* wItemCf = m_Sheet->ColRowCellRange()->ItemCF(wItemCFRef);
                        if (wItemCf != nullptr) {
                            wItemCf->Set(m_Type, 0, 0.0, m_IconType, wIconSet, "", "", "", 0.0, 100.0, m_Param8(), "");
                            // Stash the 0-based tier index + total so JsonIconSets
                            // exports them and drawIconSets() (SkUtility.js) can pick
                            // the right Material Symbols glyph + semantic color even
                            // when m_IconString is not unique across tiers (e.g.
                            // Indicators "●●●") or shared between 3- and 5-icon sets.
                            wItemCf->SetIconIndex(wIconIndex);
                            wItemCf->SetIconTotal(wIconTotal);
                            SetConditionalFormat(m_Type, sCell);
                            m_Container->AddItemCF(sCell, wItemCFRef);
                        }
                    }
                }
                break;
            default:
                break;
            }
            // Free All Element
            if (sPass== tPass::t_PassFree) {
                if (m_FormatApi!=nullptr) {
                    tCellConditionalFormat* wCellConditionalFormat=m_Container->CellIConditionalFormat(sCell);
                    if (wCellConditionalFormat!=nullptr) {
                        tAllocatorRef wItemCFRef = wCellConditionalFormat->ItemCF(m_Type);
                        tItemCF* wItemCF = m_Sheet->ColRowCellRange()->ItemCF(wItemCFRef);
                        if (wItemCF!=nullptr) {
                            wCellConditionalFormat->RemoveItemCF(wItemCFRef);
    #ifdef debugconditionalformat
                            cout << "Free -> tConditionalFormat::CallBackCell(" << sCell->StrRef() << ") Css=" << wItemCF->Css() << endl;
    #endif
                            if (wItemCF->Css()!=0) {
                                m_FormatApi->DeleteCellFormat(wItemCF->Css());
                            }
                            m_Sheet->ColRowCellRange()->DeleteItemCF(wItemCFRef);
                        }
                    }
                }
                sCell->RemoveConditionalFormat();
            } // Pass Free
#ifdef debugcell
                m_Sheet->ColRowCellRange()->DebugCell("After Pass "+wStrPass,sCell->StrRef());
#endif
    }

    void tConditionalFormat::CallBackRange(tRange* sRange,tPass sPass) {
        tIndex wRowLo = sRange->TopIndex();
        tIndex wRowHi = sRange->IterateBottom();
        tIndex wColLo = sRange->LeftIndex();
        tIndex wColHi = sRange->IterateRight();

        // JsonView: per-cell paint/free only on the visible viewport (PassSum keeps full range).
        if ((sPass == tPass::t_PassApply || sPass == tPass::t_PassFree) &&
            m_Container != nullptr && m_Container->JsonViewClipActive()) {
            tRect wClip = m_Container->JsonViewClipRect();
            if (!sRange->IntersectRect(&wClip)) {
                return;
            }
            wRowLo = std::max(wRowLo, wClip.Top());
            wRowHi = std::min(wRowHi, wClip.Bottom());
            wColLo = std::max(wColLo, wClip.Left());
            wColHi = std::min(wColHi, wClip.Right());
        }
        if (wRowLo > wRowHi || wColLo > wColHi) {
            return;
        }

        for (tIndex wRow = wRowLo; wRow <= wRowHi; wRow++) {
            for (tIndex wCol = wColLo; wCol <= wColHi; wCol++) {
                CallBackCell(m_Sheet->EnsureCell(wRow, wCol), sPass);
            }
        }
    }
    
    void tConditionalFormat::Apply(tPass sPass) {
        if (sPass==tPass::t_PassSum) {
            m_Sum=0;
            // Reset calculated values for ColorScales
            m_HasCalculatedValues = false;
            m_CalculatedMinValue = 0.0;
            m_CalculatedMaxValue = 0.0;
            m_CalculatedMidValue = 0.0;
        }
        switch (m_Type) {
            case tConditionalFormatType::t_HighlightCellsRules:
            case tConditionalFormatType::t_CustomFormulas:
                break;
            case tConditionalFormatType::t_DataBars:
            case tConditionalFormatType::t_ColorScales:
            case tConditionalFormatType::t_IconSets:
                break;
            default:
                break;
        }
       
        for (auto wRange : *m_ConditionalRanges.VectorRange()) {
            if (wRange != nullptr) {
                CallBackRange(wRange,sPass);
            }
        }
    }

    void tConditionalFormat::Formula(const tFormula* sFormula) {
        m_SharedFormula = *sFormula;
    }

    tFormula* tConditionalFormat::Formula() {
		return(m_SharedFormula.Formula());
	}

    tString tConditionalFormat::FormulaStr(tCell *sCell) {
        tFormula* wFormula=m_SharedFormula.Formula();
        if (wFormula!=nullptr) {
            return(wFormula->Str(sCell));
        }
        return("");
    };

    tSheet* tConditionalFormat::Sheet() {
        return(m_Sheet);
    }


    void tConditionalFormat::PushRef(tItem* sItem,tBool sDependent) {
        // Not Dependent
        m_VectorRef.push_back(sItem);
    }
     /// @brief Return Vector Item
        /// @return tVectorItem*
    tVectorItem* tConditionalFormat::VectorItem() { return(&m_VectorRef); }
    
    tIndex tConditionalFormat::RowIndex() {
        return(-1);
    }

    tIndex tConditionalFormat::ColIndex() {
        return(-1);
    }

    tBool tConditionalFormat::IntersectRect(tRect* sRect) {
        return(m_ConditionalRanges.IntersectRect(sRect));
    }

    void tConditionalFormat::KeyString(tString sKeyString) {
        m_KeyString = sKeyString;
    }

    tString tConditionalFormat::FormulaString() {
        return(m_Param1());
    }

    tBool tConditionalFormat::RangeExist(tRange* sRange) {
         for (auto wRange : *m_ConditionalRanges.VectorRange()) {
             if (wRange != nullptr) {
                 if(wRange==sRange) return(true);
             }
         }
        return(false);
    }

    tBool tConditionalFormat::RemoveRange(tRange* sRange) {
        return(m_ConditionalRanges.RemoveRange(sRange));
    }


    tBool tConditionalFormat::IsEmpty() {
        return(m_ConditionalRanges.VectorRange()->empty());
    }

    void tConditionalFormat::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        sWriter->Key(kJsonKeyConditionalFormatKey);
        tString wJsonKey = Ref();
        if (m_Type == tConditionalFormatType::t_IconSets && !m_Param8().empty()) {
            wJsonKey += "@" + m_Param8();
        }
        sWriter->String(wJsonKey.c_str());
        if (m_Type != tConditionalFormatType::t_None) {
            sWriter->Key(kJsonKeyConditionalFormatType);
            sWriter->String(ConditionalFormatTypeToString(m_Type));
        }   
        if (m_IconType != tIconType::t_None) {
            sWriter->Key(kJsonKeyIconSetType);
            sWriter->String(IconTypeToString(m_IconType));
        }
        if (m_Param1() != "") {
            sWriter->Key(kJsonKeyParam1);
            sWriter->String(m_Param1().c_str());
        }
        if (m_Param2() != "") {
            sWriter->Key(kJsonKeyParam2);
            sWriter->String(m_Param2().c_str());
        }
        if (m_Param3() != "") {
            sWriter->Key(kJsonKeyParam3);
            sWriter->String(m_Param3().c_str());
        }
        if (m_Param4() != "") {
            sWriter->Key(kJsonKeyParam4);
            sWriter->String(m_Param4().c_str());
        }
        if (m_Param5() != "") {
            sWriter->Key(kJsonKeyParam5);
            sWriter->String(m_Param5().c_str());
        }
        if (m_Param6() != "") {
            sWriter->Key(kJsonKeyParam6);
            sWriter->String(m_Param6().c_str());
        }
        if (m_Param7() != "") {
            sWriter->Key(kJsonKeyParam7);
            sWriter->String(m_Param7().c_str());
        }
        if (m_Param8() != "") {
            sWriter->Key(kJsonKeyParam8);
            sWriter->String(m_Param8() .c_str());
        }
        if (m_Param9() != "") {
            sWriter->Key(kJsonKeyParam9);
            sWriter->String(m_Param9().c_str());
        }
        if (m_Param10() != "") {
            sWriter->Key(kJsonKeyParam10);
            sWriter->String(m_Param10().c_str());
        }
        sWriter->EndObject();
    }

    void tConditionalFormat::Json(const Value& sValue,tSheet* sSheet) {
        tString wKey=sValue[kJsonKeyConditionalFormatKey].GetString();

        // IMPORTANT: m_Type must be read from JSON BEFORE SetKeyAndEnsureRange because
        // tConditionalRanges stores its own m_Type internally and Key() rebuilds the lookup
        // string as KeyStyleRef(m_ConditionalRanges.m_Type, Ref()) — e.g. "C2:C2.HC". The
        // container is constructed with t_None, so if we ensure-range first, the persisted
        // type stays t_None and Sheet::ConditionalFormat(t_HighlightCellsRules, "C2:C2")
        // never finds the entry on .sker reload (regression seen on Conditional.sker).
        if (sValue.HasMember(kJsonKeyConditionalFormatType)) {
            m_Type = StringToConditionalFormatType(sValue[kJsonKeyConditionalFormatType].GetString());
        }
        m_ConditionalRanges.SetKeyAndEnsureRange(m_Type,wKey,sSheet);
        if (sValue.HasMember(kJsonKeyIconSetType)) {
            m_IconType = StringToIconType(sValue[kJsonKeyIconSetType].GetString());
        }
        if (sValue.HasMember(kJsonKeyParam1)) {
            m_Param1 = sValue[kJsonKeyParam1].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam2)) {
            m_Param2 = sValue[kJsonKeyParam2].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam3)) {
            m_Param3 = sValue[kJsonKeyParam3].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam4)) {
            m_Param4 = sValue[kJsonKeyParam4].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam5)) {
            m_Param5 = sValue[kJsonKeyParam5].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam6)) {
            m_Param6 = sValue[kJsonKeyParam6].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam7)) {
            m_Param7 = sValue[kJsonKeyParam7].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam8)) {
            m_Param8 = sValue[kJsonKeyParam8].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam9)) {
            m_Param9 = sValue[kJsonKeyParam9].GetString();
        }
        if (sValue.HasMember(kJsonKeyParam10)) {
            m_Param10 = sValue[kJsonKeyParam10].GetString();
        }
        // Formula-driven types need m_SharedFormula compiled from Param1.
        // Without this, CallBackCell exits early because Formula() is null
        // and the conditional format never paints anything when the document
        // is reopened from disk (we only used to compile through the
        // tUndoConditionaFormat::Do path, which is not exercised on plain
        // .sker load).
        if (m_Type == tConditionalFormatType::t_HighlightCellsRules ||
            m_Type == tConditionalFormatType::t_CustomFormulas) {
            tString wFormula  = m_Param1();
            tString wTrueCss  = m_Param2();
            tString wFalseCss = m_Param3();
            if (!wFormula.empty()) {
                Compil(wFormula, wTrueCss, wFalseCss);
            }
        }
    }

    tString tConditionalFormat::CalculateColorScaleColor(tCell* sCell) {
        // Get cell value
        tVariant wVariant = CfNumericScanVariant(sCell);
        tDouble wCellValue = 0.0;
        
        if (wVariant.IsDouble()) {
            wCellValue = wVariant.Double();
        } else if (wVariant.IsInt()) {
            wCellValue = wVariant.Int();
        } else {
            return ""; // No valid numeric value
        }

        // Use calculated values instead of parameters
        if (!m_HasCalculatedValues) {
            return ""; // No values calculated yet
        }

        // Parse color parameters
        tString wMinColor = m_Param1();  // Color for minimum value
        tString wMidColor = m_Param2();  // Color for middle value (if 3-color scale)
        tString wMaxColor = m_Param3();  // Color for maximum value
        
        // Use calculated values from actual cell data
        tDouble wMinValue = m_CalculatedMinValue;
        tDouble wMaxValue = m_CalculatedMaxValue;
        tDouble wMidValue = m_CalculatedMidValue;

        // Determine if we have 2-color or 3-color scale
        bool wHasMidColor = (wMidColor != "");
        bool wHasMaxColor = (wMaxColor != "");

        tRGB wResultColor;

        if (wHasMidColor && wHasMaxColor) {
            // 3-color scale: min -> mid -> max
            if (wCellValue <= wMinValue) {
                wResultColor = HexToRGB(wMinColor);
            } else if (wCellValue >= wMaxValue) {
                wResultColor = HexToRGB(wMaxColor);
            } else if (wCellValue <= wMidValue) {
                // Interpolate between min and mid
                tDouble wFactor = CalculatePercentage(wCellValue, wMinValue, wMidValue);
                tRGB wMinRGB = HexToRGB(wMinColor);
                tRGB wMidRGB = HexToRGB(wMidColor);
                wResultColor = InterpolateColor(wMinRGB, wMidRGB, wFactor);
            } else {
                // Interpolate between mid and max
                tDouble wFactor = CalculatePercentage(wCellValue, wMidValue, wMaxValue);
                tRGB wMidRGB = HexToRGB(wMidColor);
                tRGB wMaxRGB = HexToRGB(wMaxColor);
                wResultColor = InterpolateColor(wMidRGB, wMaxRGB, wFactor);
            }
        } else if (wHasMaxColor) {
            // 2-color scale: min -> max
            tDouble wFactor = CalculatePercentage(wCellValue, wMinValue, wMaxValue);
            tRGB wMinRGB = HexToRGB(wMinColor);
            tRGB wMaxRGB = HexToRGB(wMaxColor);
            wResultColor = InterpolateColor(wMinRGB, wMaxRGB, wFactor);
        } else {
            // Fallback: use min color only
            wResultColor = HexToRGB(wMinColor);
        }

        return RGBToHex(wResultColor);
    }

    

    //=========================================================================
    //! Calculate DataBar percentage based on cell value and parameters
    //=========================================================================
    tDouble tConditionalFormat::CalculateDataBarPercent(tCell* sCell) {
        // For DataBars: Param4=min_value, Param5=max_value
        tVariant wVariant = CfNumericScanVariant(sCell);
        tDouble wCellValue = 0.0;
        
        if (wVariant.IsDouble()) {
            wCellValue = wVariant.Double();
        } else if (wVariant.IsInt()) {
            wCellValue = wVariant.Int();
        } else {
            return 0.0; // No valid numeric value
        }
        
        tDouble wMinValue = 0.0;
        tDouble wMaxValue = 100.0;
        
        // Use calculated values if Param4/Param5 are empty, otherwise use provided values
        if (m_Param4() != "") {
            wMinValue = tClassString(m_Param4()).ToDouble();
        } else if (m_HasCalculatedValues) {
            wMinValue = m_CalculatedMinValue;
        }
        
        if (m_Param5() != "") {
            wMaxValue = tClassString(m_Param5()).ToDouble();
        } else if (m_HasCalculatedValues) {
            wMaxValue = m_CalculatedMaxValue;
        }

        // Check if we have negative values
        bool wHasNegativeValues = (wMinValue < 0.0);
        bool wCurrentValueIsNegative = (wCellValue < 0.0);
        
        tDouble wPercentage;
        
        if (wHasNegativeValues) {
            // For centered bars with negative values, calculate percentage relative to max absolute value
            tDouble wMaxAbsValue = std::max(std::abs(wMinValue), std::abs(wMaxValue));
            
            if (wCurrentValueIsNegative) {
                // Negative value: percentage based on absolute value
                wPercentage = std::abs(wCellValue) / wMaxAbsValue;
            } else {
                // Positive value: percentage based on absolute value
                wPercentage = std::abs(wCellValue) / wMaxAbsValue;
            }
        } else {
            // No negative values: use standard percentage calculation
            wPercentage = CalculatePercentage(wCellValue, wMinValue, wMaxValue);
        }
        
        wPercentage = std::max(0.0, std::min(1.0, wPercentage)); // Clamp between 0 and 1
        
        return wPercentage;
    }

    //=========================================================================
    //! Calculate IconSet CSS based on cell value and parameters
    //=========================================================================
    tString tConditionalFormat::CalculateIconSet(tCell* sCell) {
        // Backward-compat shim: keep the legacy single-string return point so any
        // caller that didn't migrate to CalculateIconSetWithIndex still works.
        // New call sites (CallBackCell IconSets/t_PassApply) should call the
        // index-aware variant so the renderer can pick a deterministic tier.
        tInt wIndex = -1;
        tInt wTotal = 0;
        return CalculateIconSetWithIndex(sCell, wIndex, wTotal);
    }

    tString tConditionalFormat::CalculateIconSetWithIndex(tCell* sCell, tInt& sIndex, tInt& sTotal) {
        sIndex = -1;
        sTotal = 0;

        // Excel encodes IconSet thresholds with cfvo type="percent"|"percentile"|
        // "num"|"min"|"max"|"formula". Our XLSX parser strips that type and only
        // forwards the raw val (Param4..9), which means values like "20"/"40"/
        // "60"/"80" arrive as plain numbers even though OOXML meant 20%/40%/...
        // The fix: if the rule has aggregated cell-range stats AND every
        // threshold sits in [0,100], treat them as percentages of (min,max).
        // Falls back to literal values for the rare case of authored absolute
        // thresholds outside that band — covers ~all Excel built-in presets.
        tVectorString wCfvoTypes = tClassString(m_Param9()).Split(",");
        // Split(",") on "" yields one empty token; treat as no cfvo types.
        if (wCfvoTypes.size() == 1 && wCfvoTypes[0].empty()) {
            wCfvoTypes.clear();
        }
        auto wCfvoTypeAt = [&](tSize sIdx) -> tString {
            return (sIdx < wCfvoTypes.size()) ? wCfvoTypes[sIdx] : "percent";
        };
        auto wResolveCfvoValue = [&](tDouble wRaw, tSize sIdx) -> tDouble {
            const tString wType = wCfvoTypeAt(sIdx);
            if (wType == "num" || wType == "formula" || wType == "min" || wType == "max") {
                return wRaw;
            }
            if ((wType == "percent" || wType == "percentile") && m_HasCalculatedValues) {
                if (wRaw >= 0.0 && wRaw <= 100.0) {
                    const tDouble wRange = m_CalculatedMaxValue - m_CalculatedMinValue;
                    if (wRange > 0.0) {
                        return m_CalculatedMinValue + (wRaw / 100.0) * wRange;
                    }
                }
            }
            return wRaw;
        };
        auto remapThresholdsAsPercent = [&](std::vector<tDouble*> thresholds) -> void {
            if (!m_HasCalculatedValues || !wCfvoTypes.empty()) {
                return;
            }
            tDouble wRange = m_CalculatedMaxValue - m_CalculatedMinValue;
            if (wRange <= 0.0) return;
            for (tDouble* t : thresholds) {
                if (*t < 0.0 || *t > 100.0) return; // bail: looks like absolute values
            }
            for (tDouble* t : thresholds) {
                *t = m_CalculatedMinValue + (*t / 100.0) * wRange;
            }
        };
        // For IconSets: Support both 3-icon and 5-icon configurations like Excel
        // 3-icon: Param1=icon1, Param2=icon2, Param3=icon3, Param4=threshold1, Param5=threshold2, Param6=threshold3
        // 5-icon: Param1=icon1, Param2=icon2, Param3=icon3, Param4=icon4, Param5=icon5, 
        //         Param6=threshold1, Param7=threshold2, Param8=threshold3, Param9=threshold4, Param10=threshold5
        tVariant wVariant = CfNumericScanVariant(sCell);
        tDouble wCellValue = 0.0;
        
        if (wVariant.IsDouble()) {
            wCellValue = wVariant.Double();
        } else if (wVariant.IsInt()) {
            wCellValue = wVariant.Int();
        } else {
            return ""; // No valid numeric value
        }

        // Determine if we have 3 or 5 icons based on parameter usage
        // Check if Param6 is numeric to determine mode
        // In 3-icon mode: Param6 is threshold3 (numeric)
        // In 5-icon mode: Param6 is threshold1 (numeric)
        bool wParam6IsNumeric = false;
        if (m_Param6() != "" && tClassString(m_Param6()).IsNumber()) {
            wParam6IsNumeric = true;
        }
        
        // Additional check: if Param4 is numeric, it's definitely 3-icon mode
        bool wParam4IsNumeric = false;
        if (m_Param4() != "" && tClassString(m_Param4()).IsNumber()) {
            wParam4IsNumeric = true;
        }
        
        // Check if Param5 is numeric (should be numeric in 3-icon mode, icon in 5-icon mode)
        bool wParam5IsNumeric = false;
        if (m_Param5() != "" && tClassString(m_Param5()).IsNumber()) {
            wParam5IsNumeric = true;
        }
        
        // Determine mode: 
        // 3-icon mode: Param4 and Param5 are numeric (thresholds)
        // 5-icon mode: Param4 and Param5 are icons, Param6 is numeric (threshold)
        bool wIs3Icon = wParam4IsNumeric && wParam5IsNumeric;
        bool wIs5Icon = !wParam4IsNumeric && !wParam5IsNumeric && wParam6IsNumeric;
        
        // If no thresholds are set, default to 3-icon mode with automatic thresholds
        bool wUseDefaultThresholds = !wIs3Icon && !wIs5Icon;
        
        tString wSelectedIcon;
        
        if (wIs3Icon || wUseDefaultThresholds) {
            sTotal = 3;
            // 3-icon mode: icons in Param1-3, thresholds in Param4-6
            tString wIcon1 = m_Param1();      // First icon
            tString wIcon2 = m_Param2();      // Second icon
            tString wIcon3 = m_Param3();      // Third icon

            tDouble wThreshold1, wThreshold2, wThreshold3;
            
            if (wUseDefaultThresholds && m_HasCalculatedValues) {
                // Use calculated values from actual cell data for automatic thresholds
                tDouble wMinValue = m_CalculatedMinValue;
                tDouble wMaxValue = m_CalculatedMaxValue;
                tDouble wMidValue = m_CalculatedMidValue;
                
                // Create thresholds based on calculated values
                wThreshold1 = wMinValue + (wMidValue - wMinValue) * 0.5;  // 50% between min and mid
                wThreshold2 = wMidValue + (wMaxValue - wMidValue) * 0.5; // 50% between mid and max
                wThreshold3 = wMaxValue;
            } else {
                // Use default percentage-based thresholds
                wThreshold1 = 33.0;
                wThreshold2 = 66.0;
                wThreshold3 = 100.0;
                
                // Parse threshold values if provided
                if (m_Param4() != "") {
                    wThreshold1 = tClassString(m_Param4()).ToDouble();
                }
                if (m_Param5() != "") {
                    wThreshold2 = tClassString(m_Param5()).ToDouble();
                }
                if (m_Param6() != "") {
                    wThreshold3 = tClassString(m_Param6()).ToDouble();
                }
                wThreshold1 = wResolveCfvoValue(wThreshold1, 0);
                wThreshold2 = wResolveCfvoValue(wThreshold2, 1);
                wThreshold3 = wResolveCfvoValue(wThreshold3, 2);
                if (wCfvoTypes.empty()) {
                    // Convert 0..100 OOXML percent values to absolute thresholds.
                    remapThresholdsAsPercent({&wThreshold1, &wThreshold2, &wThreshold3});
                }
            }

            // Excel IconSets: top icon when value >= highest cfvo threshold.
            if (wCellValue >= wThreshold3) {
                wSelectedIcon = wIcon3; sIndex = 2;
            } else if (wCellValue >= wThreshold2) {
                wSelectedIcon = wIcon2; sIndex = 1;
            } else {
                wSelectedIcon = wIcon1; sIndex = 0;
            }
        } else if (wIs5Icon) {
            sTotal = 5;
            // 5-icon mode: icons in Param1-5, thresholds in Param6-10
            tString wIcon1 = m_Param1();      // First icon
            tString wIcon2 = m_Param2();      // Second icon
            tString wIcon3 = m_Param3();      // Third icon
            tString wIcon4 = m_Param4();      // Fourth icon
            tString wIcon5 = m_Param5();      // Fifth icon

            tDouble wThreshold1 = 20.0;
            tDouble wThreshold2 = 40.0;
            tDouble wThreshold3 = 60.0;
            tDouble wThreshold4 = 80.0;
            
            // Parse threshold values
            if (m_Param6() != "") {
                wThreshold1 = tClassString(m_Param6()).ToDouble();
            }
            if (m_Param7() != "") {
                wThreshold2 = tClassString(m_Param7()).ToDouble();
            }
            if (m_Param8() != "") {
                wThreshold3 = tClassString(m_Param8()).ToDouble();
            }
            if (m_Param9() != "") {
                wThreshold4 = tClassString(m_Param9()).ToDouble();
            }
            // Note: Param10 is the icon type, not a threshold
            // For 5 icons, we only need 4 thresholds

            // Convert OOXML percent thresholds to absolute values relative to
            // the cell-range stats. Without this, default Excel presets
            // (20/40/60/80) keep the bottom icon for every value below 20.
            remapThresholdsAsPercent({&wThreshold1, &wThreshold2, &wThreshold3, &wThreshold4});

            if (wCellValue >= wThreshold4) {
                wSelectedIcon = wIcon5; sIndex = 4;
            } else if (wCellValue >= wThreshold3) {
                wSelectedIcon = wIcon4; sIndex = 3;
            } else if (wCellValue >= wThreshold2) {
                wSelectedIcon = wIcon3; sIndex = 2;
            } else if (wCellValue >= wThreshold1) {
                wSelectedIcon = wIcon2; sIndex = 1;
            } else {
                wSelectedIcon = wIcon1; sIndex = 0;
            }
        }

        return wSelectedIcon;
    }

    tString tConditionalFormat::Debug() {
       tStringStream wStream;
       wStream << "tConditionalFormat Debug:" << endl;
       wStream << "Key:" << m_ConditionalRanges.Ref() << endl;
       if (m_Type != tConditionalFormatType::t_None) {
            wStream << "Type:" << ConditionalFormatTypeToString(m_Type) << endl;
        }
        wStream << "Icon Set Type:" << IconTypeToString(m_IconType) << endl;
       if (m_Param1() != "") {
            wStream << "Param1:" << m_Param1().c_str() << endl;
        }
        if (m_Param2() != "") {
            wStream << "Param2:" << m_Param2().c_str() << endl;
        }
        if (m_Param3()   != "") {
            wStream << "Param3:" << m_Param3().c_str() << endl;
        }
        if (m_Param4() != "") {
            wStream << "Param4:" << m_Param4().c_str() << endl;
        }
        if (m_Param5() != "") {
            wStream << "Param5:" << m_Param5().c_str() << endl;
        }
        if (m_Param6() != "") {
            wStream << "Param6:" << m_Param6().c_str() << endl;
        }
        if (m_Param7() != "") {
            wStream << "Param7:" << m_Param7().c_str() << endl;
        }
        if (m_Param8() != "") {
            wStream << "Param8:" << m_Param8() .c_str() << endl;
        }
        if (m_Param9() != "") {
            wStream << "Param9:" << m_Param9().c_str() << endl;
        }
        if (m_Param10() != "") {
            wStream << "Param10:" << m_Param10().c_str() << endl;
        }
       return(wStream.str());
    }
#ifdef checksp
    void tConditionalFormat::Check() {
        if (m_Type != tConditionalFormatType::t_None) {
            if (m_ConditionalRanges.VectorRange()->size() == 0) {
                tStringStream wStream;
                wStream << "throw: Check error on conditional format " << m_ConditionalRanges.Ref() << " No range";
                cerr << wStream.str() << endl;
                throw(tExceptionInternalError(wStream.str()));
            }
            for(auto wRange : *m_ConditionalRanges.VectorRange()) {
                if (wRange == nullptr) {
                    tStringStream wStream;
                    wStream << "throw: Check error on conditional format " << m_ConditionalRanges.Ref() << " Range is null";
                    cerr << wStream.str() << endl;
                    throw(tExceptionInternalError(wStream.str()));
                } else {
                    tBool wOk = false;
                    switch (m_Type) {
                        case tConditionalFormatType::t_HighlightCellsRules: wOk = wRange->IsCFHR(); break;
                        case tConditionalFormatType::t_DataBars: wOk = wRange->IsCFDB(); break;
                        case tConditionalFormatType::t_ColorScales: wOk = wRange->IsCFCS(); break;
                        case tConditionalFormatType::t_IconSets:  wOk = wRange->IsCFIS(); break;
                        case tConditionalFormatType::t_CustomFormulas: wOk = wRange->IsCFCF(); break;
                        default: wOk = false; break;
                        if (!wOk) {
                            tStringStream wStream;
                            wStream << "throw: Check error on conditional format " << m_ConditionalRanges.Ref() << " Range " << wRange->StrRef() << " is not in conditional format";
                            cerr << wStream.str() << endl;
                            throw(tExceptionInternalError(wStream.str()));
                        }
                    }
                }
            }
        }
    }
#endif
    // =========================================================================
    //! Range Conditionnal Format =============================================
    tRangeConditionnalFormat::tRangeConditionnalFormat() : tClass() {
    }

    tRangeConditionnalFormat::~tRangeConditionnalFormat() {
        m_VectorConditionalFormat.clear();
    }


    tConditionalFormat* tRangeConditionnalFormat::ConditionalFormat(tConditionalFormatType sConditionalFormatType) {
        size_t wIndex = Index(sConditionalFormatType);
        if (wIndex >= m_VectorConditionalFormat.size()) {
            return nullptr;
        }
        return(m_VectorConditionalFormat[wIndex]);
    }

    tInt tRangeConditionnalFormat::Index(tConditionalFormatType sType) {
        switch (sType) {
            case tConditionalFormatType::t_HighlightCellsRules: return(0);
            case tConditionalFormatType::t_DataBars: return(1);
            case tConditionalFormatType::t_ColorScales: return(2);
            case tConditionalFormatType::t_IconSets: return(3);
            case tConditionalFormatType::t_CustomFormulas: return(4);
            default: return(-1);
        }
    }

    tBool tRangeConditionnalFormat::AddConditionalFormat(tConditionalFormat* sConditionalFormat) {
        size_t wIndex = Index(sConditionalFormat->Type());
        if (wIndex >= m_VectorConditionalFormat.size()) {
            m_VectorConditionalFormat.resize(wIndex + 1, nullptr);
        }
        if (sConditionalFormat->Type() == tConditionalFormatType::t_IconSets
            && m_VectorConditionalFormat[wIndex] != nullptr) {
            m_MoreIconSets.push_back(sConditionalFormat);
            return(true);
        }
        m_VectorConditionalFormat[wIndex] = sConditionalFormat;
        return(true);
    }

    tBool tRangeConditionnalFormat::DeleteConditionalFormat(tConditionalFormatType sConditionalFormatType) {
        size_t wIndex = Index(sConditionalFormatType);
        if (wIndex >= m_VectorConditionalFormat.size()) {
            return(false);
        }
        m_VectorConditionalFormat[wIndex] = nullptr;
        return(true);
    }

    tBool tRangeConditionnalFormat::IsEmpty() {
        for (auto wConditionalFormat : m_VectorConditionalFormat) {
            if (wConditionalFormat != nullptr) {
                return(false);
            }
        }
        return(true);
    }

    // tCellConditionalFormat =============================================
    tCellConditionalFormat::tCellConditionalFormat(tColRowCellRange* sColRowCellRange) :
        tClass(),
        m_ColRowCellRange(sColRowCellRange) {
    }

    tCellConditionalFormat::~tCellConditionalFormat() {
        m_VectorItemCF.clear();
    }

    void tCellConditionalFormat::AddItemCF(tAllocatorRef sItemCF) {
        m_VectorItemCF.push_back(sItemCF);
    }

    void tCellConditionalFormat::RemoveItemCF(tAllocatorRef sItemCF) {
        m_VectorItemCF.erase(std::remove(m_VectorItemCF.begin(), m_VectorItemCF.end(), sItemCF), m_VectorItemCF.end());
    }

    tAllocatorRef tCellConditionalFormat::ItemCF(tConditionalFormatType sType) {
        if (m_ColRowCellRange == nullptr) {
            return(0);
        }
        for (auto wItemCFRef : m_VectorItemCF) {
            tItemCF* wItemCF = m_ColRowCellRange->ItemCF(wItemCFRef);
            if (wItemCF != nullptr && wItemCF->Type() == sType) {
                return wItemCFRef;
            }
        }
        return(0);
    }

    tVectorItemCF& tCellConditionalFormat::VectorItemCF() {
        std::sort(m_VectorItemCF.begin(), m_VectorItemCF.end(), [this](tAllocatorRef s1, tAllocatorRef s2) {
            if (m_ColRowCellRange == nullptr) {
                return(s1 < s2);
            }
            tItemCF* wItemCF1 = m_ColRowCellRange->ItemCF(s1);
            tItemCF* wItemCF2 = m_ColRowCellRange->ItemCF(s2);
            if (wItemCF1 == nullptr) {
                return(false);
            }
            if (wItemCF2 == nullptr) {
                return(true);
            }
            return wItemCF1->ReturnSortIndex() < wItemCF2->ReturnSortIndex();
        });
        return m_VectorItemCF;
    }

    tBool tCellConditionalFormat::IsEmpty() {
        return(m_VectorItemCF.empty());
    }

    void tCellConditionalFormat::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        sWriter->Key("itemCFs");
        sWriter->StartArray();
        
        for (auto wItemCFRef : m_VectorItemCF) {
            tItemCF* wItemCF = nullptr;
            if (m_ColRowCellRange != nullptr) {
                wItemCF = m_ColRowCellRange->ItemCF(wItemCFRef);
            }
            if (wItemCF != nullptr) {
                wItemCF->Json(sWriter);
            }
        }
        
        sWriter->EndArray();
        sWriter->EndObject();
    }

    void tCellConditionalFormat::Json(const Value& sValue) {
        if (sValue.HasMember("itemCFs") && sValue["itemCFs"].IsArray()) {
            if (m_ColRowCellRange == nullptr) {
                return;
            }
            for (auto wItemCFRef : m_VectorItemCF) {
                m_ColRowCellRange->DeleteItemCF(wItemCFRef);
            }
            m_VectorItemCF.clear();
            const Value& wArray = sValue["itemCFs"];
            
            for (SizeType i = 0; i < wArray.Size(); i++) {
                if (wArray[i].IsObject()) {
                    tAllocatorRef wItemCFRef = m_ColRowCellRange->AllocItemCF();
                    tItemCF* wItemCF = m_ColRowCellRange->ItemCF(wItemCFRef);
                    if (wItemCF != nullptr) {
                        wItemCF->Json(wArray[i]);
                        m_VectorItemCF.push_back(wItemCFRef);
                    }
                }
            }
        }
    }

    tString tCellConditionalFormat::Json() {
        StringBuffer wBuffer;
        Writer<StringBuffer> wWriter(wBuffer);
        Json(&wWriter);
        return wBuffer.GetString();
    }


    // tConditionalFormatContainer ============================================
    tConditionalFormatContainer::tConditionalFormatContainer(tSheet* sSheet)
        : tClass(),
          m_Sheet(sSheet),
          m_JsonViewClipRect(),
          m_JsonViewClipActive(false) {
    }

    tConditionalFormatContainer::~tConditionalFormatContainer() {
        ReleaseAppliedFormats();
        for (auto wRangeConditionalFormat : m_MapRange) {
            delete(wRangeConditionalFormat.second);
        }
        for (auto wConditionalFormat : m_VectoConditionalFormat) {
            delete(wConditionalFormat);
        }
    }

    tSheet* tConditionalFormatContainer::Sheet() { return(m_Sheet); }

    tBool tConditionalFormatContainer::Exist(tString sKey) {
        tConditionalFormat wConditionalFormatSearch;
        wConditionalFormatSearch.KeyString(sKey);
#ifdef debugconditionalformat
        cout << "tConditionalFormatContainer::Exist("  << wConditionalFormatSearch.Key() << ")" << endl;
#endif
        // Search in vector ===================================================
        tVectorConditionalFormat::iterator wWhere;
        wWhere = std::lower_bound(m_VectoConditionalFormat.begin(), m_VectoConditionalFormat.end(), &wConditionalFormatSearch, tComparatorConditionalFormat());

        if (wWhere != m_VectoConditionalFormat.end()) {
#ifdef debugconditionalformat
            cout << " wWhere ="  << (*wWhere)->Key() << " != " << wConditionalFormatSearch.Key() << endl;
#endif
            if ((*wWhere)->Key() == wConditionalFormatSearch.Key()) {
                return(true);
            }
        }
        return(false);
    }

    tBool tConditionalFormatContainer::AddRangeInMapRange(tConditionalFormatType sType, tString sRef,tConditionalFormat* sConditionalFormat) {
#ifdef debugconditionalformat
        cout << "tConditionalFormatContainer::AddRangeInMapRange("  << sRef << ")" << endl;
#endif
        tConditionalRanges wConditionalKey;
        wConditionalKey.SetKeyAndEnsureRange(sType,sRef,m_Sheet);
        // if Exist
        // Loop on range
        for (auto wRange : *wConditionalKey.VectorRange()) {
            if (m_MapRange.find(wRange) != m_MapRange.end()) {
#ifdef debugconditionalformat
                cout << "tConditionalFormatContainer::AddRangeInMapRange(" << m_Sheet->Name() << ":" << wRange->StrRef() << ") Range already exist" << endl;
#endif
                tRangeConditionnalFormat* wRangeConditionalFormat=m_MapRange[wRange];
                if (wRangeConditionalFormat->ConditionalFormat(sType)!=nullptr
                    && sType != tConditionalFormatType::t_IconSets) {
                    return(false);
                }
            }
        }

        for (auto wRange : *wConditionalKey.VectorRange()) {
                // Get Or Create TRangeConditionFormat
                if (m_MapRange.find(wRange) == m_MapRange.end()) {
                    tRangeConditionnalFormat* wRangeConditionalFormat=new tRangeConditionnalFormat();
                    m_MapRange[wRange] = wRangeConditionalFormat;
                }
                m_MapRange[wRange]->AddConditionalFormat(sConditionalFormat);
        }
        return(true);
    }

    tBool tConditionalFormatContainer::DeleteRangeInMapRange(tRange* sRange) {
#ifdef debugconditionalformat
        cout << "tConditionalFormatContainer::DeleteRangeInMapRange(" << sRange->StrRef() << ")" << endl;
#endif
        if (m_MapRange.find(sRange) != m_MapRange.end()) {
            delete(m_MapRange[sRange]);
            m_MapRange.erase(sRange);
            return(true);
        }
        return(false);
    }

    tConditionalFormat* tConditionalFormatContainer::Add(tConditionalFormatType sType,tString sRef,tSheet* sSheet) {
        m_Sheet=sSheet;
        // Update path: find by type+ref (KeyStyleRef), not Exist(sRef) which compares bare ref only.
        tConditionalFormat* wExisting = ConditionalFormat(sType, sRef);
        if (wExisting != nullptr) {
            return(wExisting);
        }
     
        tString wKey=KeyStyleRef(sType, sRef);

#ifdef debugconditionalformat
        cout << "tConditionalFormatContainer::Add(" << m_Sheet->Name() << ":" << wKey << ")" << endl;
#endif
        
        tConditionalFormat* wConditionalFormat = new tConditionalFormat(this,sType, sRef, sSheet);
        wConditionalFormat->KeyString(wKey);
        // Verif Range free
        if (!AddRangeInMapRange(sType,sRef,wConditionalFormat)) {
            delete(wConditionalFormat);
            return(nullptr);
        }
        // Insert in vector ===================================================
        tConditionalFormat wConditionFormatSearch;
        wConditionFormatSearch.KeyString(wKey);
        tVectorConditionalFormat::iterator wWhere;
        wWhere = std::lower_bound(m_VectoConditionalFormat.begin(), m_VectoConditionalFormat.end(), &wConditionFormatSearch, tComparatorConditionalFormat());
        if (wWhere != m_VectoConditionalFormat.end()) {
            m_VectoConditionalFormat.insert(wWhere, wConditionalFormat);
        }
        else {
            m_VectoConditionalFormat.push_back(wConditionalFormat);
        }
     
        return(wConditionalFormat);
    }

        
    tBool tConditionalFormatContainer::IsEmpty() {
        if (m_VectoConditionalFormat.empty()) {
            return(true);
        }
        return(false);
    }

    tBool tConditionalFormatContainer::RemoveByKey(tString sKey) {
#ifdef debugconditionalformat
        cout << "tConditionalFormatContainer::Remove(" << sKey << ")" << endl;
        for (tConditionalFormat* wConditionalFormat : m_VectoConditionalFormat) {
            cout << " wConditionalFormat->Key()=" << wConditionalFormat->Key() <<  endl;
        }
        cout << "Map Range" << endl;
        for(auto wItem : m_MapRange) {
            cout << " wItem.first->StrRef()=" << wItem.first << ":->" << wItem.first->StrRef() <<  endl;
        }
#endif
        // Cell -> Range use tConditonalKey for convert cell to range
        // Delete range  in vector ===================================================
        tConditionalFormat wConditionFormatSearch;
        wConditionFormatSearch.KeyString(sKey);
        tVectorConditionalFormat::iterator wWhere;
        wWhere = std::lower_bound(m_VectoConditionalFormat.begin(), m_VectoConditionalFormat.end(), &wConditionFormatSearch, tComparatorConditionalFormat());
        if (wWhere != m_VectoConditionalFormat.end()) {
            if ((*wWhere)->Key() == wConditionFormatSearch.Key()) {
                tConditionalFormat* wConditionalFormat = *wWhere;
                tConditionalRanges* wConditionalKey=wConditionalFormat->ConditionalRange();
                tColRowCellRange* wColRowCellRange = m_Sheet->ColRowCellRange();
                //* Erase Range
                for (auto wItem : *wConditionalKey->VectorRange()) {
                    tRange* wRange=wItem->Range();
                    if (wRange == nullptr || wColRowCellRange->IsDeletedRange(wRange->AllocatorRef())) {
                        continue;
                    }
                    // Remove Element with  wConditionalFormat->Type( in tRangeConditionnalFormat
                    tRangeConditionnalFormat* wRangeConditionalFormat=ConditionalFormatByRange(wRange);
                    if (wRangeConditionalFormat!=nullptr) {
                        tBool wResult=wRangeConditionalFormat->DeleteConditionalFormat(wConditionalFormat->Type());
                        if (!wResult) {
                            tStringStream wStream;
                            wStream << "tConditionalFormatContainer::Remove(" << sKey << ") DeleteConditionalFormat(" <<    ConditionalFormatTypeToString(wConditionalFormat->Type()) << ") failed";
                            cerr << wStream.str() << endl;
                            throw(tExceptionInternalError(wStream.str()));
                            return(false);
                        }
                        if (wRangeConditionalFormat->IsEmpty()) {
                            DeleteRangeInMapRange(wRange);
                        }
                        // Remove Extension for ConditionalFormat Type
                        RemoveConditionalFormat(wConditionalFormat->Type(), wRange);
                
                        // Delete Range if empty (sClean: range may already be off ColRow after col delete)
                        if (wRange->IsEmpty()) {
                            m_Sheet->ColRowCellRange()->DeleteRangeByAllocatorRef(wRange->AllocatorRef(), true);
                        }

                    } else {
                        tStringStream wStream;
                        wStream << "tConditionalFormatContainer::Remove(" << sKey << ") wRangeConditionalFormat=nullptr !d";
                        cerr << wStream.str() << endl;
                        throw(tExceptionInternalError(wStream.str()));
                    }
                    // Delete Range if empty
                    
                                        
                }
                m_VectoConditionalFormat.erase(wWhere);
                delete(wConditionalFormat);
                return(true);
            }
        }
        return(false);
    }
            
    tBool tConditionalFormatContainer::RemoveByRect(tConditionalFormatType sType, tString sRef,tRect* sAreaDelete) {
#ifdef debugconditionalformat
        cout << "tConditionalFormatContainer::Remove(" << sRef << ")" << endl;
        for (tConditionalFormat* wConditionalFormat : m_VectoConditionalFormat) {
            cout << " wConditionalFormat->Key()=" << wConditionalFormat->Key() <<  endl;
        }
        cout << "Map Range" << endl;
        for(auto wItem : m_MapRange) {
            cout << " wItem.first->StrRef()=" << wItem.first << ":->" << wItem.first->StrRef() <<  endl;
        }
#endif
        // Cell -> Range use tConditonalKey for convert cell to range
        // Delete range  in vector ===================================================
        tConditionalFormat* wConditionalFormat=nullptr;
        
        tConditionalFormat wConditionFormatSearch;
        tBool wRaz=true;
        wConditionFormatSearch.KeyString(sRef);
        tVectorConditionalFormat::iterator wWhere;
        wWhere = std::lower_bound(m_VectoConditionalFormat.begin(), m_VectoConditionalFormat.end(), &wConditionFormatSearch, tComparatorConditionalFormat());
        if (wWhere != m_VectoConditionalFormat.end()) {
            if ((*wWhere)->Key() == wConditionFormatSearch.Key()) {
                wConditionalFormat = *wWhere;
                tConditionalRanges* wConditionalKey=wConditionalFormat->ConditionalRange();
                tColRowCellRange* wColRowCellRange = m_Sheet->ColRowCellRange();
                //* Lopp on range for remove conditional format on range
                for (auto wItem : *wConditionalKey->VectorRange()) {
                    tRange* wRange=wItem->Range();
                    if (wRange == nullptr || wColRowCellRange->IsDeletedRange(wRange->AllocatorRef())) {
                        continue;
                    }
                    
                    
                    // Remove conditionalFormat
                    RemoveConditionalFormat(sType, wRange);
                    if (wRange->IsConditionalFormat()) {
                        wRaz=false;
                    }
                }
                if (wRaz) {
                    //* Lopp on range for remove conditional format on range
                    for (auto wItem : *wConditionalKey->VectorRange()) {
                        tRange* wRange=wItem->Range();
                        if (wRange == nullptr || wColRowCellRange->IsDeletedRange(wRange->AllocatorRef())) {
                            continue;
                        }
                        
                        DeleteRangeInMapRange(wRange);
                        // Is Range Empty Delete & sErase Range
                        if (sAreaDelete!=nullptr) {
                            if (wRange->InsideRect(sAreaDelete)) {
                                if (wRange->IsEmpty()) {
                                    m_Sheet->ColRowCellRange()->DeleteRangeByAllocatorRef(wRange->AllocatorRef(), true);
                                }
                            }
                        } else {
                            if (wRange->IsEmpty()) {
                                m_Sheet->ColRowCellRange()->DeleteRangeByAllocatorRef(wRange->AllocatorRef(), true);
                            }
                        }
                    }
                }
                // Erase in vector
                m_VectoConditionalFormat.erase(wWhere);
                // delete
                delete(wConditionalFormat);
                return(true);
            }
        }
        return(false);
    }
            

    tConditionalFormat* tConditionalFormatContainer::ConditionalFormat(tConditionalFormatType sType, tString sRef) {
        tString wKey=KeyStyleRef(sType, sRef);
        for (tConditionalFormat* wConditionalFormat : m_VectoConditionalFormat) {
            if (wConditionalFormat->Key() == wKey) {
                return(wConditionalFormat);
            }
        }
        return(nullptr);
    }

    
    tRangeConditionnalFormat* tConditionalFormatContainer::ConditionalFormatByRange(tRange* sRange) {
#ifdef debugconditionalformat
        cout << "tConditionalFormatContainer::ConditionalFormatByRange(" << sRange << ":-->" << sRange->StrRef() << ")" << endl;
        for(auto wItem : m_MapRange) {
            cout << " wItem.first->StrRef()=" << wItem.first << ":->" << wItem.first->StrRef() <<  endl;
        }
#endif

        if (m_MapRange.find(sRange) != m_MapRange.end()) {
            return(m_MapRange[sRange]);
        }
        return(nullptr);
    }
        
    tBool tConditionalFormatContainer::JsonViewClipActive() const {
        return m_JsonViewClipActive;
    }

    const tRect& tConditionalFormatContainer::JsonViewClipRect() const {
        return m_JsonViewClipRect;
    }

    tBool tConditionalFormatContainer::MakeIntersectRectList(tRect* sRect) {
        tBool wResult=false;
        m_VectorIntersect.clear();
        m_JsonViewClipActive = false;
        if (sRect != nullptr) {
            m_JsonViewClipRect = *sRect;
            m_JsonViewClipActive = true;
        } else {
            return false;
        }
#ifdef debugconditionalformat
        cout << "tConditionalFormatContainer::MakeIntersectRectList(" << sRect->StrRef() << ")" << endl;
#endif
        for (tConditionalFormat* wConditionalFormat : m_VectoConditionalFormat) {
            if (wConditionalFormat->IntersectRect(sRect)) {
                m_VectorIntersect.push_back(wConditionalFormat);
#ifdef debugconditionalformat
        cout << " Push wConditionFormat->Key()=" << wConditionalFormat->Key() <<  endl;
#endif
                wResult=true;
            }
        }
        return(wResult);
    }

    void  tConditionalFormatContainer::ApplyJsonView() {
        // ItemCF instances are owned by tColRowCellRange::m_AllocatorItemCF.
        // Release their applied CSS refs before returning the allocator slots.
        tFormatApi* wFormatApi = nullptr;
        tColRowCellRange* wColRowCellRange = nullptr;
        if (m_Sheet != nullptr) {
            wColRowCellRange = m_Sheet->ColRowCellRange();
            tWorkBook* wWorkBook = m_Sheet->WorkBook();
            if (wWorkBook != nullptr) {
                wFormatApi = wWorkBook->FormatApi();
            }
        }
        for(auto wItem : m_MapCell) {
            tCellConditionalFormat* wCellConditionalFormat = wItem.second;
            if (wCellConditionalFormat != nullptr && wColRowCellRange != nullptr) {
                for (tAllocatorRef wItemCFRef : wCellConditionalFormat->VectorItemCF()) {
                    tItemCF* wItemCF = wColRowCellRange->ItemCF(wItemCFRef);
                    if (wItemCF == nullptr) {
                        continue;
                    }
                    if (wFormatApi != nullptr && wItemCF->Css() != 0) {
                        wFormatApi->DeleteCellFormat(wItemCF->Css());
                    }
                    wColRowCellRange->DeleteItemCF(wItemCFRef);
                }
            }
            delete(wCellConditionalFormat);
        }
        m_MapCell.clear();
        if (m_VectorIntersect.empty()) {
            return;
        }
#ifdef debugconditionalformat
        cout << "tConditionalFormatContainer::ApplyJsonView()" << endl;
#endif
        auto wExcelCfPriority = [](tConditionalFormat* sCf) -> tInt {
            if (sCf == nullptr || sCf->Param10().empty()) {
                return 0;
            }
            return static_cast<tInt>(tClassString(sCf->Param10()).ToDouble());
        };
        tVectorContionalFormat wIconRules;
        for (tConditionalFormat* wConditionalFormat : m_VectorIntersect) {
            if (wConditionalFormat->Type() == tConditionalFormatType::t_IconSets) {
                wIconRules.push_back(wConditionalFormat);
            }
        }
        // Book 11 pairs @3Signs + @3Symbols2 on the same sqref. Excel keeps one icon
        // set per cell: 3Signs (cross/check, green at 0) must win over 3Symbols2 on
        // CADEAUX and REPAS alike. Apply 3Symbols2 first, then 3Signs replaces it.
        auto wIconSetApplyOrder = [](tConditionalFormat* sCf) -> tInt {
            if (sCf != nullptr && sCf->Param8() == "3Signs") {
                return 1;
            }
            return 0;
        };
        std::sort(wIconRules.begin(), wIconRules.end(),
            [&](tConditionalFormat* sA, tConditionalFormat* sB) {
                const tInt wKindA = wIconSetApplyOrder(sA);
                const tInt wKindB = wIconSetApplyOrder(sB);
                if (wKindA != wKindB) {
                    return wKindA < wKindB;
                }
                const tInt wPriA = wExcelCfPriority(sA);
                const tInt wPriB = wExcelCfPriority(sB);
                if (wPriA != wPriB) {
                    return wPriA < wPriB;
                }
                return false;
            });
        for (tConditionalFormat* wConditionalFormat : wIconRules) {
            wConditionalFormat->Apply(tPass::t_PassSum);
        }
        for (tConditionalFormat* wConditionalFormat : m_VectorIntersect) {
#ifdef debugconditionalformat
        cout << " Apply wConditionFormat->Key()=" << wConditionalFormat->Key() <<  endl;
#endif
            switch (wConditionalFormat->Type()) {
                case tConditionalFormatType::t_CustomFormulas:
                case tConditionalFormatType::t_HighlightCellsRules: {
                    wConditionalFormat->Apply(tPass::t_PassApply);
                    break;
                }
                    
                case tConditionalFormatType::t_DataBars: 
                case tConditionalFormatType::t_ColorScales: {
                    wConditionalFormat->Apply(tPass::t_PassSum);
                    wConditionalFormat->Apply(tPass::t_PassApply);
                    break;
                }
                case tConditionalFormatType::t_IconSets:
                    break;
                default : break;
            }
        }
        // IconSets: 3Symbols2 first, 3Signs last (replaces); within each group ascending Param10.
        for (tConditionalFormat* wConditionalFormat : wIconRules) {
            wConditionalFormat->Apply(tPass::t_PassApply);
        }
    }

    void tConditionalFormatContainer::ReleaseAppliedFormats() {
        tFormatApi* wFormatApi = nullptr;
        tColRowCellRange* wColRowCellRange = nullptr;
        if (m_Sheet != nullptr) {
            wColRowCellRange = m_Sheet->ColRowCellRange();
            tWorkBook* wWorkBook = m_Sheet->WorkBook();
            if (wWorkBook != nullptr) {
                wFormatApi = wWorkBook->FormatApi();
            }
        }
        for (auto& wEntry : m_MapCell) {
            tCellConditionalFormat* wCellConditionalFormat = wEntry.second;
            if (wCellConditionalFormat == nullptr) {
                continue;
            }
            if (wColRowCellRange != nullptr) {
                for (tAllocatorRef wItemCFRef : wCellConditionalFormat->VectorItemCF()) {
                    tItemCF* wItemCF = wColRowCellRange->ItemCF(wItemCFRef);
                    if (wItemCF == nullptr) {
                        continue;
                    }
                    if (wFormatApi != nullptr && wItemCF->Css() != 0) {
                        wFormatApi->DeleteCellFormat(wItemCF->Css());
                    }
                    wColRowCellRange->DeleteItemCF(wItemCFRef);
                }
            }
            delete wCellConditionalFormat;
        }
        m_MapCell.clear();
        m_VectorIntersect.clear();
        m_JsonViewClipActive = false;
    }

    void tConditionalFormatContainer::ApplyFree() {
        for (tConditionalFormat* wConditionalFormat : m_VectorIntersect) {
            wConditionalFormat->Apply(tPass::t_PassFree);
        }
        ReleaseAppliedFormats();
    }

    tSize tConditionalFormatContainer::Size() {
        return(m_VectoConditionalFormat.size());
    }

    tConditionalFormat* tConditionalFormatContainer::ConditionalFormatAt(tSize sIndex) {
        return sIndex < m_VectoConditionalFormat.size() ? m_VectoConditionalFormat[sIndex] : nullptr;
    }

    void tConditionalFormatContainer::AddItemCF(tCell* sCell, tAllocatorRef sItemCF) {
        if (sCell == nullptr) {
            return;
        }
        // CellIConditionalFormat must not use operator[] (it used to insert nullptr keys).
        auto wIt = m_MapCell.find(sCell);
        if (wIt == m_MapCell.end() || wIt->second == nullptr) {
            m_MapCell[sCell] = new tCellConditionalFormat(m_Sheet->ColRowCellRange());
        }
        m_MapCell[sCell]->AddItemCF(sItemCF);
    }

    tBool tConditionalFormatContainer::CellHasIconSetItemCF(tCell* sCell) {
        if (sCell == nullptr) {
            return(false);
        }
        auto wIt = m_MapCell.find(sCell);
        if (wIt == m_MapCell.end() || wIt->second == nullptr) {
            return(false);
        }
        tColRowCellRange* wColRowCellRange = m_Sheet != nullptr ? m_Sheet->ColRowCellRange() : nullptr;
        if (wColRowCellRange == nullptr) {
            return(false);
        }
        for (tAllocatorRef wItemCFRef : wIt->second->VectorItemCF()) {
            tItemCF* wItemCF = wColRowCellRange->ItemCF(wItemCFRef);
            if (wItemCF != nullptr && wItemCF->Type() == tConditionalFormatType::t_IconSets) {
                return(true);
            }
        }
        return(false);
    }

    void tConditionalFormatContainer::RemoveIconSetItemCF(tCell* sCell) {
        if (sCell == nullptr || m_Sheet == nullptr) {
            return;
        }
        auto wIt = m_MapCell.find(sCell);
        if (wIt == m_MapCell.end() || wIt->second == nullptr) {
            return;
        }
        tColRowCellRange* wColRowCellRange = m_Sheet->ColRowCellRange();
        if (wColRowCellRange == nullptr) {
            return;
        }
        tFormatApi* wFormatApi = nullptr;
        tWorkBook* wWorkBook = m_Sheet->WorkBook();
        if (wWorkBook != nullptr) {
            wFormatApi = wWorkBook->FormatApi();
        }
        tCellConditionalFormat* wCellConditionalFormat = wIt->second;
        tVectorItemCF wRefs = wCellConditionalFormat->VectorItemCF();
        for (tAllocatorRef wItemCFRef : wRefs) {
            tItemCF* wItemCF = wColRowCellRange->ItemCF(wItemCFRef);
            if (wItemCF == nullptr || wItemCF->Type() != tConditionalFormatType::t_IconSets) {
                continue;
            }
            wCellConditionalFormat->RemoveItemCF(wItemCFRef);
            if (wFormatApi != nullptr && wItemCF->Css() != 0) {
                wFormatApi->DeleteCellFormat(wItemCF->Css());
            }
            wColRowCellRange->DeleteItemCF(wItemCFRef);
        }
    }

    tCellConditionalFormat* tConditionalFormatContainer::CellIConditionalFormat(tCell* sCell) {
        if (sCell == nullptr) {
            return nullptr;
        }
        auto wIt = m_MapCell.find(sCell);
        if (wIt == m_MapCell.end()) {
            return nullptr;
        }
        return wIt->second;
    }

    //Json ================================================================
    void tConditionalFormatContainer::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartArray();
        for (tConditionalFormat* wConditionalFormat : m_VectoConditionalFormat) {
            wConditionalFormat->Json(sWriter);
        }
        sWriter->EndArray();
    }

    void tConditionalFormatContainer::Json(const Value& sValue,tSheet* sSheet) {
        for (SizeType wIndex = 0; wIndex < sValue.Size(); wIndex++) {
            const Value& wValue = sValue[wIndex];
            tConditionalFormat* wConditionalFormat = new tConditionalFormat(this,tConditionalFormatType::t_None, wValue["key"].GetString(), sSheet);
            wConditionalFormat->Json(wValue,sSheet);
            m_VectoConditionalFormat.push_back(wConditionalFormat);
            // Mirror tConditionalFormatContainer::Add(): once the JSON-loaded format knows its
            // final m_Type and ranges, register every range in m_MapRange so:
            //   - Sheet::ConditionalFormatByRange() can find the rule at render time;
            //   - tRange::Check() (when checksp is enabled) does not blow up with
            //     "Range not in Conditional Format Container".
            // Without this, .sker reload silently dropped DataBars / ColorScales / IconSets
            // from the painted output even though m_VectoConditionalFormat still listed them.
            AddRangeInMapRange(wConditionalFormat->Type(), wValue["key"].GetString(), wConditionalFormat);
        }
    }

    tString tConditionalFormatContainer::Json() {
        tString wJsonConditionalFormat;
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartArray();
        for (tConditionalFormat* wConditionalFormat : m_VectoConditionalFormat) {
            wConditionalFormat->Json(&wWriter);
        }
        wWriter.EndArray();
        wJsonConditionalFormat = wStringBuffer.GetString();
        return(wJsonConditionalFormat);
    }

    tString tConditionalFormatContainer::JsonByRect(tRect* sRect) {
        tString wJsonConditionalFormat;
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartArray();
        for (tConditionalFormat* wConditionalFormat : m_VectoConditionalFormat) {
            if (wConditionalFormat->IntersectRect(sRect)) {
                wConditionalFormat->Json(&wWriter);
            }
        }
        wWriter.EndArray();
        wJsonConditionalFormat = wStringBuffer.GetString();
        return(wJsonConditionalFormat);
    }

    tString tConditionalFormatContainer::Debug() {
        tStringStream wStream;
        wStream << "tConditionalFormatContainer Debug:" << endl;
        for (tConditionalFormat* wConditionalFormat : m_VectoConditionalFormat) {
            wStream << wConditionalFormat->Debug() << endl;
        }
        wStream << "Map Range,tConditionalFormat -------- " << endl;
        for(auto wItem : m_MapRange) {
            wStream << " wItem.first->StrRef()=" << wItem.first << ":->" << wItem.first->StrRef() << ": ConditionalFormat:->"<< wItem.second <<  endl;
        }
        return(wStream.str());
    }

#ifdef checksp
    void tConditionalFormatContainer::Check() {
        for(auto wItem : m_VectoConditionalFormat) {
            wItem->Check();
        }
    }
#endif

#ifdef checksp
    void tConditionalFormatContainer::CheckRange(tRange* sRange) {
        // Test Range in Conditional Format Container
        if (m_MapRange.find(sRange) != m_MapRange.end()) {
            tRangeConditionnalFormat* wRangeCoditionFormat=ConditionalFormatByRange(sRange);
            if (wRangeCoditionFormat==nullptr) {
                tStringStream wStream;
                wStream << "throw: Check error on range " << sRange->StrRef() << " Range not in Conditional Format Container";
                cerr << wStream.str() << endl;
                throw(tExceptionInternalError(wStream.str()));
            }
            tBool wOk=true;
            tStringStream wStream;
            if (sRange->IsCFHR()) {
                if (wRangeCoditionFormat->ConditionalFormat(tConditionalFormatType::t_HighlightCellsRules)==nullptr) {
                    wOk=false; wStream << "HighlightCellsRules ";
                }
            }
            if (sRange->IsCFDB()) {
                if (wRangeCoditionFormat->ConditionalFormat(tConditionalFormatType::t_DataBars)==nullptr) {
                    wOk=false; wStream << "DataBars ";
                }
            }
            if (sRange->IsCFCS()) {
                if (wRangeCoditionFormat->ConditionalFormat(tConditionalFormatType::t_ColorScales)==nullptr) {
                    wOk=false; wStream << "ColorScales ";
                }
            }
            if (sRange->IsCFIS()) {
                if (wRangeCoditionFormat->ConditionalFormat(tConditionalFormatType::t_IconSets)==nullptr) {
                    wOk=false; wStream << "IconSets ";
                }
            }
            if (sRange->IsCFCF()) {
                if (wRangeCoditionFormat->ConditionalFormat(tConditionalFormatType::t_CustomFormulas)==nullptr) {
                    wOk=false; wStream << "CustomFormulas ";
                }
            }
            if (!wOk) {
                tStringStream wStream;
                wStream << "throw: Check error on range " << sRange->StrRef() << " " << wStream.str();
                cerr << wStream.str() << endl;
                throw(tExceptionInternalError(wStream.str()));
            }
        } else {
            tStringStream wStream;
            wStream << "throw: Check error on range " << sRange->StrRef() << " Range not in Conditional Format Container";
            cerr << wStream.str() << endl;
            throw(tExceptionInternalError(wStream.str()));
        }
    }
#endif
}

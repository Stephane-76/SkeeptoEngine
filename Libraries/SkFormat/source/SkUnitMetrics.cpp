//=============================================================================
// SkUnitMetrics  (UnitMetrics for  Css )
//=============================================================================

#include "../include/SkUnitMetrics.hpp"

namespace SkFormat {

    // SkUnit =================================================================
    tUnitCss::tUnitCss() : tClass(), m_Unit(tUnitMetrics::none), m_Value(0) {};
    tUnitCss::tUnitCss(tFloat sValue, tUnitMetrics sUnit) : tClass(), m_Unit(sUnit), m_Value(sValue) {};

    tUnitCss::tUnitCss(const tUnitCss& sUnit) : tClass(sUnit), m_Unit(sUnit.m_Unit), m_Value(sUnit.m_Value) {};

    void tUnitCss::Unit(tUnitCss sSkUnit) {
        m_Unit = sSkUnit.m_Unit;
        m_Value = sSkUnit.m_Value;
    }

    void tUnitCss::Unit(tUnitMetrics sUnit) { m_Unit = sUnit; }
    tUnitMetrics tUnitCss::Unit() { return(m_Unit); }

    void tUnitCss::Value(tFloat sValue) { m_Value = sValue; }
    tFloat tUnitCss::Value() { return(m_Value); }


    tFloat tUnitCss::Convert(const tFloat sValue, const tUnitMetrics sFrom, const tUnitMetrics sTo) {
        return(tFloat(SkMetrics::Convert(sValue, sFrom, sTo)));
    }

    tFloat tUnitCss::Pixels() {
        return(tFloat(SkMetrics::Convert(m_Value, m_Unit, tUnitMetrics::pixels)));
    }

    tFloat tUnitCss::Inches() {
        return(Convert(m_Value, m_Unit, tUnitMetrics::inches));
    }

    tFloat tUnitCss::Points() {
        return(Convert(m_Value, m_Unit, tUnitMetrics::points));
    }

    tFloat tUnitCss::Picas() {
        return(Convert(m_Value, m_Unit, tUnitMetrics::picas));
    }

    tFloat tUnitCss::Centimeters() {
        return(Convert(m_Value, m_Unit, tUnitMetrics::centimeters));
    }

    tFloat tUnitCss::Millimeters() {
        return(Convert(m_Value, m_Unit, tUnitMetrics::millimeters));
    }

    tFloat tUnitCss::Percent() {
        return(m_Value);
    }

    tFloat tUnitCss::SizePixels(tFloat sWidthHeight) {
        switch (Unit()) {
        case tUnitMetrics::inches:
        case  tUnitMetrics::centimeters:
        case  tUnitMetrics::millimeters:
        case  tUnitMetrics::points:
        case  tUnitMetrics::picas:
        case  tUnitMetrics::pixels:
            return(Pixels());
        case tUnitMetrics::percent:
            return(Percent() * sWidthHeight / 100);
        default:
            return(0);
        }
    }

    tString tUnitCss::Str() {
        tStringStream wStream;
        if (m_Unit != tUnitMetrics::none) {
            wStream << m_Value << SkMetrics::UnitShortName(m_Unit);
        }
        return(wStream.str());
    }

    tBool tUnitCss::Empty() {
        return(m_Unit == tUnitMetrics::none);
    }

    void tUnitCss::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        sWriter->Key("v");
        sWriter->Double(m_Value);
        sWriter->Key("u");
        sWriter->Int(tInt(m_Unit));
        sWriter->EndObject();
    }

    /// @brief		Reader Json. 
    /// @param[in]	sValue Value&
    void tUnitCss::Json(const rapidjson::Value& sValue) {
        m_Value = sValue["v"].GetFloat();
        m_Unit = tUnitMetrics(sValue["u"].GetInt());
    }

    tBool tUnitCss::operator == (const tUnitCss& sUnit) {
        return((m_Unit == sUnit.m_Unit) && (m_Value == sUnit.m_Value));
    }

    bool tUnitCss::operator != (const tUnitCss& sUnit) {
        return(!(*this == sUnit));
    }

    ostream& operator<<(ostream& sStream, tUnitCss sValue) {
        sStream << sValue.Str();
        return(sStream);
    }


    // SkUnitRect =============================================================
    tUnitRectCss::tUnitRectCss() : tFormatShare(), m_All(), m_Left(), m_Top(), m_Right(), m_Bottom() {}
    tUnitRectCss::tUnitRectCss(const tUnitRectCss& sUnitRect) : tFormatShare(sUnitRect), m_All(sUnitRect.m_All), m_Left(sUnitRect.m_Left), m_Top(sUnitRect.m_Top), m_Right(sUnitRect.m_Right), m_Bottom(sUnitRect.m_Bottom) {}

    tUnitRectCss::tUnitRectCss(const tUnitCss& sAll) : tFormatShare(), m_All(sAll), m_Left(), m_Top(), m_Right(), m_Bottom() {}

    tUnitRectCss::tUnitRectCss(const tUnitCss &sLeft, const tUnitCss &sTop, const tUnitCss &sRight, const tUnitCss &sBottom) : tFormatShare(), 
        m_Left(sLeft), m_Top(sTop), m_Right(sRight), m_Bottom(sBottom) {}


    /// @brief		Clear
    void tUnitRectCss::Clear() {
        tFormatShare::Clear();
        m_All.Unit(tUnitMetrics::none);
        m_Left.Unit(tUnitMetrics::none);
        m_Top.Unit(tUnitMetrics::none);
        m_Right.Unit(tUnitMetrics::none);
        m_Bottom.Unit(tUnitMetrics::none);
    }

    tString tUnitRectCss::Key() {
        tStringStream wStream;
        if (!m_All.Empty()) { wStream << "a" << m_All << ":"; }
        if (!m_Left.Empty()) { wStream << "l" << m_Left << ":"; }
        if (!m_Top.Empty()) { wStream << "t" << m_Top << ":"; }
        if (!m_Right.Empty()) { wStream << "r" << m_Right << ":"; }
        if (!m_Bottom.Empty()) { wStream << "b" << m_Bottom << ":"; }
      
        return(wStream.str());
    }

    tString tUnitRectCss::Str(tString sRoot,tBool sReturn) {
        tStringStream wStream;
        
        tStringStream wStreamReturn;
        wStreamReturn << ";";
        if (sReturn) wStreamReturn << endl;
        
        tBool wOk = false;
        if (!m_All.Empty()) { 
            wOk = true;
            wStream << sRoot << ":" << m_All; 
        }
        if (!m_Left.Empty()) { 
            if (wOk) wStream << wStreamReturn.str();
            wOk = true;
            wStream << sRoot << "-left:" << m_Left; 
        }
        if (!m_Top.Empty()) { 
            if (wOk) wStream << wStreamReturn.str();
            wOk = true;
            wStream << sRoot << "-top:" << m_Top;
        }
        if (!m_Right.Empty()) { 
            if (wOk) wStream << wStreamReturn.str();
            wOk = true;
            wStream << sRoot << "-right:" << m_Right;
        }
        if (!m_Bottom.Empty()) {
            if (wOk) wStream << wStreamReturn.str();
            wStream << sRoot << "-bottom:" << m_Bottom;
        }
       
        return(wStream.str());
    }

    void  tUnitRectCss::Merge(tUnitRectCss*  sMerge) {
        if (!sMerge->All().Empty()) {
            m_All = sMerge->All();
        }
        if (!sMerge->Left().Empty()) {
            m_Left = sMerge->Left();
        }
        if (!sMerge->Top().Empty()) {
            m_Top = sMerge->Top();
        }
        if (!sMerge->Right().Empty()) {
            m_Right = sMerge->Right();
        }
        if (!sMerge->Bottom().Empty()) {
            m_Bottom = sMerge->Bottom();
        }
    }

    void tUnitRectCss::Rect(tUnitCss sUnit) {
        m_Left = sUnit;
        m_Top = sUnit;
        m_Right = sUnit;
        m_Bottom = sUnit;
    }

    void tUnitRectCss::All(tUnitCss sUnit) { m_All = sUnit; }
    tUnitCss& tUnitRectCss::All() { return(m_All); }

    void tUnitRectCss::Left(tUnitCss sUnit) { m_Left = sUnit; }
    tUnitCss& tUnitRectCss::Left() { return(m_Left); }

    void tUnitRectCss::Top(tUnitCss sUnit) { m_Top = sUnit; }
    tUnitCss& tUnitRectCss::Top() { return(m_Top); }

    void tUnitRectCss::Right(tUnitCss sUnit) { m_Right = sUnit; }
    tUnitCss& tUnitRectCss::Right() { return(m_Right); }

    void tUnitRectCss::Bottom(tUnitCss sUnit) { m_Bottom = sUnit; }
    tUnitCss& tUnitRectCss::Bottom() { return(m_Bottom); }

    void tUnitRectCss::ApplyUnit(tVectorUnitCss* sVector) {
        switch (sVector->size()) {
            case 0: break;
            case 1: {
                m_All=(*sVector)[0];
                break;
            }
            case 2: {
                /* top and bottom | left and right */
                m_Top=(*sVector)[0];
                m_Bottom=(*sVector)[1];
                
                m_Left=(*sVector)[0];
                m_Right=(*sVector)[1];
                break;
            }
            case 3: {
                /* top | left and right | bottom */
                m_Top=(*sVector)[0];
                m_Left=(*sVector)[1];
                m_Right=(*sVector)[1];
                m_Bottom=(*sVector)[2];
                break;
            }
            case 4:
                
            default:
                /* top | right | bottom | left */
                m_Top=(*sVector)[0];
                m_Right=(*sVector)[1];
                m_Bottom=(*sVector)[2];
                m_Left=(*sVector)[3];
                break;
        }
    }

    void tUnitRectCss::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        if (!m_All.Empty()) { sWriter->Key("a"); m_All.Json(sWriter); }
        if (!m_Left.Empty()) { sWriter->Key("l"); m_Left.Json(sWriter); }
        if (!m_Top.Empty()) { sWriter->Key("t"); m_Top.Json(sWriter); }
        if (!m_Right.Empty()) { sWriter->Key("r"); m_Right.Json(sWriter); }
        if (!m_Bottom.Empty()) { sWriter->Key("b"); m_Bottom.Json(sWriter); }
        sWriter->EndObject();
    }

    void tUnitRectCss::Json(const rapidjson::Value& sValue) {
        if (sValue.HasMember("a")) { const Value& wAll = sValue["a"]; m_All.Json(wAll); }
        if (sValue.HasMember("l")) { const Value& wLeft = sValue["l"]; m_Left.Json(wLeft); }
        if (sValue.HasMember("t")) { const Value& wTop = sValue["t"]; m_Top.Json(wTop); }
        if (sValue.HasMember("r")) { const Value& wRight = sValue["r"]; m_Right.Json(wRight); }
        if (sValue.HasMember("b")) { const Value& wBottom = sValue["b"]; m_Bottom.Json(wBottom); }
    }

    tBool tUnitRectCss::Empty() {
        return(m_All.Empty() &&  m_Left.Empty() && m_Top.Empty() && m_Right.Empty() && m_Bottom.Empty());
    }



}// End of namespace SkFormat

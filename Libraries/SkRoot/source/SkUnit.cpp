//=============================================================================
// SkUnit see SkSpreadSheet tCellClassUnit
// 17/01/2025
//=============================================================================
#include "../include/SkUnit.hpp"

#include <cmath>

namespace SkRoot {

namespace {

    tDouble RapportLengthToMeter(t_UnitLength sUnit) {
        for (const auto& wRow : CstRecUnitLength) {
            if (wRow.m_Length == sUnit) {
                return wRow.m_Rapport;
            }
        }
        return 1.0;
    }

    tDouble RapportMassToGram(t_UnitMass sUnit) {
        for (const auto& wRow : CstRecUnitMass) {
            if (wRow.m_Mass == sUnit) {
                return wRow.m_Rapport;
            }
        }
        return 1.0;
    }

    tDouble RapportTimeToSecond(t_UnitTime sUnit) {
        for (const auto& wRow : CstRecUnitTime) {
            if (wRow.m_Time == sUnit) {
                return wRow.m_Rapport;
            }
        }
        return 1.0;
    }

} // namespace


// Function ================================================================
t_UnitFamily UnitFamily(tString sValue) {
    for(auto wRecUnitFamily : CstRecUnitFamily) {
        if (sValue==wRecUnitFamily.m_Name) return(wRecUnitFamily.m_Family);
    }
    return(t_UnitFamily::None);
}

t_UnitMoney UnitMoney(tString sValue) {
    for(auto wRecUnitMoney : CstRecUnitMoney) {
        if (sValue==wRecUnitMoney.m_Name) return(wRecUnitMoney.m_Money);
    }
    return(t_UnitMoney::None);
}

t_UnitLength UnitLength(tString sValue) {
    for(auto wRecUnitLength : CstRecUnitLength) {
        if (sValue==wRecUnitLength.m_Name) return(wRecUnitLength.m_Length);
    }
    return(t_UnitLength::None);
}

t_UnitTime UnitTime(tString sValue) {
    for(auto wRecUnitTime : CstRecUnitTime) {
        if (sValue==wRecUnitTime.m_Name) return(wRecUnitTime.m_Time);
    }
    return(t_UnitTime::None);
}

t_UnitMass UnitMass(tString sValue) {
    for(auto wRecUnitMass : CstRecUnitMass) {
        if (sValue==wRecUnitMass.m_Name) return(wRecUnitMass.m_Mass);
    }
    return(t_UnitMass::None);
}


tString UnitMoneyStr(t_UnitMoney sValue) {
    for(auto wRecUnitMoney : CstRecUnitMoney) {
        if (sValue==wRecUnitMoney.m_Money) return(wRecUnitMoney.m_Name);
    }
    return("");
}


tString UnitMoneySymbol(t_UnitMoney sValue) {
    for(auto wRecUnitMoney : CstRecUnitMoney) {
        if (sValue==wRecUnitMoney.m_Money) return(wRecUnitMoney.m_Symbol);
    }
    return("");
}

// tClassUnit ==============================================================
tClassUnit::tClassUnit() : tClass(),m_Family(t_UnitFamily::None),m_Power(1) {}
tClassUnit::tClassUnit(t_UnitMoney sMoney) : tClass(),m_Family(t_UnitFamily::Monetary),m_Power(1) {
    m_Union.m_Money=sMoney;
}
tClassUnit::tClassUnit(t_UnitLength sLength) : tClass(),m_Family(t_UnitFamily::Length),m_Power(1) {
    m_Union.m_Length=sLength;
}
tClassUnit::tClassUnit(t_UnitTime sTime) : tClass(),m_Family(t_UnitFamily::Time),m_Power(1) {
    m_Union.m_Time=sTime;
}
tClassUnit::tClassUnit(t_UnitMass sMass) : tClass(),m_Family(t_UnitFamily::Mass),m_Power(1) {
    m_Union.m_Mass=sMass;
}

tClassUnit::tClassUnit(const tClassUnit& sUnitClass) : tClass(sUnitClass) {
    m_Family=sUnitClass.m_Family;
    m_Union=sUnitClass.m_Union;
    m_Power=sUnitClass.m_Power;
}

void tClassUnit::Clear() {
    m_Family=t_UnitFamily::None;
    m_Power=1;
}

t_UnitFamily tClassUnit::Family() const { return(m_Family); }

tShort tClassUnit::Power() const { return(m_Power); }
void tClassUnit::Power(tShort sValue) { m_Power=sValue; }


tString tClassUnit::GetSymbol() const {
    switch (m_Family) {
        case t_UnitFamily::None : return("");
        case t_UnitFamily::Monetary : {
            switch (m_Union.m_Money) {
                case t_UnitMoney::eur : return("€");
                case t_UnitMoney::usd : return("$");
                case t_UnitMoney::gpb : return("£");
                case t_UnitMoney::yen : return("¥");
                case t_UnitMoney::chf : return("CHF");
                case t_UnitMoney::cad : return("CA$");
                case t_UnitMoney::aud : return("A$");
                default: return("");
            }
        } // money
        case t_UnitFamily::Length : {
            switch (m_Union.m_Length) {
                case t_UnitLength::micron  : return("um");
                case t_UnitLength::millimeter  : return("mm");
                case t_UnitLength::centimeter  : return("cm");
                case t_UnitLength::meter  : return("m");
                case t_UnitLength::kilometer : return("km");
                default: return("");
            }
        } // length (metre family)
        case t_UnitFamily::Time :  {
            switch (m_Union.m_Time) {
                case t_UnitTime::millisecond : return("ms");
                case t_UnitTime::second : return("s");
                case t_UnitTime::minute : return("min");
                case t_UnitTime::hour : return("h");
                case t_UnitTime::day : return("d");
                case t_UnitTime::month : return("mo");
                case t_UnitTime::year : return("y");
                default: return("");
            }
        } // - meter (m)
        case t_UnitFamily::Mass : {
            switch (m_Union.m_Mass) {
                case t_UnitMass::mg  : return("mg");
                case t_UnitMass::g  : return("g");
                case t_UnitMass::kg  : return("kg");
                case t_UnitMass::to  : return("t");
                default: return("");
            }
        }  // - kilogram (kg)
        case t_UnitFamily::AmountOfSubstance : { break;}  // - mole (mol)
        case t_UnitFamily::ElectricCurrent : { break;}  // - ampere (A)
        case t_UnitFamily::TemperatureKelvin : { break;}  // (K)
        case t_UnitFamily::LuminousIntensityCandela : { break;}  // (cd)
        default:
            break;
    }
    return("");
}

tBool tClassUnit::IsEmpty() const {
    return(m_Family==t_UnitFamily::None);
}

tBool tClassUnit::IsSupportPower() const {
    switch (m_Family) {
        case t_UnitFamily::Length : return(true);
        case t_UnitFamily::Time : return(true);
        case t_UnitFamily::Mass : return(true);
        default:
            break;
    }
    return(false);
}

tDouble tClassUnit::SiScaleFactor() const {
    if (m_Family == t_UnitFamily::None || IsEmpty()) {
        return 1.0;
    }
    if (m_Family == t_UnitFamily::Monetary) {
        return 1.0;
    }
    tDouble wBase = 1.0;
    switch (m_Family) {
        case t_UnitFamily::Length:
            wBase = RapportLengthToMeter(m_Union.m_Length);
            break;
        case t_UnitFamily::Mass:
            wBase = RapportMassToGram(m_Union.m_Mass);
            break;
        case t_UnitFamily::Time:
            wBase = RapportTimeToSecond(m_Union.m_Time);
            break;
        default:
            return 1.0;
    }
    const tDouble wExp = static_cast<tDouble>(m_Power);
    return std::pow(wBase, wExp);
}

tDouble tClassUnit::ToSi(tDouble raw) const {
    return raw * SiScaleFactor();
}

tDouble tClassUnit::FromSi(tDouble si) const {
    const tDouble wF = SiScaleFactor();
    if (wF == 0.0) {
        return si;
    }
    return si / wF;
}

tClassUnit SiCanonicalUnit(t_UnitFamily sFamily, tShort sPower) {
    switch (sFamily) {
        case t_UnitFamily::Length: {
            tClassUnit wOut(t_UnitLength::meter);
            wOut.Power(sPower);
            return wOut;
        }
        case t_UnitFamily::Mass: {
            tClassUnit wOut(t_UnitMass::g);
            wOut.Power(sPower);
            return wOut;
        }
        case t_UnitFamily::Time: {
            tClassUnit wOut(t_UnitTime::second);
            wOut.Power(sPower);
            return wOut;
        }
        default:
            break;
    }
    return tClassUnit();
}


void tClassUnit::IncPower(tShort sValue) { m_Power+=sValue; }
void tClassUnit::DecPower(tShort sValue) {
    m_Power-=sValue;
    //if (m_Power<0) Clear();
}

void tClassUnit::Json(rapidjson::Writer<rapidjson::StringBuffer>* sWriter) {
    sWriter->StartObject();
    switch (m_Family) {
        case t_UnitFamily::Monetary : sWriter->Key("m"); sWriter->Int(tInt(m_Union.m_Money)); break;
        case t_UnitFamily::Length : sWriter->Key("l"); sWriter->Int(tInt(m_Union.m_Length)); break;
        case t_UnitFamily::Time : sWriter->Key("t"); sWriter->Int(tInt(m_Union.m_Time)); break;
        case t_UnitFamily::Mass: sWriter->Key("ma"); sWriter->Int(tInt(m_Union.m_Mass)); break;
        default:
            break;
    }
    if (m_Power != 1) {
        sWriter->Key("pw");
        sWriter->Int(tInt(m_Power));
    }
    sWriter->EndObject();
}

void tClassUnit::Json(const rapidjson::Value& sValue) {
    Clear();
    if (sValue.HasMember("m")) {
        m_Family=t_UnitFamily::Monetary;
        m_Union.m_Money=t_UnitMoney(sValue["m"].GetInt());
    }
    if (sValue.HasMember("l")) {
        m_Family=t_UnitFamily::Length;
        m_Union.m_Length=t_UnitLength(sValue["l"].GetInt());
    }
    if (sValue.HasMember("t")) {
        m_Family=t_UnitFamily::Time;
        m_Union.m_Time=t_UnitTime(sValue["t"].GetInt());
    }
    if (sValue.HasMember("ma")) {
        m_Family=t_UnitFamily::Mass;
        m_Union.m_Mass=t_UnitMass(sValue["ma"].GetInt());
    }
    if (sValue.HasMember("pw")) {
        Power(static_cast<tShort>(sValue["pw"].GetInt()));
    }
}

tBool tClassUnit::IsSameFamily(tClassUnit& sClassUnit) {
    return(m_Family==sClassUnit.m_Family);
}

tBool tClassUnit::IsSameUnity(tClassUnit& sClassUnit) {
    if ((m_Family!=sClassUnit.m_Family)) return(false);
    switch (m_Family) {
        case t_UnitFamily::None : return(true);
        case t_UnitFamily::Monetary : return(m_Union.m_Money==sClassUnit.m_Union.m_Money);
        case t_UnitFamily::Length : return(m_Union.m_Length==sClassUnit.m_Union.m_Length);
        case t_UnitFamily::Time :  return(m_Union.m_Time==sClassUnit.m_Union.m_Time);
        case t_UnitFamily::Mass :return(m_Union.m_Mass==sClassUnit.m_Union.m_Mass);
        case t_UnitFamily::AmountOfSubstance : return(false);
        case t_UnitFamily::ElectricCurrent : return(false);
        case t_UnitFamily::TemperatureKelvin : return(false);
        case t_UnitFamily::LuminousIntensityCandela : return(false);
        default:
            break;
    }
    return(false);
}

tBool tClassUnit::operator == (tClassUnit& sClassUnit) {
    if ((IsSameUnity(sClassUnit)) && (m_Power==sClassUnit.m_Power)) return(true);
    return(false);
}

tString tClassUnit::Str() const {
    tStringStream wStream;
    
    wStream <<  GetSymbol();
    if (m_Power!=1) wStream << m_Power;
    return(wStream.str());
}

#ifdef _DEBUGSK
tString tClassUnit::Debug() const {
    return(Str());
}
#endif

} // end of NameSpace

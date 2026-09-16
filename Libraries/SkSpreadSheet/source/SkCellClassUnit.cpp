//=============================================================================
// SkSpreadSheet CellClassAtribute
//=============================================================================
#include "../include/SkCellClassUnit.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkSpreadSheet.hpp"

#ifdef SK_DEBUG
 #define debugcellclassunit
#endif

namespace SkSpreadSheet {

namespace {

    /// Extract scalar for unit arithmetic: double or int; null/other types fail (no C++ exception).
    tBool NumericScalarFromVariant(const tVariant& sVal, tDouble* outScalar) {
        if (sVal.IsNull()) {
            return false;
        }
        switch (sVal.Type()) {
            case tVariantType::t_double:
                *outScalar = sVal.Double();
                return true;
            case tVariantType::t_int:
                *outScalar = static_cast<tDouble>(sVal.Int());
                return true;
            default:
                break;
        }
        return false;
    }

    // Length, mass, time: SI factors from SkUnit tables; money and other families return false.
    tBool ClassUnitUsesSiScaling(const SkRoot::tClassUnit& sUnit) {
        if (sUnit.IsEmpty()) {
            return false;
        }
        switch (sUnit.Family()) {
            case SkRoot::t_UnitFamily::Length:
            case SkRoot::t_UnitFamily::Mass:
            case SkRoot::t_UnitFamily::Time:
                return true;
            default:
                return false;
        }
    }

    /// Display scalar -> SI magnitude for compound unit num/den (e.g. m/s).
    tDouble ScalarToSiCompound(tDouble raw, const tClassUnit& num, const tClassUnit& den) {
        if (den.IsEmpty() || den.Family() == t_UnitFamily::None) {
            return num.ToSi(raw);
        }
        const tDouble wN = num.SiScaleFactor();
        const tDouble wD = den.SiScaleFactor();
        if (wD == 0.0) {
            return raw * wN;
        }
        return raw * wN / wD;
    }

    /// SI magnitude -> display scalar for compound unit num/den.
    tDouble ScalarFromSiCompound(tDouble si, const tClassUnit& num, const tClassUnit& den) {
        if (num.IsEmpty() || num.Family() == t_UnitFamily::None) {
            return si;
        }
        if (den.IsEmpty() || den.Family() == t_UnitFamily::None) {
            return num.FromSi(si);
        }
        const tDouble wN = num.SiScaleFactor();
        const tDouble wD = den.SiScaleFactor();
        if (wN == 0.0) {
            return si;
        }
        return si * wD / wN;
    }

    tBool CompoundUsesSiScaling(const tClassUnit& num, const tClassUnit& den) {
        if (!ClassUnitUsesSiScaling(num)) {
            return false;
        }
        if (!den.IsEmpty() && den.Family() != t_UnitFamily::None) {
            if (!ClassUnitUsesSiScaling(den)) {
                return false;
            }
        }
        return true;
    }

    /// Same physical dimension for addition (e.g. any two velocities L^1/T^1).
    tBool CompoundDimensionsMatchForAdd(const tClassUnit& nL, const tClassUnit& dL,
                                        const tClassUnit& nR, const tClassUnit& dR) {
        if (nL.Family() != nR.Family() || nL.Power() != nR.Power()) {
            return false;
        }
        const tBool wHasDenL = !dL.IsEmpty() && dL.Family() != t_UnitFamily::None;
        const tBool wHasDenR = !dR.IsEmpty() && dR.Family() != t_UnitFamily::None;
        if (wHasDenL != wHasDenR) {
            return false;
        }
        if (!wHasDenL) {
            return true;
        }
        return dL.Family() == dR.Family() && dL.Power() == dR.Power();
    }

} // namespace

    const tString StaticClassName  = "tCellUnit";

    //! tCellModelClassUnit =============================================================
    tCellModelClassUnit::tCellModelClassUnit(tString sName,tString sLabel, tFunctionCreate sFunctionCreate) : tCellModelClass(sName,sLabel,"Unit", sFunctionCreate) {}

    tBool tCellModelClassUnit::SaveModel() { return(false); }
    tBool tCellModelClassUnit::SaveData() { return(true); }

	// tCellClassUnit =============================================================
    tCellClassUnit::tCellClassUnit() : tCellClass(),m_Error(false) {}
    tCellClassUnit::tCellClassUnit(const tVariant& sValue, const tClassUnit& sClassUnit) : tCellClass(sValue),m_UnitClass(sClassUnit),m_Error(false) {}

    tCellClassUnit::tCellClassUnit(const tVariant& sValue, const tClassUnit& sNumerator, const tClassUnit& sDenominator)
        : tCellClass(sValue), m_UnitClass(sNumerator), m_UnitClassFrac(sDenominator), m_Error(false) {}

    tCellClassUnit::tCellClassUnit(const tCellClassUnit& sCellClassUnit) : tCellClass(sCellClassUnit) {
        m_UnitClass=sCellClassUnit.m_UnitClass;
        m_UnitClassFrac=sCellClassUnit.m_UnitClassFrac;
        m_Error=sCellClassUnit.m_Error;
    }

	tCellClassUnit::~tCellClassUnit() {}

    tVirtualClass* tCellClassUnit::Clone() { return new tCellClassUnit(*this); }


	tString tCellClassUnit::ClassName() const { return(StaticClassName); }

    void tCellClassUnit::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        m_Value.Json(sWriter);
        sWriter->Key("u");
        m_UnitClass.Json(sWriter);
        if (m_UnitClassFrac.Family()!=t_UnitFamily::None) {
            sWriter->Key("f");
            m_UnitClassFrac.Json(sWriter);
        }
        if (m_Error) {
            sWriter->Key("e");
            sWriter->Bool(true);
        }
    
        sWriter->EndObject();
    }

    void tCellClassUnit::Json(const rapidjson::Value& sValue) {
        //const Value& wValue=sValue["v"];
        m_Value.Json(sValue);
        const rapidjson::Value& wValueUnit=sValue["u"];
        m_UnitClass.Json(wValueUnit);
        
        if (sValue.HasMember("f")) {
            const rapidjson::Value& wValueUnitFrac=sValue["f"];
            m_UnitClassFrac.Json(wValueUnitFrac);
        }
        if (sValue.HasMember("e")) {
            m_Error=sValue["e"].GetBool();
        }
    }
    tBool tCellClassUnit::IsJsonJavaScript() { return(true); };

    void tCellClassUnit::JsonJavaScript(Writer<StringBuffer>* sWriter) {
        // Content
        sWriter->StartObject();
        Value()->Json(sWriter);
        sWriter->Key("u");
        sWriter->String(m_UnitClass.GetSymbol().c_str());
        // Always emit p for L/M/T (IsSupportPower) so the web/canvas never has to guess the dimension
        // (e.g. linear "cm" vs "cm²"). Omission used to mean p=1 but is easy to misread in clients.
        if (m_UnitClass.IsSupportPower() && m_UnitClass.Family() != t_UnitFamily::None) {
            sWriter->Key("p");
            sWriter->Int(m_UnitClass.Power());
        }
        if (!m_UnitClassFrac.IsEmpty()) {
            sWriter->Key("fu");
            sWriter->String(m_UnitClassFrac.GetSymbol().c_str());
            if (m_UnitClassFrac.IsSupportPower() && m_UnitClassFrac.Family() != t_UnitFamily::None) {
                sWriter->Key("fp");
                sWriter->Int(m_UnitClassFrac.Power());
            }
        }
        if (m_Error) {
            sWriter->Key("e");
            sWriter->Bool(true);
            sWriter->Key("se");
            sWriter->String("😡");
        }
        sWriter->EndObject();
    }

    tVariant tCellClassUnit::Operator_plus(tBool sLeft, const tVariant& sVariant) {
        (void)sLeft;
        tVariant wVariant(sVariant);
        tBool wSiResolved = false;
        tBool wSkipNumericCombine = false;
        if (wVariant.Type() == tVariantType::t_class) {
            tVirtualClass* wClass = wVariant.Class();
            tCellClassUnit* wCellClassUnit = dynamic_cast<tCellClassUnit*>(wClass);
            if (wCellClassUnit != nullptr) {
                if (wCellClassUnit->m_Error) {
                    m_Error = true;
                }
                const tBool wFracAny =
                    !m_UnitClassFrac.IsEmpty() || !wCellClassUnit->m_UnitClassFrac.IsEmpty();
                if (wFracAny) {
                    if (CompoundUsesSiScaling(m_UnitClass, m_UnitClassFrac)
                        && CompoundUsesSiScaling(wCellClassUnit->m_UnitClass, wCellClassUnit->m_UnitClassFrac)
                        && CompoundDimensionsMatchForAdd(m_UnitClass, m_UnitClassFrac,
                                                         wCellClassUnit->m_UnitClass,
                                                         wCellClassUnit->m_UnitClassFrac)) {
                        tDouble wDl = 0.0;
                        tDouble wDr = 0.0;
                        if (!NumericScalarFromVariant(*Value(), &wDl)
                            || !NumericScalarFromVariant(*wCellClassUnit->Value(), &wDr)) {
                            m_Error = true;
                        } else {
                            const tDouble wSiSum =
                                ScalarToSiCompound(wDl, m_UnitClass, m_UnitClassFrac)
                                + ScalarToSiCompound(wDr, wCellClassUnit->m_UnitClass, wCellClassUnit->m_UnitClassFrac);
                            m_Value = ScalarFromSiCompound(wSiSum, m_UnitClass, m_UnitClassFrac);
                            wSiResolved = true;
                        }
                    } else {
                        if (m_UnitClass != wCellClassUnit->m_UnitClass) {
                            m_Error = true;
                        }
                        if (m_UnitClassFrac != wCellClassUnit->m_UnitClassFrac) {
                            m_Error = true;
                        }
                    }
                } else if (m_UnitClass.Family() == t_UnitFamily::Monetary) {
                    if (m_UnitClass != wCellClassUnit->m_UnitClass) {
                        m_Error = true;
                    }
                } else if (ClassUnitUsesSiScaling(m_UnitClass)
                           && ClassUnitUsesSiScaling(wCellClassUnit->m_UnitClass)) {
                    if (!m_UnitClass.IsSameFamily(wCellClassUnit->m_UnitClass)) {
                        m_Error = true;
                        wSkipNumericCombine = true;
                    } else if (m_UnitClass.Power() != wCellClassUnit->m_UnitClass.Power()) {
                        m_Error = true;
                        wSkipNumericCombine = true;
                    } else {
                        tDouble wDl = 0.0;
                        tDouble wDr = 0.0;
                        if (!NumericScalarFromVariant(*Value(), &wDl)
                            || !NumericScalarFromVariant(*wCellClassUnit->Value(), &wDr)) {
                            m_Error = true;
                            wSkipNumericCombine = true;
                        } else {
                            const tDouble wSiSum =
                                m_UnitClass.ToSi(wDl) + wCellClassUnit->m_UnitClass.ToSi(wDr);
                            m_Value = m_UnitClass.FromSi(wSiSum);
                            wSiResolved = true;
                        }
                    }
                } else {
                    if (m_UnitClass != wCellClassUnit->m_UnitClass) {
                        m_Error = true;
                    }
                    if (m_UnitClassFrac != wCellClassUnit->m_UnitClassFrac) {
                        m_Error = true;
                    }
                }
            }
            tVariant wCopy = *(wClass->Value());
            wVariant = wCopy;
        }
    #ifdef debugcellclassunit
        cout << "-->" << m_Value << "+" << wVariant << "=";
    #endif
        if (!wSiResolved && !wSkipNumericCombine) {
            m_Value = m_Value + wVariant;
        }
    #ifdef debugcellclassunit
        cout << Debug() << endl;
    #endif
        return (tVariant(this));
    }
        
    tVariant tCellClassUnit::Operator_minus(tBool sLeft, const tVariant& sVariant) {
        tVariant wVariant(sVariant);
        tBool wSiResolved = false;
        tBool wSkipNumericCombine = false;
        if (wVariant.Type() == tVariantType::t_class) {
            tVirtualClass* wClass = wVariant.Class();
            tCellClassUnit* wCellClassUnit = dynamic_cast<tCellClassUnit*>(wClass);
            if (wCellClassUnit != nullptr) {
                if (wCellClassUnit->m_Error) {
                    m_Error = true;
                }
                const tBool wFracAny =
                    !m_UnitClassFrac.IsEmpty() || !wCellClassUnit->m_UnitClassFrac.IsEmpty();
                if (wFracAny) {
                    if (CompoundUsesSiScaling(m_UnitClass, m_UnitClassFrac)
                        && CompoundUsesSiScaling(wCellClassUnit->m_UnitClass, wCellClassUnit->m_UnitClassFrac)
                        && CompoundDimensionsMatchForAdd(m_UnitClass, m_UnitClassFrac,
                                                         wCellClassUnit->m_UnitClass,
                                                         wCellClassUnit->m_UnitClassFrac)) {
                        tDouble wDl = 0.0;
                        tDouble wDr = 0.0;
                        if (!NumericScalarFromVariant(*Value(), &wDl)
                            || !NumericScalarFromVariant(*wCellClassUnit->Value(), &wDr)) {
                            m_Error = true;
                        } else {
                            const tDouble wSiL = ScalarToSiCompound(wDl, m_UnitClass, m_UnitClassFrac);
                            const tDouble wSiR =
                                ScalarToSiCompound(wDr, wCellClassUnit->m_UnitClass, wCellClassUnit->m_UnitClassFrac);
                            const tDouble wSiOut = sLeft ? (wSiL - wSiR) : (wSiR - wSiL);
                            m_Value = ScalarFromSiCompound(wSiOut, m_UnitClass, m_UnitClassFrac);
                            wSiResolved = true;
                        }
                    } else {
                        if (m_UnitClass != wCellClassUnit->m_UnitClass) {
                            m_Error = true;
                        }
                        if (m_UnitClassFrac != wCellClassUnit->m_UnitClassFrac) {
                            m_Error = true;
                        }
                    }
                } else if (m_UnitClass.Family() == t_UnitFamily::Monetary) {
                    if (m_UnitClass != wCellClassUnit->m_UnitClass) {
                        m_Error = true;
                    }
                } else if (ClassUnitUsesSiScaling(m_UnitClass)
                           && ClassUnitUsesSiScaling(wCellClassUnit->m_UnitClass)) {
                    if (!m_UnitClass.IsSameFamily(wCellClassUnit->m_UnitClass)) {
                        m_Error = true;
                        wSkipNumericCombine = true;
                    } else if (m_UnitClass.Power() != wCellClassUnit->m_UnitClass.Power()) {
                        m_Error = true;
                        wSkipNumericCombine = true;
                    } else {
                        tDouble wDl = 0.0;
                        tDouble wDr = 0.0;
                        if (!NumericScalarFromVariant(*Value(), &wDl)
                            || !NumericScalarFromVariant(*wCellClassUnit->Value(), &wDr)) {
                            m_Error = true;
                            wSkipNumericCombine = true;
                        } else {
                            const tDouble wSiL = m_UnitClass.ToSi(wDl);
                            const tDouble wSiR = wCellClassUnit->m_UnitClass.ToSi(wDr);
                            const tDouble wSiOut = sLeft ? (wSiL - wSiR) : (wSiR - wSiL);
                            m_Value = m_UnitClass.FromSi(wSiOut);
                            wSiResolved = true;
                        }
                    }
                } else {
                    if (m_UnitClass != wCellClassUnit->m_UnitClass) {
                        m_Error = true;
                    }
                    if (m_UnitClassFrac != wCellClassUnit->m_UnitClassFrac) {
                        m_Error = true;
                    }
                }
            }
            tVariant wCopy = *(wClass->Value());
            wVariant = wCopy;
        }
        if (!wSiResolved && !wSkipNumericCombine) {
            if (sLeft) {
    #ifdef debugcellclassunit
                cout << "-->" << m_Value << "-" << wVariant << "=";
    #endif
                m_Value = m_Value - wVariant;
            } else {
    #ifdef debugcellclassunit
                cout << "-->" << wVariant << "-" << m_Value << "=";
    #endif
                m_Value = wVariant - m_Value;
            }
        }
    #ifdef debugcellclassunit
        cout << Debug() << endl;
    #endif
        return (tVariant(this));
    }

    void tCellClassUnit::MultiplyOrDivide(tClassUnit* sLeft,tClassUnit* sLeftFrac,tClassUnit* sRight,tClassUnit* sRightFrac) {
    #ifdef debugcellclassunit
        cout <<  "     ---> " << sLeft->Debug();
        if (!sLeftFrac->IsEmpty()) {
            cout << "/" << sLeftFrac->Debug();
        }
        cout << "*" << sRight->Debug();
        if (!sRightFrac->IsEmpty()) {
            cout << "/" << sRightFrac->Debug();
        }
        cout << endl;
    #endif
        // Don't change sRight
        tClassUnit wLocalRight(*sRight);
        // Right Fraction
        if (!sRightFrac->IsEmpty()) {
            // Is Same unit Reduces the unit
            if (sLeft->IsSameUnity(*sRightFrac)) {
                sLeft->DecPower(sRightFrac->Power());
            } else {
                if (!sLeft->IsSupportPower()) {
                    m_Error=true;
                } else {
                    if (!sLeft->IsEmpty()) {
                        if (!sLeft->IsSameFamily(*sRight)) { m_Error=true; } else {
                            if (!sLeft->IsSupportPower()) { m_Error=true; } else {
                                sLeft->IncPower(wLocalRight.Power());
                                // Not power for sLeftFrac
                                wLocalRight.Power(0);
                            }
                        }
                    }
                }
            }
        }
        // Left  Fraction
        if (!sLeftFrac->IsEmpty()) {
            if (sRight->IsSameUnity(*sLeftFrac)) {
                wLocalRight.DecPower(sLeftFrac->Power());
                sLeftFrac->DecPower(sRight->Power());
            } else {
                if (!sRight->IsEmpty()) {
                    if (!sLeft->IsSameFamily(*sRight)) { m_Error=true; } else {
                        if (!sLeft->IsSupportPower()) { m_Error=true; } else
                            sLeft->IncPower(wLocalRight.Power());
                    }
                }
            }
        }
        
        if (sLeft->Power()==0) {
            *sLeft=wLocalRight;
            if (sLeft->Power()<0) {
                m_Error=true;
            } else {
                if (sLeft->Power()==0) sLeft->Clear();
            }
        }
        
        if (sLeftFrac->Power()<=0) {
            if (sLeftFrac->Power()<0) {
                m_Error=true;
            } else {
                if (sLeftFrac->Power()==0) sLeftFrac->Clear();
            }
        }
    }

    tVariant tCellClassUnit::Operator_multiply(tBool sLeft, const tVariant& sVariant) {
        (void)sLeft;
        tVariant wVariant(sVariant);
        tBool wSiMulResolved = false;
        tBool wSkipNumericCombine = false;
        if (wVariant.Type() == tVariantType::t_class) {
            tVirtualClass* wClass = wVariant.Class();
            tCellClassUnit* wCellClassUnit = dynamic_cast<tCellClassUnit*>(wClass);
            // Multiply Length or time
            if (wCellClassUnit != nullptr) {
                if (wCellClassUnit->m_Error) {
                    m_Error = true;
                }
                tClassUnit* wLeft = &m_UnitClass;
                tClassUnit* wLeftFrac = &m_UnitClassFrac;
                tClassUnit* wRight = &wCellClassUnit->m_UnitClass;
                tClassUnit* wRightFrac = &wCellClassUnit->m_UnitClassFrac;

    #ifdef debugcellclassunit
                cout <<  "Multiply " << wLeft->Debug();
                if (!wLeftFrac->IsEmpty()) {
                    cout << "/" << wLeftFrac->Debug();
                }
                cout << " * " << wRight->Debug();
                if (!wRightFrac->IsEmpty()) {
                    cout << "/" << wRightFrac->Debug();
                }
                cout << endl;
    #endif

                // Is Fraction ===================================================
                if ((!wRightFrac->IsEmpty() || (!wLeftFrac->IsEmpty()))) {
                    const tClassUnit wSnapLNum = m_UnitClass;
                    const tClassUnit wSnapLFrac = m_UnitClassFrac;
                    const tClassUnit wSnapRNum = wCellClassUnit->m_UnitClass;
                    const tClassUnit wSnapRFrac = wCellClassUnit->m_UnitClassFrac;
                    tDouble wVl = 0.0;
                    tDouble wVr = 0.0;
                    const tBool wScalarsOk =
                        NumericScalarFromVariant(*Value(), &wVl)
                        && NumericScalarFromVariant(*wCellClassUnit->Value(), &wVr);
                    const tBool wCanSi = wScalarsOk
                        && CompoundUsesSiScaling(wSnapLNum, wSnapLFrac)
                        && CompoundUsesSiScaling(wSnapRNum, wSnapRFrac);
                    if (!wScalarsOk) {
                        m_Error = true;
                        wSkipNumericCombine = true;
                    } else {
                        MultiplyOrDivide(wLeft, wLeftFrac, wRight, wRightFrac);
                        if (!m_Error && wCanSi) {
                            const tDouble wSiProd =
                                ScalarToSiCompound(wVl, wSnapLNum, wSnapLFrac)
                                * ScalarToSiCompound(wVr, wSnapRNum, wSnapRFrac);
                            m_Value = ScalarFromSiCompound(wSiProd, m_UnitClass, m_UnitClassFrac);
                            wSiMulResolved = true;
                        }
                    }
                    if (m_Error) {
                        wSkipNumericCombine = true;
                    }
                } else {
                    if (!wLeft->IsSameFamily(*wRight)) {
                        m_Error = true;
                        wSkipNumericCombine = true;
                    } else if (!wLeft->IsSupportPower() || !wRight->IsSupportPower()) {
                        m_Error = true;
                        wSkipNumericCombine = true;
                    } else if (ClassUnitUsesSiScaling(*wLeft) && ClassUnitUsesSiScaling(*wRight)) {
                        tDouble wVl = 0.0;
                        tDouble wVr = 0.0;
                        if (!NumericScalarFromVariant(*Value(), &wVl)
                            || !NumericScalarFromVariant(*wCellClassUnit->Value(), &wVr)) {
                            m_Error = true;
                            wSkipNumericCombine = true;
                        } else {
                            const tDouble wSiProd =
                                m_UnitClass.ToSi(wVl) * wCellClassUnit->m_UnitClass.ToSi(wVr);
                            const tShort wNewPow = static_cast<tShort>(
                                m_UnitClass.Power() + wCellClassUnit->m_UnitClass.Power());
                            if (wNewPow == 0) {
                                m_UnitClass.Clear();
                                m_UnitClassFrac.Clear();
                                m_Value = wSiProd;
                            } else {
                                // Same concrete unit on both sides (e.g. cm·cm): keep it with combined power (cm²), not SI canonical (m²).
                                if (wLeft->IsSameUnity(*wRight)) {
                                    SkRoot::tClassUnit wPreserved(*wLeft);
                                    wPreserved.Power(wNewPow);
                                    m_UnitClass = wPreserved;
                                } else {
                                    m_UnitClass = SkRoot::SiCanonicalUnit(m_UnitClass.Family(), wNewPow);
                                }
                                m_Value = m_UnitClass.FromSi(wSiProd);
                            }
                            wSiMulResolved = true;
                        }
                    } else {
                        m_Error = true;
                        wSkipNumericCombine = true;
                    }
                }
            }
            tVariant wCopy = *(wClass->Value());
            wVariant = wCopy;
        }
    #ifdef debugcellclassunit
        cout << "-->" << m_Value << "*" << wVariant << "=";
    #endif
        if (!wSiMulResolved && !wSkipNumericCombine) {
            m_Value = m_Value * wVariant;
        }
    #ifdef debugcellclassunit
        cout << Debug() << endl;
    #endif
        if (m_UnitClass.IsEmpty() && m_UnitClassFrac.IsEmpty()) {
            return(m_Value);
        }
        return(tVariant(this));
    }
        
    tVariant tCellClassUnit::Operator_divide(tBool sLeft, const tVariant& sVariant) {
        tVariant wVariant(sVariant);
        tBool wSiDivideResolved = false;
        tBool wSkipNumericCombine = false;
        if (wVariant.Type() == tVariantType::t_class) {
            tVirtualClass* wClass=wVariant.Class();
            tCellClassUnit* wCellClassUnit=dynamic_cast<tCellClassUnit*>(wClass);
            // Divie
            if (wCellClassUnit!=nullptr) {
                if (wCellClassUnit->m_Error) m_Error=true;
                tClassUnit wLeft=m_UnitClass;
                tClassUnit wLeftFrac=m_UnitClassFrac;
                tClassUnit wRight=wCellClassUnit->m_UnitClass;
                tClassUnit wRightFrac=wCellClassUnit->m_UnitClassFrac;

    #ifdef debugcellclassunit
                cout <<  "Divide " << wLeft.Debug();
                if (!wLeftFrac.IsEmpty()) {
                    cout << "/" << wLeftFrac.Debug();
                }
                cout << " / " << wRight.Debug();
                if (!wRightFrac.IsEmpty()) {
                    cout << "/" << wRightFrac.Debug();
                }
                cout << endl;
    #endif

                // Invert
                // Is Fraction ===================================================
                // Invert Fraction an multiply
                if ((!wRightFrac.IsEmpty() || (!wLeftFrac.IsEmpty()))) {
                    const tClassUnit wSnapLNum = wLeft;
                    const tClassUnit wSnapLFrac = wLeftFrac;
                    const tClassUnit wSnapRNum = wRight;
                    const tClassUnit wSnapRFrac = wRightFrac;
                    tDouble wVl = 0.0;
                    tDouble wVr = 0.0;
                    const tBool wScalarsOk =
                        NumericScalarFromVariant(*Value(), &wVl)
                        && NumericScalarFromVariant(*wCellClassUnit->Value(), &wVr);
                    const tBool wCanSi = wScalarsOk
                        && CompoundUsesSiScaling(wSnapLNum, wSnapLFrac)
                        && CompoundUsesSiScaling(wSnapRNum, wSnapRFrac);
                    if (!wScalarsOk) {
                        m_Error = true;
                        wSkipNumericCombine = true;
                    } else {
                        if (sLeft) {
                            MultiplyOrDivide(&wLeft, &wLeftFrac, &wRightFrac,&wRight);
                            m_UnitClass=wLeft;
                            m_UnitClassFrac=wLeftFrac;
                        } else {
                            MultiplyOrDivide(&wRight, &wRightFrac, &wLeftFrac,&wRight);
                            m_UnitClass=wRight;
                            m_UnitClassFrac=wRightFrac;
                        }
                        if (!m_Error && wCanSi) {
                        const tDouble wSiL = ScalarToSiCompound(wVl, wSnapLNum, wSnapLFrac);
                        const tDouble wSiR = ScalarToSiCompound(wVr, wSnapRNum, wSnapRFrac);
                        const tDouble wDenom = sLeft ? wSiR : wSiL;
                        const tDouble wNumer = sLeft ? wSiL : wSiR;
                        if (wDenom == 0.0) {
                            m_Error = true;
                            wSkipNumericCombine = true;
                        } else {
                            const tDouble wRat = wNumer / wDenom;
                            m_Value = ScalarFromSiCompound(wRat, m_UnitClass, m_UnitClassFrac);
                            wSiDivideResolved = true;
                        }
                        }
                    }
                    if (m_Error) {
                        wSkipNumericCombine = true;
                    }
                } else {
                    if (m_UnitClass.IsSameUnity(wCellClassUnit->m_UnitClass)) {
                        m_UnitClass.DecPower(1);
                        if (m_UnitClass.Power()==0) m_UnitClass.Clear();
                    } else if (ClassUnitUsesSiScaling(wLeft) && ClassUnitUsesSiScaling(wRight)
                               && wLeft.IsSameFamily(wRight) && wLeft.Power()==wRight.Power()) {
                        tDouble wDl = 0.0;
                        tDouble wDr = 0.0;
                        if (!NumericScalarFromVariant(*Value(), &wDl)
                            || !NumericScalarFromVariant(*wCellClassUnit->Value(), &wDr)) {
                            m_Error = true;
                            wSkipNumericCombine = true;
                        } else {
                            const tDouble wSiL = wLeft.ToSi(wDl);
                            const tDouble wSiR = wRight.ToSi(wDr);
                            const tDouble wDenom = sLeft ? wSiR : wSiL;
                            const tDouble wNumer = sLeft ? wSiL : wSiR;
                            if (wDenom == 0.0) {
                                m_Error = true;
                                wSkipNumericCombine = true;
                            } else {
                                const tDouble wRat = wNumer / wDenom;
                                m_UnitClass.Clear();
                                m_UnitClassFrac.Clear();
                                m_Value = wRat;
                                wSiDivideResolved = true;
                            }
                        }
                    } else {
                        if (sLeft) {
                            m_UnitClassFrac=wCellClassUnit->m_UnitClass;
                        } else {
                            m_UnitClass=wCellClassUnit->m_UnitClassFrac;
                        }
                    }
                }
            }
            // Make Copy Before assing
            tVariant wCopy=*(wClass->Value());
            wVariant=wCopy;
        }
        if (!wSiDivideResolved && !wSkipNumericCombine) {
            if (sLeft) {
    #ifdef debugcellclassunit
                cout << "-->" << m_Value << "/" << wVariant << "=";
    #endif
                m_Value = m_Value / wVariant;
            }
            else {
    #ifdef debugcellclassunit
                cout << "-->" << wVariant << "/" << m_Value << "=";
    #endif
                m_Value = wVariant / m_Value;
            }
        }
    #ifdef debugcellclassunit
        cout << Debug() << endl;
    #endif
        if (m_UnitClass.IsEmpty() && m_UnitClassFrac.IsEmpty()) {
            return(m_Value);
        }
        return(tVariant(this));
    }

#ifdef checksp
	/// @brief      Check.
	void tCellClassUnit::Check() {
		
	}
#endif
    tString tCellClassUnit::Str() {
        tStringStream wStream;
        tVariant wValue=*Value();
        
        wStream << wValue << " ";
 
        wStream << m_UnitClass.Str();
        if (m_UnitClassFrac.Family()!=t_UnitFamily::None) {
            wStream << "/" <<  m_UnitClassFrac.Str();
        }
        if (m_Error) wStream << " Error";
   
        return(wStream.str());
    }
#ifdef _DEBUGSK
	tString tCellClassUnit::Debug() {
        return(Str());
	}
#endif

    tCellClassUnit* CreateCellClassUnit() { return(new tCellClassUnit); }

    tBool RegisterCellClassUnit() {
        // Register Unit Class in ClassFactory only once
        tClassFactory* wClassFactory=tClassFactory::Instance();
        tCellModelClass* wCellModelClass=dynamic_cast<tCellModelClass*>(wClassFactory->Get(StaticClassName));
        if (wCellModelClass==nullptr) {
            tCellModelClass* wCellModelClass = new tCellModelClass(StaticClassName,"Unit","Unit", &CreateCellClassUnit);
            return(tClassFactory::Instance()->Register(wCellModelClass));
        }
        return(false);
    }

    tBool UnRegisterCellClassUnit() {
        return(tClassFactory::Instance()->UnRegister("Unit"));
    }

}

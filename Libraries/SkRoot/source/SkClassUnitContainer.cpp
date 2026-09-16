//=============================================================================
// SkClassUnitContainer — load UnitCurrencyModel.json
//=============================================================================
#include "../include/SkClassUnitContainer.hpp"
#include "../include/SkFile.hpp"

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

using namespace rapidjson;

namespace SkRoot {

    static tString SkReadStringMember(const Value& sObj, const char* sKey, const tString& sDefault = {}) {
        if (!sObj.HasMember(sKey)) return sDefault;
        const Value& wVal = sObj[sKey];
        if (!wVal.IsString()) return sDefault;
        return tString(wVal.GetString(), wVal.GetStringLength());
    }

    static void SkFillNumericDimensionMap(const Value& sObj, tDimensionExponentMap& sOut) {
        sOut.clear();
        if (!sObj.IsObject()) return;
        for (auto wIt = sObj.MemberBegin(); wIt != sObj.MemberEnd(); ++wIt) {
            const Value& wKey = wIt->name;
            const Value& wVal = wIt->value;
            if (!wKey.IsString()) continue;
            if (!wVal.IsNumber()) continue;
            int wPow = wVal.IsInt() ? wVal.GetInt() : static_cast<int>(wVal.GetDouble());
            sOut.emplace(tString(wKey.GetString(), wKey.GetStringLength()), wPow);
        }
    }

    static void SkParsePhysicalUnitRow(const Value& sRow, tClassUnitModelPhysicalUnit& sOut) {
        sOut.m_Id = SkReadStringMember(sRow, "id");
        sOut.m_Symbol = SkReadStringMember(sRow, "symbol");
        sOut.m_Label = SkReadStringMember(sRow, "label");
        if (sRow.HasMember("dimensions") && sRow["dimensions"].IsObject())
            SkFillNumericDimensionMap(sRow["dimensions"], sOut.m_Dimensions);

        sOut.m_SiEquivalent.m_HasValue = false;
        if (sRow.HasMember("siEquivalent") && sRow["siEquivalent"].IsObject()) {
            const Value& wEq = sRow["siEquivalent"];
            if (wEq.HasMember("unitId") && wEq["unitId"].IsString()) {
                sOut.m_SiEquivalent.m_HasValue = true;
                sOut.m_SiEquivalent.m_UnitId =
                    tString(wEq["unitId"].GetString(), wEq["unitId"].GetStringLength());
            }
            if (wEq.HasMember("factor") && wEq["factor"].IsNumber())
                sOut.m_SiEquivalent.m_Factor = wEq["factor"].GetDouble();
            else if (sOut.m_SiEquivalent.m_HasValue)
                sOut.m_SiEquivalent.m_Factor = 1.0;
        }
    }

    static void SkParseCurrencyRow(const Value& sRow, tClassUnitModelCurrency& sOut) {
        sOut.m_Id = SkReadStringMember(sRow, "id");
        sOut.m_Iso4217 = SkReadStringMember(sRow, "iso4217");
        sOut.m_Symbol = SkReadStringMember(sRow, "symbol");
        sOut.m_Label = SkReadStringMember(sRow, "label");
        if (sRow.HasMember("numericCode") && sRow["numericCode"].IsInt())
            sOut.m_NumericCode = sRow["numericCode"].GetInt();
        if (sRow.HasMember("minorUnits") && sRow["minorUnits"].IsInt())
            sOut.m_MinorUnits = sRow["minorUnits"].GetInt();
        if (sRow.HasMember("factor") && sRow["factor"].IsNumber())
            sOut.m_Factor = sRow["factor"].GetDouble();

        if (sRow.HasMember("dimensions") && sRow["dimensions"].IsObject()) {
            const Value& wDim = sRow["dimensions"];
            if (wDim.MemberCount() > 0) {
                for (auto wIt = wDim.MemberBegin(); wIt != wDim.MemberEnd(); ++wIt) {
                    const Value& wKey = wIt->name;
                    const Value& wVal = wIt->value;
                    if (!wKey.IsString()) continue;
                    tString wName(wKey.GetString(), wKey.GetStringLength());
                    if (wVal.IsObject() && wVal.HasMember("code") && wVal["code"].IsString()) {
                        sOut.m_CurrencyDimension.m_Code =
                            tString(wVal["code"].GetString(), wVal["code"].GetStringLength());
                        if (wVal.HasMember("power") && wVal["power"].IsInt())
                            sOut.m_CurrencyDimension.m_Power = wVal["power"].GetInt();
                        else if (wVal.HasMember("power") && wVal["power"].IsNumber())
                            sOut.m_CurrencyDimension.m_Power = static_cast<int>(wVal["power"].GetDouble());
                    }
                    (void)wName;
                }
            }
        }

        sOut.m_Tags.clear();
        if (sRow.HasMember("tags") && sRow["tags"].IsArray()) {
            const Value& wArr = sRow["tags"];
            for (SizeType wI = 0; wI < wArr.Size(); ++wI) {
                if (wArr[wI].IsString())
                    sOut.m_Tags.push_back(tString(wArr[wI].GetString(), wArr[wI].GetStringLength()));
            }
        }
    }

    static void SkParseIncompatibilitySide(const Value& sObj, tClassUnitModelIncompatibilitySide& sOut) {
        sOut.m_Kind = SkReadStringMember(sObj, "kind");
        sOut.m_Id = SkReadStringMember(sObj, "id");
        sOut.m_Signature.clear();
        if (sObj.HasMember("signature") && sObj["signature"].IsObject())
            SkFillNumericDimensionMap(sObj["signature"], sOut.m_Signature);
    }

    static void SkParseMoneyRules(const Value& sObj, tClassUnitModelMoneyRules& sOut) {
        if (sObj.HasMember("addSubtractRequireSameCurrencyDimension") &&
            sObj["addSubtractRequireSameCurrencyDimension"].IsBool())
            sOut.m_AddSubtractRequireSameCurrencyDimension =
                sObj["addSubtractRequireSameCurrencyDimension"].GetBool();

        if (sObj.HasMember("multiplyPhysicalByMoney") && sObj["multiplyPhysicalByMoney"].IsObject()) {
            const Value& wSub = sObj["multiplyPhysicalByMoney"];
            if (wSub.HasMember("allowed") && wSub["allowed"].IsBool())
                sOut.m_MultiplyPhysicalByMoneyAllowed = wSub["allowed"].GetBool();
            sOut.m_MultiplyPhysicalByMoneyDescription =
                SkReadStringMember(wSub, "description");
        }
        if (sObj.HasMember("multiplyMoneyByMoney") && sObj["multiplyMoneyByMoney"].IsObject()) {
            const Value& wSub = sObj["multiplyMoneyByMoney"];
            if (wSub.HasMember("allowed") && wSub["allowed"].IsBool())
                sOut.m_MultiplyMoneyByMoneyAllowed = wSub["allowed"].GetBool();
            sOut.m_MultiplyMoneyByMoneyReason = SkReadStringMember(wSub, "reason");
        }
        if (sObj.HasMember("roundingPolicy") && sObj["roundingPolicy"].IsObject()) {
            const Value& wSub = sObj["roundingPolicy"];
            sOut.m_RoundingMode = SkReadStringMember(wSub, "mode");
            if (wSub.HasMember("applyPerOperation") && wSub["applyPerOperation"].IsBool())
                sOut.m_RoundingApplyPerOperation = wSub["applyPerOperation"].GetBool();
        }
    }

    // --- tClassUnitContainer -------------------------------------------------

    tClassUnitContainer::tClassUnitContainer() : tClass() { ClearModel(); }

    tClassUnitContainer::tClassUnitContainer(const tClassUnitContainer& sOther) : tClass(sOther) {
        m_SchemaVersion = sOther.m_SchemaVersion;
        m_Description = sOther.m_Description;
        m_DimensionBasis = sOther.m_DimensionBasis;
        m_PhysicalUnits = sOther.m_PhysicalUnits;
        m_Currencies = sOther.m_Currencies;
        m_MoneyRules = sOther.m_MoneyRules;
        m_ExplicitIncompatibilities = sOther.m_ExplicitIncompatibilities;
        m_CompatibilityHints = sOther.m_CompatibilityHints;
        m_HasCompatibilityHints = sOther.m_HasCompatibilityHints;
        m_LastError = sOther.m_LastError;
    }

    tClassUnitContainer& tClassUnitContainer::operator=(const tClassUnitContainer& sOther) {
        if (this == &sOther) return *this;
        tClass::operator=(sOther);
        m_SchemaVersion = sOther.m_SchemaVersion;
        m_Description = sOther.m_Description;
        m_DimensionBasis = sOther.m_DimensionBasis;
        m_PhysicalUnits = sOther.m_PhysicalUnits;
        m_Currencies = sOther.m_Currencies;
        m_MoneyRules = sOther.m_MoneyRules;
        m_ExplicitIncompatibilities = sOther.m_ExplicitIncompatibilities;
        m_CompatibilityHints = sOther.m_CompatibilityHints;
        m_HasCompatibilityHints = sOther.m_HasCompatibilityHints;
        m_LastError = sOther.m_LastError;
        return *this;
    }

    tClassUnitContainer::~tClassUnitContainer() = default;

    void tClassUnitContainer::ClearModel() {
        m_SchemaVersion.clear();
        m_Description.clear();
        m_DimensionBasis.clear();
        m_PhysicalUnits.clear();
        m_Currencies.clear();
        m_MoneyRules = tClassUnitModelMoneyRules{};
        m_ExplicitIncompatibilities.clear();
        m_CompatibilityHints = tClassUnitModelCompatibilityHints{};
        m_HasCompatibilityHints = false;
        m_LastError.clear();
    }

    void tClassUnitContainer::Clear() { ClearModel(); }

    tBool tClassUnitContainer::ParseJsonString(const tString& sJsonUtf8) {
        ClearModel();
        Document wDoc;
        ParseResult wOk = wDoc.Parse(sJsonUtf8.c_str());
        if (!wOk) {
            m_LastError = "JSON parse error: ";
            m_LastError += GetParseError_En(wOk.Code());
            m_LastError += " at offset ";
            {
                tStringStream wSs;
                wSs << wOk.Offset();
                m_LastError += wSs.str();
            }
            return false;
        }
        return ParseDocument(wDoc);
    }

    tBool tClassUnitContainer::LoadFromFile(const tString& sPath) {
        ClearModel();
        tFile wFile(sPath);
        if (!wFile.Exist()) {
            m_LastError = "File not found: ";
            m_LastError += sPath;
            return false;
        }
        tString wJson = wFile.LoadString();
        return ParseJsonString(wJson);
    }

    tBool tClassUnitContainer::ParseDocument(const Document& sDoc) {
        if (!sDoc.IsObject()) {
            m_LastError = "Root JSON value must be an object.";
            return false;
        }

        m_SchemaVersion = SkReadStringMember(sDoc, "schemaVersion");
        m_Description = SkReadStringMember(sDoc, "description");

        if (sDoc.HasMember("dimensionBasis") && sDoc["dimensionBasis"].IsObject()) {
            const Value& wBasis = sDoc["dimensionBasis"];
            for (auto wIt = wBasis.MemberBegin(); wIt != wBasis.MemberEnd(); ++wIt) {
                if (!wIt->name.IsString()) continue;
                if (!wIt->value.IsString()) continue;
                tString wKey(wIt->name.GetString(), wIt->name.GetStringLength());
                tString wLab(wIt->value.GetString(), wIt->value.GetStringLength());
                m_DimensionBasis[wKey] = wLab;
            }
        }

        if (sDoc.HasMember("units") && sDoc["units"].IsArray()) {
            const Value& wArr = sDoc["units"];
            m_PhysicalUnits.reserve(wArr.Size());
            for (SizeType wI = 0; wI < wArr.Size(); ++wI) {
                if (!wArr[wI].IsObject()) continue;
                tClassUnitModelPhysicalUnit wRow;
                SkParsePhysicalUnitRow(wArr[wI], wRow);
                m_PhysicalUnits.push_back(std::move(wRow));
            }
        }

        if (sDoc.HasMember("currencies") && sDoc["currencies"].IsArray()) {
            const Value& wArr = sDoc["currencies"];
            m_Currencies.reserve(wArr.Size());
            for (SizeType wI = 0; wI < wArr.Size(); ++wI) {
                if (!wArr[wI].IsObject()) continue;
                tClassUnitModelCurrency wRow;
                SkParseCurrencyRow(wArr[wI], wRow);
                m_Currencies.push_back(std::move(wRow));
            }
        }

        if (sDoc.HasMember("moneyRules") && sDoc["moneyRules"].IsObject())
            SkParseMoneyRules(sDoc["moneyRules"], m_MoneyRules);

        if (sDoc.HasMember("explicitIncompatibilities") && sDoc["explicitIncompatibilities"].IsArray()) {
            const Value& wArr = sDoc["explicitIncompatibilities"];
            for (SizeType wI = 0; wI < wArr.Size(); ++wI) {
                if (!wArr[wI].IsObject()) continue;
                const Value& wRow = wArr[wI];
                tClassUnitModelIncompatibility wRule;
                if (wRow.HasMember("left") && wRow["left"].IsObject())
                    SkParseIncompatibilitySide(wRow["left"], wRule.m_Left);
                if (wRow.HasMember("right") && wRow["right"].IsObject())
                    SkParseIncompatibilitySide(wRow["right"], wRule.m_Right);
                if (wRow.HasMember("operations") && wRow["operations"].IsArray()) {
                    const Value& wOps = wRow["operations"];
                    for (SizeType wJ = 0; wJ < wOps.Size(); ++wJ) {
                        if (wOps[wJ].IsString())
                            wRule.m_Operations.push_back(
                                tString(wOps[wJ].GetString(), wOps[wJ].GetStringLength()));
                    }
                }
                wRule.m_Severity = SkReadStringMember(wRow, "severity");
                if (wRow.HasMember("unless") && wRow["unless"].IsArray()) {
                    const Value& wUnless = wRow["unless"];
                    for (SizeType wJ = 0; wJ < wUnless.Size(); ++wJ) {
                        if (wUnless[wJ].IsString())
                            wRule.m_Unless.push_back(
                                tString(wUnless[wJ].GetString(), wUnless[wJ].GetStringLength()));
                    }
                }
                wRule.m_MessageKey = SkReadStringMember(wRow, "messageKey");
                m_ExplicitIncompatibilities.push_back(std::move(wRule));
            }
        }

        if (sDoc.HasMember("compatibilityMatrixHints") &&
            sDoc["compatibilityMatrixHints"].IsObject()) {
            const Value& wH = sDoc["compatibilityMatrixHints"];
            m_HasCompatibilityHints = true;
            m_CompatibilityHints.m_Description = SkReadStringMember(wH, "description");
            if (wH.HasMember("sameCurrencyAddition") && wH["sameCurrencyAddition"].IsBool())
                m_CompatibilityHints.m_SameCurrencyAddition = wH["sameCurrencyAddition"].GetBool();
            if (wH.HasMember("crossCurrencyAdditionWithoutFx") &&
                wH["crossCurrencyAdditionWithoutFx"].IsBool())
                m_CompatibilityHints.m_CrossCurrencyAdditionWithoutFx =
                    wH["crossCurrencyAdditionWithoutFx"].GetBool();
        }

        m_LastError.clear();
        return true;
    }

    const tClassUnitModelPhysicalUnit* tClassUnitContainer::FindPhysicalUnit(const tString& sId) const {
        for (const auto& wU : m_PhysicalUnits) {
            if (wU.m_Id == sId) return &wU;
        }
        return nullptr;
    }

    const tClassUnitModelCurrency* tClassUnitContainer::FindCurrency(const tString& sId) const {
        for (const auto& wC : m_Currencies) {
            if (wC.m_Id == sId) return &wC;
        }
        return nullptr;
    }

} // namespace SkRoot

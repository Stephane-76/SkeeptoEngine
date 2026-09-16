//=============================================================================
// SkClassUnitContainer — UnitCurrencyModel.json registry (physical units + currencies).
//=============================================================================
#ifndef SkClassUnitContainer_hpp
#define SkClassUnitContainer_hpp

#include "SkClass.hpp"
#include "SkTypes.hpp"

#include <map>
#include <vector>

namespace SkRoot {

    typedef std::map<tString, int> tDimensionExponentMap;

    //! Optional SI conversion target for a catalog unit row.
    struct tClassUnitModelSiEquivalent {
        tBool m_HasValue = false;
        tString m_UnitId;
        tDouble m_Factor = 1.0;
    };

    //! One physical unit definition from JSON "units" array.
    struct tClassUnitModelPhysicalUnit {
        tString m_Id;
        tString m_Symbol;
        tString m_Label;
        tDimensionExponentMap m_Dimensions;
        tClassUnitModelSiEquivalent m_SiEquivalent;
    };

    //! Currency dimension payload (dimensions.CURRENCY in JSON).
    struct tClassUnitCurrencyDimension {
        tString m_Code;
        int m_Power = 1;
    };

    //! One ISO currency row from JSON "currencies" array.
    struct tClassUnitModelCurrency {
        tString m_Id;
        tString m_Iso4217;
        int m_NumericCode = 0;
        tString m_Symbol;
        tString m_Label;
        int m_MinorUnits = 2;
        tDouble m_Factor = 1.0;
        tClassUnitCurrencyDimension m_CurrencyDimension;
        tVectorString m_Tags;
    };

    //! Parsed moneyRules section.
    struct tClassUnitModelMoneyRules {
        tBool m_AddSubtractRequireSameCurrencyDimension = true;
        tBool m_MultiplyPhysicalByMoneyAllowed = true;
        tString m_MultiplyPhysicalByMoneyDescription;
        tBool m_MultiplyMoneyByMoneyAllowed = false;
        tString m_MultiplyMoneyByMoneyReason;
        tString m_RoundingMode;
        tBool m_RoundingApplyPerOperation = true;
    };

    //! Left/right operand for explicitIncompatibilities entries.
    struct tClassUnitModelIncompatibilitySide {
        tString m_Kind;
        tString m_Id;
        tDimensionExponentMap m_Signature;
    };

    //! One explicit incompatibility rule.
    struct tClassUnitModelIncompatibility {
        tClassUnitModelIncompatibilitySide m_Left;
        tClassUnitModelIncompatibilitySide m_Right;
        tVectorString m_Operations;
        tString m_Severity;
        tVectorString m_Unless;
        tString m_MessageKey;
    };

    //! compatibilityMatrixHints section.
    struct tClassUnitModelCompatibilityHints {
        tString m_Description;
        tBool m_SameCurrencyAddition = true;
        tBool m_CrossCurrencyAdditionWithoutFx = false;
    };

    //=========================================================================
    //! Holds UnitCurrencyModel.json after load (lookup + rule metadata).
    class tClassUnitContainer : public tClass {
    private:
        tString m_SchemaVersion;
        tString m_Description;
        std::map<tString, tString> m_DimensionBasis;
        std::vector<tClassUnitModelPhysicalUnit> m_PhysicalUnits;
        std::vector<tClassUnitModelCurrency> m_Currencies;
        tClassUnitModelMoneyRules m_MoneyRules;
        std::vector<tClassUnitModelIncompatibility> m_ExplicitIncompatibilities;
        tClassUnitModelCompatibilityHints m_CompatibilityHints;
        tBool m_HasCompatibilityHints = false;
        tString m_LastError;

        void ClearModel();
        tBool ParseDocument(const rapidjson::Document& sDoc);

    public:
        tClassUnitContainer();
        tClassUnitContainer(const tClassUnitContainer& sOther);
        tClassUnitContainer& operator=(const tClassUnitContainer& sOther);
        ~tClassUnitContainer();

        /// @brief Reset all loaded data.
        void Clear();

        /// @brief Last parse or I/O error message (empty if none).
        tString LastError() const { return m_LastError; }

        /// @brief Load from UTF-8 JSON file path (uses tFile).
        tBool LoadFromFile(const tString& sPath);

        /// @brief Parse UTF-8 JSON string (UnitCurrencyModel schema).
        tBool ParseJsonString(const tString& sJsonUtf8);

        const tString& SchemaVersion() const { return m_SchemaVersion; }
        const tString& Description() const { return m_Description; }
        const std::map<tString, tString>& DimensionBasis() const { return m_DimensionBasis; }
        const std::vector<tClassUnitModelPhysicalUnit>& PhysicalUnits() const { return m_PhysicalUnits; }
        const std::vector<tClassUnitModelCurrency>& Currencies() const { return m_Currencies; }
        const tClassUnitModelMoneyRules& MoneyRules() const { return m_MoneyRules; }
        const std::vector<tClassUnitModelIncompatibility>& ExplicitIncompatibilities() const {
            return m_ExplicitIncompatibilities;
        }
        tBool HasCompatibilityHints() const { return m_HasCompatibilityHints; }
        const tClassUnitModelCompatibilityHints& CompatibilityHints() const { return m_CompatibilityHints; }

        /// @brief Find physical unit by id, or nullptr.
        const tClassUnitModelPhysicalUnit* FindPhysicalUnit(const tString& sId) const;

        /// @brief Find currency by id, or nullptr.
        const tClassUnitModelCurrency* FindCurrency(const tString& sId) const;
    };

} // namespace SkRoot

#endif

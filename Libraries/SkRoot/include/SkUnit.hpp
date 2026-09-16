//=============================================================================
// SkUnit  see SkSpreadSheet tCellClassUnit
// 17/01/2025
//=============================================================================
#ifndef SkUnit_hpp
#define SkUnit_hpp

#include "SkTypes.hpp"
#include "SkClass.hpp"

namespace SkRoot {

    enum  class t_UnitFamily : tByte {
        None=0,
        Monetary,
        Length, // - meter (m)
        Time, // - second (s)
        Mass, // - kilogram (kg)
        AmountOfSubstance, // - mole (mol)
        ElectricCurrent, // - ampere (A)
        TemperatureKelvin, // (K)
        LuminousIntensityCandela, // (cd)
    };

    struct tRecUnitFamily {
        t_UnitFamily m_Family;
        tString m_Name;
    };

    const tRecUnitFamily CstRecUnitFamily[] = {
        { t_UnitFamily::None ,"None"},
        { t_UnitFamily::Monetary ,"Monetary"},
        { t_UnitFamily::Length ,"Length"},
        { t_UnitFamily::Time, "Time" },
        { t_UnitFamily::Mass, "Mass" },
        { t_UnitFamily::AmountOfSubstance,"Substance" },
        { t_UnitFamily::ElectricCurrent,"Electric" },
        { t_UnitFamily::TemperatureKelvin, "Temperature" },
        { t_UnitFamily::LuminousIntensityCandela, "Luminous" }
    };

    // Money ==================================================================
    enum class t_UnitMoney : tByte {
        None,
        eur=1, // Euro
        usd, // usd
        gpb, // British pound (legacy enum name)
        yen, // yen
        chf,
        cad,
        aud
    };

    struct tRecUnitMoney {
        t_UnitMoney  m_Money;
        tString      m_Name;
        tString      m_Symbol;
    };

    const tRecUnitMoney CstRecUnitMoney[] = {
        { t_UnitMoney::eur, "eur", "€" },
        { t_UnitMoney::usd, "usd", "$" },
        { t_UnitMoney::gpb, "gpb", "£" },
        { t_UnitMoney::yen, "yen", "¥" },
        { t_UnitMoney::chf, "chf", "CHF" },
        { t_UnitMoney::cad, "cad", "CA$" },
        { t_UnitMoney::aud, "aud", "A$" },
    };

    // Length =================================================================
    enum class t_UnitLength : tByte {
        None,
        micron = 1, // SI micrometer, 1e-6 m
        millimeter,
        centimeter,
        meter,
        kilometer,
    };

    struct tRecUnitLength {
        t_UnitLength m_Length;
        tString m_Name;
        tString m_Symbol;
        /// Factor to meter (1 m = 1).
        tDouble m_Rapport;
    };

    const tRecUnitLength CstRecUnitLength[] = {
        { t_UnitLength::micron, "micron", "um", 1e-6 },
        { t_UnitLength::millimeter, "millimeter", "mm", 1e-3 },
        { t_UnitLength::centimeter, "centimeter", "cm", 1e-2 },
        { t_UnitLength::meter, "meter", "m", 1.0 },
        { t_UnitLength::kilometer, "kilometer", "km", 1e3 },
    };

    // Time ===================================================================
    enum class t_UnitTime : tByte {
        None,
        millisecond=1, // millisecond
        second,  // second  1
        minute,  // minute, 1*60
        hour,  // hours,  1*3600
        day,  // day     1*3600*24
        month,  // month
        year,  // year
    };

    struct tRecUnitTime {
        t_UnitTime  m_Time;
        tString     m_Name;
        tString     m_Symbol;
        tDouble     m_Rapport;
    };

    const tRecUnitTime CstRecUnitTime[] = {
        { t_UnitTime::millisecond, "millisecond", "ms", 1e-3 },
        { t_UnitTime::second, "second", "s", 1.0 },
        { t_UnitTime::minute, "minute", "min", 60.0 },
        { t_UnitTime::hour, "hour", "h", 3600.0 },
        { t_UnitTime::day, "day", "d", 86400.0 },
        { t_UnitTime::month, "month", "mo", 3600.0 * 24 * 30 },
        { t_UnitTime::year, "year", "y", 3600.0 * 24 * 365 },
    };

    // Mass ==================================================================
    enum class t_UnitMass : tByte {
        None,
        mg = 1,
        g,
        kg,
        to,
    };

    struct tRecUnitMass {
        t_UnitMass m_Mass;
        tString m_Name;
        tString m_Symbol;
        /// Factor to gram (gram = 1).
        tDouble m_Rapport;
    };

    const tRecUnitMass CstRecUnitMass[] = {
        { t_UnitMass::mg, "milligram", "mg", 1e-3 },
        { t_UnitMass::g, "g", "g", 1.0 },
        { t_UnitMass::kg, "kilogram", "kg", 1e3 },
        { t_UnitMass::to, "tonne", "t", 1e6 },
    };

    // Container Unit ========================================================
    union tUnit_Union {
        enum t_UnitMoney    m_Money;
        enum t_UnitLength   m_Length;
        enum t_UnitTime     m_Time;
        enum t_UnitMass     m_Mass;
    };

    // Function ==============================================================
    t_UnitFamily UnitFamily(tString sValue);
    t_UnitMoney  UnitMoney(tString sValue);
    t_UnitLength UnitLength(tString sValue);
    t_UnitTime UnitTime(tString sValue);
    t_UnitMass UnitMass(tString sValue);

    tString UnitMoneyStr(t_UnitMoney sValue);

    tString UnitMoneySymbol(t_UnitMoney sValue);

    class tClassUnit : public tClass {
    private:
        t_UnitFamily   m_Family;
        tUnit_Union    m_Union;
        tShort         m_Power;
    public:
        /// @brief Constructor
        tClassUnit();
        
        /// @brief Constructor with money unit
        /// @param[in] sMoney t_UnitMoney
        tClassUnit(t_UnitMoney sMoney);
        
        /// @brief Constructor with length unit
        /// @param[in] sLength t_UnitLength
        tClassUnit(t_UnitLength sLength);
        
        /// @brief Constructor with time unit
        /// @param[in] sTime t_UnitTime
        tClassUnit(t_UnitTime sTime);
        
        /// @brief Constructor with mass unit
        /// @param[in] sMass t_UnitMass
        tClassUnit(t_UnitMass sMass);
        
        /// @brief Copy constructor
        /// @param[in] sUnitClass tClassUnit&
        tClassUnit(const tClassUnit& sUnitClass);
        
        /// @brief Clear the unit
        void Clear();
        
        /// @brief Get the unit family
        /// @return t_UnitFamily
        t_UnitFamily Family() const;
        
        /// @brief Get the power of the unit
        /// @return tShort
        tShort Power() const;
        
        /// @brief Set the power of the unit
        /// @param[in] sValue tShort
        void Power(tShort sValue);
        
        /// @brief Return symbol of unit.
        /// @return tString
        tString GetSymbol() const;
        
        /// @brief Check if the unit is empty
        /// @return tBool
        tBool IsEmpty() const;
        
        /// @brief Check if the unit supports power
        /// @return tBool
        tBool IsSupportPower() const;
        
        /// @brief Increment the power of the unit
        /// @param[in] sValue tShort
        void IncPower(tShort sValue);
        
        /// @brief Decrement the power of the unit
        /// @param[in] sValue tShort
        void DecPower(tShort sValue);
        
        /// @brief Writer Json.
        /// @param[in] sWriter rapidjson::Writer<rapidjson::StringBuffer>*
        void Json(rapidjson::Writer<rapidjson::StringBuffer>* sWriter);
        
        /// @brief Reader Json.
        /// @param[in] sValue Value&
        void Json(const rapidjson::Value& sValue);
        
        // React =============================================================
        /// @brief Writer Json for Javasript.
        /// @param[in] sWriter rapidjson::Writer<rapidjson::StringBuffer>*
        void JsonJavaScript(rapidjson::Writer<rapidjson::StringBuffer>* sWriter);
        
        /// @brief Check if the unit is of the same family
        /// @param[in] sClassUnit tClassUnit&
        /// @return tBool
        tBool IsSameFamily(tClassUnit& sClassUnit);
        
        /// @brief Check if the unit is the same
        /// @param[in] sClassUnit tClassUnit&
        /// @return tBool
        tBool IsSameUnity(tClassUnit& sClassUnit);
        
        /// @brief Equality operator
        /// @param[in] sClassUnit tClassUnit&
        /// @return tBool
        tBool operator == (tClassUnit& sClassUnit);
        
        /// @brief Debug.
        /// @return tString
        tString Str() const;

        /// @brief pow(rapport_to_SI_base, Power); quantity_SI = raw * SiScaleFactor().
        tDouble SiScaleFactor() const;

        /// @brief Map display scalar into SI canonical quantity for this family (length→m^p, mass→g^p, time→s^p).
        tDouble ToSi(tDouble raw) const;

        /// @brief Map SI canonical quantity back into this unit's scalar.
        tDouble FromSi(tDouble si) const;

    #ifdef _DEBUGSK
        /// @brief Debug.
        tString Debug() const;
    #endif
    };

    /// Build canonical SI base unit (meter, gram, second) with exponent for multiply/divide results.
    tClassUnit SiCanonicalUnit(t_UnitFamily sFamily, tShort sPower);

} // end of Namespace


#endif

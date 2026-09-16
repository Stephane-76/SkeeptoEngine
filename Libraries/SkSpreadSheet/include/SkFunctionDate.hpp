//=============================================================================
// SkSpreadSheet Function Logical
//=============================================================================
#ifndef SkFunctionDate_hpp
#define SkFunctionDate_hpp

#include "SkFunction.hpp"

namespace SkSpreadSheet {
    
    //=========================================================================
    //! Function Today
    class tFunctionToday : public tFunction {
    public:
        /// @brief		Constructor tFunctionToday.
        tFunctionToday();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! Function Now
    class tFunctionNow : public tFunction {
    public:
        /// @brief		Constructor tFunctionNow.
        tFunctionNow();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //=========================================================================
    //! Function Date
    class tFunctionDateCreate : public tFunction {
    public:
        /// @brief		Constructor tFunctionDateCreate.
        tFunctionDateCreate();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    class tFunctionDate : public tFunction {
    public:
        /// YEAR, MONTH, DAY, etc.: element-wise on date ranges in Excel 365.
        tFunctionSpillKind SpillKind() const override;
    };

    //! Function Year
    class tFunctionYear : public tFunctionDate {
    public:
        /// @brief        Constructor tFunctionYear.
        tFunctionYear();

        /// @brief        Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //! Function Month
    class tFunctionMonth : public tFunctionDate {
    public:
        /// @brief        Constructor tFunctionMonth.
        tFunctionMonth();

        /// @brief        Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //! Function Day
    class tFunctionDay : public tFunctionDate {
    public:
        /// @brief        Constructor tFunctionDay.
        tFunctionDay();

        /// @brief        Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //! FunctionWeek Day
    class tFunctionWeekDay : public tFunctionDate {
    public:
        /// @brief        Constructor tFunctionWeekDay.
        tFunctionWeekDay();

        /// @brief        Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //! Function WeekNum
    class tFunctionWeekNum : public tFunctionDate {
    public:
        /// @brief        Constructor tFunctionWeekNum.
        tFunctionWeekNum();

        /// @brief        Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //! Function Time
    class tFunctionTime : public tFunctionDate {
    public:
        /// @brief        Constructor tFunctionTime.
        tFunctionTime();

        /// @brief        Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    class tFunctionHour : public tFunctionDate {
    public:
        /// @brief        Constructor tFunctionHour.
        tFunctionHour();

        /// @brief        Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    class tFunctionMinute : public tFunctionDate {
    public:
        /// @brief        Constructor tFunctionMinute.
        tFunctionMinute();

        /// @brief        Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    class tFunctionSecond : public tFunctionDate {
    public:
        /// @brief        Constructor tFunctionSecond.
        tFunctionSecond();

        /// @brief        Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //! Function EDATE - Add months to a date
    class tFunctionEdate : public tFunctionDate {
    public:
        /// @brief        Constructor tFunctionEdate.
        tFunctionEdate();

        /// @brief        Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// @return     tVariant
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //! Function EOMONTH - Last day of the month, offset by months
    class tFunctionEomonth : public tFunctionDate {
    public:
        /// @brief        Constructor tFunctionEomonth.
        tFunctionEomonth();

        /// @brief        Call function with arguments.
        /// @param[in]  sStackElems stack of arguments
        /// @param[in]  sNbArg number of arguments
        /// @return     tVariant date (last day of target month)
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //! Function DATEVALUE - parse date text (locale formats) to a date serial
    class tFunctionDateValue : public tFunctionDate {
    public:
        tFunctionDateValue();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //! DAYS(end_date, start_date) — whole days between two dates.
    class tFunctionDays : public tFunction {
    public:
        tFunctionDays();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
        tFunctionSpillKind SpillKind() const override;
    };

    //! DAYS360(start_date, end_date, [method]) — days on a 360-day year.
    //! method FALSE/omitted = US (NASD); TRUE = European.
    class tFunctionDays360 : public tFunction {
    public:
        tFunctionDays360();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
        tFunctionSpillKind SpillKind() const override;
    };

    //! ISOWEEKNUM(date) — ISO 8601 week number (same as tClassDate::WeekNum).
    class tFunctionIsoWeekNum : public tFunctionDate {
    public:
        tFunctionIsoWeekNum();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //! TIMEVALUE(time_text) — parse time text or serial to fraction of day [0,1).
    class tFunctionTimeValue : public tFunctionDate {
    public:
        tFunctionTimeValue();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
    };

    //! DATEDIF(start_date, end_date, unit) — Y/M/D/MD/YM/YD.
    class tFunctionDateDif : public tFunction {
    public:
        tFunctionDateDif();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
        tFunctionSpillKind SpillKind() const override;
    };

    //! YEARFRAC(start_date, end_date, [basis]) — year fraction, basis 0..4.
    class tFunctionYearFrac : public tFunction {
    public:
        tFunctionYearFrac();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
        tFunctionSpillKind SpillKind() const override;
    };

    //! Function NETWORKDAYS - count working days (Mon–Fri) between two dates.
    //! NETWORKDAYS(start_date, end_date, [holidays])
    class tFunctionNetworkDays : public tFunction {
    public:
        tFunctionNetworkDays();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
        tFunctionSpillKind SpillKind() const override;
    };

    //! NETWORKDAYS.INTL (registered NETWORKDAYS_INTL) — weekend mask + holidays.
    //! NETWORKDAYS_INTL(start_date, end_date, [weekend], [holidays])
    class tFunctionNetworkDaysIntl : public tFunction {
    public:
        tFunctionNetworkDaysIntl();
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
        tFunctionSpillKind SpillKind() const override;
    };

    //! WORKDAY / WORKDAY.INTL — date that is N working days before/after start_date.
    //! WORKDAY(start_date, days, [holidays]) — weekend Sat/Sun.
    //! WORKDAY_INTL(start_date, days, [weekend], [holidays]) — custom weekend mask.
    class tFunctionWorkDay : public tFunction {
    private:
        tBool m_Intl;
    public:
        explicit tFunctionWorkDay(tBool sIntl = false);
        tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
        tFunctionSpillKind SpillKind() const override;
    };

}; // End of namespace

#endif

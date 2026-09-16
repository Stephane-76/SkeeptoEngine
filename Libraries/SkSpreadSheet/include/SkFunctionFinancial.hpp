//=========================================================================
// Financial functions
//=========================================================================
#ifndef SkFunctionFinancial_hpp
#define SkFunctionFinancial_hpp

#include "SkFunction.hpp"
#include <string>

namespace SkSpreadSheet {

    //=========================================================================
    //! Base class for financial functions with helper methods
    //! This reduces code duplication by providing common argument extraction
    //! Methods are marked SkInline to allow compiler optimization
    class tFunctionFinancialBase : public tFunction {
    protected:
        /// @brief Extract a double argument from reversed vector
        /// @param sArgVector Argument vector (reversed by parser)
        /// @param index Index in reversed vector (0 = last argument, size-1 = first argument)
        /// @param errorPrefix Prefix for error messages (function name)
        /// @param argName Name of argument for error messages
        /// @param outValue Output parameter for extracted value
        /// @param defaultValue Default value if argument is optional and missing
        /// @param isOptional Whether argument is optional
        /// @return Error variant if extraction failed, empty variant if success
        SkInline tVariant ExtractDoubleArg(std::vector<tStackElem>* wArgs, size_t index, const char* errorPrefix, const char* argName, tDouble& outValue, tDouble defaultValue = 0.0, bool isOptional = false);
        
        /// @brief Extract an integer argument from vector
        /// @param wArgs Argument vector (from PopArgs)
        /// @param index Index in vector (0 = first arg in RPN order)
        /// @param errorPrefix Prefix for error messages (function name)
        /// @param argName Name of argument for error messages
        /// @param outValue Output parameter for extracted value
        /// @param defaultValue Default value if argument is optional and missing
        /// @param isOptional Whether argument is optional
        /// @return Error variant if extraction failed, empty variant if success
        SkInline tVariant ExtractIntArg(std::vector<tStackElem>* wArgs, size_t index, const char* errorPrefix, const char* argName, tInt& outValue, tInt defaultValue = 0, bool isOptional = false);
        
        /// @brief Extract a range argument from vector
        /// @param wArgs Argument vector (from PopArgs)
        /// @param index Index in vector (0 = first arg in RPN order)
        /// @param errorPrefix Prefix for error messages (function name)
        /// @param argName Name of argument for error messages
        /// @param outValue Output parameter for extracted range pointer
        /// @return Error variant if extraction failed, empty variant if success
        SkInline tVariant ExtractRangeArg(std::vector<tStackElem>* wArgs, size_t index, const char* errorPrefix, const char* argName, tRange*& outValue);
    public:
        tFunctionSpillKind SpillKind() const override;
    };


    //=========================================================================
    //! Function PMT - Payment
    class tFunctionPmt : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionPmt.
        tFunctionPmt();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// PMT(rate, nper, pv, [fv], [type])
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function IPMT - Interest Payment
    class tFunctionIpmt : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionIpmt.
        tFunctionIpmt();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// IPMT(rate, per, nper, pv, [fv], [type])
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function PPMT - Principal Payment
    class tFunctionPpmt : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionPpmt.
        tFunctionPpmt();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// PPMT(rate, per, nper, pv, [fv], [type])
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function PV - Present Value
    class tFunctionPv : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionPv.
        tFunctionPv();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// PV(rate, nper, pmt, [fv], [type])
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function FV - Future Value
    class tFunctionFv : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionFv.
        tFunctionFv();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// FV(rate, nper, pmt, [pv], [type])
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function RATE - Interest Rate
    class tFunctionRate : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionRate.
        tFunctionRate();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// RATE(nper, pmt, pv, [fv], [type], [guess])
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function NPER - Number of Periods
    class tFunctionNper : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionNper.
        tFunctionNper();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// NPER(rate, pmt, pv, [fv], [type])
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function NPV - Net Present Value
    class tFunctionNpv : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionNpv.
        tFunctionNpv();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// NPV(rate, value1, [value2], ...)
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function IRR - Internal Rate of Return
    class tFunctionIrr : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionIrr.
        tFunctionIrr();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// IRR(values, [guess])
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function MIRR - Modified Internal Rate of Return
    class tFunctionMirr : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionMirr.
        tFunctionMirr();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// MIRR(values, finance_rate, reinvest_rate)
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function DB - Declining Balance Depreciation
    class tFunctionDb : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionDb.
        tFunctionDb();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// DB(cost, salvage, life, period, [month])
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function DDB - Double Declining Balance Depreciation
    class tFunctionDdb : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionDdb.
        tFunctionDdb();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// DDB(cost, salvage, life, period, [factor])
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function SLN - Straight Line Depreciation
    class tFunctionSln : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionSln.
        tFunctionSln();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// SLN(cost, salvage, life)
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function SYD - Sum of Years Digits Depreciation
    class tFunctionSyd : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionSyd.
        tFunctionSyd();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// SYD(cost, salvage, life, per)
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function CUMIPMT - Cumulative Interest Payment
    class tFunctionCumipmt : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionCumipmt.
        tFunctionCumipmt();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// CUMIPMT(rate, nper, pv, start_period, end_period, type)
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function CUMPRINC - Cumulative Principal Payment
    class tFunctionCumprinc : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionCumprinc.
        tFunctionCumprinc();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// CUMPRINC(rate, nper, pv, start_period, end_period, type)
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function EFFECT - Effective Annual Interest Rate
    class tFunctionEffect : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionEffect.
        tFunctionEffect();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// EFFECT(nominal_rate, npery)
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function NOMINAL - Nominal Annual Interest Rate
    class tFunctionNominal : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionNominal.
        tFunctionNominal();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// NOMINAL(effect_rate, npery)
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function XIRR - Internal Rate of Return for Non-Periodic Cash Flows
    class tFunctionXirr : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionXirr.
        tFunctionXirr();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// XIRR(values, dates, [guess])
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function XNPV - Net Present Value for Non-Periodic Cash Flows
    class tFunctionXnpv : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionXnpv.
        tFunctionXnpv();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// XNPV(rate, values, dates)
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function VDB - Variable Declining Balance Depreciation
    class tFunctionVdb : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionVdb.
        tFunctionVdb();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// VDB(cost, salvage, life, start_period, end_period, [factor], [no_switch])
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function FVSCHEDULE - Future Value with Variable Interest Rates
    class tFunctionFvschedule : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionFvschedule.
        tFunctionFvschedule();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// FVSCHEDULE(principal, schedule)
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function PDURATION - Number of Periods to Reach Value
    class tFunctionPduration : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionPduration.
        tFunctionPduration();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// PDURATION(rate, pv, fv)
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

    //=========================================================================
    //! Function RRI - Equivalent Interest Rate
    class tFunctionRri : public tFunctionFinancialBase {
    public:
        /// @brief		Constructor SkFunctionRri.
        tFunctionRri();

        /// @brief		Call function with arguments.
        /// @param[in]  sArgVector tStackElemVector* vector of arguments
        /// RRI(nper, pv, fv)
        virtual tStackElem Call(tStackElems* sStackElems, tShort sNbArg);
    };

} // namespace SkSpreadSheet

#endif

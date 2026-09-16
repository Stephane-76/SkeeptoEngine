//=========================================================================
// Financial functions
//=========================================================================
#include <algorithm>
#include <cmath>
#include <vector>
#include "../include/SkFunctionFinancial.hpp"

namespace SkSpreadSheet {



    //=========================================================================
    //! SkInline implementations for performance (compiler can SkInline these)
    //! String concatenation only happens on error path (rare case)
    tVariant tFunctionFinancialBase::ExtractDoubleArg(std::vector<tStackElem>* wArgs, size_t index, const char* errorPrefix, const char* argName, tDouble& outValue, tDouble defaultValue, bool isOptional) {
        if (index >= wArgs->size()) {
            if (isOptional) {
                outValue = defaultValue;
                return tVariant(); // Empty variant indicates success
            }
            // String creation only on error path (rare)
            return tVariant(tClassError(tTypeError::t_arg, std::string(errorPrefix) + ": " + argName + ": argument missing"));
        }
        tStackElem& wArg = wArgs->at(index);
        tVariant wValue;
        if (!StackElemToVariant(wArg, wValue)) {
            return tVariant(tClassError(tTypeError::t_arg, std::string(errorPrefix) + ": " + argName + ": must be a value or single cell"));
        }
        
        // If Error (Propagate)
        if (wValue.IsError()) {
            #ifdef _debug_calculation
            std::cout << "DEBUG: ExtractDoubleArg() for " << errorPrefix << "::" << argName << " - propagating error: " << wValue.Str() << std::endl;
            #endif
            return(wValue);
        }
        
        // If variant is null, return 0
        if (wValue.IsNull()) {
            outValue = 0.0;
            return tVariant(); // Success
        }
        
        if (!wValue.IsInt() && !wValue.IsDouble()) {
            #ifdef _debug_calculation
            std::cout << "DEBUG: ExtractDoubleArg() for " << errorPrefix << "::" << argName << " - non-numeric value: type=" << static_cast<int>(wValue.Type()) << ", value='" << wValue.Str() << "'" << std::endl;
            #endif
            return tVariant(tClassError(tTypeError::t_arg, std::string(errorPrefix) + ": " + argName + ": must be numeric"));
        }
        if (wValue.IsDouble()) {
            outValue = wValue.Double();
            return tVariant(); // Success
        } else {
            outValue = static_cast<tDouble>(wValue.Int());
            return tVariant(); // Success
        }
    }
    
    tVariant tFunctionFinancialBase::ExtractIntArg(std::vector<tStackElem>* wArgs, size_t index, const char* errorPrefix, const char* argName, tInt& outValue, tInt defaultValue, bool isOptional) {
        if (index >= wArgs->size()) {
            if (isOptional) {
                outValue = defaultValue;
                return tVariant(); // Success - no string allocation
            }
            return tVariant(tClassError(tTypeError::t_arg, std::string(errorPrefix) + ": " + argName + ": argument missing"));
        }
        tStackElem& wArg = wArgs->at(index);
        tVariant wValue;
        if (!StackElemToVariant(wArg, wValue)) {
            return tVariant(tClassError(tTypeError::t_arg, std::string(errorPrefix) + ": " + argName + ": must be a value or single cell"));
        }
        
        // If Error (Propagate)
        if (wValue.IsError()) {
            return(wValue);
        }
        
        // If variant is null, return 0
        if (wValue.IsNull()) {
            outValue = 0;
            return tVariant(); // Success
        }
        
        if (!wValue.IsInt() && !wValue.IsDouble()) {
            return tVariant(tClassError(tTypeError::t_arg, std::string(errorPrefix) + ": " + argName + ": must be numeric"));
        }
        if (wValue.IsDouble()) {
            outValue = static_cast<tInt>(wValue.Double());
            return tVariant(); // Success
        } else {
            outValue = wValue.Int();
            return tVariant(); // Success
        }
    }
        
    
    tVariant tFunctionFinancialBase::ExtractRangeArg(std::vector<tStackElem>* wArgs, size_t index, const char* errorPrefix, const char* argName, tRange*& outValue) {
        if (index >= wArgs->size()) {
            return tVariant(tClassError(tTypeError::t_arg, std::string(errorPrefix) + ": " + argName + ": argument missing"));
        }
        
        tStackElem& wArg = wArgs->at(index);
        if (wArg.Type() == tStackType::t_Range) {
            outValue = wArg.Range();
            return tVariant(); // Success - no string allocation
        } else {
            return tVariant(tClassError(tTypeError::t_arg, std::string(errorPrefix) + ": " + argName + ": must be a range"));
        }
    }

    //=========================================================================
    //! Helper methods are now SkInline in header for better performance

    //=========================================================================
    //! Function PMT - Payment
    tFunctionPmt::tFunctionPmt() : tFunctionFinancialBase() {}

    tStackElem tFunctionPmt::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 3 || wArgs.size() > 5) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "PMT requires 3 to 5 arguments"))));
        }

        // PMT(rate, nper, pv, [fv], [type])
        // In RPN order: 
        //   3 args: pv nper rate PMT -> wArgs[0]=pv, wArgs[1]=nper, wArgs[2]=rate
        //   4 args: fv pv nper rate PMT -> wArgs[0]=fv, wArgs[1]=pv, wArgs[2]=nper, wArgs[3]=rate
        //   5 args: type fv pv nper rate PMT -> wArgs[0]=type, wArgs[1]=fv, wArgs[2]=pv, wArgs[3]=nper, wArgs[4]=rate
        tDouble wRate = 0.0;
        tDouble wNper = 0.0;
        tDouble wPv = 0.0;
        tDouble wFv = 0.0;
        tInt wType = 0;
        tVariant wError;
        
        // Calculate indices dynamically based on wArgs.size()
        // Pattern: rate=size-1, nper=size-2, pv=size-3, fv=size-4, type=size-5
        size_t wSize = wArgs.size();
        
        // Extract rate (always at index size-1)
        wError = ExtractDoubleArg(&wArgs, wSize - 1, "PMT", "rate", wRate);
        if (wError.IsError()) return wError;
        
        // Extract nper (always at index size-2)
        wError = ExtractDoubleArg(&wArgs, wSize - 2, "PMT", "nper", wNper);
        if (wError.IsError()) return wError;
        
        // Extract pv (always at index size-3)
        wError = ExtractDoubleArg(&wArgs, wSize - 3, "PMT", "pv", wPv);
        if (wError.IsError()) return wError;
        
        // Extract fv (at index size-4 if size >= 4, optional)
        if (wSize >= 4) {
            wError = ExtractDoubleArg(&wArgs, wSize - 4, "PMT", "fv", wFv, 0.0, true);
            if (wError.IsError()) return wError;
        }
        
        // Extract type (at index size-5 if size == 5, optional)
        if (wSize == 5) {
            wError = ExtractIntArg(&wArgs, wSize - 5, "PMT", "type", wType, 0, true);
            if (wError.IsError()) return wError;
        }
        
        // Clean up arguments

        // Validate inputs
        if (wNper <= 0.0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "PMT: nper must be positive"))));
        }

        if (wRate == 0.0) {
            // If rate is 0, PMT = -(pv + fv) / nper
            tDouble wPmtZeroRate = -(wPv + wFv) / wNper;
            return(tStackElem(tVariant(wPmtZeroRate)));
        }

        // Calculate PMT using standard formula
        // PMT = -pv * (rate * (1 + rate)^nper) / ((1 + rate)^nper - 1) - fv * rate / ((1 + rate)^nper - 1)
        tDouble wFactor = pow(1.0 + wRate, wNper);
        tDouble wPmt = -(wPv * wFactor + wFv) * wRate / (wFactor - 1.0);

        // Adjust for payment timing (type: 0 = end of period, 1 = beginning)
        if (wType == 1) {
            wPmt = wPmt / (1.0 + wRate);
        }

        return(tStackElem(tVariant(wPmt)));
    }

    //=========================================================================
    //! Function IPMT - Interest Payment
    tFunctionIpmt::tFunctionIpmt() : tFunctionFinancialBase() {}

    tStackElem tFunctionIpmt::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 4 || wArgs.size() > 6) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IPMT requires 4 to 6 arguments"))));
        }

        // IPMT(rate, per, nper, pv, [fv], [type])
        // In RPN order: 
        //   4 args: pv nper per rate IPMT -> wArgs[0]=pv, wArgs[1]=nper, wArgs[2]=per, wArgs[3]=rate
        //   5 args: fv pv nper per rate IPMT -> wArgs[0]=fv, wArgs[1]=pv, wArgs[2]=nper, wArgs[3]=per, wArgs[4]=rate
        //   6 args: type fv pv nper per rate IPMT -> wArgs[0]=type, wArgs[1]=fv, wArgs[2]=pv, wArgs[3]=nper, wArgs[4]=per, wArgs[5]=rate
        tDouble wRate = 0.0;
        tDouble wPer = 0.0;
        tDouble wNper = 0.0;
        tDouble wPv = 0.0;
        tDouble wFv = 0.0;
        tInt wType = 0;
        tVariant wError;
        
        // Calculate indices dynamically based on wArgs.size()
        // Pattern: rate=size-1, per=size-2, nper=size-3, pv=size-4, fv=size-5, type=size-6
        size_t wSize = wArgs.size();
        
        // Extract rate (always at index size-1)
        wError = ExtractDoubleArg(&wArgs, wSize - 1, "IPMT", "rate", wRate);
        if (wError.IsError()) return wError;
        
        // Extract per (always at index size-2)
        wError = ExtractDoubleArg(&wArgs, wSize - 2, "IPMT", "per", wPer);
        if (wError.IsError()) return wError;
        
        // Extract nper (always at index size-3)
        wError = ExtractDoubleArg(&wArgs, wSize - 3, "IPMT", "nper", wNper);
        if (wError.IsError()) return wError;
        
        // Extract pv (always at index size-4)
        wError = ExtractDoubleArg(&wArgs, wSize - 4, "IPMT", "pv", wPv);
        if (wError.IsError()) return wError;
        
        // Extract fv (at index size-5 if size >= 5, optional)
        if (wSize >= 5) {
            wError = ExtractDoubleArg(&wArgs, wSize - 5, "IPMT", "fv", wFv, 0.0, true);
            if (wError.IsError()) return wError;
        }
        
        // Extract type (at index size-6 if size == 6, optional)
        if (wSize == 6) {
            wError = ExtractIntArg(&wArgs, wSize - 6, "IPMT", "type", wType, 0, true);
            if (wError.IsError()) return wError;
        }
        
        // Clean up arguments

        // Validate inputs
        if (wPer < 1.0 || wPer > wNper) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "IPMT: per must be between 1 and nper"))));
        }

        if (wNper <= 0.0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "IPMT: nper must be positive"))));
        }

        // Calculate PMT first
        tDouble wPmt = 0.0;
        if (wRate == 0.0) {
            wPmt = -(wPv + wFv) / wNper;
        } else {
            tDouble wFactor = pow(1.0 + wRate, wNper);
            wPmt = -(wPv * wFactor + wFv) * wRate / (wFactor - 1.0);
            if (wType == 1) {
                wPmt = wPmt / (1.0 + wRate);
            }
        }

        // Calculate IPMT for period 'per'
        // IPMT = (PV * rate + PMT) * (1 + rate)^(per-1) - PMT
        // This formula calculates interest payment for period 'per'
        tDouble wIpmt = 0.0;
        if (wRate == 0.0) {
            wIpmt = 0.0;
        } else {
            tDouble wPrevFactor = pow(1.0 + wRate, wPer - 1.0);
            // Standard Excel formula for IPMT
            wIpmt = (wPv * wRate + wPmt) * wPrevFactor - wPmt;
        }

        return(tStackElem(tVariant(-wIpmt))); // Return negative (payment)
    }

    //=========================================================================
    //! Function PPMT - Principal Payment
    tFunctionPpmt::tFunctionPpmt() : tFunctionFinancialBase() {}

    tStackElem tFunctionPpmt::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 4 || wArgs.size() > 6) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "PPMT requires 4 to 6 arguments"))));
        }

        // PPMT(rate, per, nper, pv, [fv], [type])
        // In RPN order: 
        //   4 args: pv nper per rate PPMT -> wArgs[0]=pv, wArgs[1]=nper, wArgs[2]=per, wArgs[3]=rate
        //   5 args: fv pv nper per rate PPMT -> wArgs[0]=fv, wArgs[1]=pv, wArgs[2]=nper, wArgs[3]=per, wArgs[4]=rate
        //   6 args: type fv pv nper per rate PPMT -> wArgs[0]=type, wArgs[1]=fv, wArgs[2]=pv, wArgs[3]=nper, wArgs[4]=per, wArgs[5]=rate
        tDouble wRate = 0.0;
        tDouble wPer = 0.0;
        tDouble wNper = 0.0;
        tDouble wPv = 0.0;
        tDouble wFv = 0.0;
        tInt wType = 0;
        tVariant wError;
        
        // Calculate indices dynamically based on wArgs.size()
        // Pattern: rate=size-1, per=size-2, nper=size-3, pv=size-4, fv=size-5, type=size-6
        size_t wSize = wArgs.size();
        
        // Extract rate (always at index size-1)
        wError = ExtractDoubleArg(&wArgs, wSize - 1, "PPMT", "rate", wRate);
        if (wError.IsError()) return wError;
        
        // Extract per (always at index size-2)
        wError = ExtractDoubleArg(&wArgs, wSize - 2, "PPMT", "per", wPer);
        if (wError.IsError()) return wError;
        
        // Extract nper (always at index size-3)
        wError = ExtractDoubleArg(&wArgs, wSize - 3, "PPMT", "nper", wNper);
        if (wError.IsError()) return wError;
        
        // Extract pv (always at index size-4)
        wError = ExtractDoubleArg(&wArgs, wSize - 4, "PPMT", "pv", wPv);
        if (wError.IsError()) return wError;
        
        // Extract fv (at index size-5 if size >= 5, optional)
        if (wSize >= 5) {
            wError = ExtractDoubleArg(&wArgs, wSize - 5, "PPMT", "fv", wFv, 0.0, true);
            if (wError.IsError()) return wError;
        }
        
        // Extract type (at index size-6 if size == 6, optional)
        if (wSize == 6) {
            wError = ExtractIntArg(&wArgs, wSize - 6, "PPMT", "type", wType, 0, true);
            if (wError.IsError()) return wError;
        }
        
        // Clean up arguments

        // Validate inputs
        if (wPer < 1.0 || wPer > wNper) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "PPMT: per must be between 1 and nper"))));
        }

        if (wNper <= 0.0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "PPMT: nper must be positive"))));
        }

        // Calculate PMT
        tDouble wPmt = 0.0;
        if (wRate == 0.0) {
            wPmt = -(wPv + wFv) / wNper;
        } else {
            tDouble wFactor = pow(1.0 + wRate, wNper);
            wPmt = -(wPv * wFactor + wFv) * wRate / (wFactor - 1.0);
            if (wType == 1) {
                wPmt = wPmt / (1.0 + wRate);
            }
        }

        // Calculate IPMT for period 'per' (same formula as IPMT function)
        tDouble wIpmt = 0.0;
        if (wRate == 0.0) {
            wIpmt = 0.0;
        } else {
            tDouble wPrevFactor = pow(1.0 + wRate, wPer - 1.0);
            // Standard Excel formula for IPMT
            wIpmt = (wPv * wRate + wPmt) * wPrevFactor - wPmt;
        }

        // PPMT = PMT - IPMT
        // The IPMT function returns -wIpmt (negative value, e.g., -833.33)
        // Excel's PPMT formula: PPMT = PMT - IPMT
        // So: PPMT = PMT - (-wIpmt) = PMT + wIpmt
        // Example: PMT = -1073.64, IPMT = -833.33, so PPMT = -1073.64 - (-833.33) = -240.31
        tDouble wPpmt = wPmt + wIpmt;
        
        // Debug for F6 calculation
        // F6 should be around 240.31 for first period with rate=0.05/12, per=1, nper=360, pv=200000
        // If wPpmt is around -1906.98, it suggests PMT or IPMT calculation is wrong
        // Expected: PMT ≈ -1073.64, IPMT ≈ -833.33, so PPMT ≈ -240.31

        return(tStackElem(tVariant(wPpmt))); // Return negative (payment)
    }

    //=========================================================================
    //! Function PV - Present Value
    tFunctionPv::tFunctionPv() : tFunctionFinancialBase() {}

    tStackElem tFunctionPv::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 3 || wArgs.size() > 5) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "PV requires 3 to 5 arguments"))));
        }

        // PV(rate, nper, pmt, [fv], [type])
        // In RPN order: 
        //   3 args: pmt nper rate PV -> wArgs[0]=pmt, wArgs[1]=nper, wArgs[2]=rate
        //   4 args: fv pmt nper rate PV -> wArgs[0]=fv, wArgs[1]=pmt, wArgs[2]=nper, wArgs[3]=rate
        //   5 args: type fv pmt nper rate PV -> wArgs[0]=type, wArgs[1]=fv, wArgs[2]=pmt, wArgs[3]=nper, wArgs[4]=rate
        tDouble wRate = 0.0;
        tDouble wNper = 0.0;
        tDouble wPmt = 0.0;
        tDouble wFv = 0.0;
        tInt wType = 0;
        tVariant wError;
        
        // Calculate indices dynamically based on wArgs.size()
        // Pattern: rate=size-1, nper=size-2, pmt=size-3, fv=size-4, type=size-5
        size_t wSize = wArgs.size();
        
        // Extract rate (always at index size-1)
        wError = ExtractDoubleArg(&wArgs, wSize - 1, "PV", "rate", wRate);
        if (wError.IsError()) return wError;
        
        // Extract nper (always at index size-2)
        wError = ExtractDoubleArg(&wArgs, wSize - 2, "PV", "nper", wNper);
        if (wError.IsError()) return wError;
        
        // Extract pmt (always at index size-3)
        wError = ExtractDoubleArg(&wArgs, wSize - 3, "PV", "pmt", wPmt);
        if (wError.IsError()) return wError;
        
        // Extract fv (at index size-4 if size >= 4, optional)
        if (wSize >= 4) {
            wError = ExtractDoubleArg(&wArgs, wSize - 4, "PV", "fv", wFv, 0.0, true);
            if (wError.IsError()) return wError;
        }
        
        // Extract type (at index size-5 if size == 5, optional)
        if (wSize == 5) {
            wError = ExtractIntArg(&wArgs, wSize - 5, "PV", "type", wType, 0, true);
            if (wError.IsError()) return wError;
        }
        
        // Clean up arguments

        // Validate inputs
        if (wNper <= 0.0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "PV: nper must be positive"))));
        }

        // Calculate PV
        tDouble wPv = 0.0;
        if (wRate == 0.0) {
            wPv = -(wPmt * wNper + wFv);
        } else {
            tDouble wFactor = pow(1.0 + wRate, wNper);
            wPv = -(wPmt * (1.0 + wRate * wType) * ((wFactor - 1.0) / wRate) + wFv) / wFactor;
        }

        return(tStackElem(tVariant(wPv)));
    }

    //=========================================================================
    //! Function FV - Future Value
    tFunctionFv::tFunctionFv() : tFunctionFinancialBase() {}

    tStackElem tFunctionFv::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 3 || wArgs.size() > 5) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "FV requires 3 to 5 arguments"))));
        }

        // FV(rate, nper, pmt, [pv], [type])
        // In RPN order: 
        //   3 args: pmt nper rate FV -> wArgs[0]=pmt, wArgs[1]=nper, wArgs[2]=rate
        //   4 args: pv pmt nper rate FV -> wArgs[0]=pv, wArgs[1]=pmt, wArgs[2]=nper, wArgs[3]=rate
        //   5 args: type pv pmt nper rate FV -> wArgs[0]=type, wArgs[1]=pv, wArgs[2]=pmt, wArgs[3]=nper, wArgs[4]=rate
        tDouble wRate = 0.0;
        tDouble wNper = 0.0;
        tDouble wPmt = 0.0;
        tDouble wPv = 0.0;
        tInt wType = 0;
        tVariant wError;
        
        // Calculate indices dynamically based on wArgs.size()
        // Pattern: rate=size-1, nper=size-2, pmt=size-3, pv=size-4, type=size-5
        size_t wSize = wArgs.size();
        
        // Extract rate (always at index size-1)
        wError = ExtractDoubleArg(&wArgs, wSize - 1, "FV", "rate", wRate);
        if (wError.IsError()) return wError;
        
        // Extract nper (always at index size-2)
        wError = ExtractDoubleArg(&wArgs, wSize - 2, "FV", "nper", wNper);
        if (wError.IsError()) return wError;
        
        // Extract pmt (always at index size-3)
        wError = ExtractDoubleArg(&wArgs, wSize - 3, "FV", "pmt", wPmt);
        if (wError.IsError()) return wError;
        
        // Extract pv (at index size-4 if size >= 4, optional)
        if (wSize >= 4) {
            wError = ExtractDoubleArg(&wArgs, wSize - 4, "FV", "pv", wPv, 0.0, true);
            if (wError.IsError()) return wError;
        }
        
        // Extract type (at index size-5 if size == 5, optional)
        if (wSize == 5) {
            wError = ExtractIntArg(&wArgs, wSize - 5, "FV", "type", wType, 0, true);
            if (wError.IsError()) return wError;
        }
        
        // Clean up arguments

        // Validate inputs
        if (wNper <= 0.0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "FV: nper must be positive"))));
        }

        // Calculate FV
        tDouble wFv = 0.0;
        if (wRate == 0.0) {
            wFv = -(wPv + wPmt * wNper);
        } else {
            tDouble wFactor = pow(1.0 + wRate, wNper);
            wFv = -(wPv * wFactor + wPmt * (1.0 + wRate * wType) * ((wFactor - 1.0) / wRate));
        }

        return(tStackElem(tVariant(wFv)));
    }

    //=========================================================================
    //! Function RATE - Interest Rate
    tFunctionRate::tFunctionRate() : tFunctionFinancialBase() {}

    tStackElem tFunctionRate::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 3 || wArgs.size() > 6) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "RATE requires 3 to 6 arguments"))));
        }

        // RATE(nper, pmt, pv, [fv], [type], [guess])
        // In RPN order: 
        //   3 args: pv pmt nper RATE -> wArgs[0]=pv, wArgs[1]=pmt, wArgs[2]=nper
        //   4 args: fv pv pmt nper RATE -> wArgs[0]=fv, wArgs[1]=pv, wArgs[2]=pmt, wArgs[3]=nper
        //   5 args: type fv pv pmt nper RATE -> wArgs[0]=type, wArgs[1]=fv, wArgs[2]=pv, wArgs[3]=pmt, wArgs[4]=nper
        //   6 args: guess type fv pv pmt nper RATE -> wArgs[0]=guess, wArgs[1]=type, wArgs[2]=fv, wArgs[3]=pv, wArgs[4]=pmt, wArgs[5]=nper
        tDouble wNper = 0.0;
        tDouble wPmt = 0.0;
        tDouble wPv = 0.0;
        tDouble wFv = 0.0;
        tInt wType = 0;
        tDouble wGuess = 0.1; // Default guess
        tVariant wError;
        
        // Calculate indices dynamically based on wArgs.size()
        // Pattern: nper=size-1, pmt=size-2, pv=size-3, fv=size-4, type=size-5, guess=size-6
        size_t wSize = wArgs.size();
        
        // Extract nper (always at index size-1)
        wError = ExtractDoubleArg(&wArgs, wSize - 1, "RATE", "nper", wNper);
        if (wError.IsError()) return wError;
        
        // Extract pmt (always at index size-2)
        wError = ExtractDoubleArg(&wArgs, wSize - 2, "RATE", "pmt", wPmt);
        if (wError.IsError()) return wError;
        
        // Extract pv (always at index size-3)
        wError = ExtractDoubleArg(&wArgs, wSize - 3, "RATE", "pv", wPv);
        if (wError.IsError()) return wError;
        
        // Extract fv (at index size-4 if size >= 4, optional)
        if (wSize >= 4) {
            wError = ExtractDoubleArg(&wArgs, wSize - 4, "RATE", "fv", wFv, 0.0, true);
            if (wError.IsError()) return wError;
        }
        
        // Extract type (at index size-5 if size >= 5, optional)
        if (wSize >= 5) {
            wError = ExtractIntArg(&wArgs, wSize - 5, "RATE", "type", wType, 0, true);
            if (wError.IsError()) return wError;
        }
        
        // Extract guess (at index size-6 if size == 6, optional)
        if (wSize == 6) {
            wError = ExtractDoubleArg(&wArgs, wSize - 6, "RATE", "guess", wGuess, 0.1, true);
            if (wError.IsError()) return wError;
        }
        
        // Clean up arguments

        // Validate inputs
        if (wNper <= 0.0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "RATE: nper must be positive"))));
        }

        // Use Newton-Raphson method to solve for rate
        // f(rate) = pv * (1+rate)^nper + pmt * (1+rate*type) * ((1+rate)^nper - 1) / rate + fv = 0
        tDouble wRate = wGuess;
        const tInt wMaxIterations = 100;
        const tDouble wTolerance = 1e-6;
        const tDouble wMinRate = 1e-10; // Minimum rate to avoid division by zero

        for (tInt wIter = 0; wIter < wMaxIterations; ++wIter) {
            // Ensure rate is not too close to zero to avoid division issues
            if (fabs(wRate) < wMinRate) {
                wRate = (wRate >= 0) ? wMinRate : -wMinRate;
            }
            
            tDouble wFactor = pow(1.0 + wRate, wNper);
            tDouble wF = wPv * wFactor + wPmt * (1.0 + wRate * wType) * ((wFactor - 1.0) / wRate) + wFv;
            
            if (fabs(wF) < wTolerance) {
                return(tStackElem(tVariant(wRate)));
            }

            // Calculate derivative
            tDouble wFactorDeriv = wNper * pow(1.0 + wRate, wNper - 1.0);
            tDouble wFDeriv = wPv * wFactorDeriv + 
                             wPmt * wType * ((wFactor - 1.0) / wRate) +
                             wPmt * (1.0 + wRate * wType) * ((wFactorDeriv * wRate - (wFactor - 1.0)) / (wRate * wRate));

            if (fabs(wFDeriv) < wTolerance) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "RATE: cannot converge"))));
            }

            tDouble wNewRate = wRate - wF / wFDeriv;

            // Prevent negative rates or rates that are too extreme
            if (wNewRate < -0.99) {
                wNewRate = -0.99;
            } else if (wNewRate > 10.0) {
                wNewRate = 10.0;
            }
            
            // Check for convergence
            if (fabs(wNewRate - wRate) < wTolerance) {
                return(tStackElem(tVariant(wNewRate)));
            }
            
            wRate = wNewRate;
        }

        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "RATE: maximum iterations exceeded"))));
    }

    //=========================================================================
    //! Function NPER - Number of Periods
    tFunctionNper::tFunctionNper() : tFunctionFinancialBase() {}

    tStackElem tFunctionNper::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 3 || wArgs.size() > 5) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NPER requires 3 to 5 arguments"))));
        }

        // NPER(rate, pmt, pv, [fv], [type])
        // In RPN order: 
        //   3 args: pv pmt rate NPER -> wArgs[0]=pv, wArgs[1]=pmt, wArgs[2]=rate
        //   4 args: fv pv pmt rate NPER -> wArgs[0]=fv, wArgs[1]=pv, wArgs[2]=pmt, wArgs[3]=rate
        //   5 args: type fv pv pmt rate NPER -> wArgs[0]=type, wArgs[1]=fv, wArgs[2]=pv, wArgs[3]=pmt, wArgs[4]=rate
        tDouble wRate = 0.0;
        tDouble wPmt = 0.0;
        tDouble wPv = 0.0;
        tDouble wFv = 0.0;
        tInt wType = 0;
        tVariant wError;
        
        // Calculate indices dynamically based on wArgs.size()
        // Pattern: rate=size-1, pmt=size-2, pv=size-3, fv=size-4, type=size-5
        size_t wSize = wArgs.size();
        
        // Extract rate (always at index size-1)
        wError = ExtractDoubleArg(&wArgs, wSize - 1, "NPER", "rate", wRate);
        if (wError.IsError()) return wError;
        
        // Extract pmt (always at index size-2)
        wError = ExtractDoubleArg(&wArgs, wSize - 2, "NPER", "pmt", wPmt);
        if (wError.IsError()) return wError;
        
        // Extract pv (always at index size-3)
        wError = ExtractDoubleArg(&wArgs, wSize - 3, "NPER", "pv", wPv);
        if (wError.IsError()) return wError;
        
        // Extract fv (at index size-4 if size >= 4, optional)
        if (wSize >= 4) {
            wError = ExtractDoubleArg(&wArgs, wSize - 4, "NPER", "fv", wFv, 0.0, true);
            if (wError.IsError()) return wError;
        }
        
        // Extract type (at index size-5 if size == 5, optional)
        if (wSize == 5) {
            wError = ExtractIntArg(&wArgs, wSize - 5, "NPER", "type", wType, 0, true);
            if (wError.IsError()) return wError;
        }
        
        // Clean up arguments

        // Validate inputs
        if (wRate < -1.0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "NPER: rate must be >= -1"))));
        }

        // Calculate NPER
        tDouble wNper = 0.0;
        if (wRate == 0.0) {
            // If rate is 0, nper = -(pv + fv) / pmt
            if (wPmt == 0.0) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "NPER: pmt cannot be 0 when rate is 0"))));
            }
            wNper = -(wPv + wFv) / wPmt;
        } else {
            // nper = log((pmt * (1 + rate * type) - fv * rate) / (pmt * (1 + rate * type) + pv * rate)) / log(1 + rate)
            tDouble wNumerator = wPmt * (1.0 + wRate * wType) - wFv * wRate;
            tDouble wDenominator = wPmt * (1.0 + wRate * wType) + wPv * wRate;
            
            if (wDenominator == 0.0 || wNumerator / wDenominator <= 0.0) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "NPER: invalid calculation"))));
            }
            
            wNper = log(wNumerator / wDenominator) / log(1.0 + wRate);
        }

        return(tStackElem(tVariant(wNper)));
    }

    //=========================================================================
    //! Function NPV - Net Present Value
    tFunctionNpv::tFunctionNpv() : tFunctionFinancialBase() {}

    tStackElem tFunctionNpv::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NPV requires at least 2 arguments"))));
        }

        // NPV(rate, value1, [value2], ...)
        // PopArgs is LIFO; reverse so wArgs[0]=rate, [1]=value1, …
        std::reverse(wArgs.begin(), wArgs.end());

        tDouble wRate = 0.0;
        tVariant wError = ExtractDoubleArg(&wArgs, 0, "NPV", "rate", wRate);
        if (wError.IsError()) {
            return wError;
        }

        tDouble wNpv = 0.0;
        size_t wPeriodIndex = 1; // Excel discounts value1 at period 1
        for (size_t i = 1; i < wArgs.size(); ++i) {
            tStackElem* wArg = &wArgs[i];
            if (wArg->Type() == tStackType::t_Range) {
                // Handle range of values (must be checked first; StackElemToVariant would reduce to single cell)
                tRange* wRange = wArg->Range();
                tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
                if (wColRowCellRange != nullptr) {
                    // Process range cells in order (top-left to bottom-right)
                    for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); wRow++) {
                        for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); wCol++) {
                            tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                            if (wCell != nullptr) {
                                tVariant wCellValue = wCell->CalculableValue();
                                if (wCellValue.IsError()) {
                                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "NPV: values must be numeric"))));
                                }
                                tDouble wValue = 0.0;
                                // Excel treats empty/null values as 0 in financial functions
                                if (wCellValue.IsNull()) {
                                    wValue = 0.0;
                                } else if (wCellValue.IsDouble()) {
                                    wValue = wCellValue.Double();
                                } else if (wCellValue.IsInt()) {
                                    wValue = static_cast<tDouble>(wCellValue.Int());
                                } else {
                                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "NPV: values must be numeric"))));
                                }
                                tDouble wDiscountFactor = pow(1.0 + wRate, static_cast<tDouble>(wPeriodIndex));
                                wNpv += wValue / wDiscountFactor;
                                wPeriodIndex++;
                            }
                        }
                    }
                }
            } else {
                tVariant wVariant;
                if (StackElemToVariant(*wArg, wVariant)) {
                    if (wVariant.IsError()) return tStackElem(tVariant(wVariant));
                    tDouble wValue = 0.0;
                    if (wVariant.IsNull()) {
                        wValue = 0.0;
                    } else if (wVariant.IsDouble()) {
                        wValue = wVariant.Double();
                    } else if (wVariant.IsInt()) {
                        wValue = static_cast<tDouble>(wVariant.Int());
                    } else {
                        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NPV: values must be numeric"))));
                    }
                    tDouble wDiscountFactor = pow(1.0 + wRate, static_cast<tDouble>(wPeriodIndex));
                    wNpv += wValue / wDiscountFactor;
                    wPeriodIndex++;
                } else {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NPV: invalid value argument"))));
                }
            }
        }

        return(tStackElem(tVariant(wNpv)));
    }

    //=========================================================================
    //! Function IRR - Internal Rate of Return
    tFunctionIrr::tFunctionIrr() : tFunctionFinancialBase() {}

    tStackElem tFunctionIrr::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 1 || wArgs.size() > 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IRR requires 1 or 2 arguments"))));
        }

        // IRR(values, [guess])
        // In RPN: guess values IRR (if 2 args) or values IRR (if 1 arg)
        // After PopArgs: wArgs[0]=values, wArgs[1]=guess (if present)
        // Find range (values) first
        tRange* wRange = nullptr;
        for (size_t i = 0; i < wArgs.size(); i++) {
            if (wArgs[i].Type() == tStackType::t_Range) {
                wRange = wArgs[i].Range();
                break;
            }
        }
        
        if (wRange == nullptr) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IRR: values must be a range"))));
        }
        
        // Extract optional guess - if we have 2 args, guess is at the non-range position (Variant or Cell)
        tDouble wGuess = 0.1; // Default guess
        if (wArgs.size() == 2) {
            for (size_t i = 0; i < wArgs.size(); i++) {
                if (wArgs[i].Type() != tStackType::t_Range) {
                    tVariant wError = ExtractDoubleArg(&wArgs, i, "IRR", "guess", wGuess, 0.1, true);
                    if (wError.IsError()) return tStackElem(tVariant(wError));
                    break;
                }
            }
        }
        
        // Clean up arguments

        // Extract values from range
        std::vector<tDouble> wValues;
        {
            tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
            for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); wRow++) {
                for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); wCol++) {
                    tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                    if (wCell != nullptr) {
                        tVariant wCellValue = wCell->CalculableValue();
                        if (wCellValue.IsError()) {
                            // Arguments already cleaned up above
                            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "IRR: values must be numeric"))));
                        }
                        tDouble wValue = 0.0;
                        // Excel treats empty/null values as 0 in financial functions
                        if (wCellValue.IsNull()) {
                            wValue = 0.0;
                        } else if (wCellValue.IsDouble()) {
                            wValue = wCellValue.Double();
                        } else if (wCellValue.IsInt()) {
                            wValue = static_cast<tDouble>(wCellValue.Int());
                        } else {
                            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "IRR: values must be numeric"))));
                        }
                        wValues.push_back(wValue);
                    }
                }
            }
        }

        if (wValues.size() < 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "IRR: at least 2 values are required"))));
        }

        // Use Newton-Raphson method to solve for IRR
        // NPV(rate) = sum(values[i] / (1 + rate)^i) = 0
        tDouble wRate = wGuess;
        const tInt wMaxIterations = 100;
        const tDouble wTolerance = 1e-6;

        for (tInt wIter = 0; wIter < wMaxIterations; ++wIter) {
            tDouble wNpv = 0.0;
            tDouble wNpvDeriv = 0.0;
            
            for (size_t i = 0; i < wValues.size(); i++) {
                tDouble wFactor = pow(1.0 + wRate, static_cast<tDouble>(i));
                wNpv += wValues[i] / wFactor;
                if (i > 0) {
                    wNpvDeriv -= static_cast<tDouble>(i) * wValues[i] / (wFactor * (1.0 + wRate));
                }
            }
            
            if (fabs(wNpv) < wTolerance) {
                return(tStackElem(tVariant(wRate)));
            }

            if (fabs(wNpvDeriv) < wTolerance) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "IRR: cannot converge"))));
            }

            wRate = wRate - wNpv / wNpvDeriv;

            // Prevent invalid rates
            if (wRate < -0.99) {
                wRate = -0.99;
            }
        }

        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "IRR: maximum iterations exceeded"))));
    }

    //=========================================================================
    //! Function MIRR - Modified Internal Rate of Return
    tFunctionMirr::tFunctionMirr() : tFunctionFinancialBase() {}

    tStackElem tFunctionMirr::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MIRR requires 3 arguments"))));
        }

        // MIRR(values, finance_rate, reinvest_rate)
        // In RPN: values finance_rate reinvest_rate MIRR
        // After PopArgs: wArgs[0]=reinvest_rate, wArgs[1]=finance_rate, wArgs[2]=values
        // Find range first (values) - should be at index 2
        tRange* wRange = nullptr;
        for (size_t i = 0; i < wArgs.size(); i++) {
            if (wArgs[i].Type() == tStackType::t_Range) {
                wRange = wArgs[i].Range();
                break;
            }
        }
        
        if (wRange == nullptr) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MIRR: values must be a range"))));
        }
        
        // Extract rates: wArgs[0] = reinvest_rate, wArgs[1] = finance_rate
        tVariant wError;
        tDouble wReinvestRate = 0.0;
        tDouble wFinanceRate = 0.0;
        
        // Extract reinvest_rate from wArgs[0] (Variant, Cell, or single-cell Range)
        {
            tVariant wVariant;
            if (!StackElemToVariant(wArgs[0], wVariant)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MIRR: reinvest_rate must be numeric"))));
            }
            if (wVariant.IsError()) return tStackElem(tVariant(wVariant));
            if (wVariant.IsNull()) {
                wReinvestRate = 0.0;
            } else if (wVariant.IsDouble()) {
                wReinvestRate = wVariant.Double();
            } else if (wVariant.IsInt()) {
                wReinvestRate = static_cast<tDouble>(wVariant.Int());
            } else {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MIRR: reinvest_rate must be numeric"))));
            }
        }
        // Extract finance_rate from wArgs[1] (Variant, Cell, or single-cell Range)
        {
            tVariant wVariant;
            if (!StackElemToVariant(wArgs[1], wVariant)) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MIRR: finance_rate must be numeric"))));
            }
            if (wVariant.IsError()) return tStackElem(tVariant(wVariant));
            if (wVariant.IsNull()) {
                wFinanceRate = 0.0;
            } else if (wVariant.IsDouble()) {
                wFinanceRate = wVariant.Double();
            } else if (wVariant.IsInt()) {
                wFinanceRate = static_cast<tDouble>(wVariant.Int());
            } else {
                return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MIRR: finance_rate must be numeric"))));
            }
        }
        
        // Clean up arguments

        // Extract values from range
        std::vector<tDouble> wValues;
        tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
        for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); wRow++) {
            for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); wCol++) {
                tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                if (wCell != nullptr) {
                    tVariant wCellValue = wCell->CalculableValue();
                    if (wCellValue.IsError()) {
                        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "MIRR: values must be numeric"))));
                    }
                    tDouble wValue = 0.0;
                    // Excel treats empty/null values as 0 in financial functions
                    if (wCellValue.IsNull()) {
                        wValue = 0.0;
                    } else if (wCellValue.IsDouble()) {
                        wValue = wCellValue.Double();
                    } else if (wCellValue.IsInt()) {
                        wValue = static_cast<tDouble>(wCellValue.Int());
                    } else {
                        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "MIRR: values must be numeric"))));
                    }
                    wValues.push_back(wValue);
                }
            }
        }

        if (wValues.size() < 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MIRR: at least 2 values are required"))));
        }

        // Calculate MIRR
        // MIRR = (FV of positive cash flows / PV of negative cash flows)^(1/n) - 1
        // Formula: MIRR = (FV_positives / |PV_negatives|)^(1/n) - 1
        // where n = number of periods (number of cash flows - 1)
        tDouble wPositiveFv = 0.0;
        tDouble wNegativePv = 0.0;
        tInt wPeriodCount = static_cast<tInt>(wValues.size()) - 1;

        // Calculate future value of positive cash flows at reinvest_rate
        // and present value of negative cash flows at finance_rate
        for (size_t i = 0; i < wValues.size(); i++) {
            if (wValues[i] > 0) {
                // Positive cash flows: compound forward at reinvest_rate
                // Cash flow at period i is compounded to period n (last period)
                // Number of periods to compound: (n - i)
                tInt wPeriodsForward = static_cast<tInt>(wPeriodCount - i);
                wPositiveFv += wValues[i] * pow(1.0 + wReinvestRate, static_cast<tDouble>(wPeriodsForward));
            } else if (wValues[i] < 0) {
                // Negative cash flows: discount back at finance_rate
                // Cash flow at period i is discounted to period 0 (initial period)
                // Number of periods to discount: i
                tInt wPeriodsBack = static_cast<tInt>(i);
                wNegativePv += wValues[i] / pow(1.0 + wFinanceRate, static_cast<tDouble>(wPeriodsBack));
            }
        }

        if (wNegativePv >= 0 || wPositiveFv <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "MIRR: invalid cash flow pattern"))));
        }

        // Calculate MIRR: (FV_positives / |PV_negatives|)^(1/n) - 1
        // Since wNegativePv is negative, we use -wNegativePv to get the absolute value
        tDouble wRatio = wPositiveFv / (-wNegativePv);
        tDouble wMirr = pow(wRatio, 1.0 / static_cast<tDouble>(wPeriodCount)) - 1.0;

        return(tStackElem(tVariant(wMirr)));
    }

    //=========================================================================
    //! Function DB - Declining Balance depreciation
    tFunctionDb::tFunctionDb() : tFunctionFinancialBase() {}

    tStackElem tFunctionDb::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 4 || wArgs.size() > 5) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "DB requires 4 or 5 arguments"))));
        }

        // DB(cost, salvage, life, period, [month])
        // In RPN order: 
        //   4 args: period life salvage cost DB -> wArgs[0]=cost, wArgs[1]=salvage, wArgs[2]=life, wArgs[3]=period
        //   5 args: month period life salvage cost DB -> wArgs[0]=month, wArgs[1]=period, wArgs[2]=life, wArgs[3]=salvage, wArgs[4]=cost
        tDouble wCost = 0.0;
        tDouble wSalvage = 0.0;
        tDouble wLife = 0.0;
        tInt wPeriod = 0;
        tInt wMonth = 12; // Default month
        tVariant wError;
        
        // Calculate indices dynamically based on wArgs.size()
        // Pattern: cost=size-1, salvage=size-2, life=size-3, period=size-4, month=size-5
        size_t wSize = wArgs.size();
        
        // Extract cost (always at index size-1)
        wError = ExtractDoubleArg(&wArgs, wSize - 1, "DB", "cost", wCost);
        if (wError.IsError()) return wError;
        
        // Extract salvage (always at index size-2)
        wError = ExtractDoubleArg(&wArgs, wSize - 2, "DB", "salvage", wSalvage);
        if (wError.IsError()) return wError;
        
        // Extract life (always at index size-3)
        wError = ExtractDoubleArg(&wArgs, wSize - 3, "DB", "life", wLife);
        if (wError.IsError()) return wError;
        
        // Extract period (always at index size-4)
        wError = ExtractIntArg(&wArgs, wSize - 4, "DB", "period", wPeriod);
        if (wError.IsError()) return wError;
        
        // Extract month (at index size-5 if size == 5, optional)
        if (wSize == 5) {
            wError = ExtractIntArg(&wArgs, wSize - 5, "DB", "month", wMonth, 12, true);
            if (wError.IsError()) return wError;
        }
        
        // Clean up arguments

        // Validate arguments
        if (wCost < 0 || wSalvage < 0 || wLife <= 0 || wPeriod < 1 || wMonth < 1 || wMonth > 12) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "DB: invalid argument values"))));
        }

        if (wPeriod > static_cast<tInt>(wLife) + 1) {
            return(tStackElem(tVariant(0.0)));
        }

        // Calculate DB depreciation
        // Rate = 1 - ((salvage / cost) ^ (1 / life))
        // Excel rounds the rate to 3 decimal places before using it
        tDouble wRate = 1.0 - pow(wSalvage / wCost, 1.0 / wLife);
        // Round to 3 decimal places (Excel behavior)
        wRate = round(wRate * 1000.0) / 1000.0;
        
        tDouble wDepreciation = 0.0;
        
        if (wPeriod == 1) {
            // First period: cost * rate * month / 12
            // Excel DB: month is the number of months in the first year
            // For month=7, depreciation = cost * rate * (7/12)
            wDepreciation = wCost * wRate * static_cast<tDouble>(wMonth) / 12.0;
        } else if (wPeriod <= static_cast<tInt>(wLife)) {
            // Intermediate periods: (cost - accumulated depreciation) * rate
            tDouble wAccumulated = 0.0;
            // First period depreciation
            wAccumulated = wCost * wRate * static_cast<tDouble>(wMonth) / 12.0;
            // Subsequent periods
            for (tInt i = 2; i < wPeriod; i++) {
                wAccumulated += (wCost - wAccumulated) * wRate;
            }
            // Current period
            wDepreciation = (wCost - wAccumulated) * wRate;
        } else {
            // Last period: remaining value
            // Calculate accumulated depreciation up to end of life period
            tDouble wAccumulated = 0.0;
            wAccumulated = wCost * wRate * static_cast<tDouble>(wMonth) / 12.0;
            for (tInt i = 2; i <= static_cast<tInt>(wLife); i++) {
                wAccumulated += (wCost - wAccumulated) * wRate;
            }
            // Last period covers remaining months of the last year: (12 - month) months
            tInt wRemainingMonths = 12 - wMonth;
            wDepreciation = (wCost - wAccumulated) * wRate * static_cast<tDouble>(wRemainingMonths) / 12.0;
        }

        return(tStackElem(tVariant(wDepreciation)));
    }

    //=========================================================================
    //! Function DDB - Double Declining Balance depreciation
    tFunctionDdb::tFunctionDdb() : tFunctionFinancialBase() {}

    tStackElem tFunctionDdb::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 4 || wArgs.size() > 5) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "DDB requires 4 or 5 arguments"))));
        }

        // DDB(cost, salvage, life, period, [factor])
        // In RPN order: 
        //   4 args: period life salvage cost DDB -> wArgs[0]=period, wArgs[1]=life, wArgs[2]=salvage, wArgs[3]=cost
        //   5 args: factor period life salvage cost DDB -> wArgs[0]=factor, wArgs[1]=period, wArgs[2]=life, wArgs[3]=salvage, wArgs[4]=cost
        tDouble wCost = 0.0;
        tDouble wSalvage = 0.0;
        tDouble wLife = 0.0;
        tInt wPeriod = 0;
        tDouble wFactor = 2.0; // Default factor (double declining balance)
        tVariant wError;
        
        // Calculate indices dynamically based on wArgs.size()
        // Pattern: cost=size-1, salvage=size-2, life=size-3, period=size-4, factor=size-5
        size_t wSize = wArgs.size();
        
        // Extract cost (always at index size-1)
        wError = ExtractDoubleArg(&wArgs, wSize - 1, "DDB", "cost", wCost);
        if (wError.IsError()) return wError;
        
        // Extract salvage (always at index size-2)
        wError = ExtractDoubleArg(&wArgs, wSize - 2, "DDB", "salvage", wSalvage);
        if (wError.IsError()) return wError;
        
        // Extract life (always at index size-3)
        wError = ExtractDoubleArg(&wArgs, wSize - 3, "DDB", "life", wLife);
        if (wError.IsError()) return wError;
        
        // Extract period (always at index size-4)
        wError = ExtractIntArg(&wArgs, wSize - 4, "DDB", "period", wPeriod);
        if (wError.IsError()) return wError;
        
        // Extract factor (at index size-5 if size == 5, optional)
        if (wSize == 5) {
            wError = ExtractDoubleArg(&wArgs, wSize - 5, "DDB", "factor", wFactor, 2.0, true);
            if (wError.IsError()) return wError;
        }
        
        // Clean up arguments

        // Validate arguments
        if (wCost < 0 || wSalvage < 0 || wLife <= 0 || wPeriod < 1 || wFactor <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "DDB: invalid argument values"))));
        }

        if (wPeriod > static_cast<tInt>(wLife)) {
            return(tStackElem(tVariant(0.0)));
        }

        // Calculate DDB depreciation
        tDouble wRate = wFactor / wLife;
        tDouble wDepreciation = 0.0;
        tDouble wBookValue = wCost;

        for (tInt i = 1; i <= wPeriod; i++) {
            tDouble wPeriodDepreciation = wBookValue * wRate;
            tDouble wNewBookValue = wBookValue - wPeriodDepreciation;
            
            // Don't depreciate below salvage value
            if (wNewBookValue < wSalvage) {
                wPeriodDepreciation = wBookValue - wSalvage;
                wBookValue = wSalvage;
            } else {
                wBookValue = wNewBookValue;
            }
            
            if (i == wPeriod) {
                wDepreciation = wPeriodDepreciation;
            }
        }

        return(tStackElem(tVariant(wDepreciation)));
    }

    //=========================================================================
    //! Function SLN - Straight Line depreciation
    tFunctionSln::tFunctionSln() : tFunctionFinancialBase() {}

    tStackElem tFunctionSln::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SLN requires 3 arguments"))));
        }

        // SLN(cost, salvage, life)
        // In RPN order: life salvage cost SLN -> wArgs[0]=life, wArgs[1]=salvage, wArgs[2]=cost
        tDouble wCost = 0.0;
        tDouble wSalvage = 0.0;
        tDouble wLife = 0.0;
        tVariant wError;
        
        wError = ExtractDoubleArg(&wArgs, 2, "SLN", "cost", wCost);
        if (wError.IsError()) return wError;
        wError = ExtractDoubleArg(&wArgs, 1, "SLN", "salvage", wSalvage);
        if (wError.IsError()) return wError;
        wError = ExtractDoubleArg(&wArgs, 0, "SLN", "life", wLife);
        if (wError.IsError()) return wError;
        
        // Clean up arguments

        // Validate arguments
        if (wLife <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "SLN: life must be positive"))));
        }

        // Calculate SLN: (cost - salvage) / life
        return(tStackElem(tVariant((wCost - wSalvage) / wLife)));
    }

    //=========================================================================
    //! Function SYD - Sum of Years Digits depreciation
    tFunctionSyd::tFunctionSyd() : tFunctionFinancialBase() {}

    tStackElem tFunctionSyd::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 4) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SYD requires 4 arguments"))));
        }

        // SYD(cost, salvage, life, per)
        // In RPN order: per life salvage cost SYD -> wArgs[0]=per, wArgs[1]=life, wArgs[2]=salvage, wArgs[3]=cost
        tDouble wCost = 0.0;
        tDouble wSalvage = 0.0;
        tDouble wLife = 0.0;
        tInt wPer = 0;
        tVariant wError;
        
        wError = ExtractDoubleArg(&wArgs, 3, "SYD", "cost", wCost);
        if (wError.IsError()) return wError;
        wError = ExtractDoubleArg(&wArgs, 2, "SYD", "salvage", wSalvage);
        if (wError.IsError()) return wError;
        wError = ExtractDoubleArg(&wArgs, 1, "SYD", "life", wLife);
        if (wError.IsError()) return wError;
        wError = ExtractIntArg(&wArgs, 0, "SYD", "per", wPer);
        if (wError.IsError()) return wError;
        
        // Clean up arguments

        // Validate arguments
        if (wCost < 0 || wSalvage < 0 || wLife <= 0 || wPer < 1 || wPer > static_cast<tInt>(wLife)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "SYD: invalid argument values"))));
        }

        // Calculate SYD: (cost - salvage) * (life - per + 1) * 2 / (life * (life + 1))
        tDouble wSumOfYears = wLife * (wLife + 1.0) / 2.0;
        tDouble wDepreciation = (wCost - wSalvage) * (wLife - static_cast<tDouble>(wPer) + 1.0) / wSumOfYears;

        return(tStackElem(tVariant(wDepreciation)));
    }

    //=========================================================================
    //! Function CUMIPMT - Cumulative Interest Payment
    tFunctionCumipmt::tFunctionCumipmt() : tFunctionFinancialBase() {}

    tStackElem tFunctionCumipmt::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 6) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "CUMIPMT requires 6 arguments"))));
        }

        // CUMIPMT(rate, nper, pv, start_period, end_period, type)
        // In RPN order: type end_period start_period pv nper rate CUMIPMT
        // So wArgs[0]=type, wArgs[1]=end_period, wArgs[2]=start_period, wArgs[3]=pv, wArgs[4]=nper, wArgs[5]=rate
        tDouble wRate = 0.0;
        tDouble wNper = 0.0;
        tDouble wPv = 0.0;
        tInt wStartPeriod = 0;
        tInt wEndPeriod = 0;
        tInt wType = 0;
        tVariant wError;
        
        wError = ExtractDoubleArg(&wArgs, 5, "CUMIPMT", "rate", wRate);
        if (wError.IsError()) return wError;
        wError = ExtractDoubleArg(&wArgs, 4, "CUMIPMT", "nper", wNper);
        if (wError.IsError()) return wError;
        wError = ExtractDoubleArg(&wArgs, 3, "CUMIPMT", "pv", wPv);
        if (wError.IsError()) return wError;
        wError = ExtractIntArg(&wArgs, 2, "CUMIPMT", "start_period", wStartPeriod);
        if (wError.IsError()) return wError;
        wError = ExtractIntArg(&wArgs, 1, "CUMIPMT", "end_period", wEndPeriod);
        if (wError.IsError()) return wError;
        wError = ExtractIntArg(&wArgs, 0, "CUMIPMT", "type", wType);
        if (wError.IsError()) return wError;
        
        // Clean up arguments

        // Validate arguments
        if (wRate <= 0 || wNper <= 0 || wStartPeriod < 1 || wEndPeriod < wStartPeriod || wEndPeriod > static_cast<tInt>(wNper) || (wType != 0 && wType != 1)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "CUMIPMT: invalid argument values"))));
        }

        // Calculate cumulative interest
        // First calculate PMT using Excel's standard formula: PMT = (PV × rate) / (1 - (1 + rate)^-nper)
        tDouble wFv = 0.0;
        tDouble wPmt = 0.0;
        if (wRate == 0.0) {
            wPmt = -(wPv + wFv) / wNper;
        } else {
            // Excel's standard PMT formula: PMT = (PV × rate) / (1 - (1 + rate)^-nper)
            // But we need negative for cash outflow, so: PMT = -(PV × rate) / (1 - (1 + rate)^-nper)
            tDouble wFactor = pow(1.0 + wRate, wNper);
            wPmt = -(wPv * wRate) / (1.0 - 1.0 / wFactor);
            // Adjust for payment timing (type: 0 = end of period, 1 = beginning)
            if (wType == 1) {
                wPmt = wPmt / (1.0 + wRate);
            }
        }

        // Calculate cumulative interest from start_period to end_period
        // Use direct formula: Balance after period n = PV × (1 + Rate)^n + PMT × ((1 + Rate)^n - 1) / Rate
        // Interest for period n = Balance before period n × Rate
        // Balance before period n = Balance after period (n-1)
        tDouble wCumInterest = 0.0;

        for (tInt wPer = wStartPeriod; wPer <= wEndPeriod; wPer++) {
            tDouble wInterest = 0.0;
            tDouble wBalanceBeforePayment = 0.0;
            
            if (wType == 0) {
                // Payment at end of period
                // Balance before payment at period p = Balance after period (p-1)
                if (wPer == 1) {
                    wBalanceBeforePayment = wPv;
                } else {
                    // Balance after period (p-1) = PV × (1 + Rate)^(p-1) + PMT × ((1 + Rate)^(p-1) - 1) / Rate
                    tDouble wFactorPrev = pow(1.0 + wRate, static_cast<tDouble>(wPer - 1));
                    if (wRate == 0.0) {
                        wBalanceBeforePayment = wPv + wPmt * static_cast<tDouble>(wPer - 1);
                    } else {
                        wBalanceBeforePayment = wPv * wFactorPrev + wPmt * (wFactorPrev - 1.0) / wRate;
                    }
                }
                // Interest = Balance before payment × Rate
                wInterest = wBalanceBeforePayment * wRate;
            } else {
                // Payment at beginning of period
                // Balance before payment at period p = Balance after period (p-1) + PMT
                if (wPer == 1) {
                    wBalanceBeforePayment = wPv + wPmt;
                } else {
                    tDouble wFactorPrev = pow(1.0 + wRate, static_cast<tDouble>(wPer - 1));
                    if (wRate == 0.0) {
                        wBalanceBeforePayment = wPv + wPmt * static_cast<tDouble>(wPer);
                    } else {
                        wBalanceBeforePayment = wPv * wFactorPrev + wPmt * (wFactorPrev - 1.0) / wRate + wPmt;
                    }
                }
                // Interest = Balance before payment × Rate
                wInterest = wBalanceBeforePayment * wRate;
            }
            
            // Accumulate interest
            wCumInterest += wInterest;
        }
        
        // Excel returns negative value (cash outflow)
        return(tStackElem(tVariant(-wCumInterest)));
    }

    //=========================================================================
    //! Function CUMPRINC - Cumulative Principal Payment
    tFunctionCumprinc::tFunctionCumprinc() : tFunctionFinancialBase() {}

    tStackElem tFunctionCumprinc::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 6) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "CUMPRINC requires 6 arguments"))));
        }

        // CUMPRINC(rate, nper, pv, start_period, end_period, type)
        // In RPN order: type end_period start_period pv nper rate CUMPRINC
        // So wArgs[0]=type, wArgs[1]=end_period, wArgs[2]=start_period, wArgs[3]=pv, wArgs[4]=nper, wArgs[5]=rate
        tDouble wRate = 0.0;
        tDouble wNper = 0.0;
        tDouble wPv = 0.0;
        tInt wStartPeriod = 0;
        tInt wEndPeriod = 0;
        tInt wType = 0;
        tVariant wError;
        
        wError = ExtractDoubleArg(&wArgs, 5, "CUMPRINC", "rate", wRate);
        if (wError.IsError()) return wError;
        wError = ExtractDoubleArg(&wArgs, 4, "CUMPRINC", "nper", wNper);
        if (wError.IsError()) return wError;
        wError = ExtractDoubleArg(&wArgs, 3, "CUMPRINC", "pv", wPv);
        if (wError.IsError()) return wError;
        wError = ExtractIntArg(&wArgs, 2, "CUMPRINC", "start_period", wStartPeriod);
        if (wError.IsError()) return wError;
        wError = ExtractIntArg(&wArgs, 1, "CUMPRINC", "end_period", wEndPeriod);
        if (wError.IsError()) return wError;
        wError = ExtractIntArg(&wArgs, 0, "CUMPRINC", "type", wType);
        if (wError.IsError()) return wError;
        
        // Clean up arguments

        // Validate arguments
        if (wRate <= 0 || wNper <= 0 || wStartPeriod < 1 || wEndPeriod < wStartPeriod || wEndPeriod > static_cast<tInt>(wNper) || (wType != 0 && wType != 1)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "CUMPRINC: invalid argument values"))));
        }

        // Calculate cumulative principal
        // First calculate PMT using Excel's standard formula: PMT = (PV × rate) / (1 - (1 + rate)^-nper)
        tDouble wFv = 0.0;
        tDouble wPmt = 0.0;
        if (wRate == 0.0) {
            wPmt = -(wPv + wFv) / wNper;
        } else {
            // Excel's standard PMT formula: PMT = (PV × rate) / (1 - (1 + rate)^-nper)
            // But we need negative for cash outflow, so: PMT = -(PV × rate) / (1 - (1 + rate)^-nper)
            tDouble wFactor = pow(1.0 + wRate, wNper);
            wPmt = -(wPv * wRate) / (1.0 - 1.0 / wFactor);
            // Adjust for payment timing (type: 0 = end of period, 1 = beginning)
            if (wType == 1) {
                wPmt = wPmt / (1.0 + wRate);
            }
        }

        // Calculate cumulative principal from start_period to end_period
        // Excel calculates CUMPRINC as: Balance before start_period - Balance after end_period
        // This is different from summing individual principal payments
        
        // Calculate balance before start_period (balance after period start_period - 1)
        tDouble wBalanceBeforeStart = 0.0;
        if (wStartPeriod == 1) {
            wBalanceBeforeStart = wPv;
        } else {
            tDouble wFactorStart = pow(1.0 + wRate, static_cast<tDouble>(wStartPeriod - 1));
            if (wRate == 0.0) {
                wBalanceBeforeStart = wPv + wPmt * static_cast<tDouble>(wStartPeriod - 1);
            } else {
                wBalanceBeforeStart = wPv * wFactorStart + wPmt * (wFactorStart - 1.0) / wRate;
            }
        }
        
        // Calculate balance after end_period (balance before period end_period + 1)
        tDouble wBalanceAfterEnd = 0.0;
        if (wEndPeriod >= static_cast<tInt>(wNper)) {
            wBalanceAfterEnd = 0.0; // Loan fully paid
        } else {
            tDouble wFactorEnd = pow(1.0 + wRate, static_cast<tDouble>(wEndPeriod));
            if (wRate == 0.0) {
                wBalanceAfterEnd = wPv + wPmt * static_cast<tDouble>(wEndPeriod);
            } else {
                if (wType == 0) {
                    wBalanceAfterEnd = wPv * wFactorEnd + wPmt * (wFactorEnd - 1.0) / wRate;
                } else {
                    wBalanceAfterEnd = (wPv + wPmt) * wFactorEnd + wPmt * (wFactorEnd - 1.0) / wRate - wPmt;
                }
            }
        }
        
        // CUMPRINC = Balance before start - Balance after end
        tDouble wCumPrincipal = wBalanceBeforeStart - wBalanceAfterEnd;
        
        // Excel returns negative value (cash outflow)
        return(tStackElem(tVariant(-wCumPrincipal)));
    }

    //=========================================================================
    //! Function EFFECT - Effective Annual Interest Rate
    tFunctionEffect::tFunctionEffect() : tFunctionFinancialBase() {}

    tStackElem tFunctionEffect::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "EFFECT requires 2 arguments"))));
        }

        // EFFECT(nominal_rate, npery)
        // In RPN order: npery nominal_rate EFFECT -> wArgs[0]=npery, wArgs[1]=nominal_rate
        tDouble wNominalRate = 0.0;
        tInt wNpery = 0;
        tVariant wError;
        
        wError = ExtractDoubleArg(&wArgs, 1, "EFFECT", "nominal_rate", wNominalRate);
        if (wError.IsError()) return wError;
        wError = ExtractIntArg(&wArgs, 0, "EFFECT", "npery", wNpery);
        if (wError.IsError()) return wError;

        // Clean up arguments

        // Validate arguments
        if (wNpery < 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "EFFECT: npery must be >= 1"))));
        }

        // Calculate EFFECT: (1 + nominal_rate / npery)^npery - 1
        return(tStackElem(tVariant(pow(1.0 + wNominalRate / static_cast<tDouble>(wNpery), static_cast<tDouble>(wNpery)) - 1.0)));
    }

    //=========================================================================
    //! Function NOMINAL - Nominal Annual Interest Rate
    tFunctionNominal::tFunctionNominal() : tFunctionFinancialBase() {}

    tStackElem tFunctionNominal::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "NOMINAL requires 2 arguments"))));
        }

        // NOMINAL(effect_rate, npery)
        // In RPN order: npery effect_rate NOMINAL -> wArgs[0]=npery, wArgs[1]=effect_rate
        tDouble wEffectRate = 0.0;
        tInt wNpery = 0;
        tVariant wError;
        
        wError = ExtractDoubleArg(&wArgs, 1, "NOMINAL", "effect_rate", wEffectRate);
        if (wError.IsError()) return wError;
        wError = ExtractIntArg(&wArgs, 0, "NOMINAL", "npery", wNpery);
        if (wError.IsError()) return wError;

        // Clean up arguments

        // Validate arguments
        if (wEffectRate <= 0 || wNpery < 1) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "NOMINAL: invalid argument values"))));
        }

        // Calculate NOMINAL: npery * ((1 + effect_rate)^(1/npery) - 1)
        tDouble wNominal = static_cast<tDouble>(wNpery) * (pow(1.0 + wEffectRate, 1.0 / static_cast<tDouble>(wNpery)) - 1.0);

        return(tStackElem(tVariant(wNominal)));
    }

    //=========================================================================
    //! Function PDURATION - Number of Periods to Reach Value
    tFunctionPduration::tFunctionPduration() : tFunctionFinancialBase() {}

    tStackElem tFunctionPduration::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "PDURATION requires 3 arguments"))));
        }

        // PDURATION(rate, pv, fv)
        // In RPN order: fv pv rate PDURATION -> wArgs[0]=fv, wArgs[1]=pv, wArgs[2]=rate
        tDouble wRate = 0.0;
        tDouble wPv = 0.0;
        tDouble wFv = 0.0;
        tVariant wError;
        
        wError = ExtractDoubleArg(&wArgs, 2, "PDURATION", "rate", wRate);
        if (wError.IsError()) return wError;
        wError = ExtractDoubleArg(&wArgs, 1, "PDURATION", "pv", wPv);
        if (wError.IsError()) return wError;
        wError = ExtractDoubleArg(&wArgs, 0, "PDURATION", "fv", wFv);
        if (wError.IsError()) return wError;
        
        // Clean up arguments

        // Validate arguments
        if (wRate <= 0 || wPv <= 0 || wFv <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "PDURATION: invalid argument values"))));
        }

        // Calculate PDURATION: log(fv/pv) / log(1 + rate)
        return(tStackElem(tVariant(log(wFv / wPv) / log(1.0 + wRate))));
    }

    //=========================================================================
    //! Function RRI - Equivalent Interest Rate
    tFunctionRri::tFunctionRri() : tFunctionFinancialBase() {}

    tStackElem tFunctionRri::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "RRI requires 3 arguments"))));
        }

        // RRI(nper, pv, fv)
        // In RPN order: fv pv nper RRI -> wArgs[0]=fv, wArgs[1]=pv, wArgs[2]=nper
        tDouble wNper = 0.0;
        tDouble wPv = 0.0;
        tDouble wFv = 0.0;
        tVariant wError;
        
        wError = ExtractDoubleArg(&wArgs, 2, "RRI", "nper", wNper);
        if (wError.IsError()) return wError;
        wError = ExtractDoubleArg(&wArgs, 1, "RRI", "pv", wPv);
        if (wError.IsError()) return wError;
        wError = ExtractDoubleArg(&wArgs, 0, "RRI", "fv", wFv);
        if (wError.IsError()) return wError;

        // Clean up arguments

        // Validate arguments
        if (wNper <= 0 || wPv <= 0 || wFv < 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "RRI: invalid argument values"))));
        }

        // Calculate RRI: (fv/pv)^(1/nper) - 1
        return(tStackElem(tVariant(pow(wFv / wPv, 1.0 / wNper) - 1.0)));
    }

    //=========================================================================
    //! Function FVSCHEDULE - Future Value with Variable Interest Rates
    tFunctionFvschedule::tFunctionFvschedule() : tFunctionFinancialBase() {}

    tStackElem tFunctionFvschedule::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() != 2) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "FVSCHEDULE requires 2 arguments"))));
        }

        // FVSCHEDULE(principal, schedule)
        // In RPN order: schedule principal FVSCHEDULE -> wArgs[0]=schedule, wArgs[1]=principal
        tDouble wPrincipal = 0.0;
        tVariant wError = ExtractDoubleArg(&wArgs, 1, "FVSCHEDULE", "principal", wPrincipal);
        if (wError.IsError()) {
            return wError;
        }

        // First argument (schedule) - range or array
        tDouble wFv = wPrincipal;
        if (wArgs.size() >= 1) {
            tStackElem* wArg = &wArgs[0];
            if (wArg->Type() == tStackType::t_Range) {
                // Process range of interest rates
                tRange* wRange = wArg->Range();
                if (wRange) {
                    tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
                    for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); wRow++) {
                        for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); wCol++) {
                            tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                            if (wCell != nullptr) {
                                tVariant wCellValue = wCell->CalculableValue();
                                if (wCellValue.IsError()) {
                                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "FVSCHEDULE: schedule rates must be numeric"))));
                                }
                                // Excel treats empty/null values as 0 in financial functions
                                if (wCellValue.IsNull()) {
                                    // Empty cell means rate = 0, so wFv remains unchanged
                                } else if (wCellValue.IsDouble()) {
                                    tDouble wRate = wCellValue.Double();
                                    wFv = wFv * (1.0 + wRate);
                                } else if (wCellValue.IsInt()) {
                                    tDouble wRate = static_cast<tDouble>(wCellValue.Int());
                                    wFv = wFv * (1.0 + wRate);
                                } else {
                                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "FVSCHEDULE: schedule rates must be numeric"))));
                                }
                            }
                        }
                    }
                }
            } else {
                tVariant wVariant;
                if (StackElemToVariant(*wArg, wVariant)) {
                    // Single value (Variant, Cell, or single-cell Range)
                    if (wVariant.IsError()) return tStackElem(tVariant(wVariant));
                    if (!wVariant.IsNull()) {
                        tDouble wRate = wVariant.IsDouble() ? wVariant.Double() : static_cast<tDouble>(wVariant.Int());
                        wFv = wFv * (1.0 + wRate);
                    }
                } else {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "FVSCHEDULE: schedule must be a range or array"))));
                }
            }
        }

        // Clean up arguments

        return(tStackElem(tVariant(wFv)));
    }

    //=========================================================================
    //! Function VDB - Variable Declining Balance Depreciation
    tFunctionVdb::tFunctionVdb() : tFunctionFinancialBase() {}

    tStackElem tFunctionVdb::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 5 || wArgs.size() > 7) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "VDB requires 5 to 7 arguments"))));
        }

        // VDB(cost, salvage, life, start_period, end_period, [factor], [no_switch])
        // In RPN order: 
        //   5 args: end_period start_period life salvage cost VDB -> wArgs[0]=end_period, wArgs[1]=start_period, wArgs[2]=life, wArgs[3]=salvage, wArgs[4]=cost
        //   6 args: factor end_period start_period life salvage cost VDB -> wArgs[0]=factor, wArgs[1]=end_period, wArgs[2]=start_period, wArgs[3]=life, wArgs[4]=salvage, wArgs[5]=cost
        //   7 args: no_switch factor end_period start_period life salvage cost VDB -> wArgs[0]=no_switch, wArgs[1]=factor, wArgs[2]=end_period, wArgs[3]=start_period, wArgs[4]=life, wArgs[5]=salvage, wArgs[6]=cost
        tDouble wCost = 0.0;
        tDouble wSalvage = 0.0;
        tDouble wLife = 0.0;
        tDouble wStartPeriod = 0.0;
        tDouble wEndPeriod = 0.0;
        tDouble wFactor = 2.0;
        tInt wNoSwitch = 0;
        tVariant wError;
        
        // Calculate indices dynamically based on wArgs.size()
        // Pattern: cost=size-1, salvage=size-2, life=size-3, start_period=size-4, end_period=size-5, factor=size-6, no_switch=size-7
        size_t wSize = wArgs.size();
        
        // Extract cost (always at index size-1)
        wError = ExtractDoubleArg(&wArgs, wSize - 1, "VDB", "cost", wCost);
        if (wError.IsError()) return wError;
        
        // Extract salvage (always at index size-2)
        wError = ExtractDoubleArg(&wArgs, wSize - 2, "VDB", "salvage", wSalvage);
        if (wError.IsError()) return wError;
        
        // Extract life (always at index size-3)
        wError = ExtractDoubleArg(&wArgs, wSize - 3, "VDB", "life", wLife);
        if (wError.IsError()) return wError;
        
        // Extract start_period (always at index size-4)
        wError = ExtractDoubleArg(&wArgs, wSize - 4, "VDB", "start_period", wStartPeriod);
        if (wError.IsError()) return wError;
        
        // Extract end_period (always at index size-5)
        wError = ExtractDoubleArg(&wArgs, wSize - 5, "VDB", "end_period", wEndPeriod);
        if (wError.IsError()) return wError;
        
        // Extract factor (at index size-6 if size >= 6, optional)
        if (wSize >= 6) {
            wError = ExtractDoubleArg(&wArgs, wSize - 6, "VDB", "factor", wFactor, 2.0, true);
            if (wError.IsError()) return wError;
        }
        
        // Extract no_switch (at index size-7 if size == 7, optional)
        if (wSize == 7) {
            wError = ExtractIntArg(&wArgs, wSize - 7, "VDB", "no_switch", wNoSwitch, 0, true);
            if (wError.IsError()) return wError;
        }
        
        // Clean up arguments

        // Validate arguments
        if (wCost < 0 || wSalvage < 0 || wLife <= 0 || wStartPeriod < 0 || wEndPeriod < wStartPeriod || wEndPeriod > wLife || wFactor <= 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "VDB: invalid argument values"))));
        }

        // Calculate VDB depreciation
        tDouble wDepreciation = 0.0;
        tDouble wBookValue = wCost;
        
        for (tInt wPeriod = static_cast<tInt>(wStartPeriod); wPeriod < static_cast<tInt>(wEndPeriod); wPeriod++) {
            if (wPeriod >= static_cast<tInt>(wLife)) {
                break;
            }
            
            // Calculate depreciation rate
            tDouble wRate = wFactor / wLife;
            
            // Calculate depreciation for this period
            tDouble wPeriodDepreciation = wBookValue * wRate;
            
            // Check if we should switch to straight-line
            if (wNoSwitch == 0) {
                tDouble wStraightLineDepreciation = (wBookValue - wSalvage) / (wLife - static_cast<tDouble>(wPeriod));
                if (wStraightLineDepreciation > wPeriodDepreciation) {
                    wPeriodDepreciation = wStraightLineDepreciation;
                }
            }
            
            // Ensure we don't depreciate below salvage value
            if (wBookValue - wPeriodDepreciation < wSalvage) {
                wPeriodDepreciation = wBookValue - wSalvage;
            }
            
            wDepreciation += wPeriodDepreciation;
            wBookValue -= wPeriodDepreciation;
            
            if (wBookValue <= wSalvage) {
                break;
            }
        }

        return(tStackElem(tVariant(wDepreciation)));
    }

    //=========================================================================
    //! Function XNPV - Net Present Value for Non-Periodic Cash Flows
    tFunctionXnpv::tFunctionXnpv() : tFunctionFinancialBase() {}

    tStackElem tFunctionXnpv::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "XNPV requires at least 3 arguments"))));
        }

        // XNPV(rate, values, dates)
        // In RPN order: dates values rate XNPV -> wArgs[0]=dates, wArgs[1]=values, wArgs[2]=rate
        tDouble wRate = 0.0;
        tVariant wError = ExtractDoubleArg(&wArgs, 2, "XNPV", "rate", wRate);
        if (wError.IsError()) {
            return wError;
        }

        // Second argument (values) - range or array
        std::vector<tDouble> wValues;
        if (wArgs.size() >= 2) {
            tStackElem* wArg = &wArgs[1];
            if (wArg->Type() == tStackType::t_Range) {
                tRange* wRange = wArg->Range();
                if (wRange) {
                    tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
                    for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); wRow++) {
                        for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); wCol++) {
                            tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                            if (wCell != nullptr) {
                                tVariant wCellValue = wCell->CalculableValue();
                                if (wCellValue.IsError()) {
                                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XNPV: values must be numeric"))));
                                }
                                // Excel treats empty/null values as 0 in financial functions
                                if (wCellValue.IsNull()) {
                                    wValues.push_back(0.0);
                                } else if (wCellValue.IsDouble()) {
                                    wValues.push_back(wCellValue.Double());
                                } else if (wCellValue.IsInt()) {
                                    wValues.push_back(static_cast<tDouble>(wCellValue.Int()));
                                } else {
                                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XNPV: values must be numeric"))));
                                }
                            }
                        }
                    }
                }
            } else {
                tVariant wVariant;
                if (StackElemToVariant(*wArg, wVariant)) {
                    if (wVariant.IsError()) return tStackElem(tVariant(wVariant));
                    if (wVariant.IsNull()) {
                        wValues.push_back(0.0);
                    } else if (wVariant.IsDouble()) {
                        wValues.push_back(wVariant.Double());
                    } else if (wVariant.IsInt()) {
                        wValues.push_back(static_cast<tDouble>(wVariant.Int()));
                    }
                } else {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "XNPV: values must be a range or array"))));
                }
            }
        }

        // First argument (dates) - range or array
        std::vector<tDouble> wDates;
        if (wArgs.size() >= 1) {
            tStackElem* wArg = &wArgs[0];
            if (wArg->Type() == tStackType::t_Range) {
                tRange* wRange = wArg->Range();
                if (wRange) {
                    tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
                    for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); wRow++) {
                        for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); wCol++) {
                            tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                            if (wCell != nullptr) {
                                tVariant wCellValue = wCell->CalculableValue();
                                if (wCellValue.IsError()) {
                                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XNPV: dates must be numeric"))));
                                }
                                // Excel treats empty/null values as 0 in financial functions
                                if (wCellValue.IsNull()) {
                                    wDates.push_back(0.0);
                                } else if (wCellValue.IsDouble()) {
                                    wDates.push_back(wCellValue.Double());
                                } else if (wCellValue.IsInt()) {
                                    wDates.push_back(static_cast<tDouble>(wCellValue.Int()));
                                } else {
                                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XNPV: dates must be numeric"))));
                                }
                            }
                        }
                    }
                }
            } else {
                tVariant wVariant;
                if (StackElemToVariant(*wArg, wVariant)) {
                    if (wVariant.IsError()) return tStackElem(tVariant(wVariant));
                    if (wVariant.IsDouble()) {
                        wDates.push_back(wVariant.Double());
                    } else if (wVariant.IsInt()) {
                        wDates.push_back(static_cast<tDouble>(wVariant.Int()));
                    }
                } else {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "XNPV: dates must be a range or array"))));
                }
            }
        }

        // Clean up arguments

        // Validate arguments
        if (wValues.size() != wDates.size() || wValues.size() == 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XNPV: values and dates must have the same number of elements"))));
        }

        // Calculate XNPV: sum of (value / (1 + rate)^((date - first_date) / 365))
        tDouble wXnpv = 0.0;
        if (wDates.size() > 0) {
            tDouble wFirstDate = wDates[0];
            for (size_t wI = 0; wI < wValues.size(); wI++) {
                tDouble wDaysDiff = wDates[wI] - wFirstDate;
                tDouble wYearsDiff = wDaysDiff / 365.0;
                wXnpv += wValues[wI] / pow(1.0 + wRate, wYearsDiff);
            }
        }

        return(tStackElem(tVariant(wXnpv)));
    }

    //=========================================================================
    //! Function XIRR - Internal Rate of Return for Non-Periodic Cash Flows
    tFunctionXirr::tFunctionXirr() : tFunctionFinancialBase() {}

    tStackElem tFunctionXirr::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
        if (wArgs.size() < 2 || wArgs.size() > 3) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "XIRR requires 2 or 3 arguments"))));
        }

        // XIRR(values, dates, [guess])
        // PopArgs is LIFO; reverse so wArgs[0]=values, [1]=dates, [2]=guess
        std::reverse(wArgs.begin(), wArgs.end());

        // First argument (values) - range or array
        std::vector<tDouble> wValues;
        if (wArgs.size() >= 1) {
            tStackElem* wArg = &wArgs[0];
            if (wArg->Type() == tStackType::t_Range) {
                tRange* wRange = wArg->Range();
                if (wRange) {
                    tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
                    for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); wRow++) {
                        for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); wCol++) {
                            tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                            if (wCell != nullptr) {
                                tVariant wCellValue = wCell->CalculableValue();
                                if (wCellValue.IsError()) {
                                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XIRR: values must be numeric"))));
                                }
                                // Excel treats empty/null values as 0 in financial functions
                                if (wCellValue.IsNull()) {
                                    wValues.push_back(0.0);
                                } else if (wCellValue.IsDouble()) {
                                    wValues.push_back(wCellValue.Double());
                                } else if (wCellValue.IsInt()) {
                                    wValues.push_back(static_cast<tDouble>(wCellValue.Int()));
                                } else {
                                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XIRR: values must be numeric"))));
                                }
                            }
                        }
                    }
                }
            } else {
                tVariant wVariant;
                if (StackElemToVariant(*wArg, wVariant)) {
                    if (wVariant.IsError()) return tStackElem(tVariant(wVariant));
                    if (wVariant.IsNull()) {
                        wValues.push_back(0.0);
                    } else if (wVariant.IsDouble()) {
                        wValues.push_back(wVariant.Double());
                    } else if (wVariant.IsInt()) {
                        wValues.push_back(static_cast<tDouble>(wVariant.Int()));
                    }
                } else {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "XIRR: values must be a range or array"))));
                }
            }
        }

        // Second argument (dates) - range or array
        std::vector<tDouble> wDates;
        if (wArgs.size() >= 2) {
            tStackElem* wArg = &wArgs[1];
            if (wArg->Type() == tStackType::t_Range) {
                tRange* wRange = wArg->Range();
                if (wRange) {
                    tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
                    for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); wRow++) {
                        for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); wCol++) {
                            tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                            if (wCell != nullptr) {
                                tVariant wCellValue = wCell->CalculableValue();
                                if (wCellValue.IsError()) {
                                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XIRR: dates must be numeric"))));
                                }
                                // Excel treats empty/null values as 0 in financial functions
                                if (wCellValue.IsNull()) {
                                    wDates.push_back(0.0);
                                } else if (wCellValue.IsDouble()) {
                                    wDates.push_back(wCellValue.Double());
                                } else if (wCellValue.IsInt()) {
                                    wDates.push_back(static_cast<tDouble>(wCellValue.Int()));
                                } else {
                                    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XIRR: dates must be numeric"))));
                                }
                            }
                        }
                    }
                }
            } else {
                tVariant wVariant;
                if (StackElemToVariant(*wArg, wVariant)) {
                    if (wVariant.IsError()) return tStackElem(tVariant(wVariant));
                    if (wVariant.IsNull()) {
                        wDates.push_back(0.0);
                    } else if (wVariant.IsDouble()) {
                        wDates.push_back(wVariant.Double());
                    } else if (wVariant.IsInt()) {
                        wDates.push_back(static_cast<tDouble>(wVariant.Int()));
                    }
                } else {
                    return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "XIRR: dates must be a range or array"))));
                }
            }
        }

        // Optional third argument (guess) - default 0.1
        tDouble wGuess = 0.1;
        if (wArgs.size() >= 3) {
            tVariant wError = ExtractDoubleArg(&wArgs, 2, "XIRR", "guess", wGuess, 0.1, true);
            if (wError.IsError()) {
                return wError;
            }
        }
        
        // Clean up arguments

        // Validate arguments
        if (wValues.size() != wDates.size() || wValues.size() == 0) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XIRR: values and dates must have the same number of elements"))));
        }

        // Use Newton-Raphson method to find XIRR
        // XNPV(rate) = 0, solve for rate
        tDouble wRate = wGuess;
        const tInt wMaxIterations = 100;
        const tDouble wTolerance = 1e-6;
        
        tDouble wFirstDate = wDates[0];
        
        for (tInt wIter = 0; wIter < wMaxIterations; wIter++) {
            // Calculate XNPV and its derivative
            tDouble wXnpv = 0.0;
            tDouble wXnpvDerivative = 0.0;
            
            for (size_t wI = 0; wI < wValues.size(); wI++) {
                tDouble wDaysDiff = wDates[wI] - wFirstDate;
                tDouble wYearsDiff = wDaysDiff / 365.0;
                tDouble wFactor = pow(1.0 + wRate, wYearsDiff);
                wXnpv += wValues[wI] / wFactor;
                if (wYearsDiff != 0.0) {
                    wXnpvDerivative -= wValues[wI] * wYearsDiff / (wFactor * (1.0 + wRate));
                }
            }
            
            if (fabs(wXnpvDerivative) < wTolerance) {
                break;
            }
            
            tDouble wNewRate = wRate - wXnpv / wXnpvDerivative;
            
            if (fabs(wNewRate - wRate) < wTolerance) {
                wRate = wNewRate;
                break;
            }
            
            wRate = wNewRate;
            
            // Prevent negative rates or rates that are too high
            if (wRate < -0.99 || wRate > 10.0) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XIRR: could not find a solution"))));
            }
        }
        
        // Verify the solution
        tDouble wFinalXnpv = 0.0;
        for (size_t wI = 0; wI < wValues.size(); wI++) {
            tDouble wDaysDiff = wDates[wI] - wFirstDate;
            tDouble wYearsDiff = wDaysDiff / 365.0;
            wFinalXnpv += wValues[wI] / pow(1.0 + wRate, wYearsDiff);
        }
        
        if (fabs(wFinalXnpv) > 0.01) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, "XIRR: could not find a solution"))));
        }

        return(tStackElem(tVariant(wRate)));
    }

	tFunctionSpillKind tFunctionFinancialBase::SpillKind() const {
		return(tFunctionSpillKind::Aggregate);
	}

} // namespace SkSpreadSheet

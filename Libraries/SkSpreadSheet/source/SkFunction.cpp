//=============================================================================
// SkSpreadSheet Function tCell & tRange for function
//=============================================================================
#include "../include/SkFunction.hpp"
#include "../include/SkColRowCellRange.hpp"
#include "../include/SkCell.hpp"
#include "../include/SkRange.hpp"
#include <algorithm>
#include <cctype>

namespace SkSpreadSheet {

    namespace {
        void UppercaseFunctionName(tString& ioName) {
            std::transform(ioName.begin(), ioName.end(), ioName.begin(),
                           [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
        }
    }

	// Function =============================================================
	tFunction::tFunction() : tClass() {}
	tFunction::~tFunction() {}
	tStackElem tFunction::Call(tStackElems* sStackElems, tShort sNbArg) { return(tStackElem(tVariant())); }
	void tFunction::PassByRef(tItem* sItem) {}
	tBool tFunction::ByRef() { return(false); }

	tFunctionSpillKind tFunction::SpillKind() const {
		return(tFunctionSpillKind::ElementWise);
	}

#ifdef _DEBUGSK
	tString tFunction::Debug(tString sFunctionName, tStackElems sStackElemsCopy, tShort sNbArg) {
		tStringStream wStream;
        wStream << "Function: " << sFunctionName << " NbArg=" << sNbArg << " Args=" << sNbArg << endl;
    
		std::vector<tStackElem> wArgs;
        wArgs.reserve(sNbArg);
		for (tShort i = 0; i < sNbArg; i++) {
			wArgs.push_back(sStackElemsCopy.top());
			sStackElemsCopy.pop();
		}
        tCell* wCell=nullptr;
        tRange* wRange=nullptr;
        
		for (tShort i = sNbArg - 1; i >= 0; i--) {
			wStream << "    Arg[" << sNbArg  - i << "]=";
			switch (wArgs[i].Type()) {
				case tStackType::t_Variant:
					// Use stream operator directly to avoid String() exception for non-string types
					wStream << "Variant:<" << wArgs[i].Variant() << ">";
					break;
                case tStackType::t_Attribute:
				case tStackType::t_Cell:
					wCell = wArgs[i].Cell();
					if (wCell != nullptr) {
						// Use stream operator directly to avoid String() exception for non-string types
						wStream << "Cell: " << wCell->StrRef(true) << "=<" << wCell->Value() << ">";
					} else {
						wStream << "Cell: nullptr ";
					}
					break;
				case tStackType::t_Range:
					wRange = wArgs[i].Range();
					if (wRange != nullptr) {
                        wStream << "Range: " << wRange->StrRef(true)  << " " ;
                        if (wRange->Name()!="") wStream << wRange->Name()  <<" ";
                        wCell = wRange->Cell();
                        if (wCell != nullptr) {
							// Use stream operator directly to avoid String() exception for non-string types
                            wStream << "/->Cell: " << wCell->StrRef(true) << "=<" << wCell->Value() << ">";
                        } else {
                            wStream << "/->Cell: nullptr ";
                        }
                    } else {
                        wStream << "Range: nullptr ";
                    }
                    wStream << endl;
                    break;
                default : break;
            }
            wStream << endl;
        }
		return(wStream.str());
	}
#endif
    //=========================================================================
    //! Pop N arguments from stack into a temporary vector
    std::vector<tStackElem> tFunction::PopArgs(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs;
        wArgs.reserve(sNbArg);
        
        // Pop N arguments from the stack (last argument first in RPN)
        for (tShort i = 0; i < sNbArg && !sStackElems->empty(); i++) {
            tStackElem wStackElem = sStackElems->top();
            sStackElems->pop();
            wArgs.push_back(wStackElem);
        }
        
        return wArgs;
    }

    //=========================================================================
    //! Resolve stack elem (Variant, Cell, or Range) to value for function args
    tBool tFunction::StackElemToVariant(const tStackElem& sArg, tVariant& sOut) {
        switch (sArg.Type()) {
            case tStackType::t_Variant:
                sOut = tCell::CalculableScalarFromVariant(sArg.Variant());
                return true;
            case tStackType::t_Cell: {
                // Keep THIS cell (origin or MatExtend slave). Do not expand a spill slave
                // to the origin / full array — TEXT(C9) must format C9, not B9.
                tCell* wCell = sArg.Cell();
                if (wCell != nullptr) {
                    sOut = wCell->CalculableScalarValue();
                    return true;
                }
                return false;
            }
            case tStackType::t_Range: {
                tRange* wRange = sArg.Range();
                if (wRange != nullptr) {
                    tCell* wCell = wRange->EnsureCell();
                    if (wCell != nullptr) {
                        sOut = wCell->CalculableScalarValue();
                        if (sOut.IsError()) sOut = tCell::CalculableScalarFromVariant(wCell->Value());
                        return true;
                    }
                }
                return false;
            }
            case tStackType::t_Array: {
                // Excel @: an array used where a scalar is expected is the top-left element.
                tArrayValue* wArr = sArg.Array();
                if (wArr != nullptr && wArr->Count() > 0) {
                    sOut = tCell::CalculableScalarFromVariant(wArr->At(0, 0));
                    return true;
                }
                return false;
            }
            default:
                return false;
        }
    }

    tBool tFunction::StackElemToIndex(const tStackElem& sArg, tIndex& sOut) {
        tVariant v;
        if (!StackElemToVariant(sArg, v)) return false;
        if (v.IsInt()) { sOut = static_cast<tIndex>(v.Int()); return true; }
        if (v.IsDouble()) { sOut = static_cast<tIndex>(v.Double()); return true; }
        // Excel: date coerces to serial days (same basis as tClassDate(const tVariant&) int/double path).
        if (v.IsDate()) {
            const tDouble wSerial = static_cast<tDouble>(v.Date()) / 86400.0 + 25569.0;
            sOut = static_cast<tIndex>(wSerial);
            return true;
        }
        return false;
    }

    tBool tFunction::StackElemToInt(const tStackElem& sArg, tInt& sOut) {
        tVariant v;
        if (!StackElemToVariant(sArg, v)) return false;
        if (v.IsInt()) { sOut = v.Int(); return true; }
        if (v.IsDouble()) { sOut = static_cast<tInt>(v.Double()); return true; }
        // Excel: DATE(year,month,day) truncates each argument after date-to-serial coercion.
        if (v.IsDate()) {
            const tDouble wSerial = static_cast<tDouble>(v.Date()) / 86400.0 + 25569.0;
            sOut = static_cast<tInt>(wSerial);
            return true;
        }
        return false;
    }

    tBool tFunction::StackElemToBool(const tStackElem& sArg, tBool& sOut) {
        tVariant v;
        if (!StackElemToVariant(sArg, v)) return false;
        if (v.IsBool()) { sOut = v.Bool(); return true; }
        if (v.IsInt()) { sOut = (v.Int() != 0); return true; }
        if (v.IsDouble()) { sOut = (v.Double() != 0); return true; }
        return false;
    }

    tBool tFunction::StackElemToArray(const tStackElem& sArg, tArrayValue& sOut) {
        switch (sArg.Type()) {
            case tStackType::t_Array: {
                tArrayValue* wArr = sArg.Array();
                if (wArr == nullptr) return false;
                sOut = *wArr;
                return true;
            }
            case tStackType::t_Range: {
                tRange* wRange = sArg.Range();
                if (wRange == nullptr) return false;
                tColRowCellRange* wCr = wRange->ColRowCellRange();
                if (wCr == nullptr) return false;
                const tIndex wTop = wRange->TopIndex();
                const tIndex wLeft = wRange->LeftIndex();
                const tIndex wBottom = wRange->IterateBottom();
                const tIndex wRight = wRange->IterateRight();
                if (wBottom < wTop || wRight < wLeft) return false;
                const tIndex wRows = wBottom - wTop + 1;
                const tIndex wCols = wRight - wLeft + 1;
                sOut = tArrayValue(wRows, wCols);
                for (tIndex r = 0; r < wRows; ++r) {
                    for (tIndex c = 0; c < wCols; ++c) {
                        tCell* wCell = wCr->Cell(wTop + r, wLeft + c);
                        if (wCell != nullptr) {
                            sOut.At(r, c) = wCell->CalculableValue();
                        }
                    }
                }
                return true;
            }
            case tStackType::t_Cell:
            case tStackType::t_Attribute: {
                tCell* wCell = sArg.Cell();
                sOut = tArrayValue(1, 1);
                if (wCell != nullptr) {
                    sOut.At(0, 0) = wCell->CalculableValue();
                }
                return true;
            }
            case tStackType::t_Variant: {
                sOut = tArrayValue(1, 1);
                sOut.At(0, 0) = sArg.Variant();
                return true;
            }
            default:
                return false;
        }
    }

	// Reference for function =================================================
    tFunctionRef::tFunctionRef() : tClass(),m_Name(),m_Label(),m_Family(),m_NbArg(-1), m_Function(nullptr) {}
    tFunctionRef::tFunctionRef(const tFunctionRef& sFunctionRef) : tClass(sFunctionRef),m_Name(sFunctionRef.m_Name),m_Label(sFunctionRef.m_Label),m_Family(sFunctionRef.m_Family), m_NbArg(sFunctionRef.m_NbArg), m_Volatile(sFunctionRef.m_Volatile),m_Function(sFunctionRef.m_Function) {}
    tFunctionRef::tFunctionRef(tString sName,tString sLabel,tString sFamily,tInt sNbArg,tFunction* sFunction, tVolatile sVolatile) : tClass(), m_Name(sName),m_Label(sLabel),m_Family(sFamily), m_NbArg(sNbArg), m_Volatile(sVolatile), m_Function(sFunction){}

    void tFunctionRef::Name(tString sName) { m_Name=sName; };
    tString tFunctionRef::Name() { return(m_Name()); };

    void tFunctionRef::Label(tString sLabel) { m_Label=sLabel; };
    tString tFunctionRef::Label() { return(m_Label()); };

    void tFunctionRef::Family(tString sFamily) { m_Family=sFamily; };
    tString tFunctionRef::Family() { return(m_Family()); };

	tInt tFunctionRef::NbArg() { return(m_NbArg); };

	tFunction* tFunctionRef::Function() { return(m_Function); }

	// Dictionary =============================================================
    tFunctionDictionary::tFunctionDictionary() : tClass(),m_MapFunctionRef() {}
    
    tFunctionDictionary::~tFunctionDictionary() {
		for (auto wFunction : m_MapFunctionRef) {
			tFunction* wFunc = wFunction.second.Function();
			if (wFunc != nullptr) {
				delete(wFunc);
			}
		}
		m_MapFunctionRef.clear();
	}

	tBool tFunctionDictionary::AddFunctionRef(tString sName,tString sLabel,tString sFamily, tInt sNbArg, tFunction* sFunction, tVolatile sVolatile) {
		auto wIterator = m_MapFunctionRef.find(sName);
		if (wIterator != m_MapFunctionRef.end()) {
			return(false);
		}
		m_MapFunctionRef[sName] = tFunctionRef(sName,sLabel,sFamily,sNbArg, sFunction, sVolatile);
		return(true);
	}

	tBool tFunctionDictionary::DeleteFunctionRef(tString sName) {
		auto wIterator = m_MapFunctionRef.find(sName);
		if (wIterator != m_MapFunctionRef.end()) {
			m_MapFunctionRef.erase(wIterator);
			return(true);
		}
		return(false);
	}

	tFunctionRef& tFunctionDictionary::FunctionRef(tString sName) {
        UppercaseFunctionName(sName);
		auto wIterator = m_MapFunctionRef.find(sName);
		if (wIterator == m_MapFunctionRef.end()) {
			return(m_EmptyFunctionRef);
		}
		return(m_MapFunctionRef[sName]);
	}

	tInt tFunctionDictionary::NbArg(tString sName) {
        UppercaseFunctionName(sName);
		tFunctionRef wFunctionRef = FunctionRef(sName);
		return(wFunctionRef.NbArg());
	}

	tBool tFunctionDictionary::Exist(tString sName) {
        UppercaseFunctionName(sName);
		auto wIterator = m_MapFunctionRef.find(sName);
		return(!(wIterator == m_MapFunctionRef.end()));
	}

}; // end of namespace

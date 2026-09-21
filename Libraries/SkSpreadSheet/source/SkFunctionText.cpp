//======================================================================
// FILE: SkFunctionText.cpp
// DATE: 2025-08-23
//======================================================================

#include "../include/SkFunctionText.hpp"
#include "../include/SkCell.hpp"
#include "../../SkRoot/include/SkUtf.hpp"
#include "../../SkRoot/include/SkFormatDate.hpp"
#include "../../SkRoot/include/SkFormatNumber.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <regex>
#include <vector>

//#define debugtext 

namespace SkSpreadSheet {

tFunctionConcat::tFunctionConcat() : tFunction(),m_Result(){}

void tFunctionConcat::Concat(tVariant sValue) {
    tStringStream wStream;
    if(sValue.Type() == tVariantType::t_string) {
        wStream << sValue.String();
    }
    if (sValue.Type() == tVariantType::t_int) {
        wStream <<  sValue.Int();
    }
    if (sValue.Type() == tVariantType::t_double) {
        wStream <<  sValue.Double();
    }
    if (sValue.Type() == tVariantType::t_date) {
        wStream << tClassDate(sValue.Date()).UsDate();
    }
    if (sValue.Type() == tVariantType::t_bool) {
        tString wValue =  (sValue.Bool() ? "TRUE" : "FALSE");
        wStream << wValue;
    }
    
    m_Result+=wStream.str();
}

tStackElem tFunctionConcat::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    m_Result = ""; // Reset result
    // Process arguments in reverse order (first argument was last on stack)
    for (auto it = wArgs.rbegin(); it != wArgs.rend(); ++it) {
        tStackElem& wArg = *it;
        if (wArg.Type() == tStackType::t_Range) {
            tRange* wRange = wArg.Range();
            if (wRange != nullptr) {
                tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
                if (wColRowCellRange != nullptr) {
                    for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); wRow++) {
                        for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); wCol++) {
                            tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                            if (wCell != nullptr) Concat(wCell->Value());
                        }
                    }
                }
            }
        } else {
            tVariant wValue;
            if (StackElemToVariant(wArg, wValue)) {
                if (wValue.IsError()) return(tStackElem(tVariant(wValue)));
                Concat(wValue);
            }
        }
    }
    return(tStackElem(tVariant(m_Result)));
}

tFunctionLeft::tFunctionLeft() : tFunction() {}

tStackElem tFunctionLeft::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    // LEFT(text, [num_chars]) - num_chars is optional and defaults to 1.
    if ((wArgs.size() != 1) && (wArgs.size() != 2)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    // In RPN the text is pushed first, so it is the last popped element.
    tStackElem* wArgText = &wArgs[wArgs.size() - 1];

    tVariant wText;
    if (!StackElemToVariant(*wArgText, wText)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "LEFT: invalid text argument"))));
    }
    if (wText.IsError()) return(tStackElem(tVariant(wText)));
    if (wText.Type() != tVariantType::t_string) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "LEFT: text must be string"))));
    }

    tInt wLen = 1; // Excel default when num_chars is omitted.
    if (wArgs.size() == 2) {
        tVariant wLenVar;
        if (!StackElemToVariant(wArgs[0], wLenVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "LEFT: invalid num_chars argument"))));
        }
        if (wLenVar.IsError()) return(tStackElem(tVariant(wLenVar)));
        if (!StackElemToInt(wArgs[0], wLen)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "LEFT: num_chars must be numeric"))));
        }
    }
    if (wLen < 0) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "LEFT: num_chars must be >= 0"))));
    }
#ifdef debugtext
    cout << "Left(" << wText.String()  << ","  << wLen <<")" << endl;
#endif
    tClassString wClassString(wText.String());
    tString wResult=wClassString.Left(wLen);
    // Clean up
    return(tStackElem(tVariant(wResult)));
}  

tFunctionRight::tFunctionRight() : tFunction() {}

tStackElem tFunctionRight::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    // RIGHT(text, [num_chars]) - num_chars is optional and defaults to 1.
    if ((wArgs.size() != 1) && (wArgs.size() != 2)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    // In RPN the text is pushed first, so it is the last popped element.
    tStackElem* wArgText = &wArgs[wArgs.size() - 1];

    tVariant wText;
    if (!StackElemToVariant(*wArgText, wText)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "RIGHT: invalid text argument"))));
    }
    if (wText.IsError()) return(tStackElem(tVariant(wText)));
    if (wText.Type() != tVariantType::t_string) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "RIGHT: text must be string"))));
    }

    tInt wLen = 1; // Excel default when num_chars is omitted.
    if (wArgs.size() == 2) {
        tVariant wLenVar;
        if (!StackElemToVariant(wArgs[0], wLenVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "RIGHT: invalid num_chars argument"))));
        }
        if (wLenVar.IsError()) return(tStackElem(tVariant(wLenVar)));
        if (!StackElemToInt(wArgs[0], wLen)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "RIGHT: num_chars must be numeric"))));
        }
    }
    if (wLen < 0) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "RIGHT: num_chars must be >= 0"))));
    }
#ifdef debugtext
    cout << "Right(" << wText.String()  << ","  << wLen <<")" << endl;
#endif
    tClassString wClassString(wText.String());
    tString wResult=wClassString.Right(wLen);
    // Clean up
    return(tStackElem(tVariant(wResult)));
}

tFunctionMid::tFunctionMid() : tFunction() {}

tStackElem tFunctionMid::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 3) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    // MID(text, start_num, num_chars) - 3 arguments
    // In RPN: text start_num num_chars MID, so after PopArgs: wArgs[0]=num_chars, wArgs[1]=start_num, wArgs[2]=text
    tStackElem* wArgText = &wArgs[2]; // text (first arg, last popped)
    tStackElem* wArgStart = &wArgs[1]; // start_num (second arg)
    tStackElem* wArgLen = &wArgs[0]; // num_chars (third arg, first popped)
    
    tVariant wText;
    tVariant wStart;
    tVariant wLen;
    if (!StackElemToVariant(*wArgText, wText)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MID: invalid text argument"))));
    }
    if (wText.IsError()) return(tStackElem(tVariant(wText)));
    if (!StackElemToVariant(*wArgStart, wStart)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MID: invalid start_num argument"))));
    }
    if (wStart.IsError()) return(tStackElem(tVariant(wStart)));
    if (!StackElemToVariant(*wArgLen, wLen)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "MID: invalid num_chars argument"))));
    }
    if (wLen.IsError()) return(tStackElem(tVariant(wLen)));
    if ((wStart.Type() != tVariantType::t_int) || (wLen.Type() != tVariantType::t_int) || (wText.Type() != tVariantType::t_string)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    if (wStart.Int()<1) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
#ifdef debugtext
    cout << "Mid(" << wText.String()  << ","  << wStart.Int() << ","  << wLen.Int() <<")" << endl;
#endif
    tClassString wClassString(wText.String());
    tString wResult=wClassString.Mid(wStart.Int()-1, wLen.Int());
    // Clean up
    return(tStackElem(tVariant(wResult)));
}

tFunctionLen::tFunctionLen() : tFunction() {}

tStackElem tFunctionLen::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 1) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    // LEN(text) - 1 argument; In RPN: wArgs[0]=text
    tStackElem* wArg = &wArgs[0];
    tVariant wValue;
    if (!StackElemToVariant(*wArg, wValue)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "LEN: argument must be a value or cell"))));
    }
    // Return t_arg for error input (same as old "unsupported value type" behavior)
    if (wValue.IsError()) return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "LEN: unsupported value type"))));
    if (wValue.IsNull()) return(tStackElem(tVariant(tInt(0))));
    
    // Convert value to string (like Excel does)
    // Excel's LEN() converts numbers, dates, etc. to text automatically
    tString wTextString;
    if (wValue.Type() == tVariantType::t_string) {
        wTextString = wValue.String();
    } else {
        // Convert other types to string
        tStringStream wStream;
        if (wValue.Type() == tVariantType::t_int) {
            wStream << wValue.Int();
        } else if (wValue.Type() == tVariantType::t_double) {
            wStream << wValue.Double();
        } else if (wValue.Type() == tVariantType::t_date) {
            wStream << tClassDate(wValue.Date()).UsDate();
        } else if (wValue.Type() == tVariantType::t_bool) {
            wStream << (wValue.Bool() ? "TRUE" : "FALSE");
        } else {
            // Unsupported type
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "LEN: unsupported value type"))));
        }
        wTextString = wStream.str();
    }
   
    // Count UTF-8 characters instead of bytes
    tSize wResult = SkRoot::CountUtf8Characters(wTextString);
    // Clean up
    return(tStackElem(tVariant(tInt(wResult))));
}

namespace {
	// Excel EXACT coerces non-text values to their display text before comparing.
	static tBool ExactToText(const tVariant& sValue, tString& sOut) {
		if (sValue.IsError()) {
			return false;
		}
		if (sValue.IsString()) {
			sOut = sValue.String();
			return true;
		}
		if (sValue.IsBool()) {
			sOut = sValue.Bool() ? "TRUE" : "FALSE";
			return true;
		}
		if (sValue.IsNull()) {
			sOut.clear();
			return true;
		}
		if (sValue.IsInt()) {
			tStringStream wStream;
			wStream << sValue.Int();
			sOut = wStream.str();
			return true;
		}
		if (sValue.IsDouble()) {
			tStringStream wStream;
			wStream << sValue.Double();
			sOut = wStream.str();
			return true;
		}
		if (sValue.IsDate()) {
			tStringStream wStream;
			wStream << sValue;
			sOut = wStream.str();
			return true;
		}
		sOut.clear();
		return true;
	}
} // namespace

tFunctionExact::tFunctionExact() : tFunction() {}

tStackElem tFunctionExact::Call(tStackElems* sStackElems, tShort sNbArg) {
	std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
	if (wArgs.size() != 2) {
		return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "EXACT requires exactly 2 arguments"))));
	}
	// EXACT(text1, text2): PopArgs → [text2, text1]
	tVariant wA;
	tVariant wB;
	if (!StackElemToVariant(wArgs[1], wA) || !StackElemToVariant(wArgs[0], wB)) {
		return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
	}
	if (wA.IsError()) return(tStackElem(wA));
	if (wB.IsError()) return(tStackElem(wB));
	tString wSa;
	tString wSb;
	if (!ExactToText(wA, wSa) || !ExactToText(wB, wSb)) {
		return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
	}
	return(tStackElem(tVariant(wSa == wSb)));
}

tFunctionFind::tFunctionFind() : tFunction() {}

tStackElem tFunctionFind::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 2) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    // FIND(find_text, within_text) - 2 arguments
    // In RPN: find_text within_text FIND, so after PopArgs: wArgs[0]=within_text, wArgs[1]=find_text
    tStackElem* wArgFind = &wArgs[1]; // find_text (first arg, last popped)
    tStackElem* wArgText = &wArgs[0]; // within_text (second arg, first popped)
    
    tVariant wFind;
    tVariant wText;
    if (!StackElemToVariant(*wArgFind, wFind)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "FIND: invalid find_text argument"))));
    }
    if (wFind.IsError()) return(tStackElem(tVariant(wFind)));
    if (!StackElemToVariant(*wArgText, wText)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "FIND: invalid within_text argument"))));
    }
    if (wText.IsError()) return(tStackElem(tVariant(wText)));
    if ((wText.Type() != tVariantType::t_string) || (wFind.Type() != tVariantType::t_string)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "FIND: both arguments must be strings"))));
    }
#ifdef debugtext
    cout << "Find(" << wText.String()  << ","  << wFind.String() <<")" << endl;
#endif
    tClassString wClassString(wText.String());
    tSize wResult=wClassString.Find(wFind.String())+1; // Ecxel Indice on 1 
    // Clean up
    return(tStackElem(tVariant(tInt(wResult))));
}

tFunctionSearch::tFunctionSearch() : tFunction() {}

tStackElem tFunctionSearch::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 2) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    // SEARCH(find_text, within_text) - 2 arguments
    // In RPN: find_text within_text SEARCH, so after PopArgs: wArgs[0]=within_text, wArgs[1]=find_text
    tStackElem* wArgFind = &wArgs[1]; // find_text (first arg, last popped)
    tStackElem* wArgText = &wArgs[0]; // within_text (second arg, first popped)
    
    tVariant wFind;
    tVariant wText;
    if (!StackElemToVariant(*wArgFind, wFind)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SEARCH: invalid find_text argument"))));
    }
    if (wFind.IsError()) return(tStackElem(tVariant(wFind)));
    if (!StackElemToVariant(*wArgText, wText)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SEARCH: invalid within_text argument"))));
    }
    if (wText.IsError()) return(tStackElem(tVariant(wText)));
    if ((wText.Type() != tVariantType::t_string) || (wFind.Type() != tVariantType::t_string)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
#ifdef debugtext
    cout << "Search(" << wText.String()  << ","  << wFind.String() <<")" << endl;
#endif
    tClassString wClassStringText(wText.String());
    tClassString wClassStringFind(wFind.String());
                                  
    // Pass to Upper case for case insensitive search
    tString wTextUpper=wClassStringText.Upper();
    tString wFindUpper=wClassStringFind.Upper();
    tClassString wClassStringSearch(wTextUpper);
    tSize wResult=wClassStringSearch.Find(wFindUpper)+1; // Ecxel Indice on 1
    // Clean up
    return(tStackElem(tVariant(tInt(wResult))));
}

tFunctionReplace::tFunctionReplace() : tFunction() {}

tStackElem tFunctionReplace::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 4) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    // REPLACE(old_text, start_num, num_chars, new_text) - 4 arguments
    // In RPN: old_text start_num num_chars new_text REPLACE, so after PopArgs: wArgs[0]=new_text, wArgs[1]=num_chars, wArgs[2]=start_num, wArgs[3]=old_text
    tStackElem* wArgOldText = &wArgs[3]; // old_text (first arg, last popped)
    tStackElem* wArgStartNum = &wArgs[2]; // start_num (second arg)
    tStackElem* wArgNumChars = &wArgs[1]; // num_chars (third arg)
    tStackElem* wArgNewText = &wArgs[0]; // new_text (fourth arg, first popped)
    
    tVariant wText;
    tVariant wPos;
    tVariant wLen;
    tVariant wReplace;
    if (!StackElemToVariant(*wArgOldText, wText)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "REPLACE: invalid old_text argument"))));
    }
    if (wText.IsError()) return(tStackElem(tVariant(wText)));
    if (!StackElemToVariant(*wArgStartNum, wPos)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "REPLACE: invalid start_num argument"))));
    }
    if (wPos.IsError()) return(tStackElem(tVariant(wPos)));
    if (!StackElemToVariant(*wArgNumChars, wLen)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "REPLACE: invalid num_chars argument"))));
    }
    if (wLen.IsError()) return(tStackElem(tVariant(wLen)));
    if (!StackElemToVariant(*wArgNewText, wReplace)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "REPLACE: invalid new_text argument"))));
    }
    if (wReplace.IsError()) return(tStackElem(tVariant(wReplace)));
    if ((wText.Type() != tVariantType::t_string) || (wPos.Type() != tVariantType::t_int) || (wLen.Type() != tVariantType::t_int) || (wReplace.Type() != tVariantType::t_string)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
#ifdef debugtext
    cout << "Replace(" << wText.String()  << ","  << wPos.Int() << ","  << wLen.Int() << ","  << wReplace.String() <<")" << endl;
#endif
   // =REPLACE(A1, 3, 4, "Test")
    tClassString wClassString(wText.String());
    // Use direct string manipulation for position-based replacement
    tString wResult = wText.String();
    wResult.replace(wPos.Int()-1, wLen.Int(), wReplace.String());
    // Clean up
    return(tStackElem(tVariant(wResult)));
}

tFunctionSubstitute::tFunctionSubstitute() : tFunction() {}

tStackElem tFunctionSubstitute::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 3) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    
    // SUBSTITUTE(text, old_text, new_text) - 3 arguments
    // In RPN: text old_text new_text SUBSTITUTE, so after PopArgs: wArgs[0]=new_text, wArgs[1]=old_text, wArgs[2]=text
    tStackElem* wArgText = &wArgs[2]; // text (first arg, last popped)
    tStackElem* wArgOldText = &wArgs[1]; // old_text (second arg)
    tStackElem* wArgNewText = &wArgs[0]; // new_text (third arg, first popped)
    
    tVariant wText;
    tVariant wFind;
    tVariant wReplace;
    if (!StackElemToVariant(*wArgText, wText)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SUBSTITUTE: invalid text argument"))));
    }
    if (wText.IsError()) return(tStackElem(tVariant(wText)));
    if (!StackElemToVariant(*wArgOldText, wFind)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SUBSTITUTE: invalid old_text argument"))));
    }
    if (wFind.IsError()) return(tStackElem(tVariant(wFind)));
    if (!StackElemToVariant(*wArgNewText, wReplace)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SUBSTITUTE: invalid new_text argument"))));
    }
    if (wReplace.IsError()) return(tStackElem(tVariant(wReplace)));
    if ((wText.Type() != tVariantType::t_string) || (wFind.Type() != tVariantType::t_string) || (wReplace.Type() != tVariantType::t_string)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "SUBSTITUTE: all arguments must be strings"))));
    }
#ifdef debugtext
    cout << "Substitute(" << wText.String()  << ","  << wFind.String() << ","  << wReplace.String() <<")" << endl;
#endif
    tClassString wClassString(wText.String());
    wClassString.Replace(wFind.String(), wReplace.String());
    // Clean up
    return(tStackElem(tVariant(wClassString())));
}
tFunctionUpper::tFunctionUpper() : tFunction() {}

tStackElem tFunctionUpper::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 1) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "UPPER requires exactly 1 argument"))));
    }
    // UPPER(text) - 1 argument; In RPN: wArgs[0]=text
    tStackElem* wArg = &wArgs[0];
    tVariant wText;
    if (!StackElemToVariant(*wArg, wText)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "UPPER: invalid argument"))));
    }
    if (wText.IsError()) return(tStackElem(tVariant(wText)));
    if (wText.Type() != tVariantType::t_string) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "UPPER: argument must be a string"))));
    }
    tClassString wClassString(wText.String());
    tString wResult=wClassString.Upper();
    // Clean up
    return(tStackElem(tVariant(wResult)));
}

tFunctionLower::tFunctionLower() : tFunction() {}

    

tStackElem tFunctionLower::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 1) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "LOWER requires exactly 1 argument"))));
    }
    // LOWER(text) - 1 argument; In RPN: wArgs[0]=text
    tStackElem* wArg = &wArgs[0];
    tVariant wText;
    if (!StackElemToVariant(*wArg, wText)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "LOWER: invalid argument"))));
    }
    if (wText.IsError()) return(tStackElem(tVariant(wText)));
    if (wText.Type() != tVariantType::t_string) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "LOWER: argument must be a string"))));
    }
    tClassString wClassString(wText.String());
    tString wResult=wClassString.Lower();
    // Clean up
    return(tStackElem(tVariant(wResult)));   
}

tFunctionProper::tFunctionProper() : tFunction() {}

tStackElem tFunctionProper::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 1) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "PROPER requires exactly 1 argument"))));
    }
    // PROPER(text) - 1 argument; In RPN: wArgs[0]=text
    tStackElem* wArg = &wArgs[0];
    tVariant wText;
    if (!StackElemToVariant(*wArg, wText)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "PROPER: invalid argument"))));
    }
    if (wText.IsError()) return(tStackElem(tVariant(wText)));
    if (wText.Type() != tVariantType::t_string) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "PROPER: argument must be a string"))));
    }
    tClassString wClassString(wText.String());
    tString wResult=wClassString.Proper();
    // Clean up
    return(tStackElem(tVariant(wResult)));
}

tFunctionTrim::tFunctionTrim() : tFunction() {}

tStackElem tFunctionTrim::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 1) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TRIM requires exactly 1 argument"))));
    }
    // TRIM(text) - 1 argument; In RPN: wArgs[0]=text
    tStackElem* wArg = &wArgs[0];
    tVariant wText;
    if (!StackElemToVariant(*wArg, wText)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TRIM: invalid argument"))));
    }
    if (wText.IsError()) return(tStackElem(tVariant(wText)));
    if (wText.Type() != tVariantType::t_string) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TRIM: argument must be a string"))));
    }
    tClassString wClassString(wText.String());
    tString wResult=wClassString.Trim();
    // Clean up
    return(tStackElem(tVariant(wResult)));
}

tFunctionRept::tFunctionRept() : tFunction() {}

tStackElem tFunctionRept::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 2) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "REPT requires 2 arguments"))));
    }
    // REPT(text, number_times) - 2 arguments
    // In RPN: text number_times REPT, so after PopArgs: wArgs[0]=number_times, wArgs[1]=text
    tVariant wTextValue;
    if (!StackElemToVariant(wArgs[1], wTextValue)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "REPT: invalid text argument"))));
    }
    if (wTextValue.IsError()) return(tStackElem(tVariant(wTextValue)));

    tInt wRepeatCount = 0;
    if (!StackElemToInt(wArgs[0], wRepeatCount)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "REPT: invalid number_times argument"))));
    }
    if (wRepeatCount < 0) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "REPT: number_times must be >= 0"))));
    }

    // Excel-like coercion: non-string values are converted to text.
    tString wText;
    if (wTextValue.IsNull()) {
        wText = "";
    } else if (wTextValue.Type() == tVariantType::t_string) {
        wText = wTextValue.String();
    } else {
        tStringStream wStream;
        if (wTextValue.Type() == tVariantType::t_int) {
            wStream << wTextValue.Int();
        } else if (wTextValue.Type() == tVariantType::t_double) {
            wStream << wTextValue.Double();
        } else if (wTextValue.Type() == tVariantType::t_date) {
            wStream << tClassDate(wTextValue.Date()).UsDate();
        } else if (wTextValue.Type() == tVariantType::t_bool) {
            wStream << (wTextValue.Bool() ? "TRUE" : "FALSE");
        } else {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "REPT: unsupported text argument type"))));
        }
        wText = wStream.str();
    }

    tString wResult;
    if (!wText.empty() && wRepeatCount > 0) {
        wResult.reserve(static_cast<size_t>(wText.size()) * static_cast<size_t>(wRepeatCount));
    }
    for (tInt wI = 0; wI < wRepeatCount; ++wI) {
        wResult += wText;
    }
    return(tStackElem(tVariant(wResult)));
}

// Text
tFunctionText::tFunctionText() : tFunction() {}

tStackElem tFunctionText::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 2) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXT requires 2 arguments"))));
    }
    // TEXT(value, format_text) - 2 arguments
    // In RPN: value format_text TEXT, so after PopArgs: wArgs[0]=format_text, wArgs[1]=value
    tStackElem* wArgValue = &wArgs[1]; // value (first arg, last popped)
    tStackElem* wArgFormat = &wArgs[0]; // format_text (second arg, first popped)
    
    tVariant wFormatText;
    tVariant wValue;
    // Scalar resolution is in StackElemToVariant (this cell for t_Cell, including MatExtend).
    if (!StackElemToVariant(*wArgValue, wValue)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXT: invalid value argument"))));
    }
    if (wValue.IsError()) return(tStackElem(tVariant(wValue)));
    if (!StackElemToVariant(*wArgFormat, wFormatText)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXT: invalid format_text argument"))));
    }
    if (wFormatText.IsError()) return(tStackElem(tVariant(wFormatText)));
    if (wFormatText.Type() != tVariantType::t_string) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXT format_text must be a string"))));
    }

    // Excel: blank → ""; TRUE/FALSE coerce to 1/0 (serial) so date formats like "jjj" work.
    if (wValue.IsNull()) {
        return(tStackElem(tVariant(tString(""))));
    }
    if (wValue.Type() == tVariantType::t_bool) {
        wValue = tVariant(wValue.Bool() ? 1 : 0);
    } else if (wValue.Type() == tVariantType::t_class) {
        const tVariant* wInner = (wValue.Class() != nullptr) ? wValue.Class()->Value() : nullptr;
        if (wInner != nullptr) {
            wValue = tCell::CalculableScalarFromVariant(*wInner);
        }
        if (wValue.IsNull()) {
            return(tStackElem(tVariant(tString(""))));
        }
        if (wValue.Type() == tVariantType::t_bool) {
            wValue = tVariant(wValue.Bool() ? 1 : 0);
        }
    }
    
    // Convert value to string; for dates, apply format_text (Excel-style codes supported for date)
    tString wResult;
    tString wFormatStr = wFormatText.String();
    if (wValue.Type() == tVariantType::t_int || wValue.Type() == tVariantType::t_double) {
        // When format is Excel date (e.g. "mmm", "mmmm", "ddd"/"jjj"), treat int/double as Excel serial date
        // and format with locale month/day names (SkLocale via FormatWithExcelDateFormat).
        if (SkRoot::IsValidExcelDateFormat(wFormatStr)) {
            const double wSerial = wValue.Numeric();
            if (wSerial >= 1.0) {
                // Same serial → calendar conversion as tClassDate / t_date cells (local date), not gmtime UTC.
                // UTC decoding shifted the civil date in non-UTC zones and broke weekday text (e.g. calendar header row).
                tClassDate wDate(wValue);
                tInt wYear = 0;
                tInt wMonth = 0;
                tInt wDay = 0;
                wDate.YearMonthDay(wYear, wMonth, wDay);
                wResult = SkRoot::FormatWithExcelDateFormat(wYear, wMonth, wDay, wFormatStr);
            }
        }
        if (wResult.empty()) {
            if (SkRoot::IsValidExcelNumberFormat(wFormatStr)) {
                wResult = SkRoot::FormatWithExcelNumberFormat(wValue.Numeric(), wFormatStr);
            } else {
                tStringStream wStream;
                if (wValue.Type() == tVariantType::t_int) {
                    wStream << wValue.Int();
                } else {
                    wStream << wValue.Double();
                }
                wResult = wStream.str();
            }
        }
    } else if (wValue.Type() == tVariantType::t_string) {
        const auto wSecs = SkRoot::SplitExcelFormatSections(wFormatStr);
        if (wSecs.size() >= 4) {
            wResult = SkRoot::FormatExcelTextSection(wSecs, wValue.String());
        } else {
            wResult = wValue.String();
        }
    } else if (wValue.Type() == tVariantType::t_date) {
        // Date: use Excel date format parser (supports "mmm", "mmmm", "dd/mm/yyyy", etc.)
        tClassDate wDate(wValue.Date());
        tInt wYear = 0, wMonth = 0, wDay = 0;
        wDate.YearMonthDay(wYear, wMonth, wDay);
        wResult = SkRoot::FormatWithExcelDateFormat(wYear, wMonth, wDay, wFormatStr);
        if (wResult.empty()) {
            wResult = wDate.UsDate(false);
        }
    } else {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXT value must be numeric, string or date"))));
    }
    
    // Clean up
    return(tStackElem(tVariant(wResult)));
}

// Value
tFunctionValue::tFunctionValue() : tFunction() {}

tStackElem tFunctionValue::Call(tStackElems* sStackElems, tShort sNbArg) {
        std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 1) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "VALUE requires 1 argument"))));
    }
    // VALUE(text) - 1 argument; In RPN: wArgs[0]=text
    tStackElem* wArg = &wArgs[0];
    tVariant wText;
    if (!StackElemToVariant(*wArg, wText)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "VALUE: invalid argument"))));
    }
    if (wText.IsError()) return(tStackElem(tVariant(wText)));
    if (wText.Type() != tVariantType::t_string) {
        // If already a number, return as-is
        if (wText.IsInt() || wText.IsDouble()) {
            return(wText);
        }
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "VALUE argument must be a string"))));
    }
    
    // Convert string to number
    tString wStr = wText.String();
    // Remove leading/trailing whitespace
    tClassString wClassString(wStr);
    wStr = wClassString.Trim();
    
    if (wStr.empty()) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "VALUE: empty string"))));
    }
    
    // Try to parse as integer first
    tStringStream wStreamInt(wStr);
    tInt wInt;
    if (wStreamInt >> wInt) {
        // Check if we consumed the entire string
        tChar wPeek = wStreamInt.peek();
        if (wStreamInt.eof() || wPeek == EOF || wPeek == '\0') {
            return(tStackElem(tVariant(wInt)));
        }
    }
    
    // Try to parse as double
    tStringStream wStreamDouble(wStr);
    tDouble wDouble;
    if (wStreamDouble >> wDouble) {
        // Check if we consumed the entire string
        tChar wPeek = wStreamDouble.peek();
        if (wStreamDouble.eof() || wPeek == EOF || wPeek == '\0') {
            return(tStackElem(tVariant(wDouble)));
        }
    }
    
    // If parsing fails, return error
    return(tStackElem(tVariant(tClassError(tTypeError::t_value, "VALUE: cannot convert to number"))));
}

// Shared helper: coerce a scalar value to its Excel text representation.
// Returns false only for errors (caller should propagate). Null becomes "".
static tBool VariantToText(const tVariant& sValue, tString& sOut) {
    if (sValue.IsError()) return(false);
    if (sValue.IsNull()) { sOut.clear(); return(true); }
    switch (sValue.Type()) {
        case tVariantType::t_string:
            sOut = sValue.String();
            return(true);
        case tVariantType::t_int: {
            tStringStream wStream; wStream << sValue.Int(); sOut = wStream.str(); return(true);
        }
        case tVariantType::t_double: {
            tStringStream wStream; wStream << sValue.Double(); sOut = wStream.str(); return(true);
        }
        case tVariantType::t_bool:
            sOut = (sValue.Bool() ? "TRUE" : "FALSE");
            return(true);
        case tVariantType::t_date:
            sOut = tClassDate(sValue.Date()).UsDate();
            return(true);
        default:
            sOut.clear();
            return(true);
    }
}

static tString QuoteStrictText(const tString& sText) {
    tString wOut = "\"";
    for (char ch : sText) {
        if (ch == '"') wOut += "\"\"";
        else wOut += ch;
    }
    wOut += '"';
    return(wOut);
}

static tString VariantToTextStrict(const tVariant& sValue) {
    if (sValue.IsString()) {
        return(QuoteStrictText(sValue.String()));
    }
    tString wPlain;
    VariantToText(sValue, wPlain);
    return(wPlain);
}

// Round like Excel ROUND (half away from zero), then format with NumberFormatter.
static tStackElem FormatFixedLike(tDouble sNumber, tInt sDecimals, tBool sNoCommas, tBool sDollar) {
    tDouble wNumber = sNumber;
    tInt wDecimals = sDecimals;
    if (wDecimals < 0) {
        const tDouble wFactor = std::pow(10.0, static_cast<tDouble>(-wDecimals));
        wNumber = std::round(wNumber / wFactor) * wFactor;
        wDecimals = 0;
    } else {
        const tDouble wFactor = std::pow(10.0, static_cast<tDouble>(wDecimals));
        wNumber = std::round(wNumber * wFactor) / wFactor;
    }
    tString wFmt;
    if (sDollar) {
        wFmt = sNoCommas ? "$0" : "$#,##0";
    } else {
        wFmt = sNoCommas ? "0" : "#,##0";
    }
    if (wDecimals > 0) {
        wFmt += '.';
        wFmt.append(static_cast<size_t>(wDecimals), '0');
    }
    if (sDollar) {
        // Excel DOLLAR: negatives in parentheses.
        wFmt += ";(" + wFmt + ")";
    }
    tNumberFormatter* wFmtNum = tApplication::Instance()->NumberFormatter();
    if (wFmtNum == nullptr) {
        tStringStream wStream;
        wStream.setf(std::ios::fixed);
        wStream.precision(wDecimals);
        wStream << wNumber;
        return(tStackElem(tVariant(wStream.str())));
    }
    return(tStackElem(tVariant(wFmtNum->FormatNumber(wNumber, wFmt))));
}

// T ========================================================================
tFunctionT::tFunctionT() : tFunction() {}

tStackElem tFunctionT::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 1) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "T requires 1 argument"))));
    }
    tVariant wValue;
    if (!StackElemToVariant(wArgs[0], wValue)) {
        return(tStackElem(tVariant(tString(""))));
    }
    if (wValue.IsError()) return(tStackElem(wValue));
    if (wValue.IsString()) {
        return(tStackElem(tVariant(wValue.String())));
    }
    return(tStackElem(tVariant(tString(""))));
}

// FIXED ====================================================================
tFunctionFixed::tFunctionFixed() : tFunction() {}

tStackElem tFunctionFixed::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.empty() || wArgs.size() > 3) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "FIXED requires 1 to 3 arguments"))));
    }
    std::reverse(wArgs.begin(), wArgs.end());
    tVariant wNumVar;
    if (!StackElemToVariant(wArgs[0], wNumVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    if (wNumVar.IsError()) return(tStackElem(wNumVar));
    if (!wNumVar.IsNumeric()) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    tInt wDecimals = 2;
    if (wArgs.size() >= 2) {
        if (!StackElemToInt(wArgs[1], wDecimals)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
    }
    tBool wNoCommas = false;
    if (wArgs.size() >= 3) {
        if (!StackElemToBool(wArgs[2], wNoCommas)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
    }
    return FormatFixedLike(wNumVar.Numeric(), wDecimals, wNoCommas, false);
}

// DOLLAR ===================================================================
tFunctionDollar::tFunctionDollar() : tFunction() {}

tStackElem tFunctionDollar::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.empty() || wArgs.size() > 2) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "DOLLAR requires 1 or 2 arguments"))));
    }
    std::reverse(wArgs.begin(), wArgs.end());
    tVariant wNumVar;
    if (!StackElemToVariant(wArgs[0], wNumVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    if (wNumVar.IsError()) return(tStackElem(wNumVar));
    if (!wNumVar.IsNumeric()) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    tInt wDecimals = 2;
    if (wArgs.size() >= 2) {
        if (!StackElemToInt(wArgs[1], wDecimals)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
    }
    return FormatFixedLike(wNumVar.Numeric(), wDecimals, false, true);
}

// VALUETOTEXT ==============================================================
tFunctionValueToText::tFunctionValueToText() : tFunction() {}

tStackElem tFunctionValueToText::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.empty() || wArgs.size() > 2) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "VALUETOTEXT requires 1 or 2 arguments"))));
    }
    std::reverse(wArgs.begin(), wArgs.end());
    tInt wFormat = 0;
    if (wArgs.size() >= 2) {
        if (!StackElemToInt(wArgs[1], wFormat) || (wFormat != 0 && wFormat != 1)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
    }
    tVariant wValue;
    if (wArgs[0].Type() == tStackType::t_Array) {
        // Implicit intersection: top-left.
        wValue = wArgs[0].Value();
    } else if (!StackElemToVariant(wArgs[0], wValue)) {
        return(tStackElem(tVariant(tString(""))));
    }
    if (wValue.IsError()) return(tStackElem(wValue));
    if (wFormat == 1) {
        return(tStackElem(tVariant(VariantToTextStrict(wValue))));
    }
    tString wOut;
    if (!VariantToText(wValue, wOut)) {
        return(tStackElem(wValue));
    }
    return(tStackElem(tVariant(wOut)));
}

// ARRAYTOTEXT ==============================================================
tFunctionArrayToText::tFunctionArrayToText() : tFunction() {}

tStackElem tFunctionArrayToText::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.empty() || wArgs.size() > 2) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "ARRAYTOTEXT requires 1 or 2 arguments"))));
    }
    std::reverse(wArgs.begin(), wArgs.end());
    tInt wFormat = 0;
    if (wArgs.size() >= 2) {
        if (!StackElemToInt(wArgs[1], wFormat) || (wFormat != 0 && wFormat != 1)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
    }
    tArrayValue wArr;
    if (!StackElemToArray(wArgs[0], wArr) || wArr.Count() <= 0) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    const tBool wStrict = (wFormat == 1);
    tString wOut;
    if (wStrict) wOut += '{';
    for (tIndex r = 0; r < wArr.m_Rows; ++r) {
        if (r > 0) wOut += wStrict ? ";" : "; ";
        for (tIndex c = 0; c < wArr.m_Cols; ++c) {
            if (c > 0) wOut += wStrict ? "," : ", ";
            const tVariant& wV = wArr.At(r, c);
            if (wV.IsError()) return(tStackElem(wV));
            if (wStrict) {
                wOut += VariantToTextStrict(wV);
            } else {
                tString wCell;
                if (!VariantToText(wV, wCell)) return(tStackElem(wV));
                wOut += wCell;
            }
        }
    }
    if (wStrict) wOut += '}';
    return(tStackElem(tVariant(wOut)));
}

// TextAfter / TextBefore ====================================================
tFunctionTextAfterBefore::tFunctionTextAfterBefore(tBool sAfter) : tFunction(), m_After(sAfter) {}

tStackElem tFunctionTextAfterBefore::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    // TEXTAFTER/TEXTBEFORE(text, delimiter, [instance_num])
    // In RPN the args are pushed left-to-right, so after PopArgs they are reversed:
    //   size 2 -> wArgs[0]=delimiter, wArgs[1]=text
    //   size 3 -> wArgs[0]=instance_num, wArgs[1]=delimiter, wArgs[2]=text
    if ((wArgs.size() != 2) && (wArgs.size() != 3)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    const tSize wLast = wArgs.size() - 1;
    tVariant wTextVar;
    tVariant wDelimVar;
    if (!StackElemToVariant(wArgs[wLast], wTextVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXTAFTER: invalid text argument"))));
    }
    if (wTextVar.IsError()) return(tStackElem(tVariant(wTextVar)));
    if (!StackElemToVariant(wArgs[wLast - 1], wDelimVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXTAFTER: invalid delimiter argument"))));
    }
    if (wDelimVar.IsError()) return(tStackElem(tVariant(wDelimVar)));

    tInt wInstance = 1;
    if (wArgs.size() == 3) {
        tVariant wInstVar;
        if (!StackElemToVariant(wArgs[0], wInstVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXTAFTER: invalid instance_num argument"))));
        }
        if (wInstVar.IsError()) return(tStackElem(tVariant(wInstVar)));
        if (!StackElemToInt(wArgs[0], wInstance)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXTAFTER: instance_num must be numeric"))));
        }
    }
    if (wInstance == 0) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "TEXTAFTER: instance_num must not be 0"))));
    }

    tString wText;
    tString wDelim;
    VariantToText(wTextVar, wText);
    VariantToText(wDelimVar, wDelim);

    // Empty delimiter: Excel returns the whole text (TEXTAFTER) or empty (TEXTBEFORE).
    if (wDelim.empty()) {
        return(tStackElem(tVariant(m_After ? wText : tString(""))));
    }

    // Locate the requested occurrence (positive = from start, negative = from end).
    tSize wPos = tString::npos;
    if (wInstance > 0) {
        tSize wFrom = 0;
        for (tInt wI = 0; wI < wInstance; ++wI) {
            wPos = wText.find(wDelim, wFrom);
            if (wPos == tString::npos) break;
            wFrom = wPos + wDelim.size();
        }
    } else {
        tSize wSearchEnd = wText.size();
        tInt wCount = -wInstance;
        for (tInt wI = 0; wI < wCount; ++wI) {
            if (wSearchEnd == 0) { wPos = tString::npos; break; }
            wPos = wText.rfind(wDelim, wSearchEnd - 1);
            if (wPos == tString::npos) break;
            wSearchEnd = wPos;
        }
    }

    if (wPos == tString::npos) {
        // Delimiter not found: Excel returns #N/A when if_not_found is omitted.
        return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
    }
    if (m_After) {
        tSize wStart = wPos + wDelim.size();
        return(tStackElem(tVariant(wText.substr(wStart))));
    }
    return(tStackElem(tVariant(wText.substr(0, wPos))));
}

// TextJoin ==================================================================
tFunctionTextJoin::tFunctionTextJoin() : tFunction() {}

tStackElem tFunctionTextJoin::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    // TEXTJOIN(delimiter, ignore_empty, text1, [text2], ...)
    if (wArgs.size() < 3) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXTJOIN requires at least 3 arguments"))));
    }
    // In RPN, args are pushed left-to-right; PopArgs returns them reversed
    // (wArgs[0] is the last text, wArgs[size-1] is the delimiter).
    tVariant wDelimVar;
    if (!StackElemToVariant(wArgs[wArgs.size() - 1], wDelimVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXTJOIN: invalid delimiter argument"))));
    }
    if (wDelimVar.IsError()) return(tStackElem(tVariant(wDelimVar)));
    tString wDelim;
    VariantToText(wDelimVar, wDelim);

    tBool wIgnoreEmpty = true;
    if (!StackElemToBool(wArgs[wArgs.size() - 2], wIgnoreEmpty)) {
        wIgnoreEmpty = true;
    }

    tString wResult;
    tBool wFirst = true;
    // Emit one text piece, honoring ignore_empty and the delimiter placement.
    auto wAppend = [&](const tVariant& sValue) -> tBool {
        if (sValue.IsError()) return(false);
        tString wPiece;
        VariantToText(sValue, wPiece);
        if (wIgnoreEmpty && wPiece.empty()) return(true);
        if (!wFirst) wResult += wDelim;
        wResult += wPiece;
        wFirst = false;
        return(true);
    };

    // Process text arguments in source order: wArgs[size-3] .. wArgs[0].
    for (tInt wIdx = static_cast<tInt>(wArgs.size()) - 3; wIdx >= 0; --wIdx) {
        tStackElem& wArg = wArgs[static_cast<tSize>(wIdx)];
        if (wArg.Type() == tStackType::t_Range) {
            tRange* wRange = wArg.Range();
            if (wRange != nullptr) {
                tColRowCellRange* wColRowCellRange = wRange->ColRowCellRange();
                if (wColRowCellRange != nullptr) {
                    for (tIndex wRow = wRange->TopIndex(); wRow <= wRange->IterateBottom(); wRow++) {
                        for (tIndex wCol = wRange->LeftIndex(); wCol <= wRange->IterateRight(); wCol++) {
                            tCell* wCell = wColRowCellRange->Cell(wRow, wCol);
                            tVariant wCellValue = (wCell != nullptr) ? wCell->Value() : tVariant();
                            if (wCellValue.IsError()) return(tStackElem(tVariant(wCellValue)));
                            if (!wAppend(wCellValue)) return(tStackElem(tVariant(wCellValue)));
                        }
                    }
                }
            }
        } else if (wArg.Type() == tStackType::t_Array) {
            tArrayValue* wArray = wArg.Array();
            if (wArray != nullptr) {
                for (tIndex wRow = 0; wRow < wArray->m_Rows; ++wRow) {
                    for (tIndex wCol = 0; wCol < wArray->m_Cols; ++wCol) {
                        const tVariant& wValue = wArray->At(wRow, wCol);
                        if (wValue.IsError()) return(tStackElem(tVariant(wValue)));
                        if (!wAppend(wValue)) return(tStackElem(tVariant(wValue)));
                    }
                }
            }
        } else {
            tVariant wValue;
            if (StackElemToVariant(wArg, wValue)) {
                if (wValue.IsError()) return(tStackElem(tVariant(wValue)));
                if (!wAppend(wValue)) return(tStackElem(tVariant(wValue)));
            }
        }
    }
    return(tStackElem(tVariant(wResult)));
}

// Char
// CLEAN ======================================================================
tFunctionClean::tFunctionClean() : tFunction() {}

tStackElem tFunctionClean::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 1) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    tVariant wTextVar;
    if (!StackElemToVariant(wArgs[0], wTextVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    if (wTextVar.IsError()) {
        return(tStackElem(wTextVar));
    }
    tString wText;
    if (!VariantToText(wTextVar, wText)) {
        return(tStackElem(wTextVar));
    }
    // Excel CLEAN: strip ASCII control codes 0..31 (bytes).
    tString wOut;
    wOut.reserve(wText.size());
    for (unsigned char wCh : wText) {
        if (wCh >= 32) {
            wOut.push_back(static_cast<tChar>(wCh));
        }
    }
    return(tStackElem(tVariant(wOut)));
}

// NUMBERVALUE ================================================================
tFunctionNumberValue::tFunctionNumberValue() : tFunction() {}

tStackElem tFunctionNumberValue::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    // NUMBERVALUE(text, [decimal_separator], [group_separator])
    if (wArgs.size() < 1 || wArgs.size() > 3) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    std::reverse(wArgs.begin(), wArgs.end());

    tVariant wTextVar;
    if (!StackElemToVariant(wArgs[0], wTextVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    if (wTextVar.IsError()) {
        return(tStackElem(wTextVar));
    }
    tString wText;
    if (!VariantToText(wTextVar, wText)) {
        return(tStackElem(wTextVar));
    }
    // Empty / blank → 0 (Excel).
    {
        tClassString wTrim(wText);
        wText = wTrim.Trim();
    }
    if (wText.empty()) {
        return(tStackElem(tVariant(0)));
    }

    tLocale* wLocale = tApplication::Instance()->Locale();
    tChar wDec = (wLocale != nullptr) ? wLocale->Decimal() : '.';
    tChar wGroup = (wLocale != nullptr) ? wLocale->Thousand() : ',';

    auto wFirstSepChar = [](const tVariant& sVar, tChar& oCh) -> tBool {
        // Omitted args become Integer 0 via PushEmptyFunctionArg — keep locale default.
        if (sVar.IsNumeric() && sVar.Numeric() == 0.0) {
            return(false);
        }
        tString wSep;
        if (!VariantToText(sVar, wSep) || wSep.empty()) {
            return(false);
        }
        oCh = wSep[0];
        return(true);
    };

    if (wArgs.size() >= 2) {
        tVariant wDecVar;
        if (!StackElemToVariant(wArgs[1], wDecVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wDecVar.IsError()) {
            return(tStackElem(wDecVar));
        }
        (void)wFirstSepChar(wDecVar, wDec);
    }
    if (wArgs.size() >= 3) {
        tVariant wGroupVar;
        if (!StackElemToVariant(wArgs[2], wGroupVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wGroupVar.IsError()) {
            return(tStackElem(wGroupVar));
        }
        (void)wFirstSepChar(wGroupVar, wGroup);
    }

    // Strip spaces; count trailing % (each divides by 100).
    tString wNorm;
    wNorm.reserve(wText.size());
    tInt wPct = 0;
    for (tChar wCh : wText) {
        if (wCh == ' ' || wCh == '\t') {
            continue;
        }
        if (wCh == '%') {
            ++wPct;
            continue;
        }
        wNorm.push_back(wCh);
    }

    // Remove group separators before the decimal; reject group after decimal / multi decimal.
    tString wDigits;
    wDigits.reserve(wNorm.size());
    tBool wSawDec = false;
    for (tChar wCh : wNorm) {
        if (wCh == wDec) {
            if (wSawDec) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            wSawDec = true;
            wDigits.push_back('.');
            continue;
        }
        if (wCh == wGroup) {
            if (wSawDec) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
            }
            continue; // drop thousands separators before decimal
        }
        wDigits.push_back(wCh);
    }

    if (wDigits.empty() || wDigits == "+" || wDigits == "-" || wDigits == ".") {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }

    tChar* wEnd = nullptr;
    const tDouble wVal = std::strtod(wDigits.c_str(), &wEnd);
    if (wEnd == nullptr || wEnd == wDigits.c_str() || *wEnd != '\0' || !std::isfinite(wVal)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    tDouble wOut = wVal;
    for (tInt i = 0; i < wPct; ++i) {
        wOut /= 100.0;
    }
    if (std::isfinite(wOut) && wOut == std::floor(wOut) && std::fabs(wOut) < 9.0e15) {
        return(tStackElem(tVariant(static_cast<tInt>(wOut))));
    }
    return(tStackElem(tVariant(wOut)));
}

// CHAR / CODE ================================================================
tFunctionCharCode::tFunctionCharCode(tBool sToChar) : tFunction(), m_ToChar(sToChar) {}

tStackElem tFunctionCharCode::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 1) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    if (m_ToChar) {
        tInt wCode = 0;
        if (!StackElemToInt(wArgs[0], wCode)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wCode < 1 || wCode > 255) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        tVariant wResult;
        wResult.SetString(tString(1, static_cast<tChar>(wCode)));
        return(tStackElem(wResult));
    }

    // CODE(text): numeric code of the first byte (inverse of CHAR for 1..255).
    tVariant wTextVar;
    if (!StackElemToVariant(wArgs[0], wTextVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    if (wTextVar.IsError()) {
        return(tStackElem(wTextVar));
    }
    tString wText;
    if (!VariantToText(wTextVar, wText) || wText.empty()) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    return(tStackElem(tVariant(static_cast<tInt>(static_cast<unsigned char>(wText[0])))));
}

// UNICHAR / UNICODE ==========================================================
namespace {
    static tString EncodeUtf8CodePoint(uint32_t sCp) {
        tString wOut;
        if (sCp < 0x80u) {
            wOut.push_back(static_cast<tChar>(sCp));
        } else if (sCp < 0x800u) {
            wOut.push_back(static_cast<tChar>(0xC0u | ((sCp >> 6) & 0x1Fu)));
            wOut.push_back(static_cast<tChar>(0x80u | (sCp & 0x3Fu)));
        } else if (sCp < 0x10000u) {
            wOut.push_back(static_cast<tChar>(0xE0u | ((sCp >> 12) & 0x0Fu)));
            wOut.push_back(static_cast<tChar>(0x80u | ((sCp >> 6) & 0x3Fu)));
            wOut.push_back(static_cast<tChar>(0x80u | (sCp & 0x3Fu)));
        } else {
            wOut.push_back(static_cast<tChar>(0xF0u | ((sCp >> 18) & 0x07u)));
            wOut.push_back(static_cast<tChar>(0x80u | ((sCp >> 12) & 0x3Fu)));
            wOut.push_back(static_cast<tChar>(0x80u | ((sCp >> 6) & 0x3Fu)));
            wOut.push_back(static_cast<tChar>(0x80u | (sCp & 0x3Fu)));
        }
        return(wOut);
    }

    static tBool DecodeFirstUtf8CodePoint(const tString& sText, uint32_t& oCp) {
        if (sText.empty()) {
            return(false);
        }
        const unsigned char w0 = static_cast<unsigned char>(sText[0]);
        if ((w0 & 0x80u) == 0x00u) {
            oCp = w0;
            return(true);
        }
        if ((w0 & 0xE0u) == 0xC0u) {
            if (sText.size() < 2) return(false);
            const unsigned char w1 = static_cast<unsigned char>(sText[1]);
            if ((w1 & 0xC0u) != 0x80u) return(false);
            oCp = (static_cast<uint32_t>(w0 & 0x1Fu) << 6) | static_cast<uint32_t>(w1 & 0x3Fu);
            return(true);
        }
        if ((w0 & 0xF0u) == 0xE0u) {
            if (sText.size() < 3) return(false);
            const unsigned char w1 = static_cast<unsigned char>(sText[1]);
            const unsigned char w2 = static_cast<unsigned char>(sText[2]);
            if ((w1 & 0xC0u) != 0x80u || (w2 & 0xC0u) != 0x80u) return(false);
            oCp = (static_cast<uint32_t>(w0 & 0x0Fu) << 12) |
                  (static_cast<uint32_t>(w1 & 0x3Fu) << 6) |
                  static_cast<uint32_t>(w2 & 0x3Fu);
            return(true);
        }
        if ((w0 & 0xF8u) == 0xF0u) {
            if (sText.size() < 4) return(false);
            const unsigned char w1 = static_cast<unsigned char>(sText[1]);
            const unsigned char w2 = static_cast<unsigned char>(sText[2]);
            const unsigned char w3 = static_cast<unsigned char>(sText[3]);
            if ((w1 & 0xC0u) != 0x80u || (w2 & 0xC0u) != 0x80u || (w3 & 0xC0u) != 0x80u) return(false);
            oCp = (static_cast<uint32_t>(w0 & 0x07u) << 18) |
                  (static_cast<uint32_t>(w1 & 0x3Fu) << 12) |
                  (static_cast<uint32_t>(w2 & 0x3Fu) << 6) |
                  static_cast<uint32_t>(w3 & 0x3Fu);
            return(true);
        }
        return(false);
    }
} // namespace

tFunctionUniCharCode::tFunctionUniCharCode(tBool sToChar) : tFunction(), m_ToChar(sToChar) {}

tStackElem tFunctionUniCharCode::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    if (wArgs.size() != 1) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, ""))));
    }
    if (m_ToChar) {
        tInt wCode = 0;
        if (!StackElemToInt(wArgs[0], wCode)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        if (wCode < 1 || wCode > 0x10FFFF) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        // Surrogate code points are invalid scalar values → #N/A.
        if (wCode >= 0xD800 && wCode <= 0xDFFF) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
        }
        return(tStackElem(tVariant(EncodeUtf8CodePoint(static_cast<uint32_t>(wCode)))));
    }

    tVariant wTextVar;
    if (!StackElemToVariant(wArgs[0], wTextVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    if (wTextVar.IsError()) {
        return(tStackElem(wTextVar));
    }
    tString wText;
    if (!VariantToText(wTextVar, wText) || wText.empty()) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    uint32_t wCp = 0;
    if (!DecodeFirstUtf8CodePoint(wText, wCp)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    return(tStackElem(tVariant(static_cast<tInt>(wCp))));
}

// TextSplit ==================================================================
namespace {
    static void TextSplitByDelim(const tString& sText, const tString& sDelim,
                                 tBool sIgnoreEmpty, std::vector<tString>& oOut) {
        oOut.clear();
        if (sDelim.empty()) {
            // Empty delimiter: Excel splits into individual characters.
            for (tSize i = 0; i < sText.size(); ++i) {
                tString wPiece(1, sText[i]);
                if (sIgnoreEmpty && wPiece.empty()) {
                    continue;
                }
                oOut.push_back(wPiece);
            }
            if (oOut.empty()) {
                oOut.push_back(tString());
            }
            return;
        }
        tSize wStart = 0;
        while (true) {
            const tSize wPos = sText.find(sDelim, wStart);
            if (wPos == tString::npos) {
                tString wPiece = sText.substr(wStart);
                if (!(sIgnoreEmpty && wPiece.empty())) {
                    oOut.push_back(wPiece);
                }
                break;
            }
            tString wPiece = sText.substr(wStart, wPos - wStart);
            if (!(sIgnoreEmpty && wPiece.empty())) {
                oOut.push_back(wPiece);
            }
            wStart = wPos + sDelim.size();
        }
        if (oOut.empty()) {
            oOut.push_back(tString());
        }
    }
} // namespace

tFunctionTextSplit::tFunctionTextSplit() : tFunction() {}

tStackElem tFunctionTextSplit::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    // TEXTSPLIT(text, col_delimiter, [row_delimiter], [ignore_empty], [match_mode], [pad_with])
    // MVP: first 4 args; match_mode ignored (exact); pad_with defaults to #N/A.
    if (wArgs.size() < 2 || wArgs.size() > 6) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXTSPLIT requires 2 to 6 arguments"))));
    }
    std::reverse(wArgs.begin(), wArgs.end());

    tVariant wTextVar;
    if (!StackElemToVariant(wArgs[0], wTextVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXTSPLIT: invalid text"))));
    }
    if (wTextVar.IsError()) {
        return(tStackElem(wTextVar));
    }
    tVariant wColDelimVar;
    if (!StackElemToVariant(wArgs[1], wColDelimVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXTSPLIT: invalid col_delimiter"))));
    }
    if (wColDelimVar.IsError()) {
        return(tStackElem(wColDelimVar));
    }

    tString wRowDelim;
    tBool wHasRowDelim = false;
    if (wArgs.size() >= 3) {
        tVariant wRowDelimVar;
        if (!StackElemToVariant(wArgs[2], wRowDelimVar)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "TEXTSPLIT: invalid row_delimiter"))));
        }
        if (wRowDelimVar.IsError()) {
            return(tStackElem(wRowDelimVar));
        }
        if (!wRowDelimVar.IsNull()) {
            VariantToText(wRowDelimVar, wRowDelim);
            wHasRowDelim = true;
        }
    }

    tBool wIgnoreEmpty = false;
    if (wArgs.size() >= 4) {
        if (!StackElemToBool(wArgs[3], wIgnoreEmpty)) {
            wIgnoreEmpty = false;
        }
    }

    tVariant wPadWith(tClassError(tTypeError::t_na, ""));
    if (wArgs.size() >= 6) {
        tVariant wPadVar;
        if (StackElemToVariant(wArgs[5], wPadVar)) {
            if (wPadVar.IsError() && wPadVar.Error().Code() != tTypeError::t_na) {
                return(tStackElem(wPadVar));
            }
            wPadWith = wPadVar;
        }
    }

    tString wText;
    tString wColDelim;
    VariantToText(wTextVar, wText);
    VariantToText(wColDelimVar, wColDelim);

    std::vector<std::vector<tString>> wGrid;
    if (wHasRowDelim) {
        std::vector<tString> wRows;
        TextSplitByDelim(wText, wRowDelim, wIgnoreEmpty, wRows);
        tSize wMaxCols = 0;
        for (const tString& wRowText : wRows) {
            std::vector<tString> wCols;
            TextSplitByDelim(wRowText, wColDelim, wIgnoreEmpty, wCols);
            if (wCols.size() > wMaxCols) {
                wMaxCols = wCols.size();
            }
            wGrid.push_back(wCols);
        }
        if (wGrid.empty()) {
            wGrid.push_back(std::vector<tString>(1, tString()));
            wMaxCols = 1;
        }
        if (wMaxCols < 1) {
            wMaxCols = 1;
        }
        tArrayValue* wArray = new tArrayValue(static_cast<tIndex>(wGrid.size()), static_cast<tIndex>(wMaxCols));
        for (tSize r = 0; r < wGrid.size(); ++r) {
            for (tSize c = 0; c < wMaxCols; ++c) {
                if (c < wGrid[r].size()) {
                    wArray->At(static_cast<tIndex>(r), static_cast<tIndex>(c)) = tVariant(wGrid[r][c]);
                } else {
                    wArray->At(static_cast<tIndex>(r), static_cast<tIndex>(c)) = wPadWith;
                }
            }
        }
        return(tStackElem(wArray));
    }

    // Column-only split -> single row
    std::vector<tString> wCols;
    TextSplitByDelim(wText, wColDelim, wIgnoreEmpty, wCols);
    tArrayValue* wArray = new tArrayValue(1, static_cast<tIndex>(wCols.size()));
    for (tSize c = 0; c < wCols.size(); ++c) {
        wArray->At(0, static_cast<tIndex>(c)) = tVariant(wCols[c]);
    }
    return(tStackElem(wArray));
}

//=============================================================================
// REGEX* (ECMAScript / std::regex — not Excel PCRE2)
//=============================================================================
namespace {
    static tBool RegexPatternOk(const tString& sPattern, tBool sIgnoreCase) {
        try {
            const auto wFlags = sIgnoreCase
                ? (std::regex::ECMAScript | std::regex::icase)
                : std::regex::ECMAScript;
            std::regex wRx(sPattern, wFlags);
            (void)wRx;
            return true;
        } catch (const std::regex_error&) {
            return false;
        }
    }

} // namespace

tFunctionRegexTest::tFunctionRegexTest() : tFunction() {}

tStackElem tFunctionRegexTest::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    // REGEXTEST(text, pattern, [case_sensitivity])
    if (wArgs.size() < 2 || wArgs.size() > 3) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "REGEXTEST requires 2 or 3 arguments"))));
    }
    std::reverse(wArgs.begin(), wArgs.end());
    tVariant wTextVar, wPatVar;
    if (!StackElemToVariant(wArgs[0], wTextVar) || !StackElemToVariant(wArgs[1], wPatVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    if (wTextVar.IsError()) return(tStackElem(wTextVar));
    if (wPatVar.IsError()) return(tStackElem(wPatVar));
    tString wText, wPat;
    VariantToText(wTextVar, wText);
    VariantToText(wPatVar, wPat);
    tBool wIgnoreCase = false;
    if (wArgs.size() >= 3) {
        tInt wCase = 0;
        // Excel: 0 = case-sensitive (default), 1 = ignore case.
        if (!StackElemToInt(wArgs[2], wCase)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        wIgnoreCase = (wCase != 0);
    }
    if (!RegexPatternOk(wPat, wIgnoreCase)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "REGEXTEST: invalid pattern"))));
    }
    tClassString wCs(wText);
    return(tStackElem(tVariant(wCs.Regex_Search(wPat, wIgnoreCase))));
}

tFunctionRegexReplace::tFunctionRegexReplace() : tFunction() {}

tStackElem tFunctionRegexReplace::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    // REGEXREPLACE(text, pattern, replacement, [occurrence], [case_sensitivity])
    // MVP: occurrence ignored (replace all); case optional as 4th or 5th.
    if (wArgs.size() < 3 || wArgs.size() > 5) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "REGEXREPLACE requires 3 to 5 arguments"))));
    }
    std::reverse(wArgs.begin(), wArgs.end());
    tVariant wTextVar, wPatVar, wRepVar;
    if (!StackElemToVariant(wArgs[0], wTextVar) || !StackElemToVariant(wArgs[1], wPatVar) ||
        !StackElemToVariant(wArgs[2], wRepVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    if (wTextVar.IsError()) return(tStackElem(wTextVar));
    if (wPatVar.IsError()) return(tStackElem(wPatVar));
    if (wRepVar.IsError()) return(tStackElem(wRepVar));
    tString wText, wPat, wRep;
    VariantToText(wTextVar, wText);
    VariantToText(wPatVar, wPat);
    VariantToText(wRepVar, wRep);
    tBool wIgnoreCase = false;
    if (wArgs.size() >= 5) {
        tInt wCase = 0;
        if (!StackElemToInt(wArgs[4], wCase)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        wIgnoreCase = (wCase != 0);
    } else if (wArgs.size() == 4) {
        // 4th arg may be occurrence (ignored) or case; treat as case if 0/1.
        tInt wV = 0;
        if (StackElemToInt(wArgs[3], wV) && (wV == 0 || wV == 1)) {
            wIgnoreCase = (wV != 0);
        }
    }
    if (!RegexPatternOk(wPat, wIgnoreCase)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "REGEXREPLACE: invalid pattern"))));
    }
    tClassString wCs(wText);
    if (!wCs.Regex_Replace(wPat, wRep, wIgnoreCase)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "REGEXREPLACE: invalid pattern"))));
    }
    return(tStackElem(tVariant(wCs())));
}

tFunctionRegexExtract::tFunctionRegexExtract() : tFunction() {}

tStackElem tFunctionRegexExtract::Call(tStackElems* sStackElems, tShort sNbArg) {
    std::vector<tStackElem> wArgs = PopArgs(sStackElems, sNbArg);
    // REGEXEXTRACT(text, pattern, [return_mode], [case_sensitivity]) — MVP return_mode 0 only.
    if (wArgs.size() < 2 || wArgs.size() > 4) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_arg, "REGEXEXTRACT requires 2 to 4 arguments"))));
    }
    std::reverse(wArgs.begin(), wArgs.end());
    tVariant wTextVar, wPatVar;
    if (!StackElemToVariant(wArgs[0], wTextVar) || !StackElemToVariant(wArgs[1], wPatVar)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
    }
    if (wTextVar.IsError()) return(tStackElem(wTextVar));
    if (wPatVar.IsError()) return(tStackElem(wPatVar));
    tString wText, wPat;
    VariantToText(wTextVar, wText);
    VariantToText(wPatVar, wPat);
    tInt wMode = 0;
    if (wArgs.size() >= 3) {
        if (!StackElemToInt(wArgs[2], wMode) || wMode != 0) {
            // Modes 1/2 not implemented yet.
            if (wMode != 0) {
                return(tStackElem(tVariant(tClassError(tTypeError::t_value, "REGEXEXTRACT: only return_mode 0 is supported"))));
            }
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
    }
    tBool wIgnoreCase = false;
    if (wArgs.size() >= 4) {
        tInt wCase = 0;
        if (!StackElemToInt(wArgs[3], wCase)) {
            return(tStackElem(tVariant(tClassError(tTypeError::t_value, ""))));
        }
        wIgnoreCase = (wCase != 0);
    }
    if (!RegexPatternOk(wPat, wIgnoreCase)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_value, "REGEXEXTRACT: invalid pattern"))));
    }
    tClassString wCs(wText);
    tString wOut;
    if (!wCs.Regex_ExtractFirst(wPat, wOut, wIgnoreCase)) {
        return(tStackElem(tVariant(tClassError(tTypeError::t_na, ""))));
    }
    return(tStackElem(tVariant(wOut)));
}

} // End namespace

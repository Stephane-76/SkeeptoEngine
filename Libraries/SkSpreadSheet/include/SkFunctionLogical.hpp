//=============================================================================
// SkSpreadSheet Function Logical
//=============================================================================
#ifndef SkFunctionLogical_hpp
#define SkFunctionLogical_hpp

#include "SkFunction.hpp"

namespace SkSpreadSheet {
	
	//=========================================================================
	//! Function If
	class tFunctionIf : public tFunction {
	public:
		/// @brief		Constructor SkFunctionSum.
		tFunctionIf();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! IFERROR / IFNA — shared trap: return value_if_* when the first arg matches.
	//! Constructor: sNaOnly=false → IFERROR (any error); sNaOnly=true → IFNA (#N/A only).
	class tFunctionIfError : public tFunction {
	private:
		tBool m_NaOnly;
	public:
		explicit tFunctionIfError(tBool sNaOnly);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function Ifs
	class tFunctionIfs : public tFunction {
	public:
		/// @brief		Constructor SkFunctionIfs.
		tFunctionIfs();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function Switch
	//! SWITCH(expression, value1, result1, [value2, result2, ...], [default]) — returns the result of the
	//! first value that equals the expression; the optional trailing lone argument is the default, else #N/A.
	class tFunctionSwitch : public tFunction {
	public:
		/// @brief		Constructor SkFunctionSwitch.
		tFunctionSwitch();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function Choose
	//! CHOOSE(index, value1, value2, ...) — returns the value at the 1-based position given by index.
	//! Array index CHOOSE({1,2,3}, col1, col2, col3) stitches the selected args (Excel pre-HSTACK).
	//! Positional selection (unlike SWITCH which compares values); out-of-range index yields #VALUE!.
	class tFunctionChoose : public tFunction {
	public:
		/// @brief		Constructor SkFunctionChoose.
		tFunctionChoose();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function AND
	class tFunctionAnd : public tFunction {
	public:
		/// @brief		Constructor SkFunctionAnd.
		tFunctionAnd();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function OR
	class tFunctionOr : public tFunction {
	public:
		/// @brief		Constructor SkFunctionOr.
		tFunctionOr();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function XOR (Excel XOR) — TRUE when an odd number of arguments are TRUE.
	//=========================================================================
	class tFunctionXor : public tFunction {
	public:
		tFunctionXor();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function Not
	//=========================================================================
	class tFunctionNot : public tFunction {
	public:
		/// @brief		Constructor SkFunctionNot.
		tFunctionNot();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function IsBlank	
	class tFunctionIsBlank : public tFunction {
	public:
		/// @brief		Constructor SkFunctionSum.
		tFunctionIsBlank();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function Na
	class tFunctionNa : public tFunction {
	public:
		/// @brief		Constructor SkFunctionNa.
		tFunctionNa();

		/// @brief		Call function with arguments.
		/// @param[in]  sArgVector tStackElemVector* vector of arguments
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function TRUE() / FALSE() — shared implementation (Excel constants as 0-arg functions).
	//! Bare TRUE/FALSE remain boolean literals via the lexer; TRUE()/FALSE() call this.
	class tFunctionTrueFalse : public tFunction {
	private:
		tBool m_Value;
	public:
		explicit tFunctionTrueFalse(tBool sValue);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Shared IS* type predicates (Excel information functions).
	//! One Call() path; constructor selects the predicate (ISNA / ISERR / ISERROR / …).
	//! IsRef / IsFormula inspect the stack element (Cell/Range) rather than the cell value.
	enum class tIsTypeKind : tByte {
		IsNa = 0,       // ISNA: TRUE only for #N/A
		IsErr = 1,      // ISERR: TRUE for any error except #N/A
		IsError = 2,    // ISERROR: TRUE for any error including #N/A
		IsNumber = 3,   // ISNUMBER: numeric / date (Excel serial)
		IsLogical = 4,  // ISLOGICAL: boolean
		IsText = 5,     // ISTEXT: text
		IsNonText = 6,  // ISNONTEXT: anything that is not text
		IsRef = 7,      // ISREF: TRUE if arg is a Cell/Range reference (not its value)
		IsFormula = 8   // ISFORMULA: TRUE if reference's cell has a formula; else #VALUE! if not a ref
	};

	class tFunctionIsType : public tFunction {
	private:
		tIsTypeKind m_Kind;
	public:
		explicit tFunctionIsType(tIsTypeKind sKind);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! ISEVEN / ISODD — shared parity test (Excel truncates toward zero first).
	//! Constructor: sEven=true → ISEVEN; sEven=false → ISODD.
	class tFunctionIsParity : public tFunction {
	private:
		tBool m_Even;
	public:
		explicit tFunctionIsParity(tBool sEven);
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function IsOmitted (Excel ISOMITTED)
	//! Returns TRUE when its argument is a LAMBDA optional parameter that the caller did not supply.
	//! Any other value (including blank / #N/A / #VALUE!) yields FALSE.
	class tFunctionIsOmitted : public tFunction {
	public:
		tFunctionIsOmitted();

		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! N — convert value to a number (Excel information): number/date→self, TRUE→1, FALSE/text/blank→0.
	class tFunctionN : public tFunction {
	public:
		tFunctionN();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! TYPE — Excel type code: 1 number, 2 text, 4 logical, 16 error, 64 array.
	class tFunctionType : public tFunction {
	public:
		tFunctionType();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! ERROR.TYPE (registered ERROR_TYPE) — numeric code for an error value; else #N/A.
	class tFunctionErrorType : public tFunction {
	public:
		tFunctionErrorType();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! FORMULATEXT(reference) — formula text with leading '=' (Excel); #N/A if no formula.
	class tFunctionFormulaText : public tFunction {
	public:
		tFunctionFormulaText();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! CELL(info_type, [reference]) — cell metadata (address/col/row/contents/type/filename…).
	//! Without reference, uses the calling cell (PassByRef), like ROW().
	class tFunctionCellInfo : public tFunction {
	private:
		tItem* m_ItemRef;
	public:
		tFunctionCellInfo();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		void PassByRef(tItem* sItem) override;
		tBool ByRef() override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! INFO(type_text) — environment info (directory/numfile/osversion/recalc/release/system).
	class tFunctionInfo : public tFunction {
	public:
		tFunctionInfo();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! SHEET([value]) — 1-based sheet index. Arg: ref, sheet name text, or omitted (caller sheet).
	class tFunctionSheetNum : public tFunction {
	private:
		tItem* m_ItemRef;
	public:
		tFunctionSheetNum();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		void PassByRef(tItem* sItem) override;
		tBool ByRef() override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! SHEETS([reference]) — sheet count in the workbook (3D refs not supported).
	class tFunctionSheets : public tFunction {
	private:
		tItem* m_ItemRef;
	public:
		tFunctionSheets();
		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		void PassByRef(tItem* sItem) override;
		tBool ByRef() override;
		tFunctionSpillKind SpillKind() const override;
	};

	//=========================================================================
	//! Function LET (registration stub only).
	//! LET is lowered to dedicated scope opcodes at compile time, so Call() is never reached during a
	//! normal evaluation. It exists so the function dictionary knows the name "LET".
	class tFunctionLet : public tFunction {
	public:
		tFunctionLet();

		tStackElem Call(tStackElems* sStackElems, tShort sNbArg) override;
		tFunctionSpillKind SpillKind() const override;
	};

}; // End of namespace

#endif

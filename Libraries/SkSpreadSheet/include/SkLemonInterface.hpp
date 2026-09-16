//=============================================================================
// SkLemonInterface // SpreadSheet interface width Lemon
//=============================================================================
#ifndef SkLemonInterface_hpp
#define SkLemonInterface_hpp
#include <SkTypes.hpp>
#include <SkClass.hpp>
#include <utility>
#include <vector>
#include <set>

#include "SkInterfaceCompil.hpp"

#include "SkLemonReserved.hpp"
#include "SkLexerSpreadSheet.hpp"
#include "SkLexerData.hpp"

using namespace SkRoot;

namespace SkSpreadSheet {

    class tRangeData;
    class tColumnData;
    class tColRowCellRange;
    class tRange;

    enum class tErrorFormula : tChar { t_None,t_SyntaxError,t_Function,t_Data,t_Ref,t_Attribute };

	//=========================================================================
	//! Model of function for treatment in lemon parser
	class tLemonFunctionMethod : tClass {
	private:
		//! Name of function
		tString m_Name;
		//! Number of argument
		tInt m_NbArg;

		tBool m_IsMethod;
        
        // Pointer on fonction for by ref
        tFunction* m_Function;
        
        // Lexer token for SUM(A1:INDEX(Data,1,2)
        tLexerToken* m_DynamicRight;

        //! True when this call is the built-in IF (compiled to short-circuit opcodes, not an eager call).
        tBool m_IsIf;
        //! Bytecode length (m_CurrentFormula size) recorded after each argument, used to place IfCond/IfElse.
        tVectorInt m_ArgEndPos;
	public:
		/// @brief		Constructor tLemonFunction.
		tLemonFunctionMethod();

		/// @brief		Constructor of copy.
		/// @param[in]	sLemonFunction const SkLemonFunction&
		tLemonFunctionMethod(const tLemonFunctionMethod& sLemonFunction);

		/// @brief		Constructor tLemonFunction with name.
		/// @param[in]	sName tString
		/// @param[in]	sDynamicRight tLexerToken*
		/// @param[in]	sIsMethod tBool
        tLemonFunctionMethod(tString sName,tLexerToken* sDynamicRight, tBool sIsMethod);
        
        /// @brief        Return function method
        tFunction* Function();

		/// @brief		Return name.
		/// @return		tString
		tString Name();

		/// @brief		Set name.
		/// @param[in]	sName tString
		void Name(tString sName);

		/// @brief		Return number of argument.
		/// @return		tInt
		tInt NbArg();

		/// @brief		Increment number of argument.
		void IncArg();

		/// @brief		Return dynamic right.
		/// @return		tLexerToken
		tLexerToken* DynamicRight();

		/// @brief		Set dynamic right.
		/// @param[in]	sDynamicRight tLexerToken
		void DynamicRight(tLexerToken* sDynamicRight);

		/// @brief		True when this is the built-in IF (short-circuit compilation).
		/// @return		tBool
		tBool IsIf() const;

		/// @brief		Record the current bytecode length as an argument boundary (for IfCond/IfElse placement).
		/// @param[in]	sPos tInt
		void AddArgEndPos(tInt sPos);

		/// @brief		Argument boundary positions recorded during parsing.
		/// @return		const tVectorInt&
		const tVectorInt& ArgEndPos() const;
	};
	typedef stack<tLemonFunctionMethod> tStackLemonFunction;
 

	//=========================================================================
	//! Class interface with Lemon. Used to compile a cell
	class tLemonInterface : tClass {
	private:
		//! Stack of function used by parser.
		tStackLemonFunction m_StackFunction;
  
		//! For reserved word.
		tLemonReserved	     m_LemonReserved;
		//! Pointer of owner Workbook.
		tWorkBook*			 m_WorkBook;

		//! cell compiled.
        tInterfaceCompil*	m_InterfaceCompil;

        //! Formula.
		tFormula            m_CurrentFormula;
		//! For init done.
		void*				 m_LemonParser;

		//! True if error.
        tErrorFormula       m_CompilError;

        //! Parse code
        tString             m_Code;
        
		//! Explanation of the error. ==============================================
		tString             m_Error;
        tInt                m_ErrorLine;
        tInt                m_ErrorColumn;
        
        //! Locale lang =======================================================
        tChar               m_Decimal;
        tChar               m_Arg;

        //! Per Compil() pass: cache RangeData and column lookups for structured refs (Table[[#This Row],[Col]]).
        std::unordered_map<tString, tRangeData*> m_RangeDataCache;
        std::unordered_map<tString, tColumnData*> m_ColumnDataByTableAndName;
        //! Scratch buffer for Excel @[Col] / [@Col] shorthand lexemes.
        tString m_ThisRowColScratch;

        //! Uppercased LET binding names discovered in the current formula (Excel LET is case-insensitive).
        //! Populated lazily by CollectLetNames() the first time the formula-key pass meets the "LET" token
        //! (SetCellDependAndFormulaKey), so formulas without LET pay nothing. The key pass runs before the
        //! Lemon parse pass and this set is a member, so both passes agree on which identifiers are LET
        //! locals. LET locals never consume a VectorRef slot (neither a "name" placeholder in the key nor a
        //! PushRef in the bytecode), which keeps reconstruction (tFormula::Str) and the RPN evaluation
        //! aligned on the same VectorRef ordering.
        std::set<tString> m_LetBoundNames;
        //! True once CollectLetNames() has run for the current formula (guards against re-scanning on
        //! nested/sequential LET tokens). Reset at the start of each SetCellDependAndFormulaKey().
        tBool m_LetNamesCollected = false;

        //! Extra scoped names injected before a single compilation (one-shot). Used to compile a LAMBDA body
        //! where the parameter names must resolve as local scope references (LetVarRef), exactly like LET
        //! locals: they consume no VectorRef slot and are bound at call time. Set via SetInjectedScopeNames()
        //! and merged into m_LetBoundNames at the start of SetCellDependAndFormulaKey(), then cleared.
        std::set<tString> m_InjectedScopeNames;

        void ClearTableDataCache();
        tRangeData* CachedRangeData(const tString& sTableName);
        tColumnData* CachedFindColumnByName(tRangeData* sRangeData, tSheet* sSheet, tRange* sTableRange,
                                            const tString& sTableName, const tString& sColumnName);

        // Interface SetCellAndFormulaKey ==============================
        
        ///@brief Return Attribute of cell
        ///@param[in] sCell tCell*
        ///@param[in] sAttribute tSring
        ///@return tCellAttribute*
        tCellAttribute* CellAttribute(tCell* sCell,tString sAttribute);
        
        
        /// @brief
        /// @param sCellToken
        /// @param[in] sAttributeToken tLexerToken*
        /// @return tCell*
        SkInline tCell* Cell(tLexerToken* sCellToken,tLexerToken* sAttributeToken=nullptr);
        
        /// @brief 
        /// @param sCellTokenSheet 
        /// @param sCellToken
        /// @param[in] sAttributeToken tLexerToken*
        /// @return tCell*
        SkInline tCell* SheetCell(tLexerToken* sCellTokenSheet,tLexerToken* sCellToken,tLexerToken* sAttributeToken=nullptr);
        
        /// @brief 
        /// @param sCellTokenTop 
        /// @param sCellTokenBottom
         ///@param[in] sAttribute tSring
        /// @return tRange*
        SkInline tRange* Range(tLexerToken* sCellTokenTop, tLexerToken* sCellTokenBottom);
        
        /// @brief 
        /// @param sCellTokenSheet 
        /// @param sCellTokenTop 
        /// @param sCellTokenBottom
        /// @return tRange*
        SkInline tRange* Range(tLexerToken* sCellTokenSheet, tLexerToken* sCellTokenTop, tLexerToken* sCellTokenBottom);

        
        /// @brief      Push Cell on Ref
        /// @param[in]  sCell tCell*
        /// @param[in]  sAttribute tString
        SkInline void _SetCell(tCell* sCell,tString sAttribute);

        /// @brief       Push Range on Ref.
        /// @param[in]   sRange tRange*
        SkInline void _SetRange(tRange* sRange);
        
        ///@brief Is TableName
        ///@param[in] sLexerToken tLexerToken*
        ///@return tBool
        SkInline tBool IsTableName(tLexerToken* sLexerToken);
	public:
		/// @brief		Constructor SkLemonFunction.
		tLemonInterface();

		/// @brief		Clear All
		void Clear();

		/// @brief		Clear m_Current formula and m_StackFunction.
		void ClearCompil();

        /// @brief        Return Sheet by name ( if sName is empty retturn current sheet .
        /// @param[in]    sSheetName tString
        /// @return       tSheet*
        tSheet* Sheet(tString sSheetName);
        
        /// @brief       SheetByToken
        /// @param[in]   sCellTokenSheet tLexerToken*
        /// @return      tSheet*
        tSheet* SheetByToken(tLexerToken* sCellTokenSheet);
        
        
		/// @brief		Compil sCode to Cell.
        /// @param[in]  sWorkBook tWorkBook*
		/// @param[in]  sInterface  tInterfaceCompil* 
		/// @param[in]  sCode const tChar* Code
		/// @return		tBool false if error
		tBool Compil(tWorkBook* sWorkBook, tInterfaceCompil* sInterfaceCompil, const tChar* sCode);

		/// @brief		True if sName is an internal hidden name generated for an inline LAMBDA(...) (see
		///				DesugarInlineLambdas). Such names are never persisted (JsonFormulaNamed skips them) and
		///				are expanded back to their verbatim LAMBDA(...) text by tFormula::Str.
		/// @param[in]	sName const tString&
		/// @return		tBool
		static tBool IsInlineLambdaName(const tString& sName);

		/// @brief		Rewrites inline LAMBDA(...) occurrences of sCode into references to hidden named
		///				lambdas. Each outermost LAMBDA(...) span is registered as a normal named LAMBDA under a
		///				generated hidden name and replaced by that name, so =LAMBDA(x;x+1)(5) becomes name(5)
		///				and MAP(A;LAMBDA(x;x*2)) becomes MAP(A;name). Nested inline lambdas are handled when the
		///				hidden lambda's own body is compiled (recursively). No-op when sCode has no LAMBDA.
		/// @param[in]	sWorkBook tWorkBook* host workbook (owns the named-formula container)
		/// @param[in]	sCode const tString& original formula source
		/// @return		tString rewritten formula (== sCode when nothing to desugar)
		tString DesugarInlineLambdas(tWorkBook* sWorkBook, const tString& sCode);

		/// @brief		Creates a unique key only with the lexer.
		/// @param[in]  sCode const tChar* Code
		/// @return		tString
		tString SetCellDependAndFormulaKey(const tChar* sCode);

		// Reference Identifier =====================================================
		/// @brief		Add Identifier (Reserved word).
		/// @param[in]  sName tString 
		/// @param[in]  sId tInt Id of reserved word
		/// @param[in]	sClass SkVirtualClass* Class for variant
		/// @param[in]	sKind tKind Kind of IdReference
		/// @return		tBool false if name already exist
		tBool AddIdRef(tString sName, tInt sId, tVirtualClass* sClass, tKind sKind);

		/// @brief		Return id of reserved word by name.
		/// @param[in]  sName tString
		/// @return		tInt Id
		tInt Id(tString sName);

		/// @brief		Return Range Named.
		/// @param[in]  sName tString
		/// @return		tRange* 
		tRange* RangeNamed(tString sName);
  
        /// @brief        Return Cell of SkCellClass.
        /// @param[in]     sName tString
        /// @return        tCell*
        tCell* CellClass(tString sName);
        
        /// @brief        Return Cell of SkCellClass.
        /// @param[in]     sSheet tSheet*
        /// @param[in]     sName tString
        /// @return        tCell*
        tCell* CellClassWhithSheet(tSheet* sSheet,tString sName);

		// Reference Function ==================================================
		/// @brief		Return number of argument by name of function.
		/// @param[in]  sName tString
		/// @return		tInt Number of argument
		tInt NbArg(tString sName);

		// Interface Cell
		/// @brief		Return Point Col Row (notation A1 or R1C1).
		/// @param[in]  sCellToken SkLexerToken* cell
		/// @return tTempoPoint;
		tTempoPoint* GetColRow(tLexerToken* sCellToken);

		/// @brief		Create cell by token.
		/// @param[in]  sAttribute tString
		void SetCell(tLexerToken* sCellToken,tString sAttribute);
	
        /// @brief        Create cell by token for another sheet.
        /// @param[in]  sCellTokenSheet SkLexerToken* sheet
        /// @param[in]  sAttribute tString
        void SetSheetCell(tLexerToken* sCellTokenSheet, tLexerToken* sCellToken, tString sAttribute);

		/// @brief		Resolve one bound of a partial range (#REF!:MyRange, A1:MyRange).
		void SetPartialRangeBound(tLexerToken* sCellToken);

		/// @brief		Create range with 2 token top/left bottom/right.
		/// @param[in]  sCellTokenTop SkLexerToken* top/left
		/// @param[in]  sCellTokenBottom SkLexerToken* bottom/right
		void SetRange(tLexerToken* sCellTokenTop, tLexerToken* sCellTokenBottom);
		
		/// @brief		Create range by token for another sheet.
		/// @param[in]  sCellTokenSheet SkLexerToken* sheet
		/// @param[in]  sCellTokenTop SkLexerToken* top/left
		/// @param[in]  sCellTokenBottom SkLexerToken* bottom/right
		void SetSheetRange(tLexerToken* sCellTokenSheet, tLexerToken* sCellTokenTop, tLexerToken* sCellTokenBottom);

		/// @brief		Create RangeNamed by token.
		/// @param[in]  sCellTokenName SkLexerToken*
        /// @param[in]  sAttribute tString
        /// @return     tBool   (Error Range.attribute)
		tBool SetCellClassOrRangeNamed(tSheet* sSheet,tLexerToken* sCellTokenName,tString sAttribute);

		/// @brief      Named range + R1C1 suffix (Excel: MyNameR[-1]C[2]).
		tBool SetNamedRangeR1C1Offset(tSheet* sSheet, tLexerToken* sNameToken, tLexerToken* sRcToken,
		                              tString sAttribute);
  
		/// @brief		Return Table Formula Key.
		/// @param[in]  sTableName tLexerToken*
		/// @param[in]  sColumName tLexerToken*
		/// @return     tString
		tString  TableColumnKey(tLexerToken* sTableName, tLexerToken* sColumName);

        /// @brief		Set  Table
        /// @brief[out     sLex]
        /// @param[in]  sSheet tLexerToken;
        /// @param[in]  sTable  tLexerToken
		/// @param[in]  sData  tLexerToken.
        /// @param[out] sTokenLex tString&
        /// @return     tBool
        tBool SetTable(tLexer& sLex,tLexerToken* sTable, tLexerToken* sData,tString& sTokenLex);

		/// @brief		Create ErrorRef.
		/// @param[in]  sErrorToken SkLexerToken* ;
		void SetErrorRef(tLexerToken* sErrorToken);

		/// @brief		Create ErrorName.
		/// @param[in]  sErrorToken SkLexerToken* ;
		void SetErrorName(tLexerToken* sErrorToken);

		// Interface formula stack =====================================================
		/// @brief		Push integer on formula stack.
		/// @param[in]  sCellToken tLexerToken*
		/// @param[in]  sNegate    tBool negate the literal (for signed array constants like {-1,-1})
		void PushInteger(tLexerToken* sCellToken, tBool sNegate = false);

		/// @brief		Push float on formula stack.
		/// @param[in]  sCellToken tLexerToken*
		/// @param[in]  sNegate    tBool negate the literal (for signed array constants like {-1.5})
		void PushFloat(tLexerToken* sCellToken, tBool sNegate = false);

        /// @brief        Push Boolt on formula stack.
        /// @param[in]  sCellToken texerToken*
        void PushBool(tLexerToken* sCellToken);
        
		/// @brief		Push label on formula stack.
		/// @param[in]  sCellToken tLexerToken*
		void PushLabel(tLexerToken* sCellToken);

		/// @brief		Push cell on formula stack.
		/// @param[in]  sCellToken tLexerToken*
        /// @param[in]  sAttributeToken tLexerToken*
        void PushCell(tLexerToken* sCellToken,tLexerToken* sAttributeToken=nullptr);

		/// @brief		Push Excel spilled-range operator (D14#) on formula stack.
		void PushSpillRef(tLexerToken* sCellToken);

		/// @brief		Push range on formula stack.
		/// @param[in]  sCellTokenTop SkLexerToken* top/left
		/// @param[in]  sCellTokenBottom SkLexerToken* bottom/right
		void PushRange(tLexerToken* sCellTokenTop, tLexerToken* sCellTokenBottom);

		/// @brief		Push dynamic range: left = static cell, right = expression result (e.g. A1:INDEX(...)). No PushRange.
		/// @param[in]  sCellTokenLeft tLexerToken* left cell
		void PushDynamicRangeRight(tLexerToken* sCellTokenLeft);

		/// @brief		Push dynamic range: left = expression result, right = static cell (e.g. INDEX(...):B2). No PushRange.
		/// @param[in]  sCellTokenRight texerToken* right cell
		void PushDynamicRangeLeft(tLexerToken* sCellTokenRight);

		/// @brief		Push dynamic range: both bounds from stack (e.g. Function1():Function2()). No PushRange.
		void PushDynamicRangeBoth();

		/// @brief		Push cell in another sheet on formula stack.
		/// @param[in]  sCellTokenSheet SkLexerToken* sheet
		/// @param[in]  sCellToken tLexerToken*
        /// @param[in]  sAttributeToken tLexerToken*
		void PushSheetCell(tLexerToken* sCellTokenSheet, tLexerToken* sCellToken,tLexerToken* sAttributeToken=nullptr);

		/// @brief		Push sheet-qualified spilled-range operator (Sheet!D14#).
		void PushSheetSpillRef(tLexerToken* sCellTokenSheet, tLexerToken* sCellToken);

		/// @brief		Push sheet-qualified #REF! (e.g. 'Sheet'!#REF!) on formula stack.
		void PushSheetErrorRef(tLexerToken* sCellTokenSheet, tLexerToken* sErrorRefToken, tLexerToken* sAttributeToken=nullptr);

		/// @brief		Push range in another sheet on formula stack.
		/// @param[in]  sCellTokenSheet SkLexerToken* sheet
		/// @param[in]  sCellTokenTop SkLexerToken* top/left
		/// @param[in]  sCellTokenBottom SkLexerToken* bottom/right
		void PushSheetRange(tLexerToken* sCellTokenSheet, tLexerToken* sCellTokenTop, tLexerToken* sCellTokenBottom);

		/// @brief		Push ID on formula stack.
        /// @param[in]  sSheet tSheet*
        /// @param[in]  sCellToken SkLexerToken*
        /// @param[in]  sCellTokenAttribute  SkLexerToken*
        void PushID(tLexerToken* sCellToken,tLexerToken* sLexerAttribute);

        /// @brief Lemon pass: named range + R1C1 offset (VectorRef filled in pre-pass).
        void PushNamedRangeR1C1OffsetParse(tLexerToken* sNameToken, tLexerToken* sRcToken,
                                           tLexerToken* sAttributeToken);
        void PushSheetNamedRangeR1C1OffsetParse(tLexerToken* sSheetToken, tLexerToken* sNameToken,
                                                tLexerToken* sRcToken, tLexerToken* sAttributeToken);

        /// @brief       PushSheet  ID on formula stack. (Cell Class)
        /// @param[in]  sCellTokenSheet SkLexerToken* sheet
        /// @param[in]  sCellToken SkLexerToken*
         /// @param[in]  sCellTokenAttribute  SkLexerToken*
        void PushSheetID(tLexerToken* sCellTokenSheet, tLexerToken* sCellToken,tLexerToken* sLexerAttribute);

		/// @brief		Push ErrorRef on formula stack.
		/// @param[in]  sCellToken SkLexerToken*
		void PushErrorRef(tLexerToken* sCellToken);

		/// @brief		Push ErrorName on formula stack.
		/// @param[in]  sCellToken SkLexerToken*
		void PushErrorName(tLexerToken* sCellToken);

		/// @brief		Push Operator (+,-,*,/) on formula stack.
		/// @param[in]  sOp SkLexerToken*
		void PushOp(tKind sOp);

		/// @brief Excel postfix percent: emit 100 then Divide so 50% evaluates to 0.5.
		void PushPercentLiteral();

		/// @brief		Push model function. 
		/// @param[in]  sLemonFunction SkLemonFunction*
		void PushFunctionMethod(tLemonFunctionMethod* sLemonFunction);

		/// @brief		Push function by name (Interface lemon for function while parsing). 
		/// @param[in]  sFunction tString
		void PushFunctionMethod(tString  sFunction, tBool sIsMethod);

		/// @brief		Push function by name with optional dynamic range right token (e.g. SUM(A1:INDEX(...))).
		/// @param[in]  sFunction tString
		/// @param[in]  sDynamicRight tLexerToken* (cell token before colon in A1:INDEX(...), or nullptr)
		/// @param[in]  sIsMethod tBool
		void LexPushFunctionMethod(tString sFunction, tLexerToken* sDynamicRight, tBool sIsMethod);

		/// @brief		Pop and return function. 
		/// @return		SkLemonFunction
		tLemonFunctionMethod PopFunctionMethod();
  

        /// @brief		Pueh  Table
        /// @param[in]  sSheet tLexerToken;
        /// @param[in]  sTable  tLexerToken
		/// @param[in]  sCol    tLexerToken
        tBool PushTable(tLexerToken* sTable, tLexerToken* sCol);

        /// @brief		Push #This Row column ref from Excel shorthand @[Col] or [@Col].
        tBool PushThisRowColumn(tLexerToken* sCol);

		/// @brief		Increment number of argument for function on stack (top). 
		void IncArg();

		// LET support =========================================================
		/// @brief		Scan the whole formula and collect every LET binding name (uppercased).
		///             Called lazily by SetCellDependAndFormulaKey on the first "LET" token, which the
		///             sequential key pass always meets before any binding name or body reference; the
		///             resulting set (a member) then serves the later Lemon parse pass too. A binding name
		///             is a lone identifier at an even argument position of a LET( ... ) call immediately
		///             followed by an argument separator (which excludes the trailing body expression).
		/// @param[in]  sCode const tChar*
		void CollectLetNames(const tChar* sCode);

		/// @brief		True if sName (case-insensitive) is a LET binding name of the current formula.
		/// @param[in]  sName const tString&
		/// @return		tBool
		tBool IsLetName(const tString& sName) const;

		/// @brief		Inject extra scoped names for the NEXT single compilation (one-shot). Used to compile a
		///             LAMBDA body: its parameter names must resolve as local scope references (LetVarRef),
		///             so they are merged into the LET binding set at compile time and bound at call time.
		///             Names are matched case-insensitively (stored uppercased). Cleared after being applied.
		/// @param[in]  sNames const std::set<tString>&
		void SetInjectedScopeNames(const std::set<tString>& sNames);

		/// @brief		Emit the opcode opening a new LET local scope (Lemon action).
		void LetBeginScope();

		/// @brief		Emit the opcode binding the stack top to sName in the current LET scope (Lemon action).
		/// @param[in]  sName tString
		void LetBind(tString sName);

		/// @brief		Emit the opcode closing the current LET scope (Lemon action).
		void LetEndScope();

		/// @brief		Whether a multi-area named range should be expanded
		///              into one opcode per area at this point of the compile.
		///              True only for a direct non-byref function argument
		///              (aggregation-style: SUM, AVERAGE, COUNT, ...).
		///              False for bare expressions, binary operators, and
		///              byref functions (OFFSET, ROWS, COLUMNS, ...) where
		///              Excel yields #VALUE! anyway — in that case we keep
		///              the first area so VectorRef and the opcode stream
		///              stay aligned 1:1.
		/// @return		tBool
		tBool ShouldExpandMultiAreaNamedRange();

		/// @brief		Push placeholder for omitted function argument (comma with no expression).
		void PushEmptyFunctionArg();

		/// @brief		Return m_LemonParser for init done. 
		/// @return		SkLemonFunction
		void* LemonParser();

		/// @brief		Set m_LemonParser for init done. 
		/// @param[in]	sLemonParser void*
		void LemonParser(void* sLemonParser);

		/// @brief		Return current SkFormula
		/// @return		tFormula*
		tFormula* Formula();

		/// @brief		Set compil error true or false. 
		/// @param[in]	sValue tErrorFormula
		void CompilError(tErrorFormula sValue);

		/// @brief		return compil error. 
		/// @param[in]	tErrorFormula
		tErrorFormula CompilError();
  
        /// @brief		Set explanation of the error.
		/// @param[in]	sLexerError tString
		/// @param[in]	sLexLine tInt
		/// @param[in]	sLexColumn tInt
		void Error(tString sLexerError,tInt sLexLine,tInt sLexColumn);

		/// @brief		Return explanation of the error.
		/// @return		tString
		tString Error();

        /// @brief        Return line error.
        /// @return       tInt
        tInt ErrorLine();

        /// @brief        Return column error.
        /// @return       tInt
        tInt ErrorColumn();

		/// @brief        Return error with detail.
		/// @return       tString
		tString ErrorWithDetail();

		/// @brief		Place formula on current cell.
		/// @return		tString
		void SetCellFormula();

		/// @brief		Return formula key.
		/// @return		Sktring
		tString FormulaKey();

		/// @brief		Set formula key.
		/// @param[in]	sFormulaKey tString
		void FormulaKey(tString sFormulaKey);

		/// @brief		For debug on console.
		void Debug();
	};
}
#endif

//=============================================================================
// SkSpreadSheet Range Named
//=============================================================================
#ifndef SkRangeNamed_hpp
#define SkRangeNamed_hpp

#include "../include/SkRange.hpp"
#include "../include/SkRangeData.hpp"
#include "../include/SkSharedFormula.hpp"
#include "../include/SkInterfaceCompil.hpp"
#include "../include/SkStackElem.hpp"

#include <vector>
using namespace SkRoot;

namespace SkSpreadSheet {

	// Forward: do not include SkCalculationPath.hpp here — SkColRowCellRange → SkUndoRedoSaveSp → SkRangeNamed
	// can skip the CalculationPath body while the include guard is active, leaving tContainerPath undefined.
	class tContainerPath;

	/// @brief		Default JSON export filter: Named ranges and their table Data.
	/// @return		tExtension with t_Named and t_Data bits set
	tExtension DefaultJsonRangeFilter();
 
    class tFormulaNamed : public tClass {
    private:
        tAllocatorRef m_CellRef;

		//! Name
		tAllocatorRef m_NameRef;

        //! Position for row 0 -> lastRow+1 other Undo 
		tIndex 		m_Row;

        //! Stable name (do not rely on the name-label cell alone; spill buffer is authoritative).
		tString 	m_Name;

		tString 	m_FormulaStr;

        //! Spill values for this named formula (sheet _$$ only): kept off-grid so definitions/names are not overwritten.
		std::vector<std::vector<tVariant>> m_SpillBuffer;
		tRange*     m_SpillRange;
        //! True after one late-bind force-eval this definition. Scalar names keep SpillRange null —
        //! without this, every dependent re-runs InternalCalculation (Budget MATCH/lst*).
		tBool       m_LateBindForceTried;

        //! Excel evaluates ROW()/COLUMN() (no args) and names that depend on them in the calling cell.
        //! Cached after the first IsCallerRelative() walk (cycle-safe).
        tBool       m_CallerRelativeKnown = false;
        tBool       m_CallerRelative = false;
        tBool       m_CallerUsesBareRow = false;
        tBool       m_CallerUsesBareColumn = false;

        //! LAMBDA support: true when the definition is =LAMBDA(param1;...;body). The parameter names (uppercased,
        //! in order) are stored here and the compiled cell holds only the BODY expression, where each parameter
        //! resolves as a local scope reference (LetVarRef). Parameters are bound to arguments at call time.
        tBool m_IsLambda = false;
        std::vector<tString> m_LambdaParams;

        //! Closure support (inline lambdas): outer names (uppercased) referenced by the body that are bound in the
        //! ENCLOSING scope (LET vars / enclosing lambda parameters). Injected as scope names when compiling the
        //! body (so they resolve to LetVarRef) and bound to their captured values at call time (see Invoke).
        std::vector<tString> m_CapturedNames;

        //! @brief If sFormulaStr is =LAMBDA(...), fill sParams (uppercased) and sBody (last argument). Returns
        //!        true only for a well-formed LAMBDA header (>= 1 parameter + a body). Top-level split honours
        //!        nested parentheses/braces and quoted strings.
        static tBool ParseLambda(const tString& sFormulaStr, std::vector<tString>& sParams, tString& sBody);

        //! @brief Retturn tColRowCellRange
        //! @return tColRowCellRange*
        tColRowCellRange* ColRowCellRange();
   public:
           /// Column on CstSheetNamed for the visible name string (I)
        static constexpr tIndex kFormulaNamedNameLabelCol = 2;
	    /// @brief      Constructor.
        /// @param[in]  sWorkBook tWorkBook*
		/// @param[in]  sFormulaStr tString
        /// @param[in]  sIndex sRow for undo
        tFormulaNamed(tString sName,tString sFormulaStr,tIndex sRow);

		/// @brief      Destructor.
		~tFormulaNamed();
        
        ///@brief  Clear
        void Clear();
        
        ///@brief Cell
        ///@returbn tCell*
        tCell* Cell();

		///@brief Name
		///@return tString
		tString Name();
  
        ///@brief Set FormulaStr
        ///@param[ind] sFormulaStr tString
        void FormulaStr(tString sFormulaStr);
        
        ///@brief Get FormulaStr
        ///@return tString 
        tString FormulaStr();
    
        ///@brief True when this named formula is a LAMBDA (callable with arguments).
        ///@return tBool
        tBool IsLambda() const;

        ///@brief Ordered, uppercased LAMBDA parameter names (empty when not a lambda).
        ///@return const std::vector<tString>&
        const std::vector<tString>& LambdaParams() const;

        ///@brief Set the closure names captured from the enclosing scope. Must be called BEFORE Compil so the body
        ///       compiles with them injected as scope names (they resolve to LetVarRef). Used by inline lambdas.
        ///@param[in] sNames const std::vector<tString>& (uppercased outer names)
        void SetCapturedNames(const std::vector<tString>& sNames);

        ///@brief Closure names captured from the enclosing scope (empty for ordinary/named lambdas).
        ///@return const std::vector<tString>&
        const std::vector<tString>& CapturedNames() const;

        ///@brief True when this lambda captures at least one outer name (it is a closure).
        ///@return tBool
        tBool HasCapturedNames() const;

        ///@brief Invoke this LAMBDA with the given arguments (bound to the parameters as a local scope) and
        ///       return the body result. Used by direct calls (NAME(args)) and higher-order functions
        ///       (MAP/REDUCE/SCAN). sArityOk is set false on a parameter/argument count mismatch (-> #VALUE!).
        ///@param[in]  sArgs const std::vector<tStackElem>&
        ///@param[out] sArityOk tBool&
        ///@param[in]  sCapturedScope optional closure environment (outer names -> values) seeded into the body's
        ///            scope before the parameters (parameters shadow captured names on collision).
        ///@return     tVariant
        tVariant Invoke(const std::vector<tStackElem>& sArgs, tBool& sArityOk,
                        const std::map<tString, tStackElem>* sCapturedScope = nullptr);

        ///@brief Compil Formula
        ///@param[in] sCode tString
        ///@param[in] sCalculate tBool
        ///@return    tBool
        tBool Compil(const tString sCode,tBool sCalculate);
        
        
        ///@brief
        ///@param[in] sFormulaStr tStrinb
        void SetJsonFormulaValue(tString sFormulaStr);

        /// @brief Clear in-memory spill buffer and spill range pointer (range object may still exist on sheet until ClearMatrix).
        void ClearSpillBuffer();

        /// @brief Set every spill buffer cell to null (keeps dimensions). Used by Undo / reset before optional ClearMatrix.
        void ClearSpillBufferValuesToNull();

        /// @brief Ensure spill buffer is at least sH x sW; never shrinks (multiple literals / ops share one buffer on _$$).
        void ResizeSpillBuffer(tIndex sH, tIndex sW);

        /// @brief Write one spill cell (relative row/col within result rect).
        void SetSpillBufferValue(tIndex sRow, tIndex sCol, const tVariant& sValue);

        /// @brief Register logical spill rectangle (EnsureRange on _$$) for lookups and buffer indexing.
        void SetSpillRange(tRange* sRange);

        /// @brief Spill range from last calculation (logical rect on named-formula sheet).
        tRange* SpillRange() const;

        tBool LateBindForceTried() const { return m_LateBindForceTried; }
        void SetLateBindForceTried() { m_LateBindForceTried = true; }

        /// @brief True when this name (or a name it references) uses bare ROW()/COLUMN().
        /// Excel then evaluates the definition in the calling cell, not on the _$$ host row.
        tBool IsCallerRelative();

        /// @brief True when a bare COLUMN() (or a name that uses one) participates in this definition.
        tBool CallerRelativeUsesColumn() const { return m_CallerUsesBareColumn; }

        /// @brief Evaluate the definition with ROW()/COLUMN() bound to sCaller. Does not write the host cell.
        tVariant EvaluateAtCaller(tCell* sCaller);

        /// @brief If (sAbsRow,sAbsCol) lies in spill buffer, set out and return true.
        tBool TryGetSpillBufferAt(tIndex sAbsRow, tIndex sAbsCol, tVariant& out) const;

        /// @brief Row of the formula definition cell on CstSheetNamed (col 1).
        tIndex DefinitionRow() const { return m_Row; }

        /// @brief Column of the formula definition cell (always 1; name label uses kFormulaNamedNameLabelCol).
        tIndex DefinitionCol() const { return 1; }
    };
    
    
    //! Container of Named range====================================================
	class tRangeNamedContainer : public tClass {
	private:
		//! typdef ===============================================================
		// Reverse lookup: (sheetRef, rangeRef) -> name.
		// Still one-to-one at the pair level; multi-area names simply add
		// several pairs that all point to the same name.
		typedef pair<tAllocatorRef, tAllocatorRef> tPairNameRef;
		typedef map<tPairNameRef, tString> tMapRef;

		// Forward lookup: name -> (sheet, list of range refs on that sheet).
		// Multi-area is allowed on a single sheet only. The vector preserves
		// insertion order so StrRef is stable across round-trips.
		struct tNamedRangeEntry {
			tAllocatorRef m_SheetAllocatorRef;
			vector<tAllocatorRef> m_RangeAllocatorRefs;
		};
		typedef unordered_map<tString, tNamedRangeEntry> tMapName;
        
        typedef map<tPairNameRef, tRangeData*> tMapData;
        
        typedef map<tString, tFormulaNamed*> tMapFormula;

		//! Map for Name
		tMapName			m_MapName;
		//! Map fo Range
        tMapRef				m_MapRef;
        
        //! Map Data Array coupled with m_MapRef
        tMapData            m_MapData;
        
        //! Map Range named formula
        tMapFormula         m_MapFormula;

		tWorkBook*			m_WorkBook;

        tIndex FindFirstFreeFormulaNamedRow() const;
        void ClearFormulaNamedRow(tIndex sRow) const;
	public:
		/// @brief      Constructor.
		tRangeNamedContainer();
		
		/// @brief      Destructor.
		~tRangeNamedContainer();
  
     ///@brief  Clear
        void Clear();

		/// @brief      Set WorkBook.
		/// @param[in]  sWorkBook SkWorkBook
		void Set(tWorkBook* sWorkBook);

		/// @brief      Insert Named Range.
		///
		/// Append semantics: if sName already exists on the same sheet, the
		/// new range is added to the name's multi-area list. If sName already
		/// exists on a different sheet, the previous entry is fully cleared
		/// first (named ranges must stay on a single sheet). Re-inserting an
		/// already-registered (sheet, range) pair under the same name is a
		/// no-op.
		/// @param[in]  sName tString
        /// @param[in]  sSheetAllocatorRef tAllocatorRef
		/// @param[in]  sAllocatorRangeRef tAllocatorRef
		void InsertRangeNamed(tString sName,tAllocatorRef sSheetAllocatorRef, tAllocatorRef sAllocatorRangeRef);
        
        /// @brief      Insert Named Range.
		/// @param[in]  sName tString
        /// @param[in]  sFormula  tString
        /// @param[in]  sComil tBool
        /// @param[in]  sRow tIndex for Undo
        /// @return tBool
        tBool ApplyFormulaNamed(tString sName,tString sFormula,tBool sCompil,tIndex sRow);
    
		/// @brief      Delete Named Range by Ref.
		/// @param[in]  sSheetAllocatorRef tAllocatorRef
		/// @param[in]  sAllocatorRangeRef tAllocatorRef
        /// @return     tBool
		tBool DeleteRangeNamedByRef(tAllocatorRef sSheetAllocatorRef, tAllocatorRef sAllocatorRangeRef);

		/// @brief      Delete Named Range by Name.
		/// @param[in]  sName tString
        /// @return     tBool
		tBool DeleteRangeNamedByName(tString sName);

		/// @brief      Delete Range Formula by Ref.
		/// @param[in]  sSheetAllocatorRef tAllocatorRef
		/// @param[in]  sAllocatorRangeRef tAllocatorRef
		/// @return     tBool
		tBool DeleteRangeDataByRef(tAllocatorRef sSheetAllocatorRef, tAllocatorRef sAllocatorRangeRef);
        
        /// @brief       Delete named formula; optionally clear host cells and recycle row on _$$N.
        /// @param[in]  sName tString
        /// @param[in]  sReleaseHostRow tBool
        /// @return     tBool
        tBool DeleteFormulaNamed(tString sName, tBool sReleaseHostRow = true);
        
		/// @brief      Insert Data Array.
		/// @param[in]  sName tString
        /// @param[in]  sRangeData tRangeData
		/// @param[in]  sSheetAllocatorRef tAllocatorRef
		/// @param[in]  sAllocatorRangeRef tAllocatorRef
        void InsertRangeData(tString sName, tRangeData sRangeData, tAllocatorRef sSheetAllocatorRef, tAllocatorRef sAllocatorRangeRef);
        
        /// @brief      Insert Range Data.
        /// @param[in]  sName tString
        /// @param[in]  sRangeDatatRangeData
        /// @return     tRange*
        tRange* ApplyRangeData(tString sName,tRangeData sRangeData);

        /// @brief Replace RangeData metadata without re-running sort/filter.
        void ReplaceRangeDataMetadata(tString sName, tRangeData sRangeData);
        
        /// @brief      Add New Range Data.
        /// @param[in]  sName tString
        /// @param[in]  sSheetAllocatorRef tAllocatorRef
        /// @param[in]  sAllocatorRangeRef tAllocatorRef
        /// @return     tBool
        tBool AddNewRangeData(tString sName,tAllocatorRef sSheetAllocatorRef, tAllocatorRef sAllocatorRangeRef);
            
		/// @brief      Return Named Range by Ref.
		/// @param[in]  sSheetAllocatorRef tAllocatorRef
		/// @param[in]  sRangeAllocatorRef tAllocatorRef
		/// @return		tRange*
		tRange* Range(tAllocatorRef sSheetAllocatorRef, tAllocatorRef sRangeAllocatorRef);
  
		/// @brief      Return the first Named Range for sName.
		///             For back-compat: callers that want all areas of a
		///             multi-area name should use Ranges(sName) instead.
		/// @param[in]  sName tString
		///	@return		tRange* (first area, or nullptr if not found)
		tRange* Range(tString sName);

		/// @brief      Return all ranges registered under sName (multi-area).
		///             All returned ranges live on the same sheet.
		/// @param[in]  sName tString
		/// @return     std::vector<tRange*> (empty if unknown name)
		std::vector<tRange*> Ranges(tString sName);

		/// @brief      Overlap test: does any name other than sExcludeName
		///             still register (sSheetAllocatorRef, sRangeAllocatorRef)?
		///             Used by delete paths to avoid stripping flags from a
		///             range that is also part of a surviving named range.
		/// @param[in]  sExcludeName tString — name to ignore in the scan
		/// @param[in]  sSheetAllocatorRef tAllocatorRef
		/// @param[in]  sRangeAllocatorRef tAllocatorRef
		/// @return     tBool
		tBool IsRangeSharedByOtherName(tString sExcludeName,
		                               tAllocatorRef sSheetAllocatorRef,
		                               tAllocatorRef sRangeAllocatorRef);

		/// @brief      Snapshot of every registered named range key.
		///             Used by formula rendering to disambiguate overlapping
		///             multi-area names (the reverse map only stores one
		///             owner per range, so ambiguous formulas need a name
		///             scan to find the actual matching sequence).
		/// @return     std::vector<tString>
		std::vector<tString> AllNames();

		/// @brief Names of workbook defined formulas (definedName formula bodies, not cell refs).
		std::vector<tString> AllFormulaNamedNames();

		/// @brief      Return Rang Datae by Name.
		/// @param[in]  sName tString
		///	@return		tRangeData*
		tRangeData* RangeData(tString sName);

		/// @brief      Return Formula by Name.
		/// @param[in]  sName tString
		///	@return		tString
		tString FormulaNamedStr(tString sName);
  
  
        ///@brief return tFormulaNamed
        ///@param[in] sName tString
        tFormulaNamed* FormulaNamed(tString sName);

        /// @brief      Find named formula whose definition cell is sCell (FormulaNamed / CstSheetNamed sheet).
        /// @param[in]  sCell tCell* formula cell for the named formula
        /// @return     tFormulaNamed* or nullptr
        tFormulaNamed* FormulaNamedByCell(tCell* sCell);

        /// @brief      Find named formula whose spill matrix range is sRange (Excel-style name vs _$$!A1:G6 in FormulaStr).
        /// @param[in]  sRange tRange* spill rectangle on the named-formula sheet
        /// @return     tFormulaNamed* or nullptr
        tFormulaNamed* FormulaNamedBySpillRange(tRange* sRange);

        /// @brief Read spill value from in-memory buffer for _$$ cell (abs row/col on that sheet).
        /// @param sOperandRangeTop If >= 0 with sOperandRangeLeft, prefer named formulas whose definition cell
        ///        lies inside the operand rectangle (see height/width); then fall back to unfiltered scan.
        /// @param sOperandRangeLeft See sOperandRangeTop; use -1 for both to skip the filter.
        /// @param sOperandRangeHeight When >0 with sOperandRangeWidth, definition cell must lie inside
        ///        [top, top+h-1] x [left, left+w-1] (e.g. A1:G6 includes definition at column 1).
        /// @param sOperandRangeWidth See sOperandRangeHeight; both <=0 falls back to exact top-left match only.
        tBool TryFormulaNamedSpillBufferAt(tIndex sRow, tIndex sCol, tVariant& out,
                                         tIndex sOperandRangeTop = -1, tIndex sOperandRangeLeft = -1,
                                         tIndex sOperandRangeHeight = -1, tIndex sOperandRangeWidth = -1) const;

		/// @brief      Return Named Range by Ref.
		/// @param[in]  sSheetAllocatorRef tAllocatorRef
		/// @param[in]  sAllocatorRangeRef tAllocatorRef
		/// @return		tString
		tString RangeByRef(tAllocatorRef sSheetAllocatorRef, tAllocatorRef sAllocatorRangeRef);


		/// @brief      Return all Named Ranges IN sheet.
		/// @param[in]  sSheetAllocatorRef tAllocatorRef
		/// @param[out] sSheetAllocatorRef tVectorAllocatorRef 
		void GetRangeNamedsBySheet(tAllocatorRef sSheetAllocatorRef, tVectorAllocatorRef& sRangesAllocatorRef);

        ///@Brief GetRectByKind
		///@param[in] sColumnIndex tIndex Column Index
		///@param[in] sSpecialKind tKind Special Kind
        ///@param[out] sRect tRect&
        ///@retrun tBool
        tBool GetDataRangeRect(tRange* sRange,tIndex sColumnIndex,tKind sSpecialKind);
        

		// Json ===============================================================
		/// @brief		Writer Json. 
		/// @param[in]	sWriter Writer<StringBuffer>*
		/// @param[in]	sFilter Filter which extension types to export (default: Named + Data).
		void Json(Writer<StringBuffer>* sWriter, tExtension sFilter = DefaultJsonRangeFilter());

		/// @brief		Reader Json. 
		/// @param[in]	sValue Value&
		void Json(const Value& sValue);
  
        /// @brief      Compile named formula bodies after JSON load (no path allocation here).
        void JsonCompil();

        /// @brief      Queue named-formula definition cells on the calculation path (call after Json cell Compil so Path()==0 during =name compile).
        /// @param[in]  sContainerPath tContainerPath*
        /// @param[in]  sDeferSheetRecalc ReadJson defer-recalc: skip matrix named Calculation()
        void AddFormulaNamedCellsToPath(tContainerPath* sContainerPath,
                                          tBool sDeferSheetRecalc = false);

		/// @brief		Writer Json Formula Named. 
		/// @param[in]	sWriter Writer<StringBuffer>*
		void JsonFormulaNamed(Writer<StringBuffer>* sWriter);

		/// @brief		Reader Json Formula Named. 
		/// @param[in]	sValue Value&
		void JsonFormulaNamed(const Value& sValue);
        
#ifdef _DEBUGSK
        /// @brief      Debug
        tString Debug();
#endif
	};


}

#endif // SkRangeNamed_hpp

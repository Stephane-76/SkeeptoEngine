//=============================================================================
// SkSpreadSheet Cell
//=============================================================================
#ifndef tCell_hpp
#define tCell_hpp
#include <SkApplication.hpp>
#include "SkItem.hpp"
#include "SkColRow.hpp"
#include "SkFormula.hpp"
#include "SkSharedFormula.hpp"
#include "SkTools.hpp"
#include "SkInterfaceCompil.hpp"
#include "SkStackElem.hpp"
#include <map>
#include <vector>
using namespace SkRoot;

namespace SkSpreadSheet {
    
    class tCellClass;
    class tCellClassAttribute;
    // Class for extend extra value like cell (for array formula output rectangle)
    class tCellExtend : public tClass {
        protected:
            //! Valid flag for array formula output rectangle.
          tString m_Commentary;
        public:
            /// @brief      Constructor for tCellExtend.
            tCellExtend();
            /// @brief      Copy constructor for tCellExtend.
            tCellExtend(const tCellExtend& sCellExtra);
            /// @brief      Destructor for tCellExtend.
            ~tCellExtend();

            /// @brief      Clear tCellExtend.
            void Clear();
            


            // Json (override tClass — array formula output rect under key "afo")
            void Json(rapidjson::Writer<rapidjson::StringBuffer>* sWriter);
            void Json(const rapidjson::Value& sValue);
    };

    //==========================================================================
    //! Cell (Derived of tItem) and InterfaceCompil f(See  SkLemonInterface and SkConditionaelFormat)t
    class  alignas(SkAlign) tCell : public tItem, public tInterfaceCompil {
        protected:
            //! Value of Cell (variant)
            tVariant        m_Value;

            //! Shared formula (formula shared by multiple cells)
            tSharedFormula  m_SharedFormula;
            
            //! For Css    in AllocatorFormat
            tFormatRef      m_Css;
        private:
            //! Vector of reference Cell or Range in formula
            tVectorItem	    m_VectorRef;
            //! Parallel to m_VectorRef: true when PushRef(..., sDependent=true) wired AddDependent.
            //! ByRef functions (ROW/CELL/…) keep VectorRef for evaluation but leave this false so
            //! Check() does not expect an inverse edge in ContainerCellDepend.
            std::vector<tBool> m_VectorRefAddDependent;

            //! To determine the calculation path (See SkCalculationPath)
            tAllocatorRef   m_CalculationPath;

            //! When set (Excel CSE / dynamic array <f ref="...">), matrix spill is clamped to this rect (matches Excel output area).
            tAllocatorRef m_CellExtend;
        public:
            /// @brief      Constructor for tCell.
            tCell();

            /// @brief		Copy constructor for tCell
            tCell(const tCell& sCell);

            /// @brief		Destructor (destroys active union member: Formula or Spill).
            ~tCell();

            /// @brief		Clear all cell data
            void Clear();

            /// @brief		Clear formula and set variant to null (for reset operation), delete dependents
            void ClearFormulaAndVariant();
        
            /// @brief        Clear and delete formula, delete dependents
            void ClearFormula();

            /// Remove cells listed in m_ContainerCellDepend that no longer reference this cell (direct item or range covering it).
            /// Keeps dependency graph consistent after formula/value edits before Sheet::Check() (e.g. UndoCellValue).
            void PruneStaleInverseDependents();

            // Row Col ========================================================
            /// @brief      Return Row (pointer to SkColRow, only the SkColRow knows the line index).
            /// @return     SkColRow*
            tColRow* Row() const;

            /// @brief      Return Row Index (non-const version, required by tInterfaceCompil)
            /// @return     tIndex
            tIndex RowIndex() override;
            
            /// @brief      Return Row Index (const version)
            /// @return     tIndex
            tIndex RowIndex() const;

            /// @brief      Return Col (pointer to SkColRow, only the SkColRow knows the column index).
            /// @return     SkColRow*
            tColRow* Col() const;

            /// @brief      Return Col Index (non-const version, required by tInterfaceCompil)
            /// @return     tIndex
            tIndex ColIndex() override;
            
            /// @brief      Return Col Index (const version)
            /// @return     tIndex
            tIndex ColIndex() const;

            /// @brief      Return path used by MakePath in SkContainerPath.
            /// @return     tAllocatorRef
            tAllocatorRef Path() const;

            /// @brief      Set path used by MakePath in SkContainerPath.
            /// @param[in]  sPath tAllocatorRef 
            void Path(tAllocatorRef sPath);

            /// @brief      Set members after move operation.
            /// @param[in]  sRowAllocatorRef tAllocatorRef (See m_AllocatorRow in tColRowCellRange[sIndiceAllocator])
            /// @param[in]  sColAllocatorRef tAllocatorRef (See m_AllocatorCol in tColRowCellRange[sIndiceAllocator])
            void SetColRow(const tAllocatorRef sRowAllocatorRef, const tAllocatorRef sColAllocatorRef);
  
            /// @brief      Set members after allocation on SkAllocator.
            /// @param[in]  sRef  Reference Sheet or Reference DB
            /// @param[in]  sRowAllocatorRef tAllocatorRef (See m_AllocatorRow in tColRowCellRange[sIndiceAllocator])
            /// @param[in]  sColAllocatorRef tAllocatorRef (See m_AllocatorCol in tColRowCellRange[sIndiceAllocator])
            void Set(const tRefItem sRef, const tAllocatorRef sRowAllocatorRef, const tAllocatorRef sColAllocatorRef);
            
            // Index of row or col
            /// @brief      Return row index (the position in m_AllocatorRow)
            /// @return		tAllocatorRef
            tAllocatorRef RowAllocatorRef() const;

            /// @brief      Return column index (the position in m_AllocatorCol)
            /// @return		tAllocatorRef
            tAllocatorRef ColAllocatorRef() const;

            // Formula
            /// @brief      Set formula
            /// @param[in]  sFormula tFormula*
            void Formula(const tFormula* sFormula) override;
   
            /// @brief      Return Formula
            tFormula* Formula() override;
            
            /// @brief		Return formula pointer (const version)
            /// @return		const tFormula*
            const tFormula* Formula() const;

            /// @brief		Return formula as display string (delegates to tFormula::Str).
            ///             Uses tApplication locale: commas may become list separators (e.g. ; in FR) outside { } and | |;
            ///             floats use locale decimal. Inside constant arrays { } / | |, commas stay column separators.
            ///             For the stored internal key (US-style comma args in FormulaKey), use Formula()->FormulaKey().
            /// @param[in]  sR1C1 tBool
            /// @param[in]  sUser tBool table[[@[#This Row][Col] ]---> [@[Col]]
            /// @return		tString
            const tString FormulaStr(tBool sR1C1 = false, tBool sUser=false) const;

            /// @brief		A1 formula text under Locale("us") for undo/collab/.sker wire (not FormulaKey CR placeholders).
            /// @param[in]  sR1C1 tBool
            /// @param[in]  sUser tBool
            /// @return		tString
            const tString FormulaWire(tBool sR1C1 = false, tBool sUser = false) const;

            // Calcul =========================================================
            /// @brief      Call function with given parameters.
            /// @param[in]  sFormula tFormula* Get Lemon indice 
            /// @param[in]  sItemFormula const tItemFormula* pointer of item formula (function)
            /// @param[in]  sStackElement tStackElems* stack of variant (parameters)
            /// @param[in]  sLetScopes optional active LET/closure scopes (innermost last), used to bind an inline
            ///             lambda's captured outer names when the call resolves to a named/closure LAMBDA.
            /// @return		const tVariant
            const tStackElem CallFunction(tFormula* sFormula,
                                    const tItemFormula* sItemFormula,
                                    tStackElems* sStackElem,
                                    const std::vector<std::map<tString, tStackElem>>* sLetScopes = nullptr);

            /// @brief      Try to evaluate a call NAME(args) as a user-defined named LAMBDA.
            /// @param[in]  sName uppercased function name pulled from the bytecode
            /// @param[in]  sItemFormula function item (carries argument count in Extra())
            /// @param[in]  sStackElem evaluation stack (arguments are on top, popped on success)
            /// @param[out] sResult the lambda result (valid only when the call returns true)
            /// @return		tBool true if sName is a named LAMBDA and was evaluated (sResult set), else false
            ///             (arguments are left untouched so the caller can raise #NAME?).
            /// @param[in]  sLetScopes optional active LET/closure scopes (innermost last): a closure lambda's
            ///             captured outer names are looked up here and bound before the call.
            tBool CallNamedLambda(const tString& sName,
                                    const tItemFormula* sItemFormula,
                                    tStackElems* sStackElem,
                                    tStackElem& sResult,
                                    const std::vector<std::map<tString, tStackElem>>* sLetScopes = nullptr);
            
            /// @brief      Internal calculation to compute cell value.
            ///@param[in]   sFormula tFormula* (Condtitional  Format, else nullptr)
            ///@param[in]   sInjectedScope optional initial local scope (LAMBDA parameters -> arguments), seeded
            ///             as the outermost LET scope so parameter references (LetVarRef) resolve to their values.
            /// @return	    const tVariant
            const tVariant InternalCalculation(tFormula* sFormula=nullptr,
                                    const std::map<tString, tStackElem>* sInjectedScope=nullptr);

            /// @brief      Calculate cell and dependent cells.
            void Calculation();
            
            /// @brief      Return cell reference string like A1.
            /// @param[in]  sSheetName tBool
            /// @return		tString
            const tString StrRef(tBool sSheetName=false) const;

            // Value ==========================================================
            /// @brief      Set value.
            /// @param[in]  sValue tVariant
            void Value(const tVariant& sValue);

            /// @brief      Return cell value.
            /// @return		tVariant& 
            tVariant& Value();

            /// @brief      Return const cell value.
            /// @return		const tVariant& 
            const tVariant& Value() const;

            /// @brief      Return pointer to value (for optimization, no copy).
            /// @return		tVariant* 
            tVariant* PtValue();
            
            /// @brief      Return const pointer to value (for optimization, no copy).
            /// @return		const tVariant* 
            const tVariant* PtValue() const;
        
            /// @brief      Return calculable value.
            /// @return     tVariant&
            tVariant& CalculableValue();
            
            /// @brief      Return const calculable value.
            /// @return     const tVariant&
            const tVariant& CalculableValue() const;

            /// @brief      Scalar for reads (unwraps tCellUnit magnitude); formulas keep CalculableValue().
            static tVariant CalculableScalarFromVariant(const tVariant& sCalculable);

            /// @brief      CalculableScalarFromVariant(CalculableValue()).
            tVariant CalculableScalarValue() const;
        
            /// @brief      Return pointer to Class
            /// @return     tCellClass*
            tCellClass*     Class() const;

            /// @brief      Return pointer to ClassAttribute.
            /// @return		tCellClassAttribute*
            tCellClassAttribute* ClassAttribute() const;

            /// @brief      Return pointer to tCellAttribute.
            /// @param[in]  sName tString
            /// @return		tCellAttribute*
            tCellAttribute* CellAttribute(tString sName) const;

            // Css ============================================================
            /// @brief		Return CSS format reference
            /// @return		tFormatRef
            tFormatRef  Css() const;

            /// @brief		Set CSS format value
            /// @param[in] sFormatRef tFormatRef
            void Css(tFormatRef sFormatRef);

            /// @brief     Check if cell covers other cell (see JsonView)
            /// @return    tBool
            tBool Cover() const;
            
            // Matix ==========================================================
            /// @brief      Matrix / spill extent for this cell (origin or extend). For MatOrigin, prefers named-formula spill,
            ///             then RangeSpill() (OOXML ref), so overlapping CF/named rects do not win via arbitrary FindRanges() order.
            /// @return     tRange* or nullptr
            tRange* MatrixRange();

            /// @brief      If this cell is a spill cell (part of a matrix result), return the cell that contains the formula (SpillOrigin). Otherwise return nullptr.
            ///             Use when you need to find the formula cell from any cell in the spill range.
            /// @return     tCell* the origin/formula cell, or nullptr if this cell is not a spill cell (m_ArrayFormulaTag != SpillCell)
            tCell* CellMatrixRoot();

            /// @brief      Clear matrix origin and extend.
            void ClearMatrix();

            /// @brief      Excel: user typed/pasted into a spilled (MatExtend) cell — clear the spill footprint
            ///             (keep the origin formula) so a subsequent Calculation on the origin yields #SPILL!.
            /// @return     Spill origin to recalculate, or nullptr if this cell was not a spill slave.
            tCell* BreakSpillForUserOverwrite();

            /// @brief      Set every cell value in the spill rectangle to null; ClearFormula on extend cells only.
            ///             Clears named-formula spill buffer when present. Does not remove MatOrigin/MatExtend — call ClearMatrix after if needed.
            void ClearMatrixSpillValues();

            /// @brief      Set OOXML array/dynamic formula output bounds (one contiguous block); used to clamp matrix spill.
            void SetSpillRect(tIndex sTop, tIndex sLeft, tIndex sBottom, tIndex sRight);
            
            ///@brief Retunr tRangeSpill
            ///@return tRange*
            tRange* SpillRange() const;

            /// @brief      Clear matrix spill cells outside ArrayFormulaOutputRect (after import or after RecalculateAll).
            void PruneMatrixSpillOutsideArrayFormulaOutput();
            
            /// @brief      Clear array-formula output bounds (e.g. after formula removal).
            void ClearArrayFormulaOutputRect();
            

            /// @brief      OOXML / .sker spill output rect (top-left cell only gets non-empty). Uses RangeSpill(); do not gate on
            ///             IsSpillRange() on the cell — that flag can be cleared while the spill tRange still resolves (see ClampResultRectToArrayFormulaOutput).
            tTempoRect ArrayFormulaOutputRect() const;
        
    
            void AppendCellsInSpillRange(std::vector<tCell*>& sOut);
         
            // Formula ========================================================
            /// @brief      Push item formula on vector of reference and add dependent.
            /// @param[in]  sItem tItem*
            ///@param[in]  sDependent tBool
            void PushRef(tItem* sItem,tBool sDependent=true) override;
            
            /// @brief Return Vector Item
            /// @return tVectorItem*
            tVectorItem* VectorItem() override;

            /// @brief      Return Sheet (override interface)
            /// @return     tSheet*
            /// @brief      Return Sheet (non-const version, required by tInterfaceCompil)
            /// @return     tSheet*
            tSheet* Sheet() override;
            
            /// @brief      Return Sheet (const version)
            /// @return     tSheet*
            tSheet* Sheet() const;

            /// @brief      Get item formula from vector of reference.
            /// @param[in]  sIndice tIndex
            /// @return		tItem*
            ///*inline*/ tItem* Ref(tIndex sIndice) const { return m_VectorRef[sIndice]; }
            SkInline tItem* Ref(tIndex sIndice) const {
               if (sIndice >= 0 && sIndice < static_cast<tIndex>(m_VectorRef.size())) {
                    return m_VectorRef[static_cast<size_t>(sIndice)];
                } else {
                    cerr << "Bad Index ---" << endl;
                }
                return nullptr;
            }
            /// @brief      Clear vector reference.
            void ClearVectorRefAndDeleteDependant() override;

            /// @brief      Add dependent cell.
            void AddDependant();

            /// @brief      Return vector of references.
            /// @return		SkVariant 
            tVectorItem* VectorRef();
            
            /// @brief      Return const vector of references.
            /// @return		const SkVariant* 
            const tVectorItem* VectorRef() const;

            /// @brief      Swap m_VectorRef (+ AddDependent flags) with sOther in place. Used by
            ///             tConditionalFormat::CallBackCell to evaluate a CF
            ///             formula whose cell refs live on the CF object,
            ///             without ever inserting them in the cell's own
            ///             dependency graph (PushRef is what wires the graph,
            ///             and we deliberately don't call it here).
            /// @param[in,out] sOther tVectorItem& foreign vector to swap with.
            /// @param[in,out] sOtherAddDependent parallel flags for sOther (same size).
            void SwapVectorRef(tVectorItem& sOther, std::vector<tBool>& sOtherAddDependent);

            // Merged =========================================================
            /// @brief       Return merged range.
            /// @return     tRange*
            tRange* MergedRange() const;
            
            // Json ===========================================================
            /// @brief		Write JSON representation of cell. 
            /// @param[in]	sWriter Writer<StringBuffer>*
            /// @param[in]	sValue R1C1 for paste
            /// @param{in]	tPoint* sDiff (for copy diff)
            virtual void Json(Writer<StringBuffer>* sWriter, tBool sR1C1 = false, tPoint* sDiff = nullptr);

            /// @brief		Read JSON representation of cell. 
            /// @param[in]	sValue Value&
            virtual void Json(const rapidjson::Value& sValue);
            
            /// @brief      Check if cell is empty.
            /// @return		tBool
            tBool IsEmpty() const;

            /// @brief      Check if cell value is empty.
            /// @return     tBool
            tBool IsValueEmpty() const;
            
            /// @brief      Check if cell has dependents.
            /// @return		tBool
            tBool HasDependent() const;

            /// @brief      Assignment operator.
            /// @return		tCell&  
            //tCell&  operator = (const tCell& sCell);

            /// @brief      Equality operator.
            /// @return		tBool 
            tBool  operator == (const tCell& sCell) const;

            /// @brief      Return debug information string.
            /// @return		tString 
            tString Debug() const;

#ifdef checksp
            /// @brief      Check cell integrity.
            void Check();
#endif
            /// @brief      Display offset information for members.
            void Offsetof() const {
#ifndef __EMSCRIPTEN__
#ifndef __APPLE__
#ifndef __GNUC__
                cout << "tCell offset................." << endl;
                cout << " m_Value " << offsetof(tCell, m_Value) << " :" << sizeof(tVariant) << endl;
                cout << " m_VectorFormula " << offsetof(tCell, m_VectorRef) << " :" << sizeof(tVectorItem) << endl;
                cout << " m_Css " << offsetof(tCell, m_Css) << " :" << sizeof(tAllocatorRef) << endl;
                cout << " m_Path " << offsetof(tCell, m_CalculationPath) << " :" << sizeof(tAllocatorRef) << endl;
#endif
#endif
#endif
            }

 
    };

    //! Stack vector cell =====================================================
    typedef stack<tCell*> tStackCell;
    typedef vector<tCell*> tVectorCell;

} // End of namespace
#endif


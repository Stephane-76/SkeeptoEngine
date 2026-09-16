//=============================================================================
// SkSpreadSheet StackElem
//=============================================================================
#ifndef SkStackElem_hpp
#define SkStackElem_hpp

#include <SkApplication.hpp>
#include <SkVariant.hpp>
#include <vector>
#include <map>
#include "SkTools.hpp"

namespace SkSpreadSheet {
        class tCell;
        class tRange;
        //! Type of item Cell or Range . ======================================
        enum class tTypeItem : tByte { t_Cell, t_Range, t_Attribute };

        //! Extension bits for Range (16-bit bitset; indices 0..15)
        typedef tBitSet<tUShort> tExtension;
        // Extension bit indices: 0..10 used (incl. conditional format 6..10); 11..15 future use.so
        const tUShort t_Named = 0; // Named range
        const tUShort t_Merged = 1; // Merged range
        const tUShort t_Data = 2; // Data range
        const tUShort t_MatOrigin = 3;  // Matrix Origin Cell or Range
        const tUShort t_MatExtend = 4;  // Matrix Cell
        const tUShort t_SpillRange =5 ; // Matrix Spill Range
        
        // Conditional Format
        const tUShort t_CFHR = 6;  // HighLightRule
        const tUShort t_CFCS = 7;  // ColorScale
        const tUShort t_CFDB = 8;  // DataBar
        const tUShort t_CFIS = 9;  // IconSet
        const tUShort t_CFCF = 10;  // CustomFormat

        // Engine-native dynamic matrix spill (e.g. =A1:B2+C1:D2). Marks a SpillRange AFO that must be re-derived
        // on each recompute (so a resized matrix is not clamped to its previous footprint). OOXML array-formula
        // refs (SetSpillRect at import) never set this bit, so their authoritative ref is preserved by the clamp.
        const tUShort t_MatDynamic = 11;

        // Array formula (spill): discriminator for union in tCell
        enum class tArrayFormulaTag : tByte { None, SpillOrigin, SpillCell };
        struct tSpillExtent { tIndex Rows; tIndex Cols; };
    
        enum class tStackType : tChar { t_Variant,t_Range,t_Cell,t_Attribute,t_Array,t_Lambda,t_None };

        // First-class LAMBDA value carried on the evaluation stack. It references a compiled named LAMBDA
        // (tFormulaNamed on the _$$ sheet): a non-owning pointer, so copies are shallow and no deletion occurs.
        // Used to pass a lambda as an argument to higher-order functions (MAP/REDUCE/SCAN, ...).
        class tFormulaNamed;

        //=====================================================================
        //! In-memory dynamic-array value carried on the evaluation stack (Excel
        //! spill result, e.g. SEQUENCE / SORT). Transient only: it lets functions
        //! compose (SORT(SEQUENCE(...))) without materializing intermediate cells.
        //! It is never stored in a cell nor serialized; the final result is spilled
        //! to the grid by tCell::InternalCalculation.
        //=====================================================================
        struct tArrayValue {
            //! Number of rows.
            tIndex m_Rows;
            //! Number of columns.
            tIndex m_Cols;
            //! Row-major storage (size == m_Rows * m_Cols).
            std::vector<tVariant> m_Values;

            tArrayValue() : m_Rows(0), m_Cols(0), m_Values() {}
            tArrayValue(tIndex sRows, tIndex sCols)
                : m_Rows(sRows), m_Cols(sCols),
                  m_Values(static_cast<size_t>(sRows) * static_cast<size_t>(sCols)) {}

            //! Total element count.
            tIndex Count() const { return(m_Rows * m_Cols); }

            //! Value at (row, col), read-only.
            const tVariant& At(tIndex sRow, tIndex sCol) const {
                return(m_Values[static_cast<size_t>(sRow) * static_cast<size_t>(m_Cols) + static_cast<size_t>(sCol)]);
            }
            //! Value at (row, col), mutable.
            tVariant& At(tIndex sRow, tIndex sCol) {
                return(m_Values[static_cast<size_t>(sRow) * static_cast<size_t>(m_Cols) + static_cast<size_t>(sCol)]);
            }
        };

        //=====================================================================
        //! Elem fCalcul on stack
        class tStackElem : public tClass {
            private:
                //! Type of argument (Variant or range).
                tStackType  m_Type;
                // Union to store either Cell, Range, Variant, or Array pointer
                union {
                    tCell*        m_Cell;
                    tRange*       m_Range;
                    tVariant*     m_Variant;
                    tArrayValue*  m_Array;
                    tFormulaNamed* m_Lambda;
                };
                //! Closure environment for a first-class LAMBDA value (t_Lambda only): outer names captured at the
                //! point the lambda value is created (LET vars / enclosing lambda parameters), keyed uppercased.
                //! Owned (deep-copied on copy, freed on destruction); nullptr for every non-closure elem.
                std::map<tString, tStackElem>* m_CapturedScope = nullptr;
           public:
                /// @brief		Constructor
                tStackElem();
    
                /// @brief		Constructor SkArg with variant.
                /// @param[in]  sVariant tVariant (passed by const reference to avoid copy)
                tStackElem(const tVariant& sVariant);
    
                /// @brief		Constructor SkArg with range.
                /// @param[in]  sRange tRange*
                tStackElem(tRange* sRange);
    
                /// @brief		Constructor SkArg with cell.
                /// @param[in]  sCell tCell*
                tStackElem(tCell* sCell);

                /// @brief		Constructor with an in-memory array value (takes ownership; deep-copied on copy).
                /// @param[in]  sArray tArrayValue*
                tStackElem(tArrayValue* sArray);

                /// @brief		Constructor with a first-class LAMBDA value (non-owning reference).
                /// @param[in]  sLambda tFormulaNamed*
                tStackElem(tFormulaNamed* sLambda);

                /// @brief		Constructor with a first-class LAMBDA value carrying a captured closure scope.
                /// @param[in]  sLambda tFormulaNamed* (non-owning reference)
                /// @param[in]  sCaptured captured outer names -> values (deep-copied and owned by this elem)
                tStackElem(tFormulaNamed* sLambda, const std::map<tString, tStackElem>& sCaptured);
                
                /// @brief		Copy constructor.
                /// @param[in]  sOther const tStackElem&
                tStackElem(const tStackElem& sOther);
    
                /// @brief		Destructor.
                ~tStackElem();
    
                /// @brief		Assignment operator.
                /// @param[in]  sOther const tStackElem&
                /// @return		tStackElem&
                tStackElem& operator=(const tStackElem& sOther);
    
                /// @brief		Placement new operator.
                /// @param[in]  sSize size_t
                /// @param[in]  sPtr void*
                /// @return		void*
                void* operator new(size_t /*sSize*/, void* sPtr) noexcept {
                    return sPtr;
                }
    
                /// @brief		Standard new operator.
                /// @param[in]  sSize size_t
                /// @return		void*
                void* operator new(size_t sSize) {
                    return(tApplication::Instance()->AllocTemporary(sSize));
                }
    
                /// @brief		Delete operator.
                /// @param[in]  sPtr void*
                void operator delete(void* /*sPtr*/) noexcept {}
    
                /// @brief		Return type.
                /// @return		SkArgType
                tStackType Type() const;
    
                /// @brief		Return variant.
                /// @return		SkVariant
                tVariant Variant() const;
    
                /// @brief		Return range.
                /// @return		tRange*
                tRange* Range() const;
    
                /// @brief		Return cell.
                /// @return		tCell*
                tCell* Cell() const;

                /// @brief		Return the in-memory array value (nullptr unless type is t_Array).
                /// @return		tArrayValue*
                tArrayValue* Array() const;

                /// @brief		Return the referenced LAMBDA (nullptr unless type is t_Lambda).
                /// @return		tFormulaNamed*
                tFormulaNamed* Lambda() const;

                /// @brief		Return the captured closure scope (nullptr unless this is a closure lambda value).
                /// @return		const std::map<tString, tStackElem>*
                const std::map<tString, tStackElem>* CapturedScope() const;
       
                ///@brief rerturn value of StackElem
                ///@return tVariant;
                const tVariant Value() const;
#ifdef _DEBUGSK
                /// @brief		Return debug.
                /// @return		tString
                tString Debug() const;
#endif
    
    
                operator const tVariant();
        };
        typedef stack<tStackElem> tStackElems;
        typedef vector<tStackElem> tStackElemVector;
    
}

#endif

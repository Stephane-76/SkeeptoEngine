//=============================================================================
// SkSpreadSheet Range
//=============================================================================
#ifndef tRange_hpp
#define tRange_hpp

#include "SkColRow.hpp"
#include "SkItem.hpp"
#include "SkSelect.hpp"

using namespace SkRoot;

namespace SkSpreadSheet {
	
	//=========================================================================
	//! Range (Derived of tItem) (pointed by formula)
	class alignas(SkAlign) tRange : public tItem {
	private:
		//! Row bottom Index on AllocatorRow on tColRowCellRange
		tAllocatorRef m_BottomAllocatorRef;	
		//! Col right Index on AllocatorCol on tColRowCellRange
		tAllocatorRef m_RightAllocatorRef;
		//! for delete range  of allocator by allocator Recover Range has no key
		tAllocatorRef  m_AllocatorRef;
	public:
		/// @brief      constructor.
		tRange();

		/// @brief      Clear.
		void Clear();

		// Row Col 
		/// @brief      Return Top SkCoRow.
		/// @return		tInt Index
		tColRow* Top();
		
		/// @brief      Return const Top SkCoRow.
		/// @return		const tColRow*
		const tColRow* Top() const;

		/// @brief      Return Left SkCoRow.
		/// @return		tInt Index
		tColRow* Left();
		
		/// @brief      Return const Left SkCoRow.
		/// @return		const tColRow*
		const tColRow* Left() const;

		/// @brief      Return Bottom SkCoRow.
		/// @return		tInt Index
		tColRow* Bottom();
		
		/// @brief      Return const Bottom SkCoRow.
		/// @return		const tColRow*
		const tColRow* Bottom() const;

		/// @brief      Return Right SkCoRow.
		/// @return		tInt Index
		tColRow* Right();
		
		/// @brief      Return const Right SkCoRow.
		/// @return		const tColRow*
		const tColRow* Right() const;

		/// @brief      Return Top Index.
		/// @return		tInt Index
        tIndex TopIndex();

		/// @brief      Return Left Index.
		/// @return		tInt Index
        tIndex LeftIndex();

		/// @brief      Return Bottom Index.
		/// @return		tInt Index
        tIndex BottomIndex();

		/// @brief      Return Right Index.
		/// @return		tInt Index
        tIndex RightIndex();

        /// @brief      Bottom row for cell iteration (min of logical bottom and sheet LastRow()).
        ///             Logical BottomIndex() / StrRef() / ROWS() are unchanged.
        tIndex IterateBottom();

        /// @brief      Right column for cell iteration (min of logical right and sheet LastCol()).
        tIndex IterateRight();

        /// @brief      Bottom row for ColRow range registration (full-column rows only).
        tIndex AttachBottomBound();

        /// @brief      Right column for ColRow range registration (full-row cols only).
        tIndex AttachRightBound();

		/// @brief      Return index allocator (See m_AllocatorRange in tColRowCellRange[m_IndiceAllocator]).
		/// @return		tInt Index allocator
		tAllocatorRef AllocatorRef();
     
		/// @brief      Set index allocator (See m_AllocatorRange in tColRowCellRange[m_IndiceAllocator]).
		/// @param[in]  sIndexAllocator tInt 
		void  AllocatorRef(tAllocatorRef sAllocatorRef);
        
		/// @brief      Set members after alloc on SkAllocator.
		/// @param[in]  sRef  tInt (SeetRefItem pass indice Sheet or Db Ref)
		/// @param[in]  sTopAllocatorRef tInt (See m_AllocatorRow in tColRowCellRange[sIndiceAllocator])
		/// @param[in]  sLeftAllocatorRef tInt (See m_AllocatorCol in tColRowCellRange[sIndiceAllocator])
		/// @param[in]  sBottomAllocatorRef tInt (See m_AllocatorRow in tColRowCellRange[sIndiceAllocator])
		/// @param[in]  sRightAllocatorRef tInt (See m_AllocatorCol in tColRowCellRange[sIndiceAllocator])
		void Set(const tRefItem sRef, const tAllocatorRef sTopAllocatorRef, const tAllocatorRef sLeftAllocatorRef, const tAllocatorRef sBottomAllocatorRef, const tAllocatorRef sRightAllocatorRef);

		/// @brief      Return Reference of cell like A1:A2.
        /// @param[in]  sSheetName tBool
        /// @return        tString
        const tString StrRef(tBool sSheetName=false) const;

		/// @brief      Return TempoRect.
		/// @return		const tTempoRect
		const tTempoRect Rect();

		/// @brief      Return RangeNamed.
		/// @return		const tString
		const tString Name();
  
        ///@brief IsCell  Return true
        ///@return tBool
        tBool IsCell();
        
        ///@brief return Cell
        ///@return tBool
        tCell* Cell();
        
        ///@brief Ensure  Cell
        ///@return tBool
        tCell* EnsureCell();
        
		/// @brief      Return true if this range recover cell
		/// @param[in]  sRow tInt index of row
		/// @param[in]  sCol tInt index of col
		/// @return		tBool
		tBool CoveredCell(tInt sRow, tInt sCol);

		  /// @brief      Intersect rect
        /// @param[in]  sRect tRect*
        /// @return     tBool
        tBool IntersectRect(tRect* sRect);

        // @brief Enclose Rectangle: return true if sRect is fully inside this range
		/// @param[in]  sRect tRect*
		/// @return     tBool
		tBool EncloseRect(tRect* sRect);

        // @brief Inside Rectangle: return true if this range is fully inside sRect
        /// @param[in]  sRect tRect*
        /// @return     tBool
        tBool InsideRect(tRect* sRect);


        /// @brief      Is Cell Empty.
        /// @return     tBool
        tBool IsEmpty();

		/// @brief      Modify Formula dependent of Cell
		/// @return		tBool
		void ClearDependent();
      
        // Json ===============================================================
        // Just for named or merged range
        /// @brief        Writer Json.
        /// @param[in]    sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);

        // Not reader for Json
        
		/// @brief      Operator < for comparaison with tComparatorRange.
		/// @param[in]  sRange tRange
		/// @return		tBool
		tBool operator < (tRange& sRange);
		
		/// @brief      Operator ==.
        /// @param[in]  sRange tRange
        /// @return     Bool
		tBool operator == (tRange& sRange);

#ifdef checksp
		/// @brief      Check.
		void Check();
#endif


		/// @brief      Debug on console.
		tString Debug();

		/// @brief      Offset of member for information.
		void Offsetof() {
#ifndef __EMSCRIPTEN__
#ifndef __APPLE__
#ifndef __GNUC__
			cout << "tRange offset................" << endl;
			cout << " m_BottomAllocatorRef " << offsetof(tRange, m_BottomAllocatorRef) << " :" << sizeof(tAllocatorRef) << endl;
			cout << " m_RightAllocatorRef " << offsetof(tRange, m_RightAllocatorRef) << " :" << sizeof(tAllocatorRef) << endl;
			cout << " m_AllocatorRef " << offsetof(tRange, m_AllocatorRef) << " :" << sizeof(tAllocatorRef) << endl;
#endif
#endif
#endif
		}

	};

	//=========================================================================
	//! Sorted range by operator < on tRange
	class tComparatorRange {
	public:
		/// @brief      Operator() compare with tRange < operator.
		/// @param[in]  sE2 tRange*
		/// @param[in]  sE1 tRange*
		tBool SkInline operator()(tRange* sE1, tRange* sE2) {
			return(*sE1 < *sE2);
		}
	};

	//! Stack vector range ====================================================
	typedef stack<tRange*> tStackRange;
	typedef vector<tRange*> tVectorRange;



} // End of namespace

#endif

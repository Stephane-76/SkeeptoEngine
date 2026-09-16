//=============================================================================
// SkSpreadSheet Item
//=============================================================================
#ifndef SkItem_hpp
#define SkItem_hpp

#include <SkApplication.hpp>
#include "SkTools.hpp"
#include "SkColRow.hpp"

using namespace SkRoot;

namespace SkSpreadSheet {

	class tCell;
	class tCellAttribute;
	class tTempoRect;
	class tSheet;
	class tWorkBook;
	class tColRowCellRange;
	class tPath;

	//=========================================================================
	//! Ancestor Class of tCell and tRange, not virtual Class 
	class alignas(SkAlign) tItem : public tClass {
	protected:
		//! type Cell,  range or attribute
		tTypeItem		m_Type;
        // for extend value
		tExtension		m_Extension;
		//! Allocator indice for SkTableAllocatorTable (Sheet) 
		tAllocatorRef	m_ColRowCellRangeRef;
		//! Row Index on AllocatorRow on tColRowCellRange (See m_AllocatorRow in tColRowCellRange[m_IndiceAllocator])
		tAllocatorRef	m_RowAllocatorRef;
		//! Col Index on AllocatorCol on tColRowCellRange (See m_AllocatorCol in tColRowCellRange[m_IndiceAllocator])
		tAllocatorRef	m_ColAllocatorRef;							
	public:
		typedef  tClassVectorContainer<tCell> tContainerCell;
	protected:
		//! pointer of Cell depend (or cell formula for range) Cell unique and ordered by pointer
		tContainerCell m_ContainerCellDepend;
	public:
		/// @brief		Constructor of tItem
		tItem();
		/// @brief		Constructor Copy of tCell
		tItem(const tItem& sItem);
		/// @brief      Set path used by MakePath in SkContainerPath.
		/// @param[in]  sType tTypeItem t_Cell or t_Range
		/// @param[in]  sColRowCellRangeRef  tAllocatorRef
		/// @param[in]  sRowIndex tAllocatorRef (See m_AllocatorRow in tColRowCellRange[sIndiceAllocator])
		/// @param[in]  sColIndex tAllocatorRef (See m_AllocatorCol in tColRowCellRange[sIndiceAllocator])
		tItem(tTypeItem sType, tAllocatorRef sColRowCellRangeRef, tAllocatorRef sRowAllocatorRef, tAllocatorRef sColAllocatorRef);

		~tItem();

		/// @brief      Clear.
		void Clear();


		// Extension ===========================================================
		/// @brief      Return Extension.
		/// @return     tExtension
		tExtension Extension();

		/// @brief      Set Extension.
		/// @param[in]  sExtension tExtension
		void Extension(tExtension sExtension);


        /// @brief      Set type of item t_Cell, t_Attribute or t_Range.
        /// @return     tTypeItem
        SkInline void  Type(tTypeItem sTypeItem) {
            m_Type = sTypeItem;
        }

        /// @brief      Return type of item t_Cell or t_Range.
        /// @return     tTypeItem
       SkInline tTypeItem Type() const {
            return(m_Type);
        }
		/// @brief      Return if range is Named.
		/// @return     tBool;	
		tBool IsNamed() const;
		
		/// @brief      Set Named.
		void SetNamed();

		/// @brief      Remove flag Range .
		void RemoveNamed();

		/// @brief      Return if range is Merged.
		/// @return     tBool;	
		tBool IsMerged() const;

		/// @brief      Set Merged.
		void SetMerged();

		/// @brief      Remove flag Merged.
		void RemoveMerged();
        
        /// @brief      Return if range is Data.
        /// @return     tBool;
        tBool IsData();

        /// @brief      SetData.
        void SetData();

        /// @brief      Remove flag Data.
        void RemoveData();

        // Matrix Origin ==============================================
		/// @brief      Return if range is Matrix Origin.
		/// @return     tBool;
		tBool IsMatOrigin() const;
		/// @brief      Set Matrix Origin.
		void SetMatOrigin();
		/// @brief      Remove flag Matrix Origin.
		void RemoveMatOrigin();

		// Matrix Cell Extend =========================================
		/// @brief      Return if range is Matrix Cell.
		/// @return     tBool;
		tBool IsMatExtend() const;
		/// @brief      Set Matrix Cell.
		void SetMatExtend();
		/// @brief      Remove flag Matrix Cell.
		void RemoveMatExtend();

		// Matrix Spill Range =========================================
		/// @brief      Return if range is Matrix Spill Range.
		/// @return     tBool;
		tBool IsSpillRange() const;
		/// @brief      Set Matrix Spill Range.
		void SetSpillRange();
		/// @brief      Remove flag Matrix Spill Range.
		void RemoveSpillRange();

		// Native dynamic matrix ======================================
		/// @brief      Return if the spill AFO is an engine-native dynamic matrix (must re-derive, never OOXML-clamped).
		/// @return     tBool;
		tBool IsMatDynamic() const;
		/// @brief      Mark the spill AFO as an engine-native dynamic matrix.
		void SetMatDynamic();
		/// @brief      Remove the native dynamic matrix flag.
		void RemoveMatDynamic();

        /// @brief      Return if range is Conditional Format.
        /// @return     tBool;
        tBool IsConditionalFormat() const;
        
        ///@brief       Remove All ConditionalFormat
        void RemoveConditionalFormat();

		// Conditional Format HighlightCellsRule =====================================
		/// @brief      Return if range is in Conditional Format HighlightCellsRule
		/// @return     tBool;
		tBool IsCFHR() const;

		/// @brief      Set In Conditional Format HighlightCellsRule.
		void SetCFHR();

		/// @brief      Remove flag In Conditional Format HighlightCellsRule.
		void RemoveCFHR();

		// Conditional Format DataBar =====================================
		/// @brief      Remove flag In Conditional Format DataBar.
		/// @return     tBool;
		tBool IsCFDB() const;

		/// @brief      Set In Conditional Format DataBar.
		void SetCFDB();

		/// @brief      Remove flag In Conditional Format DataBar.
		void RemoveCFDB();

		// Conditional Format ColorScale ===========================================
		/// @brief      Return if range is in Conditional Format ColorScale.
		/// @return     tBool;
		tBool IsCFCS() const;
		/// @brief      Set In Conditional Format ColorScale.
		void SetCFCS();
		/// @brief      Remove flag In Conditional Format ColorScale.
		void RemoveCFCS();

		// Conditional Format IconSet ==============================================
		/// @brief      Return if range is in Conditional Format IconSet.
		/// @return     tBool;
		tBool IsCFIS() const;
		/// @brief      Set In Conditional Format IconSet.
		void SetCFIS();
		/// @brief      Remove flag In Conditional Format IconSet.
		void RemoveCFIS();

		// Condidtional Format CustomFormat =========================================
		/// @brief      Return if range is in Conditional Format CustomFormat.
		/// @return     tBool;
		tBool IsCFCF() const;
		/// @brief      Set In Conditional Format CustomFormat.
		void SetCFCF();
		/// @brief      Remove flag In Conditional Format CustomFormat.
		void RemoveCFCF();

	        
		// Row and Col Allocator Ref ==============================================
        /// @brief      Return Row Allocator Ref.
        /// @return     tAllocatorRef
        tAllocatorRef RowAllocatorRef();
        /// @brief      Return Col Allocator Ref.
        /// @return     tAllocatorRef

        tAllocatorRef ColAllocatorRef();

		/// @brief      Return IndexAllocatorCellRange.
		/// @return     tAllocatorRef
		tAllocatorRef ColRowCellRangeRef();
		/// @brief      Return owner tColRowCellRange.
		/// @return     tColRowCellRange*
		tColRowCellRange* ColRowCellRange() const;
				
		// Polymorphism static ===========================================
		/// @brief      Return Cell (Polymorphism static tItem is not virtual).
		/// @return     tCellAttribute*
		tCellAttribute* CellAttribute();
		
		/// @brief      Return const Cell (Polymorphism static tItem is not virtual).
		/// @return     const tCellAttribute*
		const tCellAttribute* CellAttribute() const;

		/// @brief      Return Cell (Polymorphism static tItem is not virtual).
		/// @return     tCell* 
		tCell* Cell();
		
		/// @brief      Return const Cell (Polymorphism static tItem is not virtual).
		/// @return     const tCell* 
		const tCell* Cell() const;

		/// @brief      Return Range (Polymorphism static tItem is not virtual).
		/// @return     tRange* 
		tRange* Range();
		
		/// @brief      Return const Range (Polymorphism static tItem is not virtual).
		/// @return     const tRange* 
		const tRange* Range() const;
        
		// Sheet & WorkBook ==============================================
		/// @brief      Return owner Sheet.
		/// @return     tSheet* 
		tSheet* Sheet() const;

		/// @brief      Return owner WorkBook.
		/// @return     tWorkBook* 
		tWorkBook* WorkBook();

		/// @brief      Copy col row and indice of ColRowCellRange to attribute.
		/// @param[in]  sItem tItem*
		void Rooted(tItem* sItem);

		/// @brief      Return Reference of cell like A1.
        /// @param[in]  sSheetName tBool
		/// @return		tString
		const tString StrRef(tBool sSheetName=false) const;

		// CellDependent =================================================
		/// @brief      Add cell dependent (this parameter cell formula use this cell). 
		/// @param[in]  sCell tCell*
		void AddDependent(tCell* sCell);

		/// @brief      Delete cell dependent (this parameter cell formula use this cell). 
		/// @param[in]  sItem tItem*
		void DeleteDependent(tCell* sCell);

		/// @brief      Return container of cell dependent.
		/// @return     tClassContainer<tItem>* 
		tContainerCell* ContainerCellDepend();
		
		/// @brief      Return const container of cell dependent.
		/// @return     const tClassContainer<tItem>* 
		const tContainerCell* ContainerCellDepend() const;

		/// @brief      Return true, if one item depend of this item.
		/// @return     Sk
		tBool NotDependent();
				
		/// @brief      Method delete Call by SkAllocator.
		void Delete();

		/// @brief      Debug.
		tString Debug();

		/// @brief      Offset of member for information.
		void Offsetof() {
/* NOT 
#ifndef __EMSCRIPTEN__
#ifndef __APPLE__
#ifndef __GNUC__

			cout << "tItem offset................." << endl;
			cout << " m_Type " << offsetof(tItem, m_Type) << " :" << sizeof(tTypeItem) << endl;
			cout << " m_Extention " << offsetof(tItem, m_Extension) << ":" << sizeof(tExtension) << endl;
			cout << " m_IndiceAllocator " << offsetof(tItem, m_IndiceAllocator) << " :" << sizeof(tShort) << endl;
			cout << " m_RowAllocatorRef " << offsetof(tItem, m_RowAllocatorRef) << " :" << sizeof(tAllocatorRef) << endl;
			cout << " m_ColAllocatorRef " << offsetof(tItem, m_ColAllocatorRef) << " :" << sizeof(tAllocatorRef) << endl;
			cout << " m_ContainerCellDepend " << offsetof(tItem, m_ContainerCellDepend) << " :" << sizeof(tContainerCell) << endl;
#endif
#endif
#endif
*/
		}
	};

	typedef vector<tItem*> tVectorItem;
	typedef stack<tItem*> SkStackItem;

} // End of namespace

#endif

//=============================================================================
// SkSpreadSheet ColRow
//=============================================================================
#ifndef SkColRow_hpp
#define SkColRow_hpp

#include <SkApplication.hpp>
#include <SkMetrics.hpp>
#include "SkTools.hpp"
#include "SkStackElem.hpp"

namespace SkSpreadSheet {
//-----------------------------------------------------------------------------
// Allows you to perform all the tests for the consistency of the entire system.
// Dependencies between cells, existence of ranges, etc.
// IMPORTANT With its use, the program really does run much slower.
// -----------------------------------------------------------------------------

// Ancestor for WorkBook & Sheet ==============================================
#define SkSpAncestor tClass

    // FormulaNamed ===========================================================
    const tString CstSheetNamed = "_$$N";
    const tString CstSheetClassAnchor = "_$$A";

    /// @brief True for workbook-internal sheets (_$$A, _$$N, …). Must not become the UI active sheet.
    inline tBool IsSystemSheetName(const tString& sSheetName) {
        return sSheetName.size() >= 3 && sSheetName.compare(0, 3, "_$$") == 0;
    }

    // Rebase =================================================================
    using tSequenceId = std::uint64_t;

    // define Elem use before declararion =====================================
    class tItem;
    class tSheet;
    class tRange;
    class tCell;
    class tColRowCellRange;

    // Index of Sheet 
    typedef  tIndex tRefItem;


    //=========================================================================
    //! Class defining a row or a column. This element is attached to the sheet by the SparseList Col or Row
    class alignas(SkAlign) tColRow : public tClass {
        private:
            //! Index.
            tIndex				m_Index;
        public:
            typedef tClassVector<tAllocatorRef> tContainerRange;
            //! Tree children links (AllocatorRef; sheet Index is position / I/O only).
            typedef vector<tAllocatorRef> tVectorColRow;
        private:
            //! Container of range (used by SkCalculationPath for find recover range by intersection).
            //! This is the only container for ranges....
            tContainerRange	m_ContainerRange;

            //! Size in millimeters of row or col.(-1) for DefaultSize
            tDouble			m_Size;
            
                 //!  Data visible for  RangeFiilter
            tBool          m_DataVisible;

            //! For Css    in AllocatorFormat
            tFormatRef      m_Css;
        
            //! Parent ColRow
            tAllocatorRef m_ParentRef;

            friend class tSaveColRow;
            friend class tSaveSelectColRow;
            friend class tColRowCellRange;
            
            //! Children col row (AllocatorRef links)
            tVectorColRow  m_Children;
        
            //! Node Open Close
            tBool          m_Open;
            
       
            //! for deletion (empty ColRow)
            tInt	        m_NbCells;
#ifdef checksp
        public:
            // Verify m_NbCells
            tInt		    m_NbCellsCheck;
#endif
        public:
            /// @brief      Constructor SkColRow.
            tColRow();

            /// @brief      Clear.
            void  Clear();
                    
            /// @brief      Clear format.
            void  DeleteFormat();

            /// @brief      Set number of line or column.
            /// @param[in]  sValue tIndex
            void Index(tIndex sValue);

            /// @brief      Return number of line or column.
            /// @param[in]  tIndex
            tIndex Index();
            
            /// @brief      Return number of line or column (const version).
            /// @return     tIndex
            tIndex Index() const;

            /// @brief      Set size in millimeters of line or column.
            /// @param[in]  sValue tDouble
            void Size(tDouble sValue);

            /// @brief      return size in millimeters of line or column.
            /// @return		Double
            tDouble Size();
        
            /// @brief      Set  col Row visible.
            /// @param[in]  sValue tBool
            void DataVisible(tBool sValue);

            /// @brief      return .
            /// @return		tBool
            tBool DataVisible();
            
            /// @brief      return Is inClosed path.
            /// @param[in]  sColRowCellRange tColRowCellRange
            /// @param[in]  sIsRow tBool
            /// @return     tBool
            tBool IsInClosedPath(tColRowCellRange* sColRowCellRange, tBool sIsRow);
        
            /// @brief      return First Parent TreeNode.
            /// @param[in]  sColRowCellRange tColRowCellRange
            /// @param[in]  sIsRow tBool
            /// @return     tColRow*
            tColRow* FirstParentTreeNode(tColRowCellRange* sColRowCellRange, tBool sIsRow);
        
            /// @brief      Find Search Up Open
            /// @param[in]  sColRowCellRange tColRowCellRange
            /// @param[in]  sIsRow tBool
            /// @return     tIndex
            tIndex SearchPrecOpen(tColRowCellRange* sColRowCellRange, tBool sIsRow);
        
            /// @brief      Find Search Down  Open
            /// @param[in]  sColRowCellRange tColRowCellRange
            /// @param[in]  sIsRow tBool
            /// @return     tIndex
            tIndex SearchNextOpen(tColRowCellRange* sColRowCellRange, tBool sIsRow);

            /// @brief      Last sheet index in this node's outline subtree (self if no children).
            /// @param[in]  sColRowCellRange tColRowCellRange
            /// @param[in]  sIsRow tBool
            /// @return     tIndex
            tIndex LastTreeDescendantIndex(tColRowCellRange* sColRowCellRange, tBool sIsRow);

            /// @brief      return Is Visible (Size=0 Tree).
            /// @param[in]  sColRowCellRange tColRowCellRange
            /// @param[in]  sIsRow tBool
            /// @return     tBool
            tBool IsVisible(tColRowCellRange* sColRowCellRange, tBool sIsRow);
    
            // Css ============================================================
            /// @brief      Return css
            /// @return     tIFormatRef
            tFormatRef  Css();

            /// @brief     Set Css Value
            /// @param[in] sFormatRef tFormatRef
            void Css(tFormatRef sFormatRef);

            /// @brief      Set parent SkColRow.
            /// @param[in]  sValue tAllocatorRef
            void ParentRef(tAllocatorRef sValue);
    
            /// @brief      Set parent SkColRow.
            /// @return    tAllocatorRef
            tAllocatorRef ParentRef();
    
            /// @brief      Set parent SkColRow.
            /// @param[in]  sColRowCellRange tColRowCellRange
            /// @param[in]  sIsRow tBool
            /// @return    tColRow*
            tColRow* ColRowParent(tColRowCellRange* sColRowCellRange, tBool sIsRow);

            /// @brief      Set  open or close.
            /// @param[in]  sValue tBool
            void Open(tBool sValue);

            /// @brief      Set parent SkColRow.
            /// @return     tBool
            tBool Open();
        
        
            /// @brief      Return nb Children Node
            /// @return tInt
            tSize NbChildren();
        
            /// @brief      Test is Children Exist
            /// @param[in]  sValue tAllocatorRef
            /// @return tBool
            tBool IsChildrenExist(tAllocatorRef sValue);
        
            /// @brief      Add Children
            /// @param[in]  sValue tAllocatorRef
            /// @return tBool
            tBool AddChildren(tAllocatorRef sValue);
        
            /// @brief      Insert Children (use before renum)
            /// @param[in]  sValue tAllocatorRef
            void InsertChildren(tAllocatorRef sValue);
            
            /// @brief      Delete Children
            /// @param[in]  sValue tAllocatorRef
            /// @return tBool
            tBool DeleteChildren(tAllocatorRef sValue);
        
            /// @brief      Test is Children Exist
            /// @param[in]  sColRowCellRange tColRowCellRange
            /// @param[in]  sIsRow tBool
            /// @return tInt
            tInt Deep(tColRowCellRange* sColRowCellRange,tBool sIsRow);
        
                    
            //! @brief  Range for calculate path
            // Managment of Range
            /// @brief		Add range return false if sRange already exist 
            /// @param[in]	sRange tRange
            /// @return		tBool
            tBool AddRange(tRange* sRange);

            /// @brief		Delete range return false if sRange don't exist 
            /// @param[in]	sRange tRange
            /// @return		tBool
            tBool DeleteRange(tRange* sRange);

            /// @brief		Test Range in list return false if sRange don't exist 
            /// @param[in]	sRange tRange
            /// @return		tBool
            tBool RangeInList(tRange* sRange);
            /// @brief		Get ranges intersection with two colrow used for recover cell
            /// @param[in]	sColRow const SkColRow&
            /// @param[out]	sResult tClassContainer<tRange>::tContainerClass&
            void  GetIntersection(tColRow* sColRow, tContainerRange::tResult* sResult);

            /// @brief		Return Container of Ranges
            /// @return		tClassUnorderedContainer<tRange>* 
            tContainerRange* ContainerRange();

            // Json ===============================================================
            /// @brief		Writer Json. 
            /// @param[in]	sWriter Writer<StringBuffer>*
            /// @param[in]  sColRowCellRange tColRowCellRange
            /// @param[in]  sIsRow tBool
            void Json(Writer<StringBuffer>* sWriter, tColRowCellRange* sColRowCellRange, tBool sIsRow);

            /// @brief		Reader Json. 
            /// @param[in]	sValue Value&
            /// @param[in]  sColRowCellRange tColRowCellRange
            /// @param[in]  sIsRow tBool
            void Json(const Value& sValue,tColRowCellRange* sColRowCellRange, tBool sIsRow);

            /// @brief		Inc numbers of cells. 
            void IncNbCells();

            /// @brief		Dec numbers of cells.. 
            void DecNbCells();

#ifdef checksp
        /// @brief      IReturn NbCell.
        /// @return        tBool
            tInt NbCells();
#endif
            /// @brief      Is ColRow is Empty.
            /// @return		tBool
            tBool IsEmpty();

            /// @brief      Vacant for sheet used extent: no child cols/rows, no attached ranges.
            /// Cell membership (m_NbCells), column/row format, and explicit size alone do not expand LastCol/LastRow.
            /// @return     tBool
            tBool IsVacantForUsedExtent();

            /// @brief     Get VectorChildren.
            /// @return    tVectorColRow*
            tColRow::tVectorColRow*  VectorChildren();

        
#ifdef checksp
            /// @brief		Debug
            void Check(tSheet*  sSheet, tBool sIsRow);
#endif

            /// @brief		Debug
            tString Debug();

			/// @brief		operator ==
			/// @param[in]  sColRow tColRow
            tBool operator == (tColRow sColRow);
        
            /// @brief        operator <
            /// @param[in]  sColRow tColRow
            tBool operator < (tColRow sColRow);

            /// @brief      friend operator << for ostream.
            /// @param[in]  os ostream& 
            /// @param[in]	sColRow const SkColRow& 
            friend ostream& operator<<(ostream& os, const tColRow& sColRow);
    };
    typedef vector<tColRow*> tVectorColRow;

    //=========================================================================
    //! Sorted tColRow  by operator < on tColRow
    class tComparatorColRow {
    public:
        /// @brief      Operator() compare with tSaveCell < operator.
        /// @param[in]  sE2 tSaveCell*
        /// @param[in]  sE1 tSaveCell*
        bool operator()(tColRow* sE1, tColRow* sE2) {
            return(*sE1 < *sE2);
        }
    };
    
} // End of namespace
#endif

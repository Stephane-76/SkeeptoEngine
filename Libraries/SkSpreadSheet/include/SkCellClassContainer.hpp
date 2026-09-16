//=============================================================================
// SkSpreadSheet Named Range
//=============================================================================
#ifndef SkCellClassContainer_hpp
#define SkCellClassContainer_hpp

#include "../include/SkCellClassAttribute.hpp"
#include <unordered_map>

using namespace SkRoot;

namespace SkSpreadSheet {

    //! Container of Cell Class ==================================================
    class tCellClassContainer : public tClass {
    public:
        //! typdef ===========================================================
        typedef map<tAllocatorRef, tString> tMapAllocatorRefJsonPayLoad;
        typedef unordered_map<tString, tAllocatorRef> tMapName;
        //! Get json by cell =====================================================
        typedef unordered_map<tAllocatorRef, tString> tMapCellJson;
        ///! Used by tUndoRedo JsonPayload et tUndoSave
        typedef map<const tPoint,tString> tMapPointJsonPayLoad;
    private:
        //! Map of  Name
        tMapName			m_MapName;
        //! Map of Cell
        tMapAllocatorRefJsonPayLoad	m_MapRef;
        //! External JSON payload attached to cells (by allocator ref)
        tMapCellJson        m_MapCellJson;

        tColRowCellRange*   m_ColRowCellRange;
    public:
        /// @brief      Constructor.
        tCellClassContainer();
        
        /// @brief      Destructor.
        ~tCellClassContainer();
        
        /// @brief        Clear all members.
        void Clear();
        
        /// @brief      Set ColRowCellRange
        /// @param[in]  sColRowCellRange tColRowCellRange
        void Set(tColRowCellRange* sColRowCellRange);
        
        /// @brief      GetNext Name
        /// @param[in]  sName tString
        /// @return     tString
        tString GetNextName(tString sName);

        /// @brief      Insert CellClassContainer.
        /// @param[in]  sName tString
        /// @param[in]  sAllocatorRef tAllocatorRef
        void InsertCellClass(tString sName, tAllocatorRef sAllocatorRef);

        /// @brief      Delete CellClassContainer by Ref.
        /// @param[in]  sSheetAllocatorRef tAllocatorRef
        /// @return     tBool
        tBool DeleteCellClassByRef(tAllocatorRef sSheetAllocatorRef);

        /// @brief      Delete CellClassContainer by Name.
        /// @param[in]  sName tString
        /// @return     tBool
        tBool DeleteCellClassByName(tString sName);

        /// @brief      Return CellClassContainer by Ref.
        /// @param[in]  sAllocatorRangeRef tAllocatorRef
        /// @return     tCell*
        tCell* CellByRef(tAllocatorRef sAllocatorRef);

        /// @brief      Return CellClassContainer by Name.
        /// @param[in]  sName tString
        /// @return     tCell*
        tCell* CellByName(tString sName);

        // Direct interface
        /// @brief GetMapName
        /// @return &tMapName
        tMapName& MapName();

        /// @brief GetMapRef
        /// @return &tMapRef
        tMapAllocatorRefJsonPayLoad& MapRef();

        // Json Payload Registry ============================================
        /// @brief      Set JSON payload for a cell (string must be a JSON string)
        /// @param[in]  sAllocatorRef tAllocatorRef
        /// @param[in]  sJson tString (minified JSON recommended)
        void SetCellJsonPayload(tAllocatorRef sAllocatorRef, const tString& sJson);

        /// @brief      Get JSON payload for a cell
        /// @param[in]  sAllocatorRef tAllocatorRef
        /// @param[out] sOutJson tString&
        /// @return     tBool true if found
        tBool GetCellJsonPayload(tAllocatorRef sAllocatorRef, tString& sOutJson);

        /// @brief      Delete JSON payload for a cell
        /// @param[in]  sAllocatorRef tAllocatorRef
        /// @return     tBool true if existed and removed
        tBool DeleteCellJsonPayload(tAllocatorRef sAllocatorRef);

        /// @brief      Direct access to JSON registry map
        /// @return     tMapCellJson&
        tMapCellJson& MapCellJsonPayload();

        // Json ===============================================================
        /// @brief		Writer Json. 
        /// @param[in]	sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);

        /// @brief		Reader Json. 
        /// @param[in]	sValue Value&
        void Json(const Value& sValue);

#ifdef _DEBUGSK
        /// @brief      Debug.
        /// @return     tString
        tString Debug();
#endif
    };


}

#endif // SkCellClassContainer_hpp

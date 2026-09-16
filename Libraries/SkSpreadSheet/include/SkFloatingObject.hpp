//=============================================================================
// SkFloatingObject — workbook registry for anchored floating objects
//=============================================================================
#ifndef SkFloatingObject_hpp
#define SkFloatingObject_hpp

#include "SkColRow.hpp"
#include "SkUndoRedoRebase.hpp"
#include <unordered_map>
#include <vector>

using namespace SkRoot;

namespace SkSpreadSheet {

    class tWorkBook;
    class tCell;
    class tSheet;

    /// @brief      Layout geometry for a floating object (workbook registry).
    /// @description Stored on tFloatingObject, not on tCellClassAttribute.
    ///             m_AnchorRowRef/m_AnchorColRef locate the visual anchor on the
    ///             target/display sheet (e.g. Sheet1!B5), not on _$$A. Rebased via
    ///             RebaseAnchor() when rows/cols change on that anchor sheet.
    class tFloatingObjectLayout : public tClass {
    private:
        //! Sheet allocator ref of the layout anchor (usually the target sheet)
        tAllocatorRef m_AnchorSheetRef;
        //! Row allocator ref of the layout anchor cell (rebased on anchor sheet ops)
        tAllocatorRef m_AnchorRowRef;
        //! Col allocator ref of the layout anchor cell (rebased on anchor sheet ops)
        tAllocatorRef m_AnchorColRef;
        tDouble m_DiffX;
        tDouble m_DiffY;
        tDouble m_Width;
        tDouble m_Height;
        tDouble m_Opacity;
        tInt m_ZIndex = 0;
    public:
        tFloatingObjectLayout();
        tFloatingObjectLayout(const tFloatingObjectLayout& sOther);

        void Assign(const tFloatingObjectLayout& sOther);
        void EnsureDefaults();
        void ClearAnchorRefs();

        /// @brief      Bind layout anchor to a cell on the target sheet.
        /// @param[in]  sCell tCell*
        void AnchorCell(tCell* sCell);

        /// @brief      Resolve anchor cell from stored refs.
        /// @param[in]  sWorkBook tWorkBook*
        /// @return     tCell* nullptr if refs are invalid
        tCell* AnchorCell(tWorkBook* sWorkBook) const;

        void DiffX(tDouble sDiffX);
        tDouble DiffX() const;

        void DiffY(tDouble sDiffY);
        tDouble DiffY() const;

        void Width(tDouble sWidth);
        tDouble Width() const;

        void Height(tDouble sHeight);
        tDouble Height() const;

        void Opacity(tDouble sOpacity);
        tDouble Opacity() const;

        void ZIndex(tInt sZIndex);
        tInt ZIndex() const;

        void Json(Writer<StringBuffer>* sWriter, tWorkBook* sWorkBook) const;
        void Json(const rapidjson::Value& sValue, tWorkBook* sWorkBook, tCell* sHostCell);

        /// @brief      Rebase anchor cell refs after structural ops on the anchor sheet.
        /// @param[in]  sWorkBook tWorkBook*
        /// @param[in]  sRebasePlan const tRebasePlan&
        /// @return     tBool false if anchor row/col was deleted
        tBool RebaseAnchor(tWorkBook* sWorkBook, const tRebasePlan& sRebasePlan);
    };

    /// @brief      One floating object entry in the workbook registry.
    /// @description Host cell lives on CstSheetClassAnchor (_$$A); display targets
    ///             another sheet. tCellClassAttribute is applied on the host cell
    ///             via undo (EnsureCellClass), not by Apply() alone.
    class tFloatingObject : public tClass {
        friend class tFloatingObjectContainer;
    private:
        tWorkBook* m_WorkBook;
        tString m_Name;
        tString m_ClassName;
        tString m_TargetSheetName;
        tAllocatorRef m_TargetSheetRef;
        //! Host cell ref on _$$A col 1 (rebased via insert/delete floating-object undo)
        tAllocatorRef m_HostCellRef;
        //! Host row index on _$$A (rebased via insert/delete floating-object undo)
        tIndex m_HostRow;
        tFloatingObjectLayout m_Layout;
        tColRowCellRange* HostColRowCellRange() const;
    public:
        tFloatingObject(tString sName, tString sClassName, tString sTargetSheetName, tIndex sHostRow, tWorkBook* sWorkBook);

        tString Name() const;
        tString ClassName() const;
        tString TargetSheetName() const;
        tAllocatorRef TargetSheetRef() const;
        tIndex HostRow() const;

        /// @brief      Host cell on _$$A (column 1) for this floating object.
        /// @return     tCell* nullptr if host row is invalid
        tCell* HostCell() const;

        tFloatingObjectLayout& Layout();
        const tFloatingObjectLayout& Layout() const;

        void Json(Writer<StringBuffer>* sWriter) const;
        void JsonLayout(const rapidjson::Value& sValue);
    };

    /// @brief      Workbook-level registry of floating objects (like named formulas).
    /// @description Apply() updates maps and host row only; callers must attach
    ///             tCellClassAttribute on the host via EnsureFloatingObjectHostClass
    ///             when using undo. On ReadJson, _$$A is loaded first (sheets[]) with
    ///             host cell classes already restored; JsonFloatingObjects only rebuilds
    ///             the registry and layout.
    class tFloatingObjectContainer : public tClass {
    private:
        tWorkBook* m_WorkBook;
        typedef unordered_map<tString, tFloatingObject*> tMapByName;
        tMapByName m_MapByName;
        typedef unordered_map<tAllocatorRef, tString> tMapHostRefToName;
        tMapHostRefToName m_MapHostRefToName;

        tIndex FindFirstFreeHostRow() const;
        void ClearHostRow(tIndex sHostRow) const;
    public:
        tFloatingObjectContainer();
        ~tFloatingObjectContainer();

        void Set(tWorkBook* sWorkBook);
        void Clear();

        /// @brief      Insert or update registry entry (does not set cell class).
        /// @param[in]  sHostRow tIndex 0 allocates next free row on _$$A
        /// @return     tFloatingObject* nullptr on failure
        tFloatingObject* Apply(tString sName, tString sClassName, tString sTargetSheetName, tIndex sHostRow = 0);

        /// @brief      Update layout for an existing entry.
        tBool ApplyLayout(tString sName, const tFloatingObjectLayout& sLayout);

        /// @brief      Remove entry; optionally clear host class and recycle host row on _$$A.
        tBool DeleteByName(tString sName, tBool sClearHostCellClass = false, tBool sReleaseHostRow = true);

        tFloatingObject* ByName(tString sName) const;
        tFloatingObject* ByHostCell(tCell* sCell) const;
        void NamesOnSheet(tAllocatorRef sTargetSheetRef, vector<tString>& sOutNames) const;

        /// @brief Highest stack order on a target sheet (0 if none).
        tInt MaxZIndexOnTargetSheet(tString sTargetSheetName) const;

        /// @brief Assign zIndex 1..n per sheet for entries still at 0 (legacy workbooks).
        void EnsureZIndexDefaults();

        /// @brief      Write floatingobjects[] (registry + layout). Host cell classes live in _$$A sheet JSON.
        void JsonFloatingObjects(Writer<StringBuffer>* sWriter);

        /// @brief      Read floatingobjects[] after sheets[] (including _$$A) are loaded.
        void JsonFloatingObjects(const rapidjson::Value& sValue);

        /// @brief      Json array of floats on a target sheet (render payload for React layer).
        void JsonFloatingObjectsForSheet(Writer<StringBuffer>* sWriter, tString sTargetSheetName);

        /// @brief      Json array of all floating objects (n,c,t,dx,dy,w,h,…).
        void JsonFloatingObjectsList(Writer<StringBuffer>* sWriter);

        /// @brief      Attach tCellClassAttribute on every host cell (_$$A) from the registry.
        void EnsureAllHostCellClasses();

        /// @brief      Rebind target-sheet allocator refs after TakeOffListSheet restore.
        /// @description Sheet off-list makes WorkBook::Sheet() nullptr; registry entries keep
        ///             the name but may hold a stale m_TargetSheetRef until the sheet is back.
        void RefreshTargetSheetRefs(tString sTargetSheetName);
    };

    /// @brief Attach tCellClassAttribute on the host cell of a floating object (_$$A).
    tBool EnsureFloatingObjectHostClass(tWorkBook* sWorkBook, tFloatingObject* sObject, tString sClassName);

    /// @brief      Resolve anchor cell ref during API/undo (e.g. "Sheet1!B5").
    /// @param[in]  sRefStr tString cell ref; empty uses sHostCell sheet + default anchor
    /// @param[in]  sHostCell tCell* floating object host on _$$A
    /// @return     tCell* nullptr if ref is invalid
    tCell* FloatingObjectAnchorCellFromRef(const tString& sRefStr, tCell* sHostCell);
}

#endif

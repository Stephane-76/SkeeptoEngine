//=============================================================================
// SkSpreadSheet Item
//=============================================================================
#include "../include/SkItem.hpp"
#include "../include/SkSheet.hpp"
#include "../include/SkColRowCellRange.hpp"

#define _debugdependant

namespace SkSpreadSheet {

    // Ancestor Class of tCell and tRange ===================================
    tItem::tItem() : tClass(),
                       m_Type(tTypeItem::t_Cell),
                       m_Extension(),
                       m_ColRowCellRangeRef(0), 
                       m_RowAllocatorRef(0), 
                       m_ColAllocatorRef(0){}

    tItem::tItem(tTypeItem sType, tAllocatorRef sColRowCellRangeRef, tAllocatorRef sRowAllocatorRef, tAllocatorRef sColAllocatorRef):
        tClass(),
        m_Type(sType), 
        m_Extension(), 
        m_ColRowCellRangeRef(sColRowCellRangeRef),
        m_RowAllocatorRef(sRowAllocatorRef),
        m_ColAllocatorRef(sColAllocatorRef){}

    tItem::tItem(const tItem& sItem) : tClass(sItem) {
        tTypeItem wType=sItem.m_Type;
        Type(wType);
        m_Extension = sItem.m_Extension;
        m_RowAllocatorRef = sItem.m_RowAllocatorRef;
        m_ColAllocatorRef = sItem.m_ColAllocatorRef;
        m_ContainerCellDepend = sItem.m_ContainerCellDepend;
        m_ColRowCellRangeRef = sItem.m_ColRowCellRangeRef;
    }

    tItem::~tItem() {
        Clear();
    }

    tExtension tItem::Extension() { return(m_Extension); }

    void tItem::Extension(tExtension sExtension) { m_Extension = sExtension; }

    void tItem::Clear() {
        m_ContainerCellDepend.Container()->clear();
    }

    tBool tItem::IsNamed() const { return(m_Extension.Value(t_Named)); }
    void tItem::SetNamed() { if (!IsNamed()) m_Extension.Set(t_Named); }
    void tItem::RemoveNamed() { if (IsNamed()) m_Extension.Clear(t_Named); }
    
    tBool tItem::IsMerged() const { return(m_Extension.Value(t_Merged)); }
    void tItem::SetMerged() { if (!IsMerged()) m_Extension.Set(t_Merged); }
    void tItem::RemoveMerged() { if (IsMerged()) m_Extension.Clear(t_Merged); }

    tBool tItem::IsData() { return(m_Extension.Value(t_Data)); }
    void tItem::SetData() { if (!IsData()) m_Extension.Set(t_Data); }
    void tItem::RemoveData() { if (IsData()) m_Extension.Clear(t_Data); }
    
      // Matrix Origin ==============================================
    tBool tItem::IsMatOrigin() const { return(m_Extension.Value(t_MatOrigin)); }
    void tItem::SetMatOrigin() { if (!IsMatOrigin()) m_Extension.Set(t_MatOrigin); }
    void tItem::RemoveMatOrigin() { if (IsMatOrigin()) m_Extension.Clear(t_MatOrigin); }

    // Matrix Cell ==============================================
    tBool tItem::IsMatExtend() const { return(m_Extension.Value(t_MatExtend)); }
    void tItem::SetMatExtend() { if (!IsMatExtend()) m_Extension.Set(t_MatExtend); }
    void tItem::RemoveMatExtend() { if (IsMatExtend()) m_Extension.Clear(t_MatExtend); }
  

    // Matrix Spill Range ==============================================
    tBool tItem::IsSpillRange() const { return(m_Extension.Value(t_SpillRange)); }
    void tItem::SetSpillRange() { if (!IsSpillRange()) m_Extension.Set(t_SpillRange); }
    void tItem::RemoveSpillRange() { if (IsSpillRange()) m_Extension.Clear(t_SpillRange); }

    // Native dynamic matrix ==============================================
    tBool tItem::IsMatDynamic() const { return(m_Extension.Value(t_MatDynamic)); }
    void tItem::SetMatDynamic() { if (!IsMatDynamic()) m_Extension.Set(t_MatDynamic); }
    void tItem::RemoveMatDynamic() { if (IsMatDynamic()) m_Extension.Clear(t_MatDynamic); }


    tBool tItem::IsConditionalFormat() const { return( IsCFCF() || IsCFCS() || IsCFDB() || IsCFIS() || IsCFHR()); }

    void tItem::RemoveConditionalFormat() {
           if (IsCFHR()) m_Extension.Clear(t_CFHR);
           if (IsCFDB()) m_Extension.Clear(t_CFDB);
           if (IsCFCS()) m_Extension.Clear(t_CFCS);
           if (IsCFIS()) m_Extension.Clear(t_CFIS);
           if (IsCFCF()) m_Extension.Clear(t_CFCF);
    }

    // Conditional Format HighlightCellsRule =====================================
    tBool tItem::IsCFHR() const { return(m_Extension.Value(t_CFHR)); }
    void tItem::SetCFHR() { if (!IsCFHR()) m_Extension.Set(t_CFHR); }
    void tItem::RemoveCFHR() { if (IsCFHR()) m_Extension.Clear(t_CFHR); }

    // Conditional Format DataBar =====================================
    tBool tItem::IsCFDB() const { return(m_Extension.Value(t_CFDB)); }
    void tItem::SetCFDB() { if (!IsCFDB()) m_Extension.Set(t_CFDB); }
    void tItem::RemoveCFDB() { if (IsCFDB()) m_Extension.Clear(t_CFDB); }

    // Conditional Format IconSet =====================================
    tBool tItem::IsCFIS() const { return(m_Extension.Value(t_CFIS)); }
    void tItem::SetCFIS() { if (!IsCFIS()) m_Extension.Set(t_CFIS); }
    void tItem::RemoveCFIS() { if (IsCFIS()) m_Extension.Clear(t_CFIS); }

    // Conditional Format ColorScale =====================================
    tBool tItem::IsCFCS() const { return(m_Extension.Value(t_CFCS)); }
    void tItem::SetCFCS() { if (!IsCFCS()) m_Extension.Set(t_CFCS); }
    void tItem::RemoveCFCS() { if (IsCFCS()) m_Extension.Clear(t_CFCS); }

    // Conditional Format CustomFormat =====================================
    tBool tItem::IsCFCF() const { return(m_Extension.Value(t_CFCF)); }
    void tItem::SetCFCF() { if (!IsCFCF()) m_Extension.Set(t_CFCF); }
    void tItem::RemoveCFCF() { if (IsCFCF()) m_Extension.Clear(t_CFCF); }

    tAllocatorRef tItem::RowAllocatorRef() { return(m_RowAllocatorRef); }

    tAllocatorRef tItem::ColAllocatorRef() { return(m_ColAllocatorRef); }

    tAllocatorRef tItem::ColRowCellRangeRef() { return(m_ColRowCellRangeRef); }

    tColRowCellRange* tItem::ColRowCellRange() const {
        return(tStaticColRowCellRange::Instance()->ColRowCellRange(m_ColRowCellRangeRef));
    }


    // Polymorphism ===========================================================
    tCellAttribute* tItem::CellAttribute() {
        if (Type() == tTypeItem::t_Attribute) return(static_cast<tCellAttribute*>(this));
        return(nullptr);
    }
    
    const tCellAttribute* tItem::CellAttribute() const {
        if (Type() == tTypeItem::t_Attribute) return(static_cast<const tCellAttribute*>(this));
        return(nullptr);
    }

    tCell* tItem::Cell() {
        if (Type() == tTypeItem::t_Cell) return(static_cast<tCell*>(this));
        if (Type() == tTypeItem::t_Attribute) return(static_cast<tCellAttribute*>(this));
        return(nullptr);
    }
    
    const tCell* tItem::Cell() const {
        if (Type() == tTypeItem::t_Cell) return(static_cast<const tCell*>(this));
        if (Type() == tTypeItem::t_Attribute) return(static_cast<const tCellAttribute*>(this));
        return(nullptr);
    }

    tRange* tItem::Range() {
        if (Type() == tTypeItem::t_Range) return(static_cast<tRange*>(this));
        return(nullptr);
    }
    
    const tRange* tItem::Range() const {
        if (Type() == tTypeItem::t_Range) return(static_cast<const tRange*>(this));
        return(nullptr);
    }

    // Sheet & WorkBook ==============================================
    tSheet* tItem::Sheet() const {
        return(ColRowCellRange()->Sheet());
    };

    tWorkBook* tItem::WorkBook() { 
        return(ColRowCellRange()->Sheet()->WorkBook());
    }

    void tItem::Rooted(tItem* sItem) {
        m_RowAllocatorRef = sItem->m_RowAllocatorRef;
        m_ColAllocatorRef = sItem->m_ColAllocatorRef;
        m_ColRowCellRangeRef = sItem->m_ColRowCellRangeRef; 
    }


    const tString tItem::StrRef(tBool sSheetName) const {
        switch(Type()) {
            case tTypeItem::t_Cell : {
                const tCell* wCell = Cell();
                if (wCell != nullptr) return(wCell->StrRef(sSheetName));
                break;
            }
            case tTypeItem::t_Range : {
                const tRange* wRange = Range();
                if (wRange != nullptr) return(wRange->StrRef(sSheetName));
                break;
            }
            case tTypeItem::t_Attribute  : {
                const tCellAttribute* wCellAttribute = CellAttribute();
                if (wCellAttribute != nullptr) return(wCellAttribute->StrRef(sSheetName));
                break;
            }
            default: break;
        }
     
        return("Item unknow type!");
    }

    void tItem::AddDependent(tCell* sCell) {
        // Set Dependent (for cell) (Range is on ColRow)
        // See SkCalculationPath (calculate all cell(s) and renge(s)
        
#ifdef debugdependant
        cout << StrRef(true) << " --> (" << sCell->StrRef(true) << ")" << endl;
#endif

        m_ContainerCellDepend.InsertClass(sCell);
        // minimum of size
        m_ContainerCellDepend.Resize();
    }

    void tItem::DeleteDependent(tCell* sCell) {
#ifdef debugdependant
        cout << StrRef(true) << " <--- (" << sCell->StrRef(true) << ")" << endl;
#endif
        m_ContainerCellDepend.DeleteClass(sCell);
    }

    tItem::tContainerCell* tItem::ContainerCellDepend() { return(&m_ContainerCellDepend); }
    
    const tItem::tContainerCell* tItem::ContainerCellDepend() const { return(&m_ContainerCellDepend); }

    tBool tItem::NotDependent() {
        return(m_ContainerCellDepend.Container()->empty());
    }

    void tItem::Delete() {}

    tString  tItem::Debug() {
        tStringStream wStream;
        tCellAttribute* wCellAttribute = CellAttribute();
        if (wCellAttribute != nullptr) {
            wStream << wCellAttribute->Debug();
        }
        else {
            tCell* wCell = Cell();
            if (wCell != nullptr) {
                wStream << wCell->Debug();
            }
            else {
                tRange* wRange = Range();
                if (wRange != nullptr) {
                    wStream << wRange->Debug();
                }
            }
        }
        return(wStream.str());
    }
} // End of namespace

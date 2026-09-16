//=============================================================================
// SkFloatingObject
//=============================================================================
#include "../include/SkFloatingObject.hpp"
#include "../include/SkUndoRedoRebase.hpp"
#include "../include/SkWorkBook.hpp"
#include "../include/SkSpreadSheet.hpp"
#include "../include/SkColRowCellRange.hpp"
#include "../include/SkCellClassAttribute.hpp"
#include "../include/SkJsonKey.hpp"
#include "../include/SkTools.hpp"
#include <algorithm>

namespace SkSpreadSheet {

    namespace {

        static tBool IsHostCellEmpty(tSheet* sSheet, tIndex sRow, tIndex sCol) {
            if (sSheet == nullptr || sRow == 0) {
                return false;
            }
            return sSheet->Cell(static_cast<tInt>(sRow), static_cast<tInt>(sCol)) == nullptr;
        }

    } // namespace

    tBool EnsureFloatingObjectHostClass(tWorkBook* sWorkBook, tFloatingObject* sObject, tString sClassName) {
        if (sObject == nullptr || sClassName.empty()) {
            return true;
        }
        tCell* const wHost = sObject->HostCell();
        if (wHost == nullptr) {
            return false;
        }
        tSheet* const wHostSheet = sWorkBook->SheetClassAnchor();
        if (wHostSheet == nullptr) {
            return false;
        }
        tCellClassAttribute* const wExisting = wHost->ClassAttribute();
        if (wExisting != nullptr) {
            tString wExistingName = wExisting->ClassName();
            if (wExistingName.empty() && wExisting->ModelClass() != nullptr) {
                wExistingName = wExisting->ModelClass()->ClassName();
            }
            if (wExistingName.empty()) {
                wExistingName = wExisting->RefName();
            }
            // Never replace a host that already carries attributes (WriteJson calls EnsureAllHostCellClasses).
            if (wExistingName == sClassName || wExisting->Size() > 0) {
                wHostSheet->InsertCellClassAttributeContainer(wHost);
                return true;
            }
        }
        wHostSheet->ColRowCellRange()->EnsureCellClass(wHost->RowIndex(), wHost->ColIndex(), sClassName);
        return true;
    }

    static tDouble ReadOptionalDoubleMember(const rapidjson::Value& sValue, const char* sKey, tDouble sDefault) {
        if (!sValue.HasMember(sKey)) {
            return (sDefault);
        }
        const rapidjson::Value& wMember = sValue[sKey];
        if (wMember.IsDouble()) {
            return (wMember.GetDouble());
        }
        if (wMember.IsInt() || wMember.IsUint()) {
            return (static_cast<tDouble>(wMember.GetDouble()));
        }
        return (sDefault);
    }

    static tInt ReadOptionalIntMember(const rapidjson::Value& sValue, const char* sKey, tInt sDefault) {
        if (!sValue.HasMember(sKey)) {
            return sDefault;
        }
        const rapidjson::Value& wMember = sValue[sKey];
        if (wMember.IsInt()) {
            return wMember.GetInt();
        }
        if (wMember.IsUint()) {
            return static_cast<tInt>(wMember.GetUint());
        }
        if (wMember.IsDouble()) {
            return static_cast<tInt>(wMember.GetDouble());
        }
        return sDefault;
    }

    static void AssignAutoZIndexIfNeeded(tFloatingObjectContainer* sContainer, tFloatingObject* sObject) {
        if (sContainer == nullptr || sObject == nullptr) {
            return;
        }
        if (sObject->Layout().ZIndex() > 0) {
            return;
        }
        const tInt wNext = sContainer->MaxZIndexOnTargetSheet(sObject->TargetSheetName()) + 1;
        sObject->Layout().ZIndex(wNext);
    }

    static tCell* CellFromAnchorRefString(const tString& sRefStr, tCell* sHostCell) {
        if (sHostCell == nullptr || sHostCell->Sheet() == nullptr || sHostCell->Sheet()->WorkBook() == nullptr) {
            return nullptr;
        }
        tWorkBook* const wBook = sHostCell->Sheet()->WorkBook();
        tString wSheetName;
        tString wCellPart = sRefStr;
        const size_t wBang = sRefStr.rfind('!');
        if (wBang != tString::npos) {
            wSheetName = sRefStr.substr(0, wBang);
            wCellPart = sRefStr.substr(wBang + 1);
        }
        tSheet* wSheet = nullptr;
        if (!wSheetName.empty()) {
            wSheet = wBook->Sheet(wSheetName);
        } else if (sHostCell->Sheet()->WorkBook() != nullptr) {
            wSheet = sHostCell->Sheet();
        }
        if (wSheet == nullptr) {
            return nullptr;
        }
        tIndex wRow = 0;
        tIndex wCol = 0;
        if (!ParseCell(wCellPart, wRow, wCol)) {
            return nullptr;
        }
        return wSheet->EnsureCell(static_cast<tInt>(wRow), static_cast<tInt>(wCol));
    }

    // tFloatingObjectLayout ====================================================
    tFloatingObjectLayout::tFloatingObjectLayout()
        : m_AnchorSheetRef(0),
          m_AnchorRowRef(0),
          m_AnchorColRef(0),
          m_DiffX(0.0),
          m_DiffY(0.0),
          m_Width(100.0),
          m_Height(100.0),
          m_Opacity(1.0),
          m_ZIndex(0) {}

    tFloatingObjectLayout::tFloatingObjectLayout(const tFloatingObjectLayout& sOther)
        : m_AnchorSheetRef(sOther.m_AnchorSheetRef),
          m_AnchorRowRef(sOther.m_AnchorRowRef),
          m_AnchorColRef(sOther.m_AnchorColRef),
          m_DiffX(sOther.m_DiffX),
          m_DiffY(sOther.m_DiffY),
          m_Width(sOther.m_Width),
          m_Height(sOther.m_Height),
          m_Opacity(sOther.m_Opacity),
          m_ZIndex(sOther.m_ZIndex) {}

    void tFloatingObjectLayout::Assign(const tFloatingObjectLayout& sOther) {
        m_AnchorSheetRef = sOther.m_AnchorSheetRef;
        m_AnchorRowRef = sOther.m_AnchorRowRef;
        m_AnchorColRef = sOther.m_AnchorColRef;
        m_DiffX = sOther.m_DiffX;
        m_DiffY = sOther.m_DiffY;
        m_Width = sOther.m_Width;
        m_Height = sOther.m_Height;
        m_Opacity = sOther.m_Opacity;
        m_ZIndex = sOther.m_ZIndex;
    }

    void tFloatingObjectLayout::EnsureDefaults() {
        if (m_Width <= 0.0) {
            m_Width = 320.0;
        }
        if (m_Height <= 0.0) {
            m_Height = 240.0;
        }
        if (m_Opacity <= 0.0) {
            m_Opacity = 1.0;
        }
    }

    void tFloatingObjectLayout::ClearAnchorRefs() {
        m_AnchorSheetRef = 0;
        m_AnchorRowRef = 0;
        m_AnchorColRef = 0;
    }

    void tFloatingObjectLayout::AnchorCell(tCell* sCell) {
        if (sCell == nullptr || sCell->ColRowCellRange() == nullptr || sCell->Sheet() == nullptr) {
            ClearAnchorRefs();
            return;
        }
        m_AnchorSheetRef = sCell->Sheet()->IndexAllocatorColRowCellRange();
        m_AnchorRowRef = sCell->RowAllocatorRef();
        m_AnchorColRef = sCell->ColAllocatorRef();
    }

    tCell* tFloatingObjectLayout::AnchorCell(tWorkBook* sWorkBook) const {
        if (sWorkBook == nullptr || m_AnchorSheetRef == 0 || m_AnchorRowRef == 0 || m_AnchorColRef == 0) {
            return nullptr;
        }
        tColRowCellRange* wAnchorGrid = nullptr;
        (void)sWorkBook;
        wAnchorGrid = tStaticColRowCellRange::Instance()->ColRowCellRange(m_AnchorSheetRef);
        if (wAnchorGrid == nullptr) {
            return nullptr;
        }
        tColRow* const wRowStripe = wAnchorGrid->ColRowByAllocatorRef(m_AnchorRowRef, true);
        tColRow* const wColStripe = wAnchorGrid->ColRowByAllocatorRef(m_AnchorColRef, false);
        if (wRowStripe == nullptr || wColStripe == nullptr) {
            return nullptr;
        }
        return wAnchorGrid->Cell(wRowStripe->Index(), wColStripe->Index());
    }

    void tFloatingObjectLayout::DiffX(tDouble sDiffX) { m_DiffX = sDiffX; }
    tDouble tFloatingObjectLayout::DiffX() const { return m_DiffX; }
    void tFloatingObjectLayout::DiffY(tDouble sDiffY) { m_DiffY = sDiffY; }
    tDouble tFloatingObjectLayout::DiffY() const { return m_DiffY; }
    void tFloatingObjectLayout::Width(tDouble sWidth) { m_Width = sWidth; }
    tDouble tFloatingObjectLayout::Width() const { return m_Width; }
    void tFloatingObjectLayout::Height(tDouble sHeight) { m_Height = sHeight; }
    tDouble tFloatingObjectLayout::Height() const { return m_Height; }
    void tFloatingObjectLayout::Opacity(tDouble sOpacity) { m_Opacity = sOpacity; }
    tDouble tFloatingObjectLayout::Opacity() const { return m_Opacity; }
    void tFloatingObjectLayout::ZIndex(tInt sZIndex) { m_ZIndex = sZIndex; }
    tInt tFloatingObjectLayout::ZIndex() const { return m_ZIndex; }

    void tFloatingObjectLayout::Json(Writer<StringBuffer>* sWriter, tWorkBook* sWorkBook) const {
        tCell* const wAnchor = AnchorCell(sWorkBook);
        if (wAnchor != nullptr) {
            sWriter->Key(kJsonKeyAnchorCell);
            sWriter->String(wAnchor->StrRef(true).c_str());
        }
        sWriter->Key(kJsonKeyDiffX);
        sWriter->Double(m_DiffX);
        sWriter->Key(kJsonKeyDiffY);
        sWriter->Double(m_DiffY);
        sWriter->Key(kJsonKeyWidth);
        sWriter->Double(m_Width);
        sWriter->Key(kJsonKeyHeight);
        sWriter->Double(m_Height);
        sWriter->Key(kJsonKeyOpacity);
        sWriter->Double(m_Opacity);
        if (m_ZIndex > 0) {
            sWriter->Key(kJsonKeyZIndex);
            sWriter->Int(m_ZIndex);
        }
    }

    void tFloatingObjectLayout::Json(const rapidjson::Value& sValue, tWorkBook* sWorkBook, tCell* sHostCell) {
        ClearAnchorRefs();
        EnsureDefaults();

        const tBool wHasAnchorStr =
            sValue.HasMember(kJsonKeyAnchorCell) && sValue[kJsonKeyAnchorCell].IsString();
        if (wHasAnchorStr) {
            const tString wRefStr(sValue[kJsonKeyAnchorCell].GetString());
            if (!wRefStr.empty()) {
                tCell* wHost = sHostCell;
                if (wHost == nullptr) {
                    wHost = tSpreadSheetContainer::Instance()->CurrentJsonCell();
                }
                AnchorCell(CellFromAnchorRefString(wRefStr, wHost));
            }
        }

        m_DiffX = ReadOptionalDoubleMember(sValue, kJsonKeyDiffX, m_DiffX);
        m_DiffY = ReadOptionalDoubleMember(sValue, kJsonKeyDiffY, m_DiffY);
        m_Width = ReadOptionalDoubleMember(sValue, kJsonKeyWidth, m_Width);
        m_Height = ReadOptionalDoubleMember(sValue, kJsonKeyHeight, m_Height);
        m_Opacity = ReadOptionalDoubleMember(sValue, kJsonKeyOpacity, m_Opacity);
        m_ZIndex = ReadOptionalIntMember(sValue, kJsonKeyZIndex, m_ZIndex);
        if (m_ZIndex <= 0) {
            m_ZIndex = ReadOptionalIntMember(sValue, kJsonKeyZIndexCompact, m_ZIndex);
        }
        EnsureDefaults();
        (void)sWorkBook;
    }

    tBool tFloatingObjectLayout::RebaseAnchor(tWorkBook* sWorkBook, const tRebasePlan& sRebasePlan) {
        if (sWorkBook == nullptr || sRebasePlan.m_Operations.empty()) {
            return true;
        }
        if (m_AnchorSheetRef == 0 || m_AnchorRowRef == 0 || m_AnchorColRef == 0) {
            return true;
        }
        tCell* const wAnchor = AnchorCell(sWorkBook);
        if (wAnchor == nullptr) {
            return false;
        }
        const auto wRebasedRow = sRebasePlan.RebaseRow(wAnchor->RowIndex());
        const auto wRebasedCol = sRebasePlan.RebaseCol(wAnchor->ColIndex());
        if (!wRebasedRow.has_value() || !wRebasedCol.has_value()) {
            ClearAnchorRefs();
            return false;
        }
        tSheet* const wSheet = wAnchor->Sheet();
        if (wSheet == nullptr) {
            return false;
        }
        tCell* const wRebasedAnchor = wSheet->Cell(static_cast<tInt>(wRebasedRow.value()), static_cast<tInt>(wRebasedCol.value()));
        if (wRebasedAnchor == nullptr) {
            return false;
        }
        AnchorCell(wRebasedAnchor);
        return true;
    }

    // tFloatingObject ==========================================================
    tColRowCellRange* tFloatingObject::HostColRowCellRange() const {
        if (m_WorkBook == nullptr) {
            return nullptr;
        }
        tSheet* const wSheet = m_WorkBook->SheetClassAnchor();
        if (wSheet == nullptr) {
            return nullptr;
        }
        return wSheet->ColRowCellRange();
    }

    tFloatingObject::tFloatingObject(tString sName, tString sClassName, tString sTargetSheetName, tIndex sHostRow, tWorkBook* sWorkBook)
        : m_WorkBook(sWorkBook),
          m_Name(sName),
          m_ClassName(sClassName),
          m_TargetSheetName(sTargetSheetName),
          m_TargetSheetRef(0),
          m_HostCellRef(0),
          m_HostRow(0),
          m_Layout() {
        if (m_WorkBook == nullptr) {
            return;
        }
        tSheet* const wHostSheet = m_WorkBook->SheetClassAnchor();
        if (wHostSheet == nullptr) {
            return;
        }
        m_HostRow = sHostRow;
        tCell* const wCell = wHostSheet->EnsureCell(m_HostRow, 1);
        m_HostCellRef = wHostSheet->ColRowCellRange()->CellAllocatorRef(m_HostRow, 1);
        (void)wCell;

        tSheet* const wTargetSheet = m_WorkBook->Sheet(sTargetSheetName);
        if (wTargetSheet != nullptr) {
            m_TargetSheetRef = wTargetSheet->IndexAllocatorColRowCellRange();
        }
        m_Layout.EnsureDefaults();
    }

    tString tFloatingObject::Name() const { return m_Name; }
    tString tFloatingObject::ClassName() const { return m_ClassName; }
    tString tFloatingObject::TargetSheetName() const { return m_TargetSheetName; }
    tAllocatorRef tFloatingObject::TargetSheetRef() const { return m_TargetSheetRef; }
    tIndex tFloatingObject::HostRow() const { return m_HostRow; }

    tCell* tFloatingObject::HostCell() const {
        tColRowCellRange* const wGrid = HostColRowCellRange();
        if (wGrid == nullptr || m_HostCellRef == 0) {
            return nullptr;
        }
        return wGrid->Cell(m_HostCellRef);
    }

    tFloatingObjectLayout& tFloatingObject::Layout() { return m_Layout; }
    const tFloatingObjectLayout& tFloatingObject::Layout() const { return m_Layout; }

    void tFloatingObject::Json(Writer<StringBuffer>* sWriter) const {
        sWriter->StartObject();
        sWriter->Key("n");
        sWriter->String(m_Name.c_str());
        sWriter->Key("c");
        sWriter->String(m_ClassName.c_str());
        sWriter->Key("t");
        sWriter->String(m_TargetSheetName.c_str());
        sWriter->Key("r");
        sWriter->Int(static_cast<int>(m_HostRow));
        m_Layout.Json(sWriter, m_WorkBook);
        sWriter->EndObject();
    }

    void tFloatingObject::JsonLayout(const rapidjson::Value& sValue) {
        m_Layout.Json(sValue, m_WorkBook, HostCell());
    }

    // tFloatingObjectContainer =================================================
    tFloatingObjectContainer::tFloatingObjectContainer() : tClass(), m_WorkBook(nullptr) {}

    tFloatingObjectContainer::~tFloatingObjectContainer() {
        Clear();
    }

    void tFloatingObjectContainer::Set(tWorkBook* sWorkBook) {
        m_WorkBook = sWorkBook;
    }

    void tFloatingObjectContainer::Clear() {
        for (auto& wPair : m_MapByName) {
            delete wPair.second;
        }
        m_MapByName.clear();
        m_MapHostRefToName.clear();
    }

    tIndex tFloatingObjectContainer::FindFirstFreeHostRow() const {
        if (m_WorkBook == nullptr) {
            return (0);
        }
        tSheet* const wHostSheet = m_WorkBook->SheetClassAnchor();
        if (wHostSheet == nullptr) {
            return (0);
        }
        const tIndex wLast = wHostSheet->LastRow();
        for (tIndex wRow = 1; wRow <= wLast; ++wRow) {
            if (IsHostCellEmpty(wHostSheet, wRow, 1)) {
                return (wRow);
            }
        }
        return (wLast + 1);
    }

    void tFloatingObjectContainer::ClearHostRow(tIndex sHostRow) const {
        if (m_WorkBook == nullptr || sHostRow == 0) {
            return;
        }
        tSheet* const wHostSheet = m_WorkBook->SheetClassAnchor();
        if (wHostSheet == nullptr) {
            return;
        }
        wHostSheet->DeleteCell(sHostRow, 1);
    }

    tFloatingObject* tFloatingObjectContainer::Apply(tString sName, tString sClassName, tString sTargetSheetName, tIndex sHostRow) {
        if (m_WorkBook == nullptr || sName.empty() || sClassName.empty() || sTargetSheetName.empty()) {
            return nullptr;
        }
        tMapByName::iterator wIterator = m_MapByName.find(sName);
        if (wIterator != m_MapByName.end()) {
            tFloatingObject* wExisting = wIterator->second;
            wExisting->m_ClassName = sClassName;
            wExisting->m_TargetSheetName = sTargetSheetName;
            tSheet* const wTargetSheet = m_WorkBook->Sheet(sTargetSheetName);
            if (wTargetSheet != nullptr) {
                wExisting->m_TargetSheetRef = wTargetSheet->IndexAllocatorColRowCellRange();
            }
            return wExisting;
        }
        tIndex wResolvedHostRow = sHostRow;
        if (wResolvedHostRow == 0) {
            wResolvedHostRow = FindFirstFreeHostRow();
        }
        tFloatingObject* const wObject = new tFloatingObject(sName, sClassName, sTargetSheetName, wResolvedHostRow, m_WorkBook);
        m_MapByName[sName] = wObject;
        if (wObject->HostCell() != nullptr) {
            m_MapHostRefToName[wObject->m_HostCellRef] = sName;
        }
        AssignAutoZIndexIfNeeded(this, wObject);
        return wObject;
    }

    tBool tFloatingObjectContainer::ApplyLayout(tString sName, const tFloatingObjectLayout& sLayout) {
        tFloatingObject* const wObject = ByName(sName);
        if (wObject == nullptr) {
            return false;
        }
        wObject->Layout().Assign(sLayout);
        return true;
    }

    tBool tFloatingObjectContainer::DeleteByName(tString sName, tBool sClearHostCellClass, tBool sReleaseHostRow) {
        tMapByName::iterator wIterator = m_MapByName.find(sName);
        if (wIterator == m_MapByName.end()) {
            return false;
        }
        tFloatingObject* const wObject = wIterator->second;
        const tIndex wHostRow = wObject->HostRow();
        if (sClearHostCellClass) {
            tCell* const wHost = wObject->HostCell();
            if (wHost != nullptr && wHost->Sheet() != nullptr) {
                wHost->Sheet()->DeleteCellClassAttributeContainer(wHost);
            }
        }
        tMapHostRefToName::iterator wHostIt = m_MapHostRefToName.find(wObject->m_HostCellRef);
        if (wHostIt != m_MapHostRefToName.end()) {
            m_MapHostRefToName.erase(wHostIt);
        }
        delete wObject;
        m_MapByName.erase(wIterator);
        if (sReleaseHostRow) {
            ClearHostRow(wHostRow);
        }
        return true;
    }

    tFloatingObject* tFloatingObjectContainer::ByName(tString sName) const {
        tMapByName::const_iterator wIterator = m_MapByName.find(sName);
        if (wIterator == m_MapByName.end()) {
            return nullptr;
        }
        return wIterator->second;
    }

    tFloatingObject* tFloatingObjectContainer::ByHostCell(tCell* sCell) const {
        if (sCell == nullptr || sCell->Sheet() == nullptr || sCell->ColRowCellRange() == nullptr) {
            return nullptr;
        }
        if (sCell->Sheet()->Name() != CstSheetClassAnchor) {
            return nullptr;
        }
        const tAllocatorRef wRef = sCell->ColRowCellRange()->CellAllocatorRef(sCell->RowIndex(), sCell->ColIndex());
        tMapHostRefToName::const_iterator wIterator = m_MapHostRefToName.find(wRef);
        if (wIterator == m_MapHostRefToName.end()) {
            return nullptr;
        }
        return ByName(wIterator->second);
    }

    void tFloatingObjectContainer::NamesOnSheet(tAllocatorRef sTargetSheetRef, vector<tString>& sOutNames) const {
        sOutNames.clear();
        for (const auto& wPair : m_MapByName) {
            if (wPair.second != nullptr && wPair.second->TargetSheetRef() == sTargetSheetRef) {
                sOutNames.push_back(wPair.first);
            }
        }
        std::sort(sOutNames.begin(), sOutNames.end());
    }

    tInt tFloatingObjectContainer::MaxZIndexOnTargetSheet(tString sTargetSheetName) const {
        tInt wMax = 0;
        for (const auto& wPair : m_MapByName) {
            tFloatingObject* const wObject = wPair.second;
            if (wObject == nullptr || wObject->TargetSheetName() != sTargetSheetName) {
                continue;
            }
            wMax = std::max(wMax, wObject->Layout().ZIndex());
        }
        return wMax;
    }

    void tFloatingObjectContainer::EnsureZIndexDefaults() {
        std::map<tString, std::vector<tFloatingObject*>> wBySheet;
        for (const auto& wPair : m_MapByName) {
            tFloatingObject* const wObject = wPair.second;
            if (wObject == nullptr) {
                continue;
            }
            wBySheet[wObject->TargetSheetName()].push_back(wObject);
        }
        for (auto& wSheetPair : wBySheet) {
            std::vector<tFloatingObject*>& wObjects = wSheetPair.second;
            tBool wNeedsAssign = false;
            for (tFloatingObject* wObject : wObjects) {
                if (wObject != nullptr && wObject->Layout().ZIndex() <= 0) {
                    wNeedsAssign = true;
                    break;
                }
            }
            if (!wNeedsAssign) {
                continue;
            }
            std::sort(wObjects.begin(), wObjects.end(), [](tFloatingObject* a, tFloatingObject* b) {
                if (a == nullptr || b == nullptr) {
                    return a < b;
                }
                if (a->HostRow() != b->HostRow()) {
                    return a->HostRow() < b->HostRow();
                }
                return a->Name() < b->Name();
            });
            tInt wNext = 1;
            for (tFloatingObject* wObject : wObjects) {
                if (wObject == nullptr) {
                    continue;
                }
                if (wObject->Layout().ZIndex() <= 0) {
                    wObject->Layout().ZIndex(wNext++);
                } else {
                    wNext = std::max(wNext, wObject->Layout().ZIndex() + 1);
                }
            }
        }
    }

    void tFloatingObjectContainer::JsonFloatingObjects(Writer<StringBuffer>* sWriter) {
        if (m_MapByName.empty()) {
            return;
        }
        std::vector<tString> wNames;
        wNames.reserve(m_MapByName.size());
        for (const auto& wPair : m_MapByName) {
            wNames.push_back(wPair.first);
        }
        std::sort(wNames.begin(), wNames.end());

        sWriter->Key(kJsonKeyFloatingObjects);
        sWriter->StartArray();
        for (const tString& wName : wNames) {
            m_MapByName.at(wName)->Json(sWriter);
        }
        sWriter->EndArray();
    }

    void tFloatingObjectContainer::RefreshTargetSheetRefs(tString sTargetSheetName) {
        if (m_WorkBook == nullptr || sTargetSheetName.empty()) {
            return;
        }
        tSheet* const wSheet = m_WorkBook->Sheet(sTargetSheetName);
        if (wSheet == nullptr) {
            return;
        }
        const tAllocatorRef wSheetRef = wSheet->IndexAllocatorColRowCellRange();
        for (auto& wPair : m_MapByName) {
            tFloatingObject* const wObject = wPair.second;
            if (wObject == nullptr || wObject->TargetSheetName() != sTargetSheetName) {
                continue;
            }
            wObject->m_TargetSheetRef = wSheetRef;
        }
        EnsureAllHostCellClasses();
    }

    void tFloatingObjectContainer::EnsureAllHostCellClasses() {
        if (m_WorkBook == nullptr) {
            return;
        }
        for (const auto& wPair : m_MapByName) {
            tFloatingObject* const wObject = wPair.second;
            if (wObject == nullptr) {
                continue;
            }
            EnsureFloatingObjectHostClass(m_WorkBook, wObject, wObject->ClassName());
        }
    }

    void tFloatingObjectContainer::JsonFloatingObjects(const rapidjson::Value& sValue) {
        if (!sValue.HasMember(kJsonKeyFloatingObjects)) {
            return;
        }
        const rapidjson::Value& wArray = sValue[kJsonKeyFloatingObjects];
        if (!wArray.IsArray()) {
            return;
        }
        for (rapidjson::SizeType wIndex = 0; wIndex < wArray.Size(); wIndex++) {
            const rapidjson::Value& wObjectJson = wArray[wIndex];
            if (!wObjectJson.HasMember("n") || !wObjectJson["n"].IsString()) {
                continue;
            }
            const tString wName = wObjectJson["n"].GetString();
            const tString wClassName = wObjectJson.HasMember("c") && wObjectJson["c"].IsString()
                ? tString(wObjectJson["c"].GetString())
                : tString("");
            const tString wTargetSheet = wObjectJson.HasMember("t") && wObjectJson["t"].IsString()
                ? tString(wObjectJson["t"].GetString())
                : tString("");
            const tIndex wHostRow = wObjectJson.HasMember("r") && wObjectJson["r"].IsInt()
                ? static_cast<tIndex>(wObjectJson["r"].GetInt())
                : static_cast<tIndex>(0);
            tFloatingObject* const wObject = Apply(wName, wClassName, wTargetSheet, wHostRow);
            if (wObject != nullptr) {
                wObject->JsonLayout(wObjectJson);
            }
        }
        EnsureZIndexDefaults();
        EnsureAllHostCellClasses();
    }

    namespace {

        static void WriteFloatingObjectApiJson(Writer<StringBuffer>* sWriter,
                                               tFloatingObject* sObject,
                                               tWorkBook* sWorkBook,
                                               tBool sIncludeTargetSheet,
                                               tBool sIncludeHostPayload) {
            if (sWriter == nullptr || sObject == nullptr || sWorkBook == nullptr) {
                return;
            }
            sWriter->StartObject();
            sWriter->Key("n");
            sWriter->String(sObject->Name().c_str());
            sWriter->Key("c");
            sWriter->String(sObject->ClassName().c_str());
            if (sIncludeTargetSheet) {
                sWriter->Key("t");
                sWriter->String(sObject->TargetSheetName().c_str());
            }

            tCell* const wAnchor = sObject->Layout().AnchorCell(sWorkBook);
            if (wAnchor != nullptr) {
                sWriter->Key("ar");
                sWriter->Int64(static_cast<int64_t>(wAnchor->RowIndex()));
                sWriter->Key("ac");
                sWriter->Int64(static_cast<int64_t>(wAnchor->ColIndex()));
            }

            sWriter->Key("dx");
            sWriter->Double(sObject->Layout().DiffX());
            sWriter->Key("dy");
            sWriter->Double(sObject->Layout().DiffY());
            sWriter->Key("w");
            sWriter->Double(sObject->Layout().Width());
            sWriter->Key("h");
            sWriter->Double(sObject->Layout().Height());
            sWriter->Key("op");
            sWriter->Double(sObject->Layout().Opacity());
            const tInt wZIndex = sObject->Layout().ZIndex();
            if (wZIndex > 0) {
                sWriter->Key(kJsonKeyZIndexCompact);
                sWriter->Int(wZIndex);
            }

            tCell* const wHost = sObject->HostCell();
            if (wHost != nullptr && wHost->Sheet() != nullptr) {
                sWriter->Key("hs");
                sWriter->String(wHost->Sheet()->Name().c_str());
                sWriter->Key("hr");
                sWriter->Int64(static_cast<int64_t>(wHost->RowIndex()));
                sWriter->Key("hc");
                sWriter->Int64(static_cast<int64_t>(wHost->ColIndex()));
            }
            if (sIncludeHostPayload && wHost != nullptr && wHost->Value().IsClass()) {
                sWriter->Key("host");
                sWriter->StartObject();
                wHost->Value().JsonJavaScript(sWriter);
                sWriter->EndObject();
            }
            sWriter->EndObject();
        }

    } // namespace

    void tFloatingObjectContainer::JsonFloatingObjectsForSheet(Writer<StringBuffer>* sWriter,
                                                               tString sTargetSheetName) {
        sWriter->StartArray();
        if (m_WorkBook == nullptr || sTargetSheetName.empty()) {
            sWriter->EndArray();
            return;
        }
        std::vector<tString> wNames;
        wNames.reserve(m_MapByName.size());
        for (const auto& wPair : m_MapByName) {
            tFloatingObject* const wObject = wPair.second;
            if (wObject == nullptr || wObject->TargetSheetName() != sTargetSheetName) {
                continue;
            }
            wNames.push_back(wPair.first);
        }
        std::sort(wNames.begin(), wNames.end(), [&](const tString& a, const tString& b) {
            tFloatingObject* const wObjA = m_MapByName.at(a);
            tFloatingObject* const wObjB = m_MapByName.at(b);
            const tInt wZa = wObjA != nullptr ? wObjA->Layout().ZIndex() : 0;
            const tInt wZb = wObjB != nullptr ? wObjB->Layout().ZIndex() : 0;
            if (wZa != wZb) {
                return wZa < wZb;
            }
            return a < b;
        });
        for (const tString& wName : wNames) {
            WriteFloatingObjectApiJson(sWriter, m_MapByName.at(wName), m_WorkBook, true, true);
        }
        sWriter->EndArray();
    }

    void tFloatingObjectContainer::JsonFloatingObjectsList(Writer<StringBuffer>* sWriter) {
        sWriter->StartArray();
        if (m_WorkBook == nullptr || m_MapByName.empty()) {
            sWriter->EndArray();
            return;
        }
        std::vector<tString> wNames;
        wNames.reserve(m_MapByName.size());
        for (const auto& wPair : m_MapByName) {
            wNames.push_back(wPair.first);
        }
        std::sort(wNames.begin(), wNames.end(), [&](const tString& a, const tString& b) {
            tFloatingObject* const wObjA = m_MapByName.at(a);
            tFloatingObject* const wObjB = m_MapByName.at(b);
            const tInt wZa = wObjA != nullptr ? wObjA->Layout().ZIndex() : 0;
            const tInt wZb = wObjB != nullptr ? wObjB->Layout().ZIndex() : 0;
            if (wZa != wZb) {
                return wZa < wZb;
            }
            return a < b;
        });
        for (const tString& wName : wNames) {
            WriteFloatingObjectApiJson(sWriter, m_MapByName.at(wName), m_WorkBook, true, false);
        }
        sWriter->EndArray();
    }

    tCell* FloatingObjectAnchorCellFromRef(const tString& sRefStr, tCell* sHostCell) {
        return CellFromAnchorRefString(sRefStr, sHostCell);
    }
}

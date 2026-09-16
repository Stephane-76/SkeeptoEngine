//
//  SkTable.cpp
//
//  Created by stephane allez on 22/05/2024.
//

#include "../include/SkTable.hpp"

#include <algorithm>

namespace SkTable {

    // Index ==================================================================
    static const tChar kIndexKeySep = static_cast<tChar>(0x1F);

    tIndex::tIndex()
        : tVirtualClass()
        , m_Name()
        , m_Columns()
        , m_Unique(true)
        , m_Model(nullptr)
        , m_Entries() {}

    tIndex::~tIndex() { Clear(); }

    void tIndex::Clear() {
        m_Name = tSharedString();
        m_Columns.clear();
        m_Unique = true;
        m_Model = nullptr;
        m_Entries.clear();
    }

    void tIndex::ClearEntries() { m_Entries.clear(); }

    void tIndex::Setup(tString sName, const tVectorString& sColumns, tBool sUnique, tTableModel* sModel) {
        ClearEntries();
        m_Name = sName;
        m_Columns = sColumns;
        m_Unique = sUnique;
        m_Model = sModel;
    }

    tBool tIndex::IsActive() const { return !m_Columns.empty() && m_Model != nullptr; }
    tBool tIndex::Unique() const { return m_Unique; }
    tString tIndex::Name() const { return m_Name(); }
    const tVectorString& tIndex::Columns() const { return m_Columns; }
    tSize tIndex::Size() const { return m_Entries.size(); }

    tString tIndex::MakeKey(tRecord* sRecord, tTableModel* sModel, const tVectorString& sColumns) {
        if (sRecord == nullptr || sModel == nullptr)
            return tString();
        tStringStream wStream;
        for (tSize wIndex = 0; wIndex < sColumns.size(); wIndex++) {
            if (wIndex != 0)
                wStream << kIndexKeySep;
            wStream << sRecord->Get(sModel, sColumns[wIndex]).Str();
        }
        return wStream.str();
    }

    tString tIndex::MakeKey(tRecord* sRecord) const {
        return MakeKey(sRecord, m_Model, m_Columns);
    }

    tBool tIndex::Insert(tRecord* sRecord) {
        if (sRecord == nullptr || !IsActive())
            return false;
        tString wKey = MakeKey(sRecord);
        auto wIt = std::lower_bound(
            m_Entries.begin(), m_Entries.end(), wKey,
            [](const std::pair<tString, tRecord*>& sEntry, const tString& sKey) {
                return sEntry.first < sKey;
            });
        if (m_Unique && wIt != m_Entries.end() && wIt->first == wKey)
            return false;
        m_Entries.insert(wIt, std::make_pair(wKey, sRecord));
        return true;
    }

    tBool tIndex::Remove(tRecord* sRecord) {
        if (sRecord == nullptr || !IsActive())
            return false;
        tString wKey = MakeKey(sRecord);
        auto wLo = std::lower_bound(
            m_Entries.begin(), m_Entries.end(), wKey,
            [](const std::pair<tString, tRecord*>& sEntry, const tString& sKey) {
                return sEntry.first < sKey;
            });
        tBool wRemoved = false;
        while (wLo != m_Entries.end() && wLo->first == wKey) {
            if (wLo->second == sRecord) {
                wLo = m_Entries.erase(wLo);
                wRemoved = true;
                if (m_Unique)
                    break;
            } else {
                ++wLo;
            }
        }
        return wRemoved;
    }

    tRecord* tIndex::FindKey(const tString& sKey) const {
        auto wIt = std::lower_bound(
            m_Entries.begin(), m_Entries.end(), sKey,
            [](const std::pair<tString, tRecord*>& sEntry, const tString& sKey) {
                return sEntry.first < sKey;
            });
        if (wIt != m_Entries.end() && wIt->first == sKey)
            return wIt->second;
        return nullptr;
    }

    tRecord* tIndex::Find(tRecord* sProbe) const {
        if (sProbe == nullptr || !IsActive())
            return nullptr;
        return FindKey(MakeKey(sProbe));
    }

    void tIndex::Rebuild(const tVectorRecord& sRecords) {
        ClearEntries();
        if (!IsActive())
            return;
        m_Entries.reserve(sRecords.size());
        for (tRecord* wRecord : sRecords) {
            if (wRecord != nullptr)
                Insert(wRecord);
        }
    }

    tSize tIndex::EntryCount() const { return m_Entries.size(); }

    tRecord* tIndex::EntryRecord(tSize sIndex) const {
        if (sIndex >= m_Entries.size())
            return nullptr;
        return m_Entries[sIndex].second;
    }

    // Status helpers =========================================================
    tString StrByStatus(tStatusType sStatusType) {
        switch (sStatusType) {
            case tStatusType::t_Nothing : return("");
            case tStatusType::t_New : return("n");
            case tStatusType::t_Modify : return("m");
            case tStatusType::t_Delete : return("d");
            default:
                break;
        }
        return("");
    }

    tStatusType StatusByStr(tString sStatusType) {
        if (sStatusType=="n") return(tStatusType::t_New);
        if (sStatusType=="m") return(tStatusType::t_Modify);
        if (sStatusType=="d") return(tStatusType::t_Delete);
        return(tStatusType::t_Nothing);
    }

    // Record =================================================================
    void tRecord::WriteFieldJson(Writer<StringBuffer>* sWriter, const tVariant& sValue) {
        switch (sValue.Type()) {
            case tVariantType::t_null:
                sWriter->Null();
                break;
            case tVariantType::t_int:
                sWriter->Int(sValue.Int());
                break;
            case tVariantType::t_bool:
                sWriter->Bool(sValue.Bool());
                break;
            case tVariantType::t_double:
                sWriter->Double(sValue.Double());
                break;
            case tVariantType::t_string:
                sWriter->String(sValue.String().c_str());
                break;
            case tVariantType::t_date: {
                tClassDate wDate(sValue.Date());
                sWriter->String(wDate.UsDate().c_str());
                break;
            }
            default:
                sWriter->Null();
                break;
        }
    }

    void tRecord::ReadFieldJson(tVariant& sVariant, const rapidjson::Value& sValue) {
        switch (sValue.GetType()) {
            case kNullType:
                sVariant.Clear();
                break;
            case kFalseType:
                sVariant.SetBool(false);
                break;
            case kTrueType:
                sVariant.SetBool(true);
                break;
            case kStringType:
                sVariant.Parse(sValue.GetString());
                break;
            case kNumberType:
                if (sValue.IsDouble())
                    sVariant.SetDouble(sValue.GetDouble());
                else if (sValue.IsInt())
                    sVariant.SetInt(sValue.GetInt());
                else if (sValue.IsUint())
                    sVariant.SetInt(static_cast<tInt>(sValue.GetUint()));
                else if (sValue.IsInt64())
                    sVariant.SetInt(static_cast<tInt>(sValue.GetInt64()));
                else if (sValue.IsUint64())
                    sVariant.SetInt(static_cast<tInt>(sValue.GetUint64()));
                break;
            default:
                sVariant.Clear();
                break;
        }
    }

    tRecord::tRecord()
        : tClass()
        , m_Status(tStatusType::t_New)
        , m_Values() {}

    tRecord::tRecord(tSize sFieldCount)
        : tClass()
        , m_Status(tStatusType::t_New)
        , m_Values(sFieldCount) {}

    tRecord::~tRecord() { Clear(); }

    void tRecord::Clear() {
        m_Values.clear();
        m_Status = tStatusType::t_New;
    }

    void tRecord::Resize(tSize sFieldCount) {
        if (m_Values.size() < sFieldCount)
            m_Values.resize(sFieldCount);
    }

    tSize tRecord::ModelFieldCount(tTableModel* sModel) {
        if (sModel == nullptr)
            return 0;
        tSize wNeed = 0;
        for (const auto& wPair : *sModel->MapColumn()) {
            if (wPair.second == nullptr)
                continue;
            const tSize wEnd = wPair.second->Order() + 1;
            if (wEnd > wNeed)
                wNeed = wEnd;
        }
        return wNeed;
    }

    tInt tRecord::IndexOf(tTableModel* sModel, const tString& sName) {
        if (sModel == nullptr)
            return -1;
        tColumnModel* wColumn = sModel->FindColumn(sName);
        if (wColumn == nullptr)
            return -1;
        return static_cast<tInt>(wColumn->Order());
    }

    void tRecord::Status(tStatusType sStatusType) { m_Status = sStatusType; }
    tStatusType tRecord::Status() const { return m_Status; }

    tSize tRecord::FieldCount() const { return m_Values.size(); }

    tBool tRecord::HasField(tTableModel* sModel, tString sName) const {
        return IndexOf(sModel, sName) >= 0;
    }

    tVariant tRecord::Get(tSize sIndex) const {
        if (sIndex >= m_Values.size())
            return tVariant();
        return m_Values[sIndex];
    }

    const tVariant& tRecord::At(tSize sIndex) const {
        static const tVariant sNull;
        if (sIndex >= m_Values.size())
            return sNull;
        return m_Values[sIndex];
    }

    tVariant& tRecord::Set(tSize sIndex) {
        if (sIndex >= m_Values.size())
            m_Values.resize(sIndex + 1);
        return m_Values[sIndex];
    }

    tVariant tRecord::Get(tTableModel* sModel, tString sName) const {
        const tInt wIndex = IndexOf(sModel, sName);
        if (wIndex < 0)
            return tVariant();
        return Get(static_cast<tSize>(wIndex));
    }

    tVariant& tRecord::Set(tTableModel* sModel, tString sName) {
        const tInt wIndex = IndexOf(sModel, sName);
        if (wIndex >= 0)
            return Set(static_cast<tSize>(wIndex));
        static tVariant sDummy;
        sDummy.Clear();
        return sDummy;
    }

    void tRecord::JsonArray(Writer<StringBuffer>* sWriter) const {
        sWriter->StartArray();
        sWriter->String(StrByStatus(m_Status).c_str());
        for (tSize wIndex = 0; wIndex < m_Values.size(); wIndex++)
            WriteFieldJson(sWriter, m_Values[wIndex]);
        sWriter->EndArray();
    }

    tBool tRecord::JsonArray(const rapidjson::Value& sValue) {
        if (!sValue.IsArray() || sValue.Size() < 1)
            return false;
        if (!sValue[0].IsString())
            return false;
        m_Status = StatusByStr(sValue[0].GetString());
        const tSize wFields = sValue.Size() - 1;
        m_Values.resize(wFields);
        for (tSize wIndex = 0; wIndex < wFields; wIndex++)
            ReadFieldJson(m_Values[wIndex], sValue[static_cast<rapidjson::SizeType>(wIndex + 1)]);
        return true;
    }

    tBool tRecord::Parse(tString sJson) {
        Document wDocument;
        wDocument.Parse(sJson.c_str());
        if (wDocument.HasParseError())
            return false;
        return JsonArray(wDocument);
    }

    tString tRecord::Stringify() const {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        JsonArray(&wWriter);
        return wStringBuffer.GetString();
    }

    // Table ==================================================================
    struct tComparatorTable {
        inline bool operator()(tTable *E1, tTable *E2) {
            return (E1->Name() < E2->Name());
        }
    };

    tTable::tTable(tTableModel* sTableModel, tDataSet* sDataSet)
        : tVirtualClass()
        , m_Name(sTableModel->Name())
        , m_TableModel(sTableModel)
        , m_OwnsModel(false)
        , m_DataSet(sDataSet)
        , m_PrimaryIndex() {
        m_ModelClass = tClassFactory::Instance()->Get(m_Name());
        InitPrimaryIndex();
    }

    tTable::tTable(tString sName)
        : tVirtualClass()
        , m_Name(sName)
        , m_TableModel(nullptr)
        , m_OwnsModel(false)
        , m_DataSet(nullptr)
        , m_PrimaryIndex() {
        m_ModelClass = tClassFactory::Instance()->Get(m_Name());
    }

    tTable::~tTable() {
        Clear();
        if (m_OwnsModel) {
            delete m_TableModel;
            m_TableModel = nullptr;
            m_OwnsModel = false;
        }
    }

    void tTable::InitPrimaryIndex() {
        m_PrimaryIndex.Clear();
        if (m_TableModel == nullptr)
            return;
        tPrimaryKeyModel* wPk = m_TableModel->PrimaryKey();
        if (wPk == nullptr || !wPk->IsDefined())
            return;
        m_PrimaryIndex.Setup(wPk->Name(), *wPk->VectorKey(), true, m_TableModel);
    }

    tTableModel* tTable::EnsureAdHocModel() {
        if (m_TableModel == nullptr) {
            m_TableModel = new tTableModel();
            m_TableModel->Name(m_Name());
            m_OwnsModel = true;
        }
        return m_TableModel;
    }

    tBool tTable::EnsureAdHocColumn(tString sName) {
        if (m_TableModel != nullptr && !m_OwnsModel)
            return false;
        EnsureAdHocModel();
        if (m_TableModel->FindColumn(sName) != nullptr)
            return true;
        tColumnModel* wColumn = new tColumnModel();
        wColumn->Name(sName);
        wColumn->Order(m_TableModel->MapColumn()->size());
        wColumn->SqlType(tSqlType::VarChar);
        if (!m_TableModel->AddColumn(wColumn)) {
            delete wColumn;
            return false;
        }
        return true;
    }

    void tTable::Clear() {
        ClearRecord();
        m_PrimaryIndex.Clear();
    }

    void tTable::ClearRecord() {
        for (auto Record : m_Records)
            delete(Record);
        m_Records.clear();
        m_PrimaryIndex.ClearEntries();
    }

    tString tTable::Name() { return m_Name(); }

    tTableModel* tTable::TableModel() { return m_TableModel; }

    tVariant tTable::Get(tRecord* sRecord, tString sName) const {
        if (sRecord == nullptr || m_TableModel == nullptr)
            return tVariant();
        return sRecord->Get(m_TableModel, sName);
    }

    tVariant& tTable::Set(tRecord* sRecord, tString sName) {
        static tVariant sDummy;
        if (sRecord == nullptr) {
            sDummy.Clear();
            return sDummy;
        }
        if (m_TableModel == nullptr || m_OwnsModel)
            EnsureAdHocColumn(sName);
        if (m_TableModel == nullptr) {
            sDummy.Clear();
            return sDummy;
        }
        sRecord->Resize(tRecord::ModelFieldCount(m_TableModel));
        return sRecord->Set(m_TableModel, sName);
    }

    tString tTable::VerifyRecord(tRecord* sRecord) {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        wWriter.Key("result");
        cout << "Table : " << m_Name() << endl;
        if (m_ModelClass != nullptr) {
            tVectorPropertyModel* wVector = m_ModelClass->VectorPropertyModel();
            for (auto wProperty : *wVector) {
                tColumnModel* wColumnModel = dynamic_cast<tColumnModel*>(wProperty);
                tVariant wNotNullVariant = wColumnModel->NotNull();
                if (wNotNullVariant.IsBool()) {
                    if (wNotNullVariant.Bool())
                        cout << "Not null..";
                }
                cout << wProperty->Name() << ":" << VariantType2Str(wProperty->Type());
                if (wColumnModel != nullptr)
                    cout << "->ID:" << wColumnModel->IsID() << "->l:" << wColumnModel->Length();
                cout << endl;
            }
        }
        (void)sRecord;
        wWriter.String("Ok");
        wWriter.EndObject();
        return wStringBuffer.GetString();
    }

    tRecord* tTable::New() {
        return new tRecord(tRecord::ModelFieldCount(m_TableModel));
    }

    tBool tTable::AddRecord(tRecord* sRecord) {
        if (sRecord == nullptr)
            return false;
        if (m_PrimaryIndex.IsActive() && !m_PrimaryIndex.Insert(sRecord))
            return false;
        m_Records.push_back(sRecord);
        return true;
    }

    tSize tTable::RecordCount() const { return m_Records.size(); }

    tRecord* tTable::RecordAt(tSize sIndex) {
        if (sIndex >= m_Records.size())
            return nullptr;
        return m_Records[sIndex];
    }

    tIndex* tTable::PrimaryIndex() { return &m_PrimaryIndex; }

    tRecord* tTable::FindByPrimaryKey(tRecord* sProbe) const {
        return m_PrimaryIndex.Find(sProbe);
    }

    void tTable::RebuildPrimaryIndex() {
        InitPrimaryIndex();
        m_PrimaryIndex.ClearEntries();
        if (!m_PrimaryIndex.IsActive())
            return;
        for (tRecord* wRecord : m_Records) {
            if (wRecord != nullptr && wRecord->Status() != tStatusType::t_Delete)
                m_PrimaryIndex.Insert(wRecord);
        }
    }

    static void CollectColumnsOrdered(tTableModel* sModel, std::vector<tColumnModel*>& sOut) {
        sOut.clear();
        if (sModel == nullptr)
            return;
        sOut.reserve(sModel->MapColumn()->size());
        for (const auto& wPair : *sModel->MapColumn()) {
            if (wPair.second != nullptr)
                sOut.push_back(wPair.second);
        }
        std::sort(sOut.begin(), sOut.end(),
            [](tColumnModel* sLeft, tColumnModel* sRight) {
                return sLeft->Order() < sRight->Order();
            });
    }

    tString tTable::StringifyData() {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        wWriter.Key("name");
        wWriter.String(m_Name().c_str());

        std::vector<tColumnModel*> wColumns;
        CollectColumnsOrdered(m_TableModel, wColumns);
        wWriter.Key("columns");
        wWriter.StartArray();
        for (tColumnModel* wColumn : wColumns)
            wWriter.String(wColumn->Name().c_str());
        wWriter.EndArray();

        wWriter.Key("records");
        wWriter.StartArray();
        for (tRecord* wRecord : m_Records) {
            if (wRecord != nullptr)
                wRecord->JsonArray(&wWriter);
        }
        wWriter.EndArray();
        wWriter.EndObject();
        return wStringBuffer.GetString();
    }

    tBool tTable::ParseData(tString sJson) {
        Document wDocument;
        wDocument.Parse(sJson.c_str());
        if (wDocument.HasParseError() || !wDocument.IsObject())
            return false;

        if (wDocument.HasMember("name") && wDocument["name"].IsString()) {
            if (tString(wDocument["name"].GetString()) != Name())
                return false;
        }

        if (wDocument.HasMember("columns") && wDocument["columns"].IsArray()) {
            const rapidjson::Value& wCols = wDocument["columns"];
            for (rapidjson::SizeType wI = 0; wI < wCols.Size(); wI++) {
                if (!wCols[wI].IsString())
                    return false;
                if (m_TableModel == nullptr || m_OwnsModel)
                    EnsureAdHocColumn(wCols[wI].GetString());
            }
        }

        if (!wDocument.HasMember("records") || !wDocument["records"].IsArray())
            return false;

        ClearRecord();
        const rapidjson::Value& wList = wDocument["records"];
        for (rapidjson::SizeType wIndex = 0; wIndex < wList.Size(); wIndex++) {
            tRecord* wRecord = New();
            if (!wRecord->JsonArray(wList[wIndex])) {
                delete wRecord;
                ClearRecord();
                return false;
            }
            if (!AddRecord(wRecord)) {
                delete wRecord;
                ClearRecord();
                return false;
            }
        }
        return true;
    }

    tBool tTable::SaveData(tString sFileName) {
        tFile wFile(sFileName);
        wFile.SaveString(StringifyData());
        return true;
    }

    tBool tTable::LoadData(tString sFileName) {
        tFile wFile(sFileName);
        if (!wFile.Exist())
            return false;
        return ParseData(wFile.LoadString());
    }

    tBool tTable::Get(tRecord* sRecord) {
        if (m_DataSet == nullptr)
            return false;
        return m_DataSet->Get(sRecord);
    }
    tBool tTable::Put(tRecord* sRecord) {
        if (m_DataSet == nullptr)
            return false;
        tString wResult = VerifyRecord(sRecord);
        (void)wResult;
        return m_DataSet->Put(sRecord);
    }
    tBool tTable::Delete(tRecord* sRecord) {
        if (m_DataSet == nullptr)
            return false;
        return m_DataSet->Delete(sRecord);
    }
    tBool tTable::Modify(tRecord* sRecord) {
        if (m_DataSet == nullptr)
            return false;
        return m_DataSet->Modify(sRecord);
    }

#ifdef _DEBUGSK
    tString tTable::Debug() {
        tStringStream wStream;
        for (auto wRecord : m_Records)
            wStream << wRecord->Stringify() << endl;
        return wStream.str();
    }
#endif

    // DataSet ================================================================
    tDataSet::tDataSet(tString sDomainName, tMetaModel* sMetaModel)
        : tVirtualClass()
        , m_DomainName(sDomainName)
        , m_MetaModel(sMetaModel)
        , m_Tables()
        , m_OwnsTable() {
        if (m_MetaModel != nullptr)
            BuildTablesFromMetaModel();
    }

    tDataSet::~tDataSet() { Clear(); }

    tString tDataSet::DomainName() const { return m_DomainName; }

    tMetaModel* tDataSet::MetaModel() const { return m_MetaModel; }

    void tDataSet::MetaModel(tMetaModel* sMetaModel) { m_MetaModel = sMetaModel; }

    void tDataSet::Clear() {
        for (tSize wI = 0; wI < m_Tables.size(); wI++) {
            if (wI < m_OwnsTable.size() && m_OwnsTable[wI])
                delete m_Tables[wI];
        }
        m_Tables.clear();
        m_OwnsTable.clear();
    }

    tBool tDataSet::AddTableInternal(tTable* sTable, tBool sOwns) {
        if (sTable == nullptr)
            return false;
        tTable wKey(sTable->Name());
        auto wIt = std::lower_bound(m_Tables.begin(), m_Tables.end(), &wKey, tComparatorTable());
        if (wIt != m_Tables.end() && (*wIt)->Name() == sTable->Name())
            return false;
        const tSize wPos = static_cast<tSize>(wIt - m_Tables.begin());
        m_Tables.insert(wIt, sTable);
        m_OwnsTable.insert(m_OwnsTable.begin() + static_cast<std::ptrdiff_t>(wPos), sOwns);
        return true;
    }

    tBool tDataSet::BuildTablesFromMetaModel() {
        if (m_MetaModel == nullptr)
            return false;
        Clear();
        tVectorTableModel* wModels = m_MetaModel->VectorTableModel();
        if (wModels == nullptr)
            return false;
        for (tTableModel* wMetaTable : *wModels) {
            if (wMetaTable == nullptr)
                continue;
            tTable* wTable = new tTable(wMetaTable, this);
            if (!AddTableInternal(wTable, true))
                delete wTable;
        }
        return true;
    }

    tBool tDataSet::AddTable(tTable* sTable, tBool sOwns) {
        return AddTableInternal(sTable, sOwns);
    }

    tTable* tDataSet::FindTable(tString sName) {
        tTable wTable(sName);
        auto wIterator = std::lower_bound(m_Tables.begin(), m_Tables.end(), &wTable, tComparatorTable());
        if (wIterator != m_Tables.end() && (*wIterator)->Name() == sName)
            return *wIterator;
        return nullptr;
    }

    tSize tDataSet::TableCount() const { return m_Tables.size(); }

    tTable* tDataSet::TableAt(tSize sIndex) const {
        if (sIndex >= m_Tables.size())
            return nullptr;
        return m_Tables[sIndex];
    }

    tString tDataSet::Select(tRecord* sBegin, tRecord* sEnd) {
        (void)sBegin;
        (void)sEnd;
        return "";
    }

    tBool tDataSet::Post(tString sJson) {
        (void)sJson;
        return false;
    }

    // MemoryDataSet ==========================================================
    tMemoryDataSet::tMemoryDataSet(tString sDomainName, tMetaModel* sMetaModel)
        : tDataSet(sDomainName, sMetaModel) {}

    tMemoryDataSet::~tMemoryDataSet() = default;

    tBool tMemoryDataSet::Open() { return true; }

    void tMemoryDataSet::Close() {}

    tBool tMemoryDataSet::Get(tRecord* sRecord) {
        // Rows already live on tTable; no external fetch.
        (void)sRecord;
        return true;
    }

    tBool tMemoryDataSet::Put(tRecord* sRecord) {
        (void)sRecord;
        return true;
    }

    tBool tMemoryDataSet::Delete(tRecord* sRecord) {
        (void)sRecord;
        return true;
    }

    tBool tMemoryDataSet::Modify(tRecord* sRecord) {
        (void)sRecord;
        return true;
    }
} // end of namespace

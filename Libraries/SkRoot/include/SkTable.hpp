//
//  SkTable.hpp
//
//  Created by stephane allez on 22/05/2024.
//

#ifndef SkTable_hpp
#define SkTable_hpp

#include <vector>

#include "SkGenericClass.hpp"
#include "SkTableModel.hpp"

namespace SkTable {
    enum class tStatusType : tChar {
        t_Nothing,
        t_New,
        t_Modify,
        t_Delete
    };

    class tTable;
    class tDataSet;
    
    /// @brief Dense row: status + values only. Schema lives on tTableModel.
    class tRecord : public tClass {
    private:
        tStatusType     m_Status;
        tVectorVariant  m_Values;

        static void WriteFieldJson(Writer<StringBuffer>* sWriter, const tVariant& sValue);
        static void ReadFieldJson(tVariant& sVariant, const rapidjson::Value& sValue);
    public:
        tRecord();
        /// @brief Pre-size value slots from a column count
        /// @param[in] sFieldCount tSize
        explicit tRecord(tSize sFieldCount);
        
        ~tRecord();
        
        /// @brief Clear values (status reset to New)
        void Clear();

        /// @brief Resize dense value slots
        /// @param[in] sFieldCount tSize
        void Resize(tSize sFieldCount);

        /// @brief Slot count from a model (max Order + 1)
        /// @param[in] sModel tTableModel*
        /// @return tSize
        static tSize ModelFieldCount(tTableModel* sModel);

        /// @brief Column order for a name (-1 if missing)
        /// @param[in] sModel tTableModel*
        /// @param[in] sName const tString&
        /// @return tInt
        static tInt IndexOf(tTableModel* sModel, const tString& sName);
        
        void Status(tStatusType sStatusType);
        tStatusType Status() const;

        tSize FieldCount() const;

        tBool HasField(tTableModel* sModel, tString sName) const;

        tVariant Get(tSize sIndex) const;
        /// @brief Const view of a slot (no copy; null sentinel if out of range)
        const tVariant& At(tSize sIndex) const;
        tVariant& Set(tSize sIndex);

        tVariant Get(tTableModel* sModel, tString sName) const;
        tVariant& Set(tTableModel* sModel, tString sName);
        
        // Json array: [ status, v0, v1, ... ]
        void JsonArray(Writer<StringBuffer>* sWriter) const;
        tBool JsonArray(const rapidjson::Value& sValue);
        tBool Parse(tString sJson);
        tString Stringify() const;
    };

    typedef std::vector<tRecord*> tVectorRecord;

    /// @brief Compact in-memory index: sorted (key → record*) without copying row data.
    ///        Keys are unit-separator joined column values. Suited for PRIMARY KEY.
    class tIndex : public tVirtualClass {
    private:
        tSharedString m_Name;
        tVectorString m_Columns;
        tBool         m_Unique;
        tTableModel*  m_Model; // non-owning schema for key extraction
        std::vector<std::pair<tString, tRecord*>> m_Entries; // sorted by key

    public:
        tIndex();
        ~tIndex() override;

        void Clear();
        void ClearEntries();

        /// @brief Configure index columns and schema used to read key values
        void Setup(tString sName, const tVectorString& sColumns, tBool sUnique, tTableModel* sModel);

        tBool IsActive() const;
        tBool Unique() const;
        tString Name() const;
        const tVectorString& Columns() const;
        tSize Size() const;

        tString MakeKey(tRecord* sRecord) const;
        static tString MakeKey(tRecord* sRecord, tTableModel* sModel, const tVectorString& sColumns);

        tBool Insert(tRecord* sRecord);
        tBool Remove(tRecord* sRecord);
        tRecord* Find(tRecord* sProbe) const;
        tRecord* FindKey(const tString& sKey) const;
        void Rebuild(const tVectorRecord& sRecords);

        tSize EntryCount() const;
        tRecord* EntryRecord(tSize sIndex) const;
    };

    class tTable : public tVirtualClass {
    private:
        tSharedString m_Name;
        tTableModel*  m_TableModel;
        tBool         m_OwnsModel; // true when model was created for name-only tables
        tDataSet*     m_DataSet;
      
        tVectorRecord m_Records;
        tIndex        m_PrimaryIndex;
        tModelClass*  m_ModelClass;

        void InitPrimaryIndex();
        tTableModel* EnsureAdHocModel();
    public:
        tTable(tTableModel* sTableModel, tDataSet* sDataSet);
        tTable(tString sName);
        virtual ~tTable();
        
        void Clear();
        void ClearRecord();
        
        tString Name();
        tTableModel* TableModel();

        /// @brief Add a column on an ad-hoc table (name-only). No-op for MetaModel schemas.
        tBool EnsureAdHocColumn(tString sName);

        /// @brief Read a field via this table's model
        tVariant Get(tRecord* sRecord, tString sName) const;
        /// @brief Write a field via this table's model (grows ad-hoc schema when owned)
        tVariant& Set(tRecord* sRecord, tString sName);
        
        tString VerifyRecord(tRecord* sRecord);
        
        tRecord* New();
        
        tBool AddRecord(tRecord* sRecord);
        tSize RecordCount() const;
        tRecord* RecordAt(tSize sIndex);

        tIndex* PrimaryIndex();
        tRecord* FindByPrimaryKey(tRecord* sProbe) const;
        void RebuildPrimaryIndex();

        /// @brief Serialize: { name, columns, records: [ [status, v0, ...], ... ] }
        tString StringifyData();
        tBool ParseData(tString sJson);
        tBool SaveData(tString sFileName);
        tBool LoadData(tString sFileName);
        
        virtual tBool Get(tRecord* sRecord);
        virtual tBool Put(tRecord* sRecord);
        virtual tBool Delete(tRecord* sRecord);
        virtual tBool Modify(tRecord* sRecord);
        
#ifdef _DEBUGSK
        tString Debug() override;
#endif
    };
    typedef std::vector<tTable*> tVectorTable;

    /// @brief Generic data source (memory, workbook, file, …).
    ///        Owns a catalog of tTable* (owned and/or borrowed) and optional MetaModel.
    class tDataSet : public tVirtualClass {
    protected:
        tString            m_DomainName;
        tMetaModel*        m_MetaModel; // non-owning schema catalog
        tVectorTable       m_Tables;
        std::vector<tBool> m_OwnsTable; // parallel to m_Tables

        /// @brief Insert keeping m_Tables sorted by name (for FindTable).
        tBool AddTableInternal(tTable* sTable, tBool sOwns);

    public:
        /// @brief Construct; when sMetaModel is set, builds owned tables from it.
        tDataSet(tString sDomainName, tMetaModel* sMetaModel = nullptr);
        ~tDataSet() override;

        tString DomainName() const;
        tMetaModel* MetaModel() const;
        void MetaModel(tMetaModel* sMetaModel);

        void Clear();

        /// @brief Rebuild owned tables from MetaModel (clears previous catalog).
        tBool BuildTablesFromMetaModel();

        /// @brief Register a table. If sOwns, DataSet deletes it in Clear/dtor.
        tBool AddTable(tTable* sTable, tBool sOwns = false);

        tTable* FindTable(tString sName);
        tSize TableCount() const;
        tTable* TableAt(tSize sIndex) const;

        virtual tBool Open() = 0;
        virtual void Close() = 0;
        virtual tBool Get(tRecord* sRecord) = 0;
        virtual tBool Put(tRecord* sRecord) = 0;
        virtual tBool Delete(tRecord* sRecord) = 0;
        virtual tBool Modify(tRecord* sRecord) = 0;
        virtual tString Select(tRecord* sBegin, tRecord* sEnd);
        virtual tBool Post(tString sJson);
    };

    /// @brief In-memory DataSet: rows live on bound/owned tTable instances.
    class tMemoryDataSet : public tDataSet {
    public:
        explicit tMemoryDataSet(tString sDomainName = "memory", tMetaModel* sMetaModel = nullptr);
        ~tMemoryDataSet() override;

        tBool Open() override;
        void Close() override;
        tBool Get(tRecord* sRecord) override;
        tBool Put(tRecord* sRecord) override;
        tBool Delete(tRecord* sRecord) override;
        tBool Modify(tRecord* sRecord) override;
    };

} // end of namespace


#endif /* SkTable_h */

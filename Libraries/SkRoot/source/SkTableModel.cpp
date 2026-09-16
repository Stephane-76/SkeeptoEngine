//=============================================================================
//  SkTableModel.cpp
//
//  Created by stephane allez on 30/03/2024.
//=============================================================================
#include "../include/SkTableModel.hpp"

#include <algorithm>
#include <cctype>

namespace SkTable {

    tString SqlType2Str(tSqlType sSqlType) {
        switch (sSqlType) {
        case tSqlType::Integer:   return "integer";
        case tSqlType::SmallInt:  return "smallint";
        case tSqlType::BigInt:    return "bigint";
        case tSqlType::Boolean:   return "boolean";
        case tSqlType::Real:      return "real";
        case tSqlType::Double:    return "double";
        case tSqlType::Decimal:   return "decimal";
        case tSqlType::Char:      return "char";
        case tSqlType::VarChar:   return "varchar";
        case tSqlType::Text:      return "text";
        case tSqlType::Date:      return "date";
        case tSqlType::Time:      return "time";
        case tSqlType::Timestamp: return "timestamp";
        case tSqlType::Blob:      return "blob";
        case tSqlType::Json:      return "json";
        case tSqlType::Unknown:
        default:                  return "unknown";
        }
    }

    tSqlType Str2SqlType(tString sName) {
        tString wLower = sName;
        std::transform(wLower.begin(), wLower.end(), wLower.begin(), ::tolower);
        if (wLower == "integer" || wLower == "int") return tSqlType::Integer;
        if (wLower == "smallint") return tSqlType::SmallInt;
        if (wLower == "bigint") return tSqlType::BigInt;
        if (wLower == "boolean" || wLower == "bool") return tSqlType::Boolean;
        if (wLower == "real" || wLower == "float") return tSqlType::Real;
        if (wLower == "double" || wLower == "double precision") return tSqlType::Double;
        if (wLower == "decimal" || wLower == "numeric") return tSqlType::Decimal;
        if (wLower == "char" || wLower == "character") return tSqlType::Char;
        if (wLower == "varchar" || wLower == "character varying") return tSqlType::VarChar;
        if (wLower == "text") return tSqlType::Text;
        if (wLower == "date") return tSqlType::Date;
        if (wLower == "time") return tSqlType::Time;
        if (wLower == "timestamp" || wLower == "datetime") return tSqlType::Timestamp;
        if (wLower == "blob" || wLower == "binary" || wLower == "varbinary") return tSqlType::Blob;
        if (wLower == "json" || wLower == "jsonb") return tSqlType::Json;
        return tSqlType::Unknown;
    }

    tVariantType SqlType2VariantType(tSqlType sSqlType) {
        switch (sSqlType) {
        case tSqlType::Integer:
        case tSqlType::SmallInt:
        case tSqlType::BigInt:
            return tVariantType::t_int;
        case tSqlType::Boolean:
            return tVariantType::t_bool;
        case tSqlType::Real:
        case tSqlType::Double:
        case tSqlType::Decimal:
            return tVariantType::t_double;
        case tSqlType::Date:
        case tSqlType::Time:
        case tSqlType::Timestamp:
            return tVariantType::t_date;
        case tSqlType::Char:
        case tSqlType::VarChar:
        case tSqlType::Text:
        case tSqlType::Json:
        case tSqlType::Blob:
            return tVariantType::t_string;
        case tSqlType::Unknown:
        default:
            return tVariantType::t_null;
        }
    }

    // tColumn ==================================================================
    tColumnModel::tColumnModel()
        : tModelProperty()
        , m_IsID(false)
        , m_Length(0)
        , m_NotNull(false)
        , m_SqlType(tSqlType::Unknown)
        , m_Precision(0)
        , m_Scale(0) {}

    tColumnModel::tColumnModel(const tColumnModel& sColumn)
        : tModelProperty(sColumn)
        , m_IsID(sColumn.m_IsID)
        , m_Length(sColumn.m_Length)
        , m_NotNull(sColumn.m_NotNull)
        , m_SqlType(sColumn.m_SqlType)
        , m_Precision(sColumn.m_Precision)
        , m_Scale(sColumn.m_Scale) {}

    tColumnModel::~tColumnModel() {}

    tString tColumnModel::ClassName() const { return("tColumn");}

    void tColumnModel::IsID(tBool sIsID) { m_IsID=sIsID; }
    tBool tColumnModel::IsID() { return(m_IsID); }

    void tColumnModel::Length(tSize sLength) { m_Length=sLength; }
    tSize tColumnModel::Length() { return(m_Length); }

    void tColumnModel::NotNull(tBool sNotNull) { m_NotNull=sNotNull; }
    tBool tColumnModel::NotNull() { return(m_NotNull); }

    void tColumnModel::SqlType(tSqlType sSqlType) {
        m_SqlType = sSqlType;
        tVariantType wVariantType = SqlType2VariantType(sSqlType);
        if (wVariantType != tVariantType::t_null)
            Type(wVariantType);
    }

    tSqlType tColumnModel::SqlType() const { return m_SqlType; }

    void tColumnModel::Precision(tInt sPrecision) { m_Precision = sPrecision; }
    tInt tColumnModel::Precision() const { return m_Precision; }

    void tColumnModel::Scale(tInt sScale) { m_Scale = sScale; }
    tInt tColumnModel::Scale() const { return m_Scale; }

    struct tComparatorColumn {
        inline bool operator()(tColumnModel* C1, tColumnModel* C2) { return (C1->Order() < C2->Order()); }
    };


    // Foreign key ============================================================
    tString FkMatch2Str(tFkMatch sMatch) {
        switch (sMatch) {
        case tFkMatch::Full:    return "full";
        case tFkMatch::Partial: return "partial";
        case tFkMatch::Simple:
        default:                return "simple";
        }
    }

    tFkMatch Str2FkMatch(tString sName) {
        tString wLower = sName;
        std::transform(wLower.begin(), wLower.end(), wLower.begin(), ::tolower);
        if (wLower == "full") return tFkMatch::Full;
        if (wLower == "partial") return tFkMatch::Partial;
        return tFkMatch::Simple;
    }

    tForeignKeyModel::tForeignKeyModel()
        : tVirtualClass()
        , m_Name()
        , m_OwnerTableName()
        , m_LinkTableName()
        , m_Match(tFkMatch::Simple) {}

    tForeignKeyModel::tForeignKeyModel(const tForeignKeyModel& sForeign)
        : tVirtualClass(sForeign)
        , m_Name(sForeign.m_Name)
        , m_OwnerTableName(sForeign.m_OwnerTableName)
        , m_LinkTableName(sForeign.m_LinkTableName)
        , m_Match(sForeign.m_Match) {
        m_VectorKey = sForeign.m_VectorKey;
        m_VectorReference = sForeign.m_VectorReference;
    }

    void tForeignKeyModel::Name(tString sName) { m_Name=sName; }
    tString tForeignKeyModel::Name() { return(m_Name()); }

    void tForeignKeyModel::OwnerTableName(tString sTableName) { m_OwnerTableName=sTableName; }
    tString tForeignKeyModel::OwnerTableName() { return(m_OwnerTableName()); }

    void tForeignKeyModel::LinkTableName(tString sTableName) { m_LinkTableName=sTableName; }
    tString tForeignKeyModel::LinkTableName() { return(m_LinkTableName()); }

    void tForeignKeyModel::PushKey(tString sKey) { m_VectorKey.push_back(sKey); }

    void tForeignKeyModel::PushReference(tString sKey) { m_VectorReference.push_back(sKey); }

    tVectorString* tForeignKeyModel::VectorKey() { return &m_VectorKey; }

    tVectorString* tForeignKeyModel::VectorReference() { return &m_VectorReference; }

    void tForeignKeyModel::Match(tFkMatch sMatch) { m_Match = sMatch; }
    tFkMatch tForeignKeyModel::Match() const { return m_Match; }

    void tForeignKeyModel::Show() {
        cout << "foreign:" << m_Name() << " ->Key";
        for(tString wKey : m_VectorKey) {
            cout << ":" << wKey;
        }
        cout << " -> Reference:"  << m_LinkTableName() << " ->Key";
        for(tString wKey:m_VectorReference) {
            cout << ":" << wKey;
        }
        cout << " -> MATCH " << FkMatch2Str(m_Match);
        cout << endl;
    }
    
    // Primary key ============================================================
    tPrimaryKeyModel::tPrimaryKeyModel()
        : tVirtualClass()
        , m_Name() {}

    tPrimaryKeyModel::tPrimaryKeyModel(const tPrimaryKeyModel& sPrimary)
        : tVirtualClass(sPrimary)
        , m_Name(sPrimary.m_Name)
        , m_VectorKey(sPrimary.m_VectorKey) {}

    void tPrimaryKeyModel::Name(tString sName) { m_Name = sName; }
    tString tPrimaryKeyModel::Name() const { return m_Name(); }

    void tPrimaryKeyModel::Clear() { m_VectorKey.clear(); }

    void tPrimaryKeyModel::PushKey(tString sKey) { m_VectorKey.push_back(sKey); }

    tVectorString* tPrimaryKeyModel::VectorKey() { return &m_VectorKey; }

    tBool tPrimaryKeyModel::IsDefined() const { return !m_VectorKey.empty(); }

    void tPrimaryKeyModel::Show() {
        cout << "Primary Key";
        if (m_Name().size() != 0)
            cout << " [" << m_Name() << "]";
        for (const tString& wKey : m_VectorKey)
            cout << ":" << wKey;
        cout << endl;
    }

    // Table ==================================================================
    tTableModel::tTableModel() : tObj(),m_Name(),m_Root(),m_PrimaryKey() {  }
    tTableModel::tTableModel(const tTableModel& sTable) : tObj(sTable) {
        m_Name = sTable.m_Name;
        m_Root = sTable.m_Root;
        m_PrimaryKey = sTable.m_PrimaryKey;
        m_VectorForeign = sTable.m_VectorForeign;
        // Deep-copy columns — shallow pointer copy would double-free in ~tTableModel / SetData.
        for (const auto& wPair : sTable.m_MapColumn) {
            if (wPair.second != nullptr)
                m_MapColumn[wPair.first] = new tColumnModel(*wPair.second);
        }
    }
    tTableModel::~tTableModel() {
        for (auto& wPair : m_MapColumn)
            delete wPair.second;
        m_MapColumn.clear();
    }

    tVirtualClass* tTableModel::Clone() { return new tTableModel(*this); }

    tString tTableModel::ClassName() const { return("tTable"); }

    void tTableModel::Name(tString sName) { m_Name=sName; }
    tString tTableModel::Name() { return(m_Name()); }

    tString tTableModel::Root() { return(m_Root()); }

    tBool tTableModel::AddColumn(tColumnModel* sColumn) {
        if (sColumn == nullptr)
            return false;
        if (m_MapColumn.find(sColumn->Name()) != m_MapColumn.end())
            return false;
        m_MapColumn[sColumn->Name()] = sColumn;
        return true;
    }

    tColumnModel* tTableModel::FindColumn(tString sName) {
        auto wIt = m_MapColumn.find(sName);
        if (wIt == m_MapColumn.end())
            return nullptr;
        return wIt->second;
    }

    tMapColumn* tTableModel::MapColumn() { return &m_MapColumn; }

    tPrimaryKeyModel* tTableModel::PrimaryKey() { return &m_PrimaryKey; }

    tVectorString* tTableModel::VectorPrimaryKey() { return m_PrimaryKey.VectorKey(); }

    void tTableModel::AddForeignKey(const tForeignKeyModel& sForeign) {
        m_VectorForeign.push_back(sForeign);
    }

    tVectorMetaForeign* tTableModel::VectorForeign() { return &m_VectorForeign; }

    tBool tTableModel::Link(tMetaModel* sMetamodel) {
        return(true);
    }

    void tTableModel::SetData() {
        tVariant wName = (*this)["name"];
        if (wName.IsString())
            m_Name = wName.String();
        tVariant wRoot = (*this)["root"];
        if (wRoot.IsString())
            m_Root = wRoot.String();

        // Drop previous factory entry for this table (avoids dangling column pointers).
        if (!m_Name().empty())
            tClassFactory::Instance()->UnRegister(m_Name());

        for (auto& wPair : m_MapColumn)
            delete wPair.second;
        m_MapColumn.clear();

        tVariant wData = (*this)["columns"];
        tObj* wObjData = wData.Class<tObj>();
        if (wObjData == nullptr)
            return;

        tModelClass* wModelClass = new tModelClass(m_Name(), "Table model", nullptr);
        tVectorProperty* wVectorProperty = wObjData->VectorProperty();
        tSize wOrder = 0;
        for (auto wProperty : (*wVectorProperty)) {
            tObj* wObjRecord = wProperty.Variant().Class<tObj>();
            if (wObjRecord == nullptr)
                continue;

            tColumnModel* wColumn = new tColumnModel();
            wColumn->Name(wProperty.Name());
            wColumn->Order(wOrder);

            tVariant wVariantJson = (*wObjRecord)["json"];
            if (wVariantJson.IsString())
                wColumn->JsonRadical(wVariantJson.String());

            tVariant wVariantType = (*wObjRecord)["type"];
            if (wVariantType.IsString())
                wColumn->Type(Str2VariantType(wVariantType.String()));

            tVariant wVariantSqlType = (*wObjRecord)["sqltype"];
            if (wVariantSqlType.IsString())
                wColumn->SqlType(Str2SqlType(wVariantSqlType.String()));

            tVariant wVariantLength = (*wObjRecord)["length"];
            if (wVariantLength.IsInt())
                wColumn->Length(wVariantLength.Int());

            tVariant wVariantPrecision = (*wObjRecord)["precision"];
            if (wVariantPrecision.IsInt())
                wColumn->Precision(wVariantPrecision.Int());

            tVariant wVariantScale = (*wObjRecord)["scale"];
            if (wVariantScale.IsInt())
                wColumn->Scale(wVariantScale.Int());

            tVariant wVariantNotNull = (*wObjRecord)["notnull"];
            if (wVariantNotNull.IsBool())
                wColumn->NotNull(wVariantNotNull.Bool());

            tVariant wVariantID = (*wObjRecord)["id"];
            if (wVariantID.IsBool())
                wColumn->IsID(wVariantID.Bool());

            m_MapColumn[wColumn->Name()] = wColumn;
            wOrder++;

            // Factory owns a clone — table model keeps the original (no double-free on shutdown).
            wModelClass->AddProperty(new tColumnModel(*wColumn));
        }

        if (!tClassFactory::Instance()->Register(wModelClass))
            delete wModelClass;
    }
    void tTableModel::SetKey() {
        tVariant wKey =(*this)["key"];
        tObj* wObjKey=wKey.Class<tObj>();
        if (wObjKey!=nullptr) {
            // primary — array of column names, or object { constraint, key: [...] }
            tVariant wPrimary=(*wObjKey)["primary"];
            m_PrimaryKey.Clear();
            tArray* wList=wPrimary.Class<tArray>();
            if (wList!=nullptr) {
                for(auto wItemKey : *wList->VectorVariant()) {
                    if (wItemKey.IsString())
                        m_PrimaryKey.PushKey(wItemKey.String());
                }
            } else {
                tObj* wObjPrimary=wPrimary.Class<tObj>();
                if (wObjPrimary!=nullptr) {
                    tVariant wVariantName=(*wObjPrimary)["constraint"];
                    if (wVariantName.IsString())
                        m_PrimaryKey.Name(wVariantName.String());
                    tVariant wVariantKey=(*wObjPrimary)["key"];
                    tArray* wKeyList=wVariantKey.Class<tArray>();
                    if (wKeyList!=nullptr) {
                        for(auto wItemKey : *wKeyList->VectorVariant()) {
                            if (wItemKey.IsString())
                                m_PrimaryKey.PushKey(wItemKey.String());
                        }
                    }
                }
            }

            // Foreign
            tVariant wForeignList=(*wObjKey)["foreign"];
            tArray* wListForeign=wForeignList.Class<tArray>();
            if (wListForeign!=nullptr) {
                for(auto wForeign : *wListForeign->VectorVariant()) {
                    tObj* wObj=wForeign.Class<tObj>();
                    if (wObj!=nullptr) {
                        tForeignKeyModel wForeignKey;
                        wForeignKey.OwnerTableName(m_Name());
                        tVariant wVariantName=(*wObj)["constraint"];
                        if (wVariantName.IsString())
                            wForeignKey.Name(wVariantName.String());
                        
                        tVariant wVariantKey=(*wObj)["key"];
                       
                        tArray* wList=wVariantKey.Class<tArray>();
                        if (wList!=nullptr) {
                            for(auto wKey : *wList->VectorVariant()) {
                                if (wKey.IsString())
                                    wForeignKey.PushKey(wKey.String());
                            }
                        }
                        
                        tVariant wVariantTable=(*wObj)["table"];
                        if (wVariantTable.IsString())
                            wForeignKey.LinkTableName(wVariantTable.String());

                        tVariant wVariantReference=(*wObj)["reference"];
                        wList=wVariantReference.Class<tArray>();
                        if (wList!=nullptr) {
                            for(auto wKey : *wList->VectorVariant()) {
                                if (wKey.Type()==tVariantType::t_string)
                                    wForeignKey.PushReference(wKey.String());
                            }
                        }

                        tVariant wVariantMatch=(*wObj)["match"];
                        if (wVariantMatch.IsString())
                            wForeignKey.Match(Str2FkMatch(wVariantMatch.String()));

                        m_VectorForeign.push_back(wForeignKey);
                    }
                }
            }
        }
    }

    tObj* tTableModel::BuildJsonObj() {
        // Own pointers with SetClass — tVariant(tVirtualClass*) clones and leaks the original.
        auto take = [](tVariant& sDest, tVirtualClass* sPtr) {
            sDest.SetClass(sPtr);
        };

        tObj* wTable = new tObj();
        (*wTable)["name"] = m_Name();
        if (m_Root().size() != 0)
            (*wTable)["root"] = m_Root();

        tObj* wColumns = new tObj();
        tVectorColumn wSorted;
        for (auto wPair : m_MapColumn)
            wSorted.push_back(wPair.second);
        sort(wSorted.begin(), wSorted.end(), tComparatorColumn());

        for (tColumnModel* wColumn : wSorted) {
            tObj* wCol = new tObj();
            (*wCol)["type"] = VariantType2Str(wColumn->Type());
            if (wColumn->SqlType() != tSqlType::Unknown)
                (*wCol)["sqltype"] = SqlType2Str(wColumn->SqlType());
            if (wColumn->Length() != 0)
                (*wCol)["length"] = static_cast<tInt>(wColumn->Length());
            if (wColumn->Precision() != 0)
                (*wCol)["precision"] = wColumn->Precision();
            if (wColumn->Scale() != 0)
                (*wCol)["scale"] = wColumn->Scale();
            if (wColumn->NotNull())
                (*wCol)["notnull"] = true;
            if (wColumn->IsID())
                (*wCol)["id"] = true;
            if (wColumn->JsonRadical().size() != 0)
                (*wCol)["json"] = wColumn->JsonRadical();
            take((*wColumns)[wColumn->Name()], wCol);
        }
        take((*wTable)["columns"], wColumns);

        tObj* wKey = new tObj();
        if (m_PrimaryKey.IsDefined()) {
            if (m_PrimaryKey.Name().size() != 0) {
                tObj* wPrimary = new tObj();
                (*wPrimary)["constraint"] = m_PrimaryKey.Name();
                tArray* wPkCols = new tArray();
                for (const tString& wColName : *m_PrimaryKey.VectorKey())
                    wPkCols->Add(wColName);
                take((*wPrimary)["key"], wPkCols);
                take((*wKey)["primary"], wPrimary);
            } else {
                tArray* wPkCols = new tArray();
                for (const tString& wColName : *m_PrimaryKey.VectorKey())
                    wPkCols->Add(wColName);
                take((*wKey)["primary"], wPkCols);
            }
        }

        if (!m_VectorForeign.empty()) {
            tArray* wForeignList = new tArray();
            for (tForeignKeyModel& wFk : m_VectorForeign) {
                tObj* wFkObj = new tObj();
                if (wFk.Name().size() != 0)
                    (*wFkObj)["constraint"] = wFk.Name();
                tArray* wLocal = new tArray();
                for (const tString& wColName : *wFk.VectorKey())
                    wLocal->Add(wColName);
                take((*wFkObj)["key"], wLocal);
                if (wFk.LinkTableName().size() != 0)
                    (*wFkObj)["table"] = wFk.LinkTableName();
                tArray* wRef = new tArray();
                for (const tString& wColName : *wFk.VectorReference())
                    wRef->Add(wColName);
                take((*wFkObj)["reference"], wRef);
                if (wFk.Match() != tFkMatch::Simple)
                    (*wFkObj)["match"] = FkMatch2Str(wFk.Match());
                tVariant wFkVar;
                wFkVar.SetClass(wFkObj);
                wForeignList->Add(wFkVar);
            }
            take((*wKey)["foreign"], wForeignList);
        }
        take((*wTable)["key"], wKey);

        return wTable;
    }

    void tTableModel::Show() {
        cout << "Table:" << m_Name() << ": Root:" << m_Root() << endl;
        tVectorColumn wVector;
        for(auto wColumn : m_MapColumn) {
            wVector.push_back(wColumn.second);
        }
        sort(wVector.begin(),wVector.end(),tComparatorColumn());
        for(auto wColumn : wVector) {
            cout << wColumn->Name();
            cout << " type=" << VariantType2Str(wColumn->Type());
            if (wColumn->SqlType() != tSqlType::Unknown) {
                cout << " ,sql=" << SqlType2Str(wColumn->SqlType());
                if (wColumn->SqlType() == tSqlType::Decimal
                    && (wColumn->Precision() != 0 || wColumn->Scale() != 0)) {
                    cout << "(" << wColumn->Precision() << "," << wColumn->Scale() << ")";
                } else if ((wColumn->SqlType() == tSqlType::VarChar || wColumn->SqlType() == tSqlType::Char)
                           && wColumn->Length() != 0) {
                    cout << "(" << wColumn->Length() << ")";
                }
            }
            if (wColumn->JsonRadical()!="") cout << " ,[" << wColumn->JsonRadical() << "]";
            if (wColumn->Length()!=0 && wColumn->SqlType() == tSqlType::Unknown)
                cout << " ,length(" << wColumn->Length() << ")";
            if (wColumn->NotNull()) cout << " ," << "NotNull";
            cout << endl;
        }
        m_PrimaryKey.Show();
        for(auto wForeign : m_VectorForeign) {
            wForeign.Show();
        }
        cout << endl;
        
    }

    tVirtualClass* CreateTable() { return(new tTableModel()); }


    struct tComparatorTable {
        inline bool operator()(tTableModel* T1, tTableModel* T2) { return (T1->Name() < T2->Name()); }
    };


    // Meta Model =============================================================
    tMetaModel::tMetaModel() : tObj() { Init(); }
    tMetaModel::~tMetaModel() { Clear(); }

    void  tMetaModel::Clear() {
        for(auto wMetaTable : m_VectorTable) {
            delete(wMetaTable);
        }
        m_VectorTable.clear();
        tObj::Clear();
    }

    void tMetaModel::Init() {
        // Register once — Register() rejects duplicates and would leak the new model.
        if (tClassFactory::Instance()->Get("tTable") != nullptr)
            return;
        tModelObj* wModelTable = new tModelObj("tTable", "Table model", &CreateTable);
        if (!tClassFactory::Instance()->Register(wModelTable))
            delete wModelTable;
    }

    tVectorTableModel* tMetaModel::VectorTableModel() { return(&m_VectorTable); }

    tTableModel* tMetaModel::FindTable(tString sName) {
        for (tTableModel* wTable : m_VectorTable) {
            if (wTable != nullptr && wTable->Name() == sName)
                return wTable;
        }
        return nullptr;
    }

    tBool tMetaModel::AddTable(tTableModel* sTable) {
        if (sTable == nullptr)
            return false;
        if (FindTable(sTable->Name()) != nullptr)
            return false;
        m_VectorTable.push_back(sTable);
        return true;
    }

    tBool tMetaModel::RemoveTable(tString sName) {
        for (auto wIt = m_VectorTable.begin(); wIt != m_VectorTable.end(); ++wIt) {
            if ((*wIt) != nullptr && (*wIt)->Name() == sName) {
                delete *wIt;
                m_VectorTable.erase(wIt);
                return true;
            }
        }
        return false;
    }

    tBool tMetaModel::ParseModel(tString sJson) {
        Clear();
        if (!Parse(sJson))
            return false;

        for (tSize wIndex = 0; wIndex < Length(); wIndex++) {
            tVariant wVariant = (*this)[wIndex];
            tArray* wList = wVariant.Class<tArray>();
            if (wList == nullptr)
                continue;
            for (tSize wItem = 0; wItem < wList->Length(); wItem++) {
                tVariant wItemVariant = (*wList)[wItem];
                tTableModel* wMetaTable = wItemVariant.Class<tTableModel>();
                if (wMetaTable != nullptr) {
                    tTableModel* wTable = new tTableModel(*wMetaTable);
                    wTable->SetData();
                    wTable->SetKey();
                    m_VectorTable.push_back(wTable);
                    continue;
                }
                // Try to create tTableModel from tObj
                tObj* wObj = wItemVariant.Class<tObj>();
                if (wObj != nullptr) {
                    tTableModel* wTable = new tTableModel();
                    tVectorString wProperties = wObj->Properties();
                    for (tString wPropName : wProperties) {
                        tVariant wPropVariant = wObj->Get(wPropName);
                        (*wTable)[wPropName] = wPropVariant;
                    }
                    wTable->SetData();
                    wTable->SetKey();
                    m_VectorTable.push_back(wTable);
                }
            }
        }
        sort(m_VectorTable.begin(), m_VectorTable.end(), tComparatorTable());

        // Keep structured table models only; drop the temporary parse bag (name/columns/key tObj tree).
        tVectorTableModel wBuilt = m_VectorTable;
        m_VectorTable.clear();
        tObj::Clear();
        m_VectorTable = wBuilt;

        return Link();
    }

    tBool tMetaModel::LoadModel(tString sFileName) {
        tFile wFile(sFileName);
        if (!wFile.Exist())
            return false;
        return ParseModel(wFile.LoadString());
    }

    tString tMetaModel::StringifyModel() {
        tObj wRoot;
        tVariant wName = Get("name");
        if (wName.IsString())
            wRoot["name"] = wName;
        tVariant wVersion = Get("version");
        if (wVersion.IsString())
            wRoot["version"] = wVersion;

        tArray* wTables = new tArray();
        tVectorTableModel wSorted = m_VectorTable;
        sort(wSorted.begin(), wSorted.end(), tComparatorTable());
        for (tTableModel* wTable : wSorted) {
            if (wTable == nullptr)
                continue;
            // SetClass takes ownership — tVariant(tVirtualClass*) would Clone and leak the original.
            tVariant wItem;
            wItem.SetClass(wTable->BuildJsonObj());
            wTables->Add(wItem);
        }
        tVariant wTablesVar;
        wTablesVar.SetClass(wTables);
        wRoot["tables"] = wTablesVar;
        return wRoot.Stringify();
    }

    tBool tMetaModel::SaveModel(tString sFileName) {
        tFile wFile(sFileName);
        wFile.SaveString(StringifyModel());
        return true;
    }

    tBool tMetaModel::Link() {
        for(auto wTableModel : m_VectorTable) {
            wTableModel->Link(this);
        }
        
        
        return(true);
    }

    void tMetaModel::Show() {
        cout << "Json -->"<< StringifyModel() << endl;
        cout << "MataModel " << (*this)["name"] << ":" << (*this)["version"] << endl;
        for(auto wTable : m_VectorTable) {
            wTable->Show();
        }
    }

    }; // end of namespace

//=============================================================================
//  SkTable.hpp
//
//  Created by stephane allez on 30/03/2024.
//=============================================================================
#ifndef SkTableModel_hpp
#define SkTableModel_hpp

#include "SkClass.hpp"
#include "SkGenericClass.hpp"
#include "SkFile.hpp"

using namespace SkRoot;

namespace SkTable {

    /// @brief SQL column type (DDL metadata; runtime values stay on tVariantType).
    enum class tSqlType : tUByte {
        Unknown = 0,
        Integer,
        SmallInt,
        BigInt,
        Boolean,
        Real,
        Double,
        Decimal,   // uses Precision / Scale
        Char,      // uses Length
        VarChar,   // uses Length
        Text,
        Date,
        Time,
        Timestamp,
        Blob,
        Json
    };

    /// @brief Convert SQL type enum to lowercase name (e.g. "varchar").
    tString SqlType2Str(tSqlType sSqlType);

    /// @brief Parse SQL type name (case-insensitive). Unknown if not recognized.
    tSqlType Str2SqlType(tString sName);

    /// @brief Map SQL type to the closest Sk variant type for in-memory values.
    tVariantType SqlType2VariantType(tSqlType sSqlType);

    // Column Data Model ======================================================
    class tColumnModel : public tModelProperty {
    private:
        tBool         m_IsID;
        tSize         m_Length;
        tBool         m_NotNull;
        tSqlType      m_SqlType;
        tInt          m_Precision;
        tInt          m_Scale;
    public:
        /// @brief Constructor
        tColumnModel();
        
        /// @brief Copy constructor
        /// @param[in] sColumn tColumnModel&
        tColumnModel(const tColumnModel& sColumn);
        
        /// @brief Destructor
        ~tColumnModel() override;
        
        /// @brief Return name of class (for factory)
        /// @return tString
        tString ClassName() const override;
        
        /// @brief Set if column is ID
        /// @param[in] sIsID tBool
        void IsID(tBool sIsID);
        
        /// @brief Get if column is ID
        /// @return tBool
        tBool IsID();
        
        /// @brief Set length of column (CHAR / VARCHAR)
        /// @param[in] sLength tSize
        void Length(tSize sLength);
        
        /// @brief Get length of column
        /// @return tSize
        tSize Length();
        
        /// @brief Set if column is not null
        /// @param[in] sNotNull tBool
        void NotNull(tBool sNotNull);
        
        /// @brief Get if column is not null
        /// @return tBool
        tBool NotNull();

        /// @brief Set SQL type; also updates tModelProperty::Type via SqlType2VariantType.
        /// @param[in] sSqlType tSqlType
        void SqlType(tSqlType sSqlType);

        /// @brief Get SQL type
        /// @return tSqlType
        tSqlType SqlType() const;

        /// @brief Set DECIMAL precision
        /// @param[in] sPrecision tInt
        void Precision(tInt sPrecision);

        /// @brief Get DECIMAL precision
        /// @return tInt
        tInt Precision() const;

        /// @brief Set DECIMAL scale
        /// @param[in] sScale tInt
        void Scale(tInt sScale);

        /// @brief Get DECIMAL scale
        /// @return tInt
        tInt Scale() const;
    };

    typedef map<tString,tColumnModel*> tMapColumn;
    typedef vector<tColumnModel*> tVectorColumn;

    /// @brief SQL MATCH option for foreign keys (default Simple).
    enum class tFkMatch : tUByte {
        Simple = 0,
        Full,
        Partial
    };

    tString FkMatch2Str(tFkMatch sMatch);
    tFkMatch Str2FkMatch(tString sName);

    // Foreign key ============================================================
    class tForeignKeyModel : public tVirtualClass {
    private:
        tSharedString m_Name;
        tSharedString m_OwnerTableName;
        tVectorString m_VectorKey;        // Vector name column
        tSharedString m_LinkTableName;
        tVectorString m_VectorReference; // Vector name column
        tFkMatch      m_Match;
    public:
        /// @brief Constructor
        tForeignKeyModel();
        
        /// @brief Copy constructor
        /// @param[in] sForeign tForeignKeyModel&
        tForeignKeyModel(const tForeignKeyModel& sForeign);
        
        /// @brief Set name of foreign key
        /// @param[in] sName tString
        void Name(tString sName);
        
        /// @brief Get name of foreign key
        /// @return tString
        tString Name();
        
        /// @brief Set owner table name
        /// @param[in] sTableName tString
        void OwnerTableName(tString sTableName);
        
        /// @brief Get owner table name
        /// @return tString
        tString OwnerTableName();
        
        /// @brief Set link table name
        /// @param[in] sTableName tString
        void LinkTableName(tString sTableName);
        
        /// @brief Get link table name
        /// @return tString
        tString LinkTableName();
        
        /// @brief Add key to vector
        /// @param[in] sKey tString
        void PushKey(tString sKey);
        
        /// @brief Add reference to vector
        /// @param[in] sKey tString
        void PushReference(tString sKey);

        /// @brief Get local key column names
        /// @return tVectorString*
        tVectorString* VectorKey();

        /// @brief Get referenced column names
        /// @return tVectorString*
        tVectorString* VectorReference();

        /// @brief Set MATCH option (SIMPLE / FULL / PARTIAL)
        /// @param[in] sMatch tFkMatch
        void Match(tFkMatch sMatch);

        /// @brief Get MATCH option
        /// @return tFkMatch
        tFkMatch Match() const;
        
        /// @brief Show foreign key details
        void Show();
    };
    typedef vector<tForeignKeyModel> tVectorMetaForeign;

    // Primary key ============================================================
    class tPrimaryKeyModel : public tVirtualClass {
    private:
        tSharedString m_Name;
        tVectorString m_VectorKey;
    public:
        /// @brief Constructor
        tPrimaryKeyModel();

        /// @brief Copy constructor
        /// @param[in] sPrimary const tPrimaryKeyModel&
        tPrimaryKeyModel(const tPrimaryKeyModel& sPrimary);

        /// @brief Set constraint name
        /// @param[in] sName tString
        void Name(tString sName);

        /// @brief Get constraint name
        /// @return tString
        tString Name() const;

        /// @brief Clear column list
        void Clear();

        /// @brief Add a primary-key column name
        /// @param[in] sKey tString
        void PushKey(tString sKey);

        /// @brief Get column names of the primary key
        /// @return tVectorString*
        tVectorString* VectorKey();

        /// @brief True if at least one column is defined
        /// @return tBool
        tBool IsDefined() const;

        /// @brief Show primary key details
        void Show();
    };

    class tMetaModel;

    // Table Data Model =======================================================
    class tTableModel : public tObj {
    private:
        tSharedString       m_Name;
        tSharedString       m_Root;
        tMapColumn          m_MapColumn;
        tPrimaryKeyModel    m_PrimaryKey;
        tVectorMetaForeign  m_VectorForeign;
    public:
        /// @brief Constructor
        tTableModel();
        
        /// @brief Copy constructor
        /// @param[in] sTable tTableModel&
        tTableModel(const tTableModel& sTable);
        
        /// @brief Destructor
        ~tTableModel() override;
        
        /// @brief Clone derived class
        /// @return tVirtualClass*
        tVirtualClass* Clone() override;
        
        /// @brief Return name of class (for factory)
        /// @return tString
        tString ClassName() const override;
        
        /// @brief Set table name
        /// @param[in] sName tString
        void Name(tString sName);
        
        /// @brief Get table name
        /// @return tString
        tString Name();
        
        /// @brief Get root name
        /// @return tString
        tString Root();

        /// @brief Add a column (table takes ownership).
        /// @param[in] sColumn tColumnModel*
        /// @return tBool false if name already exists
        tBool AddColumn(tColumnModel* sColumn);

        /// @brief Find column by name
        /// @param[in] sName tString
        /// @return tColumnModel* or nullptr
        tColumnModel* FindColumn(tString sName);

        /// @brief Get column map
        /// @return tMapColumn*
        tMapColumn* MapColumn();

        /// @brief Get primary key model
        /// @return tPrimaryKeyModel*
        tPrimaryKeyModel* PrimaryKey();
        
        /// @brief Get vector of primary-key column names (compat)
        /// @return tVectorString*
        tVectorString* VectorPrimaryKey();

        /// @brief Add a foreign key
        /// @param[in] sForeign const tForeignKeyModel&
        void AddForeignKey(const tForeignKeyModel& sForeign);

        /// @brief Get foreign keys
        /// @return tVectorMetaForeign*
        tVectorMetaForeign* VectorForeign();
        
        /// @brief Link primary and secondary keys
        /// @param[in] sMetamodel tMetaModel*
        /// @return tBool
        tBool Link(tMetaModel* sMetamodel);
        
        /// @brief Set data
        void SetData();
        
        /// @brief Set key
        void SetKey();

        /// @brief Build a JSON-ready tObj from the structured model (caller owns via tVariant).
        /// @return tObj*
        tObj* BuildJsonObj();
        
        /// @brief Show table model details
        void Show();
    };

    typedef vector<tTableModel*> tVectorTableModel;
 
    class tMetaModel : public tObj {
    private:
        tVectorTableModel m_VectorTable;
    public:
        /// @brief Constructor
        tMetaModel();
        
        /// @brief Destructor
        virtual ~tMetaModel();
        
        /// @brief Clear the meta model
        void Clear();
        
        /// @brief Initialize the meta model
        void Init();
        
        /// @brief Get vector of table models
        /// @return tVectorTableModel*
        tVectorTableModel* VectorTableModel();

        /// @brief Find table by name
        /// @param[in] sName tString
        /// @return tTableModel* or nullptr
        tTableModel* FindTable(tString sName);

        /// @brief Add table (meta model takes ownership).
        /// @param[in] sTable tTableModel*
        /// @return tBool false if name already exists
        tBool AddTable(tTableModel* sTable);

        /// @brief Remove and delete table by name
        /// @param[in] sName tString
        /// @return tBool
        tBool RemoveTable(tString sName);
        
        /// @brief Load model from file
        /// @param[in] sFileName tString
        /// @return tBool
        tBool LoadModel(tString sFileName);

        /// @brief Parse model JSON (same format as StringifyModel / SaveModel).
        /// @param[in] sJson tString
        /// @return tBool
        tBool ParseModel(tString sJson);

        /// @brief Serialize structured tables to JSON (keeps name/version if present).
        /// @return tString
        tString StringifyModel();

        /// @brief Write meta model JSON to file
        /// @param[in] sFileName tString
        /// @return tBool
        tBool SaveModel(tString sFileName);
        
        /// @brief Link table models
        /// @return tBool
        tBool Link();
        
        /// @brief Show meta model details
        void Show();
    };

} // end of namespace


#endif // SkMetaModel_hpp

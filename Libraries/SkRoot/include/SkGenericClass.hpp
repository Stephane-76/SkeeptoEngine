//=============================================================================
// SkGenericClass (Generic class)
// 24/03/2024 class for Generic Json
//=============================================================================
#ifndef SkGenericClass_hpp
#define SkGenericClass_hpp

#include "SkModelClass.hpp"

namespace SkRoot {
 
    //! tItem Generic Item ===========================================================
    class tProperty : public tClass {
    private:
        friend class tObj;
        tSharedString m_Name;
        tVariant      m_Variant;
    public:
        /// @brief Constructor
        tProperty();
        
        /// @brief Copy constructor
        /// @param[in] sProperty tProperty&
        tProperty(const tProperty& sProperty);
        
        /// @brief Set the property name
        /// @param[in] sName tString
        void Name(tString sName);
        
        /// @brief Get the property name
        /// @return tString
        tString Name();
        
        /// @brief Get the property variant
        /// @return tVariant&
        tVariant& Variant();
    };

    typedef vector<tProperty> tVectorProperty;

    class tModelObj;
    class tArray;
    //! tObj Generic class ===========================================================
    //!  Used by tCellClass 
    class tObj : public tVirtualClass {
    private:
        ///! Vector of property (Many for tArray, one for other)
        tVectorProperty  m_Properties;
    protected:
        ///! Model Obj
        tModelClass*     m_ModelObj;
        
        ///!  ObjType Name
        tSharedString    m_ObjType;
        
        /// @brief Create obj by class name (Json key "$.")
        /// @param[in] sIterator Value::ConstMemberIterator
        /// @return tObj*
        tObj* InstanteObjByJson(Value::ConstMemberIterator sIterator);
        
        // Path helper methods ===============================================
        /// @brief Split path string into vector of parts
        /// @param[in] sPath tString path with dot notation
        /// @param[out] sPathParts tVectorString& output vector
        /// @return tBool true if path is valid
        tBool SplitPath(tString sPath, tVectorString& sPathParts);
        
        /// @brief Navigate to object at given path depth
        /// @param[in] sPathParts tVectorString& path parts
        /// @param[in] sDepth tSize depth to navigate to
        /// @param[in] sCreateMissing tBool create missing objects
        /// @return tObj* pointer to object at depth, or nullptr if not found
        tObj* NavigateToPath(const tVectorString& sPathParts, tSize sDepth, tBool sCreateMissing = false);
    public:
        /// @brief Return value property for write
        /// @param[in] sName tString
        /// @return tVariant&
        tVariant& Set(tString sName);
        
        /// @brief Return value property for read
        /// @param[in] sName tString
        /// @return tVariant
        tVariant Get(tString sName);
    public:
        /// @brief Constructor
        tObj();
        
        /// @brief Copy constructor
        /// @param[in] sGenericClass tObj&
        tObj(const tObj& sGenericClass);
        
        /// @brief Destructor
        ~tObj() override;
        
        /// @brief Clear the object
        virtual void Clear();
        
        /// @brief Tell variant to manage the lifetime (owned copy)
        tBool IsCopy() override;
        
        /// @brief Set the object type
        /// @param[in] sObjType tString
        void ObjType(tString sObjType);
        
        /// @brief Get the object type
        /// @return tString
        tString ObjType();
        
        /// @brief Function Generic
        /// @param[in] sName tString
        /// @param[in] sArgument tObj
        /// @return tObj
        virtual tObj Function(tString sName, tObj sArgument);
        
        /// @brief Clone derived class
        /// @return tVirtualClass*
        tVirtualClass* Clone() override;

        /// @brief Return name of class (for factory)
        /// @return tString
        tString ClassName() const override;
        
        /// @brief Return vector of properties
        /// @return tVectorProperty*
        tVectorProperty* VectorProperty();
 
        /// @brief Set model object
        /// @param[in] sModelObj tModelObj*
        void ModelObj(tModelObj* sModelObj);

        // Json ==============================================================
        /// @brief Write JSON
        /// @param[in] sWriter Writer<StringBuffer>*
        /// @param[in] sJsonRadical tBool
        virtual void JsonObj(Writer<StringBuffer>* sWriter, tBool sJsonRadical = false);

        /// @brief Read JSON
        /// @param[in] sValue Value&
        /// @param[in] sJsonRadical tBool
        virtual void JsonObj(const rapidjson::Value& sValue, tBool sJsonRadical = false);
        
        /// @brief Parse JSON
        /// @param[in] sJson tString
        /// @param[in] sJsonRadical tBool
        /// @return tBool
        tBool Parse(tString sJson, tBool sJsonRadical = false);
        
        /// @brief Arrayfy object
        /// @param[in] sJsonRadical tBool
        /// @return tString
        tString Stringify(tBool sJsonRadical = false);
        
        /// @brief Get object property
        /// @param[in] sName tString
        /// @return tObj*
        tObj* Obj(tString sName);
        
        /// @brief Get array property (return null if none)
        /// @param[in] sName tString
        /// @return tArray*
        tArray* Array(tString sName);
        
        /// @brief Return vector of property names
        /// @return tVectorString
        tVectorString Properties();
        
        /// @brief Return true if property exists
        /// @param[in] sName tString
        /// @return tBool
        tBool HasProperty(tString sName);
        
        /// @brief Delete property
        /// @param[in] sName tString
        /// @return tBool
        tBool DeleteProperty(tString sName);
        
        // Path access with dot notation ====================================
        /// @brief Get value by dot notation path (e.g., "A.B.C")
        /// @param[in] sPath tString (dot notation: "A.B.C")
        /// @return tVariant
        tVariant GetPath(tString sPath);
        
        /// @brief Set value by dot notation path (e.g., "A.B.C")
        /// @param[in] sPath tString (dot notation: "A.B.C")
        /// @param[in] sValue tVariant
        /// @return tBool
        tBool SetPath(tString sPath, tVariant sValue);
        
        /// @brief Check if path exists (e.g., "A.B.C")
        /// @param[in] sPath tString (dot notation: "A.B.C")
        /// @return tBool
        tBool HasPath(tString sPath);
        
        /// @brief Get object at specific path
        /// @param[in] sPath tString (dot notation: "A.B")
        /// @return tObj*
        tObj* GetPathObj(tString sPath);
        
        /// @brief Get array at specific path
        /// @param[in] sPath tString (dot notation: "A.items")
        /// @return tArray*
        tArray* GetPathArray(tString sPath);
        
        /// @brief Get array element value at specific path and index
        /// @param[in] sPath tString (dot notation: "A.items")
        /// @param[in] sIndex tSize index in array
        /// @return tVariant value at index, or null variant if not found
        tVariant GetPathArrayValue(tString sPath, tSize sIndex);
        
        /// @brief Set array element value at specific path and index
        /// @param[in] sPath tString (dot notation: "A.items")
        /// @param[in] sIndex tSize index in array
        /// @param[in] sValue tVariant value to set
        /// @return tBool true if successful
        tBool SetPathArrayValue(tString sPath, tSize sIndex, tVariant sValue);
        
        // Function ==========================================================
        /// @brief Call generic function
        /// @param[in] sName tString
        /// @param[in] sArg tObj
        /// @return tObj
        tObj Call(tString sName, tObj sArg);
   
        /// @brief Return number of properties
        /// @return tSize
        tSize Length();
        
#ifdef _DEBUGSK
        /// @brief Debug
        /// @return tString
        tString Debug() override;
#endif
        // Operator ===========================================================
        /// @brief Return value property for read
        /// @param[in] sName tString
        /// @return tVariant
        tVariant operator ()(tString sName);
    
        /// @brief Return value property for write
        /// @param[in] sName tString
        /// @return tVariant&
        tVariant& operator [](tString sName);
        
        /// @brief Return value property for read
        /// @param[in] sPos tSize
        /// @return tVariant
        tVariant operator ()(tSize sPos);
    
        /// @brief Return value property for write
        /// @param[in] sPos tSize
        /// @return tVariant&
        tVariant& operator [](tSize sPos);
    };


    // List of generic ========================================================
    class tArray : public tObj {
    private:
        tVectorVariant m_VectorVariant;
    public:
        /// @brief Constructor
        tArray();
        
        /// @brief Copy constructor
        /// @param[in] sArray tArray&
        tArray(const tArray& sArray);
        
        /// @brief Destructor
        ~tArray() override;
        
        /// @brief Clear all variants
        void Clear() override;
        
        tBool IsCopy() override;
        
        /// @brief Clone derived class
        /// @return tVirtualClass*
        tVirtualClass* Clone() override;

        /// @brief Return name of class (for factory)
        /// @return tString
        tString ClassName() const override;
        
        /// @brief Return vector of variants
        /// @return tVectorVariant*
        tVectorVariant* VectorVariant();
 
        // Json ==============================================================
        /// @brief Write JSON
        /// @param[in] sWriter Writer<StringBuffer>*
        /// @param[in] sJsonRadical tBool
        void JsonObj(Writer<StringBuffer>* sWriter, tBool sJsonRadical = false) override;

        /// @brief Read JSON
        /// @param[in] sValue Value&
        /// @param[in] sJsonRadical tBool
        void JsonObj(const rapidjson::Value& sValue, tBool sJsonRadical = false) override;
        
        /// @brief Return number of variants
        /// @return tSize
        tSize Length();
        
        /// @brief Add a variant to the array
        /// @param[in] sVariant tVariant
        void Add(tVariant sVariant);
        
        /// @brief Add an integer to the array
        /// @param[in] sValue tInt
        void Add(tInt sValue);
        
        /// @brief Add a string to the array
        /// @param[in] sValue tString
        void Add(tString sValue);
        
        // Operator ===========================================================
        /// @brief Return variant by position
        /// @param[in] sPos tSize
        /// @return tVariant
        tVariant operator ()(tSize sPos);
    
        /// @brief Return variant by position for write
        /// @param[in] sPos tSize
        /// @return tVariant&
        tVariant& operator [](tSize sPos);

    };

    // Define Model ===========================================================
    typedef tObj(tObj::*tMethod)(tObj);

    // Define Model Arg or return =============================================
    class tModelArg : public tVirtualClass {
    private:
      tSharedString  m_Name;
      tSharedString  m_Type;
      tSharedString  m_Label;
    public:
        /// @brief Constructor
        tModelArg();
        
        /// @brief Copy constructor
        /// @param[in] sModelArg tModelArg&
        tModelArg(const tModelArg& sModelArg);
        
        /// @brief Constructor with parameters
        /// @param[in] sName tString
        /// @param[in] sType tString
        /// @param[in] sLabel tString
        tModelArg(tString sName, tString sType, tString sLabel);
        
        /// @brief Get the name
        /// @return tString
        tString Name();
        
        /// @brief Set the name
        /// @param[in] sName tString
        void Name(tString sName);

        /// @brief Get the type
        /// @return tString
        tString Type();
        
        /// @brief Set the type
        /// @param[in] sType tString
        void Type(tString sType);

        /// @brief Get the label
        /// @return tString
        tString Label();
        
        /// @brief Set the label
        /// @param[in] sLabel tString
        void Label(tString sLabel);
    };

    typedef vector<tModelArg> tVectorModelArg;

    // Define Function Model ==================================================
    class tModelMethod : public tVirtualClass {
    private:
        tSharedString       m_Name;
        tSharedString       m_Label;
        tVectorModelArg     m_Return;
        tVectorModelArg     m_Arg;
        tMethod             m_Method;
    public:
        /// @brief Constructor
        tModelMethod();
        
        /// @brief Constructor with parameters
        /// @param[in] sName tString
        /// @param[in] sLabel tString
        tModelMethod(tString sName, tString sLabel);
        
        /// @brief Copy constructor
        /// @param[in] sModelFunction tModelMethod&
        tModelMethod(const tModelMethod& sModelFunction);
        
        /// @brief Get the name
        /// @return tString
        tString Name();
        
        /// @brief Set the name
        /// @param[in] sName tString
        void Name(tString sName);

        /// @brief Get the label
        /// @return tString
        tString Label();
        
        /// @brief Set the label
        /// @param[in] sLabel tString
        void Label(tString sLabel);

        /// @brief Get the order
        /// @return tInt
        tInt Order();
        
        /// @brief Set the order
        /// @param[in] sOrder tInt
        void Order(tInt sOrder);

        /// @brief Add return argument
        /// @param[in] sName tString
        /// @param[in] sType tString
        /// @param[in] sLabel tString
        void AddReturn(tString sName, tString sType, tString sLabel);
        
        /// @brief Get return arguments
        /// @return tVectorModelArg
        tVectorModelArg ModelReturn();
        
        /// @brief Add argument
        /// @param[in] sName tString
        /// @param[in] sType tString
        /// @param[in] sLabel tString
        void AddArg(tString sName, tString sType, tString sLabel);
        
        /// @brief Get arguments
        /// @return tVectorModelArg
        tVectorModelArg ModelArg();
        
        /// @brief Get the method
        /// @return tMethod
        tMethod Method();
        
        /// @brief Set the method
        /// @param[in] sMethod tMethod
        template<class T>
        void Method(tObj(T::* sMethod)(tObj)) {
            // Force Cast
            m_Method = (tMethod)(sMethod);
        }
        
        /// @brief Call the method
        /// @param[in] sThis tObj*
        /// @param[in] sArg tObj
        /// @return tObj
        tObj Call(tObj* sThis, tObj sArg) {
            return(((*sThis).*(m_Method))(sArg));
        }
             
    };
    
    typedef vector<tModelMethod> tVectorModelMethod;
    struct tComparatorFunction {
        inline bool operator()(tModelMethod *E1, tModelMethod *E2) { return (E1->Order() < E2->Order()); }
    };
    
    // Define Model with Method ===============================================
    class tModelObj : public tModelClass {
    private:
        tVectorModelMethod m_VectorMethod;
    public:
        /// @brief Constructor
        tModelObj();
        
        /// @brief Constructor with parameters
        /// @param[in] Name tString
        /// @param[in] sLabel tString
        /// @param[in] sFunctionCreate tFunctionCreate
        tModelObj(tString Name, tString sLabel, tFunctionCreate sFunctionCreate);
        
        /// @brief Copy constructor
        /// @param[in] sModelObj tModelObj&
        tModelObj(const tModelObj& sModelObj);
        
        /// @brief Return true for place model in JSON
        /// @return tBool
        tBool SaveModel() override;
        
        /// @brief Add method to model
        /// @param[in] sName tString
        /// @param[in] sLabel tString
        /// @param[in] sMethod tObj(T::* sMethod)(tObj)
        /// @return tBool
        template<class T>
        tBool AddMethod(tString sName, tString sLabel, tObj(T::* sMethod)(tObj)) {
            tModelMethod wMethod(sName, sLabel);
            // Set Method
            wMethod.Method<T>(sMethod);
            m_VectorMethod.push_back(wMethod);
            return(true);
        }
        
        /// @brief Get model method by name
        /// @param[in] sName tString
        /// @return tModelMethod*
        tModelMethod* ModelMethod(tString sName);
    };

    // JSON Object Class ======================================================
    //! tJsonObject class for JSON simulation with parse and stringify
    class tJsonObject : public tVirtualClass {
    private:
        ///! Root object containing the JSON tree
        tObj m_RootObject;
        
        ///! Parse error message
        tString m_Error;
        
        ///! Parse position for error reporting
        tSize m_ParsePosition;
        
        /// @brief Parse JSON string recursively
        /// @param[in] sJson tString&
        /// @param[in] sPos tSize&
        /// @param[in] sDepth tInt
        /// @return tObj
        tObj ParseValue(tString& sJson, tSize& sPos, tInt sDepth = 0);
        
        /// @brief Parse JSON object
        /// @param[in] sJson tString&
        /// @param[in] sPos tSize&
        /// @return tObj
        tObj ParseObject(tString& sJson, tSize& sPos);
        
        /// @brief Parse JSON array
        /// @param[in] sJson tString&
        /// @param[in] sPos tSize&
        /// @return tArray
        tArray ParseArray(tString& sJson, tSize& sPos);
        
        /// @brief Parse JSON string value
        /// @param[in] sJson tString&
        /// @param[in] sPos tSize&
        /// @return tString
        tString ParseString(tString& sJson, tSize& sPos);
        
        /// @brief Parse JSON number
        /// @param[in] sJson tString&
        /// @param[in] sPos tSize&
        /// @return tVariant
        tVariant ParseNumber(tString& sJson, tSize& sPos);
        
        /// @brief Parse JSON boolean or null
        /// @param[in] sJson tString&
        /// @param[in] sPos tSize&
        /// @return tVariant
        tVariant ParseBooleanOrNull(tString& sJson, tSize& sPos);
        
        /// @brief Skip whitespace characters
        /// @param[in] sJson tString&
        /// @param[in] sPos tSize&
        void SkipWhitespace(tString& sJson, tSize& sPos);
        
        /// @brief Convert object to JSON string recursively
        /// @param[in] sObj tObj&
        /// @param[in] sIndent tInt
        /// @return tString
        tString StringifyObject(tObj& sObj,tBool sLineReturn, tInt sIndent = 0);
        
        /// @brief Convert array to JSON string recursively
        /// @param[in] sArray tArray&
        /// @param[in] sIndent tInt
        /// @return tString
        tString StringifyArray(tArray& sArray,tBool sLineReturn, tInt sIndent = 0);
        
        /// @brief Convert variant to JSON string
        /// @param[in] sVariant tVariant&
        /// @return tString
        tString StringifyVariant(tVariant& sVariant);
        
        /// @brief Escape string for JSON
        /// @param[in] sString tString
        /// @return tString
        tString EscapeString(tString sString);
        
        /// @brief Generate indentation string
        /// @param[in] sLevel tInt
        /// @return tString
        tString GetIndent(tInt sLevel);
        
    public:
        /// @brief Constructor
        tJsonObject();
        
        /// @brief Copy constructor
        /// @param[in] sJsonObject tJsonObject&
        tJsonObject(const tJsonObject& sJsonObject);
        
        /// @brief Destructor
        ~tJsonObject() override;
        
        /// @brief Clear the JSON object
        void Clear();
        
        /// @brief Clone derived class
        /// @return tVirtualClass*
        tVirtualClass* Clone() override;
        
        /// @brief Return name of class (for factory)
        /// @return tString
        tString ClassName() const override;
        
        /// @brief Parse JSON string into object tree
        /// @param[in] sJson tString
        /// @return tBool
        tBool Parse(tString sJson);
        
        /// @brief Convert object tree to JSON string
        /// @param[in] sLineReturn tBool
        /// @return tString
        tString Stringify(tBool sLineReturn = false);
        
        /// @brief Get the root object
        /// @return tObj&
        tObj& Root();
        
        /// @brief Get parse error message
        /// @return tString
        tString Error();
        
        /// @brief Get parse error position
        /// @return tSize
        tSize ErrorPosition();
        
        /// @brief Check if parsing was successful
        /// @return tBool
        tBool IsValid();
        
        /// @brief Set a value in the JSON tree
        /// @param[in] sPath tString (dot notation: "user.name")
        /// @param[in] sValue tVariant
        /// @return tBool
        tBool SetValue(tString sPath, tVariant sValue);
        
        /// @brief Get a value from the JSON tree
        /// @param[in] sPath tString (dot notation: "user.name")
        /// @return tVariant
        tVariant GetValue(tString sPath);
        
        /// @brief Check if a path exists in the JSON tree
        /// @param[in] sPath tString (dot notation: "user.name")
        /// @return tBool
        tBool HasPath(tString sPath);
        
        /// @brief Remove a value from the JSON tree
        /// @param[in] sPath tString (dot notation: "user.name")
        /// @return tBool
        tBool RemoveValue(tString sPath);
        
        /// @brief Get all property names at root level
        /// @return tVectorString
        tVectorString GetPropertyNames();
        
        /// @brief Get object at specific path
        /// @param[in] sPath tString (dot notation: "user")
        /// @return tObj*
        tObj* GetObject(tString sPath);
        
        /// @brief Get array at specific path
        /// @param[in] sPath tString (dot notation: "items")
        /// @return tArray*
        tArray* GetArray(tString sPath);
        
        // Json ==============================================================
        /// @brief Write JSON
        /// @param[in] sWriter Writer<StringBuffer>*
        /// @param[in] sJsonRadical tBool
        virtual void JsonObj(Writer<StringBuffer>* sWriter, tBool sJsonRadical = false);
        
        /// @brief Read JSON
        /// @param[in] sValue Value&
        /// @param[in] sJsonRadical tBool
        virtual void JsonObj(const rapidjson::Value& sValue, tBool sJsonRadical = false);
        
#ifdef _DEBUGSK
        /// @brief Debug
        /// @return tString
        tString Debug() override;
#endif
    };

} // end of namespace

#endif

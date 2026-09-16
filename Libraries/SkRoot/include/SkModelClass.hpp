//=============================================================================
// SkModelClass
// SkModelClass 07/05/2023 class for Json 
//=============================================================================
#ifndef SkModelClass_hpp
#define SkModelClass_hpp

#include "SkTypes.hpp"
#include "SkClass.hpp"
#include "SkSharedString.hpp"
#include "SkVariant.hpp"

using namespace SkRoot;

namespace SkRoot {

    /// ! For Call setter and getter ========================================== 
    class tSetterGetter {
    public: 
        typedef void (tVirtualClass::* tIntSetter)(tInt);
        typedef void (tVirtualClass::* tBoolSetter)(tBool);
        typedef void (tVirtualClass::* tDoubleSetter)(tDouble);
        typedef void (tVirtualClass::* tStringSetter)(tString);       
        typedef void (tVirtualClass::* tDateSetter)(tDate);
        typedef void (tVirtualClass::* tErrorSetter)(tClassError);
        typedef void (tVirtualClass::* tVirtualClassSetter)(tVirtualClass*);

        union SkSetter {
            tIntSetter m_IntSetter;
            tBoolSetter m_BoolSetter;
            tDoubleSetter m_DoubleSetter;
            tStringSetter m_StringSetter;
#ifndef __EMSCRIPTEN__32__  // For EmscriptEn 32 bits
            tDateSetter m_DateSetter;
#endif            
            tErrorSetter m_ErrorSetter;
            tVirtualClassSetter m_VirtualClassSetter;
        } m_Setter;

        typedef tInt(tVirtualClass::* tIntGetter)();
        typedef tBool(tVirtualClass::* tBoolGetter)();
        typedef tDouble(tVirtualClass::* tDoubleGetter)();
        typedef tString(tVirtualClass::* tStringGetter)();
#ifndef __EMSCRIPTEN__32__  // For EmscriptEn 32 bits
        typedef tDate(tVirtualClass::* tDateGetter)();
#endif     
        typedef tClassError(tVirtualClass::* tErrorGetter)();
        typedef tVirtualClass* (tVirtualClass::* tVirtualClassGetter)();

        union SkGetter {
            tIntGetter m_IntGetter;
            tBoolGetter m_BoolGetter;
            tDoubleGetter m_DoubleGetter;
            tStringGetter m_StringGetter;
#ifndef __EMSCRIPTEN__32__  // For EmscriptEn 32 bits
            tDateGetter m_DateGetter;
#endif
            tErrorGetter m_ErrorGetter;
            tVirtualClassGetter m_VirtualClassGetter;
        } m_Getter;
        
        tSetterGetter() : m_Setter(), m_Getter() {};

        template <class T>
        void Set(void (T::* sMethodSetter)(tInt), tInt(T::* sMethodGetter)()) {
            m_Setter.m_IntSetter = (tIntSetter)(sMethodSetter);
            m_Getter.m_IntGetter = (tIntGetter)(sMethodGetter);
        }

        template <class T>
        void Set(void (T::* sMethodSetter)(tBool), tBool(T::* sMethodGetter)()) {
            m_Setter.m_BoolSetter = (tBoolSetter)(sMethodSetter);
            m_Getter.m_BoolGetter = (tBoolGetter)(sMethodGetter);
        }

        template <class T>
        void Set(void (T::* sMethodSetter)(tDouble), tDouble(T::* sMethodGetter)()) {
            m_Setter.m_DoubleSetter = (tDoubleSetter)(sMethodSetter);
            m_Getter.m_DoubleGetter = (tDoubleGetter)(sMethodGetter);
        }

        template <class T>
        void Set(void (T::* sMethodSetter)(tString), tString(T::* sMethodGetter)()) {
            m_Setter.m_StringSetter = (tStringSetter)(sMethodSetter);
            m_Getter.m_StringGetter = (tStringGetter)(sMethodGetter);
        }
#ifndef __EMSCRIPTEN__32__  // For EmscriptEn 32 bits
        template <class T>
        void Set(void (T::* sMethodSetter)(tDate), tDate(T::* sMethodGetter)()) {
            m_Setter.m_DateSetter = (tDateSetter)(sMethodSetter);
            m_Getter.m_DateGetter = (tDateGetter)(sMethodGetter);
        }
#endif
        template <class T>
        void Set(void (T::* sMethodSetter)(tClassError), tClassError(T::* sMethodGetter)()) {
            m_Setter.m_ErrorSetter = (tErrorSetter)(sMethodSetter);
            m_Getter.m_ErrorGetter = (tErrorGetter)(sMethodGetter);
        }

        template <class T>
        void Set(void (T::* sMethodSetter)(tVirtualClass*), tVirtualClass* (T::* sMethodGetter)()) {
            m_Setter.m_VirtualClassSetter = (tVirtualClassSetter)(sMethodSetter);
            m_Getter.m_VirtualClassGetter = (tVirtualClassGetter)(sMethodGetter);
        }
    };

    /// ! Property ============================================================
    class tModelProperty : public tVirtualClass {
    private:
        tSharedString   m_Name;
        tVariantType    m_Type;
        tSharedString   m_Label;
        tSize           m_Order;
        tVariant        m_DefaultValue;
        tSetterGetter   m_SetterGetter;
        tSharedString   m_JsonRadical;
        tSharedString   m_Kind;
    public:
        /// @brief Constructor
        tModelProperty();
        
        /// @brief Constructor with parameters
        /// @param[in] sName tString
        /// @param[in] sType tVariantType
        /// @param[in] sLabel tString
        /// @param[in] sOrder tSize
        /// @param[in] sDefaultValue tVariant
        /// @param[in] sJsonRadical tString
        /// @param[in] sSetterGetter tSetterGetter
        /// @param[in] sKind tString optional semantic kind (e.g. "range" for A1 refs)
        tModelProperty(tString sName,tVariantType sType,tString sLabel,tSize sOrder,tVariant sDefaultValue,tString sJsonRadical, tSetterGetter sSetterGetter, tString sKind = "");
        
        /// @brief Copy constructor
        /// @param[in] sPropertyModel tModelProperty&
        tModelProperty(const tModelProperty& sPropertyModel);

        /// @brief Get the property name
        /// @return tString
        tString Name();
        
        /// @brief Set the property name
        /// @param[in] sName tString
        void Name(tString sName);

        /// @brief Get the property type
        /// @return tVariantType
        tVariantType Type();
        
        /// @brief Set the property type
        /// @param[in] sType tVariantType
        void Type(tVariantType sType);

        /// @brief Get the property label
        /// @return tString
        tString Label();
        
        /// @brief Set the property label
        /// @param[in] sLabel tString
        void Label(tString sLabel);

        /// @brief Get the property order
        /// @return tSize
        tSize Order();
        
        /// @brief Set the property order
        /// @param[in] sOrder tSize
        void Order(tSize sOrder);

        /// @brief Get the default value
        /// @return tVariant&
        tVariant& DefaultValue();

        /// @brief Get the JSON radical
        /// @return tString
        tString JsonRadical();
        
        /// @brief Set the JSON radical
        /// @param[in] sJsonRadical tString
        void JsonRadical(tString sJsonRadical);

        /// @brief Get the property kind (schema metadata, not value type)
        /// @return tString
        tString Kind();

        /// @brief Set the property kind
        /// @param[in] sKind tString
        void Kind(tString sKind);
        
        /// @brief Get the setter and getter
        /// @return tSetterGetter
        tSetterGetter SetterGetter();
        
        // Json ==============================================================
        /// @brief        Writer Json.
        /// @param[in]    sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter);

        /// @brief        Reader Json.
        /// @param[in]    sValue Value&
        void Json(const rapidjson::Value& sValue);
    };

    struct tComparatorProperty {
        inline bool operator()(tModelProperty *E1, tModelProperty *E2) { return (E1->Order() < E2->Order()); }
    };


    typedef function<tVirtualClass* (void)> tFunctionCreate;
    typedef map<tString,tModelProperty*> tMapPropertyModel;
    typedef vector<tModelProperty*> tVectorPropertyModel;

    /// ! Model class =========================================================
    class tModelClass : public tVirtualClass {
    protected:
        tSharedString           m_ClassName;
        tSharedString           m_Label;
        tMapPropertyModel       m_PropertiesModel;
        tVectorPropertyModel    m_OrderedPropertiesModel;
        tFunctionCreate         m_FunctionCreate;
        tMapPropertyModel       m_MapJson;
    public:
        /// @brief Constructor
        tModelClass();
        
        /// @brief Constructor with parameters
        /// @param[in] Name tString
        /// @param[in] sLabel tString
        /// @param[in] sFunctionCreate tFunctionCreate
        tModelClass(tString Name,tString sLabel,tFunctionCreate sFunctionCreate);

        /// @brief Destructor
        virtual ~tModelClass();

        /// @brief Get the class name
        /// @return tString
        tString ClassName() const override;
        
        /// @brief Set the class name
        /// @param[in] sName tString
        void ClassName(tString sName);
        
        /// @brief Return true for place model in Json (See tVariant).
        /// @return tBool
        virtual tBool SaveModel();

        /// @brief Return true for place data in Json (See tVariant).
        /// @return tBool
        virtual tBool SaveData();

        /// @brief Get the function create
        /// @return tFunctionCreate
        tFunctionCreate FunctionCreate();
        
        /// @brief Set the function create
        /// @param[in] sFunctionCreate tFunctionCreate
        void FunctionCreate(tFunctionCreate sFunctionCreate);
        
        /// @brief Create an instance of the class
        /// @return tVirtualClass*
        tVirtualClass* CreateInstance();
        
        /// @brief Get the vector of property models
        /// @return tVectorPropertyModel*
        tVectorPropertyModel* VectorPropertyModel();

        /// @brief Get the property by name
        /// @param[in] sName tString
        /// @return tModelProperty*
        tModelProperty* Property(tString sName);
        
        /// @brief Get the property by JSON radical
        /// @param[in] sName tString
        /// @return tModelProperty*
        tModelProperty* ByJsonRadical(tString sName);

        /// @brief Add a property to the model (takes ownership of sModelProperty).
        /// @param[in] sModelProperty tModelProperty*
        /// @return tBool true if registered; false if name already exists (pointer deleted).
        tBool AddProperty(tModelProperty* sModelProperty);

        /// @brief Add a property to the model
        /// @param[in] sName tString
        /// @param[in] sType tVariantType
        /// @param[in] sLabel tString
        /// @param[in] sOrder tSize
        /// @param[in] sDefaultValue tVariant
        /// @param[in] sJsonRadical tString
        /// @param[in] sSetterGetter tSetterGetter
        /// @param[in] sKind tString optional semantic kind (e.g. "range")
        /// @return tBool
        tBool AddProperty(tString sName, tVariantType sType, tString sLabel, tSize sOrder, tVariant sDefaultValue, tString sJsonRadical, tSetterGetter sSetterGetter, tString sKind = "");

        /// @brief Set the property value
        /// @param[in] sThis tVirtualClass*
        /// @param[in] sName tString
        /// @param[in] sValue tVariant
        virtual void Property(tVirtualClass* sThis, tString sName, tVariant sValue);
        
        /// @brief Get the property value
        /// @param[in] sThis tVirtualClass*
        /// @param[in] sName tString
        /// @return tVariant
        virtual tVariant Property(tVirtualClass* sThis, tString sName);

        // Json ===============================================================
        /// @brief Writer Json. 
        /// @param[in] sWriter Writer<StringBuffer>*
        virtual void JsonAssociated(Writer<StringBuffer>* sWriter, tVirtualClass* sThis);

        /// @brief Reader Json. 
        /// @param[in] sValue Value&
        virtual void JsonAssociated(const rapidjson::Value& sValue, tVirtualClass* sThis);
        
        // Json ==============================================================
        /// @brief Writer Json.
        /// @param[in] sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter) override;

        /// @brief Reader Json.
        /// @param[in] sValue Value&
        void Json(const rapidjson::Value& sValue) override;
    };

    typedef map<tString, tModelClass*> tMapClassModel;

    /// @brief Optional hook: register a minimal model when Create() misses (ReadJson unknown classes).
    typedef tBool (*tMissingClassStubFn)(tString sClassName);

    // Declare Generic class
    class tObj;
    /// ! Factory class =======================================================
    class tClassFactory : public tClass {
    private:
        tMapClassModel   m_MapClassModel;
        static tMissingClassStubFn s_MissingClassStubHandler;
    public:
        /// @brief Constructor
        tClassFactory();
        
        /// @brief Destructor
        ~tClassFactory();

        /// @brief Clear the factory
        void Clear();

        /// @brief Create an instance of a class by name
        /// @param[in] sClassName tString
        /// @return tVirtualClass*
        tVirtualClass* Create(tString sClassName);

        /// @brief Hook invoked when Create() finds no model; may register a stub and retry.
        static void SetMissingClassStubHandler(tMissingClassStubFn sHandler);

        /// @brief Create an instance of a class by JSON
        /// @param[in] sJson tString
        /// @return tObj*
        tObj* CreateByJson(tString sJson);
        
        /// @brief Register a model class
        /// @param[in] sModelClass tModelClass*
        /// @return tBool
        tBool Register(tModelClass* sModelClass);

        /// @brief Register a model class
        /// @param[in] sClassName tString
        /// @param[in] sLabel tString
        /// @param[in] sFunctionCreate tFunctionCreate
        /// @return tBool
        tBool Register(tString sClassName,tString sLabel, tFunctionCreate sFunctionCreate);
        

        /// @brief Unregister a model class
        /// @param[in] sClassName tString
        /// @return tBool
        tBool UnRegister(tString sClassName);

        /// @brief Get a model class by name
        /// @param[in] sClassName tString
        /// @return tModelClass*
        tModelClass* Get(tString sClassName);
        
        // Json ==============================================================
        /// @brief Get JSON representation of the factory
        /// @return tString
        tString Json();

        /// @brief Get JSON representation of a class by name
        /// @param[in] sClassName tString
        /// @return tString
        tString Json(tString sClassName);
        
        // Treatment of Singleton =========================================
        /// @brief Return Singleton of SkClassFactory
        /// @return SkClassFactory* (pointer of unique SkClassFactory)
        static tClassFactory* Instance();
    };

}

#endif


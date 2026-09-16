//=============================================================================
// SkModelClass
// SkModelClass 07/05/2023 class for Json 
//=============================================================================
#include "../include/SkModelClass.hpp"
#include "../include/SkApplication.hpp"
#include "../include/SkGenericClass.hpp"
namespace SkRoot {
    /// ! Property ============================================================
    tModelProperty::tModelProperty() : tVirtualClass(), m_Name(), m_Type(), m_Label(), m_Order(0), m_DefaultValue(), m_SetterGetter(),m_JsonRadical(), m_Kind() {};
    tModelProperty::tModelProperty(tString sName, tVariantType sType, tString sLabel, tSize sOrder, tVariant sDefaultValue,tString sJsonRadical, tSetterGetter sSetterGetter, tString sKind) :
        tVirtualClass(), m_Name(sName), m_Type(sType), m_Label(sLabel), m_Order(sOrder), m_DefaultValue(sDefaultValue),  m_SetterGetter(sSetterGetter),m_JsonRadical(sJsonRadical), m_Kind(sKind) {};

    tModelProperty::tModelProperty(const tModelProperty& sPropertyModel) : tVirtualClass(sPropertyModel) {
        m_Name = sPropertyModel.m_Name;
        m_Type = sPropertyModel.m_Type;
        m_Label = sPropertyModel.m_Label;
        m_Order = sPropertyModel.m_Order;
        m_DefaultValue = sPropertyModel.m_DefaultValue;
        m_SetterGetter = sPropertyModel.m_SetterGetter;
        m_JsonRadical = sPropertyModel.m_JsonRadical;
        m_Kind = sPropertyModel.m_Kind;
    }

    tString tModelProperty::Name() { return(m_Name()); }
    void tModelProperty::Name(tString sName) { m_Name = sName; }

    tVariantType tModelProperty::Type() { return(m_Type); }
    void tModelProperty::Type(tVariantType sType) { m_Type = sType; }

    tString tModelProperty::Label() { return(m_Label()); }
    void tModelProperty::Label(tString sLabel) { m_Label = sLabel; }

    tSize tModelProperty::Order() { return(m_Order); }
    void tModelProperty::Order(tSize sOrder) { m_Order = sOrder; }

    tVariant& tModelProperty::DefaultValue() { return(m_DefaultValue); }

    tString tModelProperty::JsonRadical() { return(m_JsonRadical()); };
    void tModelProperty::JsonRadical(tString sJsonRadical) { m_JsonRadical=sJsonRadical; }

    tString tModelProperty::Kind() { return(m_Kind()); }
    void tModelProperty::Kind(tString sKind) { m_Kind = sKind; }

    tSetterGetter tModelProperty::SetterGetter() { return(m_SetterGetter); }

void tModelProperty::Json(Writer<StringBuffer>* sWriter) {
    sWriter->StartObject();
    sWriter->Key("n"); sWriter->String(m_Name().c_str());
    sWriter->Key("t");
    switch (Type()) {
    case tVariantType::t_null: sWriter->String("n"); break;
    case tVariantType::t_int:  sWriter->String("i"); break;
    case tVariantType::t_bool: sWriter->String("b"); break;
    case tVariantType::t_double: sWriter->String("d"); break;
    case tVariantType::t_string: sWriter->String("s"); break;
    case tVariantType::t_error: sWriter->String("e"); break;
    case tVariantType::t_date: sWriter->String("da"); break;
    case tVariantType::t_class: sWriter->String("c"); break;
    }
    sWriter->Key("l"); sWriter->String(m_Label().c_str());
    sWriter->Key("o"); sWriter->Int64(m_Order);
    sWriter->Key("d");
    sWriter->StartObject();
    m_DefaultValue.Json(sWriter);
    sWriter->EndObject();
    if (!m_Kind().empty()) {
        sWriter->Key("k");
        sWriter->String(m_Kind().c_str());
    }
    
    // Not Setter Getter
    
    sWriter->EndObject();
}

void tModelProperty::Json(const rapidjson::Value& sValue) {
    m_Name=sValue["n"].GetString();
    
    tString wTypeStr =sValue["t"].GetString();
    m_Type = tVariantType::t_null;
    //if (wTypeStr == "n");
    if (wTypeStr == "i") m_Type = tVariantType::t_int;
    if (wTypeStr == "b") m_Type = tVariantType::t_bool;
    if (wTypeStr == "d") m_Type = tVariantType::t_double;
    if (wTypeStr == "s") m_Type = tVariantType::t_string;
    if (wTypeStr == "e") m_Type = tVariantType::t_error;
    if (wTypeStr == "da") m_Type = tVariantType::t_date;
    if (wTypeStr == "c") m_Type = tVariantType::t_class;
    
    m_Label=sValue["l"].GetString();
    m_Order=sValue["o"].GetInt64();
    m_DefaultValue.Json(sValue["d"]);
    if (sValue.HasMember("k") && sValue["k"].IsString()) {
        m_Kind = sValue["k"].GetString();
    } else {
        m_Kind = "";
    }
}
    
    /// ! Class Model =========================================================
    tModelClass::tModelClass() : tVirtualClass(), m_ClassName(), m_FunctionCreate(nullptr){}
    tModelClass::tModelClass(tString sName,tString sLabel, tFunctionCreate sFunctionCreate) : tVirtualClass(), m_ClassName(sName),m_Label(sLabel), m_FunctionCreate(sFunctionCreate) {}

    tModelClass::~tModelClass() {
        for(auto wProperty : m_OrderedPropertiesModel) {
            delete(wProperty);
        }
        m_OrderedPropertiesModel.clear();
        m_PropertiesModel.clear();
    }

    tString tModelClass::ClassName() const { return(m_ClassName()); }
    void tModelClass::ClassName(tString sName) { m_ClassName = sName; }

    tBool tModelClass::SaveModel() {
        return(true);
    }

    tBool tModelClass::SaveData() {
        return(false);
    }

    tFunctionCreate tModelClass::FunctionCreate() { return(m_FunctionCreate); }
    void tModelClass::FunctionCreate(tFunctionCreate sFunctionCreate) { m_FunctionCreate = sFunctionCreate; }

    tVirtualClass* tModelClass::CreateInstance() {
        if (m_FunctionCreate != nullptr) return(m_FunctionCreate());
        tStringStream wStream;
        wStream << "tModelClass::ClassInstance() don't create " << m_ClassName() << "Instance !";
        cout << wStream.str() << endl;
        throw(tExceptionInternalError(wStream.str()));
        return(nullptr);
    }

    tVectorPropertyModel* tModelClass::VectorPropertyModel() {
        return(&m_OrderedPropertiesModel);
    }

    tModelProperty* tModelClass::Property(tString sName) {
        tMapPropertyModel::iterator wIterator;
        wIterator = m_PropertiesModel.find(sName);
        if (wIterator != m_PropertiesModel.end()) {
            return((*wIterator).second);
        }
        return(nullptr);
    }

    tModelProperty* tModelClass::ByJsonRadical(tString sName) {
        tMapPropertyModel::iterator wIterator;
        wIterator = m_MapJson.find(sName);
        if (wIterator != m_MapJson.end()) {
            return((*wIterator).second);
        }
        return(nullptr);
    }

    tBool tModelClass::AddProperty(tModelProperty* sModelProperty) {
        if (sModelProperty == nullptr) {
            return(false);
        }
        tString wName=sModelProperty->Name();
        tString wJsonRadical=sModelProperty->JsonRadical();
        tMapPropertyModel::iterator wIterator;
        wIterator = m_PropertiesModel.find(wName);
        if (wIterator == m_PropertiesModel.end()) {
            m_PropertiesModel[wName] = sModelProperty;
            m_MapJson[wJsonRadical] = sModelProperty;
            m_OrderedPropertiesModel.push_back(sModelProperty);
            sort(m_OrderedPropertiesModel.begin(), m_OrderedPropertiesModel.end(), tComparatorProperty());
            return(true);
        }
        delete(sModelProperty);
        return(false);
    }


    tBool tModelClass::AddProperty(tString sName, tVariantType sType, tString sLabel, tSize sOrder, tVariant sDefaultValue, tString sJsonRadical, tSetterGetter sSetterGetter, tString sKind) {
        tMapPropertyModel::iterator wIterator;
        wIterator = m_PropertiesModel.find(sName);
        if (wIterator == m_PropertiesModel.end()) {
            tModelProperty* wModelProperty=new  tModelProperty(sName, sType, sLabel, sOrder, sDefaultValue, sJsonRadical, sSetterGetter, sKind);
            m_PropertiesModel[sName] = wModelProperty;
            m_MapJson[sJsonRadical] = wModelProperty;
            m_OrderedPropertiesModel.push_back(wModelProperty);
            sort(m_OrderedPropertiesModel.begin(), m_OrderedPropertiesModel.end(), tComparatorProperty());
            return(true);
        }
        if (!sKind.empty()) {
            (*wIterator).second->Kind(sKind);
            return(true);
        }
        return(false);
    }

    void tModelClass::Property(tVirtualClass* sThis, tString sName, tVariant sValue) {
        tMapPropertyModel::iterator wIterator;
        wIterator = m_PropertiesModel.find(sName);
        if (wIterator != m_PropertiesModel.end()) {
            tModelProperty* wPropertyModel = (*wIterator).second;

            if (sValue.Type() != wPropertyModel->Type()) {
                return;
            }

            switch (wPropertyModel->Type())
            {
            case tVariantType::t_null: break;
            case tVariantType::t_int: {
                auto wMethod=wPropertyModel->SetterGetter().m_Setter.m_IntSetter;
                if (wMethod != nullptr) ((*sThis).*(wMethod))(sValue.Int());
                break;
            }
            case tVariantType::t_bool: {
                auto wMethod = wPropertyModel->SetterGetter().m_Setter.m_BoolSetter;
                if (wMethod != nullptr) ((*sThis).*(wMethod))(sValue.Bool());
                break;
            }
            case tVariantType::t_double: {
                auto wMethod = wPropertyModel->SetterGetter().m_Setter.m_DoubleSetter;
                if (wMethod != nullptr) ((*sThis).*(wMethod))(sValue.Double());
                break;
            }
            case tVariantType::t_string: {
                auto wMethod = wPropertyModel->SetterGetter().m_Setter.m_StringSetter;
                if (wMethod != nullptr) ((*sThis).*(wMethod))(sValue.String());
                break;
            }
            case tVariantType::t_error: {
                auto wMethod = wPropertyModel->SetterGetter().m_Setter.m_ErrorSetter;
                if (wMethod != nullptr) ((*sThis).*(wMethod))(sValue.Error());
                break;
            }
            case tVariantType::t_date: {
#ifndef __EMSCRIPTEN__32__  // For EmscriptEn 32 bits
                auto wMethod = wPropertyModel->SetterGetter().m_Setter.m_DateSetter;
                if (wMethod != nullptr) ((*sThis).*(wMethod))(sValue.Date());
#else
                auto wMethod=wPropertyModel->SetterGetter().m_Setter.m_IntSetter;
                if (wMethod != nullptr) ((*sThis).*(wMethod))(sValue.Int());
#endif                
                break;
            }
            case tVariantType::t_class: {
                auto wMethod = wPropertyModel->SetterGetter().m_Setter.m_VirtualClassSetter;
                if (wMethod != nullptr) ((*sThis).*(wMethod))(sValue.Class());
                break;
            }

            default:
                break;
            }
        }
    }
    tVariant tModelClass::Property(tVirtualClass* sThis, tString sName) {
        tVariant wVariant;
        tMapPropertyModel::iterator wIterator;
        wIterator = m_PropertiesModel.find(sName);
        if (wIterator != m_PropertiesModel.end()) {
            tModelProperty* wPropertyModel = m_PropertiesModel[sName];

            switch (wPropertyModel->Type())
            {
            case tVariantType::t_null: break;
            case tVariantType::t_int: {
                auto wMethod = wPropertyModel->SetterGetter().m_Getter.m_IntGetter;
                if (wMethod!=nullptr) wVariant = ((*sThis).*(wMethod))();
                break;
            }
            case tVariantType::t_bool: {
                auto wMethod = wPropertyModel->SetterGetter().m_Getter.m_BoolGetter;
                if (wMethod != nullptr) wVariant = ((*sThis).*(wMethod))();
                break;
            }
            case tVariantType::t_double: {
                auto wMethod=wPropertyModel->SetterGetter().m_Getter.m_DoubleGetter;
                if (wMethod != nullptr) wVariant = ((*sThis).*(wMethod))();
                break;
            }
            case tVariantType::t_string: {
                auto wMethod=wPropertyModel->SetterGetter().m_Getter.m_StringGetter;
                if (wMethod != nullptr) wVariant = ((*sThis).*(wMethod))();
                break;
            }
            case tVariantType::t_error: {
                auto wMethod = wPropertyModel->SetterGetter().m_Getter.m_ErrorGetter;
                if (wMethod != nullptr) wVariant = ((*sThis).*(wMethod))();
                break;
            }

            case tVariantType::t_date: {
#ifndef __EMSCRIPTEN__32__
                auto wMethod = wPropertyModel->SetterGetter().m_Getter.m_DateGetter;
                if (wMethod != nullptr) wVariant = ((*sThis).*(wMethod))();
#else
                auto wMethod = wPropertyModel->SetterGetter().m_Getter.m_IntGetter;
                if (wMethod != nullptr) wVariant = ((*sThis).*(wMethod))();
#endif
                break;
            }
            case tVariantType::t_class: {
                auto wMethod =wPropertyModel->SetterGetter().m_Getter.m_VirtualClassGetter;
                if (wMethod != nullptr) wVariant = ((*sThis).*(wMethod))();
                break;
            }

            default:
                break;
            }
        }
        return(wVariant);
    }

    void tModelClass::JsonAssociated(Writer<StringBuffer>* sWriter, tVirtualClass* sThis) {
        sWriter->StartObject();
        sWriter->Key("n");
        sWriter->String(ClassName().c_str());
        sWriter->Key("o"); 
        sWriter->StartObject();
        sWriter->Key("p");
        sWriter->StartArray();
        for (auto wProperty : m_OrderedPropertiesModel) {
            sWriter->StartObject();
            sWriter->Key("n");
            tString wName = wProperty->Name();
            sWriter->String(wName.c_str());

            tVariant wVariant = Property(sThis, wName);
            wVariant.Json(sWriter);
            sWriter->EndObject();
        }
        sWriter->EndArray();
        sWriter->EndObject();
        sWriter->EndObject();
    }

    
    void tModelClass::JsonAssociated(const rapidjson::Value& sValue, tVirtualClass* sThis) {
        const rapidjson::Value& wClassNameValue=sValue["n"];
        assert(wClassNameValue.IsString());
        tString wClassName = wClassNameValue.GetString();
        tModelClass* wModelClass = tClassFactory::Instance()->Get(wClassName);
        if (wModelClass != nullptr) {
            const rapidjson::Value& wObject = sValue["o"];
            assert(wObject.IsObject());
            const rapidjson::Value& wProperties = wObject["p"];
            assert(wProperties.IsArray());
            for (SizeType wIndex = 0; wIndex < wProperties.Size(); wIndex++) {
                const rapidjson::Value& wPropertyValue = wProperties[wIndex];
                const rapidjson::Value& wPropertyNameValue = wPropertyValue["n"];
                tString wPropertyName = wPropertyNameValue.GetString();
                tVariant wVariant;
                wVariant.Json(wPropertyValue);

                Property(sThis, wPropertyName, wVariant);
            }
        }
    }

    void tModelClass::Json(Writer<StringBuffer>* sWriter) {
        sWriter->StartObject();
        sWriter->Key("n"); sWriter->String(ClassName().c_str());
        sWriter->Key("l"); sWriter->String(m_Label().c_str());
        sWriter->Key("p");
        sWriter->StartArray();
        for (auto wProperty : m_OrderedPropertiesModel) {
            wProperty->Json(sWriter);
        }
        sWriter->EndArray();
        sWriter->EndObject();
    }

    void tModelClass::Json(const rapidjson::Value& sValue) {
        m_ClassName=sValue["n"].GetString();
        m_Label=sValue["l"].GetString();
        const ::Value& wProperties = sValue["p"];
        for (SizeType wInd = 0; wInd < wProperties.Size(); wInd++) {
            tModelProperty* wModelProperty=new tModelProperty();
            wModelProperty->Json(wProperties[wInd]);
            AddProperty(wModelProperty);
        }
    }

    /// ! Class Factory =======================================================
    tMissingClassStubFn tClassFactory::s_MissingClassStubHandler = nullptr;

    tClassFactory::tClassFactory() : tClass() {
    }

    tClassFactory::~tClassFactory() {
        Clear();
    }

    void tClassFactory::Clear() {
        tMapClassModel::iterator wIterator;
        for (wIterator = m_MapClassModel.begin(); wIterator != m_MapClassModel.end(); wIterator++) {
            delete((*wIterator).second);
        }
        m_MapClassModel.clear();
    }

    void tClassFactory::SetMissingClassStubHandler(tMissingClassStubFn sHandler) {
        s_MissingClassStubHandler = sHandler;
    }

    tVirtualClass* tClassFactory::Create(tString sClassName) {
        auto wTryCreate = [this](const tString& sName) -> tVirtualClass* {
            tMapClassModel::iterator wIterator = m_MapClassModel.find(sName);
            if (wIterator == m_MapClassModel.end()) {
                return (nullptr);
            }
            tModelClass* wClassModel = wIterator->second;
            tFunctionCreate wFunctionCreate = wClassModel->FunctionCreate();
            if (wFunctionCreate != nullptr) {
                return (wFunctionCreate());
            }
            return (nullptr);
        };

        tVirtualClass* wVirtualClass = wTryCreate(sClassName);
        if (wVirtualClass == nullptr && s_MissingClassStubHandler != nullptr) {
            if (s_MissingClassStubHandler(sClassName)) {
                wVirtualClass = wTryCreate(sClassName);
            }
        }
        return (wVirtualClass);
    }
        
    tObj* tClassFactory::CreateByJson(tString sJson) {
        Document wDocument;
        try {
            wDocument.Parse(sJson.c_str());
            if (wDocument.HasParseError()) {
                cout << "Rapidjson Error (offset " <<
                    wDocument.GetErrorOffset() << ":" <<
                        GetParseErrorFunc(wDocument.GetParseError()) << ")" << endl;
                return(nullptr);
            }
        }
        catch (const tExceptionInternalError e) {
            cout << e.what() << endl;
            return nullptr;
        }
        catch (...) {
            cout << "Unknown Exception..." << endl;
            return(nullptr);
        }
        
        tString wClassName="tObj";
        for (Value::ConstMemberIterator wIterator = wDocument.MemberBegin();
             wIterator != wDocument.MemberEnd(); ++wIterator) {
            
            
            tString wName=wIterator->name.GetString();
            
            const rapidjson::Value& wValue=wIterator->value;
            // If Type Break
            if (wName=="$.") {
                if (wValue.GetType()==kStringType) {
                    wClassName=wValue.GetString();
                    break;
                }
            }
        }
        
        tObj* wReturn=nullptr;
        tModelClass* wModelClass=Get(wClassName);
        if (wModelClass!=nullptr) {
            wReturn=dynamic_cast<tObj*>(wModelClass->CreateInstance());
        } else {
            wReturn=new tObj();
        }
        
        wReturn->Parse(sJson);
        
        return(wReturn);
    }

    tBool tClassFactory::Register(tModelClass* sModelClass) {
        tMapClassModel::iterator wIterator;
        wIterator = m_MapClassModel.find(sModelClass->ClassName());
        if (wIterator == m_MapClassModel.end()) {
            m_MapClassModel[sModelClass->ClassName()] = sModelClass;
            return(true);
        }
        return(false);
    }

    tBool tClassFactory::Register(tString sName,tString sLabel, tFunctionCreate sFunctionCreate) {
        tMapClassModel::iterator wIterator;
        wIterator = m_MapClassModel.find(sName);
        if (wIterator == m_MapClassModel.end()) {
            m_MapClassModel[sName] = new tModelClass(sName,sLabel, sFunctionCreate);
            return(true);
        }
        return(false);
    }

    tBool tClassFactory::UnRegister(tString sClassName) {
        tMapClassModel::iterator wIterator;
        wIterator = m_MapClassModel.find(sClassName);
        if (wIterator != m_MapClassModel.end()) {
            delete((*wIterator).second);
            m_MapClassModel.erase(wIterator);   
            return(true);
        }
        return(false);
    }

    tModelClass* tClassFactory::Get(tString sClassName) {
        tMapClassModel::iterator wIterator;
        wIterator = m_MapClassModel.find(sClassName);
        if (wIterator != m_MapClassModel.end()) {
            return((*wIterator).second);
        }
        return(nullptr);
    }

    // Json ==============================================================
    tString tClassFactory::Json() {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        wWriter.StartObject();
        wWriter.Key("models");
        wWriter.StartArray();
        for(auto wClass : m_MapClassModel) {
            tModelClass* wModelClass=wClass.second;
            wModelClass->Json(&wWriter);
        }
        wWriter.EndArray();
        wWriter.EndObject();
        return(wStringBuffer.GetString());
    }

    tString tClassFactory::Json(tString sClassName) {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        tModelClass* wModelClass=Get(sClassName);
        if (wModelClass!=nullptr) {
            wModelClass->Json(&wWriter);
        }
        return(wStringBuffer.GetString());
    }


    tClassFactory* tClassFactory::Instance() {
        return(tApplication::Instance()->ClassFactory());
    }

}

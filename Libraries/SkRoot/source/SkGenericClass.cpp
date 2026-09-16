//=============================================================================
// SkGenericClass (Generic class)
// 24/03/2024 class for Genric Json 
//=============================================================================
#include "../include/SkGenericClass.hpp"
#include <stdexcept>
#include <iostream>

namespace SkRoot {

    tProperty::tProperty() :  tClass(),m_Name(),m_Variant() {}
    tProperty::tProperty(const tProperty& sProperty) {
        m_Name=sProperty.m_Name;
        m_Variant=sProperty.m_Variant;
    }

    void tProperty::Name(tString sName) { m_Name=sName; }
    tString  tProperty::Name() { return (m_Name.Str()); }

    tVariant& tProperty::Variant() { return(m_Variant); }

    //! Generic class ================================================================
    tObj::tObj() : tVirtualClass(),m_ModelObj(nullptr),m_ObjType("gen") {}


    tObj::tObj(const  tObj& sGenericClass) : tVirtualClass(sGenericClass) {
        m_ObjType=sGenericClass.m_ObjType;
        m_Properties=sGenericClass.m_Properties;
        m_ModelObj=sGenericClass.m_ModelObj;
    }

    tObj::~tObj() {
        Clear();
    }
    void tObj::Clear() {
        for(tVectorProperty::iterator wIterator=m_Properties.begin(); wIterator!=m_Properties.end(); wIterator++) {
            (*wIterator).Variant().Clear();
        }
        m_Properties.clear();
    }

    tBool tObj::IsCopy() { return(true); }

    tVariant& tObj::Set(tString sName) {
        for(tVectorProperty::iterator wIterator=m_Properties.begin(); wIterator!=m_Properties.end(); wIterator++) {
            if ((*wIterator).Name()==sName) {
                return((*wIterator).Variant());
            }
        }
        // Create Property
        tProperty wProperty;
        wProperty.Name(sName);
        m_Properties.push_back(wProperty);
        return(m_Properties.back().Variant());
    }

    tVariant tObj::Get(tString sName) {
        for(tVectorProperty::iterator wIterator=m_Properties.begin(); wIterator!=m_Properties.end(); wIterator++) {
            if ((*wIterator).Name()==sName) {
                return((*wIterator).Variant());
            }
        }
        // Return Null
        return(tVariant());
    }

    void tObj::ObjType(tString sObjType) { m_ObjType=sObjType; }
    tString tObj::ObjType() { return (m_ObjType.Str()); }

    tObj tObj::Function(tString sName, tObj sArgument) {
        tObj wReturn=tObj();
        return(wReturn);
    }
    
    tVirtualClass* tObj::Clone() { return new tObj(*this); }

    tString tObj::ClassName() const { return("tObj"); }

    tVectorProperty* tObj::VectorProperty() { return(&m_Properties); }

    void tObj::ModelObj(tModelObj* sModelObj) { m_ModelObj=sModelObj; }

    void tObj::JsonObj(Writer<StringBuffer>* sWriter,tBool sJsonRadical) {
        if (m_ModelObj==nullptr) {
            m_ModelObj=dynamic_cast<tModelClass*>(tClassFactory::Instance()->Get(ClassName()));
            // If Null try ObjType
            if (m_ModelObj==nullptr) {
                m_ModelObj=dynamic_cast<tModelClass*>(tClassFactory::Instance()->Get(m_ObjType()));
            }
        }

        sWriter->StartObject();
        if (!sJsonRadical) {
            if (m_ObjType()!="gen") {
                // Write type for genericity
                sWriter->Key("$.");
                sWriter->String(m_ObjType().c_str());
            }
        }
        for(tVectorProperty::iterator wIterator=m_Properties.begin(); wIterator!=m_Properties.end(); wIterator++) {
            
            tProperty wProperty=(*wIterator);
            // Write name
            tString wKey=wProperty.Name();
            if ((sJsonRadical) && (m_ModelObj!=nullptr)) {
                tModelProperty* wModelProperty= m_ModelObj->Property(wProperty.Name());
                if (wModelProperty!=nullptr) {
                    tString wJsonRadical=wModelProperty->JsonRadical();
                    if (wJsonRadical!="") wKey=wJsonRadical;
                }
            }
            sWriter->Key(wKey.c_str());
            tVariant& wVariant=wProperty.Variant();
            switch (wVariant.Type()) {
                case tVariantType::t_null: sWriter->Null(); break;
                case tVariantType::t_int: sWriter->Int(wVariant.Int()); break;
                case tVariantType::t_bool: sWriter->Bool(wVariant.Bool()); break;
                case tVariantType::t_double: sWriter->Double(wVariant.Double()); break;
                case tVariantType::t_string: sWriter->String(wVariant.String().c_str()); break;
                case tVariantType::t_date: {
                    tClassDate wDate(wVariant.Date());
                    sWriter->String(wDate.UsDate().c_str());
                    break;
                }
                case tVariantType::t_class: {
                    // Obj or descendanr tArray
                    tObj* wObj = dynamic_cast<tObj*>(wVariant.Class());
                    if (wObj != nullptr) {
                        wObj->JsonObj(sWriter,sJsonRadical);
                    }
                    break;
                }
                default:
                    sWriter->Null();
                    break;
            }
        }
        sWriter->EndObject();
    }

    tObj* tObj::InstanteObjByJson(Value::ConstMemberIterator sIterator) {
        tString wClassName="tObj";
        tObj* wObjClass=nullptr;
        const rapidjson::Value& wValueType=sIterator->value;
        // Get Type
        if (wValueType.GetType()==kStringType) {
            if (tString(sIterator->name.GetString()) =="$.") {
                wClassName=wValueType.GetString();
            }
        }
        
        tModelObj* wModelObj=dynamic_cast<tModelObj*>(tClassFactory::Instance()->Get(wClassName));
        
        if (wModelObj!=nullptr) {
            wObjClass= dynamic_cast<tObj*>(wModelObj->CreateInstance());
        }
        if (wObjClass==nullptr) {
            wObjClass=new tObj();
        }
        wObjClass->ModelObj(wModelObj);
        return(wObjClass);
    }

    // Path helper methods ===============================================
    tBool tObj::SplitPath(tString sPath, tVectorString& sPathParts) {
        sPathParts.clear();
        tString wCurrent;
        
        // Reserve space for common path depths (optimization)
        sPathParts.reserve(4);
        
        // Split path by dots
        for (tChar wChar : sPath) {
            if (wChar == '.') {
                if (!wCurrent.empty()) {
                    sPathParts.push_back(wCurrent);
                    wCurrent.clear();
                }
            } else {
                wCurrent += wChar;
            }
        }
        if (!wCurrent.empty()) {
            sPathParts.push_back(wCurrent);
        }
        
        return !sPathParts.empty();
    }
    
    tObj* tObj::NavigateToPath(const tVectorString& sPathParts, tSize sDepth, tBool sCreateMissing) {
        tObj* wCurrentObj = this;
        
        for (tSize i = 0; i < sDepth && i < sPathParts.size(); i++) {
            if (sCreateMissing && !wCurrentObj->HasProperty(sPathParts[i])) {
                tObj* wNewObj = new tObj();
                tVariant& wVariantRef = wCurrentObj->Set(sPathParts[i]);
                wVariantRef.SetClass(wNewObj);
            }
            
            // Use Obj() to get pointer directly without copying variant (avoids recursive clone)
            tObj* wNextObj = wCurrentObj->Obj(sPathParts[i]);
            if (wNextObj != nullptr) {
                wCurrentObj = wNextObj;
            } else {
                return nullptr;
            }
        }
        
        return wCurrentObj;
    }

    void tObj::JsonObj(const rapidjson::Value& sValue,tBool sJsonRadical) {
        Clear();
        if (m_ModelObj==nullptr) {
           m_ModelObj=dynamic_cast<tModelObj*>(tClassFactory::Instance()->Get(ClassName()));
            // If Null try ObjType
            if (m_ModelObj==nullptr) {
                m_ModelObj=dynamic_cast<tModelClass*>(tClassFactory::Instance()->Get(m_ObjType()));
            }
        }

        for (Value::ConstMemberIterator wIterator = sValue.MemberBegin();
            wIterator != sValue.MemberEnd(); ++wIterator) {
            
            tProperty wProperty;
            tString wKey=wIterator->name.GetString();
            // Search if Json Radical
            if ((sJsonRadical) && (m_ModelObj!=nullptr)) {
                tModelProperty* wModelProperty= m_ModelObj->ByJsonRadical(wKey);
                if (wModelProperty!=nullptr) {
                    tString wJsonRadical=wModelProperty->JsonRadical();
                    if (wJsonRadical!="") wKey=wModelProperty->Name();
                }
            }
            
            wProperty.Name(wKey);
            
            tVariant& wVariant=wProperty.Variant();
            
            //cout << "...." << wProperty.Name() << endl;
            const rapidjson::Value& wValue=wIterator->value;
            // If Type Break
            if (wProperty.Name()=="$.") {
                continue;
            }
        
            switch (wValue.GetType()) {
                case kNullType : break;      //!< null
                case kFalseType : wVariant.SetBool(false); break;     //!< false
                case kTrueType: wVariant.SetBool(true); break; //!< true
                case kObjectType: {
                    Value::ConstMemberIterator wIteratorType=wValue.MemberBegin();
                    tObj* wObjClass=nullptr;
                    if (wIteratorType!=sValue.MemberEnd()) {
                        wObjClass=InstanteObjByJson(wIteratorType);
                    } else {
                        wObjClass=new tObj();
                    }
                    wObjClass->JsonObj(wValue,sJsonRadical);
                    wVariant.SetClass(wObjClass);
                    break;
                }//!< object
                case kArrayType: {
                    tArray* wArray=new tArray();
                    wArray->JsonObj(wValue,sJsonRadical);
                    wVariant.SetClass(wArray);
                    break;
                }//!< array
                case kStringType: {
                    wVariant.Parse(wValue.GetString());
                    break;
                }   //!< string
                case kNumberType: {
                    if  (wValue.IsInt()) wVariant.SetInt(wValue.GetInt());
                    if  (wValue.IsUint()) wVariant.SetInt(wValue.GetUint());
                    if  (wValue.IsInt64()) wVariant.SetInt(tInt(wValue.GetInt64()));
                    if  (wValue.IsUint64()) wVariant.SetInt(tInt(wValue.GetUint64()));
                    if  (wValue.IsDouble()) wVariant.SetDouble(wValue.GetDouble());
                    break;
                }//!< number
                default:
                    break;
            }
            m_Properties.push_back(wProperty);
        }
    }
    
    tBool tObj::Parse(tString sJson,tBool sJsonRadical) {
        Document wDocument;
        try {
            wDocument.Parse(sJson.c_str());
            if (wDocument.HasParseError()) {
                cout << "Rapidjson Error (offset " <<
                    wDocument.GetErrorOffset() << ":" <<
                        GetParseErrorFunc(wDocument.GetParseError()) << ")" << endl;
                return(false);
            }
            JsonObj(wDocument,sJsonRadical);
        }
        catch (const tExceptionInternalError e) {
            cout << e.what() << endl;
        }
        catch (...) {
            cout << "Unknown Exception..." << endl;
        }
        return(true);
    }

    tString tObj::Stringify(tBool sJsonRadical) {
        StringBuffer wStringBuffer;
        Writer<StringBuffer> wWriter(wStringBuffer);
        JsonObj(&wWriter,sJsonRadical);
        return(wStringBuffer.GetString());
    }
     
    tObj* tObj::Obj(tString sName) {
        for(tVectorProperty::iterator wIterator=m_Properties.begin(); wIterator!=m_Properties.end(); wIterator++) {
            if ((*wIterator).Name()==sName) {
                tProperty* wProperty=&(*wIterator);
                if (wProperty->m_Variant.IsClass()) {
                    return(dynamic_cast<tObj*>(wProperty->m_Variant.Class()));
                }
            }
        }
        // Return Null
        return(nullptr);
    }

    tArray* tObj::Array(tString sName) {
        // First check if property exists and is an array
        for(tVectorProperty::iterator wIterator=m_Properties.begin(); wIterator!=m_Properties.end(); wIterator++) {
            if ((*wIterator).Name()==sName) {
                tProperty* wProperty=&(*wIterator);
                if (wProperty->m_Variant.IsClass()) {
                    tVirtualClass* wClass = wProperty->m_Variant.Class();
                    if (wClass) {
                        // Try dynamic_cast first
                        tArray* wArray = dynamic_cast<tArray*>(wClass);
                        if (wArray) {
                            return wArray;
                        }
                    }
                }
            }
        }
        // Create new array if it doesn't exist
        tArray* wNewArray = new tArray();
        // Use Set to add new property
        tVariant& wVariant = Set(sName);
        wVariant.SetClass(wNewArray);
        return(wNewArray);
    }


    tVectorString tObj::Properties() {
        tVectorString wResult;
        for(tVectorProperty::iterator wIterator=m_Properties.begin(); wIterator!=m_Properties.end(); wIterator++) {
            wResult.push_back((*wIterator).Name());
        }
        // Copy vector
        return(wResult);
    }

    tBool tObj::HasProperty(tString sName) {
        for(tVectorProperty::iterator wIterator=m_Properties.begin(); wIterator!=m_Properties.end(); wIterator++) {
            if ((*wIterator).Name()==sName) {
                return(true);
            }
        }
        // Return not ok
        return(false);
    }
    
    tBool tObj::DeleteProperty(tString sName) {
        for(tVectorProperty::iterator wIterator=m_Properties.begin(); wIterator!=m_Properties.end(); wIterator++) {
            if ((*wIterator).Name()==sName) {
                m_Properties.erase(wIterator);
                return(true);
            }
        }
        // Return not ok
        return(false);
    }

    // Path access with dot notation ====================================
    tVariant tObj::GetPath(tString sPath) {
        tVectorString wPathParts;
        if (!SplitPath(sPath, wPathParts)) {
            return tVariant();
        }
        
        // Navigate to parent object
        tObj* wCurrentObj = NavigateToPath(wPathParts, wPathParts.size() - 1, false);
        if (wCurrentObj == nullptr) {
            return tVariant();
        }
        
        // Get the final value
        return wCurrentObj->Get(wPathParts.back());
    }
    
    tBool tObj::SetPath(tString sPath, tVariant sValue) {
        tVectorString wPathParts;
        if (!SplitPath(sPath, wPathParts)) {
            return false;
        }
        
        // Navigate to parent object (create missing objects)
        tObj* wCurrentObj = NavigateToPath(wPathParts, wPathParts.size() - 1, true);
        if (wCurrentObj == nullptr) {
            return false;
        }
        
        // Set the final value
        wCurrentObj->Set(wPathParts.back()) = sValue;
        return true;
    }
    
    tBool tObj::HasPath(tString sPath) {
        tVectorString wPathParts;
        if (!SplitPath(sPath, wPathParts)) {
            return false;
        }
        
        // Navigate to parent object
        tObj* wCurrentObj = NavigateToPath(wPathParts, wPathParts.size() - 1, false);
        if (wCurrentObj == nullptr) {
            return false;
        }
        
        // Check the final property
        return wCurrentObj->HasProperty(wPathParts.back());
    }
    
    tObj* tObj::GetPathObj(tString sPath) {
        tVectorString wPathParts;
        if (!SplitPath(sPath, wPathParts)) {
            return nullptr;
        }
        
        // Navigate to target object
        return NavigateToPath(wPathParts, wPathParts.size(), false);
    }
    
    tArray* tObj::GetPathArray(tString sPath) {
        tVectorString wPathParts;
        if (!SplitPath(sPath, wPathParts)) {
            return nullptr;
        }
        
        // Navigate to parent object
        tObj* wCurrentObj = NavigateToPath(wPathParts, wPathParts.size() - 1, false);
        if (wCurrentObj == nullptr) {
            return nullptr;
        }
        
        // Get the array from the final object
        return wCurrentObj->Array(wPathParts.back());
    }
    
    tVariant tObj::GetPathArrayValue(tString sPath, tSize sIndex) {
        tArray* wArray = GetPathArray(sPath);
        if (wArray != nullptr && sIndex < wArray->Length()) {
            return (*wArray)[sIndex];
        }
        return tVariant();
    }
    
    tBool tObj::SetPathArrayValue(tString sPath, tSize sIndex, tVariant sValue) {
        tArray* wArray = GetPathArray(sPath);
        if (wArray != nullptr && sIndex < wArray->Length()) {
            (*wArray)[sIndex] = sValue;
            return true;
        }
        return false;
    }

    // Function ==========================================================
    tObj tObj::Call(tString sName,tObj sArg) {
        if (m_ModelObj==nullptr) {
           m_ModelObj=dynamic_cast<tModelObj*>(tClassFactory::Instance()->Get(ClassName()));
        }
        if (m_ModelObj!=nullptr) {
            tModelObj* wModelObj=dynamic_cast<tModelObj*>(m_ModelObj);
            if (wModelObj!=nullptr) {
                tModelMethod* wModelMethod=(wModelObj->ModelMethod(sName));
                if (wModelMethod!=nullptr) {
                    return(wModelMethod->Call(this,sArg));
                }
            }
        }
        return(sArg);
    }

    tVariant tObj::operator ()(tString sName) {
        return(Get(sName));
    }

    tVariant& tObj::operator [](tString sName) {
        return(Set(sName));
    }

    tSize tObj::Length() { return(m_Properties.size()); }
#ifdef _DEBUGSK
    tString tObj::Debug() {
        tStringStream wStream;
        for(tVectorProperty::iterator wIterator=m_Properties.begin(); wIterator!=m_Properties.end(); wIterator++) {
            wStream << (*wIterator).Name() << ":" << (*wIterator).Variant() << " ";
        }
        return(wStream.str());
    }
#endif

    tVariant tObj::operator ()(tSize sPos) {
        if (sPos>=m_Properties.size()) {
            tStringStream wStream;
            wStream << "tObj::operator("<< sPos << ") out position >= " << Length() <<" !";
            cout << wStream.str() << endl;
            throw(tExceptionInternalError(wStream.str()));
        }
        return(m_Properties[sPos].Variant());
    }

    tVariant& tObj::operator [](tSize sPos) {
        if (sPos>=m_Properties.size()) {
            tStringStream wStream;
            wStream << "tObj::operator("<< sPos << ") out position >= " << Length() <<" !";
            cout << wStream.str() << endl;
            throw(tExceptionInternalError(wStream.str()));
        }
        return(m_Properties[sPos].Variant());
    }

    // Array of generic ========================================================
    tArray::tArray() :  tObj() {}
    
    tArray::tArray(const  tArray& sArray) : tObj(sArray) {
        if (m_VectorVariant.size()>0) {
            for(tVariant& wVariant:m_VectorVariant) {
                wVariant.Clear();
            }
        }
        m_VectorVariant=sArray.m_VectorVariant;
    }
    tArray::~tArray() {
        Clear();
    }

    void tArray::Clear() {
        tObj::Clear();
        m_VectorVariant.clear();
    }

    tBool tArray::IsCopy() { return(true); }

    tVirtualClass* tArray::Clone() { return new tArray(*this); }

    tString tArray::ClassName() const { return("tArray"); }

    tVectorVariant* tArray::VectorVariant() { return(&m_VectorVariant); }

    
    void tArray::JsonObj(Writer<StringBuffer>* sWriter,tBool sJsonRadical) {
        sWriter->StartArray();
        for(tVariant wVariant:m_VectorVariant) {
            switch (wVariant.Type()) {
                case tVariantType::t_null: sWriter->Null(); break;
                case tVariantType::t_int: sWriter->Int(wVariant.Int()); break;
                case tVariantType::t_bool: sWriter->Bool(wVariant.Bool()); break;
                case tVariantType::t_double: sWriter->Double(wVariant.Double()); break;
                case tVariantType::t_string: sWriter->String(wVariant.String().c_str()); break;
                case tVariantType::t_date: {
                    tClassDate wDate(wVariant.Date());
                    sWriter->String(wDate.UsDate().c_str());
                    break;
                }
                case tVariantType::t_error: break;
                case tVariantType::t_class: {
                    tVirtualClass* wVirtualClass=wVariant.Class();
                    tObj* wObj=dynamic_cast<tObj*>(wVirtualClass);
                    if (wObj!=nullptr) {
                        wObj->JsonObj(sWriter,sJsonRadical);
                    } else {
                        wVirtualClass->Json(sWriter);
                    }
                    break;
                }
            }
        }
        sWriter->EndArray();
    }

    void tArray::JsonObj(const rapidjson::Value& sValue,tBool sJsonRadical) {
        Clear();
        for (Value::ConstValueIterator wIterator = sValue.Begin();
            wIterator != sValue.End(); ++wIterator) {
            
            //wProperty.Name(wIterator->name.GetString());
            tVariant wVariant;
            const rapidjson::Value& wValue=(*wIterator);
           
            switch (wValue.GetType()) {
                case kNullType : break;      //!< null
                case kFalseType : wVariant.SetBool(false); break;     //!< false
                case kTrueType: wVariant.SetBool(true); break; //!< true
                case kObjectType: {
                    const rapidjson::Value& wValueType=(*wIterator);
                    tObj* wObjClass=InstanteObjByJson(wValueType.MemberBegin());
       
                    wObjClass->JsonObj(wValue,sJsonRadical);
                    wVariant.SetClass(wObjClass);
                    break;
                }//!< object
                case kArrayType: {
                    tArray* wArrayClass=new tArray();
                    wArrayClass->JsonObj(wValue,sJsonRadical);
                    wVariant.SetClass(wArrayClass);
                    break;
                }//!< array
                case kStringType: {
                    // String Date
                    wVariant.Parse(wValue.GetString());
                    break;
                }   //!< string
                case kNumberType: {
                    if  (wValue.IsInt()) wVariant.SetInt(wValue.GetInt());
                    if  (wValue.IsUint()) wVariant.SetInt(wValue.GetUint());
                    if  (wValue.IsInt64()) wVariant.SetInt(tInt(wValue.GetInt64()));
                    if  (wValue.IsUint64()) wVariant.SetInt(tInt(wValue.GetUint64()));
                    if  (wValue.IsDouble()) wVariant.SetDouble(wValue.GetDouble());
                    break;
                }//!< number
                default:
                    break;
            }
            m_VectorVariant.push_back(wVariant);
        }

    }

    tSize tArray::Length() { return(m_VectorVariant.size()); }

    void tArray::Add(tVariant sVariant) {
        m_VectorVariant.push_back(sVariant);
    }
    
    void tArray::Add(tInt sValue) {
        m_VectorVariant.push_back(tVariant(sValue));
    }
    
    void tArray::Add(tString sValue) {
        m_VectorVariant.push_back(tVariant(sValue));
    }

    // Operator ===========================================================
    tVariant tArray::operator ()(tSize sPos) {
        if (sPos>=m_VectorVariant.size()) {
            tStringStream wStream;
            wStream << "tArray::operator("<< sPos << ") out position >= " << Length() <<" !";
            cout << wStream.str() << endl;
            throw(tExceptionInternalError(wStream.str()));
        }
        return(m_VectorVariant[sPos]);
    }

    tVariant& tArray::operator [](tSize sPos) {
        if (sPos>=m_VectorVariant.size()) {
            tStringStream wStream;
            wStream << "tArray::operator("<< sPos << ") out position >= " << Length() <<" !";
            cout << wStream.str() << endl;
            throw(tExceptionInternalError(wStream.str()));
        }
        return(m_VectorVariant[sPos]);
    }


    // Define Model Arg or return ===============================================
    tModelArg::tModelArg() : tVirtualClass(),m_Name(),m_Type(),m_Label() {}
    tModelArg::tModelArg(tString sName,tString sType,tString sLabel) : tVirtualClass(),m_Name(sName),m_Type(sType),m_Label(sLabel) {}
    tModelArg::tModelArg(const tModelArg& sModelArg) : tVirtualClass(sModelArg) {
        m_Name=sModelArg.m_Name;
        m_Type=sModelArg.m_Type;
        m_Label=sModelArg.m_Label;
    }

    tString tModelArg::Name() { return(m_Name()); }
    void tModelArg::Name(tString sName) { m_Name=sName; }

    tString tModelArg::Type() { return(m_Type()); }
    void tModelArg::Type(tString sType) { m_Type=sType; };

    tString tModelArg::Label(){ return(m_Label()); }
    void tModelArg::Label(tString sLabel) { m_Label=sLabel; }
      
    // Define Function Model ==================================================
    tModelMethod::tModelMethod() : tVirtualClass(),m_Name(),m_Label(),m_Method(nullptr) {}
    tModelMethod::tModelMethod(tString sName,tString sLabel) : tVirtualClass(),m_Name(sName),m_Label(sLabel),m_Method(nullptr) {};

    tModelMethod::tModelMethod(const tModelMethod& sModelFunction) {
        m_Name=sModelFunction.m_Name;
        m_Label=sModelFunction.m_Label;
        m_Return=sModelFunction.m_Return;
        m_Arg=sModelFunction.m_Arg;
        m_Method=sModelFunction.m_Method;
    }

    tString tModelMethod::Name() { return(m_Name()); }
    void tModelMethod::Name(tString sName){ m_Name=sName; }

    tString tModelMethod::Label(){ return(m_Label()); }
    void tModelMethod::Label(tString sLabel){ m_Label=sLabel; }

    void tModelMethod::AddReturn(tString sName,tString sType,tString sLabel){
        tModelArg wModelArg(sName,sType,sLabel);
        m_Return.push_back(wModelArg);
    }
    tVectorModelArg tModelMethod::ModelReturn(){ return(m_Return); }
        
    void tModelMethod::AddArg(tString sName,tString sType,tString sLabel){
        tModelArg wModelArg(sName,sType,sLabel);
        m_Arg.push_back(wModelArg);
    }
    tVectorModelArg tModelMethod::ModelArg(){ return(m_Arg); }

  
    tMethod tModelMethod::Method(){ return(m_Method); }
    /*
    template<class T>
    void tModelMethod::Method(tObj(T::* sMethod)(tObj)) {
        m_Method=sMethod;
    }
    */
    // Define Model with Method ===============================================
    tModelObj::tModelObj() : tModelClass() {}
    tModelObj::tModelObj(tString sName,tString sLabel,tFunctionCreate sFunctionCreate) : tModelClass(sName,sLabel,sFunctionCreate) {}
    tModelObj::tModelObj(const tModelObj& sModelObj) : tModelClass(sModelObj) {
        m_VectorMethod=sModelObj.m_VectorMethod;
    };

        
    tBool tModelObj::SaveModel() {
        return(false);
    }
    tModelMethod* tModelObj::ModelMethod(tString sName) {
        tVectorModelMethod::iterator wIterator;
        for(wIterator=m_VectorMethod.begin();wIterator!=m_VectorMethod.end();wIterator++) {
            if ((*wIterator).Name()==sName) {
                return(&(*wIterator));
            }
        }
        return(nullptr);
    }



    // JSON Object Implementation ==============================================
    tJsonObject::tJsonObject() : tVirtualClass(), m_RootObject(), m_Error(), m_ParsePosition(0) {
        m_RootObject.ObjType("json");
    }
    
    tJsonObject::tJsonObject(const tJsonObject& sJsonObject) : tVirtualClass(sJsonObject) {
        m_RootObject = sJsonObject.m_RootObject;
        m_Error = sJsonObject.m_Error;
        m_ParsePosition = sJsonObject.m_ParsePosition;
    }
    
    tJsonObject::~tJsonObject() {
        Clear();
    }
    
    void tJsonObject::Clear() {
        m_RootObject.Clear();
        m_Error.clear();
        m_ParsePosition = 0;
    }
    
    tVirtualClass* tJsonObject::Clone() {
        return new tJsonObject(*this);
    }
    
    tString tJsonObject::ClassName() const {
        return "tJsonObject";
    }
    
    tBool tJsonObject::Parse(tString sJson) {
        Clear();
        m_ParsePosition = 0;
        
        try {
            SkipWhitespace(sJson, m_ParsePosition);
            if (m_ParsePosition >= sJson.length()) {
                m_Error = "Empty JSON string";
                return false;
            }
            
            if (sJson[m_ParsePosition] == '{') {
                m_RootObject = ParseObject(sJson, m_ParsePosition);
            } else if (sJson[m_ParsePosition] == '[') {
                tArray wArray = ParseArray(sJson, m_ParsePosition);
                m_RootObject["root"] = tVariant(&wArray);
            } else {
                m_Error = "JSON must start with '{' or '['";
                return false;
            }
            
            SkipWhitespace(sJson, m_ParsePosition);
            if (m_ParsePosition < sJson.length()) {
                m_Error = "Unexpected characters after JSON";
                return false;
            }
            
            return true;
        } catch (...) {
            m_Error = tString("Parse error at position ") + to_string(m_ParsePosition);
            return false;
        }
    }
    
    tString tJsonObject::Stringify(tBool sLineReturn) {
        tString wResult;
        
        if (m_RootObject.HasProperty("root")) {
            tVariant wRootVariant = m_RootObject["root"];
            if (wRootVariant.IsClass()) {
                tArray* wArray = wRootVariant.Class<tArray>();
                if (wArray) {
                    wResult = StringifyArray(*wArray, sLineReturn, 0 );
                }
            }
        } else {
            wResult = StringifyObject(m_RootObject, sLineReturn,  0);
        }
        
        return wResult;
    }
    
    tObj& tJsonObject::Root() {
        return m_RootObject;
    }
    
    tString tJsonObject::Error() {
        return m_Error;
    }
    
    tSize tJsonObject::ErrorPosition() {
        return m_ParsePosition;
    }
    
    tBool tJsonObject::IsValid() {
        return m_Error.empty();
    }
    
    tBool tJsonObject::SetValue(tString sPath, tVariant sValue) {
        tVectorString wPathParts;
        tString wCurrent;
        
        // Split path by dots
        for (tChar wChar : sPath) {
            if (wChar == '.') {
                if (!wCurrent.empty()) {
                    wPathParts.push_back(wCurrent);
                    wCurrent.clear();
                }
            } else {
                wCurrent += wChar;
            }
        }
        if (!wCurrent.empty()) {
            wPathParts.push_back(wCurrent);
        }
        
        if (wPathParts.empty()) {
            return false;
        }
        
        tObj* wCurrentObj = &m_RootObject;
        
        // Navigate to the parent object
        for (tSize i = 0; i < wPathParts.size() - 1; i++) {
            if (!wCurrentObj->HasProperty(wPathParts[i])) {
                tObj* wNewObj = new tObj();
                wCurrentObj->Set(wPathParts[i]) = tVariant(wNewObj);
            }
            tVariant wVariant = wCurrentObj->Get(wPathParts[i]);
            if (wVariant.IsClass()) {
                wCurrentObj = wVariant.Class<tObj>();
                if (!wCurrentObj) {
                    return false;
                }
            } else {
                return false;
            }
        }
        
        // Set the final value
        wCurrentObj->Set(wPathParts.back()) = sValue;
        return true;
    }
    
    tVariant tJsonObject::GetValue(tString sPath) {
        tVectorString wPathParts;
        tString wCurrent;
        
        // Split path by dots
        for (tChar wChar : sPath) {
            if (wChar == '.') {
                if (!wCurrent.empty()) {
                    wPathParts.push_back(wCurrent);
                    wCurrent.clear();
                }
            } else {
                wCurrent += wChar;
            }
        }
        if (!wCurrent.empty()) {
            wPathParts.push_back(wCurrent);
        }
        
        if (wPathParts.empty()) {
            return tVariant();
        }
        
        tObj* wCurrentObj = &m_RootObject;
        
        // Navigate to the target
        for (tSize i = 0; i < wPathParts.size() - 1; i++) {
            if (!wCurrentObj->HasProperty(wPathParts[i])) {
                return tVariant();
            }
            tVariant wVariant = wCurrentObj->Get(wPathParts[i]);
            if (wVariant.IsClass()) {
                wCurrentObj = wVariant.Class<tObj>();
                if (!wCurrentObj) {
                    return tVariant();
                }
            } else {
                return tVariant();
            }
        }
        
        // Get the final value
        return wCurrentObj->Get(wPathParts.back());
    }
    
    tBool tJsonObject::HasPath(tString sPath) {
        tVectorString wPathParts;
        tString wCurrent;
        
        // Split path by dots
        for (tChar wChar : sPath) {
            if (wChar == '.') {
                if (!wCurrent.empty()) {
                    wPathParts.push_back(wCurrent);
                    wCurrent.clear();
                }
            } else {
                wCurrent += wChar;
            }
        }
        if (!wCurrent.empty()) {
            wPathParts.push_back(wCurrent);
        }
        
        if (wPathParts.empty()) {
            return false;
        }
        
        tObj* wCurrentObj = &m_RootObject;
        
        // Navigate to the target
        for (tSize i = 0; i < wPathParts.size() - 1; i++) {
            if (!wCurrentObj->HasProperty(wPathParts[i])) {
                return false;
            }
            tVariant wVariant = wCurrentObj->Get(wPathParts[i]);
            if (wVariant.IsClass()) {
                wCurrentObj = wVariant.Class<tObj>();
                if (!wCurrentObj) {
                    return false;
                }
            } else {
                return false;
            }
        }
        
        // Check the final property
        return wCurrentObj->HasProperty(wPathParts.back());
    }
    
    tBool tJsonObject::RemoveValue(tString sPath) {
        tVectorString wPathParts;
        tString wCurrent;
        
        // Split path by dots
        for (tChar wChar : sPath) {
            if (wChar == '.') {
                if (!wCurrent.empty()) {
                    wPathParts.push_back(wCurrent);
                    wCurrent.clear();
                }
            } else {
                wCurrent += wChar;
            }
        }
        if (!wCurrent.empty()) {
            wPathParts.push_back(wCurrent);
        }
        
        if (wPathParts.empty()) {
            return false;
        }
        
        tObj* wCurrentObj = &m_RootObject;
        
        // Navigate to the parent object
        for (tSize i = 0; i < wPathParts.size() - 1; i++) {
            if (!wCurrentObj->HasProperty(wPathParts[i])) {
                return false;
            }
            tVariant wVariant = wCurrentObj->Get(wPathParts[i]);
            if (wVariant.IsClass()) {
                wCurrentObj = wVariant.Class<tObj>();
                if (!wCurrentObj) {
                    return false;
                }
            } else {
                return false;
            }
        }
        
        // Remove the final property
        return wCurrentObj->DeleteProperty(wPathParts.back());
    }
    
    tVectorString tJsonObject::GetPropertyNames() {
        return m_RootObject.Properties();
    }
    
    tObj* tJsonObject::GetObject(tString sPath) {
        tVariant wVariant = GetValue(sPath);
        if (wVariant.IsClass()) {
            return wVariant.Class<tObj>();
        }
        return nullptr;
    }
    
    tArray* tJsonObject::GetArray(tString sPath) {
        tVariant wVariant = GetValue(sPath);
        if (wVariant.IsClass()) {
            return wVariant.Class<tArray>();
        }
        return nullptr;
    }
    
    // Private helper methods ==================================================
    tObj tJsonObject::ParseValue(tString& sJson, tSize& sPos, tInt sDepth) {
        SkipWhitespace(sJson, sPos);
        
        if (sPos >= sJson.length()) {
            m_Error = "Unexpected end of JSON";
            throw runtime_error("Unexpected end of JSON");
        }
        
        tChar wChar = sJson[sPos];
        
        if (wChar == '{') {
            return ParseObject(sJson, sPos);
        } else if (wChar == '[') {
            tArray wArray = ParseArray(sJson, sPos);
            tObj wObj;
            wObj["value"] = tVariant(&wArray);
            return wObj;
        } else if (wChar == '"') {
            tString wString = ParseString(sJson, sPos);
            tObj wObj;
            wObj["value"] = tVariant(wString);
            return wObj;
        } else if (wChar == 't' || wChar == 'f' || wChar == 'n') {
            tVariant wVariant = ParseBooleanOrNull(sJson, sPos);
            tObj wObj;
            wObj["value"] = wVariant;
            return wObj;
        } else if ((wChar >= '0' && wChar <= '9') || wChar == '-' || wChar == '+') {
            tVariant wVariant = ParseNumber(sJson, sPos);
            tObj wObj;
            wObj["value"] = wVariant;
            return wObj;
        } else {
            m_Error = tString("Unexpected character: ") + tString(1, wChar);
            throw runtime_error("Unexpected character");
        }
    }
    
    tObj tJsonObject::ParseObject(tString& sJson, tSize& sPos) {
        tObj wObj;
        wObj.ObjType("object");
        
        if (sJson[sPos] != '{') {
            m_Error = tString("Expected '{' at position ") + to_string(sPos);
            throw runtime_error("Expected '{'");
        }
        sPos++; // Skip '{'
        
        SkipWhitespace(sJson, sPos);
        
        if (sPos < sJson.length() && sJson[sPos] == '}') {
            sPos++; // Skip '}'
            return wObj;
        }
        
        while (sPos < sJson.length()) {
            SkipWhitespace(sJson, sPos);
            
            if (sPos >= sJson.length()) {
                m_Error = "Unexpected end of JSON in object";
                throw runtime_error("Unexpected end of JSON");
            }
            
            if (sJson[sPos] != '"') {
                m_Error = tString("Expected string key at position ") + to_string(sPos);
                throw runtime_error("Expected string key");
            }
            
            tString wKey = ParseString(sJson, sPos);
            
            SkipWhitespace(sJson, sPos);
            
            if (sPos >= sJson.length() || sJson[sPos] != ':') {
                m_Error = tString("Expected ':' at position ") + to_string(sPos);
                throw runtime_error("Expected ':'");
            }
            sPos++; // Skip ':'
            
            tObj wValue = ParseValue(sJson, sPos);
            
            if (wValue.HasProperty("value")) {
                wObj.Set(wKey) = wValue["value"];
            } else {
                wObj.Set(wKey) = tVariant(&wValue);
            }
            
            SkipWhitespace(sJson, sPos);
            
            if (sPos >= sJson.length()) {
                m_Error = "Unexpected end of JSON in object";
                throw runtime_error("Unexpected end of JSON");
            }
            
            if (sJson[sPos] == '}') {
                sPos++; // Skip '}'
                break;
            } else if (sJson[sPos] == ',') {
                sPos++; // Skip ','
            } else {
                m_Error = tString("Expected ',' or '}' at position ") + to_string(sPos);
                throw runtime_error("Expected ',' or '}'");
            }
        }
        
        return wObj;
    }
    
    tArray tJsonObject::ParseArray(tString& sJson, tSize& sPos) {
        tArray wArray;
        
        if (sJson[sPos] != '[') {
            m_Error = tString("Expected '[' at position ") + to_string(sPos);
            throw runtime_error("Expected '['");
        }
        sPos++; // Skip '['
        
        SkipWhitespace(sJson, sPos);
        
        if (sPos < sJson.length() && sJson[sPos] == ']') {
            sPos++; // Skip ']'
            return wArray;
        }
        
        while (sPos < sJson.length()) {
            SkipWhitespace(sJson, sPos);
            
            if (sPos >= sJson.length()) {
                m_Error = "Unexpected end of JSON in array";
                throw runtime_error("Unexpected end of JSON");
            }
            
            tObj wValue = ParseValue(sJson, sPos);
            
            if (wValue.HasProperty("value")) {
                wArray.VectorVariant()->push_back(wValue["value"]);
            } else {
                wArray.VectorVariant()->push_back(tVariant(&wValue));
            }
            
            SkipWhitespace(sJson, sPos);
            
            if (sPos >= sJson.length()) {
                m_Error = "Unexpected end of JSON in array";
                throw runtime_error("Unexpected end of JSON");
            }
            
            if (sJson[sPos] == ']') {
                sPos++; // Skip ']'
                break;
            } else if (sJson[sPos] == ',') {
                sPos++; // Skip ','
            } else {
                m_Error = tString("Expected ',' or ']' at position ") + to_string(sPos);
                throw runtime_error("Expected ',' or ']'");
            }
        }
        
        return wArray;
    }
    
    tString tJsonObject::ParseString(tString& sJson, tSize& sPos) {
        if (sJson[sPos] != '"') {
            m_Error = tString("Expected '\"' at position ") + to_string(sPos);
            throw runtime_error("Expected '\"'");
        }
        sPos++; // Skip opening quote
        
        tString wResult;
        
        while (sPos < sJson.length()) {
            tChar wChar = sJson[sPos];
            
            if (wChar == '"') {
                sPos++; // Skip closing quote
                return wResult;
            } else if (wChar == '\\') {
                sPos++; // Skip backslash
                if (sPos >= sJson.length()) {
                    m_Error = "Unexpected end of JSON in string";
                    throw runtime_error("Unexpected end of JSON");
                }
                
                tChar wEscaped = sJson[sPos];
                switch (wEscaped) {
                    case '"': wResult += '"'; break;
                    case '\\': wResult += '\\'; break;
                    case '/': wResult += '/'; break;
                    case 'b': wResult += '\b'; break;
                    case 'f': wResult += '\f'; break;
                    case 'n': wResult += '\n'; break;
                    case 'r': wResult += '\r'; break;
                    case 't': wResult += '\t'; break;
                    case 'u': {
                        // Simple Unicode escape (not fully implemented)
                        wResult += '?';
                        break;
                    }
                    default:
                        wResult += wEscaped;
                        break;
                }
                sPos++;
            } else {
                wResult += wChar;
                sPos++;
            }
        }
        
        m_Error = "Unterminated string";
        throw runtime_error("Unterminated string");
    }
    
    tVariant tJsonObject::ParseNumber(tString& sJson, tSize& sPos) {
        tString wNumber;
        tBool wHasDecimal = false;
        tBool wHasExponent = false;
        
        if (sPos < sJson.length() && (sJson[sPos] == '-' || sJson[sPos] == '+')) {
            wNumber += sJson[sPos];
            sPos++;
        }
        
        while (sPos < sJson.length()) {
            tChar wChar = sJson[sPos];
            
            if (wChar >= '0' && wChar <= '9') {
                wNumber += wChar;
                sPos++;
            } else if (wChar == '.' && !wHasDecimal && !wHasExponent) {
                wNumber += wChar;
                wHasDecimal = true;
                sPos++;
            } else if ((wChar == 'e' || wChar == 'E') && !wHasExponent) {
                wNumber += wChar;
                wHasExponent = true;
                sPos++;
                
                if (sPos < sJson.length() && (sJson[sPos] == '-' || sJson[sPos] == '+')) {
                    wNumber += sJson[sPos];
                    sPos++;
                }
            } else {
                break;
            }
        }
        
        if (wHasDecimal || wHasExponent) {
            return tVariant(stod(wNumber));
        } else {
            return tVariant(stoi(wNumber));
        }
    }
    
    tVariant tJsonObject::ParseBooleanOrNull(tString& sJson, tSize& sPos) {
        if (sPos + 4 <= sJson.length() && sJson.substr(sPos, 4) == "true") {
            sPos += 4;
            return tVariant(true);
        } else if (sPos + 5 <= sJson.length() && sJson.substr(sPos, 5) == "false") {
            sPos += 5;
            return tVariant(false);
        } else if (sPos + 4 <= sJson.length() && sJson.substr(sPos, 4) == "null") {
            sPos += 4;
            return tVariant();
        } else {
            m_Error = tString("Invalid boolean or null value at position ") + to_string(sPos);
            throw runtime_error("Invalid boolean or null value");
        }
    }
    
    void tJsonObject::SkipWhitespace(tString& sJson, tSize& sPos) {
        while (sPos < sJson.length()) {
            tChar wChar = sJson[sPos];
            if (wChar == ' ' || wChar == '\t' || wChar == '\n' || wChar == '\r') {
                sPos++;
            } else {
                break;
            }
        }
    }
    
    tString tJsonObject::StringifyObject(tObj& sObj,tBool sLineReturn, tInt sIndent) {
        tString wResult = "{";
        tBool wFirst = true;
        tVectorString wProperties = sObj.Properties();
        
        for (const tString& wProperty : wProperties) {
            if (!wFirst) {
                wResult += ",";
            }
            wFirst = false;
            
            if ((sLineReturn) && (sIndent >= 0)) {
                wResult += "\n" + GetIndent(sIndent + 1);
            }
            
            wResult += "\"" + EscapeString(wProperty) + "\":";
            
            if ((sLineReturn) && (sIndent >= 0)) {
                wResult += " ";
            }
            
            tVariant wVariant = sObj.Get(wProperty);
            if (wVariant.IsClass()) {
                tArray* wArray = wVariant.Class<tArray>();
                tObj* wObj = wVariant.Class<tObj>();
                if (wArray) {
                    wResult += StringifyArray(*wArray,sLineReturn, sIndent);
                } else if (wObj) {
                    wResult += StringifyObject(*wObj,sLineReturn, sIndent);
                } else {
                    wResult += "null";
                }
            } else {
                wResult += StringifyVariant(wVariant);
            }
        }
        
        if ((sLineReturn) &&  (sIndent >= 0)  && (!wProperties.empty())) {
            wResult += "\n" + GetIndent(sIndent);
        }
        
        wResult += "}";
        return wResult;
    }

    
    tString tJsonObject::StringifyArray(tArray& sArray, tBool sLineReturn, tInt sIndent) {
        tString wResult = "[";
        tBool wFirst = true;
        tVectorVariant* wVariants = sArray.VectorVariant();
        
        for (tVariant& wVariant : *wVariants) {
            if (!wFirst) {
                wResult += ",";
            }
            wFirst = false;
            
            if ((sLineReturn) && (sIndent >= 0)) {
                wResult += "\n" + GetIndent(sIndent + 1);
            }
            
            if (wVariant.IsClass()) {
                tObj* wObj = wVariant.Class<tObj>();
                tArray* wArray = wVariant.Class<tArray>();
                if (wObj) {
                    wResult += StringifyObject(*wObj, sIndent);
                } else if (wArray) {
                    wResult += StringifyArray(*wArray, sIndent);
                } else {
                    wResult += "null";
                }
            } else {
                wResult += StringifyVariant(wVariant);
            }
        }
        
        if ( sLineReturn && (sIndent >= 0) && (!wVariants->empty())) {
            wResult += "\n" + GetIndent(sIndent);
        }
        
        wResult += "]";
        return wResult;
    }

    
    tString tJsonObject::StringifyVariant(tVariant& sVariant) {
        if (sVariant.IsNull()) {
            return "null";
        } else if (sVariant.IsBool()) {
            return sVariant.Bool() ? "true" : "false";
        } else if (sVariant.IsInt()) {
            return to_string(sVariant.Int());
        } else if (sVariant.IsDouble()) {
            return to_string(sVariant.Double());
        } else if (sVariant.IsString()) {
            return "\"" + EscapeString(sVariant.String()) + "\"";
        } else {
            return "null";
        }
    }
    
    tString tJsonObject::EscapeString(tString sString) {
        tString wResult;
        
        for (tChar wChar : sString) {
            switch (wChar) {
                case '"': wResult += "\\\""; break;
                case '\\': wResult += "\\\\"; break;
                case '\b': wResult += "\\b"; break;
                case '\f': wResult += "\\f"; break;
                case '\n': wResult += "\\n"; break;
                case '\r': wResult += "\\r"; break;
                case '\t': wResult += "\\t"; break;
                default:
                    if (wChar < 32) {
                        wResult += "\\u" + to_string((int)wChar);
                    } else {
                        wResult += wChar;
                    }
                    break;
            }
        }
        
        return wResult;
    }
    
    tString tJsonObject::GetIndent(tInt sLevel) {
        tString wResult;
        for (tInt i = 0; i < sLevel; i++) {
            wResult += "  ";
        }
        return wResult;
    }
    
    void tJsonObject::JsonObj(Writer<StringBuffer>* sWriter, tBool sJsonRadical) {
        sWriter->StartObject();
        sWriter->Key("type");
        sWriter->String("tJsonObject");
        sWriter->Key("data");
        m_RootObject.JsonObj(sWriter, sJsonRadical);
        sWriter->EndObject();
    }
    
    void tJsonObject::JsonObj(const rapidjson::Value& sValue, tBool sJsonRadical) {
        if (sValue.HasMember("data") && sValue["data"].IsObject()) {
            m_RootObject.JsonObj(sValue["data"], sJsonRadical);
        }
    }
    
#ifdef _DEBUGSK
    tString tJsonObject::Debug() {
        tString wResult = "tJsonObject Debug:\n";
        wResult += tString("Valid: ") + (IsValid() ? tString("true") : tString("false")) + tString("\n");
        wResult += tString("Error: ") + Error() + tString("\n");
        wResult += tString("JSON: ") + Stringify(true) + tString("\n");
        return wResult;
    }
#endif

} // end of namespace =========================================================

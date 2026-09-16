//=============================================================================
// SkJsonShareString
/**
 * @page SkJsonShareString
 * @par
 * @par SkJsonShareString is a class that manages the JSON share string.
 * @par
 */
//=============================================================================
#include "../include/SkJsonSharedString.hpp"

namespace SkRoot {

    tJsonSharedString::tJsonSharedString() : tClass() ,m_IsActif(true),m_Key("si"), m_Strings(), m_MapString() {
    }
    tJsonSharedString::~tJsonSharedString() {
        Clear();
    }
    
    void tJsonSharedString::Clear() {
        m_Strings.clear();
        m_MapString.clear();
    }
    
    void tJsonSharedString::Begin() {;
        m_IsActif=true;
    }
    
    
    void tJsonSharedString::End() {;
        m_IsActif=false;
        Clear();
    }
    
    tBool tJsonSharedString::IsActif() { return(m_IsActif); }
   
    void tJsonSharedString::Key(tString sKey) { m_Key=sKey; }
    tString tJsonSharedString::Key() { return(m_Key); }
      
    tIndex tJsonSharedString::AddString(tString sValue) {
        tIndex wIndex=-1;
        tMapStringVector::iterator wIterator;
        // Search if exist
        wIterator=m_MapString.find(sValue);
        if (wIterator!=m_MapString.end()) {
            wIndex=(*wIterator).second;
        } else {
           // new Value
            m_Strings.push_back(sValue);
            wIndex = tIndex(m_Strings.size()) - 1; // 0 based
            m_MapString[sValue]=wIndex;
        }
        return(wIndex);
    }
    
    tString tJsonSharedString::GetString(tIndex sValue) {
        if (sValue < 0 || sValue >= (tIndex)m_Strings.size()) {
            return tString();
        }
        return (m_Strings[(tSize)sValue]);
    }

    tIndex tJsonSharedString::StringCount() const {
        return (tIndex)m_Strings.size();
    }

    void tJsonSharedString::Json(Writer<StringBuffer>* sWriter) const {
        if (!m_Strings.empty()) {
            sWriter->Key(m_Key.c_str());
            sWriter->StartArray();
            for (auto wString : m_Strings) {
                sWriter->String(wString.c_str());
            }
            sWriter->EndArray();
        }
    }

    void tJsonSharedString::Json(const rapidjson::Value& sValue) {
        Clear();
        if (sValue.HasMember(m_Key.c_str())) {
            const rapidjson::Value& wValueStrings = sValue[m_Key.c_str()];
            if (wValueStrings.IsArray()) {
                for (rapidjson::SizeType wIndex = 0; wIndex < wValueStrings.Size(); wIndex++) {
                    const rapidjson::Value& wValue = wValueStrings[wIndex];
                    // Cell "si"/"fi" indices are JSON array positions (0..n-1). Every slot must consume one
                    // vector entry; skipping non-strings used to shift indices and break WASM/native round-trip.
                    if (wValue.IsString()) {
                        const char* wChars = wValue.GetString();
                        m_Strings.push_back(wChars);
                        const tIndex wStringIndex = (tIndex)m_Strings.size() - 1;
                        m_MapString[wChars] = wStringIndex;
                    } else {
                        m_Strings.push_back(tString());
                    }
                }
            }
        }
    }

}


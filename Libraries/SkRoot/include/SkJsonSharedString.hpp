//=============================================================================
// SkJsonShareString
/**
 * @page SkJsonShareString
 * @par
 * @par SkJsonShareString is a class that manages the JSON share string.
 * @par
 */
//=============================================================================
#ifndef SkJsonShareString_hpp
#define SkJsonShareString_hpp

#include "SkClass.hpp"

using namespace rapidjson;

namespace SkRoot {
    typedef std::unordered_map<tString, tInt> tMapStringVector;
 
    class tJsonSharedString : public tClass {
    private:
        tBool               m_IsActif;
        tString             m_Key;
        tVectorString       m_Strings;
        tMapStringVector    m_MapString;
    public:
        ///@brief Constructor
        tJsonSharedString();
        
        ///@brief Destructor
        ~tJsonSharedString();
        
        ///@brief Clear
        void Clear();
        
        ///@brief Begin
        void Begin();
        
         ///@brief End
        void End();
        
        ///@brief isActif
        ///return tBool
        tBool IsActif();
        
        ///@brief setKey
        ///param[in] skey
        void Key(tString sKey);
        
        ///@brief Key
        ///return tString
        tString Key();
        
        ///@ brief Add String
        ///@param[in] sValue tString
        ///@return tIndex
        tIndex AddString(tString sValue);
        
        ///@brief GetString
        ///@param[in] sValue tIndex 
        ///@return tString
        tString GetString(tIndex sValue);

        ///@brief Number of pooled strings (same as JSON array length after load).
        tIndex StringCount() const;
        
         // Json ==============================================================
        /// @brief		Writer Json. 
        /// @param[in]	sWriter Writer<StringBuffer>*
        void Json(Writer<StringBuffer>* sWriter) const;


        /// @brief		Reader Json. 
        /// @param[in]	sValue Value&
        void Json(const rapidjson::Value& sValue);
        
        
    };
}

#endif

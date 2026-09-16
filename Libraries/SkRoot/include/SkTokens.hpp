//=============================================================================
// Skeema Tokens
/**
* @page SkTokens
* @par
* @par Allows to attach tVirtualClass to Dictionaries...
* @par Contains for example the globals of SkApplication.
*/
//=============================================================================
#ifndef SkTokens_hpp
#define SkTokens_hpp


#include "SkTypesClass.hpp"

namespace SkRoot {
    //=========================================================================
    //! Dictionary of tVirtualClass 
    typedef unordered_map<tString, tVirtualClass*> SkMapToken;
    //! Root of de Tokens 
    class tTokens : public tVirtualClass {
    private:
        SkMapToken m_Map;
    public:
        /// @brief Constructor
        tTokens();
        
        /// @brief Destructor
        virtual ~tTokens();

        /// @brief Clear the tokens
        void Clear();

        /// @brief Add tVirtualClass.
        /// @param[in] sKey unique for the system
        /// @param[in] sVirtualClass for add derived sVirtualClass 
        /// @return tBool false an element already exist
        tBool Add(tString sKey, tVirtualClass* sVirtualClass);

        /// @brief Delete tVirtualClass.
        /// @param[in] sKey unique for the system
        /// @return tBool false element don't exist
        tBool Delete(tString sKey);

        /// @brief Get tVirtualClass.
        /// @param[in] sKey unique for the system
        /// @return tVirtualClass* if nullptr element don't exist
        tVirtualClass* Token(tString sKey);
    };
}

#endif
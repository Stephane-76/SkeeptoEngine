//=============================================================================
// Skeema SharedString
/**
* @page tSharedString
* @par
* @par Manage shared string for gain of memory..
*/
//=============================================================================
#ifndef SkSharedString_hpp
#define SkSharedString_hpp

#include "SkTypes.hpp"
#include "SkClass.hpp"

namespace SkRoot {

    class tSharedStringPool;
    typedef std::unordered_map<tString, tSharedStringPool*> tMapStringPool;

    //=========================================================================
    //! SkSharedStringPool unique string 
    class tSharedStringPool : public tClass {
    protected:
        //! Number of instances
        tInt            m_Count;
        //! Pointer to the interned key owned by tSharedStringContainer
        const tString*  m_Key;
    public:
        /// @brief Constructor
        tSharedStringPool();

        tSharedStringPool(const tSharedStringPool&) = delete;
        tSharedStringPool& operator=(const tSharedStringPool&) = delete;
        
        /// @brief Destructor
        ~tSharedStringPool();

        /// @brief Bind this pool entry to the map key
        /// @param[in] sKey Interned string owned by the container map
        void BindKey(const tString* sKey);

        /// @brief Increment identical strings
        void Inc();

        /// @brief Decrement identical strings
        void Dec();

        /// @brief Check if shared string is not used
        /// @return tBool True if m_Count == 0
        tBool Empty() const;

        /// @brief Access to interned string
        /// @return const tString&
        const tString& operator()() const;
    };

    //=========================================================================
    //! Container of all SharedStringPools 
    class tSharedStringContainer : public tClass {
    private:
        tMapStringPool	m_MapStringPool;
    public:
        /// @brief Constructor
        tSharedStringContainer();
        
        /// @brief Destructor
        ~tSharedStringContainer();

        /// @brief Add shared string
        /// @param[in] sString tString 
        /// @return tSharedStringPool*
        tSharedStringPool* Add(const tString& sString);

        /// @brief Remove shared string
        /// @param[in] sSharedStringPool tSharedStringPool*
        void Remove(tSharedStringPool* sSharedStringPool);

        /// @brief Get the number of shared strings
        /// @return tSize
        tSize NbSharedString() const;
    };

    //=========================================================================
    //! Shared string itself
    class tSharedString : tClass {
        private:
            //! Pointer to SharedStringPool.
            tSharedStringPool* m_SharedStringPool;
        public:
            /// @brief Constructor
            tSharedString();
            
            /// @brief Copy constructor
            /// @param[in] sSharedString tSharedString&
            tSharedString(const tSharedString& sSharedString);

            /// @brief Move constructor
            /// @param[in] sSharedString tSharedString&&
            tSharedString(tSharedString&& sSharedString) noexcept;
            
            /// @brief Constructor by string 
            /// @param[in] sString tString
            tSharedString(const tString& sString);
            
            /// @brief Constructor by tChar* (buffer)
            /// @param[in] sChar tChar*
            tSharedString(const tChar* sChar);
    
            /// @brief Destructor
            ~tSharedString();

            /// @brief Get interned string. Valid while this instance (or another interned copy) is alive.
            /// @return const tString&
            const tString& Str() const noexcept;

            /// @brief Assignment operator
            /// @param[in] sString tSharedString&
            /// @return tSharedString&
            tSharedString& operator =(const tSharedString& sString);

            /// @brief Move assignment operator
            /// @param[in] sString tSharedString&&
            /// @return tSharedString&
            tSharedString& operator =(tSharedString&& sString) noexcept;
            
            /// @brief Addition assignment operator
            /// @param[in] sString tString&
            /// @return tSharedString&
            tSharedString& operator +=(const tString& sString);

            /// @brief Equality operator
            /// @param[in] sString tSharedString&
            /// @return tBool 
            tBool operator==(const tSharedString& sString) const noexcept;

            /// @brief Inequality operator
            /// @param[in] sString tSharedString&
            /// @return tBool 
            tBool operator!=(const tSharedString& sString) const noexcept;

            /// @brief Equality operator
            /// @param[in] sString tString&
            /// @return tBool 
            tBool operator==(const tString& sString) const;
            
            /// @brief Inequality operator
            /// @param[in] sString tString&
            /// @return tBool 
            tBool operator!=(const tString& sString) const;

            /// @brief Less than operator
            /// @param[in] sString tString&
            /// @return tBool 
            tBool operator < (const tString& sString) const;
            
            /// @brief Greater than operator
            /// @param[in] sString tString&
            /// @return tBool 
            tBool operator > (const tString& sString) const;
            
            /// @brief Less than or equal operator
            /// @param[in] sString tString&
            /// @return tBool 
            tBool operator <= (const tString& sString) const;
            
            /// @brief Greater than or equal operator
            /// @param[in] sString tString&
            /// @return tBool 
            tBool operator >= (const tString& sString) const;
            
            /// @brief Access to interned string. Valid while this instance (or another interned copy) is alive.
            /// @return const tString&
            const tString& operator()() const noexcept;
            
            // friend method ==================================================
            friend tSharedString operator+(const tSharedString& sSharedString1, const tSharedString& sSharedString2);
    };

}; // end of namespace ========================================================

#endif

//=============================================================================
// Skeema UID
/**
* @page tUID
* @par
* @par Thread-safe generator of unique 64-bit identifiers for the Skeema library.
*/
//=============================================================================
#ifndef SkUID_hpp
#define SkUID_hpp

#include "SkTypes.hpp"
#include "SkClass.hpp"
#include <unordered_map>

namespace SkRoot {

    //=========================================================================
    //! Thread-safe 64-bit UID generator
    /**
     * Generates unique 64-bit identifiers within a process using a combined
     * timestamp and counter approach. The high 32 bits hold a Unix timestamp
     * (seconds) and the low 32 bits hold a monotonically increasing counter.
     * This ensures uniqueness even at high throughput while remaining fast.
     */
    class tUid : public tClass {
    public:
        /// @brief Generate a new unique 64-bit identifier.
        /// @return tLongLong 64-bit UID
        static tLongLong New();

        /// @brief Generate a new unique identifier as 16-hex characters.
        /// @return tString Hex representation without prefix
        static tString NewHex();
    };

    //=========================================================================
    //! UID to AllocatorRef mapping manager
    /**
     * Thread-safe container for managing associations between UIDs and AllocatorRefs.
     * Provides bidirectional lookup capabilities.
     */
    class tUidMap : public tClass {
    private:
        //! Map from UID to AllocatorRef
        std::unordered_map<tLongLong, tAllocatorRef> m_UIDToRef;
        //! Map from AllocatorRef to UID
        std::unordered_map<tAllocatorRef, tLongLong> m_RefToUID;
        //! Mutex for thread safety
        mutable std::mutex m_Mutex;

    public:
        /// @brief Constructor
        tUidMap();

        /// @brief Destructor
        ~tUidMap();

        /// @brief Add a new UID-AllocatorRef association
        /// @param[in] sRef tAllocatorRef AllocatorRef to associate
        /// @return tString returned UID 
        tString AddRef(tAllocatorRef sRef);

        /// @brief Get AllocatorRef by UID
        /// @param[in] sUID tString UID (hex) to look up
        /// @return tAllocatorRef Associated AllocatorRef, or 0 if not found
        tAllocatorRef GetRefByUID(tString sUID) const;

        /// @brief Get UID by AllocatorRef
        /// @param[in] sRef tAllocatorRef AllocatorRef to look up
        /// @return tString Associated UID (hex), or empty if not found
        tString GetUIDByRef(tAllocatorRef sRef) const;

        /// @brief Remove an association by UID
        /// @param[in] sUID tString UID (hex) to remove
        /// @return tBool True if successfully removed, false if not found
        tBool RemoveByUID(tString sUID);

        /// @brief Remove an association by AllocatorRef
        /// @param[in] sRef tAllocatorRef AllocatorRef to remove
        /// @return tBool True if successfully removed, false if not found
        tBool RemoveByRef(tAllocatorRef sRef);

        /// @brief Get the number of associations
        /// @return tSize Number of UID-Ref pairs
        tSize Size() const;

        /// @brief Check if empty
        /// @return tBool True if no associations exist
        tBool Empty() const;

        /// @brief Clear all associations
        void Clear();

        /// @brief Write UID-Ref associations to JSON
        /// @param[in] sWriter rapidjson::Writer<rapidjson::StringBuffer>* JSON writer
        void Json(rapidjson::Writer<rapidjson::StringBuffer>* sWriter);

        /// @brief Read UID-Ref associations from JSON
        /// @param[in] sValue const rapidjson::Value& JSON value
        void Json(const rapidjson::Value& sValue);
    };
}

#endif



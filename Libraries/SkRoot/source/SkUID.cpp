//=============================================================================
// Skeema UID implementation
//=============================================================================

#include "../include/SkUID.hpp"

#include <atomic>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <sstream>

namespace SkRoot {

    static std::atomic<unsigned int> g_uidCounter{0};

    tLongLong tUid::New() {
        using namespace std::chrono;
        // High 32 bits: seconds since epoch
        unsigned long long seconds = static_cast<unsigned long long>(duration_cast<std::chrono::seconds>(system_clock::now().time_since_epoch()).count());
        // Low 32 bits: atomically incremented counter; wrap naturally on overflow
        unsigned int counter = g_uidCounter.fetch_add(1, std::memory_order_relaxed) + 1u;
        unsigned long long uid = (seconds << 32) | static_cast<unsigned long long>(counter);
        return static_cast<tLongLong>(uid);
    }

    tString tUid::NewHex() {
        unsigned long long uid = static_cast<unsigned long long>(tUid::New());
        std::stringstream ss;
        ss << std::hex << std::nouppercase << std::setfill('0') << std::setw(16) << uid;
        return ss.str();
    }

    //=========================================================================
    // tUIDMap implementation
    //=========================================================================

    tUidMap::tUidMap() {
    }

    tUidMap::~tUidMap() {
    }

    tString tUidMap::AddRef(tAllocatorRef sRef) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        tLongLong wUIDNum = 0;
        tBool wOk = false;

        // Search a new UID; retry if that UID is already mapped
        while (!wOk) {
            wUIDNum = tUid::New();
            if (m_UIDToRef.find(wUIDNum) == m_UIDToRef.end()) {
                wOk = true;
            }
        }
        
        // Add bidirectional mapping
        m_UIDToRef[wUIDNum] = sRef;
        m_RefToUID[sRef] = wUIDNum;
        // Return Str
        std::stringstream ss;
        ss << std::hex << std::nouppercase << std::setfill('0') << std::setw(16) << wUIDNum;
        return ss.str();
    }

    tAllocatorRef tUidMap::GetRefByUID(tString sUID) const {
        tLongLong uidNum = 0;
        if (!sUID.empty()) {
            std::stringstream conv;
            conv << std::hex << sUID;
            unsigned long long tmp = 0ULL;
            conv >> tmp;
            uidNum = static_cast<tLongLong>(tmp);
        }
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_UIDToRef.find(uidNum);
        return (it != m_UIDToRef.end()) ? it->second : 0;
    }

    tString tUidMap::GetUIDByRef(tAllocatorRef sRef) const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_RefToUID.find(sRef);
        if (it == m_RefToUID.end()) return "";
        unsigned long long uid = static_cast<unsigned long long>(it->second);
        std::stringstream ss;
        ss << std::hex << std::nouppercase << std::setfill('0') << std::setw(16) << uid;
        return ss.str();
    }

    tBool tUidMap::RemoveByUID(tString sUID) {
        tLongLong uidNum = 0;
        if (!sUID.empty()) {
            std::stringstream conv;
            conv << std::hex << sUID;
            unsigned long long tmp = 0ULL;
            conv >> tmp;
            uidNum = static_cast<tLongLong>(tmp);
        }
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_UIDToRef.find(uidNum);
        if (it != m_UIDToRef.end()) {
            tAllocatorRef ref = it->second;
            m_UIDToRef.erase(it);
            m_RefToUID.erase(ref);
            return true;
        }
        return false;
    }

    tBool tUidMap::RemoveByRef(tAllocatorRef sRef) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        auto it = m_RefToUID.find(sRef);
        if (it != m_RefToUID.end()) {
            tLongLong uid = it->second;
            m_RefToUID.erase(it);
            m_UIDToRef.erase(uid);
            return true;
        }
        return false;
    }

    tSize tUidMap::Size() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_UIDToRef.size();
    }

    tBool tUidMap::Empty() const {
        std::lock_guard<std::mutex> lock(m_Mutex);
        return m_UIDToRef.empty();
    }

    void tUidMap::Clear() {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_UIDToRef.clear();
        m_RefToUID.clear();
    }

    void tUidMap::Json(rapidjson::Writer<rapidjson::StringBuffer>* sWriter) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        sWriter->StartObject();
        sWriter->Key("uidref");
        sWriter->StartArray();
        
        for (const auto& pair : m_UIDToRef) {
            sWriter->StartObject();
            sWriter->Key("u");
            sWriter->Int64(pair.first);
            sWriter->Key("r");
            sWriter->Uint(pair.second);
            sWriter->EndObject();
        }
        
        sWriter->EndArray();
        sWriter->EndObject();
    }

    void tUidMap::Json(const rapidjson::Value& sValue) {
        std::lock_guard<std::mutex> lock(m_Mutex);
        
        // Clear existing associations
        m_UIDToRef.clear();
        m_RefToUID.clear();
        
        if (sValue.IsObject() && sValue.HasMember("uidref")) {
            const rapidjson::Value& associations = sValue["uidref"];
            if (associations.IsArray()) {
                for (rapidjson::SizeType i = 0; i < associations.Size(); i++) {
                    const rapidjson::Value& assoc = associations[i];
                    if (assoc.IsObject() && assoc.HasMember("u") && assoc.HasMember("r")) {
                        tLongLong uid = assoc["u"].GetInt64();
                        tAllocatorRef ref = assoc["r"].GetUint();
                        
                        // Add bidirectional mapping
                        m_UIDToRef[uid] = ref;
                        m_RefToUID[ref] = uid;
                    }
                }
            }
        }
    }
}



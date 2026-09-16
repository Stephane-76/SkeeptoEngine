//=============================================================================
// MongoDB ObjectId Generator
//! Generate MongoDB-style ObjectId (24 hex characters)
//=============================================================================
#include "../include/SkObjectId.hpp"

namespace SkRoot {

    //=========================================================================
    // tObjectId Implementation
    //=========================================================================

    tObjectId::tObjectId() : tClass(), m_Counter(0) {
        m_MachineId = GenerateMachineId();
    }

    tString tObjectId::GenerateMachineId() {
        // Generate a 5-byte machine ID based on random values
        // Browser and Emscripten-compatible implementation
        static std::random_device dev;
        static std::mt19937 rng(dev());
        std::uniform_int_distribution<tInt> wDist(0, 255);
        
        tStringStream wStream;
        for (int i = 0; i < 5; i++) {
            wStream << std::hex << std::setw(2) << std::setfill('0') << wDist(rng);
        }
        return wStream.str();
    }

    tString tObjectId::Generate() {
        // Get current timestamp (4 bytes) - Browser and Emscripten compatible
        auto wNow = std::chrono::system_clock::now();
        auto wTimestamp = std::chrono::duration_cast<std::chrono::seconds>(wNow.time_since_epoch()).count();
        
        // Increment counter (3 bytes, max 16777215)
        m_Counter = (m_Counter + 1) % 16777216;
        
        tStringStream wStream;
        
        // Timestamp (4 bytes) - 8 hex characters
        wStream << std::hex << std::setw(8) << std::setfill('0') << wTimestamp;
        
        // Machine ID (5 bytes) - 10 hex characters
        wStream << m_MachineId;
        
        // Counter (3 bytes) - 6 hex characters
        wStream << std::hex << std::setw(6) << std::setfill('0') << m_Counter;
        
        return wStream.str();
    }

    tString tObjectId::New() {
        // Static instance for generating new ObjectIds
        static tObjectId wGenerator;
        return wGenerator.Generate();
    }

} // end of namespace

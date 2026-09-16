//=============================================================================
// MongoDB ObjectId Generator
//! Generate MongoDB-style ObjectId (24 hex characters)
//=============================================================================
#ifndef SkObjectId_hpp
#define SkObjectId_hpp

#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>
#include <string>

#include "SkTypes.hpp"
#include "SkClass.hpp"

namespace SkRoot {
    //=========================================================================
    //! MongoDB ObjectId Generator
    //! Generates 24-character hexadecimal ObjectId similar to MongoDB
    //! Format: 8 chars (timestamp) + 10 chars (machine ID) + 6 chars (counter)
    //! Compatible with Emscripten/WASM and modern browsers
    class tObjectId : public tClass {
    private:
        //! Machine/Process ID for MongoDB ObjectId (5 bytes = 10 hex chars)
        tString m_MachineId;
        //! Counter for MongoDB ObjectId (3 bytes = 6 hex chars)
        tInt m_Counter;

        /// @brief      Generate machine ID for MongoDB ObjectId.
        /// @return     tString 5 bytes as hex string (10 characters)
        tString GenerateMachineId();

    public:
        /// @brief      Constructor.
        tObjectId();

        /// @brief      Generate MongoDB-style ObjectId (24 hex characters).
        /// @return     tString MongoDB ObjectId format
        tString Generate();

        /// @brief      Generate a new ObjectId and return it.
        /// @return     tString New MongoDB ObjectId
        static tString New();
    };
}

#endif

//=============================================================================
// Skeema Exception
//! Ancestor class of all exceptions
//=============================================================================
#ifndef SkException_hpp
#define SkException_hpp

#include <cassert>
#include <exception>
#include "../include/SkTypes.hpp"

/// @brief Assertion macro: in Debug builds the condition is checked; in Release (NDEBUG) it is a no-op.
/// @param cond Boolean expression to assert (e.g. static_cast<size_t>(sIndice) < m_VectorRef.size())
#define SK_ASSERT(cond) assert(cond)

namespace SkRoot {

    //! Ancestor exception 
    class tException : public std::exception {
        public:
            /// @brief Get the exception message
            /// @return Exception message
            virtual const tChar* what() const throw();
    };

    //! Exception for internal error
    class tExceptionInternalError : public tException {
    private:
        tString m_Msg;
    public:
        /// @brief Constructor for internal error exception
        /// @param[in] sMsg Error message
        tExceptionInternalError(tString sMsg);
        
        /// @brief Get the exception message
        /// @return Exception message
        virtual const tChar* what() const throw();
    };

    //! Exception Div0 
    class tExceptionDiv0 : public tException {
        public:
            /// @brief Get the exception message
            /// @return Exception message
            virtual const tChar* what() const throw();
    };

    //! Exception Bad type
    class tExceptionBadType : public tException {
    private:
        tString m_Msg;
    public:
        /// @brief Constructor for bad type exception
        /// @param[in] sMsg Error message
        tExceptionBadType(tString sMsg);
        
        /// @brief Get the exception message
        /// @return Exception message
        virtual const tChar* what() const throw();
    };

    //! Exception Memory
    class tExceptionMemory : public tException {
    private:
        tString m_Msg;
    public:
        /// @brief Constructor for memory exception
        /// @param[in] sMsg Error message
        tExceptionMemory(tString sMsg);
        
        /// @brief Get the exception message
        /// @return Exception message
        virtual const tChar* what() const throw();
    };

}
#endif

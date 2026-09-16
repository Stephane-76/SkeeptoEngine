//=============================================================================
// SkMessage
//=============================================================================
#ifndef SkMessage_hpp
#define SkMessage_hpp

#include "SkUser.hpp"
#include "rapidjson/error/en.h"
#include <SkClass.hpp>
#include <SkException.hpp>

namespace SkSpreadSheet {

    using namespace SkRoot;
    using namespace rapidjson;

    class tInterfaceWeb;
    class tUndoSpreadSheet;

    class tMessage : public tClass {
    private:
        //! @brief Email
        tString m_Email;
        
        //! @brief Name
        tString m_Name;
        
        //! @brief First Name
        tString m_FirstName;
        
        //! @brief Uri WorkBookk
        tString m_Uri;
        
        //! @brief Do , Undo or Redo
        tString m_DoRedo;
        
#ifdef TestMultiUser
        //! @brief Email
        tString m_User;
#endif
 
        //! Keep Json Fromat
        tString m_Json;
    public:
        //! @brief Constructor
        tMessage();
        
        //! @brief Constructor
        //! @param[in] sUser User
        //! @param[in] sUri UriWorkBook
        //! @param[in] sDoRedo tString
#ifdef TestMultiUser
        tMessage(tString sUser, tString sUri,tString sDoRedo);
#else
        tMessage(tString sEmail, tString sName, tString sFirstName, tString sUri,tString sDoRedo);
#endif
        
        //! @brief Destructor
        ~tMessage();

        /// User 
        //! @brief Get Email
        tString Email() const;
        //! @brief Get Name
        tString Name() const;
        //! @brief Get First Name
        tString FirstName() const;
        
#ifdef TestMultiUser        //! @brief Get User
        tString User() const;
#endif
        //! @brief Get UriWorkBook
        tString Uri() const;
        
        //! @brief Set UriWorkBook
        //! @param[in] sUri tString
        void Uri(tString sUri);
        
        //! @brief Get DoRedo
        tString DoRedo() const;
        
        
        // Json ===============================================================
        /// @brief		WriteJson().
        //! @param[in] sUndoSpreadSheet tUndoSpreadSheet%
        void WriteJson(tUndoSpreadSheet* sUndoSpreadSheet);
        
        /// @brief		Reader Json.
        //! @return   tUndoSpreadSheet%
        tUndoSpreadSheet*  ReadJson(tString sJson);
        
        
        tString Json();
        
    };
#ifdef TestMultiUser
    //! @brief tDispatcher used by server
    class tTestDispatcher : public tClass {
    private:
        tVectorUser m_Users;
    public:
        //! @brief Constructor
        tTestDispatcher();
        
        //! @brief Destructor
        ~tTestDispatcher();
        
        //! @brief FindUser
        //! @param[in] sEmail tString
        //! @return tUser*
        tTestUser* FindUser(tString sEmail);
        
        //! @brief AddUser
        //! @param[in] sEmail tString
        //! @return tUser*
        tTestUser* AddUser(tString sEmail);
        
        //! @brief AddUser
        //! @param[in] sUser tUser*
        void AddUser(tTestUser* sUser);
        
        //! @brief RemoveUser
        //! @param[in] sEmail tString
        //! @return tBool
        tBool RemoveUser(tString sEmail);

        //! @brief Number of registered users including the pseudo "Server".
        //!
        //! tInterfaceWeb uses this count to decide whether multiple human
        //! authors are currently connected: when UserCount() is greater than
        //! 2 (Server + more than one human) a rename of a sheet or named
        //! range is refused because there is no safe way to propagate the
        //! textual rewrite on peers.
        //! @return std::size_t
        std::size_t UserCount() const;
        
        
        // Message ============================================================
        // Web
        //! @brief Disptacht Message (Server)
        //! @param[in] sInterface tInterface
        //! @param[in] sMessage tString
        //! @return tBool
        tBool DispatchMessage(tInterfaceWeb* sInterface,tString sMessage);
        
        //! @brief Post Message to the server
        //! @param[in] sInterface tInterface
        //! @param[in] sMessage tString
        //! @return tBool
        tBool PostMessage(tInterfaceWeb* sInterface,tString sMessage);
        
        //! @brief GetMessage
        //! @param[in] sInterface tInterface
        //! @param[in] sMessage tString
        //! @return tString
        tBool GetMessage(tInterfaceWeb* sInterface,tString sMessage);
    };

    tString ChangeUri(tString sMessage,tString sUserStr);
#endif
    // Json functions ========================================================
    //! @brief StringForJson
    //! @param[in] result tString
    //! @return tString
    tString StringForJson(tString result);

    //! @brief JsonBoolResult
    //! @param[in] result bool
    //! @return tString
    tString JsonBoolResult(bool result);

    //! @brief JsonStringResult
    //! @param[in] result tString
    //! @return tString
    tString JsonStringResult(tString result);

    //! @brief JsonObjectResult
    //! @param[in] result tString
    //! @return tString
    tString JsonObjectResult(tString result);

    //! @brief JsonDoubleResult
    //! @param[in] result tString
    //! @return tString
    tString JsonDoubleResult(tString result);

    //! @brief ShowParseErrorJson
    //! @param[in] sDocJson Document*
    //! @param[in] sJsonStr tString
    void ShowParseErrorJson(Document* sDocJson,tString sJsonStr);

    // Factory functions for tUndoSpreadSheet =================================
    //! @brief CreateUndoSpreadSheet - Factory function to create undo objects
    //! @param[in] sClassName tString - The class name of the undo object to create
    //! @return tUndoSpreadSheet* - Pointer to the created undo object, or nullptr if class name is unknown
    tUndoSpreadSheet* CreateUndoSpreadSheet(tString sClassName);

    //! @brief GetUndoClassNameFromJson - Helper function to extract undo class name from JSON
    //! @param[in] sValue const rapidjson::Value& - JSON value containing the undo object
    //! @return tString - The class name extracted from JSON, or empty string if not found
    tString GetUndoClassNameFromJson(const rapidjson::Value& sValue);

#if defined(checkfo) && defined(TestMultiUser)
    class tCheckFormat {
    private:
        tVectorUser m_Users;
    public:
        //! @brief Constructor
        tCheckFormat();
        
        //! @brief Destructor
        ~tCheckFormat();
        
        //! brief Clear vector User
        void Clear();
        
        //! @brief CheckFormat
        void CheckFormat();
        
        //! @brief AddUser
        //! @param[in] sUser tUser*
        void AddUser(tTestUser* sUser);
        
        //! @brief Instance
        //! @return tCheckFormat*
        static tCheckFormat* Instance();
    };
#endif

} // namespace SkSpreadSheet


#endif /* SkMessage_hpp */

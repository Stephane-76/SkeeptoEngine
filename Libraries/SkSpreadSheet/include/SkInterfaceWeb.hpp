//=============================================================================
// SkSpreadSheet Interface
// SkInterface is the interface for the SkSpreadSheet
//=============================================================================
#ifndef SkInterfaceWeb_hpp
#define SkInterfaceWeb_hpp

#include "../include/SkApi.hpp"
#include "../include/SkUndoRedo.hpp"
#include "../include/SkMessage.hpp"
#include "../include/SkUser.hpp"

namespace SkSpreadSheet {

class tInterfaceWeb : public tApi {
private:
    // Redo Container 
    tUndoRedoContainer* m_UndoRedoContainer;
    //! @brief User
    //!  Email
    tString m_Email;
    
    //! Name
    tString m_Name;
    
    //! First Name
    tString m_FirstName;
    
    //! @brief WorkBookUri
    tString m_WorkBookUri;
#ifdef TestMultiUser
    //! @brief User
    tString m_User;
    
    //! @brief Dispatcher
    tTestDispatcher* m_Dispatcher;
#endif
public:
    //! @brief Constructor
    tInterfaceWeb();
    
#ifdef TestMultiUser
    //! @brief Constructor
    //! @param[in] sUser tStringr
    //! @param[in] sWorkBookUri  tString
    tInterfaceWeb(tString sUser, tString sWorkBookUri);
#endif
    
    //! @brief Destructor
    virtual ~tInterfaceWeb();
    
    
    //! @brief Clear
    void Clear();
    
    //! @brief Set User
    //! @param[in] sEmail tString
    //! @param[in] sName tString
    //! @param[in] sFirstName tString
    void User(tString sEmail,tString sName,tString sFirstName);
    
    //! @brief User email
    //! @return tString
    tString Email() const;
    
    tString Name() const;
    
    tString FirstName() const;
    
    //! @brief Set UriWorkBook
    //! @param[in] sUriWorkBook tString
    void UriWorkBook(tString sUriWorkBook);
    
    //! @brief UriWorkBook
    //! @return tString
    tString UriWorkBook() const;

    // Keep the setter overload visible: declaring the getter below would
    // otherwise hide every tApi::MultiUserActive signature (name hiding).
    using tApi::MultiUserActive;

    //! @brief Override that derives the multi-user state from the dispatcher.
    //!
    //! In tests built with TestMultiUser, the dispatcher holds a registry of
    //! every connected user plus a pseudo "Server" entry. As soon as there
    //! is more than one human connected (i.e. UserCount() > 2) this method
    //! returns true, which makes IsRenameAllowed() refuse sheet and named
    //! range renames. In production builds, the base-class flag set via
    //! MultiUserActive(bool) from JS is used instead.
    //! @return tBool
    tBool MultiUserActive() override;
#ifdef TestMultiUser
    //! @brief Dispatcher
    //! @return tDispatcher&
    tTestDispatcher& Dispatcher();
#endif

    //! @brief Activate m_WorkBookUri on the container before Client Do/Undo/Redo.
    //! @return tBool
    tBool SetActiveWorkBook();

    tBool UndoCellValue(tString sRef, tVariant sValue, tSheet* sSheet = nullptr) override;
    tBool Copy(tString sRef, tSheet* sSheet = nullptr) override;
    tBool UndoPaste(tString sRef, tSheet* sSheet = nullptr) override;
    tBool UndoCut(tString sRef, tSheet* sSheet = nullptr) override;
    tBool UndoMove(tString sSourceRef, tString sDestRef, tSheet* sSheet = nullptr) override;
    tCell* Cell(tIndex sRow, tIndex sCol, tSheet* sSheet = nullptr) override;
    tCell* Cell(tString sRef, tSheet* sSheet = nullptr) override;
    tRange* FindRangeNamed(tString sName) override;

    // Interface Undo =====================================================
    /// @brief      Do local.
    /// @param[in]    sUndo tUndo*
    /// @return        tBool
    tBool Do(tUndo* sUndo) override;
    
    /// @brief      Undo local.
    /// @return     tBool
    tBool Undo() override;
    
    /// @brief      Redo local.
    /// @return     tBool
    tBool Redo() override;
    
    /// @brief      Get last undo.
    /// @return     tUndo*
    tUndo* LastUndo() override;
    
    /// @brief      Get last redo.
    /// @return     tUndo*
    tUndo* LastRedo() override;
    
    
    /// Message Chat WEB ==================================================
    /// @brief PostMessage to Serveur, other user.
    /// @param[in] sMessage tString
    /// @return tBool
    tBool PostMessage(tString sMessage) override;
     
    /// @brief Receive Message from  Serveur,  other user.
    /// @param[in] sMessage tString
    /// @return tBool
    tBool GetMessage(tString sMessage) override;

    // Json User =============================================================
    //! @brief Set Json User
    //! @param[in] sJson tString
    void JsonUser(tString sJson);
    
    //! @brief Get Json User
    //! @return tString
    tString JsonUser();
    
#if defined(checkfo) && defined(TestMultiUser)
    /// @brief      Get undo vector .
    /// @return     tVectorUndio
    tVectorUndo GetUndoVector();

    /// @brief      Get  redo  vector .
    /// @return     tVectorUndio
    tVectorUndo GetRedoVector();
    
    /// @brief      Check format.
    void CheckFormat() override;
#endif
};

} // end of name space

#endif /* SkInterface_hpp */

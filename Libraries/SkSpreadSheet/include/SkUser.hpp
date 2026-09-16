//=========================================================================
// SkUser.hpp
//=========================================================================
#ifndef SkUser_hpp
#define SkUser_hpp
#include "SkApplication.hpp"

// Use TestMultiUser for unit tests only
// in CMakeLists.txt: -DSK_SPREADSHEET_TEST_MULTI_USER=ON
//#define TestMultiUser

// Multi-user *test helpers* (tTestUser, tTestDispatcher): enable only when CMake passes
// -DSK_SPREADSHEET_TEST_MULTI_USER=ON on lib SkSpreadSheet and/or test targets (compilSpreadSheet.sh).
// Default (compilAll_Xcode.sh / compilAll_Wasm.sh): OFF — collaboration uses tInterfaceWeb::Client()
// and tUISpreadSheet::PostMessage() → JavaScript (SkSpChat).
#ifdef TestMultiUser
namespace SkSpreadSheet {
  
    class tInterfaceWeb;

    class tTestUser : public tClass {
        private:
            tString m_Email;
            tString m_Name;
            tString m_WorkBookUri;
            tString m_SheetName;
            tDate   m_LastView;
            tInterfaceWeb* m_Interface;
        public:
            //! @brief Constructor
            //! @param sEmail The email of the user
            tTestUser(tString sEmail);

            //! @brief Copy constructor
            tTestUser(const tTestUser& sUser);

            //! @brief Destructor
            ~tTestUser();

            //! @brief Get the email of the user
            //! @return The email of the user
            tString Email() const;

            //! @brief Get the name of the user
            //! @return The name of the user
            tString Name() const;

            //! @brief Set the name of the user
            void Name(tString sName);

    
            //! @brief Get the work book uri of the user
            //! @return The work book uri of the user
            tString WorkBookUri() const;

            //! @brief Set the work book uri of the user
            void WorkBookUri(tString sWorkBookUri);

            //! @brief Get the sheet name of the user
            //! @return The sheet name of the user
            tString SheetName() const;

            //! @brief Set the sheet name of the user
            void SheetName(tString sSheetName);

            //! @brief Get the last view of the user
            //! @return The last view of the user
            tDate LastView() const;

            //! @brief Set the last view of the user
            void LastView(tDate sLastView);
            

            //! @brief Get the interface undo of the user
            //! @return The interface undo of the user
            tInterfaceWeb* Interface() const;

            //! @brief Set the interface undo of the user
            void Interface(tInterfaceWeb* sInterface);

            //! @brief Get the interface undo of the user
            //! @return The interface undo of the user
            tInterfaceWeb* Interface();


            //! @brief Copy operator
            tTestUser& operator=(const tTestUser& sUser);
    };

    typedef vector<tTestUser*> tVectorUser;
}
#endif //TestMultiUser

#endif

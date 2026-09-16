//=========================================================================
// SkUser.cpp
//=========================================================================

#include "../include/SkUser.hpp"
#include "../include/SkInterfaceWeb.hpp"
#ifdef TestMultiUser
namespace SkSpreadSheet {
    tTestUser::tTestUser(tString sEmail) : m_Email(sEmail), m_Name(), m_WorkBookUri(), m_SheetName(), m_LastView()
    #ifdef TestMultiUser
        ,m_Interface(nullptr)
    #endif
    {} 
    
    

    tTestUser::tTestUser(const tTestUser& sUser) : m_Email(sUser.m_Email), m_Name(sUser.m_Name), m_WorkBookUri(sUser.m_WorkBookUri), m_SheetName(sUser.m_SheetName), m_LastView(sUser.m_LastView)
    #ifdef TestMultiUser
        ,m_Interface(sUser.m_Interface)
    #endif
    {
    }
    
    tTestUser::~tTestUser() {
    }
    
    tString tTestUser::Email() const { return m_Email; }

    tString tTestUser::Name() const { return m_Name; }
    void tTestUser::Name(tString sName) { m_Name = sName; }

    tString tTestUser::WorkBookUri() const { return m_WorkBookUri; }
    void tTestUser::WorkBookUri(tString sWorkBookUri) { m_WorkBookUri = sWorkBookUri; }

    tString tTestUser::SheetName() const { return m_SheetName; }
    void tTestUser::SheetName(tString sSheetName) { m_SheetName = sSheetName; }

    tDate tTestUser::LastView() const { return m_LastView; }
    void tTestUser::LastView(tDate sLastView) { m_LastView = sLastView; }
  
#ifdef TestMultiUser
    tInterfaceWeb* tTestUser::Interface() const { return m_Interface; }

    void tTestUser::Interface(tInterfaceWeb* sInterface) { m_Interface = sInterface; }

    tInterfaceWeb* tTestUser::Interface() { return m_Interface; }
#endif

    tTestUser& tTestUser::operator=(const tTestUser& sUser) {
        m_Name = sUser.m_Name;
        m_WorkBookUri = sUser.m_WorkBookUri;
        m_SheetName = sUser.m_SheetName;
        m_LastView = sUser.m_LastView;
        #ifdef TestMultiUser
            m_Interface = sUser.m_Interface;
        #endif
        return *this;
    }

   
}   
#endif // TestMultiUser

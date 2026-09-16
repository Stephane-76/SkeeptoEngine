//=============================================================================
// SkMessage
//=============================================================================
#include "../include/SkMessage.hpp"
#include "../include/SkUndoRedoSp.hpp"
#include "../include/SkInterfaceWeb.hpp"

// Toggle collaboration trace: uncomment the next line to enable dispatcher logs.
//#define _debuginterface
#ifdef _debuginterface
#ifndef debuginterface
#define debuginterface
#endif
#endif

namespace SkSpreadSheet {

#ifdef TestMultiUser
#ifdef debuginterface
    static void DebugCollabRoute(const char* sStage, tInterfaceWeb* sInterface, tString sMessage, tString sTargetUser = {}) {
        tMessage wProbe;
        tUndoSpreadSheet* wUndo = wProbe.ReadJson(sMessage);
        cout << "[Collab] " << sStage;
        if (sInterface != nullptr) {
            cout << " uri=" << sInterface->UriWorkBook();
            if (sInterface->Client()) {
                cout << " role=Client";
            } else if (sInterface->Server()) {
                cout << " role=Server";
            }
        }
        if (!sTargetUser.empty()) {
            cout << " target=" << sTargetUser;
        }
        cout << " from=" << wProbe.User() << " doRedo=" << wProbe.DoRedo();
        if (wUndo != nullptr) {
            cout << " class=" << wUndo->ClassName();
        }
        cout << endl;
        delete wUndo;
    }
#endif
#endif
   
    // Message ==================================================================
    tMessage::tMessage() : tClass(),
    m_Email(),
    m_Name(),
    m_FirstName(),
    m_Uri(),
    m_DoRedo()
#ifdef TestMultiUser
    , m_User()
#endif
    , m_Json() {}

#ifdef TestMultiUser
    tMessage::tMessage(tString sUser, tString sUri,tString sDoRedo) : tClass(),
    m_Email(),
    m_Name(),
    m_FirstName(),
    m_Uri(sUri),
    m_DoRedo(sDoRedo),
    m_User(sUser),
    m_Json() {
    }
#else
    tMessage::tMessage(tString sEmail, tString sName, tString sFirstName, tString sUri,tString sDoRedo) : tClass(), m_Email(sEmail), m_Name(sName), m_FirstName(sFirstName), m_Uri(sUri),m_DoRedo(sDoRedo) {
    }
#endif

    tMessage::~tMessage() {}

    tString tMessage::Email() const {
        return m_Email;
    }


    tString tMessage::Name() const {
        return m_Name;
    }


    tString tMessage::FirstName() const {
        return m_FirstName;
    }
    tString tMessage::Uri() const {
        return m_Uri;
    }

    void tMessage::Uri(tString sUri) {
        m_Uri=sUri;
    }

    tString tMessage::DoRedo() const {
        return m_DoRedo;
    }


    #ifdef TestMultiUser
        tString tMessage::User() const {
            return m_User;
        }
    #endif

    void tMessage::WriteJson(tUndoSpreadSheet* sUndoSpreadSheet) {
        StringBuffer wBuffer;
        Writer<StringBuffer> wWriter(wBuffer);
        wWriter.StartObject();
#ifdef TestMultiUser
        wWriter.Key(kJsonKeyUser);
        wWriter.String(m_User.c_str());
#endif
        wWriter.Key(kJsonKeyEmail);
        wWriter.String(m_Email.c_str());
        
        wWriter.Key(kJsonKeyName);
        wWriter.String(m_Name.c_str());
        
        wWriter.Key(kJsonKeyFirstName);
        wWriter.String(m_FirstName.c_str());

        wWriter.Key(kJsonKeyUri);
        wWriter.String(m_Uri.c_str());

        wWriter.Key(kJsonKeyOp);
        wWriter.String(m_DoRedo.c_str());

        wWriter.Key(kJsonKeyMsg);
        wWriter.StartObject();
        // Wrie with US
        tString wLang=tApplication::Instance()->Locale()->Lang();;
        tApplication::Instance()->Locale("us");
        sUndoSpreadSheet->Json(&wWriter);
        tApplication::Instance()->Locale(wLang);;
        wWriter.EndObject();
        wWriter.EndObject();

        m_Json=wBuffer.GetString();
    }

    tUndoSpreadSheet* tMessage::ReadJson(tString sJson) {
        rapidjson::Document wDocument;
        rapidjson::ParseResult wParseResult = wDocument.Parse(sJson.c_str());
        if (wParseResult.IsError()) {
            cerr << "Failed to parse Message JSON" << endl;
            ShowParseErrorJson(&wDocument,sJson);
                
            throw tExceptionInternalError("Failed to parse Message JSON");
        }
#ifdef TestMultiUser
        if (wDocument.HasMember(kJsonKeyUser) && wDocument[kJsonKeyUser].IsString()) {
            m_User=wDocument[kJsonKeyUser].GetString();
        }
#endif
        if (wDocument.HasMember(kJsonKeyEmail)) {
            m_Email=wDocument[kJsonKeyEmail].GetString();
        }
        if (wDocument.HasMember(kJsonKeyName)) {
            m_Name=wDocument[kJsonKeyName].GetString();
        }
        if (wDocument.HasMember(kJsonKeyFirstName)) {
            m_FirstName=wDocument[kJsonKeyFirstName].GetString();
        }
        // Get the uri
        if (wDocument.HasMember(kJsonKeyUri) && wDocument[kJsonKeyUri].IsString()) {
            m_Uri=wDocument[kJsonKeyUri].GetString();
        }
        // Get the undo (Do Undo Redo)
        if (wDocument.HasMember(kJsonKeyOp) && wDocument[kJsonKeyOp].IsString()) {
            m_DoRedo=wDocument[kJsonKeyOp].GetString();
        }
        
        // Get the message object
        if (wDocument.HasMember(kJsonKeyMsg) && wDocument[kJsonKeyMsg].IsObject()) {
            const rapidjson::Value& wMsgValue = wDocument[kJsonKeyMsg];
            
            // Get the undo class name from JSON
            tString wClassName = GetUndoClassNameFromJson(wMsgValue);
            if (wClassName.empty()) {
                cerr << "No undo class name found in JSON" << endl;
                return nullptr;
            }
            
            // Create the undo object using factory
            tUndoSpreadSheet* wUndo = CreateUndoSpreadSheet(wClassName);
            if (wUndo != nullptr) {
                // Read With Us Langaege
                tString wLang=tApplication::Instance()->Locale()->Lang();;
                tApplication::Instance()->Locale("us");
                wUndo->IsJson(true); // Set IsJson to true
                // Deserialize the undo object from JSON
                wUndo->Json(wMsgValue);
                tApplication::Instance()->Locale(wLang);;
                return wUndo;
            }
        }
        
        return(nullptr);
    }

    
    tString tMessage::Json() {
        return(m_Json);
    }

#ifdef TestMultiUser
    // tDispatcher ============================================================
    tTestDispatcher::tTestDispatcher() : tClass(), m_Users() {
    }

    tTestDispatcher::~tTestDispatcher() {
        m_Users.clear();
    }

    tTestUser* tTestDispatcher::FindUser(tString sEmail) {
        for (tTestUser* wUser : m_Users) {
            if (wUser->Email() == sEmail) {
                return wUser;
            }
        }
        return nullptr;
    }   

    tTestUser* tTestDispatcher::AddUser(tString sEmail) {
        tTestUser* wUser=FindUser(sEmail);
        if (wUser==nullptr) {
            wUser=new tTestUser(sEmail);
            m_Users.push_back(wUser);
        }
        return wUser;
    }

    void tTestDispatcher::AddUser(tTestUser* sUser) {
         tTestUser* wUser=FindUser(sUser->Email());
        if (wUser==nullptr) {
            m_Users.push_back(sUser);
        }
    }

    std::size_t tTestDispatcher::UserCount() const {
        return(m_Users.size());
    }

    tBool tTestDispatcher::RemoveUser(tString sEmail) {
        tTestUser* wUser=FindUser(sEmail);
        if (wUser!=nullptr) {
            m_Users.erase(std::remove(m_Users.begin(), m_Users.end(), wUser), m_Users.end());
            return true;
        }
        return false;
    }

    // Message ================================================================
    tBool tTestDispatcher::DispatchMessage(tInterfaceWeb* sInterface,tString sMessage) {
#ifdef debuginterface
        DebugCollabRoute("DispatchMessage(server->peers)", sInterface, sMessage);
#endif
        // Post the message to the server
        // Get the user
       rapidjson::Document wDocument;
        rapidjson::ParseResult parseResult = wDocument.Parse(sMessage.c_str());
        if (parseResult.IsError()) {
            cerr << "Failed to parse Message JSON" << endl;
            ShowParseErrorJson(&wDocument,sMessage);
                
            throw tExceptionInternalError("Failed to parse Message JSON");
        }
        tTestUser* wOwnerUser=nullptr;
        // Get the user
        if (wDocument.HasMember(kJsonKeyUser) && wDocument[kJsonKeyUser].IsString()) {
            tString wUserStr=wDocument[kJsonKeyUser].GetString();
            wOwnerUser=FindUser(wUserStr);
            if (wOwnerUser==nullptr) {
                cerr << "User not found" << endl;
                return(false);
            }
        }
        
        
        // test MultiUser
        tMessage wMessageProbe;
        tUndoSpreadSheet* wUndoProbe = wMessageProbe.ReadJson(sMessage);
        tBool wRelayToOwner = false;
        if (wUndoProbe != nullptr) {
            tString wDoRedo = wMessageProbe.DoRedo();
            const tString wClassName = wUndoProbe->ClassName();
            wRelayToOwner = (wDoRedo == "Undo" &&
                (wClassName == "tUndoMove" || wClassName == "tUndoPaste" || wClassName == "tUndoCut"));
        }
        delete(wUndoProbe);

        for(auto wUser : m_Users) {
            // Paste/move undo: originator applies via GetMessage too (no local Undo()).
            if (wRelayToOwner || wUser->Email() != wOwnerUser->Email()) {
#ifdef debuginterface
                DebugCollabRoute("DispatchMessage->GetMessage", sInterface, sMessage, wUser->Email());
#endif
                tMessage wMessage;
                tUndoSpreadSheet* wUndo=wMessage.ReadJson(sMessage);
                wUndo->Client(true);
                wUser->Interface()->GetMessage(sMessage);
                delete(wUndo);
            }
        }
       

        return(true);
    }

    // Post the message to the server
    tBool tTestDispatcher::PostMessage(tInterfaceWeb* sInterface,tString sMessage) {
#ifdef debuginterface
        DebugCollabRoute("PostMessage(client->server)", sInterface, sMessage);
#endif
        // Post the message to the server
        // Get the user
     
        rapidjson::Document wDocument;
        rapidjson::ParseResult parseResult = wDocument.Parse(sMessage.c_str());
        if (parseResult.IsError()) {
            cerr << "Failed to parse Message JSON" << endl;
            ShowParseErrorJson(&wDocument,sMessage);
                
            throw tExceptionInternalError("Failed to parse Message JSON");
        }
        // 
        if (sInterface->Client()) {
            // test MultiUser

            tTestUser* wServer=FindUser("Server");
            if (wServer==nullptr) {
                //cerr << "Server not found" << endl;
                return(false);
            }
            // GetMessage to Server
#ifdef debuginterface
            DebugCollabRoute("PostMessage->GetMessage(server)", sInterface, sMessage, wServer->Email());
#endif
            return(wServer->Interface()->GetMessage(sMessage));

        }
        // If the interface is a server
        if (sInterface->Server()) {
            // Dispatch the message 
            DispatchMessage(sInterface, sMessage);
            tTestUser* wServer=FindUser("Server");
            if (wServer==nullptr) {
                //cerr << "Server not found" << endl;
                return(false);
            }
#ifdef debuginterface
            DebugCollabRoute("PostMessage->GetMessage(server mirror)", sInterface, sMessage, wServer->Email());
#endif
            wServer->Interface()->GetMessage(sMessage);
        }

        return(true);
    }

    // Get the message from the server
    tBool tTestDispatcher::GetMessage(tInterfaceWeb* sInterface,tString sMessage) {
        rapidjson::Document wDocument;
        rapidjson::ParseResult parseResult = wDocument.Parse(sMessage.c_str());
        if (parseResult.IsError()) {
            cerr << "Failed to parse Message JSON" << endl;
            ShowParseErrorJson(&wDocument,sMessage);
                
            return(false);
        }
        tTestUser* wUser=nullptr;
        // Get the user
        if (wDocument.HasMember("r_usr") && wDocument["r_usr"].IsString()) {
            tString wUserStr=wDocument["r_usr"].GetString();
#ifdef debuginterface
            cout << "Get Message -->" << wUserStr << endl;
#endif
            wUser=FindUser(wUserStr);
            if (wUser==nullptr) {
                cerr << "User not found" << endl;
                return(false);
            }
        }
        
        // test MultiUser

            // Post Message to server 
        if (wUser->Interface()->Client()) {
#ifdef debuginterface
            DebugCollabRoute("GetMessage(server->client)", sInterface, sMessage, wUser->Email());
#endif
            return(wUser->Interface()->GetMessage(sMessage));
        }

        return(true);
    }
#endif // end TestMultiUser
    

#if defined(checkfo) && defined(TestMultiUser)
    // Constanre of instance
    tCheckFormat* wStaticCheckFormat = nullptr;
    
    tCheckFormat::tCheckFormat() {}

    tCheckFormat::~tCheckFormat() {
    }

    void tCheckFormat::Clear() {
        m_Users.clear();
    };

    void tCheckFormat::CheckFormat() {
        tSpreadSheetContainer* wStaticSpreadSheet=tSpreadSheetContainer::Instance();
        tVectorWorkBookClass wVectorWorkBookClass;
        wStaticSpreadSheet->WorkBooksList(wVectorWorkBookClass);
        
        tFormatApi* wFormatApi=wStaticSpreadSheet->FormatApi();
        if (wFormatApi!=nullptr) {
            wFormatApi->ResetCheck();
            for(auto wWorkBook : wVectorWorkBookClass) {
                // Check
                //cout << "Check " << wWorkBook->Uri() << endl;
                wWorkBook->CheckFormat();
            }
            for(auto wUser : m_Users) {
                //cout << wUser->Name() << endl;
                // Get Undo Redo for each user and check
                tInterfaceWeb* wInterfaceWeb=wUser->Interface();
                for(auto wUndo : wInterfaceWeb->GetUndoVector()) {
                    wStaticSpreadSheet->CheckFormatUndo(wUndo);
                }
                for(auto wRedo : wInterfaceWeb->GetRedoVector()) {
                    wStaticSpreadSheet->CheckFormatUndo(wRedo);
                }
            }
            wFormatApi->Check();
        }
    }

    void tCheckFormat::AddUser(tTestUser* sUser) {
        m_Users.push_back(sUser);
    }

    tCheckFormat* tCheckFormat::Instance() {
        if (wStaticCheckFormat==nullptr) {
            wStaticCheckFormat=new tCheckFormat();
        }
        return wStaticCheckFormat;
    }
#endif
    // Json functions =========================================================
    tString StringForJson(tString result) {
        tString wResult = result;
        // Escape quotes in the result string
        size_t pos = 0;
        while ((pos = wResult.find("\"", pos)) != tString::npos) {
            wResult.replace(pos, 1, "\\\"");
            pos += 2; // Move to next character after added escape
        }
        return(wResult);
    }

    tString JsonBoolResult(bool result) {
        if (result) return "{\"result\":true}";
        return "{\"result\":false}";
    }


    tString JsonStringResult(tString result) {
        return (tString("{\"result\":\"") + result + tString("\"}"));
    }

    tString JsonObjectResult(tString result) {
        tString wResult = StringForJson(result);
        
        // Create encapsulated JSON with result as string
        return tString("{\"result\":\"") + wResult + tString("\"}");
    }

    tString JsonDoubleResult(tString result) {
        return tString("{\"result\":") + result + tString("}");
    }

    void ShowParseErrorJson(Document* sDocJson,tString sJsonStr) {
        cerr << "JSON parsing error at position " << sDocJson->GetErrorOffset() << endl;
        cerr << "Error: " << rapidjson::GetParseError_En(sDocJson->GetParseError()) << endl;
        
        // Display context around the error
        size_t errorPos = sDocJson->GetErrorOffset();
        size_t start = (errorPos > 20) ? errorPos - 20 : 0;
        size_t length = 40; // Display 40 characters around the error
        if (start + length > sJsonStr.length()) {
            length = sJsonStr.length() - start;
        }
        
        cerr << "Context: ..." << sJsonStr.substr(start, length) << "..." << endl;
        cerr << "         " << string(errorPos - start, ' ') << "^" << endl;
    }

    // Factory for tUndoSpreadSheet classes ====================================
    tUndoSpreadSheet* CreateUndoSpreadSheet(tString sClassName) {
        if (sClassName == "tUndoChangeSize") {
            return new tUndoChangeSize();
        }
        else if (sClassName == "tUndoSpreadSheetCallBack") {
            return new tUndoSpreadSheetCallBack();
        }
        else if (sClassName == "tUndoApplyMerge") {
            return new tUndoApplyMerge();
        }
        else if (sClassName == "tUndoRaz") {
            return new tUndoRaz();
        }
        else if (sClassName == "tUndoCellValue") {
            return new tUndoCellValue();
        }
        else if (sClassName == "tUndoFillSeries") {
            return new tUndoFillSeries();
        }
        else if (sClassName == "tUndoCellClassCalculable") {
            return new tUndoCellClassCalculable();
        }
        else if (sClassName == "tUndoCellAttribute") {
            return new tUndoCellAttribute();
        }
        else if (sClassName == "tUndoCellClassAttributes") {
            return new tUndoCellClassAttributes();
        }
        else if (sClassName == "tUndoCellClass") {
            return new tUndoCellClass();
        }
        else if (sClassName == "tUndoFormat") {
            return new tUndoFormat();
        }
        else if (sClassName == "tUndoPrecision") {
            return new tUndoPrecision();
        }
        else if (sClassName == "tUndoConditionalFormat") {
            return new tUndoConditionaFormat();
        }
        else if (sClassName == "tUndoDeleteConditionaFormat") {
            return new tUndoDeleteConditionaFormat();
        }
        else if (sClassName == "tUndoBorder") {
            return new tUndoBorder();
        }
        else if (sClassName == "tUndoPaste") {
            return new tUndoPaste();
        }
        else if (sClassName == "tUndoCut") {
            return new tUndoCut();
        }
        else if (sClassName == "tUndoMove") {
            return new tUndoMove();
        }
        else if (sClassName == "tUndoAddRangeNamed") {
            return new tUndoAddRangeNamed();
        }
        else if (sClassName == "tUndoDeleteRangeNamed") {
            return new tUndoDeleteRangeNamed();
        }
        else if (sClassName == "tUndoUpdateRangeNamed") {
            return new tUndoUpdateRangeNamed();
        } 
        else if (sClassName == "tUndoAddRangeData") {
            return new tUndoAddRangeData();
        }
        else if (sClassName == "tUndoApplyRangeData") {
            return new tUndoApplyRangeData();
        }
        else if (sClassName == "tUndoInsertCol") {
            return new tUndoInsertCol();
        }
        else if (sClassName == "tUndoInsertRow") {
            return new tUndoInsertRow();
        }
        else if (sClassName == "tUndoInsertRowWithLabel") {
            return new tUndoInsertRowWithLabel();
        }
        else if (sClassName == "tUndoDeleteCol") {
            return new tUndoDeleteCol();
        }
        else if (sClassName == "tUndoDeleteRow") {
            return new tUndoDeleteRow();
        }
        else if (sClassName == "tUndoAddSheet") {
            return new tUndoAddSheet();
        }
        else if (sClassName == "tUndoRenameSheet") {
            return new tUndoRenameSheet();
        }
        else if (sClassName == "tUndoSwapSheet") {
            return new tUndoSwapSheet();
        }
        else if (sClassName == "tUndoDeleteSheet") {
            return new tUndoDeleteSheet();
        } else if (sClassName == "tUndoDeleteFormulaNamed") {
            return new tUndoDeleteFormulaNamed();
        }
        else if (sClassName == "tUndoInsertFormulaNamed") {
            return new tUndoInsertFormulaNamed();
        }
        else if (sClassName == "tUndoInsertFloatingObject") {
            return new tUndoInsertFloatingObject();
        }
        else if (sClassName == "tUndoDeleteFloatingObject") {
            return new tUndoDeleteFloatingObject();
        }
        else if (sClassName == "tUndoFloatingObjectLayout" || sClassName == "tUndoFloatChangeSizePos") {
            return new tUndoFloatingObjectLayout();
        }
        else if (sClassName == "tUndoOpenCloseTree") {
            return new tUndoOpenCloseTree();
        }
        else if (sClassName == "tUndoChangeTree") {
            return new tUndoChangeTree();
        }
        else if (sClassName == "tUndoSplitView") {
            return new tUndoSplitView();
        }
        else if (sClassName == "tUndoPrintParameters") {
            return new tUndoPrintParameters();
        }
        else if (sClassName == "tUndoChangeSize") {
            return new tUndoChangeSize();
        }
        else {
            // Return nullptr if class name is not recognized
            cerr << "Unknown undo class name: " << sClassName << endl;
            return nullptr;
        }
    }

    // Helper function to get undo class name from JSON
    tString GetUndoClassNameFromJson(const rapidjson::Value& sValue) {
        if (sValue.HasMember(kJsonKeyUndo) && sValue[kJsonKeyUndo].IsString()) {
            return sValue[kJsonKeyUndo].GetString();
        }
        return "";
    }

} // namespace SkSpreadSheet

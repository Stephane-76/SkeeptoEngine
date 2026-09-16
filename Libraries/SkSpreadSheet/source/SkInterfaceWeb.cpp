//=============================================================================
// SkSpreadSheet Interface
// SkInterface is the interface for the SkSpreadSheet
//=============================================================================
#include "../include/SkInterfaceWeb.hpp"
#include "../include/SkUndoRedoRebase.hpp"
#include "../include/SkUndoRedoSp.hpp"
#include "../include/SkCollaborationLimits.hpp"
#include <SkUID.hpp>
#ifdef __EMSCRIPTEN__
// fprintf avoids iostream/syscall paths that can interact badly with Wasm EH / invoke_* glue.
#include <cstdio>
#endif

// Toggle collaboration trace: uncomment the next line to enable PostMessage/GetMessage logs.
//#define _debuginterface
#ifdef _debuginterface
#ifndef debuginterface
#define debuginterface
#endif
#endif

namespace SkSpreadSheet {

    // Interface ==================================================================
    tInterfaceWeb::tInterfaceWeb() : tApi(),
#ifdef TestMultiUser
    m_UndoRedoContainer(),
#endif
    m_Email(),
    m_Name(),
    m_FirstName(),
    m_WorkBookUri() {
#ifdef TestMultiUser
        m_Dispatcher=new tTestDispatcher();
#endif
#ifndef TestMultiUser
        m_UndoRedoContainer=tApplication::Instance()->UndoRedoContainer();
#else
        m_UndoRedoContainer=new tUndoRedoContainer();
#endif
    }

#ifdef TestMultiUser
    tInterfaceWeb::tInterfaceWeb(tString sUser, tString sWorkBookUri) : tApi(),
    m_UndoRedoContainer(),
    m_Email(),
    m_Name(),
    m_FirstName(),
    m_WorkBookUri(sWorkBookUri),
    m_User(sUser) {
        m_UndoRedoContainer=new tUndoRedoContainer();
        m_Dispatcher=new tTestDispatcher();
    }
#endif

    tInterfaceWeb::~tInterfaceWeb() {
        Clear();
#ifdef TestMultiUser
        delete(m_Dispatcher);
        m_Dispatcher=nullptr;
        delete(m_UndoRedoContainer);
#endif
    }

    void tInterfaceWeb::Clear() {
#ifdef TestMultiUser
        m_UndoRedoContainer->Clear();
#endif
    }

    void tInterfaceWeb::User(tString sEmail,tString sName,tString sFirstName) {
        m_Email=sEmail;
        m_Name=sName;
        m_FirstName=sFirstName;
    }

    tString tInterfaceWeb::Email() const {
        return(m_Email);
    }

    tString tInterfaceWeb::Name() const {
        return(m_Name);
    }

    tString tInterfaceWeb::FirstName() const {
        return(m_FirstName);
    }

    void tInterfaceWeb::UriWorkBook(tString sUriWorkBook) {
        m_WorkBookUri=sUriWorkBook;
    }

    tString tInterfaceWeb::UriWorkBook() const {
        return(m_WorkBookUri);
    }

    tBool tInterfaceWeb::MultiUserActive() {
#ifdef TestMultiUser
        // Several connected users (Server + 2+ humans) forces collaboration rules.
        if (m_Dispatcher != nullptr && m_Dispatcher->UserCount() > 2) {
            return true;
        }
#endif
        // Production and unit tests: explicit flag from JS or test setup.
        return tApi::MultiUserActive();
    }

#ifdef TestMultiUser
    tTestDispatcher& tInterfaceWeb::Dispatcher() {
        return *m_Dispatcher;
    }
#endif

    tBool tInterfaceWeb::SetActiveWorkBook() {
        tSpreadSheetContainer* wSpreadSheetContainer = tSpreadSheetContainer::Instance();
        // WASM/JS often activates workbooks via ActiveWorkBook(uri) without UriWorkBook().
        if (m_WorkBookUri.empty()) {
            tWorkBook* wActive = wSpreadSheetContainer->ActiveWorkBook();
            if (wActive != nullptr) {
                m_WorkBookUri = wActive->Uri();
            }
        }
        if (m_WorkBookUri.empty()) {
            return(false);
        }
        tBool wOk = wSpreadSheetContainer->ActiveWorkBook(m_WorkBookUri);
        if (wOk) {
#ifdef debuginterface
            cout << "SetActive WorkBook : ";
            tWorkBook* wWorkBook = wSpreadSheetContainer->ActiveWorkBook();
            tSheet* wSheet = wWorkBook->ActiveSheet();
            tColRowCellRange* wColRowCellRange = wSheet->ColRowCellRange();

            cout << wWorkBook->Uri() << ":" << wSheet->Name() << ":" << wSheet << " ColRowCellRange"
                 << wColRowCellRange << endl;
#endif
        }
        return(wOk);
    }

    tBool tInterfaceWeb::UndoCellValue(tString sRef, tVariant sValue, tSheet* sSheet) {
        if (Client() && !SetActiveWorkBook()) {
            return(false);
        }
        return(tApi::UndoCellValue(sRef, sValue, sSheet));
    }

    tBool tInterfaceWeb::Copy(tString sRef, tSheet* sSheet) {
        if (Client() && !SetActiveWorkBook()) {
            return(false);
        }
        return(tApi::Copy(sRef, sSheet));
    }

    tBool tInterfaceWeb::UndoPaste(tString sRef, tSheet* sSheet) {
        if (Client() && !SetActiveWorkBook()) {
            return(false);
        }
        if (Client()) {
            NormalizeSheet(&sSheet);
            tUndoSpreadSheet* wUndo = CreateUndoPasteOrMoveAfterCut(
                sRef, m_Mode, sSheet, m_UndoRedoContainer);
            if (wUndo == nullptr) {
                wUndo = new tUndoPaste(sRef, m_Mode, sSheet);
            }
            return(Do(wUndo));
        }
        return(tApi::UndoPaste(sRef, sSheet));
    }

    tBool tInterfaceWeb::UndoCut(tString sRef, tSheet* sSheet) {
        if (Client() && !SetActiveWorkBook()) {
            return(false);
        }
        if (Client()) {
            NormalizeSheet(&sSheet);
            tUndoCut* wUndoCut = new tUndoCut(sRef, m_Mode, sSheet);
            return(Do(wUndoCut));
        }
        return(tApi::UndoCut(sRef, sSheet));
    }

    tBool tInterfaceWeb::UndoMove(tString sSourceRef, tString sDestRef, tSheet* sSheet) {
        if (Client() && !SetActiveWorkBook()) {
            return(false);
        }
        if (Client()) {
            NormalizeSheet(&sSheet);
            tUndoMove* wUndoMove = new tUndoMove(sSourceRef, sDestRef, m_Mode, sSheet);
            return(Do(wUndoMove));
        }
        return(tApi::UndoMove(sSourceRef, sDestRef, sSheet));
    }

    tCell* tInterfaceWeb::Cell(tIndex sRow, tIndex sCol, tSheet* sSheet) {
        if (Client()) {
            (void)SetActiveWorkBook();
        }
        return(tApi::Cell(sRow, sCol, sSheet));
    }

    tCell* tInterfaceWeb::Cell(tString sRef, tSheet* sSheet) {
        if (Client()) {
            (void)SetActiveWorkBook();
        }
        return(tApi::Cell(sRef, sSheet));
    }

    tRange* tInterfaceWeb::FindRangeNamed(tString sName) {
        if (Client()) {
            if (!SetActiveWorkBook()) {
                return(nullptr);
            }
        }
        return(tApi::FindRangeNamed(sName));
    }

    tBool tInterfaceWeb::Do(tUndo* sUndo) {
        tBool wOk=false;
        if (Client()) {
            if (!SetActiveWorkBook()) {
                return(false);
            }
            tUndoSpreadSheet* wUndoSpreadSheet=dynamic_cast<tUndoSpreadSheet*>(sUndo);
            if (wUndoSpreadSheet==nullptr) {
                throw (tExceptionInternalError("Do : LastUndo is not a tUndoSpreadSheet"));
            }
        
            // Local Do and send to server after
            // Note: Registration in rebase log is done in GetMessage() for all received messages
            // This ensures consistent SequenceId handling for both local and remote operations
            wUndoSpreadSheet->IsDo(true);

            // If Do() fails the container deletes this undo — capture diagnostics before the pointer is freed.
            const tString wAttemptedUndoClass = wUndoSpreadSheet->ClassName();

            wUndoSpreadSheet->WorkBookTarget(m_WorkBookUri);
            if (tUndoSpreadSheetCallBack* wCallback = dynamic_cast<tUndoSpreadSheetCallBack*>(wUndoSpreadSheet)) {
                wCallback->RebindActiveSheet();
            }
            if (tUndoMove* wMove = dynamic_cast<tUndoMove*>(wUndoSpreadSheet)) {
                if (!wMove->EnsureCopyPayload()) {
                    delete(sUndo);
                    return(false);
                }
            }
            if (tUndoCut* wCut = dynamic_cast<tUndoCut*>(wUndoSpreadSheet)) {
                if (!wCut->EnsureCopyPayload()) {
                    delete(sUndo);
                    return(false);
                }
            }

            if (MultiUserActive() &&
                RejectCollaborationPasteIfTooLarge(wUndoSpreadSheet, true)) {
                delete(sUndo);
                return false;
            }
            
            wOk=DispatchSpreadSheetDo(m_UndoRedoContainer, wUndoSpreadSheet);
            if (wOk) {
                // Recup Last Undo
                tUndo* wUndo=m_UndoRedoContainer->LastUndo();
                tUndoSpreadSheet* wUndoSpreadSheet=dynamic_cast<tUndoSpreadSheet*>(wUndo);
                if (wUndoSpreadSheet!=nullptr) {
                    wUndoSpreadSheet->IsDo(true);
                    wUndoSpreadSheet->IsJson(false);
                    // Generate unique operation ID to find corresponding Do operation for an Undo
                    std::uint64_t wOperationId = static_cast<std::uint64_t>(tUid::New());
                    wUndoSpreadSheet->OperationId(wOperationId);
#ifdef TestMultiUser
                    if (tWorkBook* wWorkBookForLog = WorkBook(m_WorkBookUri)) {
                        RegisterCollaborationStructuralOp(wWorkBookForLog->UndoRebaseLog(), wUndoSpreadSheet, "Do");
                    }
#else
                    if (tWorkBook* wWorkBookForLog = ActiveWorkBook()) {
                        RegisterCollaborationStructuralOp(wWorkBookForLog->UndoRebaseLog(), wUndoSpreadSheet, "Do");
                    }
#endif
#ifdef debuginterface
                    wUndoSpreadSheet->DebugFlags("do direct");
#endif
#ifdef TestMultiUser
                    tMessage wMessage(m_User,m_WorkBookUri,"Do");
#else
                    tMessage wMessage(m_Email,m_Name,m_FirstName, m_WorkBookUri,"Do");
#endif
                    wMessage.Uri(ActiveWorkBook()->Uri());
#ifdef __EMSCRIPTEN__
                    // Json serialization + Host PostMessage must not unwind through broken invoke_* / table slots
                    // (seen as TypeError getWasmTableEntry is not a function). Local undo is already committed.
                    try {
#endif
                        wMessage.WriteJson(wUndoSpreadSheet);
#ifdef debuginterface
                        tString wCde=wMessage.Json();
                        cout << "Do Cde " << wCde << endl;
#endif

                        // Post message to the server (SkSpChat → websocket).
#ifdef TestMultiUser
                        m_Dispatcher->PostMessage(this,wMessage.Json());
#else
                        PostMessage(wMessage.Json());
#endif
#ifdef __EMSCRIPTEN__
                    } catch (...) {
                        std::fprintf(stderr, "[tInterfaceWeb::Do] collaboration WriteJson/PostMessage failed after local commit (class=%s)\n",
                                     wUndoSpreadSheet->ClassName().c_str());
                    }
#endif
                }
            } else {
                tStringStream wStream;
                wStream << "Local Do:" << wAttemptedUndoClass << " -> return(false)";
#ifdef __EMSCRIPTEN__
                // Important: stderr is wired in sker-app/public/index.html to console.error('C++ Error ->', …).
                // A rejected edit (business logic / CellValue refused) is not a runtime failure — use stdout.
                std::fprintf(stdout, "[SkSpreadSheet] %s\n", wStream.str().c_str());
#else
#ifdef debuginterface
                cerr << endl << wStream.str() << endl;
#endif
#endif
                //throw(wStream.str());
            }
        } else {
            wOk=m_Application->Do(sUndo);
        }
#ifdef checksp
            Check();
#endif
        return(wOk);
    }

    tBool tInterfaceWeb::Undo() {
        tBool wOk=false;
        if (Client()) {
            if (!SetActiveWorkBook()) {
                return(false);
            }
            tUndo* wUndo=m_UndoRedoContainer->LastUndo(); //.PopUndo();
            if (wUndo!=nullptr) {
                tUndoSpreadSheet* wUndoSpreadSheet=dynamic_cast<tUndoSpreadSheet*>(wUndo);
                if (wUndoSpreadSheet!=nullptr) {
                    wUndoSpreadSheet->WorkBookTarget(m_WorkBookUri);
                    if (tUndoSpreadSheetCallBack* wCallback = dynamic_cast<tUndoSpreadSheetCallBack*>(wUndoSpreadSheet)) {
                        wCallback->RebindActiveSheet();
                    }
                    wUndoSpreadSheet->IsUndo(true);
                    wUndoSpreadSheet->IsJson(false);
#ifdef TestMultiUser
                    tWorkBook* wWorkBook = WorkBook(m_WorkBookUri);
#else
                    tWorkBook* wWorkBook = ActiveWorkBook();
#endif

#ifdef debuginterface
                    wUndoSpreadSheet->DebugFlags("Undo direct");
                    // Debug Operations
                    tString wDebugOperation=wUndoSpreadSheet->WorkBook()->UndoRebaseLog().DebugUndoOperations(wUndoSpreadSheet);
                    cout << wDebugOperation << endl;
#endif
#ifdef TestMultiUser
                    tMessage wMessage(m_User,m_WorkBookUri,"Undo");
#else
                    tMessage wMessage(m_Email,m_Name,m_FirstName,m_WorkBookUri,"Undo");
#endif
#ifdef debuginterface
#ifdef _DEBUGSK
                    tSaveSelect* wSaveSelect=wUndoSpreadSheet->SaveSelect();
                    cout << "Before Rebase Ref:" << wUndoSpreadSheet->RefRebase();
                    if (wSaveSelect!=nullptr)
                        cout << " SaveSelect:" << wSaveSelect->Debug();
                    cout << endl;
#endif
#endif

                    const tString wUndoClass = wUndoSpreadSheet->ClassName();
                    const tBool wDeleteSheetUndo = (wUndoClass == "tUndoDeleteSheet");

                    // Rebase if false clear UndoRedo (delete-sheet keeps SaveSelectErase intact).
                    if (!wDeleteSheetUndo && !wUndoSpreadSheet->Rebase()) {
                        wWorkBook->ClearUndoRedo();
                        return(false);
                    }
#ifdef debuginterface
#ifdef _DEBUGSK
                    wSaveSelect=wUndoSpreadSheet->SaveSelect();
                    cout << "After Rebase Ref:" << wUndoSpreadSheet->RefRebase();
                    if (wSaveSelect!=nullptr)
                        cout << " SaveSelect:" << wSaveSelect->Debug();
#endif
                    cout << endl;
#endif
                    const tBool wMovePasteUndo =
                        (wUndoClass == "tUndoMove" || wUndoClass == "tUndoPaste" || wUndoClass == "tUndoCut");

                    if (wDeleteSheetUndo) {
                        wUndoSpreadSheet->IsJson(true);
                        wMessage.Uri(ActiveWorkBook()->Uri());
                        wMessage.WriteJson(wUndoSpreadSheet);
                        wUndoSpreadSheet->IsJson(false);
                        if (tUndoDeleteSheet* wDeleteSheet = dynamic_cast<tUndoDeleteSheet*>(wUndoSpreadSheet)) {
                            wOk = wDeleteSheet->Undo();
                        }
                        if (wOk) {
                            wOk = m_UndoRedoContainer->PopUndoToRedoWithoutExecute();
                        }
                        if (wOk) {
                            RegisterCollaborationStructuralOp(wWorkBook->UndoRebaseLog(), wUndoSpreadSheet, "Undo");
                        }
                    } else {
                        wUndoSpreadSheet->IsJson(true);
                        wMessage.Uri(ActiveWorkBook()->Uri());
                        wMessage.WriteJson(wUndoSpreadSheet);
                        wUndoSpreadSheet->IsJson(false);
                        RegisterCollaborationStructuralOp(wWorkBook->UndoRebaseLog(), wUndoSpreadSheet, "Undo");
                        if (wMovePasteUndo) {
                            wOk = m_UndoRedoContainer->PopUndoToRedoWithoutExecute();
                        } else {
                            wOk = DispatchSpreadSheetUndo(m_UndoRedoContainer);
                        }
                    }
                    if (wOk) {
#ifdef debuginterface
                        tString wCde=wMessage.Json();
                        cout << "Undo Cde " << wCde << endl;
#endif
#ifdef TestMultiUser
                        // Dispatch on server (includes originator via GetMessage relay).
                        // DeleteSheet: local Undo() already ran; peers apply via GetMessage only.
                        if (!m_Dispatcher->PostMessage(this, wMessage.Json())) {
                            if (wMovePasteUndo) {
                                GetMessage(wMessage.Json());
                            }
                        }
#else
                        PostMessage(wMessage.Json());
                        // Move/paste/cut undo: no local Undo(); originator applies via GetMessage.
                        // DeleteSheet: local Undo() already ran — do not GetMessage again (sheet exists).
                        if (wMovePasteUndo) {
                            GetMessage(wMessage.Json());
                        }
#endif
                    } else {
                        tStringStream wStream;
                        tUndoSpreadSheet* wUndoSpreadSheet=dynamic_cast<tUndoSpreadSheet*>(wUndo);
                        wStream << "Local Undo"  << ":" << wUndoSpreadSheet->ClassName() << " -> return(false) ";
                        cerr << endl << wStream.str();
                    }
                }
            } else {
#ifdef debuginterface
                cerr << "Undo : No undo" << endl;
#endif
            }
           
        } else {
            wOk=m_Application->Undo();
        }
    #ifdef checksp
        Check();
    #endif
        return(wOk);
    }

    tBool tInterfaceWeb::Redo() {
        tBool wOk=false;
        if (Client()) {
            if (!SetActiveWorkBook()) {
                return(false);
            }
            // Launch Redo local
            // Delete on rebase
            
            tUndo* wUndo=m_UndoRedoContainer->LastRedo(); // PopRedo
            if (wUndo!=nullptr) {
                // Dispath
                tUndoSpreadSheet* wUndoSpreadSheet=dynamic_cast<tUndoSpreadSheet*>(wUndo);
                if (wUndoSpreadSheet!=nullptr) {
                    wUndoSpreadSheet->WorkBookTarget(m_WorkBookUri);
                    if (tUndoSpreadSheetCallBack* wCallback = dynamic_cast<tUndoSpreadSheetCallBack*>(wUndoSpreadSheet)) {
                        wCallback->RebindActiveSheet();
                    }
#ifdef TestMultiUser
                    tWorkBook* wWorkBook = WorkBook(m_WorkBookUri);
#else
                    tWorkBook* wWorkBook = ActiveWorkBook();
#endif
                    
                    wUndoSpreadSheet->IsRedo(true);
                    wUndoSpreadSheet->IsJson(false);
                    // For Redo Clear m_Save
                    wUndoSpreadSheet->DeleteBeforeDo();
                  
                    // Rebase if false clear UndoRedo
                    if (!wUndoSpreadSheet->Rebase()) {
                        wWorkBook->ClearUndoRedo();
                        return(false);
                    }

                    wOk=DispatchSpreadSheetRedo(m_UndoRedoContainer);
                    if (wOk) {
                        // Generate new unique operation ID for Redo to ensure perfect correspondence with future Undo
                        // This allows Undo of Redo to find the exact Redo operation
                        std::uint64_t wOperationId = static_cast<std::uint64_t>(tUid::New());
                        wUndoSpreadSheet->OperationId(wOperationId);
                        // Keep the local rebase log in sync. Do() and Undo() both register
                        // their structural op here; Redo() must do the same. Otherwise the
                        // UndoRebaseLog desyncs from the actual grid: a later structural
                        // redo (Insert/Delete Col/Row by rect) builds its rebase plan
                        // against a stale log, Rebase() returns false, and the Redo path
                        // calls ClearUndoRedo() which wipes the whole undo/redo stack.
#ifdef TestMultiUser
                        if (tWorkBook* wWorkBookForLog = WorkBook(m_WorkBookUri)) {
                            RegisterCollaborationStructuralOp(wWorkBookForLog->UndoRebaseLog(), wUndoSpreadSheet, "Redo");
                        }
#else
                        if (tWorkBook* wWorkBookForLog = ActiveWorkBook()) {
                            RegisterCollaborationStructuralOp(wWorkBookForLog->UndoRebaseLog(), wUndoSpreadSheet, "Redo");
                        }
#endif
#ifdef debuginterface
                        tString wCde=wUndoSpreadSheet->WriteJson();
                        cout << "Redo Cde " << wCde << endl;
#endif
#ifdef TestMultiUser
                        tMessage wMessage(m_User,m_WorkBookUri,"Redo");
#else
                        tMessage wMessage(m_Email,m_Name,m_FirstName, m_WorkBookUri,"Redo");
#endif
                        wMessage.Uri(ActiveWorkBook()->Uri());
                        wMessage.WriteJson(wUndoSpreadSheet);
                                     
#ifdef TestMultiUser
                        // Dispach
                        m_Dispatcher->PostMessage(this,wMessage.Json());
#else
                        PostMessage(wMessage.Json());
#endif
                    } else {
                        tStringStream wStream;
                        wStream << "Local Redo"  << ":" << wUndoSpreadSheet->ClassName() << " -> return(false) ";
                        cerr << endl << wStream.str();
                        //throw(wStream.str());
                    }
                }
            } else {
#ifdef debuginterface
                cerr << "Redo : No redo" << endl;
#endif
            }
        } else {
            wOk=DispatchSpreadSheetRedo(m_UndoRedoContainer);
        }
    #ifdef checksp
        Check();
    #endif
        return(wOk);
    }

    tUndo* tInterfaceWeb::LastUndo() {
        tUndo* wUndo;
        wUndo=m_UndoRedoContainer->LastUndo();
        return(wUndo);
    }
        
    tUndo* tInterfaceWeb::LastRedo() {
        tUndo* wUndo;
        wUndo=m_UndoRedoContainer->LastRedo();
        
        return(wUndo);
    }
  
    tBool tInterfaceWeb::PostMessage(tString  sMessage) {
        return(false);
    }

    tBool tInterfaceWeb::GetMessage(tString  sMessage) {
#ifdef debuginterface
        cout << "Message :" <<  sMessage << endl;
#endif
        // Active WorkBook ====================================================
        tSpreadSheetContainer* wSpreadSheetContainer=tSpreadSheetContainer::Instance();
        tString wUriSave;
        if (tWorkBook* wActiveSave = wSpreadSheetContainer->ActiveWorkBook()) {
            wUriSave = wActiveSave->Uri();
        }
        tBool wOk=true;
        tBool wRecalcAfterUndo = false;
        tString wActiveSheetSave;
        if (Client()) {
            wOk = SetActiveWorkBook();
        }
#ifdef TestMultiUser
        if (Client() && wOk) {
            if (tWorkBook* wViewBook = WorkBook(m_WorkBookUri)) {
                if (tSheet* wViewSheet = wViewBook->ActiveSheet()) {
                    wActiveSheetSave = wViewSheet->Name();
                }
            }
        }
#else
        if (Client() && wOk) {
            if (tWorkBook* wViewBook = wSpreadSheetContainer->ActiveWorkBook()) {
                if (tSheet* wViewSheet = wViewBook->ActiveSheet()) {
                    wActiveSheetSave = wViewSheet->Name();
                }
            }
        }
#endif
        
        if (wOk) {
            tMessage wMessage;
            tUndoSpreadSheet* wUndo=wMessage.ReadJson(sMessage);
            if (wUndo == nullptr) {
                return(false);
            }
            // Flag to Json (dont't Rebase)
            wUndo->IsJson(true);
#ifdef TestMultiUser
            // Each peer applies JSON on its own workbook copy (client + server mirrors).
            if (Client() || Server()) {
                wUndo->WorkBookTarget(m_WorkBookUri);
            } else if (!wMessage.Uri().empty()) {
                wUndo->WorkBookTarget(wMessage.Uri());
            } else {
                wUndo->WorkBookTarget(m_WorkBookUri);
            }
#else
            if (!wMessage.Uri().empty()) {
                wUndo->WorkBookTarget(wMessage.Uri());
            }
#endif
            if (tUndoSpreadSheetCallBack* wCallback = dynamic_cast<tUndoSpreadSheetCallBack*>(wUndo)) {
                wCallback->RebindActiveSheet();
            }
#ifdef debuginterface
            cout << "Get Message on ";
#ifdef TestMultiUser
            cout << m_User;
            cout << " Uri:" << m_WorkBookUri << "  Message : User ->" << wMessage.User() << " Uri ->"  <<  wMessage.Uri() <<  endl;
#else
            cout << m_Email  << ","  << m_Name << "," << m_FirstName;
            cout << " Uri:" << m_WorkBookUri << "  Message :  " << wMessage.Email() << " Uri ->"  <<  wMessage.Uri() <<  endl;
#endif
           
            if (wUndo!=nullptr) {
                cout << "Operation:" << wUndo->OperationName() << endl;
            } else {
                cout << "Operation:nullptr"  << endl;
            }
            
#endif
            // Register structural operations in local log to get local SequenceId
            // Note: Coordinates received in GetMessage() are already rebased (rebase was done before PostMessage)
            // This is needed because undo from other users don't have valid SequenceId in our environment
            tString wDoRedo = wMessage.DoRedo();
            
            // Get the active workbook's rebase log (each workbook has its own log)
#ifdef TestMultiUser
            tWorkBook* wWorkBook = WorkBook(m_WorkBookUri);
#else
            tWorkBook* wWorkBook = ActiveWorkBook();
#endif
            if (wWorkBook != nullptr) {
                tUndoRebaseLog& wRebaseLog = wWorkBook->UndoRebaseLog();
                if (Client()) {
                    RegisterCollaborationStructuralOp(wRebaseLog, wUndo, wDoRedo);
                }
            }
#ifdef  debuginterface
            tUndoSpreadSheet* wUndoSpreadSheet=dynamic_cast<tUndoSpreadSheet*>(wUndo);
            tStringStream wStream;
            wStream << "Get Message Before " << wDoRedo;
            wUndoSpreadSheet->DebugFlags(wStream.str());
#endif
            tBool wApplyOk=false;
            tUndoSpreadSheet* wUndoSpreadSheetExec = dynamic_cast<tUndoSpreadSheet*>(wUndo);
            // Collab wire formulas are US (WriteJson / FormulaWire); apply under Locale us
            // even when this peer's UI locale is FR (decimal ',').
            tLocalePush wWireLocale("us");
            // Apply message
            if (wMessage.DoRedo()=="Do") {
                if (wUndoSpreadSheetExec != nullptr) {
                    wUndoSpreadSheetExec->IsDo(true);
                    wApplyOk=wUndoSpreadSheetExec->Do();
                }
            }
            if (wMessage.DoRedo()=="Redo") {
                if (wUndoSpreadSheetExec != nullptr) {
                    wUndoSpreadSheetExec->IsDo(true);
                    wApplyOk=wUndoSpreadSheetExec->Do();
                }
            }
            if (wMessage.DoRedo()=="Undo") {
                if (wUndoSpreadSheetExec != nullptr) {
                    wUndoSpreadSheetExec->IsUndo(true);
                }
#ifdef debuginterface
                tUndoSpreadSheet* wUndoSpreadSheetDebug=dynamic_cast<tUndoSpreadSheet*>(wUndo);
                if (wUndoSpreadSheetDebug != nullptr) {
                    cout << "GetMessage: Executing Undo for " << wUndoSpreadSheetDebug->ClassName();
                    cout << " SequenceId=" << wUndoSpreadSheetDebug->SequenceId();
                    if (wUndoSpreadSheetDebug->ClassName() == "tUndoDeleteRow") {
                        tUndoDeleteRow* wUndoDeleteRowDebug = dynamic_cast<tUndoDeleteRow*>(wUndoSpreadSheetDebug);
                        if (wUndoDeleteRowDebug != nullptr) {
                            if (wUndoDeleteRowDebug->Position() != static_cast<tIndex>(-1)) {
                                cout << " Pos=" << wUndoDeleteRowDebug->Position() << " Size=" << wUndoDeleteRowDebug->Size();
                            } else {
                                tRect wRectDebug = wUndoDeleteRowDebug->RectRebase();
                                cout << " Rect=[" << wRectDebug.Top() << "," << wRectDebug.Left() << ":" << wRectDebug.Bottom() << "," << wRectDebug.Right() << "]";
                            }
                        }
                    }
                    cout << endl;
                }
#endif
                wApplyOk=wUndoSpreadSheetExec != nullptr ? wUndoSpreadSheetExec->Undo() : wUndo->Undo();
#ifdef debuginterface
                if (!wApplyOk) {
                    cout << "GetMessage: Undo execution FAILED" << endl;
                } else {
                    cout << "GetMessage: Undo execution SUCCESS" << endl;
                }
#endif
                // Delete row/col/sheet undo restores cells from svse and runs save-based CalculateDo.
                if (wUndoSpreadSheetExec != nullptr) {
                    const tString wUndoClass = wUndoSpreadSheetExec->ClassName();
                    if (wUndoClass != "tUndoDeleteRow" && wUndoClass != "tUndoDeleteCol"
                        && wUndoClass != "tUndoDeleteSheet") {
                        wRecalcAfterUndo = wApplyOk;
                    }
                } else {
                    wRecalcAfterUndo = wApplyOk;
                }
            }
            if (!wApplyOk) {
                tStringStream wStream;
                tUndoSpreadSheet* wUndoSpreadSheet=dynamic_cast<tUndoSpreadSheet*>(wUndo);
                wStream << "Other " <<  wDoRedo << ":" << wUndoSpreadSheet->ClassName() << "  GetMessage -> return(false) ";
                cerr << endl << wStream.str();
                //throw(wStream.str());
            }
            
            delete(wUndo);
        }

        if (Client() && !wActiveSheetSave.empty()) {
#ifdef TestMultiUser
            tWorkBook* wRestoreBook = WorkBook(m_WorkBookUri);
#else
            tWorkBook* wRestoreBook = wSpreadSheetContainer->ActiveWorkBook();
#endif
            if (wRestoreBook != nullptr && wRestoreBook->Sheet(wActiveSheetSave) != nullptr) {
                wRestoreBook->ActiveSheet(wActiveSheetSave);
            }
        }
        
       
#ifdef TestMultiUser
        // If Server DisPatch
        
        if (Server()) {
            m_Dispatcher->DispatchMessage(this, sMessage);
        }
        wSpreadSheetContainer->ActiveWorkBook(wUriSave);
#endif
        if (wRecalcAfterUndo) {
            if (tWorkBook* wRecalcBook = wSpreadSheetContainer->ActiveWorkBook()) {
                wRecalcBook->RecalculateAll();
            }
        }
        return(true);
    }


    void tInterfaceWeb::JsonUser(tString sJson) {
        rapidjson::Document wDocument;
        rapidjson::ParseResult parseResult = wDocument.Parse(sJson.c_str());
        if (parseResult.IsError()) {
            cerr << "Failed to parse Json User" << endl;
            ShowParseErrorJson(&wDocument,sJson);
            throw tExceptionInternalError("Failed to parse Json User");
        }
        // Accept SkJsonKey short names (em/nm/fn) and long names from JS (email/name/firstname).
        auto readUserString = [&wDocument](const char* sShortKey, const char* sLongKey) -> tString {
            if (wDocument.HasMember(sShortKey) && wDocument[sShortKey].IsString()) {
                return wDocument[sShortKey].GetString();
            }
            if (wDocument.HasMember(sLongKey) && wDocument[sLongKey].IsString()) {
                return wDocument[sLongKey].GetString();
            }
            return tString();
        };
        m_Email = readUserString(kJsonKeyEmail, "email");
        m_Name = readUserString(kJsonKeyName, "name");
        m_FirstName = readUserString(kJsonKeyFirstName, "firstname");
    }

    tString tInterfaceWeb::JsonUser() {
        StringBuffer wBuffer;
        Writer<StringBuffer> wWriter(wBuffer);
        wWriter.StartObject();
        wWriter.Key(kJsonKeyEmail);
        wWriter.String(m_Email.c_str());
        wWriter.Key(kJsonKeyName);
        wWriter.String(m_Name.c_str());
        wWriter.Key(kJsonKeyFirstName);
        wWriter.String(m_FirstName.c_str());
        wWriter.EndObject();
        return(wBuffer.GetString());
    }

#if defined(checkfo) && defined(TestMultiUser)
    tVectorUndo tInterfaceWeb::GetUndoVector() {
        return(m_UndoRedoContainer->GetUndoVector());
    }

    tVectorUndo tInterfaceWeb::GetRedoVector() {
        return(m_UndoRedoContainer->GetRedoVector());
    }
    void  tInterfaceWeb::CheckFormat() {
         tCheckFormat::Instance()->CheckFormat();
    }
#endif

   
} // End of namespace

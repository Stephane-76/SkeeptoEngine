//=============================================================================
// SkRoot Undo Redo 
//=============================================================================
#include "../include/SkUndoRedo.hpp"

namespace SkRoot {

    //=========================================================================
    tUndoExtra::tUndoExtra() : tVirtualClass() {}

    //=========================================================================
    tUndo::tUndo() : tVirtualClass(), m_OperationName(), m_Extra(nullptr) {}
    tUndo::~tUndo() {
        if (m_Extra != nullptr) delete(m_Extra);
    };

    tBool tUndo::Do() {
        return(true);
    }

    tBool tUndo::Undo() {
        return(true);
    }

    void tUndo::OperationName(tString sValue) {	m_OperationName = sValue; }
    tString tUndo::OperationName() { return(m_OperationName()); }

    void tUndo::Extra(tUndoExtra* sExtra) { m_Extra = sExtra; }
    tUndoExtra* tUndo::Extra() { return(m_Extra); }

    tString tUndo::Debug() {
        return("");
    }

    //=========================================================================
    tUndoRedoContainer::tUndoRedoContainer() : tClass(),m_MaxUndoRedo(SkMaxUndoRedo),m_OnDeleteUndo(nullptr) {
    }

    tUndoRedoContainer::~tUndoRedoContainer() {
        Clear();
    }

    void tUndoRedoContainer::Clear() {
        ClearUndo();
        ClearRedo();
    }

    void tUndoRedoContainer::ClearUndo() {
        // Oldest first: newer tUndoDeleteSheet may own sheet storage older undo entries need.
        while (!m_DequeUndo.empty()) {
            tUndo* wUndo = m_DequeUndo.front();
            m_DequeUndo.pop_front();
            delete(wUndo);
        }
    }

    void tUndoRedoContainer::ClearRedo() {
        tUndo* wUndo;
        while (!m_StackRedo.empty()) {
            wUndo = m_StackRedo.top();
            m_StackRedo.pop();
            delete(wUndo);
        }
    }

    tBool tUndoRedoContainer::Do(tUndo* sUndo) {
        tBool wOk = sUndo->Do();
        if (wOk) {
            return(CommitDo(sUndo));
        }
        delete(sUndo);
        return(wOk);
    }

    tBool tUndoRedoContainer::CommitDo(tUndo* sUndo) {
        ClearRedo();
        m_DequeUndo.push_back(sUndo);
        if (m_DequeUndo.size() > m_MaxUndoRedo) {
            tUndo* wUndo = m_DequeUndo.front();
            if (m_OnDeleteUndo!=nullptr)
                m_OnDeleteUndo(wUndo);
            delete(wUndo);
            m_DequeUndo.pop_front();
        }
        return(true);
    }

    tBool tUndoRedoContainer::Undo() {
        tBool wReturn=false;
        tUndo* wUndo;
        if (!m_DequeUndo.empty()) {
            wUndo = m_DequeUndo.back();
            m_DequeUndo.pop_back();
            // Push  redo
            m_StackRedo.push(wUndo);
            wReturn = wUndo->Undo();
        }
        return(wReturn);
    }

    tBool tUndoRedoContainer::PopUndoToRedoWithoutExecute() {
        if (m_DequeUndo.empty()) {
            return(false);
        }
        tUndo* wUndo = m_DequeUndo.back();
        m_DequeUndo.pop_back();
        m_StackRedo.push(wUndo);
        return(true);
    }

    tUndo* tUndoRedoContainer::PopRedoToUndoDeque() {
        if (m_StackRedo.empty()) {
            return(nullptr);
        }
        tUndo* wUndo = m_StackRedo.top();
        m_StackRedo.pop();
        m_DequeUndo.push_back(wUndo);
        return(wUndo);
    }

    tUndo* tUndoRedoContainer::DetachLastUndo() {
        if (m_DequeUndo.empty()) {
            return(nullptr);
        }
        tUndo* wUndo = m_DequeUndo.back();
        m_DequeUndo.pop_back();
        return(wUndo);
    }

    tBool tUndoRedoContainer::Redo() {
        tBool wReturn=false;
        tUndo* wUndo;
        if (!m_StackRedo.empty()) {
            // Get redo
            wUndo = m_StackRedo.top();
            // Pop stack redo
            m_StackRedo.pop();
            // Push  undo on deque
            m_DequeUndo.push_back(wUndo);
            wReturn = wUndo->Do();
        }
        return(wReturn);
    }

    void tUndoRedoContainer::MaxUndo(tSize sMaxUndoRedo) { m_MaxUndoRedo=sMaxUndoRedo; }
    tSize tUndoRedoContainer::MaxUndo() { return(m_MaxUndoRedo); }

    void tUndoRedoContainer::OnDeleteUndo(tOnDeleteUndo* sOnDeleteUndo) {m_OnDeleteUndo=sOnDeleteUndo; }

	tUndo* tUndoRedoContainer::LastUndo() {
		if (m_DequeUndo.size()>0) return(m_DequeUndo.back());
		return(nullptr);
	}

    tUndo* tUndoRedoContainer::LastRedo() {
        if (m_StackRedo.size()>0) return(m_StackRedo.top());
        return(nullptr);
    }

    tVectorUndo tUndoRedoContainer::GetUndoVector() {
        tVectorUndo wVectorUndo;
        wVectorUndo=tVectorUndo(m_DequeUndo.begin(),m_DequeUndo.end());
        return(wVectorUndo);
    }

    tVectorUndo tUndoRedoContainer::GetRedoVector() {
        tVectorUndo wVectorRedo;
        tStackUndo wStackRedo=m_StackRedo;
        while(!wStackRedo.empty()) {
            wVectorRedo.push_back(wStackRedo.top());
            wStackRedo.pop();
        }
        return(wVectorRedo);
    }

#ifdef _DEBUGSK
    tString tUndoRedoContainer::DebugUndo() {
        tStringStream wStream;
        wStream << "Debug Undo -----------------" << endl;
        tVectorUndo wUndoVector=GetUndoVector();
        for(auto wUndo : wUndoVector) {
            wStream << wUndo->Debug();
        }
        return(wStream.str());
    };
    tString tUndoRedoContainer::DebugRedo() {
        tStringStream wStream;
        wStream << "Debug Redo -----------------" << endl;
        tVectorUndo wRedoVector=GetRedoVector();
        for(auto wUndo : wRedoVector) {
            wStream << wUndo->Debug();
        }
        return(wStream.str());
    };
#endif
}


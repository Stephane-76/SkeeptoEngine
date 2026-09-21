//=============================================================================
// SkRoot tClass
// Class ancestor of all classes
//=============================================================================

#include "../include/SkClass.hpp"
#include "../include/SkVariant.hpp"
#include "../include/SkApplication.hpp"

namespace SkRoot {

	// System For Debug leak ======================================================
#ifdef _DEBUGLeak
	int StaticTotalAlloc = 0;
	int StaticDiff = 0;
	tBool StaticReportLeakAtExit = true;
	// warning This Class is not desalocate at the end
	tClassUnorderedContainer<tClass>* StaticClassMemoryDebug = nullptr;
#endif 

#ifdef _DEBUGLeak
	const tInt wBreak_Number =2930;
    const tInt MaxView=25;
#endif


	template <class T>
	//! Contains unique class ordered by pointer. (preserves the uniqueness of T class). Used for leak memory among others 
	void tClassVectorContainer<T>::DebugApplicationMemory() {
#ifdef _DEBUGLeak
        tSize wSize=0;
		cout << "Nb of class allocated " << StaticTotalAlloc << endl;
		cout << "Nb of class not desallocated " << StaticDiff - 4 << endl;
		if (m_Vector.size() != 4) {
			for (auto wClass : m_Vector) {
				cout << wClass << ":" << typeid(*wClass).name();
				cout << ":" << wClass->GetNbAlloc();
				cout << endl;
                wSize++;
                if (wSize>MaxView) {
                    cout << "....." << endl;
                    break;
                }
                
			}
		}
#endif
	}

	template <class T>
	//! Contains unique class unordered_set  by pointer. (preserves the uniqueness of T class). Used for leak memory among others 
	void tClassUnorderedContainer<T>::DebugApplicationMemory() {
#ifdef _DEBUGLeak
        tSize wSize=0;
        
		// SkApplication exist
		tApplication::Instance()->Clear();
		cout << "Nb of class allocated " << StaticTotalAlloc << endl;
		// 3 for tApplication, tUndoRedoContainer and tClipBoard
		cout << "Nb of class not desallocated " << StaticDiff - 4 << endl;
		if (m_Unordered_set.size() != 4) {
			for (auto wClass : m_Unordered_set) {
				cout << wClass << ":" << typeid(*wClass).name();
#ifdef _DEBUGLeak
				cout << ":" << wClass->GetNbAlloc();
#endif
				cout << endl;
                wSize++;
                if (wSize>MaxView) {
                    cout << "....." << endl;
                    break;
                }
			}
		}
#endif
	} 

	// tClass ================================================================
	tClass::tClass() {
#ifdef _DEBUGLeak
        m_IndiceAlloc=0;
		// Debug leak ==========================
		if (StaticTotalAlloc == wBreak_Number) {
            //int a = 1; // warning unused variable
		}
		m_IndiceAlloc = StaticTotalAlloc;
		StaticTotalAlloc++;
		StaticDiff++;
		if (StaticClassMemoryDebug == nullptr) StaticClassMemoryDebug = new tClassUnorderedContainer<tClass>();
		if (!StaticClassMemoryDebug->InsertClass(this)) {
            cerr << "Insert Class already exist ! " << endl;
            //throw(new tExceptionMemory("Insert Class already exist !"));
        }
#endif
	}
	tClass::tClass(const tClass& sClass) {
#ifdef _DEBUGLeak	
		// Debug Leak
		if (StaticTotalAlloc == wBreak_Number) {
			//int a = 1; //warning unused variable
		}
		m_IndiceAlloc = StaticTotalAlloc;
		StaticTotalAlloc++;
		StaticDiff++;
		if (StaticClassMemoryDebug == nullptr) StaticClassMemoryDebug = new tClassUnorderedContainer<tClass>();
		if (!StaticClassMemoryDebug->InsertClass(this)) {
        	cerr << "Insert Class by Copy already exist ! " << endl;      
            //throw(new tExceptionMemory("Insert tClass already exist !"));
        }
#endif
	}

	tClass::~tClass() {
#ifdef _DEBUGLeak		
		StaticDiff--;
		if (!StaticClassMemoryDebug->DeleteClass(this)) {
		} 
#endif
	}


#ifdef _DEBUGLeak
	int tClass::GetNbAlloc() { return(m_IndiceAlloc); }
#endif

	// tVirtualClass =========================================================
	tVirtualClass::tVirtualClass() : tClass() {}
	tVirtualClass::tVirtualClass(const tVirtualClass& sVirtualClass) : tClass(sVirtualClass) {}

	tVirtualClass::~tVirtualClass() {}

	tBool tVirtualClass::IsCopy() { return(true);  };

	tVirtualClass* tVirtualClass::Clone() {
		return(new tVirtualClass(*this));
	}

	tString tVirtualClass::ClassName() const { return("VirtualClass"); };

    tBool tVirtualClass::IsCalculationPropagation() const { return(true); };

    void tVirtualClass::Value(tVariant* sValue) {}

	tVariant* tVirtualClass::Value() { return(nullptr); }

    void tVirtualClass::Json(Writer<StringBuffer>* sWriter) {}

    void tVirtualClass::Json(const rapidjson::Value& sValue) {}

    tBool tVirtualClass::IsJsonJavaScript() { return(false); }

    tBool tVirtualClass::IsReactComponent() { return(false); }

    void tVirtualClass::JsonJavaScript(Writer<StringBuffer>* sWriter) {}

    // Spreadsheet
	tVariant tVirtualClass::Operator_plus(tBool sLeft, const tVariant& sVariant) { return(0); }
	tVariant tVirtualClass::Operator_minus(tBool sLeft, const tVariant& sVariant) { return(0); }
	tVariant tVirtualClass::Operator_multiply(tBool sLeft, const tVariant& sVariant) { return(0); }
	tVariant tVirtualClass::Operator_divide(tBool sLeft, const tVariant& sVariant) { return(0); }

	tBool tVirtualClass::operator==(const tVirtualClass& sVirtualClass) const { return(ClassName() == sVirtualClass.ClassName()); }
#ifdef _DEBUGSK
    tString tVirtualClass::Debug() { return(""); };
#endif
#ifdef _DEBUGLeak
	void DebugMemory() {
		if (StaticClassMemoryDebug!=nullptr) StaticClassMemoryDebug->DebugApplicationMemory();
	}

	void ReportLeakAtExit(tBool sReport) {
		StaticReportLeakAtExit = sReport;
	}

	tBool ReportLeakAtExit() {
		return StaticReportLeakAtExit;
	}
#else
	void ReportLeakAtExit(tBool) {}
	tBool ReportLeakAtExit() { return false; }
#endif

}; // Fin du namespace ========================================================

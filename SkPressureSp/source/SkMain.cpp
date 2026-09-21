//=============================================================================
// SkPressure SpreadSheet Test 
//=============================================================================
#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include "../include/SkLemonSpreadSheet.hpp"
#include "../include/SkPressure1.hpp"
#include "../include/SkPressure2.hpp"
#include "../include/SkPressure3.hpp"

#include "../include/SkTest.hpp"

#include <SkApplication.hpp>

//using namespace xlnt;
using namespace SkSpreadSheet;
//#define DebugMemoryLeak

	tInt wDynamicNbRow = wNbRow;
	tInt wDynamicNbCol = wNbCol;
	tInt wDynamicModulo = wModulo;

	tBool wJsonTest = false;
	tBool wDeleteRow = false;

	extern "C" {
		tInt Launch(tInt NbRow, tInt NbCol, tBool wTest1, tBool wTest2, tBool wTest3);
	}

	tInt Launch(tInt NbRow, tInt NbCol, tBool wTest1, tBool wTest2, tBool wTest3) {
#ifndef NO_THROW	
		try {
#endif	
			MemoryUses();

#ifdef _DEBUGMEMORY
			const tInt Duration = 8000;
#else
#ifdef _M_X64
			const tInt Duration = 0;
#endif
#endif
			cout << "Begin of Program ===========================================" << endl;
#ifdef _M_X64
			Sleep(Duration);
#endif

			tApi* wApi = new tApi();
			wApi->IsUndoActif(true);
			wApi->WorkBook("wwww.Sker.fr/SkPressure");
			wApi->ActiveSheet("Sheet1");
			//Test(wApi);

			//SkXlsx(wApi);

			if (wTest1) {
				cout << "Pressure 1 =================================================" << endl;
				Pressure1(wApi);
				cout << "End of Pressure 1 ==========================================" << endl;
			}
#ifdef _M_X64
			Sleep(Duration);
#endif
			if (wTest2) {
				cout << "Pressure 2 =================================================" << endl;
				Pressure2(wApi);
				cout << "End of Pressure 2 ==========================================" << endl;
			}
			if (wTest3) {
				cout << "Pressure 3 =================================================" << endl;
				Pressure3(wApi);
				cout << "End of Pressure 3 ==========================================" << endl;
			}
#ifdef DebugMemoryLeak
			delete(wApi);
			tApplication::Instance()->Clear();
			cout << "End of Program =============================================" << endl;
#else
			cout << "End of Program =============================================" << endl;

#ifdef _DEBUG
			PrintSize();
#endif
#ifdef _M_X64
			Sleep(Duration);
#endif
			cout << "Delete all " << wDynamicNbRow * wDynamicNbCol << " on " << wDynamicNbRow << " rows." << endl;
			tApplication::Instance()->TimerStart();
			delete(wApi);
			cout << "Elapsed Time " << ElapsedSec() << " s" << endl;


			tApplication::Instance()->Clear();
#endif

#ifndef NO_THROW	
		}
		catch (const tException& e) {
			cerr << e.what();
		}
#endif

		MemoryUses();
		return(0);
	}


	int main(int argc, char** argv) {
#ifndef __EMSCRIPTEN_WEB__
		std::filesystem::remove_all("./Spreadsheet");


		std::cout << "SkPressure 2026" << endl;
		std::cout << "---------------" << endl;

            /*
			// Check command line and extract arguments.
			if (argc < 2) {
				std::cout << " Enter SkPressure /r:NbRow /c:NbCol /t1 /t2 /t3" << endl;
				std::cout << "	/r: number of row" << endl;
				std::cout << "	/c: number of col" << endl;
				std::cout << "	/t1  test 1" << endl;
				std::cout << "	/t2  test 2" << endl;
				std::cout << "	/t3  test 3" << endl;
				std::cout << "	/ta  all test" << endl;
				std::cout << "	/j  Json test" << endl;
				std::cout << "	/d  delete row" << endl;

				std::cout << endl;
				return(1);
			}
            */
			tBool wTest1 = false;
			tBool wTest2 = false;
			tBool wTest3 = true;
            
			for (int Ind = 1; Ind < argc; Ind++) {
				tString wArg = argv[Ind];
				if (wArg[0] == '/') {
					tChar   wArgChar = wArg[1];
					switch (wArgChar) {
					case 'R':
					case 'r': {
						wDynamicNbRow = stoi(wArg.substr(3, wArg.length() - 3).c_str());
						if (wDynamicNbRow < 10) wDynamicNbRow = 10;
						break;
					}
					case 'C':
					case 'c': {
						wDynamicNbCol = stoi(wArg.substr(3, wArg.length() - 3).c_str());
						if (wDynamicNbCol < 3) wDynamicNbCol = 3;
						break;
					}

					case 't':
					case 'T': {
						tChar wTestValue = wArg[2];
						switch (wTestValue) {
						case '1': wTest1 = true; break;
						case '2': wTest2 = true; break;
						case '3': wTest3 = true; break;
						default: {
							wTest1 = true;
							wTest2 = true;
							wTest3 = true;
							break;
						}
						}
						break;
					}
					case 'j':
					case 'J': {
						wJsonTest = true;
						break;
					}
					case 'D':
					case 'd': {
						wDeleteRow = true;
						break;
					}
					}
				}
			}
			wDynamicModulo = wDynamicNbRow / wModulo;

			return(Launch(wDynamicNbRow, wDynamicNbCol, wTest1, wTest2, wTest3));
		}
#else
		return(0);
}
#endif

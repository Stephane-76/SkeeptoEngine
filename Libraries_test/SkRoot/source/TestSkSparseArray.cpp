//==============================================================================
// TesttSparseArray
// Test library tSparseArray
//==============================================================================

#include "../include/TestSkSparseArray.hpp"

const int StaticNbElement = 10;

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION(TestSkSparseArray);

// We can send it to the API of a feature 
TestSkSparseArray::TestSkSparseArray() :CPPUNIT_NS::TestFixture(), m_Application(nullptr) {
	m_Application = nullptr;
};

class tTestString {
private:
    tString m_Value;
public:
    tTestString() : m_Value("") {}
    tTestString(tString sValue) : m_Value(sValue) {}
    
    void Clear() {}
    tBool operator==(tTestString& sString) { return(m_Value==sString.m_Value); };
    tString operator ()() { return(m_Value); }
};


void TestSkSparseArray::TestSparseArray() {
	tSparseArray<tTestString> wSparseArray;
	tTestString wTest = wSparseArray[256];
	CPPUNIT_ASSERT_MESSAGE("TestSparseArray", wTest() == "");

	wSparseArray[StaticNbElement + 10000]=tTestString("At the end...");

	for (int wInd = 0; wInd < StaticNbElement; wInd++) {
		tStringStream wStream;
		wStream << "Val=" << wInd;
		wSparseArray[wInd] = tTestString(wStream.str());
	}
	for (int wInd = 0; wInd < StaticNbElement; wInd++) {
		tStringStream wStream;
		wStream << "Val=" << wInd;
		tString wRes = (wSparseArray[wInd])();
		CPPUNIT_ASSERT_MESSAGE("TestSparseArray"+wRes, (wRes == wStream.str()));
	}
	wTest = wSparseArray[StaticNbElement + 10000];
	CPPUNIT_ASSERT_MESSAGE("TestSparseArray", wTest() == "At the end...");


};


class tTestInt {
private:
    tInt m_Value;
public:
    tTestInt(tInt sValue) : m_Value(sValue) {}
    
    void Clear() {}
    tInt operator ()() { return(m_Value); }
};

void TestSkSparseArray::TestSparseArrayPt() {
	tSparseArrayPt<tTestInt*> wSparseArray;

	tTestInt* wTest = wSparseArray[1256];
	CPPUNIT_ASSERT_MESSAGE("TestSparseArrayPt Not null value ", wTest == nullptr);

	for (int wInd = 0; wInd < StaticNbElement; wInd++) {
		wSparseArray[wInd]=new tTestInt(wInd);
	}

	wSparseArray[StaticNbElement+10000]=new tTestInt(1);

	for (int wInd = 0; wInd < StaticNbElement; wInd++) {
		tTestInt* wRes=wSparseArray[wInd];
		CPPUNIT_ASSERT_MESSAGE("TestSparseArrayPt", (*wRes)() == wInd);
	}
	wTest = wSparseArray [StaticNbElement + 10000];
	CPPUNIT_ASSERT_MESSAGE("TestSparseArrayPt", (*wTest)() == 1);

}

tString ConcatSparse(tSparseArray<tTestString>* sSparse, tInt sBegin, tInt sEnd) {
	tString wResult = "";
	for (tInt wIndex = sBegin; wIndex != sEnd + 1; wIndex++) {
		wResult += (*sSparse)[wIndex]()+";";
	}
	return(wResult);
}

void TestSkSparseArray::TestSparseArrayInsertErase() {
	tSparseArray<tTestString> wSparseArray;
	wSparseArray[0] = tTestString("Hello");
	wSparseArray[1] = tTestString("World");
	wSparseArray[2] = tTestString("Allez");
	wSparseArray[3] = tTestString("St�phane");
	CPPUNIT_ASSERT_MESSAGE("TestSparseArrayInsertErase Fill", ConcatSparse(&wSparseArray, 0, 3)=="Hello;World;Allez;St�phane;");

	wSparseArray.Insert(1, 2);
	CPPUNIT_ASSERT_MESSAGE("TestSparseArray Insert 1", ConcatSparse(&wSparseArray, 0, 5) == "Hello;;;World;Allez;St�phane;");

	wSparseArray.Erase(1, 2);
	CPPUNIT_ASSERT_MESSAGE("TestSparseArray Erase 1", ConcatSparse(&wSparseArray, 0, 3) == "Hello;World;Allez;St�phane;");

	wSparseArray.Erase(0, 3);
	CPPUNIT_ASSERT_MESSAGE("TestSparseArray Erase 2", ConcatSparse(&wSparseArray, 0, 0) == "St�phane;");

	wSparseArray.Erase(0, 1);
	CPPUNIT_ASSERT_MESSAGE("TestSparseArray Erase 2", ConcatSparse(&wSparseArray, 0, 0) == ";");
}


tString ConcatSparseInt(tSparseArrayPt<tTestInt*>* sSparse, tTestInt sBegin, tTestInt sEnd) {
	tString wResult = "";
	for (tInt wIndex = sBegin(); wIndex != sEnd() + 1; wIndex++) {
		if ((*sSparse)[wIndex] != nullptr) {
            tInt wValue=(*(*sSparse)[wIndex])();
			wResult += std::to_string(wValue) + ";";
		} else {
			wResult += ";";
		}
	}
	return(wResult);
}



void TestSkSparseArray::TestSparseArrayPtInsertErase() {
	tSparseArrayPt<tTestInt*> wSparseArray;
	wSparseArray[0] = new tTestInt(1);
	wSparseArray[1] = new tTestInt(2);
	wSparseArray[2] = new tTestInt(3);
	wSparseArray[3] = new tTestInt(4);

	CPPUNIT_ASSERT_MESSAGE("TestSparseArrayPtInsertErase Fill", ConcatSparseInt(&wSparseArray, 0, 3) == "1;2;3;4;");

	wSparseArray.Insert(1, 2);
	CPPUNIT_ASSERT_MESSAGE("TestSparseArray Insert 1", ConcatSparseInt(&wSparseArray, 0, 5) == "1;;;2;3;4;");

	wSparseArray.Erase(1, 2);
	CPPUNIT_ASSERT_MESSAGE("TestSparseArray Erase 1", ConcatSparseInt(&wSparseArray, 0, 3) == "1;2;3;4;");

	wSparseArray.Erase(0, 3);
	CPPUNIT_ASSERT_MESSAGE("TestSparseArray Erase 2", ConcatSparseInt(&wSparseArray, 0, 0) == "4;");

	wSparseArray.Erase(0, 1);
	CPPUNIT_ASSERT_MESSAGE("TestSparseArray Erase 2", ConcatSparseInt(&wSparseArray, 0, 0) == ";");
}


class tCallBackCollectInt : public tSparseArrayCallBack<tInt> {
public:
	tVectorInt m_Values;
	tInt m_StopAfter;
	tCallBackCollectInt() : tSparseArrayCallBack<tInt>(), m_StopAfter(-1) {}
	tBool CallBack(tInt sValue) override {
		m_Values.push_back(sValue);
		if (m_StopAfter >= 0 && static_cast<tInt>(m_Values.size()) >= m_StopAfter) {
			return false;
		}
		return true;
	}
};

void TestSkSparseArray::TestSparseArrayCallBackOccupied() {
	tSparseArray<tInt> wSparse;
	// Index 300 sits on track 1 (size 256). Track 0 is a nullptr hole until something is written there.
	wSparse[300] = 30;

	tCallBackCollectInt wSkipNullTrack;
	wSparse.CallBack(&wSkipNullTrack, 0, 10);
	CPPUNIT_ASSERT_MESSAGE("CallBack skips nullptr tracks in range", wSkipNullTrack.m_Values.empty());

	tCallBackCollectInt wOnlySecondTrack;
	wSparse.CallBack(&wOnlySecondTrack);
	CPPUNIT_ASSERT_MESSAGE("CallBack skips leading nullptr track",
		wOnlySecondTrack.m_Values.size() == 1 && wOnlySecondTrack.m_Values[0] == 30);

	wSparse[0] = 10;
	wSparse[2] = 20;

	tCallBackCollectInt wAll;
	wSparse.CallBack(&wAll);
	CPPUNIT_ASSERT_MESSAGE("CallBack visits only occupied slots", wAll.m_Values.size() == 3);
	CPPUNIT_ASSERT_MESSAGE("CallBack order track 0 then track 1",
		wAll.m_Values[0] == 10 && wAll.m_Values[1] == 20 && wAll.m_Values[2] == 30);

	tCallBackCollectInt wRange;
	wSparse.CallBack(&wRange, 0, 10);
	CPPUNIT_ASSERT_MESSAGE("Ranged CallBack skips holes and later tracks", wRange.m_Values.size() == 2);
	CPPUNIT_ASSERT_MESSAGE("Ranged CallBack values", wRange.m_Values[0] == 10 && wRange.m_Values[1] == 20);

	tCallBackCollectInt wTrack2;
	wSparse.CallBack(&wTrack2, 256, 400);
	CPPUNIT_ASSERT_MESSAGE("CallBack on second track", wTrack2.m_Values.size() == 1 && wTrack2.m_Values[0] == 30);

	tCallBackCollectInt wInverted;
	wSparse.CallBack(&wInverted, 10, 0);
	CPPUNIT_ASSERT_MESSAGE("CallBack begin>end is empty", wInverted.m_Values.empty());

	tCallBackCollectInt wStop;
	wStop.m_StopAfter = 1;
	wSparse.CallBack(&wStop);
	CPPUNIT_ASSERT_MESSAGE("CallBack stops when callback returns false", wStop.m_Values.size() == 1);

	tVectorInt wIndexes;
	wSparse.ForEachOccupied([&](tSize sIndex, tInt /*sValue*/) {
		wIndexes.push_back(static_cast<tInt>(sIndex));
		return true;
	});
	CPPUNIT_ASSERT_MESSAGE("ForEachOccupied reports global indexes",
		wIndexes.size() == 3 && wIndexes[0] == 0 && wIndexes[1] == 2 && wIndexes[2] == 300);

	tVectorInt wFromIndexes;
	wSparse.ForEachOccupiedFrom(3, [&](tSize sIndex, tInt /*sValue*/) {
		wFromIndexes.push_back(static_cast<tInt>(sIndex));
		return true;
	});
	CPPUNIT_ASSERT_MESSAGE("ForEachOccupiedFrom skips slots before begin",
		wFromIndexes.size() == 1 && wFromIndexes[0] == 300);
}

void TestSkSparseArray::setUp() {
	m_Application = tApplication::Instance();
};

void TestSkSparseArray::tearDown() {
}


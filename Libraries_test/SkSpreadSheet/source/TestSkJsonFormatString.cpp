//==============================================================================
// TestSkJsonFormatString
//==============================================================================
#include "../include/TestSkJsonFormatString.hpp"
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

TestSkJsonFormatString::TestSkJsonFormatString() : CPPUNIT_NS::TestFixture(), m_Application(nullptr), m_Api(nullptr) {
}

void TestSkJsonFormatString::TestJsonFormatStringStructure() {
    // Get JSON format string
    tString wJson = m_Api->JsonFormatString();
    CPPUNIT_ASSERT(!wJson.empty());
    
    // Parse JSON
    rapidjson::Document wDoc;
    rapidjson::ParseResult wParseResult = wDoc.Parse(wJson.c_str());
    CPPUNIT_ASSERT_MESSAGE("JSON parse error", wParseResult);
    
    // Verify root is an object
    CPPUNIT_ASSERT(wDoc.IsObject());
    
    // Verify "formatstring" key exists
    CPPUNIT_ASSERT(wDoc.HasMember("formatstring"));
    CPPUNIT_ASSERT(wDoc["formatstring"].IsArray());
    
    // Verify array is not empty
    CPPUNIT_ASSERT(wDoc["formatstring"].Size() > 0);
}

void TestSkJsonFormatString::TestJsonFormatStringContent() {
    // Get JSON format string
    tString wJson = m_Api->JsonFormatString();
    CPPUNIT_ASSERT(!wJson.empty());
    
    // Parse JSON
    rapidjson::Document wDoc;
    rapidjson::ParseResult wParseResult = wDoc.Parse(wJson.c_str());
    CPPUNIT_ASSERT_MESSAGE("JSON parse error", wParseResult);
    
    const rapidjson::Value& wFormatStringArray = wDoc["formatstring"];
    CPPUNIT_ASSERT(wFormatStringArray.IsArray());
    
    // Verify each element in the array has the correct structure
    for (rapidjson::SizeType i = 0; i < wFormatStringArray.Size(); i++) {
        const rapidjson::Value& wFamily = wFormatStringArray[i];
        CPPUNIT_ASSERT(wFamily.IsObject());
        
        // Verify "code" field exists and is a string
        CPPUNIT_ASSERT(wFamily.HasMember("code"));
        CPPUNIT_ASSERT(wFamily["code"].IsString());
        
        // Verify "fs" field exists and is an array
        CPPUNIT_ASSERT(wFamily.HasMember("fs"));
        CPPUNIT_ASSERT(wFamily["fs"].IsArray());
        
        // Verify each format in "fs" array has correct structure
        const rapidjson::Value& wFormats = wFamily["fs"];
        for (rapidjson::SizeType j = 0; j < wFormats.Size(); j++) {
            const rapidjson::Value& wFormat = wFormats[j];
            CPPUNIT_ASSERT(wFormat.IsObject());
            
            // Verify "f" field (format key) exists and is a string
            CPPUNIT_ASSERT(wFormat.HasMember("f"));
            CPPUNIT_ASSERT(wFormat["f"].IsString());
            
            // Verify "l" field (local format) exists and is a string
            CPPUNIT_ASSERT(wFormat.HasMember("l"));
            CPPUNIT_ASSERT(wFormat["l"].IsString());
        }
    }
}

void TestSkJsonFormatString::TestJsonFormatStringFamilies() {
    // Get JSON format string
    tString wJson = m_Api->JsonFormatString();
    CPPUNIT_ASSERT(!wJson.empty());
    
    // Parse JSON
    rapidjson::Document wDoc;
    rapidjson::ParseResult wParseResult = wDoc.Parse(wJson.c_str());
    CPPUNIT_ASSERT_MESSAGE("JSON parse error", wParseResult);
    
    const rapidjson::Value& wFormatStringArray = wDoc["formatstring"];
    CPPUNIT_ASSERT(wFormatStringArray.IsArray());
    
    // Verify that we have at least some format families
    CPPUNIT_ASSERT(wFormatStringArray.Size() > 0);
    
    // Count total formats across all families
    tInt wTotalFormats = 0;
    for (rapidjson::SizeType i = 0; i < wFormatStringArray.Size(); i++) {
        const rapidjson::Value& wFamily = wFormatStringArray[i];
        const rapidjson::Value& wFormats = wFamily["fs"];
        wTotalFormats += wFormats.Size();
    }
    
    // Verify we have formats
    CPPUNIT_ASSERT(wTotalFormats > 0);
    
    // Verify that format keys and local formats are not empty
    for (rapidjson::SizeType i = 0; i < wFormatStringArray.Size(); i++) {
        const rapidjson::Value& wFamily = wFormatStringArray[i];
        const rapidjson::Value& wFormats = wFamily["fs"];
        
        for (rapidjson::SizeType j = 0; j < wFormats.Size(); j++) {
            const rapidjson::Value& wFormat = wFormats[j];
            tString wFormatKey = wFormat["f"].GetString();
            tString wLocalFormat = wFormat["l"].GetString();
            
            // Verify format key is not empty
            CPPUNIT_ASSERT(!wFormatKey.empty());
            
            // Verify local format is not empty
            CPPUNIT_ASSERT(!wLocalFormat.empty());
        }
    }
}

void TestSkJsonFormatString::setUp() {
    std::filesystem::remove_all("./Spreadsheet");
    
    m_Application = tApplication::Instance();
    
    m_Api = new tApi;
    m_Api->NewWorkBook("test.skeema.fr/workbook1");
}

void TestSkJsonFormatString::tearDown() {
    delete m_Api;
}
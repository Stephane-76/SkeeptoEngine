//=============================================================================
// SkSpreadSheet CSV Import
//=============================================================================
#include "../include/SkCsvImport.hpp"
#include "../include/SkApi.hpp"
#include "../include/SkTools.hpp"
#include <SkFile.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace SkSpreadSheet {

    //=========================================================================
    // Constructor
    //=========================================================================
    tCsvImport::tCsvImport(tChar sDelimiter, tChar sQuote, tBool sSkipEmptyRows) 
        : tClass(),
          m_Delimiter(sDelimiter),
          m_Quote(sQuote),
          m_SkipEmptyRows(sSkipEmptyRows) {
    }

    //=========================================================================
    // Destructor
    //=========================================================================
    tCsvImport::~tCsvImport() {
    }

    //=========================================================================
    // Import CSV file using cell reference
    //=========================================================================
    tBool tCsvImport::Import(tString sFilePath, tString sDestinationRef, tSheet* sSheet, tWorkBook* sWorkBook) {
        // Parse destination cell reference
        tIndex wStartRow = 1;
        tIndex wStartCol = 1;
        if (!ParseCell(sDestinationRef, wStartRow, wStartCol)) {
            return false;
        }
        
        return Import(sFilePath, wStartRow, wStartCol, sSheet, sWorkBook);
    }

    //=========================================================================
    // Import CSV file using row/col coordinates
    //=========================================================================
    tBool tCsvImport::Import(tString sFilePath, tIndex sStartRow, tIndex sStartCol, tSheet* sSheet, tWorkBook* sWorkBook) {
        // Get active workbook if not provided
        if (sWorkBook == nullptr) {
            tSpreadSheetContainer* wContainer = tSpreadSheetContainer::Instance();
            if (wContainer == nullptr) {
                return false;
            }
            sWorkBook = wContainer->ActiveWorkBook();
            if (sWorkBook == nullptr) {
                return false;
            }
        }
        
        // Get active sheet if not provided
        if (sSheet == nullptr) {
            sSheet = sWorkBook->ActiveSheet();
            if (sSheet == nullptr) {
                std::cerr << "tCsvImport::Import: ActiveSheet is nullptr" << std::endl;
                return false;
            }
        }
        
        // In WASM/Emscripten, filesystem is mounted at /host
        // Convert absolute paths to use the mounted filesystem
        tString wFilePath = sFilePath;
#ifdef __EMSCRIPTEN__
        // If path doesn't start with /host, prepend it
        if (wFilePath.length() > 0 && wFilePath[0] == '/' && wFilePath.substr(0, 5) != "/host") {
            wFilePath = "/host" + wFilePath;
        }
#endif
        
        // Try to load file content using tFile (works better in WASM)
        tFile wFileObj(wFilePath);
        if (!wFileObj.Exist()) {
            std::cerr << "tCsvImport::Import: File does not exist: " << wFilePath << " (original: " << sFilePath << ")" << std::endl;
#ifdef __EMSCRIPTEN__
            std::cerr << "tCsvImport::Import: In WASM, make sure filesystem is mounted at /host" << std::endl;
#endif
            return false;
        }
        
        // Load entire file content
        tString wFileContent;
        try {
            wFileContent = wFileObj.LoadString();
        } catch (...) {
            std::cerr << "tCsvImport::Import: Failed to load file: " << wFilePath << " (original: " << sFilePath << ")" << std::endl;
            return false;
        }
        
        if (wFileContent.empty()) {
            std::cerr << "tCsvImport::Import: File is empty: " << wFilePath << " (original: " << sFilePath << ")" << std::endl;
            return false;
        }
        
        // Parse file content line by line
        tIndex wCurrentRow = sStartRow;
        std::vector<tString> wFields;
        std::istringstream wFileStream(wFileContent);
        tString wLine;
        
        // Read file line by line
        while (std::getline(wFileStream, wLine)) {
            // Parse line into fields
            wFields.clear();
            if (!ParseLine(wLine, wFields)) {
                continue; // Skip malformed lines
            }
            
            // Skip empty rows if flag is set
            if (m_SkipEmptyRows && wFields.empty()) {
                continue;
            }
            
            // Check if row is empty (all fields are empty strings)
            tBool wIsEmptyRow = true;
            for (const auto& wField : wFields) {
                if (!Trim(wField).empty()) {
                    wIsEmptyRow = false;
                    break;
                }
            }
            if (m_SkipEmptyRows && wIsEmptyRow) {
                continue;
            }
            
            // Insert fields into spreadsheet
            tIndex wCurrentCol = sStartCol;
            for (const auto& wField : wFields) {
                tVariant wValue = StringToVariant(Trim(wField));
                tCell* wCell = sSheet->ColRowCellRange()->EnsureCell(wCurrentRow, wCurrentCol);
                if (wCell != nullptr) {
                    sWorkBook->CellValue(wCell, wValue, false);
                }
                wCurrentCol++;
            }
            
            wCurrentRow++;
        }
        
        return true;
    }

    //=========================================================================
    // Parse CSV line into vector of strings
    //=========================================================================
    tBool tCsvImport::ParseLine(tString sLine, std::vector<tString>& sFields) {
        sFields.clear();
        
        if (sLine.empty()) {
            return true; // Empty line is valid
        }
        
        tString wCurrentField;
        tBool wInQuotes = false;
        tSize wPos = 0;
        
        while (wPos < sLine.length()) {
            tChar wChar = sLine[wPos];
            
            if (wChar == m_Quote) {
                if (wInQuotes) {
                    // Check if next character is also a quote (escaped quote)
                    if (wPos + 1 < sLine.length() && sLine[wPos + 1] == m_Quote) {
                        wCurrentField += m_Quote;
                        wPos += 2;
                        continue;
                    } else {
                        // End of quoted field
                        wInQuotes = false;
                    }
                } else {
                    // Start of quoted field
                    wInQuotes = true;
                }
            } else if (wChar == m_Delimiter && !wInQuotes) {
                // Field separator
                sFields.push_back(wCurrentField);
                wCurrentField.clear();
            } else {
                // Regular character
                wCurrentField += wChar;
            }
            
            wPos++;
        }
        
        // Add last field
        sFields.push_back(wCurrentField);
        
        return true;
    }

    //=========================================================================
    // Convert string to tVariant (auto-detect type)
    //=========================================================================
    tVariant tCsvImport::StringToVariant(tString sValue) {
        // Trim whitespace
        sValue = Trim(sValue);
        
        // Empty string
        if (sValue.empty()) {
            return tVariant();
        }
        
        // Try to parse as number (integer or double)
        // Check if it's a number (may start with + or -)
        tBool wIsNumber = true;
        tSize wStartPos = 0;
        if (sValue.length() > 0 && (sValue[0] == '+' || sValue[0] == '-')) {
            wStartPos = 1;
        }
        
        tBool wHasDecimal = false;
        for (tSize i = wStartPos; i < sValue.length(); i++) {
            if (sValue[i] == '.' && !wHasDecimal) {
                wHasDecimal = true;
            } else if (!std::isdigit(static_cast<unsigned char>(sValue[i]))) {
                wIsNumber = false;
                break;
            }
        }
        
        if (wIsNumber && sValue.length() > wStartPos) {
            // Try to parse as double first
            try {
                tDouble wDoubleValue = std::stod(sValue.c_str());
                // Check if it's actually an integer
                if (!wHasDecimal && wDoubleValue == static_cast<tInt>(wDoubleValue)) {
                    return tVariant(static_cast<tInt>(wDoubleValue));
                }
                return tVariant(wDoubleValue);
            } catch (...) {
                // Parsing failed, treat as string
            }
        }
        
        // Check for boolean values
        if (sValue == "TRUE" || sValue == "true" || sValue == "True") {
            return tVariant(true);
        }
        if (sValue == "FALSE" || sValue == "false" || sValue == "False") {
            return tVariant(false);
        }
        
        // Default: treat as string
        return tVariant(sValue);
    }

    //=========================================================================
    // Trim whitespace from string
    //=========================================================================
    tString tCsvImport::Trim(tString sStr) {
        if (sStr.empty()) {
            return sStr;
        }
        
        // Trim left
        tSize wStart = 0;
        while (wStart < sStr.length() && std::isspace(static_cast<unsigned char>(sStr[wStart]))) {
            wStart++;
        }
        
        // Trim right
        tSize wEnd = sStr.length();
        while (wEnd > wStart && std::isspace(static_cast<unsigned char>(sStr[wEnd - 1]))) {
            wEnd--;
        }
        
        return sStr.substr(wStart, wEnd - wStart);
    }

    //=========================================================================
    // Set delimiter
    //=========================================================================
    void tCsvImport::Delimiter(tChar sDelimiter) {
        m_Delimiter = sDelimiter;
    }

    //=========================================================================
    // Get delimiter
    //=========================================================================
    tChar tCsvImport::Delimiter() {
        return m_Delimiter;
    }

    //=========================================================================
    // Set quote
    //=========================================================================
    void tCsvImport::Quote(tChar sQuote) {
        m_Quote = sQuote;
    }

    //=========================================================================
    // Get quote
    //=========================================================================
    tChar tCsvImport::Quote() {
        return m_Quote;
    }

    //=========================================================================
    // Set skip empty rows
    //=========================================================================
    void tCsvImport::SkipEmptyRows(tBool sSkipEmptyRows) {
        m_SkipEmptyRows = sSkipEmptyRows;
    }

    //=========================================================================
    // Get skip empty rows
    //=========================================================================
    tBool tCsvImport::SkipEmptyRows() {
        return m_SkipEmptyRows;
    }

} // namespace SkSpreadSheet


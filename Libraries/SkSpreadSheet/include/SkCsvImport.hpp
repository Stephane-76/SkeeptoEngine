//=============================================================================
// SkSpreadSheet CSV Import
//=============================================================================
#ifndef SkCsvImport_hpp
#define SkCsvImport_hpp

#include "SkSpreadSheet.hpp"
#include "SkTools.hpp"
#include <fstream>
#include <vector>
#include <string>

namespace SkSpreadSheet {

    //=========================================================================
    //! CSV Import class for importing CSV files into spreadsheet
    //=========================================================================
    class tCsvImport : public tClass {
    private:
        //! Delimiter character (default: ',')
        tChar m_Delimiter;
        
        //! Quote character (default: '"')
        tChar m_Quote;
        
        //! Whether to skip empty rows
        tBool m_SkipEmptyRows;
        
    public:
        /// @brief      Constructor
        /// @param[in]  sDelimiter tChar delimiter character (default: ',')
        /// @param[in]  sQuote tChar quote character (default: '"')
        /// @param[in]  sSkipEmptyRows tBool skip empty rows (default: true)
        tCsvImport(tChar sDelimiter = ',', tChar sQuote = '"', tBool sSkipEmptyRows = true);
        
        /// @brief      Destructor
        ~tCsvImport();
        
        /// @brief      Import CSV file into spreadsheet
        /// @param[in]  sFilePath tString path to CSV file
        /// @param[in]  sDestinationRef tString cell reference for top-left destination (e.g., "A1")
        /// @param[in]  sSheet tSheet* target sheet (if nullptr, uses active sheet)
        /// @param[in]  sWorkBook tWorkBook* workbook (if nullptr, uses active workbook)
        /// @return     tBool true if successful, false otherwise
        tBool Import(tString sFilePath, tString sDestinationRef, tSheet* sSheet = nullptr, tWorkBook* sWorkBook = nullptr);
        
        /// @brief      Import CSV file into spreadsheet using row/col coordinates
        /// @param[in]  sFilePath tString path to CSV file
        /// @param[in]  sStartRow tIndex starting row (0-based)
        /// @param[in]  sStartCol tIndex starting column (0-based)
        /// @param[in]  sSheet tSheet* target sheet (if nullptr, uses active sheet)
        /// @param[in]  sWorkBook tWorkBook* workbook (if nullptr, uses active workbook)
        /// @return     tBool true if successful, false otherwise
        tBool Import(tString sFilePath, tIndex sStartRow, tIndex sStartCol, tSheet* sSheet = nullptr, tWorkBook* sWorkBook = nullptr);
        
        /// @brief      Set delimiter character
        /// @param[in]  sDelimiter tChar delimiter character
        void Delimiter(tChar sDelimiter);
        
        /// @brief      Get delimiter character
        /// @return     tChar delimiter character
        tChar Delimiter();
        
        /// @brief      Set quote character
        /// @param[in]  sQuote tChar quote character
        void Quote(tChar sQuote);
        
        /// @brief      Get quote character
        /// @return     tChar quote character
        tChar Quote();
        
        /// @brief      Set skip empty rows flag
        /// @param[in]  sSkipEmptyRows tBool skip empty rows
        void SkipEmptyRows(tBool sSkipEmptyRows);
        
        /// @brief      Get skip empty rows flag
        /// @return     tBool skip empty rows flag
        tBool SkipEmptyRows();
        
    private:
        /// @brief      Parse CSV line into vector of strings
        /// @param[in]  sLine tString CSV line
        /// @param[out] sFields std::vector<tString>& output vector of fields
        /// @return     tBool true if successful
        tBool ParseLine(tString sLine, std::vector<tString>& sFields);
        
        /// @brief      Convert string to tVariant (auto-detect type)
        /// @param[in]  sValue tString value to convert
        /// @return     tVariant converted value
        tVariant StringToVariant(tString sValue);
        
        /// @brief      Trim whitespace from string
        /// @param[in]  sStr tString string to trim
        /// @return     tString trimmed string
        tString Trim(tString sStr);
    };

} // namespace SkSpreadSheet

#endif // SkCsvImport_hpp


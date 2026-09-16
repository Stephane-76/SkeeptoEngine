//=============================================================================
//  Skeema File
/**
* @page SkFile
* @par
* @par Class for file management..
*/
//=============================================================================
#ifndef SkFile_hpp
#define SkFile_hpp

#include <exception>
#include <stdio.h>
#include <fstream>
#include <streambuf>
#include <filesystem>
#include <sys/stat.h>

#include "../include/SkTypes.hpp"
#include "../include/SkClass.hpp"

namespace SkRoot {
    //=========================================================================
    //! Class file, m_Buffer is not owner by object
    class tFile : public tVirtualClass {
        private:
            tString m_FileName;
            tSize    m_Length;

        public:
            /// @brief Constructor SkFile.
            tFile();

            /// @brief Constructor SkFile with file name.
            /// @param[in] sFileName tString
            tFile(tString sFileName);
        
            /// @brief Set the file name.
            /// @param[in] sFileName tString
            void FileName(tString sFileName);

            /// @brief Return the length of the file.
            /// @return tSize
            tSize Length();

            /// @brief Write file with buffer.
            /// @param[in] sBuffer tChar* Buffer
            /// @param[in] sSize tSize SizeOfBuffer
            void SaveBuffer(const tChar* sBuffer, tSize sSize);

            /// @brief Read file into buffer.
            /// @return tChar* Buffer
            tChar* LoadAndAllocBuffer();

            /// @brief Write file with string.
            /// @param[in] sString tString 
            void SaveString(tString sString);

            /// @brief Read file into string.
            /// @return tString  
            tString LoadString();

            /// @brief Delete the file.         
            void Delete();

            /// @brief Test if the file exists.
            /// @return tBool true if exists
            tBool Exist();

            /// @brief Return the file name.
            /// @return tString 
            tString FileName();

            /// @brief Return the file name without extension.
            /// @return tString 
            tString FileNameWE();

            /// @brief Return the directory of the file.
            /// @return tString 
            tString Directory();

            /// @brief Return the extension of the file.
            /// @return tString 
            tString Extension();
    };
    typedef vector<tFile> tVectorFile;

    //========================================================================
    // Class For Read Directory
    class tDirectory : public tClass {
    private:
        tString     m_Directory;
        tString     m_Select;
        tVectorFile m_VectorFile;
    public:
        /// @brief Constructor SkDirectory.
        tDirectory();

        /// @brief Constructor SkDirectory with directory.
        /// @param[in] sDirectory tString
        tDirectory(tString sDirectory);
        
        /// @brief Clear the directory.
        void Clear();
        
        /// @brief Load files with selection.
        /// @param[in] sDirectory tString
        /// @param[in] sSelect tString
        /// @return tBool
        tBool LoadFile(tString sDirectory, tString sSelect);
        
        /// @brief Return the vector of files.
        /// @return tVectorFile&
        tVectorFile& VectorFile();
    };
} // End of namespace
#endif

//=============================================================================
// Skeema File
// Class for file management
//=============================================================================
#include "../include/SkFile.hpp"
#include "../include/SkApplication.hpp"

namespace SkRoot {

    tFile::tFile() :tVirtualClass(), m_FileName(""), m_Length(0) {}
    tFile::tFile(tString sFileName) : tVirtualClass(), m_FileName(sFileName), m_Length(0) {};

    void  tFile::FileName(tString sFileName) {
        m_FileName=sFileName;
    }

    tSize tFile::Length() { return(m_Length); }

    void tFile::SaveBuffer(const tChar* sBuffer, tSize sSize) {
        m_Length = sSize;

        std::ofstream wFileStream;
        wFileStream.open(m_FileName.c_str());           // open output file
        wFileStream.write(sBuffer, m_Length);
        wFileStream.close();
    }

    tChar* tFile::LoadAndAllocBuffer() {
        std::ifstream wFileStream;
        wFileStream.open(m_FileName.c_str(), std::ios::binary | std::ios::ate);           // open input file
        if (!wFileStream) {
            m_Length = 0;
            tChar* sBuffer = (tChar*) tApplication::Instance()->AllocMem(1);
            sBuffer[0] = '\0';
            return(sBuffer);
        }
        const std::streamoff wPos = wFileStream.tellg();
        if (wPos < 0) {
            m_Length = 0;
            tChar* sBuffer = (tChar*) tApplication::Instance()->AllocMem(1);
            sBuffer[0] = '\0';
            return(sBuffer);
        }
        m_Length = static_cast<tSize>(wPos);
        wFileStream.seekg(0, std::ios::beg);    // go back to the beginning
        tChar* sBuffer = (tChar*) tApplication::Instance()->AllocMem(m_Length+1);
        wFileStream.read(sBuffer, static_cast<std::streamsize>(m_Length));
        wFileStream.close();                    // close file handle

        sBuffer[m_Length] = '\0'; // terminate buffer width 0
        return(sBuffer);
    };

    void tFile::SaveString(tString sString) {
        m_Length = sString.length();
        std::ofstream wFileStream;
        wFileStream.open(m_FileName.c_str(),ios::out | ios::binary);  // open output file
        wFileStream.write(sString.c_str(), m_Length);
        wFileStream.close();

    }
    tString tFile::LoadString() {
        std::ifstream wFileStream;
        wFileStream.open(m_FileName.c_str(), std::ios::in | std::ios::binary);
        if (!wFileStream) {
            m_Length = 0;
            return tString();
        }
        wFileStream.seekg(0, std::ios::end);
        const std::streamoff wPos = wFileStream.tellg();
        if (wPos < 0) {
            m_Length = 0;
            return tString();
        }
        m_Length = static_cast<tSize>(wPos);
        wFileStream.seekg(0, std::ios::beg);
        tChar* sBuffer = (tChar*)tApplication::Instance()->AllocMem(m_Length + 1);
        wFileStream.read(sBuffer, static_cast<std::streamsize>(m_Length));
        wFileStream.close();
        sBuffer[m_Length] = '\0';
        tString wResult = tString(sBuffer, m_Length);
        tApplication::Instance()->FreeMem((void*)sBuffer);
        return(wResult);
    }


    void tFile::Delete() {
        std::remove(m_FileName.c_str());
    }

    tBool tFile::Exist() {
        struct stat buffer;
        return (stat(m_FileName.c_str(), &buffer) == 0);
    }

    tString tFile::FileName() {
        tSize wIndex = m_FileName.find_last_of(cSlash);
        return(m_FileName.substr(wIndex+1));
    }

    tString tFile::FileNameWE() {
        tString wFileName = FileName();
        tSize wIndex = wFileName.find_last_of('.');
        if (wIndex > 0) {
            return(wFileName.substr(0,wIndex));
        }
        return(wFileName);
    }

    tString tFile::Directory() {
        tSize wIndex = m_FileName.find_last_of(cSlash);
        if (wIndex > 0) {
            return(m_FileName.substr(0,wIndex));
        }
        return("");
    }

    tString tFile::Extension() {
        tSize wIndex = m_FileName.find_last_of('.');
        return(m_FileName.substr(wIndex + 1));
    };

    tDirectory::tDirectory() : tClass(),m_Directory(),m_Select() {
        
    }

    tDirectory::tDirectory(tString sDirectory) : tClass(),m_Directory(sDirectory), m_Select() {
        
    }

    
    void tDirectory::Clear() {
        m_VectorFile.clear();
    }

    tBool tDirectory::LoadFile(tString sDirectory,tString sSelect) {
        Clear();
        // Spécifiez le chemin du répertoire
        std::filesystem::path directoryPath = sDirectory;
        

         // Vérifiez si le répertoire existe
         if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath)) {
             return(false);
         }

         // Itération sur les fichiers du répertoire
         for (const auto& wEntry : std::filesystem::directory_iterator(directoryPath)) {
             if (std::filesystem::is_regular_file(wEntry.status())) {
                 //std::cout << "Fichier : " << wEntry.path() << std::endl;
                 tString wPath=wEntry.path().string();
                 tSize wIndex = wPath.find_last_of(cSlash);
                 tString wFileName=wPath.substr(wIndex+1);
                 tClassString wClassString(wFileName);
                 if (wClassString.Regex_Match(sSelect)) {
                     // Selection Ok
                     m_VectorFile.push_back(tFile(wPath));
                 }
            
                 // Futur
                 //std::filesystem::file_time_type wTime = std::filesystem::last_write_time(wEntry);
                 //std::filesystem::file_status wStatus=std::filesystem::status(wEntry);
                 
             }
         }
        return(true);
    }
    tVectorFile& tDirectory::VectorFile() { return(m_VectorFile); }

} // End of namespace

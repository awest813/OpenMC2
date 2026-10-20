#include "gameos.hpp"
#include "toolos.hpp"
#include "fileio.hpp"

#include<string.h>

// gos_FileSize
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//#include <unistd.h> // close()
#include "platform_io.h"
//

#include <stdio.h> // fopen
#include "platform_io.h"

#include <errno.h>
#include <filesystem>

#include "platform_io.h" // will be removed

gosFileStream::gosFileStream( const char *FileName, gosEnum_FileWriteStatus fwstatus )
{
    gosASSERT(FileName);
    gosASSERT(strlen(FileName) < MAX_PATH);
    m_writeEnabled = fwstatus;
    strcpy(m_Filename, FileName);

    // ???
    pNext = 0;

    const char* mode = 0;
    switch(fwstatus) {
        case READONLY:
            mode = "rb";
            break;
        // see gosEnum_FileWriteStatus in toolos.hpp ("rwb" is not a valid mode, it opened files read-only)
        case READWRITE: // read and write, clears the file on open
            mode = "w+b";
            break;
        case APPEND: // read and write, keeps the file if it exists
            mode = "a+b";
            break;
        default:
            STOP(("Unknown file open mode"));
            gosASSERT(0);
    }

    m_hFile = fopen(m_Filename, mode);
    if(!m_hFile)
    {
        int err = errno; // save errno from being possible overwritten by sprintf
        fprintf(stderr, "gosFileStream: failed to open file: %s : %s\n", m_Filename, strerror(err));
        STOP(("Failed to open file"));
    }
}

DWORD gosFileStream::Seek( int where, gosEnum_FileSeekType seek_type)
{
    gosASSERT(m_hFile);
    int whence = 0;
    switch(seek_type) {
        case FROMSTART:
            whence = SEEK_SET;
            break;
        case FROMCURRENT:
            whence = SEEK_CUR;
            break;
        case FROMEND:
            whence = SEEK_END;
            break;
        default:
            STOP(("Unknown seek_type"));
            gosASSERT(0);
    }

    // returns the new position (like SetFilePointer), or (DWORD)-1 on error
    if(fseek((FILE*)m_hFile, where, whence) != 0)
        return (DWORD)-1;
    return (DWORD)ftell((FILE*)m_hFile);
}

DWORD gosFileStream::Read( void *buffer, DWORD length )
{
    gosASSERT(m_hFile);
    size_t size = 1;
    size_t nmemb = length;
    size_t num_items = fread(buffer, size, nmemb, (FILE*)m_hFile);
    gosASSERT(num_items == nmemb); // .. can trigger if eof
    return num_items * size;
}

DWORD gosFileStream::Write( const void *buffer, DWORD length )
{
    gosASSERT(m_hFile);
    size_t size = 1;
    size_t nmemb = length;
    size_t num_items = fwrite(buffer, size, nmemb, (FILE*)m_hFile);
    gosASSERT(num_items == nmemb); // .. can trigger if eof
    return num_items * size;
}

gosFileStream::~gosFileStream()
{
    gosASSERT(m_hFile);
    fclose((FILE*)m_hFile);
}

// toolos.hpp
void __stdcall gos_FileSetReadWrite(char const* FileName)
{
    STOP(("gos function not implemented"));
}

DWORD __stdcall gos_FileSize(char const* FileName)
{
	int fd = _open(FileName, O_RDONLY);
    if(fd == -1) {
        STOP(("gos_FileSize failed"));
        return 0;
    }

    struct stat buf;
    fstat(fd, &buf);
    _close(fd);
    
    return buf.st_size;
}
__int64 __stdcall gos_FileTimeStamp(char const* FileName)
{
    return 0;
}
// HGOSFILE = gosFileStream*
void __stdcall gos_OpenFile(gosFileStream** out_fstream, char const* path, gosEnum_FileWriteStatus fwstatus)
{
    *out_fstream = new gosFileStream(path, fwstatus);
}

DWORD __stdcall gos_ReadFile(HGOSFILE hfile, void* buf, DWORD size)
{
    gosFileStream* fstream = hfile;
    return fstream->Read(buf, size);
}

DWORD __stdcall gos_WriteFile(HGOSFILE hfile, void const* buf, DWORD size)
{
    gosFileStream* fstream = hfile;
    return fstream->Write(buf, size);
}

DWORD __stdcall gos_SeekFile(HGOSFILE hfile, gosEnum_FileSeekType type, int offset)
{
    gosFileStream* fstream = hfile;
    return fstream->Seek(offset, type);
}

void __stdcall gos_CloseFile(gosFileStream* hfile)
{
    gosFileStream* fstream = hfile;
    delete fstream;
}
////////////////////////////////////////////////////////////////////////////////
bool __stdcall gos_DoesFileExist(char const* FileName)
{
    if(Environment.HookDoesFileExist)
        return Environment.HookDoesFileExist(FileName);
    return gos_FileExists(FileName);
}
bool __stdcall gos_CreateDirectory(char const* FileName)
{
    std::error_code ec;
    std::filesystem::create_directories(FileName, ec);
    return !ec && std::filesystem::is_directory(FileName, ec);
}
char* __stdcall gos_FindDirectories(char const* DirectoryName)
{
    STOP(("gos function not implemented"));
    return NULL;
}
void __stdcall gos_FindDirectoriesClose()
{
    STOP(("gos function not implemented"));
}
char* __stdcall gos_FindDirectoriesNext()
{
    STOP(("gos function not implemented"));
    return NULL;
}
char* __stdcall gos_FindFiles(char const* PathFileName)
{
    STOP(("gos function not implemented"));
    return NULL;
}
void __stdcall gos_FindFilesClose()
{
    STOP(("gos function not implemented"));
}
char* __stdcall gos_FindFilesNext()
{
    STOP(("gos function not implemented"));
    return NULL;
}

void __stdcall gos_GetCurrentPath(char* Buffer, int buf_len)
{
    gosASSERT(Buffer);
    if(getcwd(Buffer, buf_len) == NULL) {
        int last_error = errno;
        STOP(("Current path is longer than buffer provided for it, getwd: %s\n", strerror(last_error)));
    }
}

// reads a whole file into memory allocated with gos_Malloc (the caller frees it with gos_Free)
void __stdcall gos_GetFile(char const* FileName, BYTE** MemoryImage, SIZE_T* Size)
{
    gosASSERT(MemoryImage && Size);
    *MemoryImage = NULL;
    *Size = 0;

    if(Environment.HookGetFile) {
        DWORD size = 0;
        Environment.HookGetFile(FileName, MemoryImage, &size);
        *Size = size;
        return;
    }

    FILE* f = fopen(FileName, "rb");
    if(!f) {
        STOP(("gos_GetFile: cannot open %s: %s\n", FileName, strerror(errno)));
        return;
    }
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    if(len < 0) {
        fclose(f);
        STOP(("gos_GetFile: cannot get the size of %s\n", FileName));
        return;
    }
    BYTE* data = (BYTE*)gos_Malloc(len > 0 ? len : 1);
    if(len > 0 && fread(data, 1, len, f) != (size_t)len) {
        fclose(f);
        gos_Free(data);
        STOP(("gos_GetFile: failed to read %s\n", FileName));
        return;
    }
    fclose(f);
    *MemoryImage = data;
    *Size = (SIZE_T)len;
}

// sebi
bool __stdcall gos_FileExists(char const* FileName)
{
    if(0 == access(FileName, F_OK))
        return true;
    return false;
}

////////////////////////////////////////////////////////////////////////////////

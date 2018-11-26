#ifndef ABSTRACTFILE_H
#define ABSTRACTFILE_H
#include "common.h"

class tabstractfile
{
protected:
    void           ChangeMode (int, mode_t);
    int            ReadFileMode();
    size_t         FileSize  (struct stat);
    bool           FileExists(int);
    int            FileOpen  (string, int);

public:
    int pFile;
    tabstractfile();
    int              FileClose (int);
    bool             FileStat  (int, struct stat &);
    bool             FileStat  (string, struct stat &);
    size_t           FileSize  (int);
    void             WriteFile (int , string);
    void             FsyncFile (int);
    int              FileDelete(string);
    bool             FileExists(struct stat);
    bool             FileExists(string);
    int              AppendFileMode();
    int              TruncateFileMode();
    bool             MakeDirectory(string, mode_t, uid_t, gid_t);
    bool             MakeDirectory(string, string, mode_t, uid_t, gid_t);
};

#endif // ABSTRACTFILE_H

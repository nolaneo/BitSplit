#ifndef FILETREEDIRECTORY_H
#define FILETREEDIRECTORY_H

#include "fileinterface.h"
#include "filetreenode.h"

class FileTreeDirectory
{
public:
    FileTreeDirectory();

    bool required, inTree;
    uint id, depth, start, end;
    FileTreeNode * node;

    QSet< FileTreeNode * > subNodes;
    QSet< FileTreeDirectory * > previouslyMatched;
};

#endif // FILETREEDIRECTORY_H

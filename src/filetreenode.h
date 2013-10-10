#ifndef FILETREENODE_H
#define FILETREENODE_H

#include <QtCore>
#include "filelink.h"
class FileTreeDirectory;

class FileTreeNode
{
public:
    FileTreeNode();
    QString fileName;
    QFileInfo fileInfo;
    QStringList reducedName;
    QSet<uint> reducedHash;
    QList< FileLink > links;
    QString fileType;
    FileTreeNode * parent;
    FileTreeNode * match;
    FileTreeDirectory * directory;
    bool matched;
    void insertLink(FileTreeNode *, float);
    quint64 fileId;
};

#endif // FILETREENODE_H

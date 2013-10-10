#ifndef FILELINK_H
#define FILELINK_H

class FileTreeNode;
class FileLink
{
public:
    FileLink(FileTreeNode *, FileTreeNode *, float);
    FileTreeNode * l, *r;
    float probability;

    float getProbability() const;
    bool operator < (const FileLink & ) const;
};

#endif // FILELINK_H

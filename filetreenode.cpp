#include "filetreenode.h"

FileTreeNode::FileTreeNode()
{
}

void FileTreeNode::insertLink(FileTreeNode * r, float p){
    FileLink link(this, r, p);
    links<<link;
}

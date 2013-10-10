#include "filelink.h"

FileLink::FileLink(FileTreeNode * left, FileTreeNode * right, float prob)
{
    l = left;
    r = right;
    probability = prob;
}

float FileLink::getProbability() const{
    return probability;
}

bool FileLink::operator < (const FileLink & other) const{
    return   probability>other.getProbability();
}

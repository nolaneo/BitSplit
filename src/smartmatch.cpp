#include "smartmatch.h"

#define treeNodes fileInterface->treeNodes
#define treeDirs fileInterface->treeDirs

SmartMatch::SmartMatch()
{
    //############################################## TEMP SETTINGS ###################################################
    copyHidden = false;
    examineFileSize = false;
    checkSize = false;
    splitRegex = QRegExp("[\\\\,\\. %#-]+");
    regex = QRegExp("(^[A-Za-z]:$)|(^A$)|(^THE$)|(^AND$)|(^OF$)|(^\\(\\d{4}\\)$)|(^\\(\\d+KBS\\)$)|(^\\(?480P\\)?$)|(^\\(?720P\\)?$)|(^\\(?1080P\\)?$)|(XVID)|(DVDRIP)|(BLURAY)|(AC3)|(X264)|(BRRIP)|(AAC)");
    depth = 1;
    threshold = 0.99;
    minimumSize = 0;
    hashVsString = true;

    //################################################################################################################
}
void SmartMatch::updateGuiLabels(){
    if(constructingTree)
        fileInterface->single(currentL->fileInfo.absoluteFilePath());
    else
        fileInterface->mpTopBottom(currentL->fileInfo.absoluteFilePath(), currentR->fileInfo.absoluteFilePath());
}
void SmartMatch::run(){
    noErrors = true;
    fileInterface->log("Beginning file tree creation...");
    initalSetup();


    fileInterface->log("Nodes total: "+QString::number(treeNodes.size()));
    fileInterface->log("Directories total: "+QString::number(treeDirs.size()));
    fileInterface->log("File tree creation completed.");

    currentL = treeDirs.at(0).node;
    currentR = treeDirs.at(rightRootStart).node;

    emit startTime();


    emit setTopBarLineStack(2);
    fileInterface->mpOperation("Matching files");
    fileInterface->log("Beginning file matching...");
    runSmartMatch(&treeDirs[0], &treeDirs[rightRootStart]);
    fileInterface->log("File matching completed.");

    //debugPrintMatches(&treeDirs[0]);
    emit stopTime();


    fileInterface->mpOperation("Complete");
    fileInterface->log("SmartMatch complete.");
    fileInterface->log("Linking right matches.");


    linkRightMatches(&treeDirs[0]);


    fileInterface->log("Computing GUI tree requirements.");
    computeGuiTree();
    fileInterface->log("Completed GUI tree requirements.");


    fileInterface->log("Showing GUI tree.");
    treeDirs[0].inTree = false;
    if(fileInterface->copyDirection == LeftToRight || fileInterface->copyDirection == BothDirections){
        fileInterface->insertFileTreeItem(true, treeDirs[0].node, 0, false);
        showGuiTree(&treeDirs[0]);
        fileInterface->popStack();
    }
    treeDirs[rightRootStart].inTree = false;
    if(fileInterface->copyDirection == RightToLeft || fileInterface->copyDirection == BothDirections){
        fileInterface->insertFileTreeItem(true, treeDirs[rightRootStart].node, 0, false);
        showGuiTree(&treeDirs[rightRootStart]);
    }

    fileInterface->log("Completed GUI tree.");
    fileInterface->treeConstructed();
    

    emit stopTestTime();

    /* Clean up */
    unmatchedLeft.clear();
    unmatchedRight.clear();

    fileInterface->finshedMatch();

}

/***********************************************************************
    Inital setup creates the two root nodes which are required to populate the rest of the tree.
*/
void SmartMatch::initalSetup(){
    constructingTree = true;
    fileInterface->mpOperation("Creating left tree");
    dirId = 0;
    FileTreeNode leftRootNode, rightRootNode;
    FileTreeDirectory leftRootDir, rightRootDir;

    try{
        leftRootNode.fileName = fileInterface->lDir;
        QFile lFile (fileInterface->lDir);
        if(!lFile.exists())
            throw 1;

        leftRootNode.fileInfo = QFileInfo(lFile);
        leftRootNode.parent = NULL;
        leftRootNode.fileId = 0;
        treeNodes<<leftRootNode;
        leftRootDir.id = dirId++;

        /* Setting the pointers from node to dir and vice versa */
        leftRootDir.node = &treeNodes[treeNodes.size()-1];
        treeDirs<<leftRootDir;
        treeNodes.last().directory = &treeDirs[treeDirs.size()-1];
        currentL = &treeNodes[0];
        emit startTime();
        constructTree(&treeDirs[treeDirs.size()-1], 0, &unmatchedLeft);

        fileInterface->mpOperation("Creating right tree");
        rightRootNode.fileName = fileInterface->rDir;
        QFile rFile (fileInterface->rDir);

        rightRootNode.fileInfo = QFileInfo(rFile);
        if(!lFile.exists())
            throw 2;

        rightRootNode.fileId = 0;
        rightRootNode.parent = NULL;
        treeNodes<<rightRootNode;
        rightRootDir.id = dirId++;

        /* Setting the pointers from node to dir and vice versa */
        rightRootDir.node = &treeNodes[treeNodes.size()-1];
        treeDirs<<rightRootDir;
        treeNodes.last().directory = &treeDirs[treeDirs.size()-1];
        rightRootStart = treeDirs.size()-1;
        fileInterface->rightRootStart = rightRootStart;
        constructTree(&treeDirs[treeDirs.size()-1], 0, &unmatchedRight);
        emit stopTime();

        treeDirs[0].node->matched = true;
        treeDirs[rightRootStart].node->matched = true;
        treeDirs[0].required = true;
        treeDirs[0].node->match = treeDirs[rightRootStart].node;
        treeDirs[rightRootStart].node->match = treeDirs[0].node;
        treeDirs[rightRootStart].required = true;

        computeRequiredDirectories(&treeDirs[0]);
        computeRequiredDirectories(&treeDirs[rightRootStart]);


    }catch(int i){
        fileInterface->log("ERROR!: Number #" + QString::number(i) + " (Directory does not exist)");
        emit operationFailed();
        noErrors = false;
        this->sleep(ULONG_MAX);
        return;

    }catch(std::exception &e){
        fileInterface->log(QString("ERROR! (File tree creation): " + QString(e.what())));
        emit operationFailed();
        noErrors = false;
        this->sleep(ULONG_MAX);
        return;
    }

    constructingTree = false;

    //debugPrintTree(&treeDirs[0]);
    //debugPrintTree(&treeDirs[rightRootStart]);

}

/***********************************************************************
    Given a FileTreeDirectory pointer, this function creates a file tree based on the information contained in that node.
    Each node created is stored in the FileTreeNode list contained in the FileInterface class.
*/
void SmartMatch::constructTree(FileTreeDirectory * dir, uint d, QSet<FileTreeNode*> * unmatchedSet){
    try{
        dir->depth = d;
        dir->start = treeNodes.size();

        QFileInfoList info;
        uint count = 0;

        if(copyHidden)
            info = QDir(dir->node->fileInfo.absoluteFilePath()).entryInfoList(fileInterface->fileTypeFilter, QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);
        else
            info = QDir(dir->node->fileInfo.absoluteFilePath()).entryInfoList(fileInterface->fileTypeFilter, QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);

        /* Create each node. Nodes must be either a directory or a file greater than the minimum size. For files, increment the total count for this directory */
        for(int i = 0; i < info.size(); ++i){
            /* Make sure the item still exists before we add anything */
            if(info.at(i).exists()){
                if(!checkSize || ( info.at(i).isDir() || (quint64)info.at(i).size() > minimumSize) ) {
                    FileTreeNode item;
                    item.parent = dir->node;
                    item.directory = NULL;
                    item.matched = false;
                    item.match = NULL;
                    item.fileInfo = info[i];
                    item.fileName = info[i].fileName();
                    item.fileId = 0;

                    if(!info.at(i).isDir()){
                        count++;
                        /* Check if the file type is contained in one of the default file type lists AUDIO, VIDEO or IMAGES*/
                        for(int j = 0; j < fileTypes.size() && !exactSuffix; ++j){
                            if(fileTypes.at(j).contains(info.at(i).suffix()))
                                item.fileType = fileTypeNames.at(j);
                        }
                        if(item.fileType == NULL)
                            item.fileType = info.at(i).suffix();
                    }



                    /* Perform filename reduction using the user specified regular expressions */
                    QString current;
                    item.reducedName = item.fileName.toUpper().split(splitRegex);
                    for(int j = 0; j < item.reducedName.size();){
                        current = item.reducedName.at(j);
                        if(current.contains(regex))
                            item.reducedName.removeAll(current);
                        else
                            j++;
                    }
                    /* Hash each word of the reduced file name */
                    for(int j = 0; j < item.reducedName.size(); ++j){
                        item.reducedHash.insert(qHash(item.reducedName[j]));
                    }
                    item.reducedName.clear();

                    treeNodes<<item;
                    currentL = &treeNodes.last();
                    unmatchedSet->insert(&treeNodes[treeNodes.size()-1]);
                    dir->subNodes.insert(&treeNodes[treeNodes.size()-1]);
                }
            }
        }

        dir->end = treeNodes.size();

        /* If the file count in this directory is greater than zero after filters are applied, mark it as required for copying */
        if(count>0)
           dir->required = true;
        else
           dir->required = false;

        /* Create each directory node extension */
        for(int i = 0, j = 0; i < info.size(); ++i){
            if(info.at(i).exists()){
                if(info[i].isDir()){
                    FileTreeDirectory newDirectory;
                    newDirectory.depth = d;
                    newDirectory.id = dirId++;
                    if(checkSize)
                        newDirectory.node = &treeNodes[dir->start+j];
                    else
                        newDirectory.node = &treeNodes[dir->start+i];

                    treeDirs<<newDirectory;

                    if(checkSize)
                        treeNodes[dir->start+j].directory = &treeDirs[treeDirs.size()-1];
                    else
                        treeNodes[dir->start+i].directory = &treeDirs[treeDirs.size()-1];

                    constructTree(&treeDirs[treeDirs.size()-1], d+1, unmatchedSet);
                }
            }
            /*
            We only increment J if what we are dealing with is a directory OR a file greater than minimum size.
            Because those files below the user specified minimum size are ignored, using "i" as the index for treeDirs would cause seg faults.
            */
            if(info.at(i).isDir() || (quint64)info.at(i).size() > minimumSize){
                ++j;
            }
        }

    }catch(std::exception &e){
        fileInterface->log(QString("ERROR! Node insertion: " + QString(e.what())));
        noErrors = false;
        this->sleep(ULONG_MAX);
        return;
    }
}

/***********************************************************************
    For directories which only contain subdirectories and no files we recursively check to see if they should be included in the file tree.
*/

void SmartMatch::computeRequiredDirectories(FileTreeDirectory * dir){

    for(uint i = dir->start; i < dir->end; ++i){
        if(treeNodes[i].fileInfo.isDir() && treeNodes[i].directory->required){
            FileTreeNode * p = dir->node;
            while(!p->directory->required){
                p->directory->required = true;
                p = p->parent;
            }
            break;
        }
    }
    for(uint i = dir->start; i < dir->end; ++i)
        if(treeNodes[i].fileInfo.isDir())
            computeRequiredDirectories(treeNodes[i].directory);

}

/***********************************************************************
    Containing function which executes both the initial matches and the following link finialisation as well as recursive calls to match
*/
void SmartMatch::runSmartMatch(FileTreeDirectory * lDir, FileTreeDirectory * rDir){
    performMatching(lDir, rDir, lDir->depth+depth, rDir->depth+depth);

    sortLinks(lDir, lDir->depth + depth);

    QHash<FileTreeNode*, FileLink> fileLinks;
    finaliseMatches(lDir, &fileLinks);

    //fileInterface->log("FileLinks Size: "+ QString::number(fileLinks.size()));
    QHash<FileTreeNode*, FileLink>::iterator itr;
    for(itr = fileLinks.begin(); itr != fileLinks.end(); ++itr)
        if(itr.value().l->fileInfo.isDir())
            runSmartMatch(itr.value().l->directory, itr.value().r->directory);

}

void SmartMatch::sortLinks(FileTreeDirectory * dir, uint d){
    for(uint i = dir->start; i < dir->end && dir->depth < d; ++i){
        qSort(treeNodes[i].links.begin(), treeNodes[i].links.end());
        /*
        Because qSort will leave any links with equal probability in an arbitrary order,
        we must sort those equal links based on their parents to ensure to most likely link is always selected.
        */
        for(int j = 0; j < treeNodes[i].links.size() && treeNodes[i].links.size() > 1; ++j){
            for(int k = j + 1; k < treeNodes[i].links.size(); ++k){
                if(treeNodes[i].links.at(j).probability == treeNodes[i].links.at(k).probability){
                    if(compareParents(&*treeNodes[i].links[k].r, &*treeNodes[i].links[j].r, &treeNodes[i])){
                        FileLink temp = treeNodes[i].links[j];
                        treeNodes[i].links[j] = treeNodes[i].links[k];
                        treeNodes[i].links[k] = temp;
                        ++j;
                        break;
                    }
                }
            }
        }
        if(treeNodes[i].fileInfo.isDir())
            sortLinks(treeNodes[i].directory, d);
    }
}

/***********************************************************************
    Recursively match directories against each other.
    Matching will only occur where two directories have not previously been matched against each other.
*/
void SmartMatch::performMatching(FileTreeDirectory * lDir, FileTreeDirectory * rDir, uint lDepth, uint rDepth){
    currentL = lDir->node;
    currentR = rDir->node;
    //qDebug()<<"NODES: "<<lDir->node->fileInfo.absoluteFilePath()<<" & "<<rDir->node->fileInfo.absoluteFilePath();
    for(uint i = lDir->start; i < lDir->end && lDir->depth < lDepth; ++i){
        if(!lDir->previouslyMatched.contains(rDir)){
            for(uint j = rDir->start; j < rDir->end && rDir->depth < rDepth; ++j){
                //if(unmatchedLeft.contains(&treeNodes[i]) && unmatchedRight.contains(&treeNodes[j]))
                    match(&treeNodes[i], &treeNodes[j]);
            }
        }
    }
    for(uint j = rDir->start; j < rDir->end && rDir->depth < rDepth; ++j){
        if(treeNodes[j].fileInfo.isDir()){
            performMatching(lDir, treeNodes[j].directory, lDepth, rDepth);
        }
    }
    for(uint i = lDir->start; i < lDir->end && lDir->depth < lDepth; ++i){
        if(treeNodes[i].fileInfo.isDir()){
            performMatching(treeNodes[i].directory, rDir, lDepth, rDepth);
        }
    }
}

/***********************************************************************
    Match two given nodes. If the calculated probabilty is higher than the user passed threshold then a candidate link is created
*/
void SmartMatch::match(FileTreeNode * l, FileTreeNode * r){

    /* Ensure that both nodes are both directories or both files of the same type (or similar type e.g. both audio mp3 vs aac etc) */
    if(l->fileInfo.isDir() == r->fileInfo.isDir() || (!l->fileInfo.isDir() && l->fileType == r->fileType)){

        float matches, probability;
        matches = 0;

        QSet<uint>::iterator itr;
        for(itr = r->reducedHash.begin(); itr != r->reducedHash.end(); ++itr){
            if(l->reducedHash.contains(*itr))
                ++matches;
        }

       /*
       FORMULA:
           ( Number of Matches / ( (L Reduced size + R reduced size) / 2 )
           * ( min(size of original name L or R) / max(size of original name L or R) )
           OPTIONAL * ( min(filesize of L or R) / max(filesize of L or R) )
       */
       probability = matches / (((float)l->reducedHash.size() + (float)r->reducedHash.size()) / 2.0)
               * ((float)qMin(l->fileName.size(), r->fileName.size())/(float)qMax(l->fileName.size(), r->fileName.size()));

       //fileInterface->log("Candidate Match for: "+l->fileName + " -vs- "+ r->fileName + " -- P= " + QString::number(probability));
       //qDebug()<<("Candidate Match for: "+l->fileName + " -vs- "+ r->fileName + " -- P= " + QString::number(probability));;
       /* If the examine file size flag is set we append the match probability */
       if(examineFileSize && !l->fileInfo.isDir())
           probability = probability * ((float)qMin(l->fileInfo.size(), r->fileInfo.size())/(float)qMax(l->fileInfo.size(), r->fileInfo.size()));


       if(probability > threshold){
           //unmatchedRight.remove(r);
           l->insertLink(r, probability);
       }
    }
}

/***********************************************************************
    Recursively finalise the previously calculated matches by removing duplicate matches based on higher probability
*/
void SmartMatch::finaliseMatches(FileTreeDirectory * dir, QHash<FileTreeNode *, FileLink> * fileLinks){
    for(uint i = dir->start; i < dir->end; i++)
        cascadeLink(&treeNodes[i], fileLinks);

    for(uint i = dir->start; i < dir->end; i++)
        if(treeNodes.at(i).fileInfo.isDir())
            finaliseMatches(treeNodes.at(i).directory, fileLinks);
}

/***********************************************************************
    Link right nodes to left match nodes when matching is complete
*/
void SmartMatch::linkRightMatches(FileTreeDirectory * dir){
    for(uint i = dir->start; i < dir->end; ++i){
        if(treeNodes[i].matched){
            fileInterface->insertMatchTableItem(treeNodes[i].links.first().probability,
                                                treeNodes[i].links.first().l->fileInfo.absoluteFilePath(),
                                                treeNodes[i].links.first().r->fileInfo.absoluteFilePath(),
                                                treeNodes[i].links.first().l->fileInfo.suffix(),
                                                treeNodes[i].links.first().r->fileInfo.suffix(),
                                                treeNodes[i].links.first().l->fileInfo.size(),
                                                treeNodes[i].links.first().r->fileInfo.size(),
                                                treeNodes[i].links.first().r->fileInfo.isDir(),
                                                0);
            treeNodes[i].match->match = &treeNodes[i];
            treeNodes[i].match->matched = true;
            unmatchedRight.remove(treeNodes[i].match);
        }
        if(treeNodes[i].fileInfo.isDir())
            linkRightMatches(treeNodes[i].directory);
    }
}

/***********************************************************************
    Inserts a file link to the hash table passed in. Should there already be an entry in the table with lower precendence that link it cascaded to it's next available link.
    Conversely, if the existing link has a high precedence, the current link is discarded and the next one is tried.
*/
void SmartMatch::cascadeLink(FileTreeNode * currentNode, QHash<FileTreeNode *, FileLink> * fileLinks){
    //qDebug()<<"CURRENT NODE: "<<currentNode->fileInfo.absoluteFilePath();
    while(!currentNode->links.isEmpty()){

        /* First determine if there is already a link to the right hand item from some other node */
        if(fileLinks->contains(currentNode->links.first().r)){
            QHash<FileTreeNode *, FileLink>::iterator existingLink;
            existingLink = fileLinks->find(currentNode->links.first().r);

            FileTreeNode * existing = existingLink.value().l;

            /* Check if the probability of the current file matching the right is higher */
            if(currentNode->links.first().probability > existingLink.value().probability){
                fileLinks->insert(currentNode->links.first().r, currentNode->links.first());
                currentNode->matched = true;
                unmatchedLeft.remove(currentNode);
                currentNode->match = currentNode->links.first().r;

                existing->links.removeFirst();
                cascadeLink(existing, fileLinks);
                break;
            }
            /*  Check if the probability of the current file matching is the same as the existing link */
            else if(currentNode->links.first().probability == existingLink.value().probability){
                //qDebug()<<"-------------------------------------------------";
                //qDebug()<<"SAME P FOR: "<<currentNode->fileInfo.absoluteFilePath()<< " -AND- "<<existing->fileInfo.absoluteFilePath() <<" -FOR-"<<existingLink.value().r->fileInfo.absoluteFilePath();
                if(compareParents(currentNode, existing, existingLink.value().r)){
                    //qDebug()<<"SELECTED: "<<currentNode->fileInfo.absoluteFilePath();
                    FileTreeNode * oldNode = existingLink.value().l;

                    fileLinks->insert(currentNode->links.first().r, currentNode->links.first());
                    currentNode->matched = true;
                    unmatchedLeft.remove(currentNode);
                    currentNode->match = currentNode->links.first().r;

                    oldNode->links.removeFirst();
                    cascadeLink(oldNode, fileLinks);
                    break;
                }else{
                    currentNode->links.removeFirst();
                }
            }
            /* The existing link is a better match, remove the currentNode and try the next*/
            else{
                currentNode->links.removeFirst();
            }
        }
        /* else there is currently nothing this item so just insert a new entry in the hash table */
        else{
            currentNode->matched = true;
            unmatchedLeft.remove(currentNode);
            currentNode->match = currentNode->links.first().r;
            currentNode->links.first().r->matched = true;

            fileLinks->insert(currentNode->links.first().r, currentNode->links.first());
            break;
        }
    }
    /* If at this point the list of links is empty for this item, we need to mark it for copying*/
    if(currentNode->links.isEmpty()){
        currentNode->matched = false;
        unmatchedLeft.insert(currentNode);
        currentNode->match = NULL;
    }
}

/***********************************************************************
    When two left hand items are matched to a single right hand item with equal probability we check their parents vs that of the right item.
    The item with the best matching parent is chosen as the new link. If there is no discernable difference all the way to the root nodes then the existing link stays.
*/

bool SmartMatch::compareParents(FileTreeNode * left1, FileTreeNode * left2, FileTreeNode * r){
    //qDebug()<<"COMPARE PARENTS: "<<left1->fileInfo.absoluteFilePath() <<" AND "<<left2->fileInfo.absoluteFilePath()<<" R IS: "<<r->fileInfo.absoluteFilePath();
    /* If we have reached the root node in either L1 or L2 we cant run a comparison so we check if R has also reached the root (implying equivalence) */
    if(left1->parent == NULL && r->parent == NULL && left2->parent != NULL){
        return true;
    }else if(left2->parent == NULL && r->parent == NULL && left1->parent != NULL){
        return false;
    }

    /*
    More logic to prevent unbounded indexing into the match object list.
    In this case, if one of the objects points to the root node, but both R and it's opposite do not, pick the opposite.
    */
    if(left1->parent == NULL && r->parent != NULL && left2->parent != NULL)
        return false;
    if(left2->parent == NULL && r->parent != NULL && left1->parent != NULL)
        return true;

    /* These two files are exactly alike for matching purposes, stick with the current one */
    if(left1->parent == NULL && left2->parent == NULL)
        return false;

    /* Just R has reached the root, stick with what we have */
    if(r->parent == NULL)
        return false;

    float leftMatch1, leftMatch2;
    int matches = 0;

    QSet<uint>::iterator itr;
    /* Find out the probability of left1 matching R*/
    for(itr = left1->reducedHash.begin(); itr!= left1->reducedHash.end(); ++itr){
        if(r->reducedHash.contains(*itr))
            matches++;
    }
    leftMatch1 = matches / (((float)left1->reducedHash.size() + (float)r->reducedHash.size()) / 2.0)
            * ((float)qMin(left1->fileName.size(), r->fileName.size())/(float)qMax(left1->fileName.size(), r->fileName.size()));

    matches = 0;
    /* Find out the probability of left2 matching R*/
    for(itr = left2->reducedHash.begin(); itr!= left2->reducedHash.end(); ++itr){
        if(r->reducedHash.contains(*itr))
            matches++;
    }
    leftMatch2 = matches / (((float)left2->reducedHash.size() + (float)r->reducedHash.size()) / 2.0)
            * ((float)qMin(left2->fileName.size(), r->fileName.size())/(float)qMax(left2->fileName.size(), r->fileName.size()));

    if(leftMatch1>leftMatch2){
        return true;
    }else if(leftMatch1<leftMatch2){
        return false;
    }else{
        return compareParents(left1->parent, left2->parent, r->parent);
    }
}

/***********************************************************************
    This function computes whether a directory is needed or not the GUI file tree listing.
    A directory is only needed if one of it's children is marked for copying.
*/
void SmartMatch::computeGuiTree(){
    if(fileInterface->copyDirection == LeftToRight || fileInterface->copyDirection == BothDirections){
        foreach(FileTreeNode * x, unmatchedLeft){
            //qDebug()<<"UML: " << x->fileInfo.absoluteFilePath();
            if(!x->fileInfo.isDir() || x->directory->required){
                FileTreeNode * p = x->parent;
                while(p != NULL){
                    if(p->directory->inTree == true)
                        break;
                    else{
                        p->directory->inTree = true;
                        p = p->parent;
                    }
                }
            }
        }
    }

    if(fileInterface->copyDirection == RightToLeft || fileInterface->copyDirection == BothDirections){
        foreach(FileTreeNode * x, unmatchedRight){
           // qDebug()<<"UMR: " << x->fileInfo.absoluteFilePath();
            if(!x->fileInfo.isDir() || x->directory->required){
            FileTreeNode * p = x->parent;
                while(p != NULL){
                    if(p->directory->inTree == true)
                        break;
                    else{
                        p->directory->inTree = true;
                        p = p->parent;
                    }
                }
            }
        }
    }
}
/***********************************************************************
    This function gathers the information needed for adding entries to the GUI file tree.
    It also clears the previouslyMatched set to save memory because at this stage it will not be needed again.
*/
void SmartMatch::showGuiTree(FileTreeDirectory * dir){
    dir->previouslyMatched.clear();

    if(dir->inTree && dir->required)
        fileInterface->insertFileTreeItem(true, dir->node, 0, !dir->node->matched);

    for(uint i = dir->start; i < dir->end && dir->required; ++i){
        if(treeNodes.at(i).fileInfo.isDir() && treeNodes.at(i).directory->inTree && dir->required){
            showGuiTree(treeNodes[i].directory);
            fileInterface->popStack();
        }
        else if(!treeNodes.at(i).fileInfo.isDir() && !treeNodes[i].matched)
            fileInterface->insertFileTreeItem(false, &treeNodes[i], 1, !treeNodes[i].matched);
    }

}

/***********************************************************************
    DEBUG: Print the constructed file tree file tree
*/
void SmartMatch::debugPrintTree(FileTreeDirectory * dir){
    QString str = dir->node->fileInfo.fileName();
    str = str.rightJustified(5*(dir->depth)+str.size(), ' ');
    fileInterface->log(str);

    for(uint i = dir->start; i < dir->end; ++i){
        if(treeNodes.at(i).fileInfo.isDir()){
            debugPrintTree(treeNodes.at(i).directory);
            fileInterface->log("");
        }else{
            str = "|-"+treeNodes.at(i).fileName;
            str = str.rightJustified(5*(dir->depth)+str.size(), ' ');
            fileInterface->log(str);
        }
    }

    str = "..................";
    str = str.rightJustified(5*(dir->depth)+str.size(), ' ');
    fileInterface->log(str);
}

void SmartMatch::debugPrintMatches(FileTreeDirectory * dir){
    for(uint i = dir->start; i < dir->end; ++i){
        if(!treeNodes[i].links.isEmpty())
            fileInterface->log("MATCHED: P="+QString::number(treeNodes[i].links.first().probability)+" -- "+treeNodes[i].links.first().l->fileInfo.absoluteFilePath()+" -WITH- "+treeNodes[i].links.first().r->fileInfo.absoluteFilePath());
        if(treeNodes.at(i).fileInfo.isDir()){
            debugPrintMatches(treeNodes.at(i).directory);
        }
    }
    fileInterface->log("");
}

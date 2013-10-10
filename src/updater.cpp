#include "updater.h"

#define treeNodes fileInterface->treeNodes
#define treeDirs fileInterface->treeDirs

#define LRN 1
#define RRN 2
#define LMV 4
#define RMV 8
#define LMD 16
#define RMD 32

Updater::Updater()
{
    //###################### TEMP SETTINGS #####################
    checkSize = false;
    copyHidden = false;
}

void Updater::run(){

    lastSync = fileInterface->getTime();

    fileInterface->selectLeftIDs(&databaseL);
    fileInterface->selectRightIDs(&databaseR);

    initialSetup();
/*
    qDebug()<<"\n---- EXISTING LEFT ----";
    foreach(FileTreeNode * f, existingL)
        qDebug()<<f->fileName;

    qDebug()<<"\n---- EXISTING RIGHT ----";
    foreach(FileTreeNode * f, existingR)
        qDebug()<<f->fileName;

    QHash<quint64, quint64>::iterator debug;
    qDebug()<<"\n---- DATABASE LEFT ----";
    for(debug = databaseL.begin(); debug != databaseL.end(); ++debug)
        qDebug()<<"KEY: "<<debug.key()<<" -- VALUE: "<<debug.value();

    qDebug()<<"\n---- DATABASE RIGHT ----";
    for(debug = databaseR.begin(); debug != databaseR.end(); ++debug)
        qDebug()<<"KEY: "<<debug.key()<<" -- VALUE: "<<debug.value();
*/

    computeRequiredDirectories(&treeDirs[0]);
    computeRequiredDirectories(&treeDirs[rightRootStart]);

    linkTrees(&treeDirs[0]);

    fileInterface->mpOperation("Looking for updates");
    findUpdates(&treeDirs[0], true);
    findUpdates(&treeDirs[rightRootStart], false);

    fileInterface->actions = mkdirActions + otherActions;
    fileInterface->insertUpdateItems();

    emit complete();
}


void Updater::initialSetup(){

    fileInterface->mpOperation("Creating left tree");

    FileTreeNode leftRootNode, rightRootNode;
    FileTreeDirectory leftRootDir, rightRootDir;

    try{
        leftRootNode.fileName = fileInterface->lDir;
        QFile lFile (fileInterface->lDir);
        if(!lFile.exists())
            throw 1;

        leftRootNode.fileInfo = QFileInfo(lFile);
        leftRootNode.parent = NULL;
        leftRootNode.fileId = diskTool.identifyFile(leftRootNode.fileInfo.absoluteFilePath());
        treeNodes<<leftRootNode;

        /* Setting the pointers from node to dir and vice versa */
        leftRootDir.node = &treeNodes[treeNodes.size()-1];
        treeDirs<<leftRootDir;
        treeNodes.last().directory = &treeDirs[treeDirs.size()-1];

        constructTree(&treeDirs[treeDirs.size()-1], true);

        fileInterface->mpOperation("Creating right tree");
        rightRootNode.fileName = fileInterface->rDir;
        QFile rFile (fileInterface->rDir);

        rightRootNode.fileInfo = QFileInfo(rFile);
        if(!lFile.exists())
            throw 2;

        rightRootNode.parent = NULL;
        rightRootNode.fileId = diskTool.identifyFile(rightRootNode.fileInfo.absoluteFilePath());
        treeNodes<<rightRootNode;

        /* Setting the pointers from node to dir and vice versa */
        rightRootDir.node = &treeNodes[treeNodes.size()-1];
        treeDirs<<rightRootDir;
        treeNodes.last().directory = &treeDirs[treeDirs.size()-1];
        rightRootStart = treeDirs.size()-1;
        fileInterface->rightRootStart = rightRootStart;
        constructTree(&treeDirs[treeDirs.size()-1], false);


        treeDirs[0].node->matched = true;
        treeDirs[0].required = true;
        treeDirs[0].node->match = treeDirs[rightRootStart].node;

        treeDirs[rightRootStart].node->matched = true;
        treeDirs[rightRootStart].required = true;
        treeDirs[rightRootStart].node->match = treeDirs[0].node;

    }catch(int i){
        fileInterface->log("ERROR!: Number #" + QString::number(i) + " (Directory does not exist)");
        emit operationFailed();
        this->wait(ULONG_MAX);
        return;

    }catch(std::exception &e){
        fileInterface->log(QString("ERROR! (Left file tree creation): " + QString(e.what())));
        emit operationFailed();
        this->wait(ULONG_MAX);
        return;
    }



}

void Updater::constructTree(FileTreeDirectory * dir, bool left){
    try{
        dir->start = treeNodes.size();

        QFileInfoList info;
        uint count = 0;

        if(copyHidden)
            info = QDir(dir->node->fileInfo.absoluteFilePath()).entryInfoList(fileInterface->fileTypeFilter, QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);
        else
            info = QDir(dir->node->fileInfo.absoluteFilePath()).entryInfoList(fileInterface->fileTypeFilter, QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);

        /* Create each node. Nodes must be either a directory or a file greater than the minimum size. For files, increment the total count for this directory */
        for(int i = 0; i < info.size(); ++i){
            fileInterface->single(info.at(i).absoluteFilePath());
            /* Make sure the item still exists before we add anything */
            if(info.at(i).exists()){
                if(!checkSize || info.at(i).isDir() || (quint64)(info.at(i).size()) > minimumSize){
                    FileTreeNode item;
                    item.parent = dir->node;
                    item.matched = false;
                    item.match = NULL;
                    item.directory = NULL;
                    item.fileInfo = info[i];
                    item.fileName = info[i].fileName();
                    emit fileInterface->single(info.at(i).absoluteFilePath());
                    item.fileId = diskTool.identifyFile(info.at(i).absoluteFilePath());
                    //qDebug()<<"UPDATER -- ID for: "<< item.fileInfo.absoluteFilePath() << " is: "<<item.fileId;

                    if(!info.at(i).isDir()){
                        count++;
                    }

                    treeNodes<<item;
                    dir->subNodes.insert(&treeNodes[treeNodes.size()-1]);

                    if(left)
                        existingL.insert(item.fileId, &treeNodes[treeNodes.size()-1]);
                    else
                        existingR.insert(item.fileId, &treeNodes[treeNodes.size()-1]);
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
                    if(checkSize)
                        newDirectory.node = &treeNodes[dir->start+j];
                    else
                        newDirectory.node = &treeNodes[dir->start+i];

                    treeDirs<<newDirectory;

                    if(checkSize)
                        treeNodes[dir->start+j].directory = &treeDirs[treeDirs.size()-1];
                    else
                        treeNodes[dir->start+i].directory = &treeDirs[treeDirs.size()-1];

                    constructTree(&treeDirs[treeDirs.size()-1], left);
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
        this->wait(ULONG_MAX);
        return;
    }
}

void Updater::computeRequiredDirectories(FileTreeDirectory * dir){

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
    Once the trees have been created we can link items based on the matches which were previously in the database
*/
void Updater::linkTrees(FileTreeDirectory * dir){
    for(uint i = dir->start; i < dir->end; ++i){
        if(databaseL.contains(treeNodes[i].fileId) && existingR.contains(databaseL.value(treeNodes[i].fileId))){
            FileTreeNode * l, * r;
            l = &treeNodes[i];
            r = existingR.value(databaseL.value(treeNodes[i].fileId));

            l->match = r;
            r->match = l;

            l->matched = true;
            r->matched = true;
        }
        if(treeNodes[i].fileInfo.isDir())
            linkTrees(treeNodes[i].directory);
    }
}

/***********************************************************************
    These functions iterate through a directory tree to find if there are update actions or copying required.
*/
void Updater::findUpdates(FileTreeDirectory * dir, bool left){
    for(uint i = dir->start; i < dir->end; ++i){
        fileInterface->single(treeNodes[i].fileInfo.absoluteFilePath());
        if(treeNodes[i].matched == false){
            if(left){
                if(databaseL.contains(treeNodes[i].fileId)){
                    //qDebug()<<"Database contains key: "<<treeNodes[i].fileId<<" for item: "<<treeNodes[i].fileInfo.absoluteFilePath();
                    if(!existingR.contains(databaseL.value(treeNodes[i].fileId))){
                        qDebug()<<"Matching ID for this was: "<<databaseL.value(treeNodes[i].fileId);
                        qDebug()<<"DELETE: "<<treeNodes[i].fileName;
                        //DELETE
                    }
                }else if(treeNodes[i].fileInfo.isFile() || treeNodes[i].directory->required){
                    qDebug()<<"Database DOESNT contain key: "<<treeNodes[i].fileId<<" for item: "<<treeNodes[i].fileInfo.absoluteFilePath();

                    qDebug()<<"COPY: "<<treeNodes[i].fileName;
                    //COPY
                    ACTION  act;
                    act.src = &treeNodes[i];
                    act.dst = NULL;
                    if(treeNodes[i].fileInfo.isFile())
                        act.op = CPY;
                    else
                        act.op = MKDIR;

                    if(left)
                        act.d = LeftToRight;
                    else
                        act.d = RightToLeft;

                    if(treeNodes[i].fileInfo.isDir()){
                        mkdirActions.append(act);
                        findUpdates(treeNodes[i].directory, left);
                    }else{
                        otherActions.append(act);
                    }
                }
            }else{
                if(databaseR.contains(treeNodes[i].fileId)){
                    if(!existingL.contains(databaseR.value(treeNodes[i].fileId))){
                        qDebug()<<"DELETE: "<<treeNodes[i].fileName;
                        //DELETE
                    }
                }else if(treeNodes[i].fileInfo.isFile() || treeNodes[i].directory->required){
                        qDebug()<<"COPY: "<<treeNodes[i].fileName;
                        ACTION  act;
                        act.src = &treeNodes[i];
                        act.dst = NULL;

                        if(treeNodes[i].fileInfo.isFile())
                            act.op = CPY;
                        else
                            act.op = MKDIR;

                        if(left)
                            act.d = LeftToRight;
                        else
                            act.d = RightToLeft;

                        if(treeNodes[i].fileInfo.isDir()){
                            mkdirActions.append(act);
                            findUpdates(treeNodes[i].directory, left);
                        }else{
                            otherActions.append(act);
                        }
                }
            }
        }else if(left){
            checkDifferences(&treeNodes[i], treeNodes[i].match);
        }

        if(treeNodes[i].fileInfo.isDir())
            findUpdates(treeNodes[i].directory, left);
    }
}

/***********************************************************************
    This function generates actions for specific cases for how the link between two files has changed.
*/
void Updater::checkDifferences(FileTreeNode * l , FileTreeNode * r){

    uint modification = 0;
    /* Check for renames if appropriate */
    if(fileInterface->rename){
        if(l->fileName != fileInterface->getFileName(l->fileId, true))
            modification = modification | LRN;

        if(r->fileName != fileInterface->getFileName(r->fileId, false))
            modification = modification | RRN;
    }

    /* Check for movement if appropriate */
    if(fileInterface->move){
        if(l->parent != NULL && l->parent->fileId != fileInterface->getParentId(l->fileId, true))
            modification = modification | LMV;

        if(r->parent != NULL && r->parent->fileId != fileInterface->getParentId(r->fileId, false))
            modification = modification | RMV;

    }

    /* Check for modification */
    if(l->fileInfo.lastModified().toTime_t() > lastSync)
        modification = modification | LMD;

    if(r->fileInfo.lastModified().toTime_t() > lastSync)
        modification = modification | RMD;

    if(modification == 0)
        return;

    /* And now to painstakingly code 64 case statements... */
    switch(modification){

    case(LRN):{
        rename(l, r, LeftToRight);
        break;
    }

    case(RRN):{
        rename(r, l, RightToLeft);
        break;
    }

    case(LRN | RRN):{
        if(l->fileInfo.lastRead() >= r->fileInfo.lastRead())
            rename(l, r, LeftToRight);
        else
            rename(r, l, RightToLeft);
        break;
    }

    case(LMV):{
        move(l, r, LeftToRight);
        break;
    }

    case(LRN | LMV):{
        rename(l, r, LeftToRight);
        move(l, r, LeftToRight);
        break;
    }

    case(RRN | LMV):{
        rename(r, l, RightToLeft);
        move(l, r, LeftToRight);
        break;
    }

    case(LRN | RRN | LMV):{
        if(l->fileInfo.lastRead() >= r->fileInfo.lastRead())
            rename(l, r, LeftToRight);
        else
            rename(r, l, RightToLeft);
        move(l, r, LeftToRight);
        break;
    }

    case(RMV):{
        move(r, l, RightToLeft);
        break;
    }

    case(LRN | RMV):{
        rename(l, r, LeftToRight);
        move(r, l, RightToLeft);
        break;
    }

    case(RRN | RMV):{
        rename(r, l, RightToLeft);
        move(r, l, RightToLeft);
        break;
    }

    case(LRN | RRN | RMV):{
        if(l->fileInfo.lastRead() >= r->fileInfo.lastRead())
            rename(l, r, LeftToRight);
        else
            rename(r, l, RightToLeft);
        move(r, l, RightToLeft);
        break;
    }

    case(LMV | RMV):{
        if(l->fileInfo.lastRead() >= r->fileInfo.lastRead())
            move(l, r, LeftToRight);
        else
            move(r, l, RightToLeft);
        break;
    }

    case(LRN | LMV | RMV):{
        break;
    }

    case(RRN | LMV | RMV):{
        break;
    }

    case(LRN | RRN | LMV | RMV):{
        break;
    }

    case(LMD):{
        break;
    }

    case(LRN | LMD):{
        break;
    }

    case(RRN | LMD):{
        break;
    }

    case(LRN | RRN | LMD):{
        break;
    }

    case(LMV | LMD):{
        break;
    }

    case(LRN | LMV | LMD):{
        break;
    }

    case(RRN | LMV | LMD):{
        break;
    }

    case(LRN | RRN | LMV | LMD):{
        break;
    }

    case(RMV | LMD):{
        break;
    }

    case(LRN | RMV | LMD):{
        break;
    }

    case(RRN | RMV | LMD):{
        break;
    }

    case(LRN | RRN | RMV | LMD):{
        break;
    }

    case(LMV | RMV | LMD):{
        break;
    }

    case(LRN | LMV | RMV | LMD):{
        break;
    }

    case(RRN | LMV | RMV | LMD):{
        break;
    }

    case(LRN | RRN | LMV | RMV | LMD):{
        break;
    }

    case(RMD):{
        break;
    }

    case(LRN | RMD):{
        break;
    }

    case(RRN | RMD):{
        break;
    }

    case(LRN | RRN | RMD):{
        break;
    }

    case(LMV | RMD):{
        break;
    }

    case(LRN | LMV | RMD):{
        break;
    }

    case(RRN | LMV | RMD):{
        break;
    }

    case(LRN | RRN | LMV | RMD):{
        break;
    }

    case(RMV | RMD):{
        break;
    }

    case(LRN | RMV | RMD):{
        break;
    }

    case(RRN | RMV | RMD):{
        break;
    }

    case(LRN | RRN | RMV | RMD):{
        break;
    }

    case(LMV | RMV | RMD):{
        break;
    }

    case(LRN | LMV | RMV | RMD):{
        break;
    }

    case(RRN | LMV | RMV | RMD):{
        break;
    }

    case(LRN | RRN | LMV | RMV | RMD):{
        break;
    }

    case(LMD | RMD):{
        break;
    }

    case(LRN | LMD | RMD):{
        break;
    }

    case(RRN | LMD | RMD):{
        break;
    }

    case(LRN | RRN | LMD | RMD):{
        break;
    }

    case(LMV | LMD | RMD):{
        break;
    }

    case(LRN | LMV | LMD | RMD):{
        break;
    }

    case(RRN | LMV | LMD | RMD):{
        break;
    }

    case(LRN | RRN | LMV | LMD | RMD):{
        break;
    }

    case(RMV | LMD | RMD):{
        break;
    }

    case(LRN | RMV | LMD | RMD):{
        break;
    }

    case(RRN | RMV | LMD | RMD):{
        break;
    }

    case(LRN | RRN | RMV | LMD | RMD):{
        break;
    }

    case(LMV | RMV | LMD | RMD):{
        break;
    }

    case(LRN | LMV | RMV | LMD | RMD):{
        break;
    }

    case(RRN | LMV | RMV | LMD | RMD):{
        break;
    }

    case(LRN | RRN | LMV | RMV | LMD | RMD):{
        break;
    }

    }//END SWITCH

    //fileInterface->actions.append(act);
    //fileInterface->insertActionTableItem();

}

void Updater::rename(FileTreeNode * src, FileTreeNode * dst, CopyDirection cd){
    ACTION a;
    a.src = src;
    a.dst = dst;
    a.op = RN;
    a.d = cd;
    otherActions.append(a);
}

void Updater::move(FileTreeNode * src, FileTreeNode * dst, CopyDirection cd){
    ACTION a;
    a.src = src;
    a.dst = dst;
    a.op = MV;
    a.d = cd;
    otherActions.append(a);
}

void Updater::update(FileTreeNode * src, FileTreeNode * dst, CopyDirection cd){
    ACTION a;
    a.src = src;
    a.dst = dst;
    a.op = UP;
    a.d = cd;
    otherActions.append(a);
}

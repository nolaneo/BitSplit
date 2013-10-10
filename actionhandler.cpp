//ADD DATABASE INSERTIONS AFTER COPY AND MKDIR

#include "actionhandler.h"

#define GIGABYTE 1073741824.0
#define MEGABYTE 1048576.0
#define KILOBYTE 1024.0

#define treeNodes fileInterface->treeNodes
#define treeDirs fileInterface->treeDirs
#define actions fileInterface->actions

ActionHandler::ActionHandler()
{
    fileCopier.setProgressInterval(100);
    connect(&fileCopier, SIGNAL(done(bool)), this, SLOT(qtCopierComplete(bool)));
    connect(&fileCopier, SIGNAL(dataTransferProgress(int,qint64)), this, SLOT(copierUpdate(int,qint64)));
    connect(&fileCopier, SIGNAL(error(int,QtFileCopier::Error,bool)), this, SLOT(error(int,QtFileCopier::Error,bool)));
}

void ActionHandler::run(){
    try{
        totalSize = 1;
        totalCopied = 0;
        lastCopied = 0;
        guiCalls = 0;
        currentAction = 0;

        /* Calculate total filesize of items to copy */
        foreach(ACTION a, actions){
            if(a.op == CPY && a.src->fileInfo.isFile())
                totalSize += a.src->fileInfo.size();
        }

        fileInterface->log("Bytes to copy: " + QString::number(totalSize));

        getNextAction();

    }catch(std::exception &e){
        fileInterface->log(QString("ERROR! (General Action Handler): " + QString(e.what())));
        emit exception();
        this->wait(ULONG_MAX);
    }catch(...){
        fileInterface->log(QString("ERROR! (Action Handler): "));
        emit exception();
        this->wait(ULONG_MAX);
    }
}

void ActionHandler::getNextAction(){
    if(actions.size() > 0){
        switch(actions.first().op){
        case CPY:
            handleCopy();
            break;

        case MKDIR:
            handleMkDir();
            break;

        case MV:
            handleMove();
            break;

        case RM:
            handleRemove();
            break;

        case RN:
            handleRename();
            break;
        }
    }else{
        emit complete();
    }
}

void ActionHandler::handleCopy(){
    fileInterface->mpOperation("Copying");
    emit updateActionLabel(actions.first().src->fileInfo.absoluteFilePath());
    emit setInProgress(currentAction);
    bytesCopied = 0;
    sizeOfFile = actions.first().src->fileInfo.size();

    QString src, dst;
    src = actions.first().src->fileInfo.absoluteFilePath();
    dst = fileInterface->generateTarget(actions.first().src);

    fileCopier.copy(src, dst, QtFileCopier::Force);
}

void ActionHandler::handleMkDir(){
    fileInterface->mpOperation("Create Directory");
    emit updateActionLabel(actions.first().src->fileName);

    QString  dst;
    dst = fileInterface->generateTarget(actions.first().src);
    QDir().mkdir(dst);
    fileInterface->addNewEntry();
    emit setComplete(currentAction);
    ++currentAction;
    actions.removeFirst();
    getNextAction();
}

/***********************************************************************
    Moves the file represented by Action.dst based on the location of the file Action.src
*/
void ActionHandler::handleMove(){
    if(table->item(currentAction, 0)->checkState() == Qt::Checked){
        emit setInProgress(currentAction);
        fileInterface->mpOperation("Moving");
        emit updateActionLabel(actions.first().dst->fileName);

        QString src, dst;

        src = actions.first().dst->fileInfo.absoluteFilePath();
        dst = actions.first().dst->fileName;

        FileTreeNode * p = actions.first().src->parent;
        while(!p->matched)
            dst.prepend(p->fileName+'/');

        dst.prepend(fileInterface->generateSource(p->match)+'/');
        moveTarget = dst;

        if(actions.first().dst->fileInfo.isDir())
            fileCopier.moveDirectory(src, dst, QtFileCopier::Force);
        else
            fileCopier.move(src, dst, QtFileCopier::Force);

    }else{
         emit setSkipped(currentAction);
         ++currentAction;
         actions.removeFirst();
         getNextAction();
    }
}

/***********************************************************************
    NYI
*/
void ActionHandler::handleRemove(){
    if(table->item(currentAction, 0)->checkState() == Qt::Checked){
        emit setInProgress(currentAction);
        fileInterface->mpOperation("Deleting");
        emit updateActionLabel(actions.first().dst->fileName);
        emit setComplete(currentAction);
    }else{
         emit setSkipped(currentAction);
    }
    ++currentAction;
    actions.removeFirst();
    getNextAction();
}

/***********************************************************************
    Renames the file represented by Action.dst to the name of the file represented by Action.src
*/
void ActionHandler::handleRename(){

    if(table->item(currentAction, 0)->checkState() == Qt::Checked){
        emit setInProgress(currentAction);
        fileInterface->mpOperation("Renaming");
        emit updateActionLabel(actions.first().dst->fileName);
        QString suffix;
        if(actions.first().dst->fileInfo.isDir())
            suffix = "";
        else
            suffix = "." + actions.first().dst->fileInfo.suffix();
        QString target = fileInterface->generateSource(actions.first().dst->parent)+'/'+actions.first().src->fileInfo.baseName()+suffix;

        if(!QDir().rename(actions.first().dst->fileInfo.absoluteFilePath(), target)){
            fileInterface->log("Failed to rename: " + actions.first().dst->fileInfo.absoluteFilePath());
            emit setError(currentAction);
        }else{
            QFileInfo * old = &actions.first().dst->fileInfo;
            QFile file(target);
            QFileInfo * fi = new QFileInfo(file);
            actions.first().dst->fileName = file.fileName();
            actions.first().dst->fileInfo = *fi;
            delete(old);

            fileInterface->updateDatabase();
            emit setComplete(currentAction);
        }

    }else{
        emit setSkipped(currentAction);
    }
    ++currentAction;
    actions.removeFirst();
    getNextAction();
}

/***********************************************************************
    When the QtFileCopier completes either a copy or a move action, the database is updated and the next action is called.
*/
void ActionHandler::qtCopierComplete(bool b){

    if(actions.first().op == CPY){
        totalCopied += sizeOfFile;
        fileInterface->addNewEntry();

    }else if(actions.first().op == MV){
        QFileInfo * old = &actions.first().dst->fileInfo;
        QFile file(moveTarget);
        QFileInfo * fi = new QFileInfo(file);
        actions.first().dst->fileName = file.fileName();
        actions.first().dst->fileInfo = *fi;
        delete(old);
        fileInterface->updateDatabase();
    }

    emit setComplete(currentAction);
    ++currentAction;
    actions.removeFirst();
    getNextAction();
}

/***********************************************************************
    The copier will signal an update every 100ms. When this happens the GUI is updated (progress bar, speed, percentage complete)
*/
void ActionHandler::copierUpdate(int id, qint64 bytes){
    bytesCopied = bytes;

    if(fileCopier.state() == QtFileCopier::Busy){
        guiCalls++;
        float progress = float(totalCopied + bytesCopied)/float(totalSize);
        emit updateProgress((int)(progress*100));
        emit updatePercentageLabel(QString::number(qFloor(progress*100)) + '%');

        float speed = (float)(totalCopied + bytesCopied)/((float)guiCalls/10.0);
        QString currentSpeed;

        if(speed >= GIGABYTE)
            currentSpeed = ( QString::number(speed/GIGABYTE, 'f', 1) + " GB/s");
        else if(speed >= MEGABYTE)
            currentSpeed = ( QString::number(speed/MEGABYTE, 'f', 1) + " MB/s");
        else if(speed >= KILOBYTE)
            currentSpeed = ( QString::number(speed/KILOBYTE, 'f', 1) + " KB/s");
        else
            currentSpeed = ( QString::number(speed) + " B/s");

        emit updateSpeedLabel(currentSpeed);
    }
}

/***********************************************************************
    If the copier encounters an error, e.g. write protected, FileNotFound etc. That action is skipped and the relevant item is marked as an error on the GUI.
*/
void ActionHandler::error(int id, QtFileCopier::Error e, bool b){
    if(e != QtFileCopier::NoError){
        fileInterface->log("Could not copy or move file: " + actions.first().src->fileInfo.absoluteFilePath() + " ERROR: " + e);
        fileCopier.skip();

        emit setError(currentAction);
        currentAction++;
        actions.removeFirst();
        getNextAction();
    }
}

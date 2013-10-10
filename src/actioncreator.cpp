#include "actioncreator.h"

#define treeNodes fileInterface->treeNodes
#define treeDirs fileInterface->treeDirs

ActionCreator::ActionCreator()
{
}

void ActionCreator::run(){

    try{
        fileInterface->mpOperation("Creating\nSync\nOperations");
        fileInterface->mpBottom("");
        fileInterface->addDatabaseEntry(&treeNodes[0], LeftToRight); //Add root entry to database.
        calculateActions(&treeDirs[0], LeftToRight, false);
        calculateActions(&treeDirs[fileInterface->rightRootStart], RightToLeft, true);
        qDebug()<<"INSIDE CREATOR ACTION SIZE: "<<fileInterface->actions.size();

        foreach(ACTION a, fileInterface->actions){
            qDebug()<<"ACTION: "<<a.src->fileName<< " OP: "<<a.op;
        }

        emit complete();

    }catch(std::exception &e){
        fileInterface->log(QString("ERROR! (Action Creation): " + QString(e.what())));
        emit operationFailed();
        this->wait(ULONG_MAX);
    }catch(...){
        fileInterface->log(QString("ERROR! (Action Creation)"));
        emit operationFailed();
        this->wait(ULONG_MAX);
    }
}

void ActionCreator::calculateActions(FileTreeDirectory * dir, CopyDirection cd, bool moveRenameAdd){

    for(uint i = dir->start; i < dir->end; ++i){
        fileInterface->single(treeNodes[i].fileName);
        if(treeNodes.at(i).matched){
            /* This item was matched, check for move or rename actions if they're selected (Only for items which are not the root nodes) */
                if(moveRenameAdd && fileInterface->rename && treeNodes[i].fileInfo.baseName() != treeNodes[i].match->fileInfo.baseName() && treeNodes[i].parent != NULL){
                    ACTION act;
                    act.src = &treeNodes[i];
                    act.dst = treeNodes[i].match;
                    qDebug()<<"MATCH FOR: "<<treeNodes[i].fileInfo.absoluteFilePath()<<" WAS "<<treeNodes[i].match->fileInfo.absoluteFilePath();
                    act.op = RN;
                    act.d = cd;
                    fileInterface->actions.append(act);
                    fileInterface->insertActionTableItem();

                }

                if(moveRenameAdd && fileInterface->move){
                    if(treeNodes[i].parent->match != treeNodes[i].match->parent){
                        ACTION act;
                        act.src = &treeNodes[i];
                        act.dst = treeNodes[i].match;
                        act.op = MV;
                        act.d = cd;
                        fileInterface->actions.append(act);
                        fileInterface->insertActionTableItem();

                    }
                }

                if(!fileInterface->addDatabaseEntry(&treeNodes[i], cd))
                    emit operationFailed();


                if(treeNodes[i].fileInfo.isDir() && treeNodes[i].directory->required){
                    calculateActions(treeNodes[i].directory, cd, moveRenameAdd);
                }

        }else if(!treeNodes.at(i).fileInfo.isDir() || treeNodes.at(i).directory->required){
            /* This item was not matched, caluclate copy actions*/
            if(fileInterface->nodeToTable.contains(&treeNodes[i]) && fileInterface->nodeToTable.value(&treeNodes[i])->checkState(0) == Qt::Checked){
                ACTION  act;
                act.src = &treeNodes[i];
                act.dst = NULL;
                act.d = cd;

                if(treeNodes[i].fileInfo.isDir() && treeNodes[i].directory->required){
                    act.op = MKDIR;
                    fileInterface->actions.append(act);
                    fileInterface->insertActionTableItem();
                    calculateActions(treeNodes[i].directory, cd, moveRenameAdd);
                }else{
                    act.op = CPY;
                    fileInterface->actions.append(act);
                    fileInterface->insertActionTableItem();
                }
            }
        }
    }
}

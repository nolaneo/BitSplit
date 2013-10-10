#define GIGABYTE 1073741824.0
#define MEGABYTE 1048576.0
#define KILOBYTE 1024.0

#include "fileinterface.h"

FileInterface::FileInterface()
{
    rename = true;
    move = true;
    quickSync = true;

    file = new QIcon(":/images/general/file.png");
    newfile = new QIcon(":/images/general/filenew.png");
    directory = new QIcon(":/images/general/folder16.png");
    newdirectory = new QIcon(":/images/general/folder16new.png");

    renamedirectory = new QIcon(":/images/general/folder16rename.png");
    movedirectory = new QIcon(":/images/general/folder16move.png");
    deletedirectory = new QIcon(":/images/general/folder16delete.png");

    renamefile = new QIcon(":/images/general/filerename.png");
    movefile = new QIcon(":/images/general/filemove.png");
    deletefile = new QIcon(":/images/general/filedelete.png");

    actionNum = -1;

    rowColor = false;
}

/***********************************************************************
    This function handles the inital loading of the SQL database. If the database cannot be opened the fuction returns FALSE.
*/
bool FileInterface::loadDatabase(){
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(qApp->applicationDirPath()+"/data/database/syncDatabase.db");
    if(!db.open()){
        log(db.lastError().text());
        return false;
    }

    QSqlQuery qry;
    qry.prepare("CREATE TABLE IF NOT EXISTS profiles(name VARCHAR(30) PRIMARY KEY, filters VARCHAR(1024), lDir VARCHAR(260), rDir VARCHAR(260), lastSync INTEGER)");
    if(!qry.exec()){
        log("ERROR: Failed to create database profiles table.");
        return false;
    }

    log("Database loaded");
    return true;
}

/***********************************************************************
    This function inserts a new profile into the database. Returns FALSE on error.
*/
bool FileInterface::insertProfile(){
    QSqlQuery qry;
    qry.prepare(QString("INSERT INTO profiles (name, filters, lDir, rDir, lastSync) VALUES(\"[" + profileName + "]\",\"" + QString(fileFilterNames.join("/")) +"\",\"" + lDir + "\",\"" + rDir + "\",0)"));
    if(!qry.exec()){
        log("ERROR: Could not add to master table: " + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
        return false;
    }else{
        qry.prepare(QString("CREATE TABLE " + profileName + " (leftID BIGINT PRIMARY KEY, rightID BIGINT UNIQUE, leftPath VARCHAR(260) UNIQUE, rightPath VARCHAR(260) UNIQUE)"));
        if(!qry.exec()){
            log("ERROR: Could not create table for this profile: " + qry.lastError().databaseText());
            return false;
        }else
            log("Saved profile");
    }

    return true;
}

/***********************************************************************
    This functions loads the six previously synced profiles from the database for display in the GUI.
*/
QList<GUIRECENT> FileInterface::loadRecentProfiles(){
    QList<GUIRECENT> recentProfiles;
    QSqlQuery qry;
    qry.prepare("SELECT name, lDir, rDir, lastSync FROM profiles ORDER BY lastSync DESC");
    if(!qry.exec()){
         log("ERROR: Could not load recent profiles: " + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
    }else{
        for(int i = 0; i < 6; ++i){
            if(qry.next()){
                GUIRECENT recent;
                recent.n = (qry.value(0).toString().remove(0,1));
                recent.n = recent.n.remove(recent.n.size()-1, recent.n.size());
                recent.l = qry.value(1).toString();
                recent.r = qry.value(2).toString();
                QDateTime t;
                t.setTime_t(qry.value(3).toULongLong());
                recent.d = "Last synced: " + t.toString("yyyy/MM/dd - hh:mm");
                recent.ft = loadFilters('[' + recent.n + ']').join(", ");
                recentProfiles.append(recent);

                log("Recent Profile -- NAME: " + recent.n + " LEFT: " + recent.l + " RIGHT: "+ recent.r + " - " + recent.d + " FILTERS: " + recent.ft);

            }else
                break;
        }
    }

    log("Loaded recent profile information");
    return recentProfiles;
}

/***********************************************************************
    This function loads the file filters from the files listed in the database entry for some profile
*/
QStringList FileInterface::loadFilters(QString pName){
    QSqlQuery qry;
    QStringList filters;

    qry.prepare(QString("SELECT filters FROM profiles WHERE name = \"" + pName + "\""));

    if(!qry.exec()){
        log("ERROR: Could not get filters for profile: " + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());

    }else{
        if(qry.next()){
            QStringList filenames = qry.value(0).toString().split('/');
            foreach(QString s, filenames){
                QFile filter(qApp->applicationDirPath()+"/data/filters/"+s);
                if(filter.exists()){
                    if (filter.open(QIODevice::ReadOnly | QIODevice::Text)){
                        QTextStream in(&filter);

                        while (!in.atEnd())
                            filters += in.readLine();

                    }else
                        log("ERROR: Could not open filter file.");

                }else
                    log("ERROR: File filter: " + s + " could not be found.");

            }
        }else
            log("ERROR: There were no filters selected for this profile.");
    }

    log("Loaded filters");
    return filters;
}

/***********************************************************************
    This function inserts a row for the currently loaded profile
*/
bool FileInterface::addDatabaseEntry(FileTreeNode * node, CopyDirection cd){
    if(!quickSync){

        quint64 lId, rId;
        QString lp, rp;
        if(cd == LeftToRight){
            lp = node->fileInfo.absoluteFilePath();
            rp = node->match->fileInfo.absoluteFilePath();

            if(node->fileId == 0)
                node->fileId = diskTool.identifyFile(lp);
            lId = node->fileId;

            if(node->match->fileId == 0)
                node->match->fileId = diskTool.identifyFile(rp);
            rId = node->match->fileId;
        }else{
            rp = node->fileInfo.absoluteFilePath();
            lp = node->match->fileInfo.absoluteFilePath();

            if(node->fileId == 0)
                node->fileId = diskTool.identifyFile(rp);
            rId = node->fileId;

            if(node->match->fileId == 0)
                node->match->fileId = diskTool.identifyFile(lp);
            lId = node->match->fileId;
        }

        QSqlQuery qry;
        qry.prepare("INSERT OR REPLACE INTO [" + profileName + "] (leftID, rightID, leftPath, rightPath) VALUES(" + QString::number(lId) + "," + QString::number(rId) + ",\"" + lp + "\",\"" + rp + "\")" );

        if(!qry.exec()){
            log("ERROR: Could not insert row: " + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
            return false;
        }
    }

    return true;

}

/***********************************************************************
    This function updates/inserts a row in the database after a copy/mkdir action has been performed.
*/

bool FileInterface::addNewEntry(){
    quint64 lId, rId;
    QString lp, rp;


    if(actions.first().d == LeftToRight){
        lp = actions.first().src->fileInfo.absoluteFilePath();
        rp = generateTarget(actions.first().src);
    }else{
        rp = actions.first().src->fileInfo.absoluteFilePath();
        lp = generateTarget(actions.first().src);
    }

    lId = diskTool.identifyFile(lp);
    rId = diskTool.identifyFile(rp);

    QSqlQuery qry;
    qry.prepare("INSERT OR REPLACE INTO [" + profileName + "] (leftID, rightID, leftPath, rightPath) VALUES(" + QString::number(lId)
                + "," + QString::number(rId) + ",\"" + lp +"\",\"" + rp +"\")");

    qDebug()<<("INSERT OR REPLACE INTO [" + profileName + "] (leftID, rightID, leftPath, rightPath) VALUES(" + QString::number(lId)
               + "," + QString::number(rId) + ",\"" + lp +"\",\"" + rp +"\")");

    if(!qry.exec()){
        log("ERROR: Could not insert/replace row for rename: " + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
        return false;
    }
    return true;
}

/***********************************************************************
    This function updates/inserts a row in the database after a rename/move action has been performed
*/

bool FileInterface::updateDatabase(){
    quint64 lId, rId;
    QString lp, rp;
    lId = 0; rId = 0;

    if(actions.first().d == LeftToRight){
        lp = actions.first().src->fileInfo.absoluteFilePath();
        rp = actions.first().src->match->fileInfo.absoluteFilePath();

        if(actions.first().src->fileId == 0)
            actions.first().src->fileId = diskTool.identifyFile(lp);
        lId = actions.first().src->fileId;

        if(actions.first().src->match->fileId == 0)
            actions.first().src->match->fileId = diskTool.identifyFile(rp);
        rId = actions.first().src->match->fileId;
    }else{
        rp = actions.first().src->fileInfo.absoluteFilePath();
        lp = actions.first().src->match->fileInfo.absoluteFilePath();

        if(actions.first().src->fileId == 0)
            actions.first().src->fileId = diskTool.identifyFile(rp);
        rId = actions.first().src->fileId;

        if(actions.first().src->match->fileId == 0)
            actions.first().src->match->fileId = diskTool.identifyFile(lp);
        lId = actions.first().src->match->fileId;
    }

    QSqlQuery qry;
    qry.prepare("INSERT OR REPLACE INTO [" + profileName + "] (leftID, rightID, leftPath, rightPath) VALUES(" + QString::number(lId)
                + "," + QString::number(rId) + ",\"" + lp +"\",\"" + rp +"\")");

    qDebug()<<("INSERT OR REPLACE INTO [" + profileName + "] (leftID, rightID, leftPath, rightPath) VALUES(" + QString::number(lId)
               + "," + QString::number(rId) + ",\"" + lp +"\",\"" + rp +"\")");

    if(!qry.exec()){
        log("ERROR: Could not insert/replace row for rename: " + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
        return false;
    }

    if(actions.first().dst->fileInfo.isDir())
        //recursiveUpdate(actions.first().dst->directory, actions.first().dst->fileInfo.absoluteFilePath()+'/');

    return true;
}
/* NYFI contains bug w/ recursive rename on inital matching */
void FileInterface::recursiveUpdate(FileTreeDirectory * dir, QString path){
    quint64 id;
    QSqlQuery qry;

    QSet< FileTreeNode * >::iterator itr;
    for(itr = dir->subNodes.begin(); itr != dir->subNodes.end(); ++itr){
        FileTreeNode * node = *itr;
        QFileInfo * old = &node->fileInfo;
        QFileInfo * fi = new QFileInfo(QFile(path+node->fileName));
        delete(old); //Deleting old fileInfo
        node->fileInfo = *fi;
        if(node->fileId == 0)
            node->fileId = diskTool.identifyFile(path+node->fileName);
        id = node->fileId;

        QString newpath = path+node->fileName;
        if(actions.first().d == LeftToRight)
            qry.prepare("UPDATE [" + profileName + "] SET rightPath = \"" + newpath + "\" WHERE rightID = \"" + QString::number(id) + "\"");
        else
            qry.prepare("UPDATE [" + profileName + "] SET leftPath = \"" + newpath + "\" WHERE leftID = \"" + QString::number(id) + "\"");

        if(!qry.exec())
            log("ERROR: Could not update row for recursive rename: " + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());

        if(node->fileInfo.isDir())
            recursiveUpdate(node->directory, newpath+'/');

    }
}

/***********************************************************************
    These functions extract the file IDs from the database which are needed to determine file movement, naming, deletion and creation.
*/

void FileInterface::selectLeftIDs(QHash<quint64, quint64> * hash){
    QSqlQuery qry;
    qry.prepare("SELECT leftID, rightID FROM [" + profileName + "]");
    if(!qry.exec()){
        qDebug()<<("SELECT leftID, rightID FROM [" + profileName + "]");
        log("ERROR: Unable to select left ID set." + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
    }else{
        while(qry.next())
            hash->insert(qry.value(0).toULongLong(), qry.value(1).toULongLong());
    }
}

void FileInterface::selectRightIDs(QHash<quint64, quint64> * hash){
    QSqlQuery qry;
    qry.prepare("SELECT leftID, rightID FROM [" + profileName + "]");
    if(!qry.exec()){
        qDebug()<<("SELECT leftID, rightID FROM [" + profileName + "]");
        log("ERROR: Unable to select right ID set." + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
    }else{
        while(qry.next())
            hash->insert(qry.value(1).toULongLong(), qry.value(0).toULongLong());
    }
}
/***********************************************************************
    Get the last sync time for the current profile
*/
uint FileInterface::getTime(){
    QSqlQuery qry;

    qry.prepare("SELECT lastSync FROM profiles WHERE name = \"["+profileName+"]\"");
    if(!qry.exec()){
        log("ERROR: Unable to get last sync time." + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
        return 0;
    }else{
        if(qry.next())
            return qry.value(0).toUInt();
        else{
            log("ERROR: There was no value for last sync time." + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
            return 0;
        }
    }
}

/***********************************************************************
    When a synchronisation is complete, this function is called to update the last sync time
*/

bool FileInterface::updateTime(){
    QSqlQuery qry;
    uint now = QDateTime::currentDateTime().toTime_t();
    qry.prepare("UPDATE profiles SET lastSync = \""+QString::number(now)+"\" WHERE name = \"["+profileName+"]\"");
    if(!qry.exec()){
        log("ERROR: Could not update last sync time." + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
        return false;
    }
    return true;
}

/***********************************************************************
    This function returns the filename associated with some ID
*/
QString FileInterface::getFileName(quint64 id, bool left){
    QSqlQuery qry;

    if(left)
        qry.prepare("SELECT leftPath FROM [" + profileName + "] WHERE leftID = \"" + QString::number(id) +"\"");
    else
        qry.prepare("SELECT rightPath FROM [" + profileName + "] WHERE rightID = \"" + QString::number(id) +"\"");

    if(!qry.exec()){
        log("ERROR: Unable to select filename from database. ID: " + QString::number(id) + " --- " + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
        return "";
    }else if(qry.next()){
        return qry.value(0).toString().split('/').last();
    }else{
        log("ERROR: No filename returned from database for ID: " + QString::number(id) + " --- " + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
        return "";
    }
}
/***********************************************************************
    This function returns the parentID associated with some ID
*/
quint64 FileInterface::getParentId(quint64 id, bool left){
    QSqlQuery qry;

    if(left)
        qry.prepare("SELECT leftPath FROM [" + profileName + "] WHERE leftID = \"" + QString::number(id) +"\"");
    else
        qry.prepare("SELECT rightPath FROM [" + profileName + "] WHERE rightID = \"" + QString::number(id) +"\"");

    if(!qry.exec()){
        log("ERROR: Unable to select filename from database (for parent ID). ID: " + QString::number(id)+ " --- " + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
        return 0;
    }else if(qry.next()){
        int size = qry.value(0).toString().split('/').last().size();
        QString path = qry.value(0).toString().left(qry.value(0).toString().size()-(size+1));

        if(left)
            qry.prepare("SELECT leftID FROM [" + profileName + "] WHERE leftPath = \"" + path +"\"");
        else
            qry.prepare("SELECT rightID FROM [" + profileName + "] WHERE rightPath = \"" + path +"\"");

        if(!qry.exec()){
            log("ERROR: Unable to select parent ID from database."+ qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
            return 0;
        }else{
            if(qry.next())
                return qry.value(0).toULongLong();
            else{
                log("ERROR: There was no parent ID found for this item: " + QString::number(id)+ " --- " + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
                return 0;
            }
        }

    }else{
        log("ERROR: No filename returned from database for ID (for parent ID): " + QString::number(id)+ " --- " + qry.lastError().databaseText() + " -- " + qry.lastError().driverText());
        return 0;
    }
}

/************************************************************************
 ************************************************************************
 ************************************************************************/

void FileInterface::log(QString s){
    qDebug()<<s;
    emit updateLog(s);
}

void FileInterface::single(QString s){
    emit updateSingle(s);
}

void FileInterface::mpTop(QString s){
    emit updateTopMP(s);
}

void FileInterface::mpBottom(QString s){
    emit updateBottomMP(s);
}

void FileInterface::mpOperation(QString s){
    emit updateOperationMP(s);
}

void FileInterface::mpTopBottom(QString t, QString b){
    emit updateTopMP(t);
    emit updateBottomMP(b);
}

void FileInterface::finshedMatch(){
    emit matchComplete();
}

void FileInterface::insertFileTreeItem(bool dir, FileTreeNode * node, uint type, bool chk){
    QTreeWidgetItem * wi = new QTreeWidgetItem();
    wi->setText(0, node->fileName);

    switch(type){
    case 0:
        if(chk)
            wi->setIcon(0, *newdirectory);
        else
            wi->setIcon(0,*directory);
        break;

    case 1:
        wi->setIcon(0,*newfile);
    }
    wi->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
    if(chk){
        QString target = generateTarget(node);
        wi->setText(1, target);
        wi->setFlags(wi->flags()|Qt::ItemIsUserCheckable);
        wi->setCheckState(0,Qt::Checked);
    }

    nodeToTable.insert(node, wi);
    emit updateFileTree( dir, wi );
}

void FileInterface::popStack(){
    emit pop();
}

void FileInterface::treeConstructed(){
    emit expandTree();
}

QString FileInterface::generateSource(FileTreeNode * node){
    QString path;
        path = node->fileName;
        if(node->parent != NULL)
            path.prepend(generateSource(node->parent)+'/');
    return path;
}

QString FileInterface::generateTarget(FileTreeNode * node){
    QString path;
    if(node->matched && node->match != NULL){
        path = node->match->fileName;
        if(node->match->parent != NULL)
            path.prepend(generateSource(node->match->parent)+'/');
    }else{
        path = node->fileName;
        if(node->parent != NULL)
            path.prepend(generateTarget(node->parent)+'/');
    }

    return path;
}

void FileInterface::insertMatchTableItem(float p, QString l, QString r, QString lType, QString rType, quint64 lSize, quint64 rSize, bool dir, uint i){

    QTableWidgetItem * probability = new QTableWidgetItem(QString::number(p), QTableWidgetItem::Type);
    QTableWidgetItem * icon = new QTableWidgetItem("", QTableWidgetItem::Type);
    QTableWidgetItem * left = new QTableWidgetItem(l, QTableWidgetItem::Type);
    QTableWidgetItem * right = new QTableWidgetItem(r, QTableWidgetItem::Type);
    QTableWidgetItem * leftType = new QTableWidgetItem(lType, QTableWidgetItem::Type);
    QTableWidgetItem * rightType = new QTableWidgetItem(rType, QTableWidgetItem::Type);

    QString ls, rs;


    if(!dir){
        if(lSize >= GIGABYTE)
            ls = QString::number((float)lSize/GIGABYTE, 'f', 2) + " GB";
        else if(lSize >= MEGABYTE)
            ls = QString::number((float)lSize/MEGABYTE, 'f', 2) + " MB";
        else if(lSize >= KILOBYTE)
            ls = QString::number((float)lSize/KILOBYTE, 'f', 2) + " KB";
        else
            ls = QString::number(lSize) + " B";


        if(rSize >= GIGABYTE)
            rs = QString::number((float)rSize/GIGABYTE, 'f', 2) + " GB";
        else if(rSize >= MEGABYTE)
            rs = QString::number((float)rSize/MEGABYTE, 'f', 2) + " MB";
        else if(rSize >= KILOBYTE)
            rs = QString::number((float)rSize/KILOBYTE, 'f', 2) + " KB";
        else
            rs = QString::number(rSize) + " B";
    }

    if(dir)
        icon->setIcon(*directory);
    else{
        switch(i){
            case 0:
                icon->setIcon(*file);
                break;
        }
    }

    QTableWidgetItem * leftSize = new QTableWidgetItem(ls, QTableWidgetItem::Type);
    QTableWidgetItem * rightSize = new QTableWidgetItem(rs, QTableWidgetItem::Type);


    QList<QTableWidgetItem *>  * row = new QList<QTableWidgetItem *>();

    row->append(probability);
    row->append(icon);
    row->append(left);
    row->append(leftType);
    row->append(leftSize);
    row->append(right);
    row->append(rightType);
    row->append(rightSize);

    foreach(QTableWidgetItem* item, *row){
        if(rowColor)
            item->setBackgroundColor(QColor(225,225,225));
    }

    rowColor = !rowColor;

    emit insertMatchTableRow(row);
}

void FileInterface::insertActionTableItem(){

    ACTION a;
    if(actionNum >= 0)
        a = actions[actionNum];
    else
        a = actions.back();

    QList<QTableWidgetItem *>  * row = new QList<QTableWidgetItem *>();
    QTableWidgetItem * check = new QTableWidgetItem();
    //check->setCheckState(Qt::Checked);
    QTableWidgetItem * icon = new QTableWidgetItem("", QTableWidgetItem::Type);
    QTableWidgetItem * action;
    QTableWidgetItem * src;
    QTableWidgetItem * dst;

    switch(a.op){
    case CPY:
    {
        check->setCheckState(Qt::Checked);
        check->setFlags(Qt::NoItemFlags);
        icon->setIcon(*newfile);
        action = new QTableWidgetItem("Copy", QTableWidgetItem::Type);
        src = new QTableWidgetItem(generateSource(a.src), QTableWidgetItem::Type);
        dst = new QTableWidgetItem(generateTarget(a.src), QTableWidgetItem::Type);
        break;
    }
    case MKDIR:
    {
        check->setCheckState(Qt::Checked);
        check->setFlags(Qt::NoItemFlags);
        icon->setIcon(*newdirectory);
        action = new QTableWidgetItem("New Directory", QTableWidgetItem::Type);
        src = new QTableWidgetItem(generateSource(a.src), QTableWidgetItem::Type);
        dst = new QTableWidgetItem(generateTarget(a.src), QTableWidgetItem::Type);
        break;
    }
    case RN:
    {
        check->setCheckState(Qt::Checked);
        if(a.src->fileInfo.isDir())
            icon->setIcon(*renamedirectory);
        else
            icon->setIcon(*renamefile);

        action = new QTableWidgetItem("Rename", QTableWidgetItem::Type);
        src = new QTableWidgetItem(generateSource(a.dst), QTableWidgetItem::Type);
        QString suffix;
        if(a.dst->fileInfo.isDir())
            suffix = "";
        else
            suffix = "." + a.dst->fileInfo.suffix();
        dst = new QTableWidgetItem(generateSource(a.dst->parent)+'/'+a.src->fileInfo.baseName()+suffix, QTableWidgetItem::Type);
        break;

    }

    case MV:
    {
        check->setCheckState(Qt::Checked);
        if(a.src->fileInfo.isDir())
            icon->setIcon(*movedirectory);
        else
            icon->setIcon(*movefile);

        action = new QTableWidgetItem("Move", QTableWidgetItem::Type);
        src = new QTableWidgetItem(generateSource(a.dst), QTableWidgetItem::Type);
        QString path = a.dst->fileName;
        FileTreeNode * p = a.src->parent;
        while(!p->matched){
            path.prepend(p->fileName+'/');
            p = p->parent;
        }


        path.prepend(generateSource(p->match)+'/');

        dst = new QTableWidgetItem(path, QTableWidgetItem::Type);
        break;

    }
    }

    row->append(check);
    row->append(icon);
    row->append(action);
    row->append(src);
    row->append(dst);

    foreach(QTableWidgetItem * item, *row){
        if(rowColor)
            item->setBackgroundColor(QColor(225,225,225));
    }

    rowColor = !rowColor;
    emit insertActionTableRow(row);
}

void FileInterface::insertUpdateItems(){
    for(int i = 0; i < actions.size(); ++i){
        actionNum = i;
        insertActionTableItem();
    }
}

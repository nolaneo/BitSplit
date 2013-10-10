#ifndef FILEINTERFACE_H
#define FILEINTERFACE_H

#include <QtCore>
#include <QTreeWidgetItem>
#include <QTableWidgetItem>
#include <QTextStream>
#include <QDateTime>
#include <QSqlRecord>
#include <QSqlQuery>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlDriver>

#include "filetreedirectory.h"
#include "filetreenode.h"
#include "removebutton.h"
#include "disktool.h"

enum CopyDirection{LeftToRight, RightToLeft, BothDirections};
enum Op{CPY, RM, MKDIR, MV, RN, UP};

struct NEWPROFILE{
    QString name;
    QString lDir;
    QString rDir;
    QString fileTypeFilters;
    CopyDirection cd;
    uint depth;
    float threshold;
    uint minSize;
    bool rename;
    bool hidden;
    bool move;
    bool similar;
    bool deletion;
    bool sizeCheck;
};

struct GUIRECENT{
    QString n; //Name
    QString l; //Left
    QString r; //Right
    QString d; //Date
    QString ft;//FileTypes
};

struct ACTION{
    FileTreeNode * src;
    FileTreeNode * dst;
    Op op;
    CopyDirection d;
};
class FileInterface : public QObject
{
    Q_OBJECT
public:
    bool quickSync;

    FileInterface();

    QSqlDatabase db;

    DiskTool diskTool;

    QList< FileTreeNode > treeNodes;
    QList< FileTreeDirectory > treeDirs;

    QString lDir, rDir, profileName;

    QStringList fileFilterNames;
    QStringList fileTypeFilter;

    QIcon * file;
    QIcon * newfile;
    QIcon * directory;
    QIcon * newdirectory;

    QIcon * renamedirectory;
    QIcon * deletedirectory;
    QIcon * movedirectory;

    QIcon * renamefile;
    QIcon * deletefile;
    QIcon * movefile;

    CopyDirection copyDirection;

    bool debug, rowColor, rename, move, deletion;
    QHash <FileTreeNode*, QTreeWidgetItem*> nodeToTable;
    uint rightRootStart;
    QList<ACTION> actions;

    void log(QString);
    void single(QString);
    void mpTop(QString);
    void mpBottom(QString);
    void mpOperation(QString);
    void mpTopBottom(QString, QString);
    void finshedMatch();

    void insertFileTreeItem(bool, FileTreeNode *, uint, bool);
    void insertMatchTableItem(float, QString, QString, QString, QString, quint64, quint64, bool, uint);
    void insertActionTableItem();
    void insertUpdateItems();

    void popStack();
    void treeConstructed();

    /* Database functions */
    bool loadDatabase();
    bool insertProfile();
    QList<GUIRECENT> loadRecentProfiles();
    QStringList loadFilters(QString);
    bool addDatabaseEntry(FileTreeNode *, CopyDirection);
    bool addNewEntry();
    bool updateDatabase();
    void recursiveUpdate(FileTreeDirectory *, QString);
    void selectLeftIDs(QHash<quint64, quint64> *);
    void selectRightIDs(QHash<quint64, quint64> *);
    uint getTime();
    bool updateTime();
    QString getFileName(quint64, bool);
    quint64 getParentId(quint64, bool);


    QString generateSource(FileTreeNode * node);
    QString generateTarget(FileTreeNode * node);

signals:
    void updateLog(QString);
    void updateSingle(QString);
    void updateTopMP(QString);
    void updateBottomMP(QString);
    void updateOperationMP(QString);
    void matchComplete();
    void updateFileTree(bool, QTreeWidgetItem *);
    void pop();
    void expandTree();
    void insertMatchTableRow(QList<QTableWidgetItem *>*);
    void insertActionTableRow(QList<QTableWidgetItem *>*);

private:
    int actionNum;
};

#endif // FILEINTERFACE_H

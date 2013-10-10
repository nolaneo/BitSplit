#ifndef UPDATER_H
#define UPDATER_H

#include<QThread>
#include "fileinterface.h"
#include "disktool.h"

class Updater : public QThread
{
    Q_OBJECT

public:
    Updater();
    FileInterface * fileInterface;
    QStringList fileTypeNames;

    bool checkSize;
    bool copyHidden;
    bool deletion;
    quint64 minimumSize;
    QHash<quint64, quint64> databaseL, databaseR;
    uint lastSync;

protected:
    void run();

signals:
    void operationFailed();
    void complete();

private:
    QList< ACTION > mkdirActions, otherActions;
    QSet<quint64> leftIDs, rightIDs;
    uint rightRootStart;
    DiskTool diskTool;

    void initialSetup();
    void constructTree(FileTreeDirectory *, bool);
    void linkTrees(FileTreeDirectory *);
    void findUpdates(FileTreeDirectory *, bool);
    void checkDifferences(FileTreeNode*, FileTreeNode*);
    void computeRequiredDirectories(FileTreeDirectory *);

    void rename(FileTreeNode *, FileTreeNode *, CopyDirection);
    void move(FileTreeNode *, FileTreeNode *, CopyDirection);
    void update(FileTreeNode *, FileTreeNode *, CopyDirection);

    QHash<quint64, FileTreeNode*> existingL, existingR;

};

#endif // UPDATER_H

#ifndef SMARTMATCH_H
#define SMARTMATCH_H

#include <QtCore>
#include <typeinfo>
#include "fileinterface.h"
#include "disktool.h"


class SmartMatch : public QThread
{
    Q_OBJECT

public:
    SmartMatch();

    FileInterface * fileInterface;
    QRegExp regex, splitRegex;
    float threshold;
    uint depth;
    quint64 minimumSize;
    bool copyHidden, examineFileSize, exactSuffix, isQuickSync, checkSize, constructingTree;
    QList< QStringList > fileTypes;
    QStringList fileTypeNames;

    bool hashVsString;

protected:
    void run();

public slots:
    void updateGuiLabels();

private:

    void initalSetup();
    void constructTree(FileTreeDirectory *, uint, QSet<FileTreeNode*> *);
    void runSmartMatch(FileTreeDirectory *, FileTreeDirectory *);
    void performMatching(FileTreeDirectory *, FileTreeDirectory *, uint, uint);
    void match(FileTreeNode *, FileTreeNode *);
    void finaliseMatches(FileTreeDirectory *, QHash<FileTreeNode*, FileLink> * );
    void cascadeLink(FileTreeNode *, QHash<FileTreeNode*, FileLink> *);
    bool compareParents(FileTreeNode *, FileTreeNode *, FileTreeNode *);
    void sortLinks(FileTreeDirectory *, uint);
    void computeRequiredDirectories(FileTreeDirectory *);

    void linkRightMatches(FileTreeDirectory *);
    void computeGuiTree();
    void showGuiTree(FileTreeDirectory *);

    void debugPrintTree(FileTreeDirectory *);
    void debugPrintMatches(FileTreeDirectory *);

    bool noErrors;
    int count;
    uint dirId;
    uint rightRootStart;
    FileTreeNode * currentL, * currentR;
    QSet< FileTreeNode * > unmatchedLeft, unmatchedRight;
    DiskTool diskTool;

signals:
    void startTime();
    void stopTime();
    void stopTestTime();
    void operationFailed();
    void setTopBarLineStack(int);

};

#endif // SMARTMATCH_H

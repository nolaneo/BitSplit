#ifndef ACTIONHANDLER_H
#define ACTIONHANDLER_H

#include <QThread>
#include "fileinterface.h"
#include "qtfilecopier.h"

class ActionHandler : public QThread
{
    Q_OBJECT
public:
    ActionHandler();
    FileInterface * fileInterface;
    QTableWidget * table;

protected:
    void run();

signals:
    void complete();

    void updateProgress(int);
    void updateActionLabel(QString);
    void updateSpeedLabel(QString);
    void updatePercentageLabel(QString);

    /* Table update icon signals */
    void setInProgress(int);
    void setSkipped(int);
    void setComplete(int);
    void setError(int);

    void exception();


public slots:
    void qtCopierComplete(bool);
    void copierUpdate(int,qint64);
    void error(int,QtFileCopier::Error,bool);

private:
    void getNextAction();
    void handleCopy();
    void handleMkDir();
    void handleMove();
    void handleRename();
    void handleRemove();

    int currentAction;
    quint64 totalSize, totalCopied, average, guiCalls;
    quint64 bytesCopied, sizeOfFile, lastCopied;
    bool noErrors;
    QtFileCopier fileCopier;
    QString moveTarget;
};

#endif // ACTIONHANDLER_H

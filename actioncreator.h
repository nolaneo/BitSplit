#ifndef ACTIONCREATOR_H
#define ACTIONCREATOR_H

#include <QThread>
#include "fileinterface.h"

class ActionCreator : public QThread
{
    Q_OBJECT
public:
    ActionCreator();
    FileInterface * fileInterface;

protected:
    void run();

signals:
    void actionsCreated();
    void operationFailed();
    void complete();

private:
    void calculateActions(FileTreeDirectory *, CopyDirection, bool);

};

#endif // ACTIONCREATOR_H

#ifndef BITSPLIT_H
#define BITSPLIT_H

#include "ui_bitsplit.h"
#include "editsettings.h"
#include "newprofile.h"
#include "fileinterface.h"
#include "smartmatch.h"
#include "actioncreator.h"
#include "actionhandler.h"
#include "updater.h"

#include <QMovie>
#include <QTreeWidgetItem>
#include <QSystemTrayIcon>
#include <QCloseEvent>
#include <QFontDatabase>
#include <QMessageBox>
#include <typeinfo>

class BitSplit : public QMainWindow
{
    Q_OBJECT
public:
    BitSplit();
    QTimer * guiUpdater;
    QTimer * syncAnimator;
    QTime testTimer; 

protected:
    void changeEvent(QEvent *);
    void createTrayIcon();
    virtual void closeEvent(QCloseEvent *);

public slots:

    void toggleTree(QTreeWidgetItem *, int);
    void insertTreeItem(bool, QTreeWidgetItem *);
    void popParent();
    void expandTree();

    void insertMatchTableRow(QList<QTableWidgetItem *>*);
    void insertActionTableRow(QList<QTableWidgetItem *>*);

    void runDeveloperTest();

    void editSettings();
    void newProfile();

    void searchForSync();
    void setTopBarLineStack(int);
    void runUpdater();
    void updaterComplete();
    void runMatcher();
    void matchComplete();
    void createActions();
    void actionsComplete();
    void runActionHandler();
    void actionHandlerComplete();

    void showMain();
    void showLog();
    void endTimer();

    void error();

    void setError(int);
    void setSkip(int);
    void setSync(int);
    void setDone(int);
    void showSync();

private slots:
    void tableMenu(QPoint);
    void trayHide(QSystemTrayIcon::ActivationReason reason);
    void onShowHide();
    void allowExit();

private:
    void setupTable();
    void showRecentProfiles();

    QFontDatabase fontDb;
    QSystemTrayIcon * trayIcon;
    QMenu * tableContext;
    QIcon *  remove;
    QIcon * errorIcon, * done, * skip, * sync1, * sync2, * sync3, * sync4, * sync5, * sync6, * sync7, * sync8;
    QList < QIcon * > syncAnimation;
    Ui::BitSplit ui;
    FileInterface fileInterface;
    SmartMatch smartMatch;
    ActionCreator actionCreator;
    ActionHandler actionHandler;
    Updater updater;
    bool blockToggle, exit;
    QList<QTreeWidgetItem*> stack;
    QList<GUIRECENT> recentProfiles;
    int currentAction, frame;
    bool color;
    bool searchExisting;

};

#endif // BITSPLIT_H

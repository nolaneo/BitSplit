#include "bitsplit.h"

BitSplit::BitSplit()
{
    fontDb.addApplicationFont(":/fonts/OpenSans-Light.ttf");
    fontDb.addApplicationFont(":/fonts/OpenSans-Regular.ttf");
    exit = true;
    blockToggle = false;
    searchExisting = false;

    /* Timer setup*/
    guiUpdater  = new QTimer( this );
    guiUpdater->setInterval(25);

    syncAnimator  = new QTimer( this );
    syncAnimator->setInterval(75);

    /* Interface setup */
    fileInterface.debug = false;
    fileInterface.copyDirection = BothDirections;
    actionCreator.fileInterface = &fileInterface;
    smartMatch.fileInterface = &fileInterface;
    actionHandler.fileInterface = &fileInterface;
    updater.fileInterface = &fileInterface;

    /* Set up icons */
    remove  = new QIcon(":images/general/remove.png");

    done    = new QIcon(":images/table/tick.png");
    errorIcon   = new QIcon(":images/table/error.png");
    skip    = new QIcon(":images/table/skip.png");

    sync1   = new QIcon(":images/table/sync1.png");
    syncAnimation.append(sync1);
    sync2   = new QIcon(":images/table/sync2.png");
    syncAnimation.append(sync2);
    sync3   = new QIcon(":images/table/sync3.png");
    syncAnimation.append(sync3);
    sync4   = new QIcon(":images/table/sync4.png");
    syncAnimation.append(sync4);
    sync5   = new QIcon(":images/table/sync5.png");
    syncAnimation.append(sync5);
    sync6   = new QIcon(":images/table/sync6.png");
    syncAnimation.append(sync6);
    sync7   = new QIcon(":images/table/sync7.png");
    syncAnimation.append(sync7);
    sync8   = new QIcon(":images/table/sync8.png");
    syncAnimation.append(sync8);


    ui.setupUi( this );
    ui.runButton->setDisabled(true);

    /* Connect buttons to their dialogs */
    connect(ui.editSettingsButton,  SIGNAL(clicked()),  this,   SLOT(editSettings()));
    connect(ui.newProfileButton,    SIGNAL(clicked()),  this,   SLOT(newProfile()));


    /* Log Connections*/
    connect(ui.actionView_Log,  SIGNAL(triggered()),        this,       SLOT(showLog()));
    connect(ui.exitLog,         SIGNAL(clicked()),          this,       SLOT(showMain()));
    connect(&fileInterface,     SIGNAL(updateLog(QString)), ui.log,     SLOT(append(QString)));
    connect(ui.errorViewLog,    SIGNAL(clicked()),          this,       SLOT(showLog()));

    /* Top Bar connections*/
    connect(&fileInterface, SIGNAL(matchComplete()),            this,               SLOT(matchComplete()));
    connect(&fileInterface, SIGNAL(updateSingle(QString)),      ui.topBarSingle,    SLOT(setText(QString)));
    connect(&fileInterface, SIGNAL(updateTopMP(QString)),       ui.mp_top,          SLOT(setText(QString)));
    connect(&fileInterface, SIGNAL(updateBottomMP(QString)),    ui.mp_bottom,       SLOT(setText(QString)));
    connect(&fileInterface, SIGNAL(updateOperationMP(QString)), ui.mp_operation,    SLOT(setText(QString)));
    connect(guiUpdater,     SIGNAL(timeout()),                  &smartMatch,        SLOT(updateGuiLabels()));
    connect(&smartMatch,    SIGNAL(startTime()),                guiUpdater,         SLOT(start()));
    connect(&smartMatch,    SIGNAL(stopTime()),                 guiUpdater,         SLOT(stop()));
    connect(&smartMatch,    SIGNAL(setTopBarLineStack(int)),    this,               SLOT(setTopBarLineStack(int)));

    /* Unmatched files tree Connections */
    connect(ui.unmatchedTree,   SIGNAL(itemChanged(QTreeWidgetItem*,int)),  this,   SLOT(toggleTree(QTreeWidgetItem*,int)));
    connect(&fileInterface,     SIGNAL(updateFileTree(bool, QTreeWidgetItem *)), this, SLOT(insertTreeItem(bool, QTreeWidgetItem *)));
    connect(&fileInterface,     SIGNAL(pop()),                              this,   SLOT(popParent()));
    connect(&fileInterface,     SIGNAL(expandTree()), this, SLOT(expandTree()));

    /* Matched Table Connections */
    connect(&fileInterface, SIGNAL(insertMatchTableRow(QList<QTableWidgetItem*>*)), this, SLOT(insertMatchTableRow(QList<QTableWidgetItem*>*)));

    /* Action Table Connections*/
    connect(&fileInterface, SIGNAL(insertActionTableRow(QList<QTableWidgetItem*>*)), this, SLOT(insertActionTableRow(QList<QTableWidgetItem*>*)));

    /* Error connections */
    connect(&smartMatch,    SIGNAL(operationFailed()),  this, SLOT(error()));
    connect(&actionCreator,    SIGNAL(operationFailed()),  this, SLOT(error()));
    connect(&actionHandler,     SIGNAL(exception()), this, SLOT(error()));

    /* Developer Test Connections */
    connect(ui.actionDeveloper_Test, SIGNAL(triggered()), this, SLOT(runDeveloperTest()));
    connect(&smartMatch, SIGNAL(stopTestTime()), this, SLOT(endTimer()));

    /* Action Handler Connections */
    connect(&actionHandler, SIGNAL(updateProgress(int)),            ui.progressBar,         SLOT(setValue(int)));
    connect(&actionHandler, SIGNAL(updateActionLabel(QString)),     ui.actionLabel,         SLOT(setText(QString)));
    connect(&actionHandler, SIGNAL(updateSpeedLabel(QString)),      ui.speedLabel,          SLOT(setText(QString)));
    connect(&actionHandler, SIGNAL(updatePercentageLabel(QString)), ui.mp_busyIndicator,    SLOT(setText(QString)));
    connect(&actionHandler, SIGNAL(setComplete(int)),               this,                   SLOT(setDone(int)));
    connect(&actionHandler, SIGNAL(setSkipped(int)),                this,                   SLOT(setSkip(int)));
    connect(&actionHandler, SIGNAL(setError(int)),                  this,                   SLOT(setError(int)));
    connect(&actionHandler, SIGNAL(setInProgress(int)),             this,                   SLOT(setSync(int)));

    /* Updater Connections */
    connect(&updater,       SIGNAL(complete()),     this,   SLOT(updaterComplete()));

    /* General Connections & Button Stack Connections */
    connect(syncAnimator,           SIGNAL(timeout()),  this,   SLOT(showSync()));
    connect(ui.finishEditButton,    SIGNAL(clicked()),  this,   SLOT(createActions()));
    connect(&actionCreator,         SIGNAL(complete()), this,   SLOT(actionsComplete()));
    connect(ui.runButton,           SIGNAL(clicked()),  this,   SLOT(runActionHandler()));
    connect(ui.searchButton,        SIGNAL(clicked()),  this,   SLOT(searchForSync()));
    connect(&actionHandler,         SIGNAL(complete()), this,   SLOT(actionHandlerComplete()));

    /* General Setup*/
    frame = 0;
    currentAction = -1;
    actionHandler.table = ui.actionsTable;
    ui.mp_ProfileLabel->fontMetrics().width(ui.mp_ProfileLabel->text());
    createTrayIcon();
    setupTable();
    fileInterface.loadDatabase();
    recentProfiles = fileInterface.loadRecentProfiles();
    if(recentProfiles.size() > 0)
        showRecentProfiles();

}

/* General table setup */
void BitSplit::setupTable(){

    ui.actionsTable->setColumnWidth(0, 22);
    ui.actionsTable->setColumnWidth(1, 22);
    ui.actionsTable->setColumnWidth(2, 100);

    ui.matchedTable->setColumnWidth(0, 30);
    ui.matchedTable->setColumnWidth(1, 22);
    ui.matchedTable->setColumnWidth(2, 400);
    ui.matchedTable->setColumnWidth(3, 35);
    ui.matchedTable->setColumnWidth(4, 75);
    ui.matchedTable->setColumnWidth(5, 400);
    ui.matchedTable->setColumnWidth(6, 35);
    ui.matchedTable->setColumnWidth(7, 75);   

    tableContext = new QMenu(ui.matchedTable);
    tableContext->addAction(*remove, "Remove this match");

    ui.matchedTable->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui.matchedTable, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(tableMenu(QPoint)));
}

/***********************************************************************
    This function populates the recently used profiles tabs which are shown at program startup.
*/
void BitSplit::showRecentProfiles(){
    QList<QLabel*> profileNames;
    profileNames.append(ui.recentProfileLabel_1);
    profileNames.append(ui.recentProfileLabel_2);
    profileNames.append(ui.recentProfileLabel_3);
    profileNames.append(ui.recentProfileLabel_4);
    profileNames.append(ui.recentProfileLabel_5);
    profileNames.append(ui.recentProfileLabel_6);
    QList<QLabel*> profileLeft;
    profileLeft.append(ui.recentProfileLeftDir_1);
    profileLeft.append(ui.recentProfileLeftDir_2);
    profileLeft.append(ui.recentProfileLeftDir_3);
    profileLeft.append(ui.recentProfileLeftDir_4);
    profileLeft.append(ui.recentProfileLeftDir_5);
    profileLeft.append(ui.recentProfileLeftDir_6);
    QList<QLabel*> profileRight;
    profileRight.append(ui.recentProfileRightDir_1);
    profileRight.append(ui.recentProfileRightDir_2);
    profileRight.append(ui.recentProfileRightDir_3);
    profileRight.append(ui.recentProfileRightDir_4);
    profileRight.append(ui.recentProfileRightDir_5);
    profileRight.append(ui.recentProfileRightDir_6);
    QList<QLabel*> profileSync;
    profileSync.append(ui.recentProfileLastSynced_1);
    profileSync.append(ui.recentProfileLastSynced_2);
    profileSync.append(ui.recentProfileLastSynced_3);
    profileSync.append(ui.recentProfileLastSynced_4);
    profileSync.append(ui.recentProfileLastSynced_5);
    profileSync.append(ui.recentProfileLastSynced_6);
    QList<QLabel*> profileFilter;
    profileFilter.append(ui.recentProfileTypes_1);
    profileFilter.append(ui.recentProfileTypes_2);
    profileFilter.append(ui.recentProfileTypes_3);
    profileFilter.append(ui.recentProfileTypes_4);
    profileFilter.append(ui.recentProfileTypes_5);
    profileFilter.append(ui.recentProfileTypes_6);
    QList<QStackedWidget*> profileWidget;
    profileWidget.append(ui.recentProfile_1);
    profileWidget.append(ui.recentProfile_2);
    profileWidget.append(ui.recentProfile_3);
    profileWidget.append(ui.recentProfile_4);
    profileWidget.append(ui.recentProfile_5);
    profileWidget.append(ui.recentProfile_6);

    for(int i = 0; i < 6 && i < recentProfiles.size(); ++i){
        profileNames[i]->setText(recentProfiles[i].n);
        profileLeft[i]->setText(recentProfiles[i].l);
        profileRight[i]->setText(recentProfiles[i].r);
        profileSync[i]->setText(recentProfiles[i].d);
        profileFilter[i]->setText(recentProfiles[i].ft);
        profileWidget[i]->setCurrentIndex(0);
    }

    ui.stackedWidget->setCurrentIndex(2);
}

void BitSplit::tableMenu(QPoint p){
    tableContext->exec(ui.matchedTable->viewport()->mapToGlobal(p));
}

/* GUI BUTTON ACTIONS */
void BitSplit::editSettings(){
    EditSettings settingsDialog(this);
    settingsDialog.exec();
}

void BitSplit::newProfile(){
    NewProfile profileDialog(this);
    if(profileDialog.exec() == NewProfile::Accepted){
        fileInterface.profileName = profileDialog.profile->name;
        fileInterface.lDir = profileDialog.profile->lDir;
        fileInterface.rDir = profileDialog.profile->rDir;
        fileInterface.fileFilterNames.append("Video.txt");  //TMP
        fileInterface.fileTypeFilter<<"*.mkv"<<"*.avi"; //TMP
        fileInterface.quickSync = false; //TMP
        ui.stackedWidget->setCurrentIndex(3);
        ui.mp_lDirLabel->setText(profileDialog.profile->lDir);
        ui.mp_rDirLabel->setText(profileDialog.profile->rDir);
        fileInterface.insertProfile();

        ui.buttonStack->setCurrentIndex(3);
    }

}

/* GUI PAGE CHANGE ACTIONS */
void BitSplit::showMain(){
    ui.stackedWidget->setCurrentIndex(3);
}

void BitSplit::showLog(){
    ui.stackedWidget->setCurrentIndex(1);
}


/* DEVELOPER TEST */
void BitSplit::runDeveloperTest(){

    QMessageBox msgBox;
    msgBox.setInformativeText("Run SmartMatch or Updater?");
    msgBox.addButton("SmartMatch", QMessageBox::AcceptRole);
    msgBox.addButton("Updater", QMessageBox::RejectRole);


    fileInterface.fileTypeFilter<<"*.mkv"<<"*.avi";
    fileInterface.debug = true;
    fileInterface.profileName = "Movies";
    fileInterface.lDir = "I:/Movies";
    fileInterface.rDir = "Z:/Movies";
    fileInterface.fileFilterNames.append("Video.txt");
    fileInterface.quickSync = false;
    ui.stackedWidget->setCurrentIndex(3);
    ui.mp_lDirLabel->setText(fileInterface.lDir);
    ui.mp_rDirLabel->setText(fileInterface.rDir);
    fileInterface.insertProfile();

    if(msgBox.exec() == QMessageBox::AcceptRole){
        runMatcher();
    }else{
        ui.stackedWidget->setCurrentIndex(4);
        runUpdater();
    }
}


void BitSplit::searchForSync(){
    if(searchExisting){
        runUpdater();
    }else{
        runMatcher();
    }
}

void BitSplit::runUpdater(){
    fileInterface.actions.clear();
    fileInterface.treeDirs.clear();
    fileInterface.treeNodes.clear();
    ui.topBarStack->setCurrentIndex(1);
    ui.mp_ProfileLabel->setText(fileInterface.profileName);
    QMovie *movie = new QMovie(":/images/busy.gif");
    ui.mp_busyIndicator->setMovie(movie);
    movie->start();

    try{
        updater.start(QThread::Priority(3));
    }catch(std::exception &e){
        fileInterface.log(QString("ERROR! (General Updater): " + QString(e.what())));
        error();
    }
}

void BitSplit::updaterComplete(){
    ui.mp_operation->setText("Complete");
    ui.topBarSingle->setText("");
    ui.mp_busyIndicator->setPixmap(QPixmap(":/images/complete.png"));
    if(fileInterface.actions.size() > 0){
        ui.runButton->setEnabled(true);
    }else{
        ui.topBarSingle->setText("No sync operations required.");
        fileInterface.updateTime();
    }
}

void BitSplit::runMatcher(){
    ui.topBarStack->setCurrentIndex(1);
    ui.mp_ProfileLabel->setText(fileInterface.profileName);
    QMovie *movie = new QMovie(":/images/busy.gif");
    ui.mp_busyIndicator->setMovie(movie);
    movie->start();

    testTimer.start();
    try{
        smartMatch.start(QThread::Priority(3));
    }catch(std::exception &e){
        fileInterface.log(QString("ERROR! (General SmartMatch): " + QString(e.what())));
        error();
    }
}

void BitSplit::matchComplete(){
    ui.topBarLineStack->setCurrentIndex(1);
    ui.mp_busyIndicator->setPixmap(QPixmap(":/images/complete.png"));
    ui.finishEditButton->setEnabled(true);
    ui.buttonStack->setCurrentIndex(1);

}

void BitSplit::createActions(){
    fileInterface.rowColor = false;
    QMovie *movie = new QMovie(":/images/busy.gif");
    ui.mp_busyIndicator->setMovie(movie);
    movie->start();

    ui.buttonStack->setCurrentIndex(0);
    ui.stackedWidget->setCurrentIndex(4);
    try{
        actionCreator.start(QThread::Priority(3));
    }catch(std::exception &e){
        fileInterface.log(QString("ERROR! (General ActionCreator): " + QString(e.what())));
        error();
    }
}

void BitSplit::actionsComplete(){
    ui.mp_operation->setText("Complete");
    ui.topBarSingle->setText("");
    ui.mp_busyIndicator->setPixmap(QPixmap(":/images/complete.png"));
    if(fileInterface.actions.size() > 0){
        ui.runButton->setEnabled(true);
    }else{
        ui.topBarSingle->setText("No sync operations required.");
        fileInterface.updateTime();
    }
}

void BitSplit::runActionHandler(){
    try{
        ui.topBarStack->setCurrentIndex(1);
        ui.topBarLineStack->setCurrentIndex(0);
        actionHandler.start(QThread::Priority(3));
    }catch(std::exception &e){
        fileInterface.log(QString("ERROR! (General Action Handler): " + QString(e.what())));
        error();
    }
}

void BitSplit::actionHandlerComplete(){
    ui.topBarLineStack->setCurrentIndex(1);
    ui.mp_operation->setText("Complete");
    ui.topBarSingle->setText("");
    ui.mp_busyIndicator->clear();
    ui.mp_busyIndicator->setPixmap(QPixmap(":/images/complete.png"));
    ui.runButton->setDisabled(true);
    fileInterface.updateTime();
}

void BitSplit::setTopBarLineStack(int i){
    ui.topBarLineStack->setCurrentIndex(i);
}

/***********************************************************************
    When an item is checked on unchecked in the file tree, this function will set all the children of that item to be the same state.
    If the item was checked, all parent items are also checked.
*/
void BitSplit::toggleTree(QTreeWidgetItem * item, int i){

    for(int i = 0; i < item->childCount() && !blockToggle; ++i){
        if(item->child(i)->flags() == (Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled))
            item->child(i)->setCheckState(0,item->checkState(0));
        if(item->child(i)->childCount()>0)
            toggleTree(item->child(i),0);
    }

    QTreeWidgetItem * p = item->parent();

    blockToggle = true;
    while(p != NULL){
        if(p->flags() == (Qt::ItemIsSelectable|Qt::ItemIsUserCheckable|Qt::ItemIsEnabled) && p->checkState(0) != Qt::Checked && item->checkState(0) == Qt::Checked)
            p->setCheckState(0, Qt::Checked);
        p = p->parent();
    }
    blockToggle = false;
}

/***********************************************************************
    Inserts an item into the unmatched files tree
*/
void BitSplit::insertTreeItem(bool dir, QTreeWidgetItem * wi){
    if(stack.isEmpty())
        ui.unmatchedTree->addTopLevelItem(wi);
    else
        stack.last()->addChild(wi);
    if(dir)
        stack<<wi;

}

void BitSplit::popParent(){
    if(!stack.isEmpty())
        stack.removeLast();
}

void BitSplit::expandTree(){
    ui.unmatchedTree->expandAll();
}

void BitSplit::insertMatchTableRow(QList<QTableWidgetItem *> * row){
    int r = ui.matchedTable->rowCount();
    ui.matchedTable->insertRow(r);
    for(int i = 0; i < row->size(); ++i){
        ui.matchedTable->setItem(r, i, row->at(i));
    }
}

void BitSplit::insertActionTableRow(QList<QTableWidgetItem *> * row){
    int r = ui.actionsTable->rowCount();
    ui.actionsTable->insertRow(r);
    for(int i = 0; i < row->size(); ++i){
        ui.actionsTable->setItem(r, i, row->at(i));
    }
}

void BitSplit::endTimer(){
    fileInterface.log("OPERATION TOOK: "+QString::number(testTimer.elapsed()));
}

void BitSplit::error(){
    actionHandler.wait(ULONG_MAX);
    actionCreator.wait(ULONG_MAX);
    smartMatch.wait(ULONG_MAX);
    updater.wait(ULONG_MAX);
    ui.topBarStack->setCurrentIndex(0);
    ui.stackedWidget->setCurrentIndex(5);
}

/***********************************************************************
    These functions implement the X button being a close to tray button. Closing from the tray will then exit the progam.
*/
void BitSplit::closeEvent(QCloseEvent * event){
    if(exit){
        actionCreator.exit();
        smartMatch.exit();
        trayIcon->hide();
        event->accept();
    }
    else{
        onShowHide();
        event->ignore();
    }
}
void BitSplit::allowExit(){
    exit = true;
    this->close();
}

/***********************************************************************
    This function handles events such as translating the UI and minimisation to tray.
*/
void BitSplit::changeEvent(QEvent * e){
    switch (e->type())
     {
         case QEvent::LanguageChange:

             break;
         case QEvent::WindowStateChange:
             {
                 if (this->windowState() & Qt::WindowMinimized)
                    onShowHide();
                 break;

             }
         default:
             break;
     }

     QMainWindow::changeEvent(e);

}

/***********************************************************************
    This function creates a tray icon with the necessary actions and connects it to the main program.
*/
void BitSplit::createTrayIcon(){

    trayIcon = new QSystemTrayIcon(QIcon(":images/general/icon16.png"), this);

    connect( trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)), this, SLOT( trayHide(QSystemTrayIcon::ActivationReason)) );

    QAction * showHideAction = new QAction("Show/Hide", trayIcon);
    connect(showHideAction, SIGNAL(triggered()), this, SLOT(onShowHide()));

    QAction * exitAction = new QAction("Exit", trayIcon);
    connect(exitAction, SIGNAL(triggered()), this, SLOT(allowExit()));

    QMenu * trayMenu = new QMenu();

    trayMenu->addAction(exitAction);
    trayMenu->addAction(showHideAction);

    trayIcon->setContextMenu( trayMenu );

    trayIcon->show();
}

/***********************************************************************
    These functions lets the user minimise/restore the program from the tray.
*/
void BitSplit::trayHide(QSystemTrayIcon::ActivationReason reason){
    if(reason && reason != QSystemTrayIcon::DoubleClick)
        return;

    onShowHide();
}

void BitSplit::onShowHide(){
    if(this->isVisible()){
        QTimer::singleShot(250, this, SLOT(hide()));
        trayIcon->showMessage("BitSplit", "BitSplit is still running, right click this icon to exit.", QSystemTrayIcon::MessageIcon(1), 3000);
    }else{
        this->activateWindow();
        this->showNormal();
        this->show();
        this->raise();
        this->setFocus();
    }
}

/***********************************************************************
    These functions change the icon in the operations table to match what is happening in the handler.
*/

void BitSplit::setDone(int row){
    syncAnimator->stop();
    QTableWidgetItem * old = ui.actionsTable->itemAt(row, 1);
    QTableWidgetItem * current = new QTableWidgetItem();
    if(row % 2 == 1)
        current->setBackgroundColor(QColor(225,225,225));
    current->setIcon(*done);
    ui.actionsTable->setItem(row, 1, current);
    delete(old);
}

void BitSplit::setError(int row){
    syncAnimator->stop();
    QTableWidgetItem * old = ui.actionsTable->itemAt(row, 1);
    QTableWidgetItem * current = new QTableWidgetItem();
    if(row % 2 == 1)
        current->setBackgroundColor(QColor(225,225,225));
    current->setIcon(*errorIcon);
    ui.actionsTable->setItem(row, 1, current);
    delete(old);
}

void BitSplit::setSkip(int row){
    syncAnimator->stop();
    QTableWidgetItem * old = ui.actionsTable->itemAt(row, 1);
    QTableWidgetItem * current = new QTableWidgetItem();
    if(row % 2 == 1)
        current->setBackgroundColor(QColor(225,225,225));
    current->setIcon(*skip);
    ui.actionsTable->setItem(row, 1, current);
    delete(old);
}

void BitSplit::setSync(int row){
    if(row % 2 == 1)
        color = true;
    else
        color = false;

    currentAction = row;
    syncAnimator->start();
}

void BitSplit::showSync(){
    if(frame>7)
        frame = 0;
    QTableWidgetItem * old = ui.actionsTable->itemAt(currentAction, 1);
    QTableWidgetItem * current = new QTableWidgetItem();
    if(color)
        current->setBackgroundColor(QColor(225,225,225));
    current->setIcon(*syncAnimation[frame]);
    ui.actionsTable->setItem(currentAction, 1, current);
    delete(old);
    frame++;
}

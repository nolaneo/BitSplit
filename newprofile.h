#ifndef NEWPROFILE_H
#define NEWPROFILE_H

#include "ui_newProfile.h"
#include "fileinterface.h"
#include "disktool.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QtCore>

class NewProfile : public QDialog
{
    Q_OBJECT
public:
    NewProfile(QWidget *);
    Ui::NewProfile ui;
    bool validateDirectoryChoices();
    NEWPROFILE * profile;

public slots:
    void nextPage();
    void previousPage();

    /* PAGE 0 VALIDATION */
    void validateProfileName();

    /* PAGE 1 VALIDATION */
    void validateLeftDirectory();
    void validateRightDirectory();

    /* PAGE 2 VALIDATION */
    void validateCheckBox1(bool);
    void validateCheckBox2();

    void accept();

private:
    QFileDialog fileDialog;
    QString lDir, rDir; //CHANGE THIS TO BE A PROFILE PACKAGE
    DiskTool diskTool;
};

#endif // NEWPROFILE_H

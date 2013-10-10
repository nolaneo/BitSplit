#include "newprofile.h"

NewProfile::NewProfile( QWidget  *parent  ) : QDialog( parent )
{
    profile = new NEWPROFILE;
    ui.setupUi(this);
    fileDialog.setDirectory(QDir::rootPath());
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);

    /* "NEXT/DONE" BUTTON CONNECTIONS*/
    connect(ui.p0Next,  SIGNAL(clicked()),   this,   SLOT(nextPage()));
    connect(ui.p1Next,  SIGNAL(clicked()),   this,   SLOT(nextPage()));
    connect(ui.p2Next,  SIGNAL(clicked()),   this,   SLOT(nextPage()));
    connect(ui.p3Next,  SIGNAL(clicked()),   this,   SLOT(nextPage()));
    connect(ui.p4Next,  SIGNAL(clicked()),   this,   SLOT(nextPage()));
    connect(ui.p5Next,  SIGNAL(clicked()),   this,   SLOT(nextPage()));
    connect(ui.p6Done,  SIGNAL(clicked()),   this,   SLOT(accept()));

    /* "BACK/CANCEL" BUTTON CONNECTIONS*/
    connect(ui.p0Cancel,    SIGNAL(clicked()),  this,   SLOT(close()));
    connect(ui.p1Back,      SIGNAL(clicked()),  this,   SLOT(previousPage()));
    connect(ui.p2Back,      SIGNAL(clicked()),  this,   SLOT(previousPage()));
    connect(ui.p3Back,      SIGNAL(clicked()),  this,   SLOT(previousPage()));
    connect(ui.p4Back,      SIGNAL(clicked()),  this,   SLOT(previousPage()));
    connect(ui.p5Back,      SIGNAL(clicked()),  this,   SLOT(previousPage()));
    connect(ui.p6Back,      SIGNAL(clicked()),  this,   SLOT(previousPage()));

    /* PAGE 0 CONNECTIONS*/
    connect(ui.p0NameEdit,  SIGNAL(textChanged(QString)),   this,    SLOT(validateProfileName()));

    /* PAGE 1 CONNECTIONS*/
    connect(ui.selectLeft,      SIGNAL(clicked()),  this,   SLOT(validateLeftDirectory()));
    connect(ui.selectRight,     SIGNAL(clicked()),  this,   SLOT(validateRightDirectory()));

    /* PAGE 2 CONNECTIONS*/
    connect(ui.allFilesCheck,   SIGNAL(clicked(bool)),  this,   SLOT(validateCheckBox1(bool)));
    connect(ui.audioCheck,      SIGNAL(clicked()),      this,   SLOT(validateCheckBox2()));
    connect(ui.officeCheck,     SIGNAL(clicked()),      this,   SLOT(validateCheckBox2()));
    connect(ui.photoCheck,      SIGNAL(clicked()),      this,   SLOT(validateCheckBox2()));
    connect(ui.videoCheck,      SIGNAL(clicked()),      this,   SLOT(validateCheckBox2()));
    connect(ui.customCheck,     SIGNAL(clicked()),      this,   SLOT(validateCheckBox2()));

}

void NewProfile::nextPage(){
    if(ui.stackedWidget->currentIndex() == 4 && ui.oneDir->isChecked() )
        ui.stackedWidget->setCurrentIndex(ui.stackedWidget->currentIndex()+2);
    else
        ui.stackedWidget->setCurrentIndex(ui.stackedWidget->currentIndex()+1);
}

void NewProfile::previousPage(){
    if(ui.stackedWidget->currentIndex() == 6 && ui.oneDir->isChecked() )
        ui.stackedWidget->setCurrentIndex(ui.stackedWidget->currentIndex()-2);
    else
        ui.stackedWidget->setCurrentIndex(ui.stackedWidget->currentIndex()-1);
}

/***********************************************************************
    This slot ensure that the user has input a valid profile name before progress can be made through the wizard.
*/
void NewProfile::validateProfileName(){
    if(ui.p0NameEdit->text() != NULL)
        ui.p0Next->setEnabled(true);
    else
        ui.p0Next->setDisabled(true);
}

/***********************************************************************
    The following slots ensure that the user has selected two directories before progress can be made through the wizard.
*/
void NewProfile::validateLeftDirectory(){
    QString tmp = QString(fileDialog.getExistingDirectory());
    if(tmp != NULL){
        profile->lDir = tmp.replace("\\", "/");;
        ui.selectLeftLabel->setText(profile->lDir);
    }

    if(profile->lDir != NULL && profile->rDir != NULL && validateDirectoryChoices())
        ui.p1Next->setEnabled(true);
    else
        ui.p1Next->setDisabled(true);
}

void NewProfile::validateRightDirectory(){
    QString tmp = QString(fileDialog.getExistingDirectory());
    if(tmp != NULL){
        profile->rDir = tmp.replace("\\", "/");
        ui.selectRightLabel->setText(profile->rDir);
    }

    if(profile->lDir != NULL && profile->rDir != NULL && validateDirectoryChoices())
        ui.p1Next->setEnabled(true);
    else
        ui.p1Next->setDisabled(true);
}

/***********************************************************************
    This function checks different aspects of the directory choices the user makes to ensure they are valid.
    E.g.
        - One directory may not be a subdirectory of the other.
        - If both directories are on the same physical hard disk, a warning will be presented to tell the user that files will not be safe in case of a crash.
*/

bool NewProfile::validateDirectoryChoices(){
    int length;
    if(profile->lDir.size() >= profile->rDir.size())
        length = profile->rDir.size();
    else
        length = profile->lDir.size();

    if(profile->lDir.left(length) == profile->rDir.left(length)){
        QMessageBox::warning(this, "Recursive Error", "Error: You cannot synchronise a directory with itself or one of its subdirectories.\n\nPlease choose a different directory.", "Ok");
        return false;
    }

    QString lDrive, rDrive;
    lDrive = profile->lDir.split('/').first();
    rDrive = profile->rDir.split('/').first();

    if(diskTool.identifyDisk(lDrive) == diskTool.identifyDisk(rDrive)){
        if(lDrive == rDrive){
            QMessageBox::information(this, "File Safety Notice", "The chosen directories both use the same physical hard drive: " + lDrive + "\n\n"
                                     "In the event of a hard drive failure, both the original files and the synchronised files could be lost.\n\n"
                                     "We recommend choosing a separate physical hard drive for important back-ups", "Ok");
        }else{
            QMessageBox::information(this, "File Safety Notice", "Though the chosen directories appear to be on seperate drives\n"
                                     "they are actually on separate partitions (divisions) of the same physical hard drive.\n\n"
                                     "In the event of a hard drive failure, both the original files and the synchronised files could be lost.\n\n"
                                     "We recommend choosing a separate physical hard drive for important back-ups", "Ok");
        }
    }

    return true;
}

/***********************************************************************
    The following two slots ensure that the check boxes on page 2 must have some valid input from the user before progress through the wizard can be made.
    i.e. At least one of the check boxes must be selected & if the "All Files" check box is selected, no other check boxes can be selected.
*/
void NewProfile::validateCheckBox1(bool checked){
    if(checked)
        ui.p2Next->setEnabled(true);

    if(checked && (ui.audioCheck->isChecked() || ui.officeCheck->isChecked()|| ui.photoCheck->isChecked() || ui.videoCheck->isChecked() || ui.customCheck->isChecked())){

        ui.audioCheck->setChecked(false);
        ui.officeCheck->setChecked(false);
        ui.photoCheck->setChecked(false);
        ui.videoCheck->setChecked(false);
        ui.customCheck->setChecked(false);

    }else if(!checked && !(ui.audioCheck->isChecked() || ui.officeCheck->isChecked()|| ui.photoCheck->isChecked() || ui.videoCheck->isChecked() || ui.customCheck->isChecked())){
        ui.p2Next->setDisabled(true);
    }
}

void NewProfile::validateCheckBox2(){
    if(ui.audioCheck->isChecked() || ui.officeCheck->isChecked()|| ui.photoCheck->isChecked() || ui.videoCheck->isChecked() || ui.customCheck->isChecked()){

        ui.p2Next->setEnabled(true);
        ui.allFilesCheck->setChecked(false);

    }else if(!(ui.audioCheck->isChecked() || ui.officeCheck->isChecked()|| ui.photoCheck->isChecked() || ui.videoCheck->isChecked() || ui.customCheck->isChecked())){
        ui.allFilesCheck->setChecked(true);
    }
}

void NewProfile::accept(){
    profile->name = ui.p0NameEdit->text();

    //QString fileTypeFilters;

    if(ui.copyBothCheck->isChecked()){
        profile->cd = BothDirections;
    }else if(ui.copyLRcheck){
        profile->cd = LeftToRight;
    }else{
        profile->cd = RightToLeft;
    }

    if(ui.bothDirs){
        profile->depth = ui.depthSlider->value();
        profile->threshold = ui.thresholdSlider->value()/100;
    }else{
        profile->depth = 1;
        profile->threshold = 0.666;
    }

    if(ui.sizeCheck->isChecked())
        profile->minSize = ui.sizeSpinBox->value() * 1024 * (ui.sizeComboBox->currentIndex()+1);
    else
        profile->minSize = 0;

    if(ui.allowRenameCheck->isChecked())
        profile->rename = true;
    else
        profile->rename = false;

    if(ui.allowHidden->isChecked())
        profile->hidden = true;
    else
        profile->hidden = false;

    if(ui.allowMoveCheck->isChecked())
        profile->move = true;
    else
        profile->move = false;

    if(ui.allowSimilar->isChecked())
        profile->similar = true;
    else
        profile->similar = false;

    if(ui.allowDeletionCheck->isChecked())
        profile->deletion = true;
    else
        profile->deletion = false;

    if(ui.useFileSizeCheck->isChecked())
        profile->sizeCheck = true;
    else
        profile->sizeCheck = false;

    emit accepted();
    this->done(QDialog::Accepted);

}

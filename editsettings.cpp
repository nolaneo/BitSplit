#include "editsettings.h"

EditSettings::EditSettings( QWidget  *parent  ) : QDialog( parent )
{
    ui.setupUi(this);
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui.fileFilterTable->setHorizontalHeaderLabels(QStringList()<<"File Filter"<<"Extensions");
    ui.regexTable->setHorizontalHeaderLabels(QStringList()<<"Regular Expressions");
}

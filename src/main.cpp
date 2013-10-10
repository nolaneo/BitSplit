#include <QApplication>
#include <QFile>
#include "bitsplit.h"
int main(int argc, char *argv[])
{
    qApp->addLibraryPath(qApp->applicationDirPath());
    QApplication app( argc, argv );
    BitSplit bitsplit;

    bitsplit.show();

    return app.exec();
}


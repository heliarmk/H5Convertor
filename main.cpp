#include "cvth5dialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    cvtH5Dialog w;
    w.show();

    return a.exec();
}

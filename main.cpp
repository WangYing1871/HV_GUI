
#include <QApplication>
#include "pwb.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    pwb window;
    window.setGeometry(50,50,window.size().width(),window.size().height());
    window.show();
    return a.exec();
}

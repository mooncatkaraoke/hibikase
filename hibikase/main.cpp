// SPDX-License-Identifier: GPL-2.0-or-later

#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.showMaximized();

    return a.exec();
}

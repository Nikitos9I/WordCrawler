#include "wordcrawler.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WordCrawler w;
    w.show();

    return a.exec();
}

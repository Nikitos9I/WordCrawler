#ifndef WORDCRAWLER_H
#define WORDCRAWLER_H

#include <QMainWindow>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QDir>
#include <QFileSystemWatcher>
#include <QCommonStyle>
#include <QMessageBox>
#include <indexer.h>
#include <QtConcurrent>

namespace Ui {
class WordCrawler;
}

class WordCrawler : public QMainWindow {
    Q_OBJECT

public:
    explicit WordCrawler(QWidget *parent = nullptr);
    ~WordCrawler();

private slots:
    void selectDirectory();
    void selectDirectory1(QString dir);
    void ifDirChanged(const QString & dirName);
    void ifFileChanged(const QString & fileName);
    void showAboutDialog();
    void ifInputFieldStatusChanged();
    void listenStatus(QString value, QString info);
    void ifFound(QString fileName, long long size);
    void notFound(QString message);
    void cancel();
    void doSearch();

private:
    Ui::WordCrawler *ui;

    // Dictionary
    indexer * _indexer;
};

#endif // WORDCRAWLER_H

#ifndef INDEXER_H
#define INDEXER_H

#include <QMainWindow>
#include <QFileInfoList>
#include <QDir>
#include <QDirIterator>
#include <QVector>
#include <QSet>
#include <string>
#include <algorithm>
#include <iostream>
#include <functional>
#include <QDebug>

class indexer : public QMainWindow {
    Q_OBJECT

    struct _file {
        _file(QString const & fileName) {
            this->fileName = fileName;
        }

        _file() {
            this->fileName = "noname_file";
        }

        void setPath(QString const & filePath) {
            this->filePath = filePath;
        }

        void setSize(long long size) {
            this->size = size;
        }

        long long getSize() {
            return size;
        }

        QString getFilePath() {
            return filePath;
        }

        void setTrigrams(QSet<qint64> trigrams) {
            this->trigrams = trigrams;
        }

        QSet<qint64> getTrigrams() {
            return this->trigrams;
        }

    private:
        QString fileName;
        QString filePath;
        long long size;
        QSet<qint64> trigrams;
    };

    QString globalDir;
    bool canceled;
    QVector<_file> encodedFiles;
    QVector<qint64> encodeString(QString input);

    QFileInfoList getFiles();

signals:
    void changeInputFieldStatus();
    void addElementToUi(QString name, long long size);
    void notFound(QString message);
    void sendStatus(QString value, QString info);

public:
    indexer(QString const& globalDir) {
        this->globalDir = globalDir;
    }

    void encodeFile();
    void search(QString input);
    void cancel();
};

#endif // INDEXER_H

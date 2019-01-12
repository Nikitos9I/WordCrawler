#include "indexer.h"

QFileInfoList indexer::getFiles() {
    emit sendStatus("", "Search files for indexing");

    QDirIterator d(globalDir, QDir::Hidden | QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    QFileInfoList files;

    while (d.hasNext()) {
        files.push_back(QFileInfo(d.next()));
    }

    return files;
}

qint64 encodeTrigramValue(char ch1, char ch2, char ch3) {
    return (static_cast<uint32_t>(reinterpret_cast<unsigned char const &>(ch1) << 16) |
                static_cast<uint32_t>(reinterpret_cast<unsigned char const &>(ch2) << 8) |
                static_cast<uint32_t>(reinterpret_cast<unsigned char const &>(ch3)));
}

void indexer::encodeFile() {
    canceled = false;
    int doneFiles = 0;
    QFileInfoList files = getFiles();

    for (QFileInfo fileInfo : files) {
        QFile file(fileInfo.filePath());
        file.open(QIODevice::ReadOnly);

        if (file.isOpen()) {
            char buffer[1024];
            qint64 num;
            QSet<qint64> trigrams;
            bool isBinaryFile = false;

            while ((num = file.read(buffer, 1024)) != 0) {
                for (int i = 2; i < num; ++i) {
                    qint64 currentTrigram = encodeTrigramValue(buffer[i - 2], buffer[i - 1], buffer[i]);
                    trigrams.insert(currentTrigram);
                }

                if (trigrams.size() > 80000) {
                    trigrams.clear();
                    isBinaryFile = true;
                    break;
                }

                if (canceled == true) {
                    sendStatus("", "Indexing canceled");
                    return;
                }
            }

            if (isBinaryFile)
                continue;

            _file myFile(fileInfo.fileName());
            myFile.setPath(fileInfo.filePath());
            myFile.setSize(fileInfo.size());
            myFile.setTrigrams(trigrams);
            encodedFiles.push_back(myFile);
        }

        ++doneFiles;
        emit sendStatus(QString::number(1.0 * doneFiles / files.size() * 100, 'f', 2), "% completed");
    }

    emit sendStatus("100,00", "% completed");
    emit changeInputFieldStatus();
}

QVector<qint64> indexer::encodeString(QString input) {
    QVector<qint64> trigrams;

    for (int i = 2; i < input.length(); ++i) {
        qint64 currentTrigram = encodeTrigramValue(
                input.toStdString().c_str()[i - 2],
                input.toStdString().c_str()[i - 1],
                input.toStdString().c_str()[i]);

        trigrams.push_back(currentTrigram);
    }

    return trigrams;
}

template <typename Container>
bool in_quote(const Container& cont, const QString& s) {
    return std::search(cont.begin(), cont.end(), s.begin(), s.end()) != cont.end();
}

void indexer::search(QString input) {
    emit sendStatus("", "search started");
    canceled = false;
    QVector<qint64> inputStringTrigrams = encodeString(input);
    int goodFiles = 0;

    for (_file myFile : encodedFiles) {
        QSet<qint64> caughtTrigrams;

        for (int o = 0; o < inputStringTrigrams.size(); ++o) {
            qint64 actualTrigram = inputStringTrigrams[o];

            if (myFile.getTrigrams().contains(actualTrigram)) {
                caughtTrigrams.insert(actualTrigram);
            }

            if (canceled == true) {
                sendStatus("", "Searching canceled");
                return;
            }
        }

        bool isGoodFile = caughtTrigrams.size() >= inputStringTrigrams.size();

        if (!isGoodFile)
            continue;

        QFile file(myFile.getFilePath());
        file.open(QIODevice::ReadOnly);

        char buffer[1024];
        qint64 num;
        while ((num = file.read(buffer, 1024)) != 0) {
            if (!in_quote(std::string(buffer), input)) {
                isGoodFile = false;
            }
        }

        if (isGoodFile) {
            ++goodFiles;
            emit addElementToUi(myFile.getFilePath(), myFile.getSize());
        }
    }

    emit sendStatus("", " search ended");
    emit sendStatus(QString::number(goodFiles), " files found");

    if (goodFiles == 0) {
        emit notFound("No one expected file in this directory");
    }
}

void indexer::cancel() {
    canceled = true;
}

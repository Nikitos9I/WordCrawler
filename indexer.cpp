#include "indexer.h"

QFileInfoList indexer::getFiles() {
    emit sendStatus("", "Search files for indexing");

    QDirIterator d(globalDir, QDir::Hidden | QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    QFileInfoList files;

    while (d.hasNext()) {
        files.push_back(QFileInfo(d.next()));

        if (canceled == true) {
            sendStatus("", "Indexing files canceled");
            break;
        }
    }

    return files;
}

qint64 encodeTrigramValue(ushort ch1, ushort ch2, ushort ch3) {
    return (static_cast<qint64>(reinterpret_cast<unsigned char const &>(ch1) << 16) |
                static_cast<qint64>(reinterpret_cast<unsigned char const &>(ch2) << 8) |
                static_cast<qint64>(reinterpret_cast<unsigned char const &>(ch3)));
}

void indexer::encodeFile() {
    canceled = false;
    int doneFiles = 0;
    QFileInfoList files = getFiles();

    for (QFileInfo fileInfo : files) {
        QFile file(fileInfo.filePath());
        file.open(QIODevice::ReadOnly | QIODevice::Text);

        if (file.isOpen()) {
//            char buffer[2048];
            QString buffer;
//            qint64 num;
            QSet<qint64> trigrams;
            bool isBinaryFile = false;
            QTextStream stream(&file);

            while (!stream.atEnd()) {
                buffer += stream.read(2048);
                for (int i = 2; i < buffer.length(); ++i) {
                    qint64 currentTrigram = encodeTrigramValue(buffer[i - 2].unicode(), buffer[i - 1].unicode(), buffer[i].unicode());
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
                buffer = buffer.mid(buffer.length() - 2, 2);
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
                input[i - 2].unicode(),
                input[i - 1].unicode(),
                input[i].unicode());

        trigrams.push_back(currentTrigram);
    }

    return trigrams;
}

template <typename Container>
bool in_quote(const Container& cont, const std::string& s) {
    return std::search(cont.begin(), cont.end(), s.begin(), s.end()) != cont.end();
}

void indexer::search(QString input) {
    emit sendStatus("", "search started");
    canceled = false;
    QVector<qint64> inputStringTrigrams = encodeString(input);
    int goodFiles = 0;

    for (_file myFile : encodedFiles) {
        QSet<qint64> caughtTrigrams;

        bool isGoodFile = true;
        for (int o = 0; o < inputStringTrigrams.size(); ++o) {
            qint64 actualTrigram = inputStringTrigrams[o];

            if (!myFile.getTrigrams().contains(actualTrigram)) {
               isGoodFile = false;
            }

            if (canceled == true) {
                sendStatus("", "Searching canceled");
                return;
            }
        }

        if (!isGoodFile)
            continue;

        QFile file(myFile.getFilePath());
        file.open(QIODevice::ReadOnly);
        QTextStream stream(&file);

        bool isGGGP = false;
        qint64 size = input.size();
        QString buffer = stream.read(size);
        while (!stream.atEnd()) {
            buffer += stream.read(size);
            if (in_quote(buffer.toStdString(), input.toStdString())) {
                isGGGP = true;
            }
            buffer = buffer.mid(buffer.length());
        }

        if (isGGGP) {
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

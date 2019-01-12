#include "wordcrawler.h"
#include "ui_wordcrawler.h"

WordCrawler::WordCrawler(QWidget *parent) : QMainWindow(parent), ui(new Ui::WordCrawler) {
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));

    /** CSS STYLE **/

    QCommonStyle style;
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout_us->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));

//    ui->dirLabel->setFixedSize(75, 22);
    ui->dirLabel->setAlignment(Qt::AlignCenter);

    ui->dirNameLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
//    ui->dirNameLabel->setFixedHeight(22);
    ui->dirNameLabel->setText(QDir::homePath());
    ui->dirNameLabel->setCursor(Qt::IBeamCursor);

    ui->treeWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    ui->lineEdit->setPlaceholderText("Wait awhile...");
    ui->lineEdit->setDisabled(true);

//    ui->chooseDirButton->setFixedSize(100, 20);

//    ui->cancelButton->setFixedSize(200, 20);

    /**  ACTIONS  **/

    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->chooseDirButton, &QAbstractButton::clicked, this, &WordCrawler::selectDirectory);
    connect(ui->actionAbout_us, &QAction::triggered, this, &WordCrawler::showAboutDialog);
    connect(ui->lineEdit, &QLineEdit::returnPressed, this, &WordCrawler::doSearch);
    connect(ui->cancelButton, &QAbstractButton::clicked, this, &WordCrawler::cancel);
}

void WordCrawler::selectDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
                                                    QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    ui->dirNameLabel->setText(dir);
    ui->treeWidget->clear();

    ui->lineEdit->setText("");
    ui->lineEdit->setPlaceholderText("Wait awhile...");
    ui->lineEdit->setDisabled(true);

    QFileSystemWatcher * fsWatcher = new QFileSystemWatcher(this);
    fsWatcher->addPath(dir);

    connect(fsWatcher, &QFileSystemWatcher::fileChanged, this, &WordCrawler::ifFileChanged);
    connect(fsWatcher, &QFileSystemWatcher::directoryChanged, this, &WordCrawler::ifDirChanged);

    _indexer = new indexer(dir);

    connect(_indexer, &indexer::changeInputFieldStatus, this, &WordCrawler::ifInputFieldStatusChanged);
    connect(_indexer, &indexer::sendStatus, this, &WordCrawler::listenStatus);
    connect(_indexer, &indexer::addElementToUi, this, &WordCrawler::ifFound);
    connect(_indexer, &indexer::notFound, this, &WordCrawler::notFound);

    QFuture<void> myIndexer = QtConcurrent::run(_indexer, &indexer::encodeFile);
}

void WordCrawler::selectDirectory1(QString dir) {
    ui->lineEdit->setPlaceholderText("Wait awhile...");
    ui->lineEdit->setDisabled(true);

    QFileSystemWatcher * fsWatcher = new QFileSystemWatcher(this);
    fsWatcher->addPath(dir);

    connect(fsWatcher, &QFileSystemWatcher::fileChanged, this, &WordCrawler::ifDirChanged);
    connect(fsWatcher, &QFileSystemWatcher::directoryChanged, this, &WordCrawler::ifDirChanged);

    _indexer = new indexer(dir);

    connect(_indexer, &indexer::changeInputFieldStatus, this, &WordCrawler::ifInputFieldStatusChanged);
    connect(_indexer, &indexer::sendStatus, this, &WordCrawler::listenStatus);
    connect(_indexer, &indexer::addElementToUi, this, &WordCrawler::ifFound);
    connect(_indexer, &indexer::notFound, this, &WordCrawler::notFound);

    QFuture<void> myIndexer = QtConcurrent::run(_indexer, &indexer::encodeFile);
}

void WordCrawler::ifDirChanged(const QString & dirName) {
    selectDirectory1(dirName);
}

void WordCrawler::ifFileChanged(const QString & fileName) {
    QFileInfo fileInfo(fileName);
    qDebug() << fileInfo.dir().path();
    selectDirectory1(fileInfo.dir().path());
}

void WordCrawler::listenStatus(QString value, QString info) {
    if (value.size() == 0) {
        ui->statusBar->showMessage(info);
        return;
    }

    ui->statusBar->showMessage(value + info);
}

void WordCrawler::cancel() {
    QtConcurrent::run(_indexer, &indexer::cancel);
}

void WordCrawler::notFound(QString message) {
    auto item = new QTreeWidgetItem(ui->treeWidget);
    item->setText(0, message);
    item->setText(1, QString("=("));
    ui->treeWidget->addTopLevelItem(item);
}

void WordCrawler::doSearch() {
    QString input = ui->lineEdit->text();
    ui->treeWidget->clear();

    if (input.length() < 3) {
        notFound("Input filed must be more then two letters");
        return;
    }

    QFuture<void> myIndexer = QtConcurrent::run(_indexer, &indexer::search, input);
}

QPair<long long, QString> makeSizeCorrectly(long long size) {
    if (size < 1024) {
        return QPair<long long, QString>(size, " B");
    }

    if (size < 1024 * 1024) {
        return QPair<long long, QString>(size / 1024, " KB");
    }

    if (size < 1024 * 1024 * 1024) {
        return QPair<long long, QString>(size / (1024 * 1024), " MB");
    }

    return QPair<long long, QString>(size / (1024 * 1024 * 1024), " GB");
}

void WordCrawler::ifFound(QString fileName, long long size) {
    QPair<long long, QString> goodSize = makeSizeCorrectly(size);

    auto item = new QTreeWidgetItem(ui->treeWidget);
    item->setText(0, fileName);
    item->setText(1, QString::number(goodSize.first) + QString(goodSize.second));
    ui->treeWidget->addTopLevelItem(item);
}

void WordCrawler::ifInputFieldStatusChanged() {
    ui->lineEdit->setPlaceholderText("Ready to use...");
    ui->lineEdit->setDisabled(false);
}

void WordCrawler::showAboutDialog() {
    QMessageBox::aboutQt(this);
}

WordCrawler::~WordCrawler() {
    delete ui;
}

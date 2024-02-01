/* Copyright (c) 2020-2023 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "archive_widget.h"

#include "ui_archive_widget.h"

Archive_widget::Archive_widget(QWidget *pParent) : XShortcutsWidget(pParent), ui(new Ui::Archive_widget)
{
    ui->setupUi(this);

    g_pFilterTable = new QSortFilterProxyModel(this);

    ui->comboBoxType->addItem(tr("Tree"));
    ui->comboBoxType->addItem(tr("Table"));

    ui->groupBoxFilter->setEnabled(false);
    ui->comboBoxType->setCurrentIndex(0);

    g_nCurrentFileSize = 0;
    g_bCurrentFileIsRoot = false;
    g_type = CreateViewModelProcess::TYPE_UNKNOWN;
}

void Archive_widget::setFileName(const QString &sFileName, const FW_DEF::OPTIONS &options, const QSet<XBinary::FT> &stAvailableOpenFileTypes)
{
    setData(CreateViewModelProcess::TYPE_FILE, sFileName, options, stAvailableOpenFileTypes);
}

void Archive_widget::setDirectoryName(const QString &sDirectoryName, const FW_DEF::OPTIONS &options, const QSet<XBinary::FT> &stAvailableOpenFileTypes)
{
    setData(CreateViewModelProcess::TYPE_DIRECTORY, sDirectoryName, options, stAvailableOpenFileTypes);
}

void Archive_widget::setData(CreateViewModelProcess::TYPE type, const QString &sName, const FW_DEF::OPTIONS &options, const QSet<XBinary::FT> &stAvailableOpenFileTypes)
{
    g_type = type;
    g_sName = sName;
    g_options = options;

    g_stAvailableOpenFileTypes = stAvailableOpenFileTypes;

    if (!g_stAvailableOpenFileTypes.count()) {
        // TODO more
        // TODO Check mb Archives
        // TODO APJ, JAR, ZIP, IPA
        g_stAvailableOpenFileTypes.insert(XBinary::FT_MSDOS);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_NE);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_LE);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_LX);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_PE);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_ELF);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_DEX);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_MACHO);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_MACHOFAT);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_BMP);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_PNG);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_JPEG);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_GIF);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_TIFF);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_TEXT);
        g_stAvailableOpenFileTypes.insert(XBinary::FT_ANDROIDXML);
    }

    ui->tableViewArchive->setSortingEnabled(false);

    QAbstractItemModel *pOldTreeModel = ui->treeViewArchive->model();
    QStandardItemModel *pNewTreeModel = nullptr;

    QAbstractItemModel *pOldTableModel = g_pFilterTable->sourceModel();
    QStandardItemModel *pNewTableModel = nullptr;

    g_pFilterTable->setSourceModel(0);
    ui->tableViewArchive->setModel(0);

    ui->comboBoxType->setCurrentIndex(0);
    ui->lineEditFilter->clear();

    QSet<XBinary::FT> stFilterFileTypes;

    if (g_options.bFilter) {
        stFilterFileTypes = g_stAvailableOpenFileTypes;
    }

    g_listViewRecords.clear();

    DialogCreateViewModel dialogCreateViewModel(XOptions::getMainWidget(this));  // TODO ProcessDialog
    dialogCreateViewModel.setData(type, sName, &g_listRecords, &pNewTreeModel, &pNewTableModel, stFilterFileTypes, &g_listViewRecords);

    dialogCreateViewModel.exec();

    ui->treeViewArchive->setModel(pNewTreeModel);

    ui->treeViewArchive->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeViewArchive->header()->setSectionResizeMode(1, QHeaderView::Interactive);

    g_pFilterTable->setSourceModel(pNewTableModel);
    ui->tableViewArchive->setModel(g_pFilterTable);

    ui->tableViewArchive->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui->tableViewArchive->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    ui->tableViewArchive->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Interactive);

    deleteOldAbstractModel(&pOldTreeModel);
    deleteOldAbstractModel(&pOldTableModel);

    ui->tableViewArchive->setSortingEnabled(true);
    ui->tableViewArchive->sortByColumn(0, Qt::AscendingOrder);

    ui->tableViewArchive->setColumnWidth(0, 20);  // TODO consts !!!

    ui->treeViewArchive->expand(pNewTreeModel->index(0, 0));

    connect(ui->treeViewArchive->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
            SLOT(onTreeElement_selected(QItemSelection, QItemSelection)));
    connect(ui->tableViewArchive->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
            SLOT(onTableElement_selected(QItemSelection, QItemSelection)));

    if (g_options.bFilter) {
        ui->comboBoxType->setCurrentIndex(1);  // TODO enum !!!

        if (g_listRecords.count()) {
            ui->tableViewArchive->setCurrentIndex(ui->tableViewArchive->model()->index(0, 0));
        }
    }
}

QString Archive_widget::getCurrentRecordFileName()
{
    return g_sCurrentRecordFileName;
}

QList<CreateViewModelProcess::RECORD> Archive_widget::getRecordsByFileType(XBinary::FT fileType)
{
    QList<CreateViewModelProcess::RECORD> listResult;

    qint32 nNumberOfRecords = g_listViewRecords.count();

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        if (XBinary::checkFileType(fileType, g_listViewRecords.at(i).ft)) {
            listResult.append(g_listViewRecords.at(i));
        }
    }

    return listResult;
}

Archive_widget::~Archive_widget()
{
    delete ui;
}

void Archive_widget::on_treeViewArchive_customContextMenuRequested(const QPoint &pos)
{
    QModelIndexList listIndexes = ui->treeViewArchive->selectionModel()->selectedIndexes();

    if (listIndexes.size() > 0) {
        showContext(g_sCurrentRecordFileName, g_bCurrentFileIsRoot, ui->treeViewArchive->viewport()->mapToGlobal(pos));
    }
}

void Archive_widget::on_tableViewArchive_customContextMenuRequested(const QPoint &pos)
{
    QModelIndexList listIndexes = ui->tableViewArchive->selectionModel()->selectedIndexes();

    if (listIndexes.size() > 0) {
        showContext(g_sCurrentRecordFileName, g_bCurrentFileIsRoot, ui->tableViewArchive->viewport()->mapToGlobal(pos));
    }
}

void Archive_widget::showContext(const QString &sRecordFileName, bool bIsRoot, QPoint point)
{
    if (sRecordFileName != "") {
        QMenu contextMenu(this);

        QAction actionOpen(tr("Open"), this);

        if (!g_options.bNoWindowOpen) {
            if (isOpenAvailable(sRecordFileName, bIsRoot)) {
                actionOpen.setShortcut(getShortcuts()->getShortcut(X_ID_ARCHIVE_OPEN));
                connect(&actionOpen, SIGNAL(triggered()), this, SLOT(openRecord()));
                contextMenu.addAction(&actionOpen);
            }
        }

        QAction actionScan(tr("Scan"), this);
        actionScan.setShortcut(getShortcuts()->getShortcut(X_ID_ARCHIVE_SCAN));
        connect(&actionScan, SIGNAL(triggered()), this, SLOT(scanRecord()));
        contextMenu.addAction(&actionScan);

        QAction actionHex(tr("Hex"), this);
        actionHex.setShortcut(getShortcuts()->getShortcut(X_ID_ARCHIVE_HEX));
        connect(&actionHex, SIGNAL(triggered()), this, SLOT(hexRecord()));
        contextMenu.addAction(&actionHex);

        QAction actionStrings(tr("Strings"), this);
        actionStrings.setShortcut(getShortcuts()->getShortcut(X_ID_ARCHIVE_STRINGS));
        connect(&actionStrings, SIGNAL(triggered()), this, SLOT(stringsRecord()));
        contextMenu.addAction(&actionStrings);

        QAction actionEntropy(tr("Entropy"), this);
        actionEntropy.setShortcut(getShortcuts()->getShortcut(X_ID_ARCHIVE_ENTROPY));
        connect(&actionEntropy, SIGNAL(triggered()), this, SLOT(entropyRecord()));
        contextMenu.addAction(&actionEntropy);

        QAction actionHash(tr("Hash"), this);
        actionHash.setShortcut(getShortcuts()->getShortcut(X_ID_ARCHIVE_HASH));
        connect(&actionHash, SIGNAL(triggered()), this, SLOT(hashRecord()));
        contextMenu.addAction(&actionHash);

        QMenu menuCopy(tr("Copy"), this);
        QAction actionCopyFilename(tr("File name"), this);
        actionCopyFilename.setShortcut(getShortcuts()->getShortcut(X_ID_ARCHIVE_COPY_FILENAME));
        connect(&actionCopyFilename, SIGNAL(triggered()), this, SLOT(copyFileName()));
        menuCopy.addAction(&actionCopyFilename);
        contextMenu.addMenu(&menuCopy);

        QAction actionDump(tr("Dump to file"), this);

        if (!bIsRoot) {
            actionDump.setShortcut(getShortcuts()->getShortcut(X_ID_ARCHIVE_DUMPTOFILE));
            connect(&actionDump, SIGNAL(triggered()), this, SLOT(dumpRecord()));
            contextMenu.addAction(&actionDump);
        }

        // contextMenu.addMenu(getShortcuts()->getRowCopyMenu(this, ui->tableView_XXXX));

        contextMenu.exec(point);
    }
}

bool Archive_widget::isOpenAvailable(const QString &sRecordFileName, bool bIsRoot)
{
    bool bResult = false;

    QSet<XBinary::FT> stFileTypes;

    if (bIsRoot || (g_type == CreateViewModelProcess::TYPE_DIRECTORY)) {
        stFileTypes = XFormats::getFileTypes(sRecordFileName, true);
    } else {
        XArchive::RECORD record = XArchive::getArchiveRecord(sRecordFileName, &g_listRecords);

        QByteArray baData = XArchives::decompress(g_sName, &record, true);
        stFileTypes = XFormats::getFileTypes(&baData, true);
    }

    if (XBinary::isFileTypePresent(&stFileTypes, &g_stAvailableOpenFileTypes)) {
        bResult = true;
    }

    return bResult;
}

void Archive_widget::openRecord()
{
    if (!g_options.bNoWindowOpen) {
        if (isOpenAvailable(g_sCurrentRecordFileName, g_bCurrentFileIsRoot)) {
            handleAction(ACTION_OPEN);
        } else {
            handleAction(ACTION_HEX);
        }
    }
    // TODO open for g_options.bNoWindowOpen
}

void Archive_widget::scanRecord()
{
    handleAction(ACTION_SCAN);
}

void Archive_widget::hexRecord()
{
    handleAction(ACTION_HEX);
}

void Archive_widget::stringsRecord()
{
    handleAction(ACTION_STRINGS);
}

void Archive_widget::entropyRecord()
{
    handleAction(ACTION_ENTROPY);
}

void Archive_widget::hashRecord()
{
    handleAction(ACTION_HASH);
}

void Archive_widget::copyFileName()
{
    handleAction(ACTION_COPYFILENAME);
}

void Archive_widget::dumpRecord()
{
    handleAction(ACTION_DUMP);
}

void Archive_widget::handleAction(Archive_widget::ACTION action)
{
    qint64 nSize = g_nCurrentFileSize;
    bool bIsRoot = g_bCurrentFileIsRoot;
    QString sRecordFileName = g_sCurrentRecordFileName;

    if (sRecordFileName != "") {
        if (bIsRoot || (g_type == CreateViewModelProcess::TYPE_DIRECTORY)) {
            if (action == ACTION_OPEN) {
                _handleActionOpenFile(sRecordFileName, sRecordFileName, true);
            } else if (action == ACTION_COPYFILENAME) {
                QGuiApplication::clipboard()->setText(sRecordFileName);
            } else {
                QFile file;

                file.setFileName(sRecordFileName);

                if (XBinary::tryToOpen(&file)) {
                    _handleActionDevice(action, &file);

                    file.close();
                }
            }
        } else {
            XArchive::RECORD record = XArchive::getArchiveRecord(sRecordFileName, &g_listRecords);

            if (action == ACTION_OPEN) {
                QTemporaryFile fileTemp;

                if (fileTemp.open()) {
                    QString sTempFileName = fileTemp.fileName();

                    DialogUnpackFile dialogUnpackFile(XOptions::getMainWidget(this));

                    dialogUnpackFile.setData(g_sName, &record, sTempFileName);

                    // TODO timer
                    if (dialogUnpackFile.exec() == QDialog::Accepted) {
                        _handleActionOpenFile(sTempFileName, record.sFileName, false);
                    }
                }
            } else if (action == ACTION_COPYFILENAME) {
                QGuiApplication::clipboard()->setText(record.sFileName);
            } else if (action == ACTION_DUMP) {
                QString sSaveFileName = QFileInfo(g_sName).absolutePath() + QDir::separator() + QFileInfo(record.sFileName).fileName();

                sSaveFileName = QFileDialog::getSaveFileName(this, tr("Save file"), sSaveFileName, QFileInfo(record.sFileName).completeSuffix());

                if (sSaveFileName != "") {
                    DialogUnpackFile dialogUnpackFile(XOptions::getMainWidget(this));

                    dialogUnpackFile.setData(g_sName, &record, sSaveFileName);

                    // TODO timer
                    if (dialogUnpackFile.exec() != QDialog::Accepted) {
                        QMessageBox::critical(XOptions::getMainWidget(this), tr("Error"), tr("Cannot save file"));
                    }
                }
            } else {
                if (nSize <= XArchive::getCompressBufferSize()) {
                    QByteArray baData = XArchives::decompress(g_sName, &record);

                    QBuffer buffer;

                    buffer.setBuffer(&baData);

                    if (buffer.open(QIODevice::ReadOnly)) {
                        _handleActionDevice(action, &buffer);

                        buffer.close();
                    }
                } else {
                    QTemporaryFile fileTemp;

                    if (fileTemp.open()) {
                        QString sTempFileName = fileTemp.fileName();

                        DialogUnpackFile dialogUnpackFile(this);

                        dialogUnpackFile.setData(g_sName, &record, sTempFileName);

                        if (dialogUnpackFile.exec() == QDialog::Accepted) {
                            QFile file;

                            file.setFileName(sTempFileName);

                            if (file.open(QIODevice::ReadOnly)) {
                                _handleActionDevice(action, &file);

                                file.close();
                            }
                        }
                    }
                }
            }
        }
    }
}

void Archive_widget::_handleActionDevice(Archive_widget::ACTION action, QIODevice *pDevice)
{
    if (action == ACTION_SCAN) {
        DialogNFDScan dialogStaticScan(this);
        dialogStaticScan.setData(pDevice, true, XBinary::FT_UNKNOWN);

        dialogStaticScan.exec();
    } else if (action == ACTION_HEX) {
        XHexView::OPTIONS options = {};

        // TODO options

        DialogHexView dialogHexView(this, pDevice, options, nullptr);  // TODO XINfoDB !!!
        dialogHexView.setGlobal(getShortcuts(), getGlobalOptions());

        dialogHexView.exec();
    } else if (action == ACTION_STRINGS) {
        SearchStringsWidget::OPTIONS stringsOptions = {};
        stringsOptions.bMenu_Hex = true;
        stringsOptions.bMenu_Demangle = true;
        stringsOptions.bAnsi = true;
        stringsOptions.bUTF8 = false;
        stringsOptions.bUnicode = true;
        stringsOptions.bCStrings = false;

        DialogSearchStrings dialogSearchStrings(this);
        dialogSearchStrings.setGlobal(getShortcuts(), getGlobalOptions());
        dialogSearchStrings.setData(pDevice, XBinary::FT_UNKNOWN, stringsOptions, true);

        dialogSearchStrings.exec();
    } else if (action == ACTION_ENTROPY) {
        DialogEntropy dialogEntropy(this);
        dialogEntropy.setGlobal(getShortcuts(), getGlobalOptions());
        dialogEntropy.setData(pDevice);

        dialogEntropy.exec();
    } else if (action == ACTION_HASH) {
        DialogHash dialogHash(this);
        dialogHash.setGlobal(getShortcuts(), getGlobalOptions());
        dialogHash.setData(pDevice, XBinary::FT_UNKNOWN);

        dialogHash.exec();
    }
}

void Archive_widget::_handleActionOpenFile(const QString &sFileName, const QString &sTitle, bool bReadWrite)
{
    QSet<XBinary::FT> stFileTypes = XFormats::getFileTypes(sFileName, true);

    if (stFileTypes.contains(XBinary::FT_PNG) || stFileTypes.contains(XBinary::FT_JPEG) || stFileTypes.contains(XBinary::FT_TIFF) ||
        stFileTypes.contains(XBinary::FT_GIF) || stFileTypes.contains(XBinary::FT_BMP)) {
        DialogShowImage dialogShowImage(this, sFileName, sTitle);

        dialogShowImage.exec();
    } else if (stFileTypes.contains(XBinary::FT_TEXT)) {
        DialogTextInfo dialogTextInfo(this);
        dialogTextInfo.setTitle(sTitle);
        dialogTextInfo.setWrap(false);

        dialogTextInfo.setFileName(sFileName);

        dialogTextInfo.exec();
    } else if (stFileTypes.contains(XBinary::FT_ANDROIDXML)) {
        QString sString = XAndroidBinary::getDecoded(sFileName);

        DialogTextInfo dialogTextInfo(this);
        dialogTextInfo.setTitle(sTitle);
        dialogTextInfo.setWrap(false);

        dialogTextInfo.setText(sString);

        dialogTextInfo.exec();
    } else if (stFileTypes.contains(XBinary::FT_MSDOS) || stFileTypes.contains(XBinary::FT_NE) || stFileTypes.contains(XBinary::FT_LE) ||
               stFileTypes.contains(XBinary::FT_LX) || stFileTypes.contains(XBinary::FT_PE) || stFileTypes.contains(XBinary::FT_ELF) ||
               stFileTypes.contains(XBinary::FT_MACHO) || stFileTypes.contains(XBinary::FT_DEX)) {
        QFile file;

        file.setFileName(sFileName);

        bool bOpen = false;

        if (bReadWrite) {
            bOpen = XBinary::tryToOpen(&file);
        } else {
            bOpen = file.open(QIODevice::ReadOnly);
        }

        if (bOpen) {
            FW_DEF::OPTIONS options = g_options;  // TODO options from setData
            options.sTitle = sTitle;

            if (stFileTypes.contains(XBinary::FT_PE)) {
                options.nStartType = SPE::TYPE_IMAGE_FILE_HEADER;
                options.nImageBase = -1;

                DialogPE dialogPE(this);
                dialogPE.setGlobal(getShortcuts(), getGlobalOptions());
                dialogPE.setData(&file, options);

                dialogPE.exec();
            } else if (stFileTypes.contains(XBinary::FT_LE)) {
                options.nStartType = SLE::TYPE_VXD_HEADER;
                options.nImageBase = -1;

                DialogLE dialogLE(this);
                dialogLE.setGlobal(getShortcuts(), getGlobalOptions());
                dialogLE.setData(&file, options);

                dialogLE.exec();
            } else if (stFileTypes.contains(XBinary::FT_NE)) {
                options.nStartType = SNE::TYPE_OS2_HEADER;
                options.nImageBase = -1;

                DialogNE dialogNE(this);
                dialogNE.setGlobal(getShortcuts(), getGlobalOptions());
                dialogNE.setData(&file, options);

                dialogNE.exec();
            } else if (stFileTypes.contains(XBinary::FT_MSDOS)) {
                options.nStartType = SMSDOS::TYPE_DOS_HEADER;
                options.nImageBase = -1;

                DialogMSDOS dialogMSDOS(this);
                dialogMSDOS.setGlobal(getShortcuts(), getGlobalOptions());
                dialogMSDOS.setData(&file, options);

                dialogMSDOS.exec();
            } else if (stFileTypes.contains(XBinary::FT_ELF)) {
                options.nStartType = SELF::TYPE_Elf_Ehdr;
                options.nImageBase = -1;

                DialogELF dialogELF(this);
                dialogELF.setGlobal(getShortcuts(), getGlobalOptions());
                dialogELF.setData(&file, options);

                dialogELF.exec();
            } else if (stFileTypes.contains(XBinary::FT_MACHO)) {
                options.nStartType = SMACH::TYPE_mach_header;
                options.nImageBase = -1;

                DialogMACH dialogMACH(this);
                dialogMACH.setGlobal(getShortcuts(), getGlobalOptions());
                dialogMACH.setData(&file, options);

                dialogMACH.exec();
            } else if (stFileTypes.contains(XBinary::FT_DEX)) {
                options.nStartType = SDEX::TYPE_HEADER;
                options.nImageBase = -1;

                DialogDEX dialogDEX(this);
                dialogDEX.setGlobal(getShortcuts(), getGlobalOptions());
                dialogDEX.setData(&file, options);

                dialogDEX.exec();
            }

            file.close();
        }
    }
}

void Archive_widget::on_comboBoxType_currentIndexChanged(int nIndex)
{
    ui->stackedWidgetArchive->setCurrentIndex(nIndex);

    ui->groupBoxFilter->setEnabled(nIndex);
}

void Archive_widget::on_lineEditFilter_textChanged(const QString &sString)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    g_pFilterTable->setFilterRegularExpression(sString);
#else
    g_pFilterTable->setFilterRegExp(sString);
#endif
    g_pFilterTable->setFilterCaseSensitivity(Qt::CaseInsensitive);
    g_pFilterTable->setFilterKeyColumn(1);
}

void Archive_widget::on_treeViewArchive_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index)

    QModelIndexList listIndexes = ui->treeViewArchive->selectionModel()->selectedIndexes();

    if (listIndexes.size() > 0) {
        openRecord();
    }
}

void Archive_widget::on_tableViewArchive_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index)

    QModelIndexList listIndexes = ui->tableViewArchive->selectionModel()->selectedIndexes();

    if (listIndexes.size() > 0) {
        openRecord();
    }
}

void Archive_widget::onTreeElement_selected(const QItemSelection &itemSelected, const QItemSelection &itemDeselected)
{
    Q_UNUSED(itemSelected)
    Q_UNUSED(itemDeselected)

    QModelIndexList listIndexes = ui->treeViewArchive->selectionModel()->selectedIndexes();

    if (listIndexes.count() > 0) {
        QModelIndex _index = listIndexes.at(0);

        g_sCurrentRecordFileName = ui->treeViewArchive->model()->data(_index, Qt::UserRole + CreateViewModelProcess::UR_PATH).toString();
        g_bCurrentFileIsRoot = ui->treeViewArchive->model()->data(_index, Qt::UserRole + CreateViewModelProcess::UR_ISROOT).toBool();
        g_nCurrentFileSize = ui->treeViewArchive->model()->data(_index, Qt::UserRole + CreateViewModelProcess::UR_SIZE).toLongLong();
    }
}

void Archive_widget::registerShortcuts(bool bState)
{
    Q_UNUSED(bState)

    // TODO !!!
}

void Archive_widget::onTableElement_selected(const QItemSelection &itemSelected, const QItemSelection &itemDeselected)
{
    Q_UNUSED(itemSelected)
    Q_UNUSED(itemDeselected)

    QModelIndexList listIndexes = ui->tableViewArchive->selectionModel()->selectedIndexes();

    if (listIndexes.count() > 0) {
        QModelIndex _index = listIndexes.at(0);

        g_sCurrentRecordFileName = ui->tableViewArchive->model()->data(_index, Qt::UserRole + CreateViewModelProcess::UR_PATH).toString();
        g_bCurrentFileIsRoot = ui->tableViewArchive->model()->data(_index, Qt::UserRole + CreateViewModelProcess::UR_ISROOT).toBool();
        g_nCurrentFileSize = ui->tableViewArchive->model()->data(_index, Qt::UserRole + CreateViewModelProcess::UR_SIZE).toLongLong();
    }
}

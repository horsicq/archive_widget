/* Copyright (c) 2020-2026 hors<horsicq@gmail.com>
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

    m_pFilterTable = new QSortFilterProxyModel(this);

    ui->comboBoxType->addItem(tr("Tree"));
    ui->comboBoxType->addItem(tr("Table"));

    ui->groupBoxFilter->setEnabled(false);
    ui->comboBoxType->setCurrentIndex(0);

    m_nCurrentFileSize = 0;
    m_bCurrentFileIsRoot = false;
    m_type = CreateViewModelProcess::TYPE_UNKNOWN;
}

void Archive_widget::setFileName(const QString &sFileName, XBinary::FT fileType, const FW_DEF::OPTIONS &options, const QSet<XBinary::FT> &stAvailableOpenFileTypes)
{
    setData(CreateViewModelProcess::TYPE_FILE, sFileName, fileType, options, stAvailableOpenFileTypes);
}

void Archive_widget::setDirectoryName(const QString &sDirectoryName, const FW_DEF::OPTIONS &options, const QSet<XBinary::FT> &stAvailableOpenFileTypes)
{
    setData(CreateViewModelProcess::TYPE_DIRECTORY, sDirectoryName, XBinary::FT_UNKNOWN, options, stAvailableOpenFileTypes);
}

void Archive_widget::setData(CreateViewModelProcess::TYPE type, const QString &sName, XBinary::FT fileType, const FW_DEF::OPTIONS &options,
                             const QSet<XBinary::FT> &stAvailableOpenFileTypes)
{
    m_type = type;
    m_sName = sName;
    m_options = options;

    m_stAvailableOpenFileTypes = stAvailableOpenFileTypes;

    if (!m_stAvailableOpenFileTypes.count()) {
        // TODO more
        // TODO Check mb Archives
        // TODO APK, JAR, ZIP, IPA, NPM
        m_stAvailableOpenFileTypes.insert(XBinary::FT_MSDOS);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_NE);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_LE);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_LX);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_PE);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_ELF);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_DEX);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_MACHO);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_MACHOFAT);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_BMP);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_PNG);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_JPEG);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_GIF);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_TIFF);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_TEXT);
        m_stAvailableOpenFileTypes.insert(XBinary::FT_ANDROIDXML);
    }

    ui->tableViewArchive->setSortingEnabled(false);

    QAbstractItemModel *pOldTreeModel = ui->treeViewArchive->model();
    QStandardItemModel *pNewTreeModel = nullptr;

    QAbstractItemModel *pOldTableModel = m_pFilterTable->sourceModel();
    QStandardItemModel *pNewTableModel = nullptr;

    m_pFilterTable->setSourceModel(0);
    ui->tableViewArchive->setModel(0);

    ui->comboBoxType->setCurrentIndex(0);
    ui->lineEditFilter->clear();

    QSet<XBinary::FT> stFilterFileTypes;

    if (m_options.bFilter) {
        stFilterFileTypes = m_stAvailableOpenFileTypes;
    }

    m_listViewRecords.clear();

    DialogCreateViewModel dialogCreateViewModel(XOptions::getMainWidget(this));
    dialogCreateViewModel.setData(type, sName, fileType, &m_listRecords, &pNewTreeModel, &pNewTableModel, stFilterFileTypes, &m_listViewRecords);

    dialogCreateViewModel.showDialogDelay();

    ui->treeViewArchive->setModel(pNewTreeModel);

    ui->treeViewArchive->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->treeViewArchive->header()->setSectionResizeMode(1, QHeaderView::Interactive);

    m_pFilterTable->setSourceModel(pNewTableModel);
    ui->tableViewArchive->setModel(m_pFilterTable);  // TODO XTableView

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

    if (m_options.bFilter) {
        ui->comboBoxType->setCurrentIndex(1);  // TODO enum !!!

        if (m_listRecords.count()) {
            ui->tableViewArchive->setCurrentIndex(ui->tableViewArchive->model()->index(0, 0));
        }
    }
}

QString Archive_widget::getCurrentRecordFileName()
{
    return m_sCurrentRecordFileName;
}

QList<CreateViewModelProcess::RECORD> Archive_widget::getRecordsByFileType(XBinary::FT fileType)
{
    QList<CreateViewModelProcess::RECORD> listResult;

    qint32 nNumberOfRecords = m_listViewRecords.count();

    for (qint32 i = 0; i < nNumberOfRecords; i++) {
        if (XBinary::checkFileType(fileType, m_listViewRecords.at(i).ft)) {
            listResult.append(m_listViewRecords.at(i));
        }
    }

    return listResult;
}

void Archive_widget::adjustView()
{
}

void Archive_widget::reloadData(bool bSaveSelection)
{
    Q_UNUSED(bSaveSelection)

    // TODO
}

Archive_widget::~Archive_widget()
{
    delete ui;
}

void Archive_widget::on_treeViewArchive_customContextMenuRequested(const QPoint &pos)
{
    QModelIndexList listIndexes = ui->treeViewArchive->selectionModel()->selectedIndexes();

    if (listIndexes.size() > 0) {
        showContext(m_sCurrentRecordFileName, m_bCurrentFileIsRoot, ui->treeViewArchive->viewport()->mapToGlobal(pos));
    }
}

void Archive_widget::on_tableViewArchive_customContextMenuRequested(const QPoint &pos)
{
    QModelIndexList listIndexes = ui->tableViewArchive->selectionModel()->selectedIndexes();

    if (listIndexes.size() > 0) {
        showContext(m_sCurrentRecordFileName, m_bCurrentFileIsRoot, ui->tableViewArchive->viewport()->mapToGlobal(pos));
    }
}

void Archive_widget::showContext(const QString &sRecordFileName, bool bIsRoot, QPoint point)
{
    if (sRecordFileName != "") {
        QMenu contextMenu(this);  // TODO

        QAction actionOpen(tr("Open"), this);

        if (!m_options.bNoWindowOpen) {
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

    if (bIsRoot || (m_type == CreateViewModelProcess::TYPE_DIRECTORY)) {
        stFileTypes = XFormats::getFileTypes(sRecordFileName, true);
    } else {
        XArchive::RECORD record = XArchive::getArchiveRecord(sRecordFileName, &m_listRecords);

        QByteArray baData = XArchives::decompress(m_sName, &record, nullptr, 0, 0x200);
        stFileTypes = XFormats::getFileTypes(&baData, true);
    }

    if (XBinary::isFileTypePresent(&stFileTypes, &m_stAvailableOpenFileTypes)) {
        bResult = true;
    }

    return bResult;
}

void Archive_widget::openRecord()
{
    if (!m_options.bNoWindowOpen) {
        if (isOpenAvailable(m_sCurrentRecordFileName, m_bCurrentFileIsRoot)) {
            handleAction(ACTION_OPEN);
        } else {
            handleAction(ACTION_HEX);
        }
    }
    // TODO open for m_options.bNoWindowOpen
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
    qint64 nSize = m_nCurrentFileSize;
    bool bIsRoot = m_bCurrentFileIsRoot;
    QString sRecordFileName = m_sCurrentRecordFileName;

    if (sRecordFileName != "") {
        if (bIsRoot || (m_type == CreateViewModelProcess::TYPE_DIRECTORY)) {
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
            XArchive::RECORD record = XArchive::getArchiveRecord(sRecordFileName, &m_listRecords);

            if (action == ACTION_OPEN) {
                QTemporaryFile fileTemp;

                if (fileTemp.open()) {
                    QString sTempFileName = fileTemp.fileName();

                    DialogUnpackFile dialogUnpackFile(XOptions::getMainWidget(this));

                    dialogUnpackFile.setData(m_sName, &record, sTempFileName);

                    // TODO timer
                    if (dialogUnpackFile.exec() == QDialog::Accepted) {
                        _handleActionOpenFile(sTempFileName, record.spInfo.sRecordName, false);
                    }
                }
            } else if (action == ACTION_COPYFILENAME) {
                QGuiApplication::clipboard()->setText(record.spInfo.sRecordName);
            } else if (action == ACTION_DUMP) {
                QString sSaveFileName = QFileInfo(m_sName).absolutePath() + QDir::separator() + QFileInfo(record.spInfo.sRecordName).fileName();

                sSaveFileName = QFileDialog::getSaveFileName(this, tr("Save file"), sSaveFileName, QFileInfo(record.spInfo.sRecordName).completeSuffix());

                if (sSaveFileName != "") {
                    DialogUnpackFile dialogUnpackFile(XOptions::getMainWidget(this));

                    dialogUnpackFile.setData(m_sName, &record, sSaveFileName);

                    // TODO timer
                    if (dialogUnpackFile.exec() != QDialog::Accepted) {
                        QMessageBox::critical(XOptions::getMainWidget(this), tr("Error"), tr("Cannot save file"));
                    }
                }
            } else {
                if (nSize <= XArchive::getCompressBufferSize()) {
                    QByteArray baData = XArchives::decompress(m_sName, &record);

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
                        dialogUnpackFile.setGlobal(getShortcuts(), getGlobalOptions());
                        dialogUnpackFile.setData(m_sName, &record, sTempFileName);

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
        XHexViewWidget::OPTIONS options = {};

        // TODO options

        DialogHexView dialogHexView(this);
        dialogHexView.setGlobal(getShortcuts(), getGlobalOptions());
        dialogHexView.setData(pDevice, options);
        // TODO XINfoDB !!!

        dialogHexView.exec();
    } else if (action == ACTION_STRINGS) {
        SearchStringsWidget::OPTIONS stringsOptions = {};
        stringsOptions.bMenu_Hex = true;
        stringsOptions.bMenu_Demangle = true;
        stringsOptions.bAnsi = true;
        // stringsOptions.bUTF8 = false;
        stringsOptions.bUnicode = true;
        stringsOptions.bNullTerminated = false;

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
        QString sString = XAndroidBinary::getDecoded(sFileName, nullptr);

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
            FW_DEF::OPTIONS options = m_options;  // TODO options from setData
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
    m_pFilterTable->setFilterRegularExpression(sString);
#else
    m_pFilterTable->setFilterRegExp(sString);
#endif
    m_pFilterTable->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_pFilterTable->setFilterKeyColumn(1);
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

        m_sCurrentRecordFileName = ui->treeViewArchive->model()->data(_index, Qt::UserRole + CreateViewModelProcess::UR_PATH).toString();
        m_bCurrentFileIsRoot = ui->treeViewArchive->model()->data(_index, Qt::UserRole + CreateViewModelProcess::UR_ISROOT).toBool();
        m_nCurrentFileSize = ui->treeViewArchive->model()->data(_index, Qt::UserRole + CreateViewModelProcess::UR_SIZE).toLongLong();
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

        m_sCurrentRecordFileName = ui->tableViewArchive->model()->data(_index, Qt::UserRole + CreateViewModelProcess::UR_PATH).toString();
        m_bCurrentFileIsRoot = ui->tableViewArchive->model()->data(_index, Qt::UserRole + CreateViewModelProcess::UR_ISROOT).toBool();
        m_nCurrentFileSize = ui->tableViewArchive->model()->data(_index, Qt::UserRole + CreateViewModelProcess::UR_SIZE).toLongLong();
    }
}

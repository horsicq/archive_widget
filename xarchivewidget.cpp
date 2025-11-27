/* Copyright (c) 2020-2025 hors<horsicq@gmail.com>
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
#include "xarchivewidget.h"

#include "ui_xarchivewidget.h"

XArchiveWidget::XArchiveWidget(QWidget *pParent) : XShortcutsWidget(pParent), ui(new Ui::XArchiveWidget)
{
    ui->setupUi(this);

    m_pDevice = nullptr;
    m_fileType = XBinary::FT_UNKNOWN;
    m_pModel = nullptr;
    m_pFilterModel = new QSortFilterProxyModel(this);
    m_nCurrentFileSize = 0;

    setupTableView();
}

XArchiveWidget::~XArchiveWidget()
{
    if (m_pModel) {
        delete m_pModel;
    }

    delete ui;
}

void XArchiveWidget::setData(XBinary::FT fileType, QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    m_fileType = fileType;
    m_pDevice = pDevice;

    if (m_pDevice) {
        loadRecords();
    }
}

QString XArchiveWidget::getCurrentRecordFileName()
{
    return m_sCurrentRecordFileName;
}

void XArchiveWidget::adjustView()
{
}

void XArchiveWidget::reloadData(bool bSaveSelection)
{
    Q_UNUSED(bSaveSelection)

    loadRecords();
}

void XArchiveWidget::on_tableViewRecords_customContextMenuRequested(const QPoint &pos)
{
    QModelIndexList listIndexes = ui->tableViewRecords->selectionModel()->selectedIndexes();

    if (listIndexes.size() > 0) {
        showContext(m_sCurrentRecordFileName, ui->tableViewRecords->viewport()->mapToGlobal(pos));
    }
}

void XArchiveWidget::showContext(const QString &sRecordFileName, QPoint point)
{
    if (sRecordFileName != "") {
        QMenu contextMenu(this);

        QAction actionHex(tr("Hex"), this);
        connect(&actionHex, SIGNAL(triggered()), this, SLOT(hexRecord()));
        contextMenu.addAction(&actionHex);

        QAction actionStrings(tr("Strings"), this);
        connect(&actionStrings, SIGNAL(triggered()), this, SLOT(stringsRecord()));
        contextMenu.addAction(&actionStrings);

        QAction actionEntropy(tr("Entropy"), this);
        connect(&actionEntropy, SIGNAL(triggered()), this, SLOT(entropyRecord()));
        contextMenu.addAction(&actionEntropy);

        QAction actionHash(tr("Hash"), this);
        connect(&actionHash, SIGNAL(triggered()), this, SLOT(hashRecord()));
        contextMenu.addAction(&actionHash);

        QMenu menuCopy(tr("Copy"), this);
        QAction actionCopyFilename(tr("File name"), this);
        connect(&actionCopyFilename, SIGNAL(triggered()), this, SLOT(copyFileName()));
        menuCopy.addAction(&actionCopyFilename);
        contextMenu.addMenu(&menuCopy);

        QAction actionDump(tr("Dump to file"), this);
        connect(&actionDump, SIGNAL(triggered()), this, SLOT(dumpRecord()));
        contextMenu.addAction(&actionDump);

        contextMenu.exec(point);
    }
}

void XArchiveWidget::hexRecord()
{
    handleAction(ACTION_HEX);
}

void XArchiveWidget::stringsRecord()
{
    handleAction(ACTION_STRINGS);
}

void XArchiveWidget::entropyRecord()
{
    handleAction(ACTION_ENTROPY);
}

void XArchiveWidget::hashRecord()
{
    handleAction(ACTION_HASH);
}

void XArchiveWidget::copyFileName()
{
    handleAction(ACTION_COPYFILENAME);
}

void XArchiveWidget::dumpRecord()
{
    handleAction(ACTION_DUMP);
}

void XArchiveWidget::handleAction(XArchiveWidget::ACTION action)
{
    QString sRecordFileName = m_sCurrentRecordFileName;

    if (sRecordFileName != "") {
        if (action == ACTION_COPYFILENAME) {
            QGuiApplication::clipboard()->setText(sRecordFileName);
        } else if (action == ACTION_DUMP) {
            XArchive::RECORD record = XArchive::getArchiveRecord(sRecordFileName, &m_listRecords);
            QString sSaveFileName = QFileInfo(record.spInfo.sRecordName).fileName();

            sSaveFileName = QFileDialog::getSaveFileName(this, tr("Save file"), sSaveFileName, QFileInfo(record.spInfo.sRecordName).completeSuffix());

            if (sSaveFileName != "") {
                if (m_pDevice) {
                    XArchive archive(m_pDevice);
                    if (!archive.decompressToFile(&record, sSaveFileName)) {
                        QMessageBox::critical(this, tr("Error"), tr("Cannot save file"));
                    }
                }
            }
        } else {
            XArchive::RECORD record = XArchive::getArchiveRecord(sRecordFileName, &m_listRecords);

            if (m_pDevice) {
                XArchive archive(m_pDevice);
                QByteArray baData = archive.decompress(&record);

                if (!baData.isEmpty()) {
                    QBuffer buffer;
                    buffer.setBuffer(&baData);

                    if (buffer.open(QIODevice::ReadOnly)) {
                        if (action == ACTION_HEX) {
                            QMessageBox::information(this, tr("Info"), tr("Hex view not implemented yet"));
                        } else if (action == ACTION_STRINGS) {
                            QMessageBox::information(this, tr("Info"), tr("Strings view not implemented yet"));
                        } else if (action == ACTION_ENTROPY) {
                            QMessageBox::information(this, tr("Info"), tr("Entropy view not implemented yet"));
                        } else if (action == ACTION_HASH) {
                            QMessageBox::information(this, tr("Info"), tr("Hash view not implemented yet"));
                        }

                        buffer.close();
                    }
                }
            }
        }
    }
}

void XArchiveWidget::on_lineEditFilter_textChanged(const QString &sString)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    m_pFilterModel->setFilterRegularExpression(sString);
#else
    m_pFilterModel->setFilterRegExp(sString);
#endif
    m_pFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_pFilterModel->setFilterKeyColumn(0);
}

void XArchiveWidget::on_tableViewRecords_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index)

    QModelIndexList listIndexes = ui->tableViewRecords->selectionModel()->selectedIndexes();

    if (listIndexes.size() > 0) {
        hexRecord();
    }
}

void XArchiveWidget::onTableElement_selected(const QItemSelection &itemSelected, const QItemSelection &itemDeselected)
{
    Q_UNUSED(itemSelected)
    Q_UNUSED(itemDeselected)

    QModelIndexList listIndexes = ui->tableViewRecords->selectionModel()->selectedIndexes();

    if (listIndexes.count() > 0) {
        QModelIndex sourceIndex = m_pFilterModel->mapToSource(listIndexes.at(0));
        qint32 nRow = sourceIndex.row();

        if (nRow >= 0 && nRow < m_listRecords.count()) {
            m_sCurrentRecordFileName = m_listRecords.at(nRow).spInfo.sRecordName;
            m_nCurrentFileSize = m_listRecords.at(nRow).spInfo.nUncompressedSize;
        }
    }
}

void XArchiveWidget::registerShortcuts(bool bState)
{
    Q_UNUSED(bState)
}

void XArchiveWidget::loadRecords()
{
    if (m_pModel) {
        delete m_pModel;
        m_pModel = nullptr;
    }

    m_pModel = new QStandardItemModel(this);
    m_pModel->setHorizontalHeaderLabels(QStringList() << tr("Name") << tr("Size") << tr("Compressed") << tr("Method"));

    if (m_pDevice) {
        XArchive *pArchive = new XArchive(m_pDevice);
        
        // Use new streaming API to get all records
        XBinary::UNPACK_STATE state = {};
        QMap<XBinary::UNPACK_PROP, QVariant> mapProperties;
        
        if (pArchive->initUnpack(&state, mapProperties, nullptr)) {
            m_listRecords.clear();
            
            while (state.nCurrentIndex < state.nNumberOfRecords) {
                XBinary::ARCHIVERECORD record = pArchive->infoCurrent(&state, nullptr);
                
                QList<QStandardItem *> listItems;

                QString sFileName = record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
                QStandardItem *pItemName = new QStandardItem(sFileName);
                pItemName->setEditable(false);
                listItems.append(pItemName);

                qint64 nUncompressedSize = record.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
                QStandardItem *pItemSize = new QStandardItem(QString::number(nUncompressedSize));
                pItemSize->setEditable(false);
                pItemSize->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                listItems.append(pItemSize);

                qint64 nCompressedSize = record.mapProperties.value(XBinary::FPART_PROP_COMPRESSEDSIZE).toLongLong();
                QStandardItem *pItemCompressed = new QStandardItem(QString::number(nCompressedSize));
                pItemCompressed->setEditable(false);
                pItemCompressed->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
                listItems.append(pItemCompressed);

                XBinary::COMPRESS_METHOD compressMethod = (XBinary::COMPRESS_METHOD)record.mapProperties.value(XBinary::FPART_PROP_COMPRESSMETHOD).toInt();
                QStandardItem *pItemMethod = new QStandardItem(XBinary::compressMethodToString(compressMethod));
                pItemMethod->setEditable(false);
                listItems.append(pItemMethod);

                m_pModel->appendRow(listItems);
                
                // Store record info for later use
                XArchive::RECORD oldRecord = {};
                oldRecord.spInfo.sRecordName = sFileName;
                oldRecord.spInfo.nUncompressedSize = nUncompressedSize;
                oldRecord.spInfo.compressMethod = compressMethod;
                oldRecord.nDataOffset = record.nStreamOffset;
                oldRecord.nDataSize = record.nStreamSize;
                m_listRecords.append(oldRecord);
                
                pArchive->moveToNext(&state, nullptr);
            }
            
            pArchive->finishUnpack(&state, nullptr);
        }
        
        delete pArchive;
    }

    m_pFilterModel->setSourceModel(m_pModel);
    ui->tableViewRecords->setModel(m_pFilterModel);
    ui->tableViewRecords->setSortingEnabled(true);

    setupTableView();
}

void XArchiveWidget::setupTableView()
{
    ui->tableViewRecords->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewRecords->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewRecords->horizontalHeader()->setStretchLastSection(false);
    ui->tableViewRecords->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableViewRecords->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->tableViewRecords->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    ui->tableViewRecords->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    ui->tableViewRecords->verticalHeader()->setVisible(false);
    ui->tableViewRecords->verticalHeader()->setDefaultSectionSize(20);

    connect(ui->tableViewRecords->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
            SLOT(onTableElement_selected(QItemSelection, QItemSelection)));
}

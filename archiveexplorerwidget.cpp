/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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
#include "archiveexplorerwidget.h"

#include "ui_archiveexplorerwidget.h"

ArchiveExplorerWidget::ArchiveExplorerWidget(QWidget *pParent) : XShortcutsWidget(pParent), ui(new Ui::ArchiveExplorerWidget)
{
    ui->setupUi(this);

    m_pDevice = nullptr;
    m_fileType = XBinary::FT_UNKNOWN;
    m_pModel = nullptr;
    m_nCurrentFileSize = 0;
}

ArchiveExplorerWidget::~ArchiveExplorerWidget()
{
    delete ui;
}

void ArchiveExplorerWidget::setData(XBinary::FT fileType, QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    m_fileType = fileType;
    m_pDevice = pDevice;

    if (m_pDevice) {
        loadRecords();
    }
}

QString ArchiveExplorerWidget::getCurrentRecordFileName()
{
    return m_sCurrentRecordFileName;
}

void ArchiveExplorerWidget::adjustView()
{
}

void ArchiveExplorerWidget::reloadData(bool bSaveSelection)
{
    Q_UNUSED(bSaveSelection)

    loadRecords();
}

void ArchiveExplorerWidget::on_tableViewRecords_customContextMenuRequested(const QPoint &pos)
{
    QModelIndexList listIndexes = ui->tableViewRecords->selectionModel()->selectedIndexes();

    if (listIndexes.size() > 0) {
        showContext(m_sCurrentRecordFileName, ui->tableViewRecords->viewport()->mapToGlobal(pos));
    }
}

void ArchiveExplorerWidget::showContext(const QString &sRecordFileName, QPoint point)
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

void ArchiveExplorerWidget::hexRecord()
{
    handleAction(ACTION_HEX);
}

void ArchiveExplorerWidget::stringsRecord()
{
    handleAction(ACTION_STRINGS);
}

void ArchiveExplorerWidget::entropyRecord()
{
    handleAction(ACTION_ENTROPY);
}

void ArchiveExplorerWidget::hashRecord()
{
    handleAction(ACTION_HASH);
}

void ArchiveExplorerWidget::copyFileName()
{
    handleAction(ACTION_COPYFILENAME);
}

void ArchiveExplorerWidget::dumpRecord()
{
    handleAction(ACTION_DUMP);
}

void ArchiveExplorerWidget::handleAction(ArchiveExplorerWidget::ACTION action)
{
    QString sRecordFileName = m_sCurrentRecordFileName;

    if (sRecordFileName != "") {
        if (action == ACTION_COPYFILENAME) {
            QGuiApplication::clipboard()->setText(sRecordFileName);
        } else if (action == ACTION_DUMP) {
            qint32 nRow = -1;

            QModelIndexList listIndexes = ui->tableViewRecords->selectionModel()->selectedIndexes();

            if (listIndexes.count() > 0) {
                QModelIndex sourceIndex = ui->tableViewRecords->getProxyModel()->mapToSource(listIndexes.at(0));
                nRow = sourceIndex.row();
            }

            if ((nRow >= 0) && (nRow < m_listArchiveRecords.count())) {
                const XBinary::ARCHIVERECORD &record = m_listArchiveRecords.at(nRow);
                QString sOrigName = record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
                QString sSaveFileName = QFileInfo(sOrigName).fileName();

                sSaveFileName = QFileDialog::getSaveFileName(this, tr("Save file"), sSaveFileName, QFileInfo(sOrigName).completeSuffix());

                if (sSaveFileName != "") {
                    if (m_pDevice) {
                        XArchive *pArchive = static_cast<XArchive *>(XFormats::getClass(m_fileType, m_pDevice));

                        if (pArchive) {
                            QList<XArchive::RECORD> listOldRecords = pArchive->getRecords(-1, nullptr);
                            XArchive::RECORD oldRecord = XArchive::getArchiveRecord(sOrigName, &listOldRecords);

                            XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
                            bool bResult = pArchive->decompressToFile(&oldRecord, sSaveFileName, &pdStruct);

                            delete pArchive;

                            if (!bResult) {
                                QMessageBox::critical(this, tr("Error"), tr("Cannot save file"));
                            }
                        } else {
                            QMessageBox::critical(this, tr("Error"), tr("Cannot open archive"));
                        }
                    }
                }
            }
        } else {
            // TODO: Implement hex, strings, entropy, hash actions
        }
    }
}

void ArchiveExplorerWidget::on_tableViewRecords_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index)

    QModelIndexList listIndexes = ui->tableViewRecords->selectionModel()->selectedIndexes();

    if (listIndexes.size() > 0) {
        hexRecord();
    }
}

void ArchiveExplorerWidget::onTableElement_selected(const QItemSelection &itemSelected, const QItemSelection &itemDeselected)
{
    Q_UNUSED(itemSelected)
    Q_UNUSED(itemDeselected)

    QModelIndexList listIndexes = ui->tableViewRecords->selectionModel()->selectedIndexes();

    if (listIndexes.count() > 0) {
        QModelIndex sourceIndex = ui->tableViewRecords->getProxyModel()->mapToSource(listIndexes.at(0));
        qint32 nRow = sourceIndex.row();

        if ((nRow >= 0) && (nRow < m_listArchiveRecords.count())) {
            const XBinary::ARCHIVERECORD &record = m_listArchiveRecords.at(nRow);

            m_sCurrentRecordFileName = record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
            m_nCurrentFileSize = record.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
        }
    }
}

void ArchiveExplorerWidget::registerShortcuts(bool bState)
{
    Q_UNUSED(bState)
}

void ArchiveExplorerWidget::loadRecords()
{
    m_listArchiveRecords.clear();
    m_sCurrentRecordFileName.clear();
    m_nCurrentFileSize = 0;

    QList<XBinary::FPART_PROP> listColumns;

    if (m_pDevice) {
        XArchive *pArchive = static_cast<XArchive *>(XFormats::getClass(m_fileType, m_pDevice));

        if (pArchive) {
            listColumns = pArchive->getAvailableFPARTProperties();

            XBinary::UNPACK_STATE state = {};
            QMap<XBinary::UNPACK_PROP, QVariant> mapProperties;

            bool bInit = pArchive->initUnpack(&state, mapProperties, nullptr);

            if (bInit) {
                while (state.nCurrentIndex < state.nNumberOfRecords) {
                    XBinary::ARCHIVERECORD record = pArchive->infoCurrent(&state, nullptr);
                    m_listArchiveRecords.append(record);
                    if (!pArchive->moveToNext(&state, nullptr)) {
                        break;
                    }
                }

                pArchive->finishUnpack(&state, nullptr);
            }

            delete pArchive;
        }
    }

    if (listColumns.isEmpty()) {
        listColumns.append(XBinary::FPART_PROP_ORIGINALNAME);
        listColumns.append(XBinary::FPART_PROP_COMPRESSEDSIZE);
        listColumns.append(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
        listColumns.append(XBinary::FPART_PROP_STREAMOFFSET);
        listColumns.append(XBinary::FPART_PROP_STREAMSIZE);
        listColumns.append(XBinary::FPART_PROP_HANDLEMETHOD);
    }

    m_pModel = new XModel_ArchiveRecords(listColumns, &m_listArchiveRecords, this);

    ui->tableViewRecords->setCustomModel(m_pModel, true);

    connect(ui->tableViewRecords->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this,
            SLOT(onTableElement_selected(QItemSelection, QItemSelection)));
}

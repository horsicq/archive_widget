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

#include <QSet>

#include <algorithm>

#include "ui_archiveexplorerwidget.h"

ArchiveExplorerWidget::ArchiveExplorerWidget(QWidget *pParent) : XShortcutsWidget(pParent), ui(new Ui::ArchiveExplorerWidget)
{
    ui->setupUi(this);

    m_pDevice = nullptr;
    m_fileType = XBinary::FT_UNKNOWN;
    m_pModel = nullptr;
    m_nCurrentFileSize = 0;
    m_bAdvanced = false;
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

    loadRecords();
}

const QList<XBinary::ARCHIVERECORD> *ArchiveExplorerWidget::getArchiveRecords() const
{
    return &m_listArchiveRecords;
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

void ArchiveExplorerWidget::on_checkBoxAdvanced_toggled(bool bChecked)
{
    m_bAdvanced = bChecked;

    if (m_pDevice) {
        loadRecords();
    }
}

void ArchiveExplorerWidget::on_tableViewRecords_customContextMenuRequested(const QPoint &pos)
{
    if (!ui->tableViewRecords->selectionModel()) {
        return;
    }

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

            if (!ui->tableViewRecords->selectionModel() || !ui->tableViewRecords->getProxyModel()) {
                return;
            }

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
                        XArchive *pArchive = static_cast<XArchive *>(XFormats::createClass(m_fileType, m_pDevice));

                        if (pArchive) {
                            // records were listed with the streaming unpack API, so dump the
                            // selected row through the same API: it handles solid archives
                            // (e.g. 7z LZMA2 blocks) that the legacy per-record path cannot
                            bool bResult = false;

                            XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
                            XBinary::UNPACK_STATE state = {};
                            QMap<XBinary::UNPACK_PROP, QVariant> mapProperties;

                            if (pArchive->initUnpack(&state, mapProperties, &pdStruct)) {
                                bool bSeekOk = true;

                                for (qint32 nIndex = 0; (nIndex < nRow) && bSeekOk; nIndex++) {
                                    bSeekOk = pArchive->moveToNext(&state, &pdStruct);
                                }

                                if (bSeekOk && (state.nCurrentIndex < state.nNumberOfRecords)) {
                                    QFile fileResult(sSaveFileName);

                                    if (fileResult.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
                                        bResult = pArchive->unpackCurrent(&state, &fileResult, &pdStruct);
                                        fileResult.close();
                                    }
                                }

                                pArchive->finishUnpack(&state, &pdStruct);
                            }

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

    if (!ui->tableViewRecords->selectionModel()) {
        return;
    }

    QModelIndexList listIndexes = ui->tableViewRecords->selectionModel()->selectedIndexes();

    if (listIndexes.size() > 0) {
        hexRecord();
    }
}

void ArchiveExplorerWidget::onTableElement_selected(const QItemSelection &itemSelected, const QItemSelection &itemDeselected)
{
    Q_UNUSED(itemSelected)
    Q_UNUSED(itemDeselected)

    if (!ui->tableViewRecords->selectionModel() || !ui->tableViewRecords->getProxyModel()) {
        return;
    }

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
        XArchive *pArchive = static_cast<XArchive *>(XFormats::createClass(m_fileType, m_pDevice));

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

    // show every property the format parser actually filled, in a fixed preferred
    // order, so each archive type exposes the maximum available information
    {
        QList<XBinary::FPART_PROP> listPreferred;
        listPreferred.append(XBinary::FPART_PROP_ORIGINALNAME);
        listPreferred.append(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
        listPreferred.append(XBinary::FPART_PROP_COMPRESSEDSIZE);
        listPreferred.append(XBinary::FPART_PROP_HANDLEMETHOD);
        listPreferred.append(XBinary::FPART_PROP_DATETIME);
        listPreferred.append(XBinary::FPART_PROP_MTIME);
        listPreferred.append(XBinary::FPART_PROP_CTIME);
        listPreferred.append(XBinary::FPART_PROP_ATIME);
        listPreferred.append(XBinary::FPART_PROP_UNCOMPRESSEDCRC);
        listPreferred.append(XBinary::FPART_PROP_RESULTCRC);
        listPreferred.append(XBinary::FPART_PROP_ENCRYPTED);
        listPreferred.append(XBinary::FPART_PROP_FILEMODE);
        listPreferred.append(XBinary::FPART_PROP_USERNAME);
        listPreferred.append(XBinary::FPART_PROP_GROUPNAME);
        listPreferred.append(XBinary::FPART_PROP_UID);
        listPreferred.append(XBinary::FPART_PROP_GID);
        listPreferred.append(XBinary::FPART_PROP_LINKNAME);
        listPreferred.append(XBinary::FPART_PROP_INFO);

        QList<XBinary::FPART_PROP> listPresent;
        QSet<XBinary::FPART_PROP> stAdded;
        qint32 nNumberOfPreferred = listPreferred.count();
        qint32 nNumberOfRecords = m_listArchiveRecords.count();

        for (qint32 i = 0; i < nNumberOfPreferred; i++) {
            XBinary::FPART_PROP fpartProp = listPreferred.at(i);
            bool bPresent = false;

            for (qint32 j = 0; (j < nNumberOfRecords) && (!bPresent); j++) {
                bPresent = m_listArchiveRecords.at(j).mapProperties.contains(fpartProp);
            }

            if (bPresent) {
                listPresent.append(fpartProp);
                stAdded.insert(fpartProp);
            }
        }

        if (m_bAdvanced) {
            // advanced mode: additionally show every remaining property the
            // format parser filled, in enum order
            QSet<XBinary::FPART_PROP> stAll;

            for (qint32 j = 0; j < nNumberOfRecords; j++) {
                QList<XBinary::FPART_PROP> listKeys = m_listArchiveRecords.at(j).mapProperties.keys();
                qint32 nNumberOfKeys = listKeys.count();

                for (qint32 k = 0; k < nNumberOfKeys; k++) {
                    stAll.insert(listKeys.at(k));
                }
            }

            QList<XBinary::FPART_PROP> listAll = stAll.values();
            std::sort(listAll.begin(), listAll.end());

            qint32 nNumberOfProps = listAll.count();

            for (qint32 i = 0; i < nNumberOfProps; i++) {
                XBinary::FPART_PROP fpartProp = listAll.at(i);

                if (!stAdded.contains(fpartProp)) {
                    listPresent.append(fpartProp);
                    stAdded.insert(fpartProp);
                }
            }
        }

        if (listPresent.count() > 1) {
            // stream location inside the archive file is always known
            if (!stAdded.contains(XBinary::FPART_PROP_STREAMOFFSET)) {
                listPresent.append(XBinary::FPART_PROP_STREAMOFFSET);
            }

            if (!stAdded.contains(XBinary::FPART_PROP_STREAMSIZE)) {
                listPresent.append(XBinary::FPART_PROP_STREAMSIZE);
            }

            listColumns = listPresent;
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
            SLOT(onTableElement_selected(QItemSelection, QItemSelection)), Qt::UniqueConnection);
}

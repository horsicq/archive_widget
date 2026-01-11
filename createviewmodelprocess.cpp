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
#include "createviewmodelprocess.h"

CreateViewModelProcess::CreateViewModelProcess(QObject *pParent) : QObject(pParent)
{
    m_pPdStruct = nullptr;
    m_type = TYPE_UNKNOWN;
    m_fileType = XBinary::FT_UNKNOWN;
}

void CreateViewModelProcess::setData(TYPE type, const QString &sName, XBinary::FT fileType, QList<XArchive::RECORD> *pListArchiveRecords,
                                     QStandardItemModel **ppTreeModel, QStandardItemModel **ppTableModel, const QSet<XBinary::FT> &stFilterFileTypes,
                                     QList<RECORD> *pListViewRecords, XBinary::PDSTRUCT *pPdStruct)
{
    // Validate inputs
    if (!pListArchiveRecords || !ppTreeModel || !ppTableModel || !pListViewRecords) {
        emit errorMessage("Invalid parameters provided to CreateViewModelProcess");
        return;
    }

    this->m_type = type;
    this->m_sName = sName;
    this->m_fileType = fileType;
    this->m_pListArchiveRecords = pListArchiveRecords;
    this->m_ppTreeModel = ppTreeModel;
    this->m_ppTableModel = ppTableModel;
    this->m_stFilterFileTypes = stFilterFileTypes;
    this->m_pListViewRecords = pListViewRecords;
    this->m_pPdStruct = pPdStruct;
}

void CreateViewModelProcess::process()
{
    QElapsedTimer scanTimer;
    scanTimer.start();

    emit progressValueChanged(0);
    emit progressMessageChanged("Initializing...");

    XBinary::FT ftPref = m_fileType;

    if (m_type == TYPE_FILE) {
        if (ftPref == XBinary::FT_UNKNOWN) {
            QSet<XBinary::FT> stFT = XFormats::getFileTypes(m_sName, true, m_pPdStruct);
            ftPref = XBinary::_getPrefFileType(&stFT);
        }

        *m_pListArchiveRecords = XArchives::getRecords(m_sName, ftPref, -1, m_pPdStruct);

        if (m_pListArchiveRecords->isEmpty()) {
            emit errorMessage("Failed to read archive records");
            emit completed(scanTimer.elapsed());
            return;
        }
    } else if (m_type == TYPE_DIRECTORY) {
        *m_pListArchiveRecords = XArchives::getRecordsFromDirectory(m_sName, -1, m_pPdStruct);

        if (m_pListArchiveRecords->isEmpty()) {
            emit errorMessage("No files found in directory");
            emit completed(scanTimer.elapsed());
            return;
        }
    }

    qint64 nFileSize = XBinary::getSize(m_sName);

    qint32 nNumberOfRecords = m_pListArchiveRecords->count();

    qint32 _nFreeIndex = XBinary::getFreeIndex(m_pPdStruct);
    XBinary::setPdStructInit(m_pPdStruct, _nFreeIndex, nNumberOfRecords);

    if (nNumberOfRecords == 0) {
        RECORD _record = {};
        _record.sRecordName = m_sName;
        _record.ft = ftPref;
        _record.bIsVirtual = false;

        m_pListViewRecords->append(_record);
    }

    *m_ppTreeModel = new QStandardItemModel;
    (*m_ppTreeModel)->setColumnCount(2);

    *m_ppTableModel = new QStandardItemModel;
    (*m_ppTableModel)->setColumnCount(3);

    QString sBaseName = QFileInfo(m_sName).fileName();

    QStandardItem *pRootItemName = new QStandardItem;
    pRootItemName->setText(sBaseName);
    pRootItemName->setData(m_sName, Qt::UserRole + UR_PATH);
    pRootItemName->setData(nFileSize, Qt::UserRole + UR_SIZE);
    pRootItemName->setData(true, Qt::UserRole + UR_ISROOT);

    (*m_ppTreeModel)->setItem(0, 0, pRootItemName);

    QStandardItem *pRootItemSize = new QStandardItem;
    pRootItemSize->setText(QString::number(nFileSize));
    pRootItemSize->setTextAlignment(Qt::AlignRight);

    (*m_ppTreeModel)->setItem(0, 1, pRootItemSize);

    bool bFilter = m_stFilterFileTypes.count();

    QMap<QString, QStandardItem *> mapItems;

    for (qint32 i = 0, j = 0; (i < nNumberOfRecords) && XBinary::isPdStructNotCanceled(m_pPdStruct); i++) {
        XArchive::RECORD record = m_pListArchiveRecords->at(i);
        QString sRecordFileName = record.spInfo.sRecordName;

        // Emit progress update
        emit progressValueChanged((i * 100) / nNumberOfRecords);
        emit progressMessageChanged(QString("Processing: %1").arg(sRecordFileName));

        bool bAdd = true;

        QSet<XBinary::FT> stFT;

        if (m_type == TYPE_FILE) {
            QByteArray baData = XArchives::decompress(m_sName, &record, m_pPdStruct);

            stFT = XFormats::getFileTypes(&baData, true);
        } else if (m_type == TYPE_DIRECTORY) {
            stFT = XFormats::getFileTypes(sRecordFileName, true);
        }

        XBinary::FT ftPref = XBinary::_getPrefFileType(&stFT);

        RECORD _record = {};
        _record.sRecordName = sRecordFileName;
        _record.ft = ftPref;
        _record.bIsVirtual = true;

        m_pListViewRecords->append(_record);

        if (bFilter) {
            bAdd = XBinary::isFileTypePresent(&stFT, &m_stFilterFileTypes);
        }

        if (bAdd) {
            QString _sRecordFileName = sRecordFileName;

            while (_sRecordFileName.size() > 1) {
                if (_sRecordFileName.at(0) == QChar('/')) {
                    _sRecordFileName = _sRecordFileName.mid(1, -1);
                } else {
                    break;
                }
            }

            qint32 nNumberOfParts = _sRecordFileName.count("/");

            for (qint32 k = 0; k <= nNumberOfParts; k++) {
                QString sPart = _sRecordFileName.section("/", k, k);
                QString sRelPart;

                if (sPart != "") {
                    if (k != nNumberOfParts) {
                        sRelPart = _sRecordFileName.section("/", 0, k);
                    } else {
                        sRelPart = _sRecordFileName;
                    }

                    if (!mapItems.contains(sRelPart)) {
                        QStandardItem *pItemName = new QStandardItem;
                        pItemName->setText(sPart);

                        if (k == (nNumberOfParts)) {
                            pItemName->setData(sRecordFileName, Qt::UserRole + UR_PATH);
                            pItemName->setData(record.spInfo.nUncompressedSize, Qt::UserRole + UR_SIZE);
                            pItemName->setData(false, Qt::UserRole + UR_ISROOT);
                            pItemName->setData(ftPref, Qt::UserRole + UR_FT);
                        }

                        QStandardItem *pItemParent = nullptr;

                        if (k == 0) {
                            pItemParent = pRootItemName;
                        } else {
                            pItemParent = mapItems.value(_sRecordFileName.section("/", 0, k - 1));
                        }

                        QList<QStandardItem *> listItems;

                        listItems.append(pItemName);

                        if (k == (nNumberOfParts)) {
                            QStandardItem *pItemSize = new QStandardItem;
                            pItemSize->setData(record.spInfo.nUncompressedSize, Qt::DisplayRole);
                            pItemSize->setTextAlignment(Qt::AlignRight);

                            listItems.append(pItemSize);
                        }

                        pItemParent->appendRow(listItems);

                        mapItems.insert(sRelPart, pItemName);
                    }
                }
            }

            QList<QStandardItem *> listItems;

            QStandardItem *pItemNumber = new QStandardItem;
            pItemNumber->setData(j, Qt::DisplayRole);
            pItemNumber->setTextAlignment(Qt::AlignRight);
            pItemNumber->setData(sRecordFileName, Qt::UserRole + UR_PATH);
            pItemNumber->setData(record.spInfo.nUncompressedSize, Qt::UserRole + UR_SIZE);
            pItemNumber->setData(false, Qt::UserRole + UR_ISROOT);
            pItemNumber->setData(ftPref, Qt::UserRole + UR_FT);
            listItems.append(pItemNumber);

            QStandardItem *pItemName = new QStandardItem;
            pItemName->setText(sRecordFileName);
            listItems.append(pItemName);

            QStandardItem *pItemSize = new QStandardItem;
            pItemSize->setData(record.spInfo.nUncompressedSize, Qt::DisplayRole);
            pItemSize->setTextAlignment(Qt::AlignRight);
            listItems.append(pItemSize);

            (*m_ppTableModel)->appendRow(listItems);

            j++;
        }

        XBinary::setPdStructCurrentIncrement(m_pPdStruct, _nFreeIndex);
        XBinary::setPdStructStatus(m_pPdStruct, _nFreeIndex, m_pListArchiveRecords->at(i).spInfo.sRecordName);
    }

    (*m_ppTreeModel)->setHeaderData(0, Qt::Horizontal, tr("File"));
    (*m_ppTreeModel)->setHeaderData(1, Qt::Horizontal, tr("Size"));

    (*m_ppTableModel)->setHeaderData(0, Qt::Horizontal, "");
    (*m_ppTableModel)->setHeaderData(1, Qt::Horizontal, tr("File"));
    (*m_ppTableModel)->setHeaderData(2, Qt::Horizontal, tr("Size"));

    XBinary::setPdStructFinished(m_pPdStruct, _nFreeIndex);

    emit progressValueChanged(100);
    emit progressMessageChanged("Completed");

    emit completed(scanTimer.elapsed());
}

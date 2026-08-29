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
#include "xgenericarchivewidget.h"

#include "ui_xgenericarchivewidget.h"

XGenericArchiveWidget::XGenericArchiveWidget(QWidget *pParent) : XShortcutsWidget(pParent), ui(new Ui::XGenericArchiveWidget)
{
    ui->setupUi(this);

    m_pDevice = nullptr;
    m_fileType = XBinary::FT_UNKNOWN;
    m_pModel = nullptr;
    m_pFilterModel = new QSortFilterProxyModel(this);
    m_nCurrentFileSize = 0;

    loadRecords();
}

XGenericArchiveWidget::~XGenericArchiveWidget()
{
    m_pFilterModel->setSourceModel(nullptr);
    ui->tableViewRecords->setModel(nullptr);

    if (m_pModel) {
        delete m_pModel;
    }

    delete ui;
}

void XGenericArchiveWidget::setData(XBinary::FT fileType, QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    m_fileType = fileType;
    m_pDevice = pDevice;

    loadRecords();
}

QString XGenericArchiveWidget::getCurrentRecordFileName()
{
    return m_sCurrentRecordFileName;
}

void XGenericArchiveWidget::adjustView()
{
}

void XGenericArchiveWidget::reloadData(bool bSaveSelection)
{
    Q_UNUSED(bSaveSelection)

    loadRecords();
}

void XGenericArchiveWidget::on_tableViewRecords_customContextMenuRequested(const QPoint &pos)
{
    Q_UNUSED(pos)
    // Context menu can be implemented by derived classes
}

void XGenericArchiveWidget::on_lineEditFilter_textChanged(const QString &sString)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 12, 0)
    m_pFilterModel->setFilterRegularExpression(sString);
#else
    m_pFilterModel->setFilterRegExp(sString);
#endif
    m_pFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    m_pFilterModel->setFilterKeyColumn(0);
}

void XGenericArchiveWidget::on_tableViewRecords_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index)
    // Double click action can be implemented by derived classes
}

void XGenericArchiveWidget::onTableElement_selected(const QItemSelection &itemSelected, const QItemSelection &itemDeselected)
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

void XGenericArchiveWidget::registerShortcuts(bool bState)
{
    Q_UNUSED(bState)
}

void XGenericArchiveWidget::loadRecords()
{
    m_listRecords.clear();
    m_sCurrentRecordFileName.clear();
    m_nCurrentFileSize = 0;
    m_pFilterModel->setSourceModel(nullptr);
    ui->tableViewRecords->setModel(nullptr);

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

                XBinary::HANDLE_METHOD compressMethod = (XBinary::HANDLE_METHOD)record.mapProperties.value(XBinary::FPART_PROP_HANDLEMETHOD).toInt();
                QStandardItem *pItemMethod = new QStandardItem(XBinary::handleMethodToString(compressMethod));
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

void XGenericArchiveWidget::setupTableView()
{
    ui->tableViewRecords->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableViewRecords->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableViewRecords->horizontalHeader()->setStretchLastSection(false);
    if (ui->tableViewRecords->horizontalHeader()->count() >= 4) {
        ui->tableViewRecords->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
        ui->tableViewRecords->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        ui->tableViewRecords->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
        ui->tableViewRecords->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    }
    ui->tableViewRecords->verticalHeader()->setVisible(false);
    ui->tableViewRecords->verticalHeader()->setDefaultSectionSize(20);

    QItemSelectionModel *pSelectionModel = ui->tableViewRecords->selectionModel();
    if (pSelectionModel) {
        connect(pSelectionModel, SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(onTableElement_selected(QItemSelection, QItemSelection)),
                Qt::UniqueConnection);
    }
}

// copyright (c) 2020 hors<horsicq@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#include "archive_widget.h"
#include "ui_archive_widget.h"

Archive_widget::Archive_widget(QWidget *pParent) :
    QWidget(pParent),
    ui(new Ui::Archive_widget)
{
    ui->setupUi(this);
}

void Archive_widget::setData(QString sFileName)
{
    this->g_sFileName=sFileName;

    qint64 nFileSize=XBinary::getSize(sFileName);

    g_listRecords=XArchives::getRecords(sFileName);

    int nNumberOfRecords=g_listRecords.count();

    QAbstractItemModel *pOldModel=ui->treeViewArchive->model();

    QStandardItemModel *pNewModel=new QStandardItemModel;
    pNewModel->setColumnCount(2);

    QString sBaseName=QFileInfo(sFileName).fileName();

    QStandardItem *pRootItemName=new QStandardItem;
    pRootItemName->setText(sBaseName);
    pRootItemName->setData(sFileName,Qt::UserRole+UR_PATH);
    pRootItemName->setData(nFileSize,Qt::UserRole+UR_SIZE);
    pRootItemName->setData(true,Qt::UserRole+UR_ISROOT);

    pNewModel->setItem(0,0,pRootItemName);

    QStandardItem *pRootItemSize=new QStandardItem;
    pRootItemSize->setText(QString::number(nFileSize));
    pRootItemSize->setTextAlignment(Qt::AlignRight);

    pNewModel->setItem(0,1,pRootItemSize);

    QMap<QString,QStandardItem *> mapItems;

    // TODO move to thread
    for(int i=0;i<nNumberOfRecords;i++)
    {
        XArchive::RECORD record=g_listRecords.at(i);

        QString sRecordFileName=record.sFileName;

        int nNumberOfParts=sRecordFileName.count("/");

        for(int j=0;j<=nNumberOfParts;j++)
        {
            QString sPart=sRecordFileName.section("/",j,j);
            QString sRelPart;

            if(j!=nNumberOfParts)
            {
                sRelPart=sRecordFileName.section("/",0,j);
            }
            else
            {
                sRelPart=sRecordFileName;
            }

            if(!mapItems.contains(sRelPart))
            {
                QStandardItem *pItemName=new QStandardItem;
                pItemName->setText(sPart);

                if(j==(nNumberOfParts))
                {
                    pItemName->setData(Qt::UserRole+1);

                    pItemName->setData(sRecordFileName,Qt::UserRole+UR_PATH);
                    pItemName->setData(record.nUncompressedSize,Qt::UserRole+UR_SIZE);
                    pRootItemName->setData(false,Qt::UserRole+UR_ISROOT);
                }

                QStandardItem *pParent=0;

                if(j==0)
                {
                    pParent=pRootItemName;
                }
                else
                {
                    pParent=mapItems.value(sRecordFileName.section("/",0,j-1));
                }

                QList<QStandardItem *> listItems;

                listItems.append(pItemName);

                if(j==(nNumberOfParts))
                {
                    QStandardItem *pItemSize=new QStandardItem;
                    pItemSize->setText(QString::number(record.nUncompressedSize));
                    pItemSize->setTextAlignment(Qt::AlignRight);

                    listItems.append(pItemSize);
                }

                pParent->appendRow(listItems);

                mapItems.insert(sRelPart,pItemName);
            }
        }
    }

    pNewModel->setHeaderData(0,Qt::Horizontal,tr("File"));
    pNewModel->setHeaderData(1,Qt::Horizontal,tr("Size"));

    ui->treeViewArchive->setModel(pNewModel);

    ui->treeViewArchive->header()->setSectionResizeMode(0,QHeaderView::Stretch);
    ui->treeViewArchive->header()->setSectionResizeMode(1,QHeaderView::Interactive);

    ui->treeViewArchive->expand(pNewModel->index(0,0));

    delete pOldModel;
}

Archive_widget::~Archive_widget()
{
    delete ui;
}

void Archive_widget::on_treeViewArchive_customContextMenuRequested(const QPoint &pos)
{
    QModelIndexList listIndexes=ui->treeViewArchive->selectionModel()->selectedIndexes();

    if(listIndexes.size()>0)
    {
        QModelIndex index=listIndexes.at(0);

        qint64 nSize=ui->treeViewArchive->model()->data(index,Qt::UserRole+UR_SIZE).toLongLong();
        bool bIsRoot=ui->treeViewArchive->model()->data(index,Qt::UserRole+UR_ISROOT).toBool();
        QString sRecordFileName=ui->treeViewArchive->model()->data(index,Qt::UserRole+UR_PATH).toString();

        QSet<XBinary::FT> stFileTypes;

        if(bIsRoot)
        {
            stFileTypes=XBinary::getFileTypes(sRecordFileName);
        }
        else
        {
            XArchive::RECORD record=XArchive::getArchiveRecord(sRecordFileName,&g_listRecords);

            QByteArray baData=XArchives::decompress(g_sFileName,&record,true);
            stFileTypes=XBinary::getFileTypes(&baData);
        }

        QMenu contextMenu(this);

        QAction actionScan(tr("Scan"),this);
        connect(&actionScan, SIGNAL(triggered()), this, SLOT(scanRecord()));
        contextMenu.addAction(&actionScan);

        QAction actionHex(QString("Hex"),this);
        connect(&actionHex, SIGNAL(triggered()), this, SLOT(hexRecord()));
        contextMenu.addAction(&actionHex);

        QAction actionStrings(tr("Strings"),this);
        connect(&actionStrings, SIGNAL(triggered()), this, SLOT(stringsRecord()));
        contextMenu.addAction(&actionStrings);

        QAction actionEntropy(QString("Entropy"),this);
        connect(&actionEntropy, SIGNAL(triggered()), this, SLOT(entropyRecord()));
        contextMenu.addAction(&actionEntropy);

        QAction actionHash(QString("Hash"),this);
        connect(&actionHash, SIGNAL(triggered()), this, SLOT(hashRecord()));
        contextMenu.addAction(&actionHash);

        contextMenu.exec(ui->treeViewArchive->viewport()->mapToGlobal(pos));
    }
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

void Archive_widget::handleAction(Archive_widget::ACTION action)
{
    QModelIndexList listIndexes=ui->treeViewArchive->selectionModel()->selectedIndexes();

    if(listIndexes.size()>0)
    {
        QModelIndex index=listIndexes.at(0);

        qint64 nSize=ui->treeViewArchive->model()->data(index,Qt::UserRole+UR_SIZE).toLongLong();
        bool bIsRoot=ui->treeViewArchive->model()->data(index,Qt::UserRole+UR_ISROOT).toBool();
        QString sRecordFileName=ui->treeViewArchive->model()->data(index,Qt::UserRole+UR_PATH).toString();

        if(bIsRoot)
        {
            QFile file;

            file.setFileName(sRecordFileName);

            if(file.open(QIODevice::ReadOnly))
            {
                _handleAction(action,&file);

                file.close();
            }
        }
        else
        {
            if(nSize<=XArchive::getCompressBufferSize())
            {
                XArchive::RECORD record=XArchive::getArchiveRecord(sRecordFileName,&g_listRecords);

                QByteArray baData=XArchives::decompress(g_sFileName,&record);

                QBuffer buffer;

                buffer.setBuffer(&baData);

                if(buffer.open(QIODevice::ReadOnly))
                {
                    _handleAction(action,&buffer);

                    buffer.close();
                }
            }
            else
            {
                // nSize
            }
        }
    }
}

void Archive_widget::_handleAction(Archive_widget::ACTION action, QIODevice *pDevice)
{
    if(action==ACTION_HEX)
    {
        DialogHex dialogHex(this,pDevice);

        dialogHex.exec();
    }
}

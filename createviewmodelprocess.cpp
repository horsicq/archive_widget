// copyright (c) 2020-2021 hors<horsicq@gmail.com>
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
#include "createviewmodelprocess.h"

CreateViewModelProcess::CreateViewModelProcess(QObject *pParent) : QObject(pParent)
{
    g_bIsStop=false;
}

void CreateViewModelProcess::setData(QString sFileName, QList<XArchive::RECORD> *pListArchiveRecords, QStandardItemModel **ppTreeModel, QStandardItemModel **ppTableModel, QSet<XBinary::FT> stFilterFileTypes)
{
    this->g_sFileName=sFileName;
    this->g_pListArchiveRecords=pListArchiveRecords;
    this->g_ppTreeModel=ppTreeModel;
    this->g_ppTableModel=ppTableModel;
    this->g_stFilterFileTypes=stFilterFileTypes;
}

void CreateViewModelProcess::stop()
{
     g_bIsStop=true;
}

void CreateViewModelProcess::process()
{
    QElapsedTimer scanTimer;
    scanTimer.start();

    *g_pListArchiveRecords=XArchives::getRecords(g_sFileName);

    qint64 nFileSize=XBinary::getSize(g_sFileName);

    int nNumberOfRecords=g_pListArchiveRecords->count();

    *g_ppTreeModel=new QStandardItemModel;
    (*g_ppTreeModel)->setColumnCount(2);

    *g_ppTableModel=new QStandardItemModel;
    (*g_ppTableModel)->setColumnCount(3);

    QString sBaseName=QFileInfo(g_sFileName).fileName();

    QStandardItem *pRootItemName=new QStandardItem;
    pRootItemName->setText(sBaseName);
    pRootItemName->setData(g_sFileName,Qt::UserRole+UR_PATH);
    pRootItemName->setData(nFileSize,Qt::UserRole+UR_SIZE);
    pRootItemName->setData(true,Qt::UserRole+UR_ISROOT);

    (*g_ppTreeModel)->setItem(0,0,pRootItemName);

    QStandardItem *pRootItemSize=new QStandardItem;
    pRootItemSize->setText(QString::number(nFileSize));
    pRootItemSize->setTextAlignment(Qt::AlignRight);

    (*g_ppTreeModel)->setItem(0,1,pRootItemSize);

    bool bFilter=g_stFilterFileTypes.count();

    QMap<QString,QStandardItem *> mapItems;

    for(int i=0,j=0;(i<nNumberOfRecords)&&(!g_bIsStop);i++)
    {
        XArchive::RECORD record=g_pListArchiveRecords->at(i);
        QString sRecordFileName=record.sFileName;

        bool bAdd=true;

        if(bFilter)
        {
            QByteArray baData=XArchives::decompress(g_sFileName,&record,true);

            QSet<XBinary::FT> stFT=XBinary::getFileTypes(&baData,true);

            bAdd=XBinary::isFileTypePresent(&stFT,&g_stFilterFileTypes);
        }

        if(bAdd)
        {
            int nNumberOfParts=sRecordFileName.count("/");

            for(int j=0;j<=nNumberOfParts;j++)
            {
                QString sPart=sRecordFileName.section("/",j,j);
                QString sRelPart;

                if(sPart!="")
                {
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
                            pItemName->setData(sRecordFileName,Qt::UserRole+UR_PATH);
                            pItemName->setData(record.nUncompressedSize,Qt::UserRole+UR_SIZE);
                            pItemName->setData(false,Qt::UserRole+UR_ISROOT);
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
                            pItemSize->setData(record.nUncompressedSize,Qt::DisplayRole);
                            pItemSize->setTextAlignment(Qt::AlignRight);

                            listItems.append(pItemSize);
                        }

                        pParent->appendRow(listItems);

                        mapItems.insert(sRelPart,pItemName);
                    }
                }
            }

            QList<QStandardItem *> listItems;

            QStandardItem *pItemNumber=new QStandardItem;
            pItemNumber->setData(j,Qt::DisplayRole);
            pItemNumber->setTextAlignment(Qt::AlignRight);
            pItemNumber->setData(sRecordFileName,Qt::UserRole+UR_PATH);
            pItemNumber->setData(record.nUncompressedSize,Qt::UserRole+UR_SIZE);
            pItemNumber->setData(false,Qt::UserRole+UR_ISROOT);
            listItems.append(pItemNumber);

            QStandardItem *pItemName=new QStandardItem;
            pItemName->setText(sRecordFileName);
            listItems.append(pItemName);

            QStandardItem *pItemSize=new QStandardItem;
            pItemSize->setData(record.nUncompressedSize,Qt::DisplayRole);
            pItemSize->setTextAlignment(Qt::AlignRight);
            listItems.append(pItemSize);

            (*g_ppTableModel)->appendRow(listItems);

            j++;
        }
    }

    (*g_ppTreeModel)->setHeaderData(0,Qt::Horizontal,tr("File"));
    (*g_ppTreeModel)->setHeaderData(1,Qt::Horizontal,tr("Size"));

    (*g_ppTableModel)->setHeaderData(0,Qt::Horizontal,"");
    (*g_ppTableModel)->setHeaderData(1,Qt::Horizontal,tr("File"));
    (*g_ppTableModel)->setHeaderData(2,Qt::Horizontal,tr("Size"));

    g_bIsStop=false;

    emit completed(scanTimer.elapsed());
}

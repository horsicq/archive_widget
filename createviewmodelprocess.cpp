// Copyright (c) 2020-2021 hors<horsicq@gmail.com>
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
    g_type=TYPE_UNKNOWN;
    g_bIsStop=false;
}

void CreateViewModelProcess::setData(TYPE type, QString sName, QList<XArchive::RECORD> *pListArchiveRecords, QStandardItemModel **ppTreeModel, QStandardItemModel **ppTableModel, QSet<XBinary::FT> stFilterFileTypes)
{
    this->g_type=type;
    this->g_sName=sName;
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

    if(g_type==TYPE_ARCHIVE)
    {
        *g_pListArchiveRecords=XArchives::getRecords(g_sName);
    }
    else if(g_type==TYPE_DIRECTORY)
    {
        *g_pListArchiveRecords=XArchives::getRecordsFromDirectory(g_sName);
    }

    qint64 nFileSize=XBinary::getSize(g_sName);

    qint32 nNumberOfRecords=g_pListArchiveRecords->count();

    *g_ppTreeModel=new QStandardItemModel;
    (*g_ppTreeModel)->setColumnCount(2);

    *g_ppTableModel=new QStandardItemModel;
    (*g_ppTableModel)->setColumnCount(3);

    QString sBaseName=QFileInfo(g_sName).fileName();

    QStandardItem *pRootItemName=new QStandardItem;
    pRootItemName->setText(sBaseName);
    pRootItemName->setData(g_sName,Qt::UserRole+UR_PATH);
    pRootItemName->setData(nFileSize,Qt::UserRole+UR_SIZE);
    pRootItemName->setData(true,Qt::UserRole+UR_ISROOT);

    (*g_ppTreeModel)->setItem(0,0,pRootItemName);

    QStandardItem *pRootItemSize=new QStandardItem;
    pRootItemSize->setText(QString::number(nFileSize));
    pRootItemSize->setTextAlignment(Qt::AlignRight);

    (*g_ppTreeModel)->setItem(0,1,pRootItemSize);

    bool bFilter=g_stFilterFileTypes.count();

    QMap<QString,QStandardItem *> mapItems;

    for(qint32 i=0,j=0;(i<nNumberOfRecords)&&(!g_bIsStop);i++)
    {
        XArchive::RECORD record=g_pListArchiveRecords->at(i);
        QString sRecordFileName=record.sFileName;

        bool bAdd=true;

        if(bFilter)
        {
            QSet<XBinary::FT> stFT;

            if(g_type==TYPE_ARCHIVE)
            {
                QByteArray baData=XArchives::decompress(g_sName,&record,true);

                stFT=XBinary::getFileTypes(&baData,true);
            }
            else if(g_type==TYPE_DIRECTORY)
            {
                stFT=XBinary::getFileTypes(sRecordFileName,true);
            }

            bAdd=XBinary::isFileTypePresent(&stFT,&g_stFilterFileTypes);
        }

        if(bAdd)
        {
            QString _sRecordFileName=sRecordFileName;

            while(_sRecordFileName.size()>1)
            {
                if(_sRecordFileName.at(0)==QChar('/'))
                {
                    _sRecordFileName=_sRecordFileName.mid(1,-1);
                }
                else
                {
                    break;
                }
            }

            qint32 nNumberOfParts=_sRecordFileName.count("/");

            for(qint32 k=0;k<=nNumberOfParts;k++)
            {
                QString sPart=_sRecordFileName.section("/",k,k);
                QString sRelPart;

                if(sPart!="")
                {
                    if(k!=nNumberOfParts)
                    {
                        sRelPart=_sRecordFileName.section("/",0,k);
                    }
                    else
                    {
                        sRelPart=_sRecordFileName;
                    }

                    if(!mapItems.contains(sRelPart))
                    {
                        QStandardItem *pItemName=new QStandardItem;
                        pItemName->setText(sPart);

                        if(k==(nNumberOfParts))
                        {
                            pItemName->setData(sRecordFileName,Qt::UserRole+UR_PATH);
                            pItemName->setData(record.nUncompressedSize,Qt::UserRole+UR_SIZE);
                            pItemName->setData(false,Qt::UserRole+UR_ISROOT);
                        }

                        QStandardItem *pParent=nullptr;

                        if(k==0)
                        {
                            pParent=pRootItemName;
                        }
                        else
                        {
                            pParent=mapItems.value(_sRecordFileName.section("/",0,k-1));
                        }

                        QList<QStandardItem *> listItems;

                        listItems.append(pItemName);

                        if(k==(nNumberOfParts))
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

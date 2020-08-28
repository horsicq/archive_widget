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
    this->sFileName=sFileName;

    QList<XArchive::RECORD> listRecords=XArchives::getRecords(sFileName);

    int nNumberOfRecords=listRecords.count();

    QAbstractItemModel *pOldModel=ui->treeViewArchive->model();

    QStandardItemModel *pNewModel=new QStandardItemModel;
    pNewModel->setColumnCount(2);

    QString sBaseName=QFileInfo(sFileName).fileName();

    QStandardItem *pRootItemName=new QStandardItem;
    pRootItemName->setText(sBaseName);

    pNewModel->setItem(0,0,pRootItemName);

    QStandardItem *pRootItemSize=new QStandardItem;
    pRootItemSize->setText(QString::number(XBinary::getSize(sFileName)));
    pRootItemSize->setTextAlignment(Qt::AlignRight);

    pNewModel->setItem(0,1,pRootItemSize);

    QMap<QString,QStandardItem *> mapItems;

    // TODO move to thread
    for(int i=0;i<nNumberOfRecords;i++)
    {
        XArchive::RECORD record=listRecords.at(i);

        QString sFileName=record.sFileName;

        int nNumberOfParts=sFileName.count("/");

        for(int j=0;j<=nNumberOfParts;j++)
        {
            QString sPart=sFileName.section("/",j,j);
            QString sRelPart;

            if(j!=nNumberOfParts)
            {
                sRelPart=sFileName.section("/",0,j);
            }
            else
            {
                sRelPart=sFileName;
            }

            if(!mapItems.contains(sRelPart))
            {
                QStandardItem *pItemName=new QStandardItem;
                pItemName->setText(sPart);

                QStandardItem *pParent=0;

                if(j==0)
                {
                    pParent=pRootItemName;
                }
                else
                {
                    pParent=mapItems.value(sFileName.section("/",0,j-1));
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

    ui->treeViewArchive->expandAll();

    delete pOldModel;
}

Archive_widget::~Archive_widget()
{
    delete ui;
}

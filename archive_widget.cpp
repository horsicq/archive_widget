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

    QAbstractItemModel *pOldModel=ui->treeViewArchive->model();
    QStandardItemModel *pNewModel=0;

    DialogCreateViewModel dialogCreateViewModel(this);

    dialogCreateViewModel.setData(sFileName,&g_listRecords,&pNewModel);

    dialogCreateViewModel.exec();

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

        bool bIsRoot=ui->treeViewArchive->model()->data(index,Qt::UserRole+CreateViewModelProcess::UR_ISROOT).toBool();
        QString sRecordFileName=ui->treeViewArchive->model()->data(index,Qt::UserRole+CreateViewModelProcess::UR_PATH).toString();

        if(sRecordFileName!="")
        {
            QSet<XBinary::FT> stFileTypes;

            if(bIsRoot)
            {
                stFileTypes=XBinary::getFileTypes(sRecordFileName,true);
            }
            else
            {
                XArchive::RECORD record=XArchive::getArchiveRecord(sRecordFileName,&g_listRecords);

                QByteArray baData=XArchives::decompress(g_sFileName,&record,true);
                stFileTypes=XBinary::getFileTypes(&baData,true);
            }

            QMenu contextMenu(this);

            QAction actionOpen(tr("Open"),this);

            if( stFileTypes.contains(XBinary::FT_MSDOS)||
                stFileTypes.contains(XBinary::FT_NE)||
                stFileTypes.contains(XBinary::FT_LE)||
                stFileTypes.contains(XBinary::FT_PE)||
                stFileTypes.contains(XBinary::FT_ELF)||
                stFileTypes.contains(XBinary::FT_MACH)||
                stFileTypes.contains(XBinary::FT_PNG)||
                stFileTypes.contains(XBinary::FT_JPEG)||
                stFileTypes.contains(XBinary::FT_GIF)||
                stFileTypes.contains(XBinary::FT_TIFF)||
                stFileTypes.contains(XBinary::FT_TEXT))
            {
                connect(&actionOpen, SIGNAL(triggered()), this, SLOT(openRecord()));
                contextMenu.addAction(&actionOpen);
            }

            QAction actionScan(tr("Scan"),this);
            connect(&actionScan, SIGNAL(triggered()), this, SLOT(scanRecord()));
            contextMenu.addAction(&actionScan);

            QAction actionHex(QString("Hex"),this);
            connect(&actionHex, SIGNAL(triggered()), this, SLOT(hexRecord()));
            contextMenu.addAction(&actionHex);

            QAction actionStrings(tr("Strings"),this);
            connect(&actionStrings, SIGNAL(triggered()), this, SLOT(stringsRecord()));
            contextMenu.addAction(&actionStrings);

            QAction actionEntropy(tr("Entropy"),this);
            connect(&actionEntropy, SIGNAL(triggered()), this, SLOT(entropyRecord()));
            contextMenu.addAction(&actionEntropy);

            QAction actionHash(tr("Hash"),this);
            connect(&actionHash, SIGNAL(triggered()), this, SLOT(hashRecord()));
            contextMenu.addAction(&actionHash);

            QAction actionDump(tr("Dump"),this);

            if(!bIsRoot)
            {
                connect(&actionDump, SIGNAL(triggered()), this, SLOT(dumpRecord()));
                contextMenu.addAction(&actionDump);
            }

            contextMenu.exec(ui->treeViewArchive->viewport()->mapToGlobal(pos));
        }
    }
}

void Archive_widget::openRecord()
{
    handleAction(ACTION_OPEN);
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

void Archive_widget::dumpRecord()
{
    handleAction(ACTION_DUMP);
}

void Archive_widget::handleAction(Archive_widget::ACTION action)
{
    QModelIndexList listIndexes=ui->treeViewArchive->selectionModel()->selectedIndexes();

    if(listIndexes.size()>0)
    {
        QModelIndex index=listIndexes.at(0);

        qint64 nSize=ui->treeViewArchive->model()->data(index,Qt::UserRole+CreateViewModelProcess::UR_SIZE).toLongLong();
        bool bIsRoot=ui->treeViewArchive->model()->data(index,Qt::UserRole+CreateViewModelProcess::UR_ISROOT).toBool();
        QString sRecordFileName=ui->treeViewArchive->model()->data(index,Qt::UserRole+CreateViewModelProcess::UR_PATH).toString();

        if(bIsRoot)
        {
            if(action==ACTION_OPEN)
            {
                _handleActionOpenFile(sRecordFileName);
            }
            else
            {
                QFile file;

                file.setFileName(sRecordFileName);

                if(file.open(QIODevice::ReadOnly))
                {
                    _handleActionDevice(action,&file);

                    file.close();
                }
            }
        }
        else
        {
            XArchive::RECORD record=XArchive::getArchiveRecord(sRecordFileName,&g_listRecords);

            if(action==ACTION_OPEN)
            {
                QTemporaryFile fileTemp;

                if(fileTemp.open())
                {
                    QString sTempFileName=fileTemp.fileName();

                    DialogUnpackFile dialogUnpackFile(this);

                    dialogUnpackFile.setData(g_sFileName,&record,sTempFileName);

                    if(dialogUnpackFile.exec()==QDialog::Accepted)
                    {
                        _handleActionOpenFile(sTempFileName);
                    }
                }
            }
            else if(action==ACTION_DUMP)
            {
                QString sSaveFileName=QFileInfo(g_sFileName).absolutePath()+QDir::separator()+QFileInfo(record.sFileName).fileName();

                sSaveFileName=QFileDialog::getSaveFileName(this,tr("Save file"),sSaveFileName,QFileInfo(record.sFileName).completeSuffix());

                if(sSaveFileName!="")
                {
                    DialogUnpackFile dialogUnpackFile(this);

                    dialogUnpackFile.setData(g_sFileName,&record,sSaveFileName);

                    if(dialogUnpackFile.exec()!=QDialog::Accepted)
                    {
                        QMessageBox::critical(this,tr("Error"),tr("Cannot save file"));
                    }
                }
            }
            else
            {
                if(nSize<=XArchive::getCompressBufferSize())
                {
                    QByteArray baData=XArchives::decompress(g_sFileName,&record);

                    QBuffer buffer;

                    buffer.setBuffer(&baData);

                    if(buffer.open(QIODevice::ReadOnly))
                    {
                        _handleActionDevice(action,&buffer);

                        buffer.close();
                    }
                }
                else
                {
                    QTemporaryFile fileTemp;

                    if(fileTemp.open())
                    {
                        QString sTempFileName=fileTemp.fileName();

                        DialogUnpackFile dialogUnpackFile(this);

                        dialogUnpackFile.setData(g_sFileName,&record,sTempFileName);

                        if(dialogUnpackFile.exec()==QDialog::Accepted)
                        {
                            QFile file;

                            file.setFileName(sTempFileName);

                            if(file.open(QIODevice::ReadOnly))
                            {
                                _handleActionDevice(action,&file);

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
    if(action==ACTION_SCAN)
    {
        DialogStaticScan dialogStaticScan(this,pDevice,true);

        dialogStaticScan.exec();
    }
    else if(action==ACTION_HEX)
    {
        DialogHex dialogHex(this,pDevice);

        dialogHex.exec();
    }
    else if(action==ACTION_STRINGS)
    {
        DialogSearchStrings dialogSearchStrings(this,pDevice,nullptr,true);

        dialogSearchStrings.exec();
    }
    else if(action==ACTION_ENTROPY)
    {
        DialogEntropy dialogEntropy(this,pDevice);

        dialogEntropy.exec();
    }
    else if(action==ACTION_HASH)
    {
        DialogHash dialogHash(this,pDevice);

        dialogHash.exec();
    }
}

void Archive_widget::_handleActionOpenFile(QString sFileName)
{
    QSet<XBinary::FT> stFileTypes=XBinary::getFileTypes(sFileName,true);

    if( stFileTypes.contains(XBinary::FT_PNG)||
        stFileTypes.contains(XBinary::FT_JPEG)||
        stFileTypes.contains(XBinary::FT_TIFF)||
        stFileTypes.contains(XBinary::FT_GIF))
    {
        DialogShowImage dialogShowImage(this,sFileName);

        dialogShowImage.exec();
    }
    else if(stFileTypes.contains(XBinary::FT_TEXT))
    {
        DialogShowText dialogShowText(this,sFileName);

        dialogShowText.exec();
    }
    else if( stFileTypes.contains(XBinary::FT_MSDOS)||
             stFileTypes.contains(XBinary::FT_NE)||
             stFileTypes.contains(XBinary::FT_LE)||
             stFileTypes.contains(XBinary::FT_PE)||
             stFileTypes.contains(XBinary::FT_ELF)||
             stFileTypes.contains(XBinary::FT_MACH))
    {
        QFile file;

        file.setFileName(sFileName);

        if(file.open(QIODevice::ReadOnly))
        {
            FW_DEF::OPTIONS options={};

            if(stFileTypes.contains(XBinary::FT_PE))
            {
                options.nStartType=SPE::TYPE_HEURISTICSCAN;

                DialogPE dialogPE(this);

                dialogPE.setData(&file,&options);

                dialogPE.exec();
            }
            else if(stFileTypes.contains(XBinary::FT_LE))
            {
                options.nStartType=SLE::TYPE_HEURISTICSCAN;

                DialogLE dialogLE(this);

                dialogLE.setData(&file,&options);

                dialogLE.exec();
            }
            else if(stFileTypes.contains(XBinary::FT_NE))
            {
                options.nStartType=SNE::TYPE_HEURISTICSCAN;

                DialogNE dialogNE(this);

                dialogNE.setData(&file,&options);

                dialogNE.exec();
            }
            else if(stFileTypes.contains(XBinary::FT_MSDOS))
            {
                options.nStartType=SMSDOS::TYPE_HEURISTICSCAN;

                DialogMSDOS dialogMSDOS(this);

                dialogMSDOS.setData(&file,&options);

                dialogMSDOS.exec();
            }
            else if(stFileTypes.contains(XBinary::FT_ELF))
            {
                options.nStartType=SELF::TYPE_HEURISTICSCAN;

                DialogELF dialogELF(this);

                dialogELF.setData(&file,&options);

                dialogELF.exec();
            }
            else if(stFileTypes.contains(XBinary::FT_MACH))
            {
                options.nStartType=SMACH::TYPE_HEURISTICSCAN;

                DialogMACH dialogMACH(this);

                dialogMACH.setData(&file,&options);

                dialogMACH.exec();
            }

            file.close();
        }
    }
}

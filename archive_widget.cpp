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
#include "archive_widget.h"
#include "ui_archive_widget.h"

Archive_widget::Archive_widget(QWidget *pParent) :
    QWidget(pParent),
    ui(new Ui::Archive_widget)
{
    ui->setupUi(this);

    g_pFilterTable=new QSortFilterProxyModel(this);

    ui->comboBoxType->addItem(tr("Tree"));
    ui->comboBoxType->addItem(tr("Table"));

    ui->groupBoxFilter->setEnabled(false);

    ui->comboBoxType->setCurrentIndex(0);

    setShortcuts(&g_scEmpty);
    // mb TODO registerShortcuts
}

void Archive_widget::setData(QString sFileName, FW_DEF::OPTIONS *pOptions, QWidget *pParent)
{
    this->g_sFileName=sFileName;
    this->g_pOptions=pOptions;

    ui->tableViewArchive->setSortingEnabled(false);

    QAbstractItemModel *pOldTreeModel=ui->treeViewArchive->model();
    QStandardItemModel *pNewTreeModel=0;

    QAbstractItemModel *pOldTableModel=g_pFilterTable->sourceModel();
    QStandardItemModel *pNewTableModel=0;

    g_pFilterTable->setSourceModel(0);
    ui->tableViewArchive->setModel(0);

    ui->comboBoxType->setCurrentIndex(0);
    ui->lineEditFilter->clear();

    DialogCreateViewModel dialogCreateViewModel(pParent);

    dialogCreateViewModel.setData(sFileName,&g_listRecords,&pNewTreeModel,&pNewTableModel);

    dialogCreateViewModel.exec();

    ui->treeViewArchive->setModel(pNewTreeModel);

    ui->treeViewArchive->header()->setSectionResizeMode(0,QHeaderView::Stretch);
    ui->treeViewArchive->header()->setSectionResizeMode(1,QHeaderView::Interactive);

    g_pFilterTable->setSourceModel(pNewTableModel);
    ui->tableViewArchive->setModel(g_pFilterTable);

    ui->tableViewArchive->horizontalHeader()->setSectionResizeMode(0,QHeaderView::Interactive);
    ui->tableViewArchive->horizontalHeader()->setSectionResizeMode(1,QHeaderView::Stretch);
    ui->tableViewArchive->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Interactive);

    delete pOldTreeModel;
    delete pOldTableModel;

    ui->tableViewArchive->setSortingEnabled(true);
    ui->tableViewArchive->sortByColumn(0,Qt::AscendingOrder);

    ui->tableViewArchive->setColumnWidth(0,20);

    ui->treeViewArchive->expand(pNewTreeModel->index(0,0));
}

void Archive_widget::setShortcuts(XShortcuts *pShortcuts)
{
    g_pShortcuts=pShortcuts;
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

        showContext(sRecordFileName,bIsRoot,ui->treeViewArchive->viewport()->mapToGlobal(pos));
    }
}

void Archive_widget::on_tableViewArchive_customContextMenuRequested(const QPoint &pos)
{
    QModelIndexList listIndexes=ui->tableViewArchive->selectionModel()->selectedIndexes();

    if(listIndexes.size()>0)
    {
        QModelIndex index=listIndexes.at(0);

        bool bIsRoot=ui->tableViewArchive->model()->data(index,Qt::UserRole+CreateViewModelProcess::UR_ISROOT).toBool();
        QString sRecordFileName=ui->tableViewArchive->model()->data(index,Qt::UserRole+CreateViewModelProcess::UR_PATH).toString();

        showContext(sRecordFileName,bIsRoot,ui->tableViewArchive->viewport()->mapToGlobal(pos));
    }
}

void Archive_widget::showContext(QString sRecordFileName, bool bIsRoot, QPoint point)
{
    if(sRecordFileName!="")
    {
        QMenu contextMenu(this);

        QAction actionOpen(tr("Open"),this);

        if(isOpenAvailable(sRecordFileName,bIsRoot))
        {
            actionOpen.setShortcut(g_pShortcuts->getShortcut(XShortcuts::ID_ARCHIVE_OPEN));
            connect(&actionOpen, SIGNAL(triggered()), this, SLOT(openRecord()));
            contextMenu.addAction(&actionOpen);
        }

        QAction actionScan(tr("Scan"),this);
        actionScan.setShortcut(g_pShortcuts->getShortcut(XShortcuts::ID_ARCHIVE_SCAN));
        connect(&actionScan, SIGNAL(triggered()), this, SLOT(scanRecord()));
        contextMenu.addAction(&actionScan);

        QAction actionHex(tr("Hex"),this);
        actionHex.setShortcut(g_pShortcuts->getShortcut(XShortcuts::ID_ARCHIVE_HEX));
        connect(&actionHex, SIGNAL(triggered()), this, SLOT(hexRecord()));
        contextMenu.addAction(&actionHex);

        QAction actionStrings(tr("Strings"),this);
        actionStrings.setShortcut(g_pShortcuts->getShortcut(XShortcuts::ID_ARCHIVE_STRINGS));
        connect(&actionStrings, SIGNAL(triggered()), this, SLOT(stringsRecord()));
        contextMenu.addAction(&actionStrings);

        QAction actionEntropy(tr("Entropy"),this);
        actionEntropy.setShortcut(g_pShortcuts->getShortcut(XShortcuts::ID_ARCHIVE_ENTROPY));
        connect(&actionEntropy, SIGNAL(triggered()), this, SLOT(entropyRecord()));
        contextMenu.addAction(&actionEntropy);

        QAction actionHash(tr("Hash"),this);
        actionHash.setShortcut(g_pShortcuts->getShortcut(XShortcuts::ID_ARCHIVE_HASH));
        connect(&actionHash, SIGNAL(triggered()), this, SLOT(hashRecord()));
        contextMenu.addAction(&actionHash);

        QMenu menuCopy(tr("Copy"),this);
        QAction actionCopyFilename(tr("Filename"),this);
        actionCopyFilename.setShortcut(g_pShortcuts->getShortcut(XShortcuts::ID_ARCHIVE_COPYFILENAME));
        connect(&actionCopyFilename, SIGNAL(triggered()), this, SLOT(copyFileName()));
        menuCopy.addAction(&actionCopyFilename);
        contextMenu.addMenu(&menuCopy);

        QAction actionDump(tr("Dump to file"),this);

        if(!bIsRoot)
        {
            actionDump.setShortcut(g_pShortcuts->getShortcut(XShortcuts::ID_ARCHIVE_DUMPTOFILE));
            connect(&actionDump, SIGNAL(triggered()), this, SLOT(dumpRecord()));
            contextMenu.addAction(&actionDump);
        }

        contextMenu.exec(point);
    }
}

bool Archive_widget::isOpenAvailable(QString sRecordFileName, bool bIsRoot)
{
    bool bResult=false;

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

    if( stFileTypes.contains(XBinary::FT_MSDOS)||
        stFileTypes.contains(XBinary::FT_NE)||
        stFileTypes.contains(XBinary::FT_LE)||
        stFileTypes.contains(XBinary::FT_PE)||
        stFileTypes.contains(XBinary::FT_ELF)||
        stFileTypes.contains(XBinary::FT_DEX)||
        stFileTypes.contains(XBinary::FT_MACHO)||
        stFileTypes.contains(XBinary::FT_PNG)||
        stFileTypes.contains(XBinary::FT_JPEG)||
        stFileTypes.contains(XBinary::FT_GIF)||
        stFileTypes.contains(XBinary::FT_TIFF)||
        stFileTypes.contains(XBinary::FT_TEXT)||
        stFileTypes.contains(XBinary::FT_ANDROIDXML))
    {
        bResult=true;
    }

    return bResult;
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

void Archive_widget::copyFileName()
{
    handleAction(ACTION_COPYFILENAME);
}

void Archive_widget::dumpRecord()
{
    handleAction(ACTION_DUMP);
}

void Archive_widget::handleAction(Archive_widget::ACTION action)
{
    qint64 nSize=0;
    bool bIsRoot=false;
    QString sRecordFileName;

    if(ui->stackedWidgetArchive->currentIndex()==0)
    {
        QModelIndexList listIndexes=ui->treeViewArchive->selectionModel()->selectedIndexes();

        if(listIndexes.size()>0)
        {
            QModelIndex index=listIndexes.at(0);

            nSize=ui->treeViewArchive->model()->data(index,Qt::UserRole+CreateViewModelProcess::UR_SIZE).toLongLong();
            bIsRoot=ui->treeViewArchive->model()->data(index,Qt::UserRole+CreateViewModelProcess::UR_ISROOT).toBool();
            sRecordFileName=ui->treeViewArchive->model()->data(index,Qt::UserRole+CreateViewModelProcess::UR_PATH).toString();
        }
    }
    else if(ui->stackedWidgetArchive->currentIndex()==1)
    {
        QModelIndexList listIndexes=ui->tableViewArchive->selectionModel()->selectedIndexes();

        if(listIndexes.size()>0)
        {
            QModelIndex index=listIndexes.at(0);

            nSize=ui->tableViewArchive->model()->data(index,Qt::UserRole+CreateViewModelProcess::UR_SIZE).toLongLong();
            bIsRoot=ui->tableViewArchive->model()->data(index,Qt::UserRole+CreateViewModelProcess::UR_ISROOT).toBool();
            sRecordFileName=ui->tableViewArchive->model()->data(index,Qt::UserRole+CreateViewModelProcess::UR_PATH).toString();
        }
    }

    if(sRecordFileName!="")
    {
        if(bIsRoot)
        {
            if(action==ACTION_OPEN)
            {
                _handleActionOpenFile(sRecordFileName,sRecordFileName);
            }
            else if(action==ACTION_COPYFILENAME)
            {
                QGuiApplication::clipboard()->setText(sRecordFileName);
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
                        _handleActionOpenFile(sTempFileName,record.sFileName);
                    }
                }
            }
            else if(action==ACTION_COPYFILENAME)
            {
                QGuiApplication::clipboard()->setText(record.sFileName);
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
        DialogHexView dialogHexView(this,pDevice);
        dialogHexView.setShortcuts(g_pShortcuts);

        dialogHexView.exec();
    }
    else if(action==ACTION_STRINGS)
    {
        MultiSearch::OPTIONS stringsOptions={};
        stringsOptions.bMenu_Hex=true;
        stringsOptions.bAnsi=true;
        stringsOptions.bUnicode=true;

        DialogSearchStrings dialogSearchStrings(this,pDevice,stringsOptions,true);
        dialogSearchStrings.setShortcuts(g_pShortcuts);

        dialogSearchStrings.exec();
    }
    else if(action==ACTION_ENTROPY)
    {
        DialogEntropy dialogEntropy(this,pDevice);
        dialogEntropy.setShortcuts(g_pShortcuts);

        dialogEntropy.exec();
    }
    else if(action==ACTION_HASH)
    {
        DialogHash dialogHash(this,pDevice);
        dialogHash.setShortcuts(g_pShortcuts);

        dialogHash.exec();
    }
}

void Archive_widget::_handleActionOpenFile(QString sFileName, QString sTitle)
{
    QSet<XBinary::FT> stFileTypes=XBinary::getFileTypes(sFileName,true);

    if( stFileTypes.contains(XBinary::FT_PNG)||
        stFileTypes.contains(XBinary::FT_JPEG)||
        stFileTypes.contains(XBinary::FT_TIFF)||
        stFileTypes.contains(XBinary::FT_GIF))
    {
        DialogShowImage dialogShowImage(this,sFileName,sTitle);

        dialogShowImage.exec();
    }
    else if(stFileTypes.contains(XBinary::FT_TEXT))
    {
        DialogShowText dialogShowText(this,sTitle);

        dialogShowText.setData(sFileName,DialogShowText::TYPE_FILECONTENT);

        dialogShowText.exec();
    }
    else if(stFileTypes.contains(XBinary::FT_ANDROIDXML))
    {
        QString sString=XAndroidBinary::getDecoded(sFileName);

        DialogShowText dialogShowText(this,sTitle);

        dialogShowText.setData(sString,DialogShowText::TYPE_PLAINTEXT);

        dialogShowText.exec();
    }
    else if( stFileTypes.contains(XBinary::FT_MSDOS)||
             stFileTypes.contains(XBinary::FT_NE)||
             stFileTypes.contains(XBinary::FT_LE)||
             stFileTypes.contains(XBinary::FT_PE)||
             stFileTypes.contains(XBinary::FT_ELF)||
             stFileTypes.contains(XBinary::FT_MACHO)||
             stFileTypes.contains(XBinary::FT_DEX))
    {
        QFile file;

        file.setFileName(sFileName);

        if(file.open(QIODevice::ReadOnly))
        {
            FW_DEF::OPTIONS options=*g_pOptions; // TODO options from setData
            options.sTitle=sTitle;

            if(stFileTypes.contains(XBinary::FT_PE))
            {
                options.nStartType=SPE::TYPE_IMAGE_FILE_HEADER;

                DialogPE dialogPE(this);

                dialogPE.setData(&file,options);

                dialogPE.exec();
            }
            else if(stFileTypes.contains(XBinary::FT_LE))
            {
                options.nStartType=SLE::TYPE_VXD_HEADER;

                DialogLE dialogLE(this);

                dialogLE.setData(&file,options);

                dialogLE.exec();
            }
            else if(stFileTypes.contains(XBinary::FT_NE))
            {
                options.nStartType=SNE::TYPE_OS2_HEADER;

                DialogNE dialogNE(this);

                dialogNE.setData(&file,options);

                dialogNE.exec();
            }
            else if(stFileTypes.contains(XBinary::FT_MSDOS))
            {
                options.nStartType=SMSDOS::TYPE_DOS_HEADER;

                DialogMSDOS dialogMSDOS(this);

                dialogMSDOS.setData(&file,options);

                dialogMSDOS.exec();
            }
            else if(stFileTypes.contains(XBinary::FT_ELF))
            {
                options.nStartType=SELF::TYPE_Elf_Ehdr;

                DialogELF dialogELF(this);

                dialogELF.setData(&file,options);

                dialogELF.exec();
            }
            else if(stFileTypes.contains(XBinary::FT_MACHO))
            {
                options.nStartType=SMACH::TYPE_mach_header;

                DialogMACH dialogMACH(this);

                dialogMACH.setData(&file,options);

                dialogMACH.exec();
            }
            else if(stFileTypes.contains(XBinary::FT_DEX))
            {
                options.nStartType=SDEX::TYPE_HEADER;

                DialogDEX dialogDEX(this);

                dialogDEX.setData(&file,options);

                dialogDEX.exec();
            }

            file.close();
        }
    }
}

void Archive_widget::on_comboBoxType_currentIndexChanged(int nIndex)
{
    ui->stackedWidgetArchive->setCurrentIndex(nIndex);

    ui->groupBoxFilter->setEnabled(nIndex);
}

void Archive_widget::on_lineEditFilter_textChanged(const QString &sString)
{
    g_pFilterTable->setFilterRegExp(sString);
    g_pFilterTable->setFilterCaseSensitivity(Qt::CaseInsensitive);
    g_pFilterTable->setFilterKeyColumn(1);
}

void Archive_widget::on_treeViewArchive_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index)

    QModelIndexList listIndexes=ui->treeViewArchive->selectionModel()->selectedIndexes();

    if(listIndexes.size()>0)
    {
        QModelIndex _index=listIndexes.at(0);

        bool bIsRoot=ui->treeViewArchive->model()->data(_index,Qt::UserRole+CreateViewModelProcess::UR_ISROOT).toBool();
        QString sRecordFileName=ui->treeViewArchive->model()->data(_index,Qt::UserRole+CreateViewModelProcess::UR_PATH).toString();

        if(isOpenAvailable(sRecordFileName,bIsRoot))
        {
            handleAction(ACTION_OPEN);
        }
        else
        {
            handleAction(ACTION_HEX);
        }
    }
}

void Archive_widget::on_tableViewArchive_doubleClicked(const QModelIndex &index)
{
    Q_UNUSED(index)

    QModelIndexList listIndexes=ui->tableViewArchive->selectionModel()->selectedIndexes();

    if(listIndexes.size()>0)
    {
        QModelIndex _index=listIndexes.at(0);

        bool bIsRoot=ui->tableViewArchive->model()->data(_index,Qt::UserRole+CreateViewModelProcess::UR_ISROOT).toBool();
        QString sRecordFileName=ui->tableViewArchive->model()->data(_index,Qt::UserRole+CreateViewModelProcess::UR_PATH).toString();

        if(isOpenAvailable(sRecordFileName,bIsRoot))
        {
            handleAction(ACTION_OPEN);
        }
        else
        {
            handleAction(ACTION_HEX);
        }
    }
}

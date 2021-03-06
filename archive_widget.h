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
#ifndef ARCHIVE_WIDGET_H
#define ARCHIVE_WIDGET_H

#include "xarchives.h"
#include "xandroidbinary.h"
#include "dialogentropy.h"
#include "dialoghash.h"
#include "dialogsearchstrings.h"
#include "dialoghexview.h"
#include "dialogstaticscan.h"
#include "dialogcreateviewmodel.h"
#include "dialogunpackfile.h"
#include "dialogshowimage.h"
#include "dialogshowtext.h"
#include "dialogmsdos.h"
#include "dialogne.h"
#include "dialogle.h"
#include "dialogpe.h"
#include "dialogelf.h"
#include "dialogmach.h"
#include "dialogdex.h"

namespace Ui {
class Archive_widget;
}

class Archive_widget : public XShortcutsWidget
{
    Q_OBJECT

    enum ACTION
    {
        ACTION_OPEN=0,
        ACTION_SCAN,
        ACTION_HEX,
        ACTION_STRINGS,
        ACTION_ENTROPY,
        ACTION_HASH,
        ACTION_COPYFILENAME,
        ACTION_DUMP
    };

public:
    explicit Archive_widget(QWidget *pParent=nullptr);
    void setFileName(QString sFileName,FW_DEF::OPTIONS options,QSet<XBinary::FT> stAvailableOpenFileTypes,QWidget *pParent=nullptr); // TODO options for Viewers TODO Device
    void setDirectoryName(QString sDirectoryName,FW_DEF::OPTIONS options,QSet<XBinary::FT> stAvailableOpenFileTypes,QWidget *pParent=nullptr);
    void setData(CreateViewModelProcess::TYPE type,QString sName,FW_DEF::OPTIONS options,QSet<XBinary::FT> stAvailableOpenFileTypes,QWidget *pParent=nullptr);
    void setShortcuts(XShortcuts *pShortcuts);
    QString getCurrentRecordFileName();
    ~Archive_widget();

public slots:
    void openRecord();

private slots:
    void on_treeViewArchive_customContextMenuRequested(const QPoint &pos);
    void on_tableViewArchive_customContextMenuRequested(const QPoint &pos);
    void showContext(QString sRecordFileName,bool bIsRoot,QPoint point);
    bool isOpenAvailable(QString sRecordFileName,bool bIsRoot);

    void scanRecord();
    void hexRecord();
    void stringsRecord();
    void entropyRecord();
    void hashRecord();
    void copyFileName();
    void dumpRecord();

    void handleAction(ACTION action);
    void _handleActionDevice(ACTION action,QIODevice *pDevice);
    void _handleActionOpenFile(QString sFileName,QString sTitle,bool bReadWrite);
    void on_comboBoxType_currentIndexChanged(int nIndex);
    void on_lineEditFilter_textChanged(const QString &sString);
    void on_treeViewArchive_doubleClicked(const QModelIndex &index);
    void on_tableViewArchive_doubleClicked(const QModelIndex &index);

    void onTreeElement_selected(const QItemSelection &selected,const QItemSelection &prev); // TrackSelection
    void onTableElement_selected(const QItemSelection &selected,const QItemSelection &prev); // TrackSelection

protected:
    virtual void registerShortcuts(bool bState);

signals:
    void openAvailable(bool bState);

private:
    Ui::Archive_widget *ui;
    CreateViewModelProcess::TYPE g_type;
    QString g_sName;
    FW_DEF::OPTIONS g_options;
    QList<XArchive::RECORD> g_listRecords;
    QSortFilterProxyModel *g_pFilterTable;
    QSet<XBinary::FT> g_stAvailableOpenFileTypes;
    qint64 g_nCurrentFileSize;
    bool g_bCurrentFileIsRoot;
    QString g_sCurrentRecordFileName;
};

#endif // ARCHIVE_WIDGET_H

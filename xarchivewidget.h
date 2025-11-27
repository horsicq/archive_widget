/* Copyright (c) 2020-2025 hors<horsicq@gmail.com>
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
#ifndef XARCHIVEWIDGET_H
#define XARCHIVEWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QSortFilterProxyModel>
#include <QMenu>
#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QTemporaryFile>
#include "xarchive.h"
#include "xshortcutswidget.h"

namespace Ui {
class XArchiveWidget;
}

class XArchiveWidget : public XShortcutsWidget {
    Q_OBJECT

    enum ACTION {
        ACTION_HEX = 0,
        ACTION_STRINGS,
        ACTION_ENTROPY,
        ACTION_HASH,
        ACTION_COPYFILENAME,
        ACTION_DUMP
    };

public:
    explicit XArchiveWidget(QWidget *pParent = nullptr);
    ~XArchiveWidget();

    void setData(XBinary::FT fileType, QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1,);
    QString getCurrentRecordFileName();
    virtual void adjustView();
    virtual void reloadData(bool bSaveSelection);

private slots:
    void on_tableViewRecords_customContextMenuRequested(const QPoint &pos);
    void showContext(const QString &sRecordFileName, QPoint point);
    void hexRecord();
    void stringsRecord();
    void entropyRecord();
    void hashRecord();
    void copyFileName();
    void dumpRecord();
    void handleAction(ACTION action);
    void on_lineEditFilter_textChanged(const QString &sString);
    void on_tableViewRecords_doubleClicked(const QModelIndex &index);
    void onTableElement_selected(const QItemSelection &itemSelected, const QItemSelection &itemDeselected);

protected:
    virtual void registerShortcuts(bool bState);

private:
    void loadRecords();
    void setupTableView();

private:
    Ui::XArchiveWidget *ui;
    QIODevice *m_pDevice;
    XBinary::FT m_fileType;
    QList<XArchive::RECORD> m_listRecords;
    QStandardItemModel *m_pModel;
    QSortFilterProxyModel *m_pFilterModel;
    QString m_sCurrentRecordFileName;
    qint64 m_nCurrentFileSize;
};

#endif  // XARCHIVEWIDGET_H

/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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
#ifndef ARCHIVEEXPLORERWIDGET_H
#define ARCHIVEEXPLORERWIDGET_H

#include <QMenu>
#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QFileDialog>
#include <QTemporaryFile>
#include <QStringList>
#include "xarchives.h"
#include "xmodel_archiverecords.h"
#include "xshortcutswidget.h"

namespace Ui {
class ArchiveExplorerWidget;
}

class ArchiveExplorerWidget : public XShortcutsWidget {
    Q_OBJECT

public:
    explicit ArchiveExplorerWidget(QWidget *pParent = nullptr);
    ~ArchiveExplorerWidget();

    void setData(XBinary::FT fileType, QIODevice *pDevice, bool bIsImage = false, XADDR nModuleAddress = -1);
    QString getCurrentRecordFileName();
    const QList<XBinary::ARCHIVERECORD> *getArchiveRecords() const;
    virtual void adjustView();
    virtual void reloadData(bool bSaveSelection);

signals:
    void recordsLoaded(qint32 nNumberOfRecords);
    void currentRecordChanged(const QString &sRecordFileName, qint64 nFileSize);
    void extractAllRequested();
    void testRequested();

private slots:
    void on_toolButtonExtractAll_clicked();
    void on_toolButtonTest_clicked();
    void on_checkBoxAdvanced_toggled(bool bChecked);
    void on_tableViewRecords_customContextMenuRequested(const QPoint &pos);
    void showContext(const QString &sRecordFileName, QPoint point);
    void openRecord();
    void extractRecord();
    void copyFileName();
    void copyRecordPath();
    void copyRecordDetails();
    void showRecordProperties();
    void refreshRecords();
    void on_tableViewRecords_doubleClicked(const QModelIndex &index);
    void onCurrentRecordChanged(const QModelIndex &current, const QModelIndex &previous);

protected:
    virtual void registerShortcuts(bool bState);

private:
    qint32 getCurrentRecordIndex() const;
    bool extractRecordToDevice(qint32 nRow, QIODevice *pOutputDevice);
    bool extractRecordToFile(qint32 nRow, const QString &sFileName);
    QString getRecordDetails(const XBinary::ARCHIVERECORD &record) const;
    void updateActions();
    void loadRecords();

private:
    Ui::ArchiveExplorerWidget *ui;
    QIODevice *m_pDevice;
    XBinary::FT m_fileType;
    QList<XBinary::ARCHIVERECORD> m_listArchiveRecords;
    XModel_ArchiveRecords *m_pModel;
    QString m_sCurrentRecordFileName;
    qint64 m_nCurrentFileSize;
    bool m_bAdvanced;
    QStringList m_listTemporaryFiles;
};

#endif  // ARCHIVEEXPLORERWIDGET_H

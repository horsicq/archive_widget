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
#ifndef DIALOGARCHIVE_H
#define DIALOGARCHIVE_H

#include <QDialog>

#include "archive_widget.h"

// TODO DialogShortcuts !!!
namespace Ui {
class DialogArchive;
}

class DialogArchive : public XShortcutsDialog {
    Q_OBJECT

public:
    explicit DialogArchive(QWidget *pParent = nullptr);
    ~DialogArchive();

    virtual void adjustView();

    void setFileName(const QString &sFileName, XBinary::FT fileType, const FW_DEF::OPTIONS &options, const QSet<XBinary::FT> &stAvailableFileTypes);
    void setDevice(QIODevice *pDevice, XBinary::FT fileType, const FW_DEF::OPTIONS &options, QSet<XBinary::FT> stAvailableFileTypes);
    void setDirectory(const QString &sDirectoryName, const FW_DEF::OPTIONS &options, QSet<XBinary::FT> stAvailableFileTypes);
    void setGlobal(XShortcuts *pShortcuts, XOptions *pXOptions);
    QString getCurrentRecordFileName();

private slots:
    void on_pushButtonClose_clicked();
    void on_pushButtonOpen_clicked();

protected:
    virtual void registerShortcuts(bool bState);

private:
    Ui::DialogArchive *ui;
    QString m_sCurrentRecordFileName;
    FW_DEF::OPTIONS m_options;
};

#endif  // DIALOGARCHIVE_H

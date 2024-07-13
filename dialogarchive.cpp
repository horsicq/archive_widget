/* Copyright (c) 2020-2024 hors<horsicq@gmail.com>
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
#include "dialogarchive.h"

#include "ui_dialogarchive.h"

DialogArchive::DialogArchive(QWidget *pParent) : XShortcutsDialog(pParent, true), ui(new Ui::DialogArchive)
{
    ui->setupUi(this);

    g_options = {};
}

DialogArchive::~DialogArchive()
{
    delete ui;
}

void DialogArchive::adjustView()
{
}

void DialogArchive::setFileName(const QString &sFileName, XBinary::FT fileType, const FW_DEF::OPTIONS &options, const QSet<XBinary::FT> &stAvailableFileTypes)
{
    if (options.sTitle != "") {
        setWindowTitle(options.sTitle);
    }

    g_options = options;
    ui->widget->setFileName(sFileName, fileType, options, stAvailableFileTypes);
}

void DialogArchive::setDevice(QIODevice *pDevice, XBinary::FT fileType, const FW_DEF::OPTIONS &options, QSet<XBinary::FT> stAvailableFileTypes)
{
    setFileName(XBinary::getDeviceFileName(pDevice), fileType, options, stAvailableFileTypes);
}

void DialogArchive::setDirectory(const QString &sDirectoryName, const FW_DEF::OPTIONS &options, QSet<XBinary::FT> stAvailableFileTypes)
{
    if (options.sTitle != "") {
        setWindowTitle(options.sTitle);
    }

    g_options = options;
    ui->widget->setDirectoryName(sDirectoryName, options, stAvailableFileTypes);
}

void DialogArchive::setGlobal(XShortcuts *pShortcuts, XOptions *pXOptions)
{
    ui->widget->setGlobal(pShortcuts, pXOptions);
}

QString DialogArchive::getCurrentRecordFileName()
{
    return g_sCurrentRecordFileName;
}

void DialogArchive::on_pushButtonClose_clicked()
{
    reject();
}

void DialogArchive::on_pushButtonOpen_clicked()
{
    g_sCurrentRecordFileName = ui->widget->getCurrentRecordFileName();

    if (!g_options.bNoWindowOpen) {
        ui->widget->openRecord();
    } else {
        accept();
    }
}

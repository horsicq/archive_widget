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
#ifndef UNPACKFILEPROCESS_H
#define UNPACKFILEPROCESS_H

#include "xarchives.h"

class UnpackFileProcess : public QObject {
    Q_OBJECT

public:
    explicit UnpackFileProcess(QObject *pParent = nullptr);

    void setData(const QString &sFileName, XArchive::RECORD *pRecord, const QString &sResultFileName, XBinary::PDSTRUCT *pPdStruct);
    void setData(const QString &sFileName, const QString &sResultFileFolder, XBinary::PDSTRUCT *pPdStruct);
    void setData(QIODevice *pDevice, XArchive::RECORD *pRecord, const QString &sResultFileName, XBinary::PDSTRUCT *pPdStruct);
    void setData(QIODevice *pDevice, const QString &sResultFileFolder, XBinary::PDSTRUCT *pPdStruct);

signals:
    void errorMessage(const QString &sText);
    void completed(qint64 nElapsed);

public slots:
    void process();

private:
    QString g_sFileName;
    QIODevice *g_pDevice;
    XArchive::RECORD *g_pRecord;
    QString g_sResultFileName;
    QString g_sResultFileFolder;
    XBinary::PDSTRUCT *g_pPdStruct;
};

#endif  // UNPACKFILEPROCESS_H

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
#include "unpackfileprocess.h"

UnpackFileProcess::UnpackFileProcess(QObject *pParent) : QObject(pParent)
{
    m_pDevice = nullptr;
    m_pRecord = nullptr;
    m_pPdStruct = nullptr;
}

void UnpackFileProcess::setData(const QString &sFileName, XArchive::RECORD *pRecord, const QString &sResultFileName, XBinary::PDSTRUCT *pPdStruct)
{
    m_sFileName = sFileName;
    m_pRecord = pRecord;
    m_sResultFileName = sResultFileName;
    m_pPdStruct = pPdStruct;
}

void UnpackFileProcess::setData(const QString &sFileName, const QString &sResultFileFolder, XBinary::PDSTRUCT *pPdStruct)
{
    m_sFileName = sFileName;
    m_sResultFileFolder = sResultFileFolder;
    m_pPdStruct = pPdStruct;
}

void UnpackFileProcess::setData(QIODevice *pDevice, XArchive::RECORD *pRecord, const QString &sResultFileName, XBinary::PDSTRUCT *pPdStruct)
{
    m_pDevice = pDevice;
    m_pRecord = pRecord;
    m_sResultFileName = sResultFileName;
    m_pPdStruct = pPdStruct;
}

void UnpackFileProcess::setData(QIODevice *pDevice, const QString &sResultFileFolder, XBinary::PDSTRUCT *pPdStruct)
{
    m_pDevice = pDevice;
    m_sResultFileFolder = sResultFileFolder;
    m_pPdStruct = pPdStruct;
}

void UnpackFileProcess::process()
{
    QElapsedTimer scanTimer;
    scanTimer.start();

    qint32 _nFreeIndex = XBinary::getFreeIndex(m_pPdStruct);
    XBinary::setPdStructInit(m_pPdStruct, _nFreeIndex, 0);

    bool bResult = false;

    if (m_sFileName != "") {
        if (m_sResultFileName != "") {
            bResult = XArchives::decompressToFile(m_sFileName, m_pRecord, m_sResultFileName,
                                                  m_pPdStruct);  // TODO Error signals
        } else if (m_sResultFileFolder != "") {
            bResult = XArchives::decompressToFolder(m_sFileName, m_sResultFileFolder, m_pPdStruct);  // TODO Error signals
        }
    } else if (m_pDevice) {
        if (m_sResultFileName != "") {
            bResult = XArchives::decompressToFile(m_pDevice, m_pRecord, m_sResultFileName,
                                                  m_pPdStruct);  // TODO Error signals
        } else if (m_sResultFileFolder != "") {
            bResult = XArchives::decompressToFolder(m_pDevice, m_sResultFileFolder, m_pPdStruct);  // TODO Error signals
        }
    }

    if (!(bResult)) {
        XBinary::setPdStructStopped(m_pPdStruct);
    }

    XBinary::setPdStructFinished(m_pPdStruct, _nFreeIndex);

    emit completed(scanTimer.elapsed());
}

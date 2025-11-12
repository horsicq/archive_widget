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
#include "dialogunpackfile.h"

DialogUnpackFile::DialogUnpackFile(QWidget *pParent) : XDialogProcess(pParent)
{
    m_pUnpackFileProcess = new UnpackFileProcess;
    m_pThread = new QThread;

    m_pUnpackFileProcess->moveToThread(m_pThread);

    connect(m_pThread, SIGNAL(started()), m_pUnpackFileProcess, SLOT(process()));
    connect(m_pUnpackFileProcess, SIGNAL(completed(qint64)), this, SLOT(onCompleted(qint64)));
}

DialogUnpackFile::~DialogUnpackFile()
{
    stop();
    waitForFinished();

    m_pThread->quit();
    m_pThread->wait();

    delete m_pThread;
    delete m_pUnpackFileProcess;
}

void DialogUnpackFile::setData(const QString &sFileName, XArchive::RECORD *pRecord, const QString &sResultFileName)
{
    m_pUnpackFileProcess->setData(sFileName, pRecord, sResultFileName, getPdStruct());
    m_pThread->start();
}

void DialogUnpackFile::setData(const QString &sFileName, const QString &sResultFileFolder)
{
    m_pUnpackFileProcess->setData(sFileName, sResultFileFolder, getPdStruct());
    m_pThread->start();
}

void DialogUnpackFile::setData(QIODevice *pDevice, XArchive::RECORD *pRecord, const QString &sResultFileName)
{
    m_pUnpackFileProcess->setData(pDevice, pRecord, sResultFileName, getPdStruct());
    m_pThread->start();
}

void DialogUnpackFile::setData(QIODevice *pDevice, const QString &sResultFileFolder)
{
    m_pUnpackFileProcess->setData(pDevice, sResultFileFolder, getPdStruct());
    m_pThread->start();
}

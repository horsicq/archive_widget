/* Copyright (c) 2020-2023 hors<horsicq@gmail.com>
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

#include "ui_dialogunpackfile.h"

DialogUnpackFile::DialogUnpackFile(QWidget *pParent) : XDialogProcess(pParent)
{
    g_pUnpackFileProcess = new UnpackFileProcess;
    g_pThread = new QThread;

    g_pUnpackFileProcess->moveToThread(g_pThread);

    connect(g_pThread, SIGNAL(started()), g_pUnpackFileProcess, SLOT(process()));
    connect(g_pUnpackFileProcess, SIGNAL(completed(qint64)), this, SLOT(onCompleted(qint64)));
}

DialogUnpackFile::~DialogUnpackFile()
{
    stop();
    waitForFinished();

    g_pThread->quit();
    g_pThread->wait();

    delete g_pThread;
    delete g_pUnpackFileProcess;
}

void DialogUnpackFile::setData(const QString &sFileName, XArchive::RECORD *pRecord, QString sResultFileName)
{
    g_pUnpackFileProcess->setData(sFileName, pRecord, sResultFileName, getPdStruct());
    g_pThread->start();
}

void DialogUnpackFile::setData(QString sFileName, QString sResultFileFolder)
{
    g_pUnpackFileProcess->setData(sFileName, sResultFileFolder, getPdStruct());
    g_pThread->start();
}

void DialogUnpackFile::setData(QIODevice *pDevice, XArchive::RECORD *pRecord, QString sResultFileName)
{
    g_pUnpackFileProcess->setData(pDevice, pRecord, sResultFileName, getPdStruct());
    g_pThread->start();
}

void DialogUnpackFile::setData(QIODevice *pDevice, const QString &sResultFileFolder)
{
    g_pUnpackFileProcess->setData(pDevice, sResultFileFolder, getPdStruct());
    g_pThread->start();
}

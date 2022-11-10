/* Copyright (c) 2020-2022 hors<horsicq@gmail.com>
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
#include "dialogcreateviewmodel.h"

#include "ui_dialogcreateviewmodel.h"

DialogCreateViewModel::DialogCreateViewModel(QWidget *pParent)
    : QDialog(pParent), ui(new Ui::DialogCreateViewModel) {
    ui->setupUi(this);

    pCreateViewModelProcess = new CreateViewModelProcess;
    pThread = new QThread;

    pCreateViewModelProcess->moveToThread(pThread);

    connect(pThread, SIGNAL(started()), pCreateViewModelProcess,
            SLOT(process()));
    connect(pCreateViewModelProcess, SIGNAL(completed(qint64)), this,
            SLOT(onCompleted(qint64)));

    g_pTimer = new QTimer(this);
    connect(g_pTimer, SIGNAL(timeout()), this, SLOT(timerSlot()));
}

DialogCreateViewModel::~DialogCreateViewModel() {
    pCreateViewModelProcess->stop();

    g_pTimer->stop();

    pThread->quit();
    pThread->wait();

    delete ui;

    delete pThread;
    delete pCreateViewModelProcess;
}

void DialogCreateViewModel::setData(
    CreateViewModelProcess::TYPE type, QString sName,
    QList<XArchive::RECORD> *pListArchiveRecords,
    QStandardItemModel **ppTreeModel, QStandardItemModel **ppTableModel,
    QSet<XBinary::FT> stFilterFileTypes,
    QList<CreateViewModelProcess::RECORD> *pListViewRecords) {
    pCreateViewModelProcess->setData(type, sName, pListArchiveRecords,
                                     ppTreeModel, ppTableModel,
                                     stFilterFileTypes, pListViewRecords);
    pThread->start();
    g_pTimer->start(N_REFRESH_DELAY);
    ui->progressBarTotal->setMaximum(100);
}

void DialogCreateViewModel::on_pushButtonCancel_clicked() {
    pCreateViewModelProcess->stop();
}

void DialogCreateViewModel::onCompleted(qint64 nElapsed) {
    Q_UNUSED(nElapsed)

    this->close();
}

void DialogCreateViewModel::timerSlot() {
    CreateViewModelProcess::STATS stats =
        pCreateViewModelProcess->getCurrentStats();

    ui->labelTotal->setText(QString::number(stats.nTotal));
    ui->labelCurrent->setText(QString::number(stats.nCurrent));
    ui->labelCurrentStatus->setText(stats.sStatus);

    if (stats.nTotal) {
        ui->progressBarTotal->setValue(
            (int)((stats.nCurrent * 100) / stats.nTotal));
    }
}

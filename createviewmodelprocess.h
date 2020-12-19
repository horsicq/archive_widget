// copyright (c) 2020 hors<horsicq@gmail.com>
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
#ifndef CREATEVIEWMODELPROCESS_H
#define CREATEVIEWMODELPROCESS_H

#include <QElapsedTimer>
#include <QStandardItemModel>
#include "xarchives.h"

class CreateViewModelProcess : public QObject
{
    Q_OBJECT
public:
    enum UR
    {
        UR_PATH=0,
        UR_SIZE,
        UR_ISROOT
    };

    explicit CreateViewModelProcess(QObject *pParent=nullptr);
    void setData(QString sFileName,QList<XArchive::RECORD> *pListArchiveRecords,QStandardItemModel **ppTreeModel,QStandardItemModel **ppListModel);

signals:
    void errorMessage(QString sText);
    void completed(qint64 nElapsed);

public slots:
    void stop();
    void process();

private:
    QIODevice *pDevice;
    QString sFileName;
    QList<XArchive::RECORD> *pListArchiveRecords;
    QStandardItemModel **ppTreeModel;
    QStandardItemModel **ppTableModel;
    bool bIsStop;
};

#endif // CREATEVIEWMODELPROCESS_H

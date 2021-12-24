/* Copyright (c) 2020-2021 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
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

    enum TYPE
    {
        TYPE_UNKNOWN=0,
        TYPE_ARCHIVE,
        TYPE_DIRECTORY
    };

    explicit CreateViewModelProcess(QObject *pParent=nullptr);

    void setData(CreateViewModelProcess::TYPE type,QString sName,QList<XArchive::RECORD> *pListArchiveRecords,QStandardItemModel **ppTreeModel,QStandardItemModel **ppTableModel,QSet<XBinary::FT> stFilterFileTypes);

signals:
    void errorMessage(QString sText);
    void completed(qint64 nElapsed);

public slots:
    void stop();
    void process();

private:
    TYPE g_type;
    QString g_sName;
    QList<XArchive::RECORD> *g_pListArchiveRecords;
    QStandardItemModel **g_ppTreeModel;
    QStandardItemModel **g_ppTableModel;
    QSet<XBinary::FT> g_stFilterFileTypes;
    bool g_bIsStop;
};

#endif // CREATEVIEWMODELPROCESS_H

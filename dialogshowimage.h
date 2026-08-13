/* Copyright (c) 2020-2026 hors<horsicq@gmail.com>
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
#ifndef DIALOGSHOWIMAGE_H
#define DIALOGSHOWIMAGE_H

#include "xshortcutsdialog.h"
#include <QPixmap>

class QAction;
class QContextMenuEvent;
class QEvent;
class QWheelEvent;

namespace Ui {
class DialogShowImage;
}

// TODO move to FormatDialogs
class DialogShowImage : public XShortcutsDialog {
    Q_OBJECT

public:
    explicit DialogShowImage(QWidget *pParent, const QString &sFileName, const QString &sTitle);
    ~DialogShowImage() override;

    void adjustView() override;
    bool isImageLoaded() const;
    bool saveToFile(const QString &sFileName) const;

private:
    void createActions();
    void setLoadError(const QString &sMessage);
    void updateImageDisplay();
    void setZoomFactor(qreal factor);
    void zoomIn();
    void zoomOut();
    void fitToWindow();
    void actualSize();
    void copyToClipboard();
    void saveAs();
    void updateImageInfo();
    void updateActionState();
    void showContextMenu(const QPoint &globalPosition);

    Ui::DialogShowImage *ui;
    QPixmap m_originalPixmap;
    QString m_sSourceFileName;
    QString m_sImageFormat;
    QString m_sLoadError;
    qreal m_zoomFactor = 1.0;
    bool m_fitToWindow = true;
    bool m_bUpdatingImage = false;
    bool m_bUiReady = false;

    QAction *m_pActionZoomOut = nullptr;
    QAction *m_pActionActualSize = nullptr;
    QAction *m_pActionFitToWindow = nullptr;
    QAction *m_pActionZoomIn = nullptr;
    QAction *m_pActionCopy = nullptr;
    QAction *m_pActionSaveAs = nullptr;
    QAction *m_pActionClose = nullptr;

protected:
    void registerShortcuts(bool bState) override;
    bool eventFilter(QObject *pObject, QEvent *pEvent) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
};

#endif  // DIALOGSHOWIMAGE_H

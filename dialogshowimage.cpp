/* Copyright (c) 2020-2026 hors<horsicq@gmail.com>
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
#include "dialogshowimage.h"

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageReader>
#include <QImageWriter>
#include <QKeySequence>
#include <QMenu>
#include <QPushButton>
#include <QSaveFile>
#include <QScrollBar>
#include <QStandardPaths>
#include <QTimer>
#include <QToolButton>
#include <QWheelEvent>

#include "ui_dialogshowimage.h"

namespace {
constexpr qreal MINIMUM_ZOOM_FACTOR = 0.1;
constexpr qreal MAXIMUM_ZOOM_FACTOR = 5.0;
constexpr qreal ZOOM_STEP = 1.2;
constexpr qint64 MAXIMUM_IMAGE_PIXELS = 100000000;
constexpr int MAXIMUM_IMAGE_DIMENSION = 32768;

QString shortcutText(const QKeySequence &shortcut)
{
    return shortcut.toString(QKeySequence::NativeText);
}
}  // namespace

DialogShowImage::DialogShowImage(QWidget *pParent, const QString &sFileName, const QString &sTitle)
    : XShortcutsDialog(pParent, true), ui(new Ui::DialogShowImage), m_sSourceFileName(sFileName)
{
    ui->setupUi(this);
    m_bUiReady = true;
    setWindowTitle(sTitle.trimmed().isEmpty() ? tr("Image") : sTitle);

    ui->scrollAreaImage->viewport()->installEventFilter(this);
    ui->labelImage->installEventFilter(this);

    ui->scrollAreaImage->setAccessibleName(tr("Image viewport"));
    ui->scrollAreaImage->setAccessibleDescription(tr("Scrollable image preview. Hold Control and use the mouse wheel to zoom."));
    ui->labelImage->setAccessibleName(tr("Image preview"));
    ui->labelInfo->setAccessibleName(tr("Image information"));

    createActions();
    ui->toolButtonZoomOut->setDefaultAction(m_pActionZoomOut);
    ui->toolButtonActualSize->setDefaultAction(m_pActionActualSize);
    ui->toolButtonFit->setDefaultAction(m_pActionFitToWindow);
    ui->toolButtonZoomIn->setDefaultAction(m_pActionZoomIn);
    ui->toolButtonCopy->setDefaultAction(m_pActionCopy);
    ui->toolButtonSaveAs->setDefaultAction(m_pActionSaveAs);
    ui->toolButtonZoomOut->setText(QString::fromUtf8("\xE2\x88\x92"));
    ui->toolButtonActualSize->setText(QStringLiteral("1:1"));
    ui->toolButtonFit->setText(tr("&Fit"));
    ui->toolButtonZoomIn->setText(QStringLiteral("+"));
    ui->toolButtonCopy->setText(tr("&Copy"));
    ui->toolButtonSaveAs->setText(tr("&Save As..."));

    connect(ui->buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    if (QPushButton *pCloseButton = ui->buttonBox->button(QDialogButtonBox::Close)) {
        pCloseButton->setDefault(false);
        pCloseButton->setAutoDefault(false);
    }

    QImageReader reader(sFileName);
    reader.setDecideFormatFromContent(true);
    reader.setAutoTransform(true);
    const QSize declaredSize = reader.size();
    if (declaredSize.isValid() && ((declaredSize.width() > MAXIMUM_IMAGE_DIMENSION) || (declaredSize.height() > MAXIMUM_IMAGE_DIMENSION) ||
                                   ((static_cast<qint64>(declaredSize.width()) * declaredSize.height()) > MAXIMUM_IMAGE_PIXELS))) {
        setLoadError(tr("Unable to load image: the image dimensions are too large."));
        return;
    }

    const QByteArray baDetectedFormat = reader.format();
    const QImage image = reader.read();
    if (image.isNull()) {
        QString sError = reader.errorString().trimmed();
        if (sError.isEmpty()) {
            sError = QFileInfo::exists(sFileName) ? tr("the file is not a supported image") : tr("the file does not exist");
        }
        setLoadError(tr("Unable to load image: %1.").arg(sError));
        return;
    }
    if ((image.width() > MAXIMUM_IMAGE_DIMENSION) || (image.height() > MAXIMUM_IMAGE_DIMENSION) ||
        ((static_cast<qint64>(image.width()) * image.height()) > MAXIMUM_IMAGE_PIXELS)) {
        setLoadError(tr("Unable to load image: the decoded image dimensions are too large."));
        return;
    }

    m_originalPixmap = QPixmap::fromImage(image);
    m_sImageFormat = QString::fromLatin1(baDetectedFormat.isEmpty() ? reader.format() : baDetectedFormat).toUpper();
    if (m_sImageFormat.isEmpty()) {
        m_sImageFormat = tr("Unknown format");
    }

    m_zoomFactor = 1.0;
    m_fitToWindow = true;
    updateImageInfo();
    updateActionState();
    QTimer::singleShot(0, this, [this]() { updateImageDisplay(); });
}

DialogShowImage::~DialogShowImage()
{
    m_bUiReady = false;
    ui->scrollAreaImage->viewport()->removeEventFilter(this);
    ui->labelImage->removeEventFilter(this);
    delete ui;
}

void DialogShowImage::createActions()
{
    const auto createAction = [this](const QString &sObjectName, const QString &sText, const QKeySequence &shortcut) {
        QAction *pAction = new QAction(sText, this);
        pAction->setObjectName(sObjectName);
        pAction->setShortcut(shortcut);
        pAction->setShortcutContext(Qt::WidgetWithChildrenShortcut);
        addAction(pAction);
        return pAction;
    };

    QKeySequence zoomOutShortcut = QKeySequence::ZoomOut;
    if (zoomOutShortcut.isEmpty()) {
        zoomOutShortcut = QKeySequence(QStringLiteral("Ctrl+-"));
    }
    QKeySequence zoomInShortcut = QKeySequence::ZoomIn;
    if (zoomInShortcut.isEmpty()) {
        zoomInShortcut = QKeySequence(QStringLiteral("Ctrl++"));
    }

    m_pActionZoomOut = createAction(QStringLiteral("actionZoomOut"), tr("Zoom Out"), zoomOutShortcut);
    m_pActionActualSize = createAction(QStringLiteral("actionActualSize"), tr("Actual Size"), QKeySequence(QStringLiteral("Ctrl+0")));
    m_pActionFitToWindow = createAction(QStringLiteral("actionFitToWindow"), tr("Fit to Window"), QKeySequence(QStringLiteral("F")));
    m_pActionZoomIn = createAction(QStringLiteral("actionZoomIn"), tr("Zoom In"), zoomInShortcut);
    m_pActionCopy = createAction(QStringLiteral("actionCopy"), tr("Copy"), QKeySequence::Copy);
    QKeySequence saveAsShortcut = QKeySequence::SaveAs;
    if (saveAsShortcut.isEmpty()) {
        saveAsShortcut = QKeySequence(QStringLiteral("Ctrl+Shift+S"));
    }
    m_pActionSaveAs = createAction(QStringLiteral("actionSaveAs"), tr("Save As..."), saveAsShortcut);
    m_pActionClose = createAction(QStringLiteral("actionClose"), tr("Close"), QKeySequence::Close);

    m_pActionFitToWindow->setCheckable(true);
    connect(m_pActionZoomOut, &QAction::triggered, this, [this]() { zoomOut(); });
    connect(m_pActionActualSize, &QAction::triggered, this, [this]() { actualSize(); });
    connect(m_pActionFitToWindow, &QAction::triggered, this, [this]() { fitToWindow(); });
    connect(m_pActionZoomIn, &QAction::triggered, this, [this]() { zoomIn(); });
    connect(m_pActionCopy, &QAction::triggered, this, [this]() { copyToClipboard(); });
    connect(m_pActionSaveAs, &QAction::triggered, this, [this]() { saveAs(); });
    connect(m_pActionClose, &QAction::triggered, this, &QDialog::reject);

    m_pActionZoomOut->setToolTip(tr("Zoom out (%1)").arg(shortcutText(m_pActionZoomOut->shortcut())));
    m_pActionActualSize->setToolTip(tr("Show the image at its actual size (%1)").arg(shortcutText(m_pActionActualSize->shortcut())));
    m_pActionFitToWindow->setToolTip(tr("Fit the image inside the window (%1)").arg(shortcutText(m_pActionFitToWindow->shortcut())));
    m_pActionZoomIn->setToolTip(tr("Zoom in (%1)").arg(shortcutText(m_pActionZoomIn->shortcut())));
    m_pActionCopy->setToolTip(tr("Copy the original image (%1)").arg(shortcutText(m_pActionCopy->shortcut())));
    m_pActionSaveAs->setToolTip(tr("Save a copy of the image (%1)").arg(shortcutText(m_pActionSaveAs->shortcut())));
}

void DialogShowImage::setLoadError(const QString &sMessage)
{
    m_originalPixmap = QPixmap();
    m_sImageFormat.clear();
    m_sLoadError = sMessage;
    ui->labelImage->clear();
    ui->labelImage->setText(tr("No image preview is available."));
    ui->labelImage->setAccessibleDescription(sMessage);
    ui->labelImage->resize(qMax(1, ui->scrollAreaImage->viewport()->width()), qMax(1, ui->scrollAreaImage->viewport()->height()));
    updateImageInfo();
    updateActionState();
}

bool DialogShowImage::isImageLoaded() const
{
    return !m_originalPixmap.isNull();
}

bool DialogShowImage::saveToFile(const QString &sFileName) const
{
    if (m_originalPixmap.isNull() || sFileName.trimmed().isEmpty()) {
        return false;
    }

    QByteArray baFormat = QFileInfo(sFileName).suffix().toLatin1().toUpper();
    if (baFormat == QByteArrayLiteral("JPG")) {
        baFormat = QByteArrayLiteral("JPEG");
    }
    if (baFormat.isEmpty()) {
        baFormat = QByteArrayLiteral("PNG");
    }

    QSaveFile file(sFileName);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QImageWriter writer(&file, baFormat);
    if (!writer.write(m_originalPixmap.toImage())) {
        file.cancelWriting();
        return false;
    }

    return file.commit();
}

void DialogShowImage::adjustView()
{
    if (m_fitToWindow) {
        updateImageDisplay();
    }
}

void DialogShowImage::registerShortcuts(bool bState)
{
    Q_UNUSED(bState)
}

bool DialogShowImage::eventFilter(QObject *pObject, QEvent *pEvent)
{
    // setupUi() can deliver events before every generated child pointer has
    // been initialized (for example while QDialog creates its size grip).
    if (!m_bUiReady) {
        return XShortcutsDialog::eventFilter(pObject, pEvent);
    }

    if (pObject == ui->scrollAreaImage->viewport()) {
        if ((pEvent->type() == QEvent::Resize) && m_fitToWindow) {
            updateImageDisplay();
        } else if (pEvent->type() == QEvent::Wheel) {
            QWheelEvent *pWheelEvent = static_cast<QWheelEvent *>(pEvent);
            if (pWheelEvent->modifiers() & Qt::ControlModifier) {
                const int nDelta = pWheelEvent->angleDelta().y() ? pWheelEvent->angleDelta().y() : pWheelEvent->pixelDelta().y();
                if (nDelta > 0) {
                    zoomIn();
                } else if (nDelta < 0) {
                    zoomOut();
                }
                if (nDelta != 0) {
                    pWheelEvent->accept();
                    return true;
                }
            }
        } else if (pEvent->type() == QEvent::ContextMenu) {
            QContextMenuEvent *pContextMenuEvent = static_cast<QContextMenuEvent *>(pEvent);
            showContextMenu(pContextMenuEvent->globalPos());
            return true;
        }
    } else if ((pObject == ui->labelImage) && (pEvent->type() == QEvent::ContextMenu)) {
        QContextMenuEvent *pContextMenuEvent = static_cast<QContextMenuEvent *>(pEvent);
        showContextMenu(pContextMenuEvent->globalPos());
        return true;
    }

    return XShortcutsDialog::eventFilter(pObject, pEvent);
}

void DialogShowImage::updateImageDisplay()
{
    if (m_originalPixmap.isNull() || m_bUpdatingImage) {
        return;
    }

    m_bUpdatingImage = true;
    const QSize sourceSize = m_originalPixmap.size();
    QSize displaySize;

    if (m_fitToWindow) {
        const QSize viewportSize = ui->scrollAreaImage->viewport()->size();
        if (viewportSize.width() > 0 && viewportSize.height() > 0) {
            displaySize = sourceSize.scaled(viewportSize, Qt::KeepAspectRatio);
            if ((displaySize.width() > sourceSize.width()) || (displaySize.height() > sourceSize.height())) {
                displaySize = sourceSize;
            }
        }
    } else {
        displaySize = QSize(qMax(1, qRound(sourceSize.width() * m_zoomFactor)), qMax(1, qRound(sourceSize.height() * m_zoomFactor)));
    }

    if (!displaySize.isEmpty()) {
        m_zoomFactor = static_cast<qreal>(displaySize.width()) / sourceSize.width();
        ui->labelImage->setText(QString());
        ui->labelImage->setPixmap(m_originalPixmap);
        ui->labelImage->setScaledContents(displaySize != sourceSize);
        ui->labelImage->resize(displaySize);
        ui->labelImage->setAccessibleDescription(
            tr("%1 by %2 pixel %3 image shown at %4 percent.").arg(sourceSize.width()).arg(sourceSize.height()).arg(m_sImageFormat).arg(qRound(m_zoomFactor * 100)));
    }

    m_bUpdatingImage = false;
    updateImageInfo();
    updateActionState();
}

void DialogShowImage::setZoomFactor(qreal factor)
{
    if (m_originalPixmap.isNull()) {
        return;
    }

    const QScrollBar *pHorizontalScrollBar = ui->scrollAreaImage->horizontalScrollBar();
    const QScrollBar *pVerticalScrollBar = ui->scrollAreaImage->verticalScrollBar();
    const qreal horizontalPosition = pHorizontalScrollBar->maximum() ? static_cast<qreal>(pHorizontalScrollBar->value()) / pHorizontalScrollBar->maximum() : 0.5;
    const qreal verticalPosition = pVerticalScrollBar->maximum() ? static_cast<qreal>(pVerticalScrollBar->value()) / pVerticalScrollBar->maximum() : 0.5;

    m_zoomFactor = qBound(MINIMUM_ZOOM_FACTOR, factor, MAXIMUM_ZOOM_FACTOR);
    m_fitToWindow = false;
    updateImageDisplay();

    QScrollBar *pNewHorizontalScrollBar = ui->scrollAreaImage->horizontalScrollBar();
    QScrollBar *pNewVerticalScrollBar = ui->scrollAreaImage->verticalScrollBar();
    pNewHorizontalScrollBar->setValue(qRound(pNewHorizontalScrollBar->maximum() * horizontalPosition));
    pNewVerticalScrollBar->setValue(qRound(pNewVerticalScrollBar->maximum() * verticalPosition));
}

void DialogShowImage::zoomIn()
{
    setZoomFactor(m_zoomFactor * ZOOM_STEP);
}

void DialogShowImage::zoomOut()
{
    setZoomFactor(m_zoomFactor / ZOOM_STEP);
}

void DialogShowImage::fitToWindow()
{
    if (m_originalPixmap.isNull()) {
        return;
    }

    m_fitToWindow = true;
    updateImageDisplay();
}

void DialogShowImage::actualSize()
{
    setZoomFactor(1.0);
}

void DialogShowImage::contextMenuEvent(QContextMenuEvent *pEvent)
{
    showContextMenu(pEvent->globalPos());
    pEvent->accept();
}

void DialogShowImage::showContextMenu(const QPoint &globalPosition)
{
    QMenu menu(this);
    menu.addAction(m_pActionZoomOut);
    menu.addAction(m_pActionActualSize);
    menu.addAction(m_pActionFitToWindow);
    menu.addAction(m_pActionZoomIn);
    menu.addSeparator();
    menu.addAction(m_pActionCopy);
    menu.addAction(m_pActionSaveAs);
    menu.addSeparator();
    menu.addAction(m_pActionClose);
    menu.exec(globalPosition);
}

void DialogShowImage::copyToClipboard()
{
    if (!m_originalPixmap.isNull()) {
        QApplication::clipboard()->setImage(m_originalPixmap.toImage());
    }
}

void DialogShowImage::saveAs()
{
    if (m_originalPixmap.isNull()) {
        return;
    }

    QString sBaseName = QFileInfo(windowTitle()).completeBaseName();
    if (sBaseName.isEmpty()) {
        sBaseName = QFileInfo(m_sSourceFileName).completeBaseName();
    }
    if (sBaseName.isEmpty()) {
        sBaseName = tr("Image");
    }
    const QString sDefaultPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + QLatin1Char('/') + sBaseName + QStringLiteral(".png");
    const QString sFileName = QFileDialog::getSaveFileName(this, tr("Save Image As"), sDefaultPath,
                                                           tr("PNG image (*.png);;JPEG image (*.jpg *.jpeg);;Bitmap image (*.bmp);;TIFF image (*.tif *.tiff)"));
    if (!sFileName.isEmpty() && !saveToFile(sFileName)) {
        ui->labelInfo->setText(tr("Unable to save the image to %1.").arg(QDir::toNativeSeparators(sFileName)));
    }
}

void DialogShowImage::updateImageInfo()
{
    if (m_originalPixmap.isNull()) {
        ui->labelInfo->setText(m_sLoadError.isEmpty() ? tr("No image loaded.") : m_sLoadError);
        return;
    }

    const QSize size = m_originalPixmap.size();
    QString sZoom;
    if (m_fitToWindow) {
        sZoom = tr("Fit to window (%1%)").arg(qRound(m_zoomFactor * 100));
    } else if (qFuzzyCompare(m_zoomFactor, 1.0)) {
        sZoom = tr("Actual size (100%)");
    } else {
        sZoom = tr("%1%").arg(qRound(m_zoomFactor * 100));
    }
    ui->labelInfo->setText(tr("%1 x %2 pixels | %3 | %4").arg(size.width()).arg(size.height()).arg(m_sImageFormat, sZoom));
}

void DialogShowImage::updateActionState()
{
    const bool bLoaded = !m_originalPixmap.isNull();
    m_pActionZoomOut->setEnabled(bLoaded && (m_zoomFactor > MINIMUM_ZOOM_FACTOR + 0.0001));
    m_pActionActualSize->setEnabled(bLoaded);
    m_pActionFitToWindow->setEnabled(bLoaded);
    m_pActionFitToWindow->setChecked(bLoaded && m_fitToWindow);
    m_pActionZoomIn->setEnabled(bLoaded && (m_zoomFactor < MAXIMUM_ZOOM_FACTOR - 0.0001));
    m_pActionCopy->setEnabled(bLoaded);
    m_pActionSaveAs->setEnabled(bLoaded);
    m_pActionClose->setEnabled(true);
}

void DialogShowImage::wheelEvent(QWheelEvent *pEvent)
{
    if (pEvent->modifiers() & Qt::ControlModifier) {
        const int nDelta = pEvent->angleDelta().y() ? pEvent->angleDelta().y() : pEvent->pixelDelta().y();
        if (nDelta > 0) {
            zoomIn();
        } else if (nDelta < 0) {
            zoomOut();
        }
        if (nDelta != 0) {
            pEvent->accept();
            return;
        }
    }

    XShortcutsDialog::wheelEvent(pEvent);
}

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
#include "dialogshowimage.h"

#include <QMessageBox>
#include <QFileInfo>
#include <QDebug>
#include <QContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QClipboard>
#include <QApplication>
#include <QFileDialog>
#include <QStandardPaths>
#include <QWheelEvent>

#include "ui_dialogshowimage.h"

DialogShowImage::DialogShowImage(QWidget *pParent, const QString &sFileName, const QString &sTitle) : XShortcutsDialog(pParent, true), ui(new Ui::DialogShowImage)
{
    ui->setupUi(this);

    setWindowTitle(sTitle);

    // Check if file exists
    if (!QFileInfo::exists(sFileName)) {
        QMessageBox::critical(this, tr("Error"), tr("Image file does not exist: %1").arg(sFileName));
        return;
    }

    // Load the image
    QPixmap pixmap(sFileName);
    if (pixmap.isNull()) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to load image: %1").arg(sFileName));
        return;
    }

    // Store original pixmap
    m_originalPixmap = pixmap;
    m_currentPixmap = pixmap;
    m_zoomFactor = 1.0;
    m_fitToWindow = true; // Start with fit to window

    // Set initial display
    updateImageDisplay();

    // Set window size based on image size, but constrain to reasonable limits
    QSize imageSize = pixmap.size();
    QSize maxSize(1200, 800); // Reasonable maximum window size

    if (imageSize.width() > maxSize.width() || imageSize.height() > maxSize.height()) {
        imageSize.scale(maxSize, Qt::KeepAspectRatio);
    }

    resize(imageSize + QSize(20, 60)); // Add some padding for window decorations

    // Display image information
    updateImageInfo();
}

DialogShowImage::~DialogShowImage()
{
    delete ui;
}

void DialogShowImage::adjustView()
{
    if (m_fitToWindow) {
        updateImageDisplay();
    }
}

void DialogShowImage::on_pushButtonClose_clicked()
{
    this->close();
}

void DialogShowImage::registerShortcuts(bool bState)
{
    if (bState) {
        // Zoom shortcuts
        // getShortcuts()->addShortcut(Qt::Key_Plus, this, SLOT(zoomIn()));
        // getShortcuts()->addShortcut(Qt::Key_Equal, this, SLOT(zoomIn()));
        // getShortcuts()->addShortcut(Qt::CTRL + Qt::Key_Plus, this, SLOT(zoomIn()));
        // getShortcuts()->addShortcut(Qt::CTRL + Qt::Key_Equal, this, SLOT(zoomIn()));
        // getShortcuts()->addShortcut(Qt::Key_Minus, this, SLOT(zoomOut()));
        // getShortcuts()->addShortcut(Qt::CTRL + Qt::Key_Minus, this, SLOT(zoomOut()));
        // getShortcuts()->addShortcut(Qt::Key_0, this, SLOT(actualSize()));
        // getShortcuts()->addShortcut(Qt::CTRL + Qt::Key_0, this, SLOT(actualSize()));
        // getShortcuts()->addShortcut(Qt::Key_F, this, SLOT(fitToWindow()));

        // // Close shortcuts
        // getShortcuts()->addShortcut(Qt::Key_Escape, this, SLOT(close()));
        // getShortcuts()->addShortcut(Qt::CTRL + Qt::Key_W, this, SLOT(close()));
    } else {
        // Remove shortcuts when disabled
        // getShortcuts()->clear();
    }
}

void DialogShowImage::updateImageDisplay()
{
    if (m_fitToWindow) {
        // Scale to fit the label size while maintaining aspect ratio
        QSize labelSize = ui->labelImage->size();
        QPixmap scaledPixmap = m_originalPixmap.scaled(labelSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->labelImage->setPixmap(scaledPixmap);
    } else {
        // Apply zoom factor
        QSize newSize = m_originalPixmap.size() * m_zoomFactor;
        QPixmap scaledPixmap = m_originalPixmap.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        ui->labelImage->setPixmap(scaledPixmap);
    }
}

void DialogShowImage::setZoomFactor(qreal factor)
{
    m_zoomFactor = qMax(0.1, qMin(5.0, factor)); // Limit zoom between 10% and 500%
    m_fitToWindow = false;
    updateImageDisplay();
    updateImageInfo();
    adjustView();
}

void DialogShowImage::zoomIn()
{
    setZoomFactor(m_zoomFactor * 1.2);
}

void DialogShowImage::zoomOut()
{
    setZoomFactor(m_zoomFactor / 1.2);
}

void DialogShowImage::fitToWindow()
{
    m_fitToWindow = true;
    updateImageDisplay();
    updateImageInfo();
}

void DialogShowImage::actualSize()
{
    m_zoomFactor = 1.0;
    m_fitToWindow = false;
    updateImageDisplay();
    updateImageInfo();
}

void DialogShowImage::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    // Zoom actions
    QAction *zoomInAction = menu.addAction(tr("Zoom In"), this, SLOT(zoomIn()));
    zoomInAction->setShortcut(QKeySequence::ZoomIn);

    QAction *zoomOutAction = menu.addAction(tr("Zoom Out"), this, SLOT(zoomOut()));
    zoomOutAction->setShortcut(QKeySequence::ZoomOut);

    QAction *actualSizeAction = menu.addAction(tr("Actual Size"), this, SLOT(actualSize()));
    actualSizeAction->setShortcut(tr("Ctrl+0"));

    QAction *fitToWindowAction = menu.addAction(tr("Fit to Window"), this, SLOT(fitToWindow()));
    fitToWindowAction->setShortcut(tr("F"));

    menu.addSeparator();

    // Copy and save actions
    QAction *copyAction = menu.addAction(tr("Copy"), this, SLOT(copyToClipboard()));
    copyAction->setShortcut(QKeySequence::Copy);

    QAction *saveAsAction = menu.addAction(tr("Save As..."), this, SLOT(saveAs()));

    menu.addSeparator();

    // Close action
    QAction *closeAction = menu.addAction(tr("Close"), this, SLOT(close()));
    closeAction->setShortcut(QKeySequence::Close);

    menu.exec(event->globalPos());
}

void DialogShowImage::copyToClipboard()
{
    if (!m_originalPixmap.isNull()) {
        QApplication::clipboard()->setPixmap(m_originalPixmap);
    }
}

void DialogShowImage::saveAs()
{
    if (m_originalPixmap.isNull()) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, tr("Save Image"),
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation),
        tr("Images (*.png *.jpg *.jpeg *.bmp *.gif)"));

    if (!fileName.isEmpty()) {
        if (!m_originalPixmap.save(fileName)) {
            QMessageBox::critical(this, tr("Error"), tr("Failed to save image to %1").arg(fileName));
        }
    }
}

void DialogShowImage::updateImageInfo()
{
    if (m_originalPixmap.isNull()) {
        ui->labelInfo->setText(tr("No image loaded"));
        return;
    }

    QSize size = m_originalPixmap.size();
    QString format = "Unknown";

    // Try to determine format from file extension
    QFileInfo fileInfo(windowTitle());
    QString extension = fileInfo.suffix().toLower();

    if (extension == "png") format = "PNG";
    else if (extension == "jpg" || extension == "jpeg") format = "JPEG";
    else if (extension == "bmp") format = "BMP";
    else if (extension == "gif") format = "GIF";
    else if (extension == "tiff" || extension == "tif") format = "TIFF";

    QString info = tr("Size: %1 x %2 | Format: %3 | Zoom: %4%")
                   .arg(size.width())
                   .arg(size.height())
                   .arg(format)
                   .arg(qRound(m_zoomFactor * 100));

    ui->labelInfo->setText(info);
}

void DialogShowImage::wheelEvent(QWheelEvent *event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        // Ctrl + wheel = zoom
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        // Pass to parent for default handling
        QWidget::wheelEvent(event);
    }
}

/* Copyright (c) 2025-2026 hors<horsicq@gmail.com>
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
#include "archiveexplorerwidget.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSet>
#include <QSignalBlocker>
#include <QStorageInfo>
#include <QTemporaryFile>
#include <QUrl>

#include <algorithm>

#include "ui_archiveexplorerwidget.h"
#include "xoptions.h"

namespace {

const qint64 N_MAX_OPEN_RECORD_SIZE = 256LL * 1024LL * 1024LL;
const qint32 N_MAX_OPEN_TEMPORARY_FILES = 4;

QString getArchiveRecordBaseName(QString sRecordFileName)
{
    sRecordFileName.replace(QChar('\\'), QChar('/'));

    while (sRecordFileName.endsWith(QChar('/'))) {
        sRecordFileName.chop(1);
    }

    return sRecordFileName.section(QChar('/'), -1);
}

bool isArchiveFolderRecord(const XBinary::ARCHIVERECORD &record)
{
    QString sRecordFileName = record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();

    return record.mapProperties.value(XBinary::FPART_PROP_ISFOLDER).toBool() || sRecordFileName.endsWith(QChar('/')) || sRecordFileName.endsWith(QChar('\\'));
}

}  // namespace

ArchiveExplorerWidget::ArchiveExplorerWidget(QWidget *pParent) : XShortcutsWidget(pParent), ui(new Ui::ArchiveExplorerWidget)
{
    ui->setupUi(this);

    m_pDevice = nullptr;
    m_fileType = XBinary::FT_UNKNOWN;
    m_pModel = nullptr;
    m_nCurrentFileSize = 0;
    m_bAdvanced = false;
    m_bArchiveAvailable = false;
    m_bUserFileType = false;
    m_archiveSource = ARCHIVE_SOURCE_NATIVE;

    XOptions::adjustToolButton(ui->toolButtonExtractAll, XOptions::ICONTYPE_EXTRACTOR);
    XOptions::adjustToolButton(ui->toolButtonTest, XOptions::ICONTYPE_SCAN);

    updateActions();
}

ArchiveExplorerWidget::~ArchiveExplorerWidget()
{
    qint32 nNumberOfTemporaryFiles = m_listTemporaryFiles.count();

    for (qint32 i = 0; i < nNumberOfTemporaryFiles; i++) {
        QFile::remove(m_listTemporaryFiles.at(i));
    }

    delete ui;
}

void ArchiveExplorerWidget::setData(XBinary::FT fileType, QIODevice *pDevice, bool bIsImage, XADDR nModuleAddress)
{
    Q_UNUSED(bIsImage)
    Q_UNUSED(nModuleAddress)

    // setData() establishes a new archive session. Never carry credentials
    // across sessions, including when a caller reuses the same QFile object.
    ui->lineEditPassword->clear();

    m_pDevice = pDevice;

    // A fresh archive session starts in auto-detect mode: the best reader
    // (including the ip7z source) may pick the format. The user forcing a type
    // via the combobox switches to that exact type through the native reader.
    m_bUserFileType = false;

    // Populate the file-type selector from the device (same pattern as
    // XEntropyWidget). Block signals so the programmatic population does not
    // trigger a redundant reload; the loadRecords() below is the initial load.
    {
        QSignalBlocker signalBlocker(ui->comboBoxType);
        ui->comboBoxType->clear();

        if (m_pDevice) {
            m_fileType = XFormats::setFileTypeComboBox(fileType, m_pDevice, ui->comboBoxType, XBinary::FT_FLAG_FORMATS | XBinary::FT_FLAG_STATICUNPACKERS);
        } else {
            m_fileType = fileType;
        }
    }

    loadRecords();
    updateActions();
}

const QList<XBinary::ARCHIVERECORD> *ArchiveExplorerWidget::getArchiveRecords() const
{
    return &m_listArchiveRecords;
}

QString ArchiveExplorerWidget::getPassword() const
{
    return ui->lineEditPassword->text();
}

bool ArchiveExplorerWidget::isArchiveAvailable() const
{
    return m_bArchiveAvailable;
}

QString ArchiveExplorerWidget::getCurrentRecordFileName()
{
    return m_sCurrentRecordFileName;
}

void ArchiveExplorerWidget::adjustView()
{
}

void ArchiveExplorerWidget::reloadData(bool bSaveSelection)
{
    QString sSelectedRecord;
    qint32 nSelectedRow = -1;
    XADDR nSelectedStreamOffset = 0;
    XADDR nSelectedStreamSize = 0;
    bool bRestoreSelection = false;

    if (bSaveSelection) {
        nSelectedRow = getCurrentRecordIndex();

        if ((nSelectedRow >= 0) && (nSelectedRow < m_listArchiveRecords.count())) {
            const XBinary::ARCHIVERECORD &record = m_listArchiveRecords.at(nSelectedRow);
            sSelectedRecord = record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
            nSelectedStreamOffset = record.nStreamOffset;
            nSelectedStreamSize = record.nStreamSize;
            bRestoreSelection = true;
        }
    }

    loadRecords();
    updateActions();

    if (bRestoreSelection && m_pModel && ui->tableViewRecords->getProxyModel()) {
        qint32 nNumberOfRecords = m_listArchiveRecords.count();
        qint32 nTargetRow = -1;

        auto isSelectedRecord = [&sSelectedRecord, nSelectedStreamOffset, nSelectedStreamSize](const XBinary::ARCHIVERECORD &record) {
            return (record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString() == sSelectedRecord) &&
                   (record.nStreamOffset == nSelectedStreamOffset) && (record.nStreamSize == nSelectedStreamSize);
        };

        if ((nSelectedRow >= 0) && (nSelectedRow < nNumberOfRecords) && isSelectedRecord(m_listArchiveRecords.at(nSelectedRow))) {
            nTargetRow = nSelectedRow;
        } else {
            for (qint32 i = 0; i < nNumberOfRecords; i++) {
                if (isSelectedRecord(m_listArchiveRecords.at(i))) {
                    nTargetRow = i;
                    break;
                }
            }
        }

        if (nTargetRow >= 0) {
            QModelIndex sourceIndex = m_pModel->index(nTargetRow, 0);
            QModelIndex proxyIndex = ui->tableViewRecords->getProxyModel()->mapFromSource(sourceIndex);

            if (proxyIndex.isValid()) {
                ui->tableViewRecords->setCurrentIndex(proxyIndex);
                ui->tableViewRecords->selectRow(proxyIndex.row());
            }
        }
    }
}

void ArchiveExplorerWidget::on_comboBoxType_currentIndexChanged(int nIndex)
{
    Q_UNUSED(nIndex)

    if (!m_pDevice) {
        return;
    }

    // The user picked a specific file type: re-interpret the file as exactly
    // that type through the native reader (bypassing ip7z auto-detection, which
    // would otherwise ignore the selection). If the chosen type cannot be
    // unpacked, loadRecords() shows the file itself and leaves Extract/Test
    // disabled.
    m_bUserFileType = true;
    m_fileType = (XBinary::FT)(ui->comboBoxType->currentData().toInt());

    reloadData(true);
}

void ArchiveExplorerWidget::on_toolButtonExtractAll_clicked()
{
    if (ui->toolButtonExtractAll->isEnabled()) {
        emit extractAllRequested();
    }
}

void ArchiveExplorerWidget::on_toolButtonTest_clicked()
{
    if (ui->toolButtonTest->isEnabled()) {
        emit testRequested();
    }
}

void ArchiveExplorerWidget::on_lineEditPassword_editingFinished()
{
    if (m_pDevice && m_pDevice->isOpen()) {
        reloadData(true);
    }
}

void ArchiveExplorerWidget::on_checkBoxAdvanced_toggled(bool bChecked)
{
    m_bAdvanced = bChecked;

    if (m_pDevice) {
        reloadData(true);
    }
}

void ArchiveExplorerWidget::on_tableViewRecords_customContextMenuRequested(const QPoint &pos)
{
    if (!ui->tableViewRecords->selectionModel()) {
        return;
    }

    QModelIndex index = ui->tableViewRecords->indexAt(pos);

    if (index.isValid()) {
        ui->tableViewRecords->setCurrentIndex(index);
        ui->tableViewRecords->selectRow(index.row());
        showContext(m_sCurrentRecordFileName, ui->tableViewRecords->viewport()->mapToGlobal(pos));
    }
}

void ArchiveExplorerWidget::showContext(const QString &sRecordFileName, QPoint point)
{
    XShortcuts *pShortcuts = getShortcuts();
    qint32 nRow = getCurrentRecordIndex();

    if (sRecordFileName.isEmpty() || (pShortcuts == nullptr) || (nRow < 0) || (nRow >= m_listArchiveRecords.count())) {
        return;
    }

    QMenu contextMenu(this);
    QList<XShortcuts::MENUITEM> listMenuItems;
    const XBinary::ARCHIVERECORD &record = m_listArchiveRecords.at(nRow);
    bool bIsFolder = isArchiveFolderRecord(record);

    auto appendMenuItem = [&listMenuItems](const QString &sText, const QObject *pRecv, const char *pMethod, XOptions::ICONTYPE iconType,
                                           quint64 nSubgroups) {
        XShortcuts::MENUITEM menuItem = {};
        menuItem.sText = sText;
        menuItem.pRecv = pRecv;
        menuItem.pMethod = pMethod;
        menuItem.iconType = iconType;
        menuItem.nSubgroups = nSubgroups;
        listMenuItems.append(menuItem);
    };

    // These actions require a real, unpackable archive. When the file is only
    // shown as a single fallback entry (m_bArchiveAvailable == false) they are
    // omitted, matching the disabled Extract / Test toolbar buttons.
    if (m_bArchiveAvailable) {
        if (!bIsFolder) {
            pShortcuts->_addMenuItem(&listMenuItems, X_ID_ARCHIVE_OPEN, this, SLOT(openRecord()), XShortcuts::GROUPID_NONE);
            appendMenuItem(tr("Extract"), this, SLOT(extractRecord()), XOptions::ICONTYPE_EXTRACTOR, XShortcuts::GROUPID_NONE);
        }

        appendMenuItem(tr("Extract all"), this, SLOT(on_toolButtonExtractAll_clicked()), XOptions::ICONTYPE_EXTRACTOR, XShortcuts::GROUPID_NONE);
        appendMenuItem(tr("Test archive"), this, SLOT(on_toolButtonTest_clicked()), XOptions::ICONTYPE_SCAN, XShortcuts::GROUPID_NONE);
        pShortcuts->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_NONE);
    }

    pShortcuts->_addMenuItem(&listMenuItems, X_ID_ARCHIVE_COPY_FILENAME, this, SLOT(copyFileName()), XShortcuts::GROUPID_COPY);
    appendMenuItem(tr("Member path"), this, SLOT(copyRecordPath()), XOptions::ICONTYPE_PATH, XShortcuts::GROUPID_COPY);
    appendMenuItem(tr("Row details"), this, SLOT(copyRecordDetails()), XOptions::ICONTYPE_COPY, XShortcuts::GROUPID_COPY);

    pShortcuts->_addMenuSeparator(&listMenuItems, XShortcuts::GROUPID_NONE);
    appendMenuItem(tr("Properties"), this, SLOT(showRecordProperties()), XOptions::ICONTYPE_INFO, XShortcuts::GROUPID_NONE);
    appendMenuItem(tr("Refresh"), this, SLOT(refreshRecords()), XOptions::ICONTYPE_RELOAD, XShortcuts::GROUPID_NONE);

    pShortcuts->adjustContextMenu(&contextMenu, &listMenuItems);
    contextMenu.exec(point);
}

void ArchiveExplorerWidget::openRecord()
{
    qint32 nRow = getCurrentRecordIndex();

    if ((nRow < 0) || (nRow >= m_listArchiveRecords.count())) {
        return;
    }

    const XBinary::ARCHIVERECORD &record = m_listArchiveRecords.at(nRow);

    if (isArchiveFolderRecord(record)) {
        return;
    }

    QString sRecordFileName = record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
    QString sBaseName = getArchiveRecordBaseName(sRecordFileName);
    QString sSuffix = QFileInfo(sBaseName).suffix().toLower();
    QString sSafeSuffix;
    qint32 nSuffixLength = sSuffix.length();

    for (qint32 i = 0; i < nSuffixLength; i++) {
        if (sSuffix.at(i).isLetterOrNumber()) {
            sSafeSuffix.append(sSuffix.at(i));
        }
    }

    if (!record.mapProperties.contains(XBinary::FPART_PROP_UNCOMPRESSEDSIZE)) {
        QMessageBox::information(this, tr("Open file"), tr("The unpacked size is unknown. Extract the file before opening it."));
        return;
    }

    qint64 nUncompressedSize = record.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();

    if ((nUncompressedSize < 0) || (nUncompressedSize > N_MAX_OPEN_RECORD_SIZE)) {
        QMessageBox::information(this, tr("Open file"),
                                 tr("Files larger than %1 cannot be opened directly. Extract the file first.")
                                     .arg(XBinary::bytesCountToString(N_MAX_OPEN_RECORD_SIZE, 1024)));
        return;
    }

    QStorageInfo temporaryStorage(QDir::tempPath());
    const qint64 nRequiredFreeSpace = (nUncompressedSize * 2) + (64LL * 1024LL * 1024LL);

    if (temporaryStorage.isValid() && temporaryStorage.isReady() && (temporaryStorage.bytesAvailable() < nRequiredFreeSpace)) {
        QMessageBox::critical(this, tr("Error"), tr("There is not enough free space to open this file."));
        return;
    }

    const QSet<QString> stExecutableSuffixes = QSet<QString>() << "appimage" << "bat" << "bin" << "cmd" << "com" << "command" << "cpl" << "desktop" << "exe"
                                                                 << "hta" << "jar" << "js" << "jse" << "lnk" << "msi" << "msp" << "pif" << "pl" << "ps1" << "py"
                                                                 << "reg" << "run" << "scr" << "sh" << "url" << "vbe" << "vbs" << "wsf" << "wsh";

    if (stExecutableSuffixes.contains(sSafeSuffix)) {
        QMessageBox::StandardButton result = QMessageBox::warning(
            this, tr("Open file"),
            tr("This file type can run code. Open \"%1\" with its default application?").arg(sBaseName),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        if (result != QMessageBox::Yes) {
            return;
        }
    }

    while (m_listTemporaryFiles.count() >= N_MAX_OPEN_TEMPORARY_FILES) {
        QString sOldestFileName = m_listTemporaryFiles.first();

        if (!QFile::exists(sOldestFileName) || QFile::remove(sOldestFileName)) {
            m_listTemporaryFiles.removeFirst();
        } else {
            QMessageBox::information(this, tr("Open file"), tr("Close one of the previously opened files and try again."));
            return;
        }
    }

    QString sTemplate = QDir(QDir::tempPath()).filePath("xfileunpacker_XXXXXX");

    if (!sSafeSuffix.isEmpty()) {
        sTemplate += QString(".%1").arg(sSafeSuffix);
    }

    QTemporaryFile fileTemp(sTemplate);

    if (!fileTemp.open()) {
        QMessageBox::critical(this, tr("Error"), tr("Cannot create temporary file"));
        return;
    }

    QString sTemporaryFileName = fileTemp.fileName();

    if (!extractRecordToDevice(nRow, &fileTemp) || !fileTemp.flush()) {
        QMessageBox::critical(this, tr("Error"), tr("Cannot extract file"));
        return;
    }

    fileTemp.close();
    fileTemp.setAutoRemove(false);
    m_listTemporaryFiles.append(sTemporaryFileName);

    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(sTemporaryFileName))) {
        m_listTemporaryFiles.removeAll(sTemporaryFileName);
        QFile::remove(sTemporaryFileName);
        QMessageBox::critical(this, tr("Error"), tr("Cannot open file"));
    }
}

void ArchiveExplorerWidget::extractRecord()
{
    qint32 nRow = getCurrentRecordIndex();

    if ((nRow < 0) || (nRow >= m_listArchiveRecords.count())) {
        return;
    }

    const XBinary::ARCHIVERECORD &record = m_listArchiveRecords.at(nRow);

    if (isArchiveFolderRecord(record)) {
        return;
    }

    QString sRecordFileName = record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
    QString sSaveFileName = getArchiveRecordBaseName(sRecordFileName);
    QFile *pSourceFile = qobject_cast<QFile *>(m_pDevice);

    if (pSourceFile && !pSourceFile->fileName().isEmpty()) {
        sSaveFileName = QFileInfo(pSourceFile->fileName()).absoluteDir().filePath(sSaveFileName);
    }

    sSaveFileName = QFileDialog::getSaveFileName(this, tr("Extract file"), sSaveFileName);

    if (!sSaveFileName.isEmpty()) {
        if (extractRecordToFile(nRow, sSaveFileName)) {
            XBinary::setFileProperties(record.mapProperties, sSaveFileName);
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Cannot extract file"));
        }
    }
}

void ArchiveExplorerWidget::copyFileName()
{
    QString sFileName = getArchiveRecordBaseName(m_sCurrentRecordFileName);

    if (!sFileName.isEmpty()) {
        QGuiApplication::clipboard()->setText(sFileName);
    }
}

void ArchiveExplorerWidget::copyRecordPath()
{
    if (!m_sCurrentRecordFileName.isEmpty()) {
        QGuiApplication::clipboard()->setText(m_sCurrentRecordFileName);
    }
}

void ArchiveExplorerWidget::copyRecordDetails()
{
    qint32 nRow = getCurrentRecordIndex();

    if ((nRow >= 0) && (nRow < m_listArchiveRecords.count())) {
        QGuiApplication::clipboard()->setText(getRecordDetails(m_listArchiveRecords.at(nRow)));
    }
}

void ArchiveExplorerWidget::showRecordProperties()
{
    qint32 nRow = getCurrentRecordIndex();

    if ((nRow < 0) || (nRow >= m_listArchiveRecords.count())) {
        return;
    }

    QMessageBox messageBox(QMessageBox::Information, tr("Properties"), getRecordDetails(m_listArchiveRecords.at(nRow)), QMessageBox::Ok, this);
    messageBox.setTextFormat(Qt::PlainText);
    messageBox.setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    messageBox.exec();
}

void ArchiveExplorerWidget::refreshRecords()
{
    reloadData(true);
}

qint32 ArchiveExplorerWidget::getCurrentRecordIndex() const
{
    if (!ui->tableViewRecords->getProxyModel()) {
        return -1;
    }

    QModelIndex proxyIndex = ui->tableViewRecords->currentIndex();

    if (!proxyIndex.isValid()) {
        return -1;
    }

    return ui->tableViewRecords->getProxyModel()->mapToSource(proxyIndex).row();
}

bool ArchiveExplorerWidget::extractRecordToDevice(qint32 nRow, QIODevice *pOutputDevice)
{
    if ((nRow < 0) || (nRow >= m_listArchiveRecords.count()) || !pOutputDevice || !pOutputDevice->isOpen() || !pOutputDevice->isWritable() || !m_pDevice ||
        !m_pDevice->isOpen()) {
        return false;
    }

    const XBinary::ARCHIVERECORD &record = m_listArchiveRecords.at(nRow);
    const QString sRecordFileName = record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
    QFile *pSourceFile = qobject_cast<QFile *>(m_pDevice);

    if (m_archiveSource == ARCHIVE_SOURCE_IP7Z) {
        if (!pSourceFile || pSourceFile->fileName().isEmpty() || sRecordFileName.isEmpty()) return false;

        QString sError;
        return XArchives::isIp7zSourceAvailable() &&
               XArchives::extractArchiveRecordWithIp7zSource(pSourceFile->fileName(), sRecordFileName,
                                                            getPassword(), pOutputDevice, &sError, nullptr);
    }

    XBinary *pArchive = XFormats::createClass(m_fileType, m_pDevice);

    if (!pArchive) {
        return false;
    }

    XBinary::PDSTRUCT pdStruct = XBinary::createPdStruct();
    QMap<XBinary::UNPACK_PROP, QVariant> mapProperties;
    mapProperties.insert(XBinary::UNPACK_PROP_PASSWORD, getPassword());
    const bool bResult = pArchive->unpackRecordByIndex(
        nRow, &record, pOutputDevice, mapProperties, &pdStruct);

    delete pArchive;

    return bResult;
}

bool ArchiveExplorerWidget::extractRecordToFile(qint32 nRow, const QString &sFileName)
{
    if (sFileName.isEmpty()) {
        return false;
    }

    QSaveFile fileResult(sFileName);

    if (!fileResult.open(QIODevice::WriteOnly)) {
        return false;
    }

    if (extractRecordToDevice(nRow, &fileResult)) {
        return fileResult.commit();
    }

    fileResult.cancelWriting();

    return false;
}

QString ArchiveExplorerWidget::getRecordDetails(const XBinary::ARCHIVERECORD &record) const
{
    QList<XBinary::FPART_PROP> listProperties = record.mapProperties.keys();
    listProperties.removeAll(XBinary::FPART_PROP_ORIGINALNAME);
    std::sort(listProperties.begin(), listProperties.end());
    listProperties.prepend(XBinary::FPART_PROP_ORIGINALNAME);

    if (!listProperties.contains(XBinary::FPART_PROP_STREAMOFFSET)) {
        listProperties.append(XBinary::FPART_PROP_STREAMOFFSET);
    }

    if (!listProperties.contains(XBinary::FPART_PROP_STREAMSIZE)) {
        listProperties.append(XBinary::FPART_PROP_STREAMSIZE);
    }

    QList<XBinary::ARCHIVERECORD> listRecords;
    listRecords.append(record);
    XModel_ArchiveRecords model(listProperties, &listRecords);
    QStringList listDetails;
    qint32 nNumberOfProperties = listProperties.count();

    for (qint32 i = 0; i < nNumberOfProperties; i++) {
        QString sTitle = model.headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
        QString sValue = model.data(model.index(0, i), Qt::DisplayRole).toString();

        if (!sTitle.isEmpty() && !sValue.isEmpty()) {
            listDetails.append(QString("%1: %2").arg(sTitle, sValue));
        }
    }

    if (listDetails.isEmpty()) {
        return tr("No properties available");
    }

    return listDetails.join(QChar('\n'));
}

void ArchiveExplorerWidget::on_tableViewRecords_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        return;
    }

    ui->tableViewRecords->setCurrentIndex(index);
    ui->tableViewRecords->selectRow(index.row());

    // Opening a member requires extracting it; only meaningful for a real archive.
    if (m_bArchiveAvailable) {
        openRecord();
    }
}

void ArchiveExplorerWidget::onCurrentRecordChanged(const QModelIndex &current, const QModelIndex &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)

    QString sCurrentRecordFileName;
    qint64 nCurrentFileSize = 0;

    qint32 nRow = getCurrentRecordIndex();

    if ((nRow >= 0) && (nRow < m_listArchiveRecords.count())) {
        const XBinary::ARCHIVERECORD &record = m_listArchiveRecords.at(nRow);

        sCurrentRecordFileName = record.mapProperties.value(XBinary::FPART_PROP_ORIGINALNAME).toString();
        nCurrentFileSize = record.mapProperties.value(XBinary::FPART_PROP_UNCOMPRESSEDSIZE).toLongLong();
    }

    if ((m_sCurrentRecordFileName != sCurrentRecordFileName) || (m_nCurrentFileSize != nCurrentFileSize)) {
        m_sCurrentRecordFileName = sCurrentRecordFileName;
        m_nCurrentFileSize = nCurrentFileSize;

        emit currentRecordChanged(m_sCurrentRecordFileName, m_nCurrentFileSize);
    }
}

void ArchiveExplorerWidget::registerShortcuts(bool bState)
{
    Q_UNUSED(bState)
}

void ArchiveExplorerWidget::updateActions()
{
    bool bIsArchive = isArchiveAvailable();

    ui->toolButtonExtractAll->setEnabled(bIsArchive);
    ui->toolButtonTest->setEnabled(bIsArchive);
}

void ArchiveExplorerWidget::loadRecords()
{
    // XTableView may still be sorting or filtering the current model on a
    // worker. Detach it before mutating the QList used as that model's backing
    // store, then install the freshly populated model below.
    ui->tableViewRecords->clear();
    m_pModel = nullptr;
    m_bArchiveAvailable = false;
    m_archiveSource = ARCHIVE_SOURCE_NATIVE;

    bool bHadCurrentRecord = !m_sCurrentRecordFileName.isEmpty() || (m_nCurrentFileSize != 0);

    m_listArchiveRecords.clear();
    m_sCurrentRecordFileName.clear();
    m_nCurrentFileSize = 0;

    if (bHadCurrentRecord) {
        emit currentRecordChanged(QString(), 0);
    }

    QList<XBinary::FPART_PROP> listColumns;

    if (m_pDevice) {
        QFile *pSourceFile = qobject_cast<QFile *>(m_pDevice);
        bool bUseNativeReader = true;
        // Packer/installer handle-method types (Inno Setup, NSIS, ...) must use the native
        // XStaticUnpacker reader: the ip7z (7-Zip) source would open them as a plain PE and list
        // raw sections/overlay instead of the installed files.
        const bool bPreferNative =
            XArchives::isNativeReaderPreferredFileType(m_fileType, m_pDevice, nullptr) || XFormats::isStaticUnpacker(m_fileType);

        if ((!m_bUserFileType) && !bPreferNative && pSourceFile && !pSourceFile->fileName().isEmpty() && XArchives::isIp7zSourceAvailable()) {
            QList<XBinary::ARCHIVERECORD> listIp7zRecords;
            QString sError;

            if (XArchives::listArchiveWithIp7zSource(pSourceFile->fileName(), getPassword(), &listIp7zRecords, &sError, nullptr)) {
                m_listArchiveRecords = listIp7zRecords;
                m_bArchiveAvailable = true;
                m_archiveSource = ARCHIVE_SOURCE_IP7Z;
                bUseNativeReader = false;
            } else if (!XArchives::isIp7zUnsupportedFormatError(sError) && !getPassword().isEmpty()) {
                bUseNativeReader = false;
            }
        }

        if (bUseNativeReader) {
            XBinary *pArchive = XFormats::createClass(m_fileType, m_pDevice);

            if (pArchive) {
                listColumns = pArchive->getAvailableFPARTProperties();
                XBinary::UNPACK_STATE state = {};
                QMap<XBinary::UNPACK_PROP, QVariant> mapProperties;
                mapProperties.insert(XBinary::UNPACK_PROP_PASSWORD, getPassword());
                bool bInit = pArchive->initUnpack(&state, mapProperties, nullptr);

                if (!bInit) {
                    state = XBinary::UNPACK_STATE();
                    mapProperties.insert(XBinary::UNPACK_PROP_METADATAONLY, true);
                    bInit = pArchive->initUnpack(&state, mapProperties, nullptr);
                }

                bool bComplete = bInit;
                while (bComplete && (state.nCurrentIndex < state.nNumberOfRecords)) {
                    const XBinary::ARCHIVERECORD record = pArchive->infoCurrent(&state, nullptr);
                    if (record.mapProperties.isEmpty()) {
                        bComplete = false;
                        break;
                    }
                    m_listArchiveRecords.append(record);
                    if (!pArchive->moveToNext(&state, nullptr) && (state.nCurrentIndex < state.nNumberOfRecords)) {
                        bComplete = false;
                    }
                }

                bComplete = bComplete && (state.nCurrentIndex == state.nNumberOfRecords) &&
                            pArchive->finishUnpack(&state, nullptr);
                if (bComplete) {
                    m_bArchiveAvailable = true;
                } else {
                    m_listArchiveRecords.clear();
                    listColumns.clear();
                }

                delete pArchive;
            }
        }
    }

    // If the selected file type could not be unpacked (initUnpack returned
    // false, or the record enumeration was incomplete), fall back to showing the
    // file itself as a single entry. m_bArchiveAvailable stays false, so
    // Extract / Extract all / Test remain disabled.
    if ((!m_bArchiveAvailable) && m_pDevice && m_listArchiveRecords.isEmpty()) {
        XBinary::ARCHIVERECORD record = {};

        QString sName = tr("(file)");
        QFile *pSourceFile = qobject_cast<QFile *>(m_pDevice);

        if (pSourceFile && !pSourceFile->fileName().isEmpty()) {
            sName = QFileInfo(pSourceFile->fileName()).fileName();
        }

        qint64 nSize = m_pDevice->size();

        record.nStreamOffset = 0;
        record.nStreamSize = nSize;
        record.mapProperties.insert(XBinary::FPART_PROP_ORIGINALNAME, sName);
        record.mapProperties.insert(XBinary::FPART_PROP_UNCOMPRESSEDSIZE, nSize);

        m_listArchiveRecords.append(record);
    }

    // show every property the format parser actually filled, in a fixed preferred
    // order, so each archive type exposes the maximum available information
    {
        QList<XBinary::FPART_PROP> listPreferred;
        listPreferred.append(XBinary::FPART_PROP_ORIGINALNAME);
        listPreferred.append(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
        listPreferred.append(XBinary::FPART_PROP_COMPRESSEDSIZE);
        listPreferred.append(XBinary::FPART_PROP_HANDLEMETHOD);
        listPreferred.append(XBinary::FPART_PROP_DATETIME);
        listPreferred.append(XBinary::FPART_PROP_MTIME);
        listPreferred.append(XBinary::FPART_PROP_CTIME);
        listPreferred.append(XBinary::FPART_PROP_ATIME);
        listPreferred.append(XBinary::FPART_PROP_UNCOMPRESSEDCRC);
        listPreferred.append(XBinary::FPART_PROP_RESULTCRC);
        listPreferred.append(XBinary::FPART_PROP_CHECKSUM);
        listPreferred.append(XBinary::FPART_PROP_CHECKSUMTYPE);
        listPreferred.append(XBinary::FPART_PROP_ENCRYPTED);
        listPreferred.append(XBinary::FPART_PROP_FILEMODE);
        listPreferred.append(XBinary::FPART_PROP_USERNAME);
        listPreferred.append(XBinary::FPART_PROP_GROUPNAME);
        listPreferred.append(XBinary::FPART_PROP_UID);
        listPreferred.append(XBinary::FPART_PROP_GID);
        listPreferred.append(XBinary::FPART_PROP_LINKNAME);
        listPreferred.append(XBinary::FPART_PROP_INFO);
        listPreferred.append(XBinary::FPART_PROP_HOSTOS);

        QList<XBinary::FPART_PROP> listPresent;
        QSet<XBinary::FPART_PROP> stAdded;
        qint32 nNumberOfPreferred = listPreferred.count();
        qint32 nNumberOfRecords = m_listArchiveRecords.count();

        for (qint32 i = 0; i < nNumberOfPreferred; i++) {
            XBinary::FPART_PROP fpartProp = listPreferred.at(i);
            bool bPresent = false;

            for (qint32 j = 0; (j < nNumberOfRecords) && (!bPresent); j++) {
                bPresent = m_listArchiveRecords.at(j).mapProperties.contains(fpartProp);
            }

            if (bPresent) {
                listPresent.append(fpartProp);
                stAdded.insert(fpartProp);
            }
        }

        if (m_bAdvanced) {
            // advanced mode: additionally show every remaining property the
            // format parser filled, in enum order
            QSet<XBinary::FPART_PROP> stAll;

            for (qint32 j = 0; j < nNumberOfRecords; j++) {
                QList<XBinary::FPART_PROP> listKeys = m_listArchiveRecords.at(j).mapProperties.keys();
                qint32 nNumberOfKeys = listKeys.count();

                for (qint32 k = 0; k < nNumberOfKeys; k++) {
                    stAll.insert(listKeys.at(k));
                }
            }

            QList<XBinary::FPART_PROP> listAll = stAll.values();
            std::sort(listAll.begin(), listAll.end());

            qint32 nNumberOfProps = listAll.count();

            for (qint32 i = 0; i < nNumberOfProps; i++) {
                XBinary::FPART_PROP fpartProp = listAll.at(i);

                if (!stAdded.contains(fpartProp)) {
                    listPresent.append(fpartProp);
                    stAdded.insert(fpartProp);
                }
            }
        }

        if (listPresent.count() > 1) {
            // stream location inside the archive file is always known
            if (!stAdded.contains(XBinary::FPART_PROP_STREAMOFFSET)) {
                listPresent.append(XBinary::FPART_PROP_STREAMOFFSET);
            }

            if (!stAdded.contains(XBinary::FPART_PROP_STREAMSIZE)) {
                listPresent.append(XBinary::FPART_PROP_STREAMSIZE);
            }

            listColumns = listPresent;
        }
    }

    if (listColumns.isEmpty()) {
        listColumns.append(XBinary::FPART_PROP_ORIGINALNAME);
        listColumns.append(XBinary::FPART_PROP_COMPRESSEDSIZE);
        listColumns.append(XBinary::FPART_PROP_UNCOMPRESSEDSIZE);
        listColumns.append(XBinary::FPART_PROP_STREAMOFFSET);
        listColumns.append(XBinary::FPART_PROP_STREAMSIZE);
        listColumns.append(XBinary::FPART_PROP_HANDLEMETHOD);
    }

    m_pModel = new XModel_ArchiveRecords(listColumns, &m_listArchiveRecords, this);

    ui->tableViewRecords->setCustomModel(m_pModel, true);

    connect(ui->tableViewRecords->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this,
            SLOT(onCurrentRecordChanged(QModelIndex, QModelIndex)), Qt::UniqueConnection);

    emit recordsLoaded(m_listArchiveRecords.count());
}

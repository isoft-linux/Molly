/*
 * Copyright (C) 2017 Leslie Zhai <xiang.zhai@i-soft.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "parttofilewidget.h"
#include "imgdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QHeaderView>
#include <QProgressBar>

#include <UDisks2Qt5/UDisksObject>
#include <UDisks2Qt5/UDisksDrive>

#include <pthread.h>
#include <libpartclone.h>

static QProgressBar *m_progress = Q_NULLPTR;
static pthread_t m_thread;
static pthread_mutex_t m_mutex = PTHREAD_MUTEX_INITIALIZER;

static void *callBack(void *percent, void *remaining);

PartToFileWidget::PartToFileWidget(UDisksClient *oUDisksClient, 
                                   QWidget *parent, 
                                   Qt::WindowFlags f)
  : QWidget(parent, f),
    m_UDisksClient(oUDisksClient)
{
    connect(m_UDisksClient, &UDisksClient::objectAdded, [=](const UDisksObject::Ptr &object) {
        getDriveObjects();
    });
    connect(m_UDisksClient, &UDisksClient::objectRemoved, [=](const UDisksObject::Ptr &object) {
        getDriveObjects();
    });
    connect(m_UDisksClient, &UDisksClient::objectsAvailable, [=]() {
        getDriveObjects();
    });

    auto *vbox = new QVBoxLayout;
    auto *hbox = new QHBoxLayout;
    auto *label = new QLabel(tr("Please choose the Disk:"));
    hbox->addWidget(label);
    m_combo = new QComboBox;
    hbox->addWidget(m_combo);
    vbox->addLayout(hbox);
    m_table = new QTableWidget;
    QStringList headers {tr("Partition"), tr("Size"), tr("Type"), tr("OS"), tr("Status")};
    m_table->setColumnCount(headers.size());
    m_table->setHorizontalHeaderLabels(headers);
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_browseBtn = new QPushButton(tr("Browse"));
    m_cloneBtn = new QPushButton(tr("Clone"));
    m_cloneBtn->setEnabled(false);
    m_edit = new QLineEdit;
    connect(m_edit, &QLineEdit::textChanged, [=](const QString &text) {
        QList<QTableWidgetItem *> items = m_table->selectedItems();
        m_cloneBtn->setEnabled(items.size() && ImgDialog::isPathWritable(text) && (!m_edit->text().isEmpty()));
    });
    connect(m_browseBtn, &QPushButton::clicked, [=]() {
        QList<QTableWidgetItem *> items = m_table->selectedItems();
#ifdef DEBUG
        qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << items;
#endif
        if (items.size()) {
            auto *dlg = new ImgDialog(items[0]->text(), 
                                      m_OSMap, 
                                      tr("Choose a Partition to save the image"), 
                                      this);
            connect(dlg, &ImgDialog::savePathSelected, [=](QString path) {
                dlg->close();
#ifdef DEBUG
                qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << path;
#endif
                m_edit->setText(path);
            });
            dlg->exec();
        }
    });
    m_browseBtn->setEnabled(false);
    connect(m_table, &QTableWidget::itemSelectionChanged, [=]() {
        QList<QTableWidgetItem *> items = m_table->selectedItems();
        if (items.size()) {
            m_browseBtn->setEnabled(true);
            m_edit->setText("");
        }
    });
    vbox->addWidget(m_table);
    hbox = new QHBoxLayout;
    label = new QLabel(tr("Partition image save path:"));
    hbox->addWidget(label);
    hbox->addWidget(m_edit);
    hbox->addWidget(m_browseBtn);
    vbox->addLayout(hbox);
    hbox = new QHBoxLayout;
    m_progress = new QProgressBar;
    m_progress->setTextVisible(true);
    vbox->addWidget(m_progress);
    m_progress->setVisible(false);
    auto *backBtn = new QPushButton(tr("Back"));
    connect(backBtn, &QPushButton::clicked, [=]() { Q_EMIT back(); });
    connect(m_cloneBtn, &QPushButton::clicked, [=]() {
        if (m_edit->text().isEmpty()) {
            return;
        }
        m_isError = false;
#ifdef DEBUG
        qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << m_isClone;
#endif
        QList<QTableWidgetItem *> items = m_table->selectedItems();
        if (m_isClone) {
            if (items.size() && ImgDialog::isPathWritable(m_edit->text())) {
                m_progress->setVisible(true);
                m_progress->setValue(0);
                m_cloneBtn->setText(tr("Cancel"));
                m_combo->setEnabled(false);
                m_table->setEnabled(false);
                m_browseBtn->setEnabled(false);
                m_edit->setEnabled(false);
                backBtn->setEnabled(false);
                pthread_create(&m_thread, NULL, startRoutine, this);
            }
        } else {
            if (items.size() > 4) {
                items[4]->setText(tr("Cancel"));
            }
            m_progress->setVisible(false);
            m_cloneBtn->setText(tr("Canceling..."));
            m_cloneBtn->setEnabled(false);
            partCloneCancel(1);
        }
        m_isClone = !m_isClone;
    });
    hbox->addWidget(m_cloneBtn);
    hbox->addWidget(backBtn);
    vbox->addLayout(hbox);
    setLayout(vbox);

    connect(this, &PartToFileWidget::error, [=](QString message) {
        m_isError = true;
#ifdef DEBUG
        qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << m_isError << message;
#endif
        m_isClone = true;
        QList<QTableWidgetItem *> items = m_table->selectedItems();
        if (items.size() > 4) {
            items[4]->setText(tr("Error"));
        }
        m_progress->setVisible(false);
        m_cloneBtn->setText(tr("Clone"));
        m_combo->setEnabled(true);
        m_table->setEnabled(true);
        m_browseBtn->setEnabled(true);
        m_edit->setEnabled(true);
        backBtn->setEnabled(true);
    });
    connect(this, &PartToFileWidget::finished, [=]() {
#ifdef DEBUG
        qDebug() << "DEBUG:" << __PRETTY_FUNCTION__;
#endif
        m_isClone = true;
        QList<QTableWidgetItem *> items = m_table->selectedItems();
        if (!m_isError && items.size() > 4) {
            items[4]->setText(tr("Finished"));
        }
        m_progress->setVisible(false);
        m_cloneBtn->setText(tr("Clone"));
        m_cloneBtn->setEnabled(true);
        m_combo->setEnabled(true);
        m_table->setEnabled(true);
        m_browseBtn->setEnabled(true);
        m_edit->setEnabled(true);
        backBtn->setEnabled(true);
    });
}

PartToFileWidget::~PartToFileWidget()
{
    if (m_combo) {
        delete m_combo;
        m_combo = Q_NULLPTR;
    }

    if (m_browseBtn) {
        delete m_browseBtn;
        m_browseBtn = Q_NULLPTR;
    }
}

void PartToFileWidget::setOSMap(OSMapType OSMap) 
{ 
    m_OSMap = OSMap; 
#ifdef DEBUG
    qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << m_OSMap;
#endif
    comboTextChanged(m_combo->currentText());
}

bool PartToFileWidget::isPartAbleToShow(const UDisksPartition *part, 
                                        UDisksBlock *blk,
                                        UDisksFilesystem *fsys, 
                                        QTableWidgetItem *item) 
{
#ifdef DEBUG
    if (fsys)
        qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << fsys->mountPoints();
#endif
    if (!part || part->isContainer() || !blk || blk->idType() == "swap" || 
        !fsys || (!fsys->mountPoints().isEmpty() && !fsys->mountPoints().contains("/var/lib/os-prober/mount"))) {
        item->setFlags(Qt::NoItemFlags);
        return false;
    }

    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    return true;
}

void PartToFileWidget::comboTextChanged(QString text) 
{
    m_browseBtn->setEnabled(false);
    m_table->setRowCount(0);
    m_table->clearContents();

    if (text.isEmpty())
        return;

    QDBusObjectPath tblPath = QDBusObjectPath(udisksDBusPathPrefix + text.mid(5));
    QList<UDisksPartition *> parts;
    for (const UDisksObject::Ptr partPtr : m_UDisksClient->getPartitions(tblPath)) {
        UDisksPartition *part = partPtr->partition();
        parts << part;
    }
    qSort(parts.begin(), parts.end(), [](UDisksPartition *p1, UDisksPartition *p2) -> bool {
        return p1->number() < p2->number();
    });

    int row = 0;
    for (const UDisksPartition *part : parts) {
        UDisksObject::Ptr objPtr = m_UDisksClient->getObject(QDBusObjectPath(udisksDBusPathPrefix + text.mid(5) + QString::number(part->number())));
        if (!objPtr)
            continue;
        UDisksBlock *blk = objPtr->block();
        if (!blk) 
            continue;
        UDisksFilesystem *fsys = objPtr->filesystem();
#ifdef DEBUG
        qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << fsys;
#endif

        m_table->insertRow(row);
        QString partStr = text + QString::number(part->number());
        auto *item = new QTableWidgetItem(partStr);
        isPartAbleToShow(part, blk, fsys, item);
        m_table->setItem(row, 0, item);

        item = new QTableWidgetItem(QString::number(part->size() / 1073741824.0, 'f', 1) + " G");
        isPartAbleToShow(part, blk, fsys, item);
        m_table->setItem(row, 1, item);

        item = new QTableWidgetItem(blk->idType());
        isPartAbleToShow(part, blk, fsys, item);
        m_table->setItem(row, 2, item);

#ifdef DEBUG
        qDebug() << "DEBUG:" << __PRETTY_FUNCTION__ << m_OSMap << partStr;
#endif
        item = new QTableWidgetItem(m_OSMap[partStr]);
        isPartAbleToShow(part, blk, fsys, item);
        m_table->setItem(row, 3, item);

        item = new QTableWidgetItem("");
        isPartAbleToShow(part, blk, fsys, item);
        m_table->setItem(row, 4, item);
        row++;
    }

    UDisksObject::Ptr blkPtr = m_UDisksClient->getObject(tblPath);
    if (blkPtr) {
        UDisksBlock *blk = blkPtr->block();
        if (blk) {
            UDisksObject::Ptr drvPtr = blk->driveObjectPtr();
            if (drvPtr) {
                UDisksDrive *drv = drvPtr->drive();
                if (drv)
                    m_combo->setToolTip(drv->id());
            }
        }
    }
}

void PartToFileWidget::getDriveObjects() 
{
    m_combo->clear();

    for (const UDisksObject::Ptr drvPtr : m_UDisksClient->getObjects(UDisksObject::Drive)) {
        UDisksDrive *drv = drvPtr->drive();
        if (!drv)
            continue;
        UDisksBlock *blk = drv->getBlock();
        if (!blk)
            continue;
        m_combo->addItem(blk->preferredDevice());
    }

    connect(m_combo, &QComboBox::currentTextChanged, [=](const QString &text) {
        comboTextChanged(text);
    });
    comboTextChanged(m_combo->currentText());
}

static void *callBack(void *percent, void *remaining) 
{
    pthread_mutex_trylock(&m_mutex);
    float *value = (float *)percent;
    char *str = (char *)remaining;
    if (m_progress) {
        m_progress->setValue((int)*value);
        m_progress->setFormat(QString::number((int)*value) + "% " + QString(str));
    }
    pthread_mutex_unlock(&m_mutex);
    return Q_NULLPTR;
}

void *PartToFileWidget::errorRoutine(void *arg, void *msg) 
{
    PartToFileWidget *thisPtr = (PartToFileWidget *)arg;
    if (!thisPtr)
        return Q_NULLPTR;

    char *str = (char *)msg;
    Q_EMIT thisPtr->error(str ? QString(str) : "");

    return Q_NULLPTR;
}

void *PartToFileWidget::startRoutine(void *arg) 
{
    PartToFileWidget *thisPtr = (PartToFileWidget *)arg;
    if (!thisPtr || !thisPtr->m_table || !thisPtr->m_edit || !thisPtr->m_UDisksClient)
        return Q_NULLPTR;
    partType type = LIBPARTCLONE_UNKNOWN;
    QList<QTableWidgetItem *> items = thisPtr->m_table->selectedItems();
    if (items.isEmpty())
        return Q_NULLPTR;
    QString part = items[0]->text();
    QString img = thisPtr->m_edit->text();

    if (img.isEmpty())
        return Q_NULLPTR;
    UDisksObject::Ptr blkPtr = thisPtr->m_UDisksClient->getObject(QDBusObjectPath(udisksDBusPathPrefix + part.mid(5)));
    if (!blkPtr)
        return Q_NULLPTR;
    UDisksBlock *blk = blkPtr->block();
    if (!blk)
        return Q_NULLPTR;
    QString strType = blk->idType();
    if (strType.startsWith("ext"))
        type = LIBPARTCLONE_EXTFS;
    else if (strType == "ntfs")
        type = LIBPARTCLONE_NTFS;
    else if (strType == "vfat")
        type = LIBPARTCLONE_FAT;
    // TODO: more file system test
    partClone(type, 
              part.toStdString().c_str(), 
              img.toStdString().c_str(),
              1,
              callBack,
              errorRoutine,
              thisPtr);
    Q_EMIT thisPtr->finished();
    return Q_NULLPTR;
}

#include "moc_parttofilewidget.cpp"

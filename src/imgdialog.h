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

#ifndef IMG_DIALOG_H
#define IMG_DIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTableWidget>
#include <QPushButton>

#include <UDisks2Qt5/UDisksClient>

class PrevWidget;
class NextWidget;
class ImgDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * Partition or Disk clone to Image save path Dialog.
     *
     * @param selected the selected Partition or Disk
     * @param OSMap isoft-os-prober object ptr
     * @param title the ImgDialog's windowTitle
     */
    explicit ImgDialog(QString selected, 
                       QMap<QString, QString> OSMap, 
                       QString title, 
                       QWidget *parent = Q_NULLPTR, 
                       Qt::WindowFlags f = Qt::Dialog);
    virtual ~ImgDialog();
    static bool isPathWritable(QString path);

Q_SIGNALS:
    void savePathSelected(QString path);

private:
    void getDriveObjects();
    bool isPartAbleToShow(const UDisksPartition *part, 
                          UDisksBlock *blk,
                          UDisksFilesystem *fsys,
                          bool isSelected, 
                          QTableWidgetItem *item);

    UDisksClient *m_UDisksClient = Q_NULLPTR;
    QMap<QString, QString> m_OSMap;
    qulonglong m_selectedSize = 0;
    PrevWidget *m_prev = Q_NULLPTR;
    NextWidget *m_next = Q_NULLPTR;
};

class PrevWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PrevWidget(QWidget *parent = Q_NULLPTR, 
                        Qt::WindowFlags f = Qt::Tool);
    virtual ~PrevWidget();

Q_SIGNALS:
    void other();
    void next(QString text);

friend class ImgDialog;
private:
    QListWidget *list = Q_NULLPTR;
};

class NextWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NextWidget(QString selected, 
                        UDisksClient *oUDisksClient, 
                        QWidget *parent = Q_NULLPTR, 
                        Qt::WindowFlags f = Qt::Tool);
    virtual ~NextWidget();

Q_SIGNALS:
    void savePathSelected(QString path);
    void prev();

friend class ImgDialog;
private:
    QTableWidget *table = Q_NULLPTR;
    QPushButton *confirmBtn = Q_NULLPTR;
};

#endif // IMG_DIALOG_H

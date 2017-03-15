/*
 * Copyright (C) 2017 fj <fujiang.zhu@i-soft.com.cn>
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

#ifndef DISKTOFILE_WIDGET_H
#define DISKTOFILE_WIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QPushButton>

#include <UDisks2Qt5/UDisksClient>
#include <UDisks2Qt5/UDisksPartition>
#include <UDisks2Qt5/UDisksBlock>
#include <UDisks2Qt5/UDisksFilesystem>

#include "stepwidget.h"

class DiskToFileWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DiskToFileWidget(OSProberType *OSProber,
                              QWidget *parent = Q_NULLPTR, 
                              Qt::WindowFlags f = Qt::Tool);
    virtual ~DiskToFileWidget();

Q_SIGNALS:
    void back();
    void next(StepType type);

friend class ImgDialog;
private:
    void getDriveObjects();
    void comboTextChanged(QString text);
    static bool isPartAbleToShow(const UDisksPartition *part, 
                                 UDisksBlock *blk,
                                 UDisksFilesystem *fsys, 
                                 QTableWidgetItem *item);

    QTableWidget *m_table = Q_NULLPTR;
    QPushButton *m_browseBtn = Q_NULLPTR;
    QPushButton *m_cloneBtn = Q_NULLPTR;
    OSProberType *m_OSProber = Q_NULLPTR;
    UDisksClient *m_UDisksClient = Q_NULLPTR;
    QMap<QString, QString> m_OSMap;
};

#endif // DISKTOFILE_WIDGET_H
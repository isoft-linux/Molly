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

#ifndef RESTORE_WIDGET_H
#define RESTORE_WIDGET_H

#include <QWidget>

#include "stepwidget.h"

class RestoreWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RestoreWidget(QWidget *parent = Q_NULLPTR, Qt::WindowFlags f = Qt::Tool);
    virtual ~RestoreWidget();

Q_SIGNALS:
    void next(StepType type);
    void back();
};

#endif // RESTORE_WIDGET_H

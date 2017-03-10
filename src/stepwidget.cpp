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

#include "stepwidget.h"
#include "wizardwidget.h"
#include "partclonewidget.h"
#include "diskclonewidget.h"
#include "parttofilewidget.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QIcon>
#include <QHBoxLayout>
#include <QStackedWidget>
#include <QDebug>

StepWidget::StepWidget(int argc, char **argv, QWidget *parent, Qt::WindowFlags f) 
    : QWidget(parent, f) 
{
    setWindowTitle(tr("iSOFT Partition or Disk clone and restore Assistant"));
    setWindowIcon(QIcon::fromTheme("drive-harddisk"));
    setFixedSize(625, 400);
    move((QApplication::desktop()->width() - width()) / 2, 
         (QApplication::desktop()->height() - height()) / 2);

    auto *hbox = new QHBoxLayout;
    hbox->setContentsMargins(0, 0, 0, 0);
    setLayout(hbox);
    
    auto *stack = new QStackedWidget;
    auto *wizard = new WizardWidget;
    connect(wizard, &WizardWidget::next, [=](StepType type) { stack->setCurrentIndex(type); });
    auto *partClone = new PartCloneWidget;
    connect(partClone, &PartCloneWidget::back, [=]() { stack->setCurrentIndex(WIZARD); });
    connect(partClone, &PartCloneWidget::next, [=](StepType type) { stack->setCurrentIndex(type); });
    auto *diskClone = new DiskcloneWidget;
    connect(diskClone, &DiskcloneWidget::back, [=]() { stack->setCurrentIndex(WIZARD); });
    auto *partToFile = new PartToFileWidget;
    connect(partToFile, &PartToFileWidget::back, [=]() { stack->setCurrentIndex(PARTCLONE); });
    stack->addWidget(wizard);       // 0
    stack->addWidget(partClone);    // 1
    stack->addWidget(diskClone);    // 2
    stack->addWidget(partToFile);   // 3
    hbox->addWidget(stack);
}

StepWidget::~StepWidget()
{
}

#include "moc_stepwidget.cpp"

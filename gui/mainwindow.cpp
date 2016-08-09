/*
 * Copyright (C) 2015-2016 Olzhas Rakhimov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QGraphicsScene>

#include "event.h"

namespace scram {
namespace gui {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    auto *aboutQtAction = new QAction(tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("About the Qt toolkit"));
    connect(aboutQtAction, &QAction::triggered, qApp, &QApplication::aboutQt);
    ui->menuHelp->addAction(aboutQtAction);

    auto *scene = new QGraphicsScene;
    ui->diagrams->setScene(scene);
}

MainWindow::~MainWindow() { delete ui; }

} // namespace gui
} // namespace scram

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
#include <QCoreApplication>
#include <QGraphicsScene>
#include <QMessageBox>

#include "event.h"

namespace scram {
namespace gui {

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupActions();

    auto *scene = new QGraphicsScene;
    ui->diagrams->setScene(scene);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setupActions()
{
    connect(ui->actionAboutQt, &QAction::triggered, qApp,
            &QApplication::aboutQt);
    connect(ui->actionAboutScram, &QAction::triggered, this, [this] {
        QMessageBox::about(
            this, tr("About SCRAM"),
            tr("<h1 align=\"center\">SCRAM %1</h1>"
               "The GUI front-end for SCRAM,<br/>"
               "a command-line risk analysis multi-tool.<br/><br/>"
               "License: GPLv3+<br/>"
               "Homepage: <a href=\"%2\">%2</a><br/>"
               "The Model Exchange Format: <a href=\"%3\">%3</a><br/>"
               "Technical support: <a href=\"%4\">forum</a><br/>"
               "Bug Tracker: <a href=\"%5\">%5</a>")
                .arg(QCoreApplication::applicationVersion())
                .arg(QString::fromLatin1("https://scram-pra.org"))
                .arg(QString::fromLatin1("https://open-psa.github.io/mef"))
                .arg(QString::fromLatin1(
                    "https://groups.google.com/forum/#!forum/scram-users"))
                .arg(QString::fromLatin1(
                    "https://github.com/rakhimov/scram/issues")));
    });
    ui->actionExit->setShortcuts(QKeySequence::Quit);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    ui->actionNewProject->setShortcuts(QKeySequence::New);
    connect(ui->actionNewProject, &QAction::triggered, this,
            &MainWindow::createNewProject);
}

void MainWindow::createNewProject()
{
    m_config.clear();
    /// @todo Implement the new project creation.
}

} // namespace gui
} // namespace scram

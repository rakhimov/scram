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
#include <QFileDialog>
#include <QGraphicsScene>
#include <QMessageBox>

#include "event.h"
#include "src/config.h"
#include "src/error.h"
#include "src/initializer.h"
#include "src/xml.h"

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

MainWindow::~MainWindow() = default;

void MainWindow::setConfig(const std::string &config_path)
{
    try {
        Config config(config_path);
        mef::Initializer(config.input_files(), config.settings());
        m_config.parser = scram::ConstructDomParser(config_path);
        m_input_files = config.input_files();
    } catch (scram::Error &err) {
        QMessageBox::critical(this, tr("Configuration Error"),
                              QString::fromUtf8(err.what()));
        return;
    }

    m_config.file = config_path;
    m_config.xml = m_config.parser->get_document()->get_root_node();

    emit configChanged();
}

void MainWindow::addInputFiles(const std::vector<std::string> &input_files) {
    if (input_files.empty())
        return;

    try {
        Config config(m_config.file);
        std::vector<std::string> all_input(m_input_files);
        all_input.insert(all_input.end(), input_files.begin(),
                         input_files.end());
        mef::Initializer(all_input, config.settings());
    } catch (scram::Error &err) {
        QMessageBox::critical(this, tr("Initialization Error"),
                              QString::fromUtf8(err.what()));
        return;
    }

    xmlpp::Element* container_element = [this] {
        xmlpp::NodeSet elements = m_config.xml->find("./input-files");
        if (elements.empty())
            return m_config.xml->add_child("input-files");
        return static_cast<xmlpp::Element *>(elements.front());
    }();
    for (const auto& input_file : input_files)
        container_element->add_child("file")->add_child_text(input_file);
    m_input_files.insert(m_input_files.end(), input_files.begin(),
                         input_files.end());

    emit configChanged();
}

void MainWindow::setupActions()
{
    connect(ui->actionAboutQt, &QAction::triggered, qApp,
            &QApplication::aboutQt);
    connect(ui->actionAboutScram, &QAction::triggered, this, [this] {
        QMessageBox::about(
            this, tr("About SCRAM"),
            tr("<h1>SCRAM %1</h1>"
               "The GUI front-end for SCRAM,<br/>"
               "a command-line risk analysis multi-tool.<br/><br/>"
               "License: GPLv3+<br/>"
               "Homepage: <a href=\"%2\">%2</a><br/>"
               "Technical Support: <a href=\"%3\">%3</a><br/>"
               "Bug Tracker: <a href=\"%4\">%4</a><br/><br/>"
               "This program is distributed in the hope that it will be useful,"
               " but WITHOUT ANY WARRANTY; without even the implied warranty of"
               " MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
               "GNU General Public License for more details.")
                .arg(QCoreApplication::applicationVersion(),
                     QString::fromLatin1("https://scram-pra.org"),
                     QString::fromLatin1("scram-users@googlegroups.com"),
                     QString::fromLatin1(
                         "https://github.com/rakhimov/scram/issues")));
    });
    ui->actionExit->setShortcut(QKeySequence::Quit);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    ui->actionNewProject->setShortcut(QKeySequence::New);
    connect(ui->actionNewProject, &QAction::triggered, this,
            &MainWindow::createNewProject);
    ui->actionOpenProject->setShortcut(QKeySequence::Open);
    connect(ui->actionOpenProject, &QAction::triggered, this,
            &MainWindow::openProject);
}

void MainWindow::createNewProject()
{
    m_config.file.clear();
    m_config.parser->parse_memory("<?xml version=\"1.0\"?><scram/>");
    m_config.xml = m_config.parser->get_document()->get_root_node();
    m_input_files.clear();

    emit configChanged();
}

void MainWindow::openProject()
{
    QString filename = QFileDialog::getOpenFileName(
        this, tr("Open Project"), QDir::currentPath(),
        tr("XML files (*.scram *.xml);;All files (*.*)"));
    if (filename.isNull())
        return;
    setConfig(filename.toStdString());
}

} // namespace gui
} // namespace scram

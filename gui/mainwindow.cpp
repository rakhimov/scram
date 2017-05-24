/*
 * Copyright (C) 2015-2017 Olzhas Rakhimov
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
#include <QTableWidget>

#include "event.h"
#include "guiassert.h"

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

    connect(ui->treeWidget, &QTreeWidget::itemActivated, this,
            &MainWindow::showElement);
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this,
            [this](int index) { ui->tabWidget->removeTab(index); });
}

MainWindow::~MainWindow() = default;

void MainWindow::setConfig(const std::string &configPath,
                           std::vector<std::string> inputFiles)
{
    try {
        Config config(configPath);
        mef::Initializer(config.input_files(), config.settings());
        inputFiles.insert(inputFiles.begin(), config.input_files().begin(),
                          config.input_files().end());
        addInputFiles(inputFiles);
        m_config.reset(configPath);
        m_settings = config.settings();
    } catch (scram::Error &err) {
        QMessageBox::critical(this, tr("Configuration Error"),
                              QString::fromUtf8(err.what()));
        return;
    }

    ui->actionSaveProject->setEnabled(true);
    ui->actionSaveProjectAs->setEnabled(true);

    emit configChanged();
}

void MainWindow::addInputFiles(const std::vector<std::string> &inputFiles)
{
    if (inputFiles.empty())
        return;

    try {
        std::vector<std::string> all_input = extractInputFiles();
        all_input.insert(all_input.end(), inputFiles.begin(),
                         inputFiles.end());
        mef::Initializer(all_input, m_settings);
    } catch (scram::Error &err) {
        QMessageBox::critical(this, tr("Initialization Error"),
                              QString::fromUtf8(err.what()));
        return;
    }

    for (const auto& input : inputFiles)
        m_inputFiles.emplace_back(input);

    resetTreeWidget();

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

    ui->actionNewProject->setShortcut(QKeySequence::New);
    connect(ui->actionNewProject, &QAction::triggered, this,
            &MainWindow::createNewProject);

    ui->actionOpenProject->setShortcut(QKeySequence::Open);
    connect(ui->actionOpenProject, &QAction::triggered, this,
            &MainWindow::openProject);

    ui->actionSaveProject->setShortcut(QKeySequence::Save);
    connect(ui->actionSaveProject, &QAction::triggered, this,
            &MainWindow::saveProject);

    ui->actionSaveProjectAs->setShortcut(QKeySequence::SaveAs);
    connect(ui->actionSaveProjectAs, &QAction::triggered, this,
            &MainWindow::saveProjectAs);
}

void MainWindow::createNewProject()
{
    GUI_ASSERT(m_config.parser,);
    m_config.parser->parse_memory("<?xml version=\"1.0\"?><scram/>");
    m_config.file.clear();
    m_config.xml = m_config.parser->get_document()->get_root_node();
    m_inputFiles.clear();

    resetTreeWidget();

    ui->actionSaveProject->setEnabled(true);
    ui->actionSaveProjectAs->setEnabled(true);

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

void MainWindow::saveProject()
{
    if (m_config.file.empty())
        return saveProjectAs();

    xmlpp::Element *container_element = [this] {
        xmlpp::NodeSet elements = m_config.xml->find("./input-files");
        if (elements.empty())
            return m_config.xml->add_child("input-files");
        return static_cast<xmlpp::Element *>(elements.front());
    }();

    for (xmlpp::Node *node : container_element->find("./file"))
        container_element->remove_child(node);

    /// @todo Save as relative paths.
    for (const XmlFile &input : m_inputFiles)
        container_element->add_child("file")->add_child_text(input.file);

    m_config.parser->get_document()->write_to_file_formatted(
        m_config.file, m_config.parser->get_document()->get_encoding());

    /// @todo Save the input files.
}

void MainWindow::saveProjectAs()
{
    QString filename = QFileDialog::getSaveFileName(
        this, tr("Save Project"), QDir::currentPath(),
        tr("XML files (*.scram *.xml);;All files (*.*)"));
    if (filename.isNull())
        return;
    /// @todo Save the input files into custom places.
    m_config.file = filename.toStdString();
    saveProject();
}

void MainWindow::showElement(QTreeWidgetItem *item)
{
    QString name = item->data(0, Qt::DisplayRole).toString();
    if (name == tr("Basic Events")) {
        auto *table = new QTableWidget(nullptr);
        table->setColumnCount(2);
        table->setHorizontalHeaderLabels({tr("Name"), tr("Label")});
        int row = 0;
        for (const XmlFile &file : m_inputFiles) {
            xmlpp::NodeSet xml_nodes = file.xml->find("//define-basic-event");
            table->setRowCount(row + xml_nodes.size());
            for (const xmlpp::Node *xml_node : xml_nodes) {
                table->setItem(
                    row, 0, new QTableWidgetItem(
                              QString::fromStdString(scram::GetAttributeValue(
                                  XmlElement(xml_node), "name"))));
                xmlpp::NodeSet label = xml_node->find("./label");
                if (!label.empty()) {
                    table->setItem(
                        row, 1,
                        new QTableWidgetItem(
                            QString::fromStdString(scram::GetContent(
                                XmlElement(label.front())->get_child_text()))));
                }
                ++row;
            }
        }
        table->setWordWrap(false);
        table->resizeColumnsToContents();
        table->setSortingEnabled(true);
        ui->tabWidget->addTab(table, name);
    }
}

MainWindow::XmlFile::XmlFile() : xml(nullptr), parser(new xmlpp::DomParser()) {}

MainWindow::XmlFile::XmlFile(std::string filename) : file(std::move(filename))
{
    parser = scram::ConstructDomParser(file);
    xml = parser->get_document()->get_root_node();
}

void MainWindow::XmlFile::reset(std::string filename)
{
    parser->parse_file(filename);
    file = std::move(filename);
    xml = parser->get_document()->get_root_node();
}

std::vector<std::string> MainWindow::extractInputFiles()
{
    std::vector<std::string> inputFiles;
    for (const XmlFile &input : m_inputFiles)
        inputFiles.push_back(input.file);
    return inputFiles;
}

std::vector<xmlpp::Node *> MainWindow::extractModelXml()
{
    std::vector<xmlpp::Node *> modelXml;
    for (const XmlFile &input : m_inputFiles)
        modelXml.push_back(input.xml);
    return modelXml;
}

void MainWindow::resetTreeWidget()
{
    ui->treeWidget->clear();
    if (m_inputFiles.empty()) {
        ui->treeWidget->setHeaderLabel(tr("Model"));
    } else {
        std::string name = scram::GetAttributeValue(
            XmlElement(m_inputFiles.front().xml), "name");
        ui->treeWidget->setHeaderLabel(
            tr("Model: %1").arg(QString::fromStdString(name)));
    }

    auto *faultTrees = new QTreeWidgetItem({tr("Fault Trees")});
    for (const XmlFile &file : m_inputFiles) {
        for (const xmlpp::Node *ft_node :
             file.xml->find("./define-fault-tree")) {
            faultTrees->addChild(new QTreeWidgetItem({QString::fromStdString(
                scram::GetAttributeValue(XmlElement(ft_node), "name"))}));
        }
    }

    auto *modelData = new QTreeWidgetItem({tr("Model Data")});
    modelData->addChildren({new QTreeWidgetItem({tr("Basic Events")}),
                            new QTreeWidgetItem({tr("House Events")}),
                            new QTreeWidgetItem({tr("Parameters")})});

    ui->treeWidget->addTopLevelItems(
        {faultTrees, new QTreeWidgetItem({tr("CCF Groups")}), modelData});
}

} // namespace gui
} // namespace scram

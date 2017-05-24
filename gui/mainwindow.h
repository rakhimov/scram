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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>
#include <string>
#include <vector>

#include <QMainWindow>
#include <QTreeWidgetItem>

#include <libxml++/libxml++.h>

#include "src/settings.h"

namespace Ui {
class MainWindow;
}

namespace scram {
namespace gui {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setConfig(const std::string &configPath,
                   std::vector<std::string> inputFiles = {});
    void addInputFiles(const std::vector<std::string> &inputFiles);

signals:
    void configChanged();

private slots:
    /**
     * @brief Opens a new project configuration.
     *
     * The current project and input files are reset.
     *
     * @todo Check if the current documents need saving.
     */
    void createNewProject();

    /**
     * @brief Opens an existing project configuration.
     *
     * The current project and input files are reset.
     */
    void openProject();

    /**
     * @brief Saves the project to a file.
     *
     * If the project is new,
     * it does not have a default destination file.
     * The user is required to specify the file upon save.
     */
    void saveProject();

    /**
     * @brief Saves the project to a potentially different file.
     */
    void saveProjectAs();

    /**
     * Processes the element activation in the tree view.
     */
    void showElement(QTreeWidgetItem *item);

private:
    /// Keeps the file location and XML data together.
    struct XmlFile {
        XmlFile();
        explicit XmlFile(std::string filename);
        void reset(std::string filename);
        std::string file; ///< The original location of the document.
        xmlpp::Node *xml; ///< The root element of the document.
        std::unique_ptr<xmlpp::DomParser> parser; ///< The document holder.
    };

    void setupActions(); ///< Setup all the actions with connections.

    /// @returns The current set input file paths.
    std::vector<std::string> extractInputFiles();

    /// @returns The current set of XML model files.
    std::vector<xmlpp::Node *> extractModelXml();

    /**
     * Resets the tree widget with the new model.
     */
    void resetTreeWidget();

    std::unique_ptr<Ui::MainWindow> ui;
    XmlFile m_config; ///< The main project configuration file.
    std::vector<XmlFile> m_inputFiles;  ///< The project model files.
    core::Settings m_settings; ///< The analysis settings.
};

} // namespace gui
} // namespace scram

#endif // MAINWINDOW_H

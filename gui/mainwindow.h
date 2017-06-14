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

#include <QDir>
#include <QMainWindow>
#include <QRegularExpressionValidator>
#include <QTreeWidgetItem>

#include <libxml++/libxml++.h>

#include "src/settings.h"
#include "src/model.h"

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
    void createNewModel();

    /**
     * @brief Opens model files.
     */
    void openFiles(QString directory = QDir::homePath());

    /**
     * @brief Saves the project to a file.
     *
     * If the project is new,
     * it does not have a default destination file.
     * The user is required to specify the file upon save.
     */
    void saveModel();

    /**
     * @brief Saves the project to a potentially different file.
     */
    void saveModelAs();

    /**
     * @brief Exports the current active document/diagram.
     */
    void exportAs();

    /**
     * @brief Prints the current document view.
     */
    void print();

    /**
     * Processes the element activation in the tree view.
     */
    void showElement(QTreeWidgetItem *item);

private:
    void setupActions(); ///< Setup all the actions with connections.

    /**
     * Resets the tree widget with the new model.
     */
    void resetTreeWidget();

    std::unique_ptr<Ui::MainWindow> ui;
    std::vector<std::string> m_inputFiles;  ///< The project model files.
    core::Settings m_settings; ///< The analysis settings.
    std::shared_ptr<mef::Model> m_model; ///< The analysis model.
    QRegularExpressionValidator m_percentValidator;  ///< Zoom percent input.
};

} // namespace gui
} // namespace scram

#endif // MAINWINDOW_H

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

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QLineEdit>
#include <QMainWindow>
#include <QRegularExpressionValidator>
#include <QTreeWidgetItem>
#include <QUndoStack>

#include "src/model.h"
#include "src/risk_analysis.h"
#include "src/settings.h"

#include "eventdialog.h"
#include "model.h"
#include "zoomableview.h"

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
     * Exports the current analysis report.
     */
    void exportReportAs();

private:
    void setupStatusBar(); ///< Setup widgets in the status bar.
    void setupActions(); ///< Setup all the actions with connections.

    void setupZoomableView(ZoomableView *view); ///< Connect to actions.

    /// Connects print actions.
    ///
    /// @tparam T  Any type with print() and printPreview() functions.
    template <class T>
    void setupPrintableView(T *view);

    /// Connects the export actions to a widget.
    ///
    /// @tparam  Any type with exportAs() function.
    template <class T>
    void setupExportableView(T *view);

    /// Connects the search bar to the model of the view.
    ///
    /// @tparam T  Any model type with setFilterRegExp(QString).
    template <class T>
    void setupSearchable(QObject *view, T *model);

    template <class ContainerModel>
    QAbstractItemView *constructElementTable(model::Model *guiModel,
                                             QWidget *parent);

    void editElement(EventDialog *dialog, model::Element *element);
    void editElement(EventDialog *dialog, model::HouseEvent *element);
    void editElement(EventDialog *dialog, model::BasicEvent *element);

    /**
     * Resets the tree view of the new model.
     */
    void resetModelTree();

    /// Activates the model tree elements.
    void activateModelTree(const QModelIndex &index);
    /// Activates the fault tree view.
    void activateFaultTreeDiagram(mef::FaultTree *faultTree);

    /**
     * @brief Resets the report view.
     *
     * @param analysis  The analysis with results.
     *                  nullptr to clear the report widget.
     */
    void resetReportWidget(std::unique_ptr<core::RiskAnalysis> analysis);

    /**
     * Saves the model and sets the model file.
     *
     * @param destination  The destination file to become the main model file.
     */
    void saveToFile(std::string destination);

    /// Override to save the model before closing the application.
    void closeEvent(QCloseEvent *event) override;

    std::unique_ptr<Ui::MainWindow> ui;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QUndoStack *m_undoStack;
    QLineEdit *m_searchBar;

    std::vector<std::string> m_inputFiles;  ///< The project model files.
    core::Settings m_settings; ///< The analysis settings.
    std::shared_ptr<mef::Model> m_model; ///< The analysis model.
    std::unique_ptr<model::Model> m_guiModel;  ///< The GUI Model wrapper.
    QRegularExpressionValidator m_percentValidator;  ///< Zoom percent input.
    QComboBox *m_zoomBox; ///< The main zoom chooser/displayer widget.
    std::unique_ptr<core::RiskAnalysis> m_analysis; ///< Report container.
    std::unordered_map<QTreeWidgetItem *, std::function<void()>>
        m_reportActions; ///< Actions on elements of the report tree widget.
};

} // namespace gui
} // namespace scram

#endif // MAINWINDOW_H

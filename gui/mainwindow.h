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

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <QAbstractItemView>
#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QLineEdit>
#include <QMainWindow>
#include <QSettings>
#include <QTableView>
#include <QTimer>
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

    /// Loads a model and analysis configuration from a file.
    ///
    /// @param[in] configPath  The path to the configuration file.
    /// @param[in] inputFiles  Additional input files for model initialization.
    ///
    /// @returns true if the initialization is successful.
    ///
    /// @post No side effects are left-over
    ///       if the initialization is not successful.
    bool setConfig(const std::string &configPath,
                   std::vector<std::string> inputFiles = {});

    /// Adds a new set of model elements from input files.
    ///
    /// @param[in] inputFiles  Paths to input files.
    ///
    /// @returns true if the addition is successful.
    ///
    /// @post If the addition of any file is not successful,
    ///       the model is left in its original state
    ///       as if this function had not been called (i.e., transactional).
    bool addInputFiles(const std::vector<std::string> &inputFiles);

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
     * @brief Implicitly saves the modified model
     *        only if the destination is available.
     */
    void autoSaveModel();

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

    /// Handles element addition with a dialog.
    void addElement();

private:
    static const int LAYOUT_VERSION = 0; ///< Layout compatibility version.

    void setupStatusBar();   ///< Sets up widgets in the status bar.
    void setupActions();     ///< Sets up all the actions with connections.
    void setupConnections(); ///< Sets up all the remaining connections.
    void loadPreferences();  ///< Loads the persistent application preferences.
    void savePreferences();  ///< Writes the 'unsaved' application preferences.
    void setupStartPage();   ///< Sets up a new start page.

    /// @returns The model name to be used for a title (e.g., main window).
    QString getModelNameForTitle();

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

    /// Sets up remove action activation upon selection.
    /// Only selection of top indices activate the removal.
    ///
    /// @tparam T  The type of the objects to remove.
    ///
    /// @pre Selections are single row.
    /// @pre The model is proxy.
    template <class T>
    void setupRemovable(QAbstractItemView *view);

    /// @tparam T  The MEF type.
    ///
    /// @returns The fault tree container of the element.
    template <class T>
    mef::FaultTree *getFaultTree(T *) { return nullptr; }

    /// @tparam T  The model proxy type.
    template <class T>
    void removeEvent(T *event, mef::FaultTree *faultTree);

    /// @tparam ContainerModel  The container model type.
    /// @tparam Ts  The argument types for the container model.
    ///
    /// @param[in,out] parent  The parent to own the table view.
    /// @param[in] modelArgs  The arguments for container model constructor.
    ///
    /// @returns Table view ready for inclusion to tabs.
    template <class ContainerModel, typename... Ts>
    QTableView *constructTableView(QWidget *parent, Ts&&... modelArgs);

    template <class ContainerModel>
    QAbstractItemView *constructElementTable(model::Model *guiModel,
                                             QWidget *parent);

    /// @returns A new element constructed with the dialog data.
    template <class T>
    std::unique_ptr<T> extract(const EventDialog &dialog);

    mef::FaultTree *getFaultTree(const EventDialog &dialog);

    template <class T>
    void editElement(EventDialog *dialog, model::Element *element);
    void editElement(EventDialog *dialog, model::HouseEvent *element);
    void editElement(EventDialog *dialog, model::BasicEvent *element);
    void editElement(EventDialog *dialog, model::Gate *element);

    /**
     * Resets the tree view of the new model.
     */
    void resetModelTree();

    /// Activates the model tree elements.
    void activateModelTree(const QModelIndex &index);
    /// Activates the fault tree view.
    void activateFaultTreeDiagram(mef::FaultTree *faultTree);
    /// Activates the report tree elements.
    void activateReportTree(const QModelIndex &index);

    /**
     * @brief Resets the report view.
     *
     * @param analysis  The analysis with results.
     *                  nullptr to clear the report widget.
     */
    void resetReportTree(std::unique_ptr<core::RiskAnalysis> analysis);

    /**
     * Saves the model and sets the model file.
     *
     * @param destination  The destination file to become the main model file.
     */
    void saveToFile(std::string destination);

    /**
     * Updates the recent file tracking.
     *
     * @param filePaths  The list to append to the recent file list.
     *                   An empty list to clear the recent file list.
     *
     * @pre The list contains valid absolute input file paths.
     *
     * @note This does not store the result path list into persistent settings.
     */
    void updateRecentFiles(QStringList filePaths);

    /// Override to save the model before closing the application.
    void closeEvent(QCloseEvent *event) override;

    /// Safely closes the tab with the given index in the widget.
    ///
    /// @param[in] index  The index of the tab to be removed.
    ///
    /// @post Show/hide order is respected for safe delete.
    /// @post The closed tab is deleted.
    void closeTab(int index);

    /// Runs the analysis with the current model.
    void runAnalysis();

    std::unique_ptr<Ui::MainWindow> ui;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QUndoStack *m_undoStack;
    QComboBox *m_zoomBox; ///< The main zoom chooser/displayer widget.
    QLineEdit *m_searchBar;
    QTimer *m_autoSaveTimer;
    QSettings m_preferences;
    std::array<QAction *, 5> m_recentFileActions;

    std::vector<std::string> m_inputFiles;  ///< The project model files.
    core::Settings m_settings; ///< The analysis settings.
    std::shared_ptr<mef::Model> m_model; ///< The analysis model.
    std::unique_ptr<model::Model> m_guiModel;  ///< The GUI Model wrapper.
    std::unique_ptr<core::RiskAnalysis> m_analysis; ///< Report container.
};

} // namespace gui
} // namespace scram

#endif // MAINWINDOW_H

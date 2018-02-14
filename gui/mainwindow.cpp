/*
 * Copyright (C) 2015-2018 Olzhas Rakhimov
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

/// @file

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_namedialog.h"
#include "ui_startpage.h"

#include <algorithm>
#include <sstream>
#include <type_traits>

#include <QApplication>
#include <QCoreApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QPrinter>
#include <QProgressDialog>
#include <QSvgGenerator>
#include <QtConcurrent>
#include <QtOpenGL>

#include <boost/exception/get_error_info.hpp>
#include <boost/filesystem.hpp>

#include "src/config.h"
#include "src/env.h"
#include "src/error.h"
#include "src/expression/constant.h"
#include "src/expression/exponential.h"
#include "src/ext/algorithm.h"
#include "src/ext/find_iterator.h"
#include "src/ext/variant.h"
#include "src/initializer.h"
#include "src/reporter.h"
#include "src/serialization.h"
#include "src/xml.h"

#include "diagram.h"
#include "elementcontainermodel.h"
#include "eventdialog.h"
#include "guiassert.h"
#include "importancetablemodel.h"
#include "modeltree.h"
#include "overload.h"
#include "preferencesdialog.h"
#include "printable.h"
#include "producttablemodel.h"
#include "reporttree.h"
#include "settingsdialog.h"
#include "validator.h"

namespace scram::gui {

/// The dialog to set the model name.
class NameDialog : public QDialog, public Ui::NameDialog
{
public:
    /// @param[in,out] parent  The owner widget.
    explicit NameDialog(QWidget *parent) : QDialog(parent)
    {
        setupUi(this);
        nameLine->setValidator(Validator::name());
    }
};

/// The initial start tab.
class StartPage : public QWidget, public Ui::StartPage
{
public:
    /// @param[in,out] parent  The owner widget.
    explicit StartPage(QWidget *parent = nullptr) : QWidget(parent)
    {
        setupUi(this);
    }
};

/// The dialog to block user input while waiting for a long-running process.
class WaitDialog : public QProgressDialog
{
public:
    /// @param[in,out] parent  The owner widget.
    explicit WaitDialog(QWidget *parent) : QProgressDialog(parent)
    {
        setFixedSize(size());
        setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint
                       | Qt::FramelessWindowHint);
        setCancelButton(nullptr);
        setRange(0, 0);
        setMinimumDuration(0);
    }

private:
    /// Intercepts disruptive keyboard presses.
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Escape)
            return event->accept();
        QProgressDialog::keyPressEvent(event);
    }
};

/// The default view for graphics views (e.g., fault tree diagram).
class DiagramView : public ZoomableView, public Printable
{
public:
    using ZoomableView::ZoomableView;

    /// Exports the image of the diagram.
    void exportAs()
    {
        QString filename = QFileDialog::getSaveFileName(
            this, QObject::tr("Export As"), QDir::homePath(),
            QObject::tr("SVG files (*.svg);;All files (*.*)"));
        QSize sceneSize = scene()->sceneRect().size().toSize();

        QSvgGenerator generator;
        generator.setFileName(filename);
        generator.setSize(sceneSize);
        generator.setViewBox(
            QRect(0, 0, sceneSize.width(), sceneSize.height()));
        generator.setTitle(filename);
        QPainter painter;
        painter.begin(&generator);
        scene()->render(&painter);
        painter.end();
    }

private:
    void doPrint(QPrinter *printer) override
    {
        QPainter painter(printer);
        painter.setRenderHint(QPainter::Antialiasing);
        scene()->render(&painter);
    }
};

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      m_undoStack(new QUndoStack(this)),
      m_zoomBox(new QComboBox), // Will be owned by the tool bar later.
      m_autoSaveTimer(new QTimer(this))
{
    ui->setupUi(this);

    m_zoomBox->setEditable(true);
    m_zoomBox->setEnabled(false);
    m_zoomBox->setInsertPolicy(QComboBox::NoInsert);
    m_zoomBox->setValidator(Validator::percent());
    for (QAction *action : ui->menuZoom->actions()) {
        m_zoomBox->addItem(action->text());
        connect(action, &QAction::triggered, m_zoomBox,
                [action, this] { m_zoomBox->setCurrentText(action->text()); });
    }
    m_zoomBox->setCurrentText(QStringLiteral("100%"));
    ui->zoomToolBar->addWidget(m_zoomBox); // Transfer the ownership.

    setupStatusBar();
    setupActions();
    setupConnections();
    loadPreferences();
    setupStartPage();
}

MainWindow::~MainWindow() = default;

namespace { // Error message dialog for SCRAM exceptions.

void displayError(const scram::IOError &err, const QString &text,
                  QWidget *parent = nullptr)
{
    QMessageBox message(QMessageBox::Critical, QObject::tr("IO Error"), text,
                        QMessageBox::Ok, parent);

    const std::string *filename =
        boost::get_error_info<boost::errinfo_file_name>(err);
    GUI_ASSERT(filename, );
    message.setInformativeText(
        QObject::tr("File: %1").arg(QString::fromStdString(*filename)));

    std::stringstream detail;
    if (const std::string *mode =
            boost::get_error_info<boost::errinfo_file_open_mode>(err)) {
        detail << "Open mode: " << *mode << "\n";
    }
    if (const int *errnum = boost::get_error_info<boost::errinfo_errno>(err)) {
        detail << "Error code: " << *errnum << "\n";
        detail << "Error string: " << std::strerror(*errnum) << "\n";
    }
    detail << "\n" << err.what() << std::endl;
    message.setDetailedText(QString::fromStdString(detail.str()));

    message.exec();
}

template <class Tag>
void displayErrorInfo(QString tag_string, const scram::Error &err,
                      QString *info)
{
    if (const auto *value = boost::get_error_info<Tag>(err)) {
        std::stringstream ss;
        ss << *value;
        auto value_string = QString::fromStdString(ss.str());

        //: Error information tag and its value.
        info->append(QObject::tr("%1: %2").arg(tag_string, value_string));
        info->append(QStringLiteral("\n"));
    }
}

void displayError(const scram::Error &err, const QString &title,
                  const QString &text, QWidget *parent = nullptr)
{
    QMessageBox message(QMessageBox::Critical, title, text, QMessageBox::Ok,
                        parent);
    QString info;
    auto newLine = [&info] { info.append(QStringLiteral("\n")); };

    displayErrorInfo<errinfo_value>(QObject::tr("Value"), err, &info);

    if (const std::string *filename =
            boost::get_error_info<boost::errinfo_file_name>(err)) {
        info.append(
            QObject::tr("File: %1").arg(QString::fromStdString(*filename)));
        newLine();
        if (const int *line =
                boost::get_error_info<boost::errinfo_at_line>(err)) {
            info.append(QObject::tr("Line: %1").arg(*line));
            newLine();
        }
    }

    displayErrorInfo<mef::errinfo_connective>(QObject::tr("MEF Connective"),
                                              err, &info);
    displayErrorInfo<mef::errinfo_reference>(QObject::tr("MEF reference"), err,
                                             &info);
    displayErrorInfo<mef::errinfo_base_path>(QObject::tr("MEF base path"), err,
                                             &info);
    displayErrorInfo<mef::errinfo_element_id>(QObject::tr("MEF Element ID"),
                                              err, &info);
    displayErrorInfo<mef::errinfo_element_type>(QObject::tr("MEF Element type"),
                                                err, &info);
    displayErrorInfo<mef::errinfo_container_id>(QObject::tr("MEF Container"),
                                                err, &info);
    displayErrorInfo<mef::errinfo_container_type>(
        QObject::tr("MEF Container type"), err, &info);
    displayErrorInfo<mef::errinfo_attribute>(QObject::tr("MEF Attribute"), err,
                                             &info);
    displayErrorInfo<mef::errinfo_cycle>(QObject::tr("Cycle"), err, &info);

    if (const std::string *xml_element =
            boost::get_error_info<xml::errinfo_element>(err)) {
        info.append(QObject::tr("XML element: %1")
                        .arg(QString::fromStdString(*xml_element)));
        newLine();
    }
    if (const std::string *xml_attribute =
            boost::get_error_info<xml::errinfo_attribute>(err)) {
        info.append(QObject::tr("XML attribute: %1")
                        .arg(QString::fromStdString(*xml_attribute)));
        newLine();
    }
    message.setInformativeText(info);

    std::stringstream detail;
    detail << boost::core::demangled_name(typeid(err)) << "\n\n";
    detail << err.what() << std::endl;
    message.setDetailedText(QString::fromStdString(detail.str()));

    message.exec();
}

} // namespace

bool MainWindow::setConfig(const std::string &configPath,
                           std::vector<std::string> inputFiles)
{
    try {
        Config config(configPath);
        inputFiles.insert(inputFiles.begin(), config.input_files().begin(),
                          config.input_files().end());
        mef::Initializer(inputFiles, config.settings());
        if (!addInputFiles(inputFiles))
            return false;
        m_settings = config.settings();
    } catch (const scram::IOError &err) {
        displayError(err, tr("Configuration file error"), this);
        return false;
    } catch (const scram::xml::Error &err) {
        displayError(err, tr("XML Validity Error"),
                     tr("Invalid configuration file"), this);
        return false;
    } catch (const scram::SettingsError &err) {
        displayError(err, tr("Configuration Error"),
                     tr("Invalid configurations"), this);
        return false;
    }
    return true;
}

bool MainWindow::addInputFiles(const std::vector<std::string> &inputFiles)
{
    static xml::Validator validator(env::install_dir()
                                    + "/share/scram/gui.rng");

    if (inputFiles.empty())
        return true;
    if (isWindowModified() && !saveModel())
        return false;

    try {
        std::vector<std::string> allInput = m_inputFiles;
        allInput.insert(allInput.end(), inputFiles.begin(), inputFiles.end());
        std::unique_ptr<mef::Model> newModel = [this, &allInput] {
            return mef::Initializer(allInput, m_settings,
                                    /*allow_extern=*/false, &validator)
                .model();
        }();

        for (const mef::FaultTree &faultTree : newModel->fault_trees()) {
            if (faultTree.top_events().size() != 1) {
                QMessageBox::critical(
                    this, tr("Initialization Error"),
                    //: Single top/root event fault tree are expected by GUI.
                    tr("Fault tree '%1' must have a single top-gate.")
                        .arg(QString::fromStdString(faultTree.name())));
                return false;
            }
        }

        m_model = std::move(newModel);
        m_inputFiles = std::move(allInput);
    } catch (const scram::IOError &err) {
        displayError(err, tr("Input file error"), this);
        return false;
    } catch (const scram::xml::Error &err) {
        displayError(err, tr("XML Validity Error"), tr("Invalid input file"),
                     this);
        return false;
    } catch (const scram::mef::ValidityError &err) {
        displayError(err,
                     //: The error upon initialization from a file.
                     tr("Initialization Error"), tr("Invalid input model"),
                     this);
        return false;
    }

    emit configChanged();
    return true;
}

void MainWindow::setupStatusBar()
{
    m_searchBar = new QLineEdit;
    m_searchBar->setHidden(true);
    m_searchBar->setFrame(false);
    m_searchBar->setMaximumHeight(m_searchBar->fontMetrics().height());
    m_searchBar->setSizePolicy(QSizePolicy::MinimumExpanding,
                               QSizePolicy::Fixed);
    //: The search bar.
    m_searchBar->setPlaceholderText(tr("Find/Filter (Perl Regex)"));
    ui->statusBar->addPermanentWidget(m_searchBar);
}

void MainWindow::setupActions()
{
    connect(ui->actionAboutQt, &QAction::triggered, qApp,
            &QApplication::aboutQt);
    connect(ui->actionAboutScram, &QAction::triggered, this, [this] {
        QString legal = QStringLiteral(
            "This program is distributed in the hope that it will be useful, "
            "but WITHOUT ANY WARRANTY; without even the implied warranty of "
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
            "GNU General Public License for more details.");
        QMessageBox::about(
            this, tr("About SCRAM"),
            tr("<h1>SCRAM %1</h1>"
               "The GUI front-end for SCRAM,<br/>"
               "a command-line risk analysis multi-tool.<br/><br/>"
               "License: GPLv3+<br/>"
               "Homepage: <a href=\"%2\">%2</a><br/>"
               "Technical Support: <a href=\"%3\">%3</a><br/>"
               "Bug Tracker: <a href=\"%4\">%4</a><br/><br/>%5")
                .arg(QCoreApplication::applicationVersion(),
                     QStringLiteral("https://scram-pra.org"),
                     QStringLiteral("scram-users@googlegroups.com"),
                     QStringLiteral("https://github.com/rakhimov/scram/issues"),
                     legal));
    });

    // File menu actions.
    ui->actionExit->setShortcut(QKeySequence::Quit);

    ui->actionNewModel->setShortcut(QKeySequence::New);
    connect(ui->actionNewModel, &QAction::triggered, this,
            &MainWindow::createNewModel);

    ui->actionOpenFiles->setShortcut(QKeySequence::Open);
    connect(ui->actionOpenFiles, &QAction::triggered, this,
            [this] { openFiles(); });

    ui->actionSave->setShortcut(QKeySequence::Save);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveModel);

    ui->actionSaveAs->setShortcut(QKeySequence::SaveAs);
    connect(ui->actionSaveAs, &QAction::triggered, this,
            &MainWindow::saveModelAs);

    ui->actionPrint->setShortcut(QKeySequence::Print);

    connect(ui->actionExportReportAs, &QAction::triggered, this,
            &MainWindow::exportReportAs);

    QAction *menuRecentFilesStart = ui->menuRecentFiles->actions().front();
    for (QAction *&fileAction : m_recentFileActions) {
        fileAction = new QAction(this);
        fileAction->setVisible(false);
        ui->menuRecentFiles->insertAction(menuRecentFilesStart, fileAction);
        connect(fileAction, &QAction::triggered, this, [this, fileAction] {
            auto filePath = fileAction->text();
            GUI_ASSERT(!filePath.isEmpty(), );
            if (addInputFiles({filePath.toStdString()}))
                updateRecentFiles({filePath});
        });
    }
    connect(ui->actionClearList, &QAction::triggered, this,
            [this] { updateRecentFiles({}); });

    // View menu actions.
    ui->actionZoomIn->setShortcut(QKeySequence::ZoomIn);
    ui->actionZoomOut->setShortcut(QKeySequence::ZoomOut);

    // Edit menu actions.
    ui->actionRemoveElement->setShortcut(QKeySequence::Delete);
    connect(ui->actionAddElement, &QAction::triggered, this,
            &MainWindow::addElement);
    connect(ui->actionRenameModel, &QAction::triggered, this, [this] {
        NameDialog nameDialog(this);
        if (!m_model->HasDefaultName())
            nameDialog.nameLine->setText(m_guiModel->id());
        if (nameDialog.exec() == QDialog::Accepted) {
            QString name = nameDialog.nameLine->text();
            if (name != QString::fromStdString(m_model->GetOptionalName())) {
                m_undoStack->push(new model::Model::SetName(std::move(name),
                                                            m_guiModel.get()));
            }
        }
    });
    connect(ui->actionPreferences, &QAction::triggered, this, [this] {
        PreferencesDialog dialog(&m_preferences, m_undoStack, m_autoSaveTimer,
                                 this);
        dialog.exec();
    });

    // Undo/Redo actions
    m_undoAction = m_undoStack->createUndoAction(this, tr("Undo:"));
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));

    m_redoAction = m_undoStack->createRedoAction(this, tr("Redo:"));
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-redo")));

    ui->menuEdit->insertAction(ui->menuEdit->actions().front(), m_redoAction);
    ui->menuEdit->insertAction(m_redoAction, m_undoAction);
    ui->editToolBar->insertAction(ui->editToolBar->actions().front(),
                                  m_redoAction);
    ui->editToolBar->insertAction(m_redoAction, m_undoAction);
    connect(m_undoStack, &QUndoStack::cleanChanged, ui->actionSave,
            &QAction::setDisabled);
    connect(m_undoStack, &QUndoStack::cleanChanged,
            [this](bool clean) { setWindowModified(!clean); });

    // Search/filter bar shortcuts.
    auto *searchAction = new QAction(this);
    searchAction->setShortcuts({QKeySequence::Find, Qt::Key_Slash});
    m_searchBar->addAction(searchAction);
    connect(searchAction, &QAction::triggered, [this] {
        if (m_searchBar->isHidden())
            return;
        m_searchBar->setFocus();
        m_searchBar->selectAll();
    });

    // Providing shortcuts for the tab widget manipulations.
    auto *closeCurrentTab = new QAction(this);
    auto *nextTab = new QAction(this);
    auto *prevTab = new QAction(this);

    closeCurrentTab->setShortcut(QKeySequence::Close);
    nextTab->setShortcut(QKeySequence::NextChild);
    // QTBUG-15746: QKeySequence::PreviousChild does not work.
    prevTab->setShortcut(Qt::CTRL | Qt::Key_Backtab);

    ui->tabWidget->addAction(closeCurrentTab);
    ui->tabWidget->addAction(nextTab);
    ui->tabWidget->addAction(prevTab);

    auto switchTab = [this](bool toNext) {
        int numTabs = ui->tabWidget->count();
        if (!numTabs)
            return;
        int currentIndex = ui->tabWidget->currentIndex();
        int nextIndex = [currentIndex, numTabs, toNext] {
            int ret = currentIndex + (toNext ? 1 : -1);
            if (ret < 0)
                return numTabs - 1;
            if (ret >= numTabs)
                return 0;
            return ret;
        }();
        ui->tabWidget->setCurrentIndex(nextIndex);
    };

    connect(closeCurrentTab, &QAction::triggered, ui->tabWidget,
            [this] { MainWindow::closeTab(ui->tabWidget->currentIndex()); });
    connect(nextTab, &QAction::triggered, ui->tabWidget,
            [switchTab] { switchTab(true); });
    connect(prevTab, &QAction::triggered, ui->tabWidget,
            [switchTab] { switchTab(false); });
}

void MainWindow::setupConnections()
{
    connect(ui->modelTree, &QTreeView::activated, this,
            &MainWindow::activateModelTree);
    connect(ui->reportTree, &QTreeView::activated, this,
            &MainWindow::activateReportTree);
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this,
            &MainWindow::closeTab);

    connect(ui->actionSettings, &QAction::triggered, this, [this] {
        SettingsDialog dialog(m_settings, this);
        if (dialog.exec() == QDialog::Accepted)
            m_settings = dialog.settings();
    });
    connect(ui->actionRun, &QAction::triggered, this, &MainWindow::runAnalysis);

    connect(this, &MainWindow::configChanged, [this] {
        m_undoStack->clear();
        setWindowTitle(QStringLiteral("%1[*]").arg(getModelNameForTitle()));
        ui->actionSaveAs->setEnabled(true);
        ui->actionAddElement->setEnabled(true);
        ui->actionRenameModel->setEnabled(true);
        ui->actionRun->setEnabled(true);
        resetModelTree();
        resetReportTree(nullptr);
    });
    connect(m_undoStack, &QUndoStack::indexChanged, ui->reportTree, [this] {
        if (m_analysis)
            resetReportTree(nullptr);
    });
    connect(m_autoSaveTimer, &QTimer::timeout, this,
            &MainWindow::autoSaveModel);
}

void MainWindow::loadPreferences()
{
    m_preferences.beginGroup(QStringLiteral("MainWindow"));
    restoreGeometry(
        m_preferences.value(QStringLiteral("geometry")).toByteArray());
    restoreState(m_preferences.value(QStringLiteral("state")).toByteArray(),
                 LAYOUT_VERSION);
    m_preferences.endGroup();

    m_undoStack->setUndoLimit(
        m_preferences.value(QStringLiteral("undoLimit"), 0).toInt());

    GUI_ASSERT(m_autoSaveTimer->isActive() == false, );
    int interval =
        m_preferences.value(QStringLiteral("autoSave"), 300000).toInt();
    if (interval)
        m_autoSaveTimer->start(interval);

    updateRecentFiles(
        m_preferences.value(QStringLiteral("recentFiles")).toStringList());
}

void MainWindow::savePreferences()
{
    m_preferences.beginGroup(QStringLiteral("MainWindow"));
    m_preferences.setValue(QStringLiteral("geometry"), saveGeometry());
    m_preferences.setValue(QStringLiteral("state"), saveState(LAYOUT_VERSION));
    m_preferences.endGroup();

    QStringList fileList;
    for (QAction *fileAction : m_recentFileActions) {
        if (!fileAction->isVisible())
            break;
        fileList.push_back(fileAction->text());
    }
    m_preferences.setValue(QStringLiteral("recentFiles"), fileList);
}

void MainWindow::setupStartPage()
{
    auto *startPage = new StartPage(this);
    QString examplesDir =
        QString::fromStdString(env::install_dir() + "/share/scram/input");
    startPage->exampleModelsButton->setEnabled(QDir(examplesDir).exists());
    connect(startPage->newModelButton, &QAbstractButton::clicked,
            ui->actionNewModel, &QAction::trigger);
    connect(startPage->openModelButton, &QAbstractButton::clicked,
            ui->actionOpenFiles, &QAction::trigger);
    connect(startPage->exampleModelsButton, &QAbstractButton::clicked, this,
            [this, examplesDir] { openFiles(examplesDir); });
    ui->tabWidget->addTab(startPage, startPage->windowIcon(),
                          startPage->windowTitle());

    startPage->recentFilesBox->setVisible(
        m_recentFileActions.front()->isVisible());
    for (QAction *fileAction : m_recentFileActions) {
        if (!fileAction->isVisible())
            break;
        auto *button =
            new QCommandLinkButton(QFileInfo(fileAction->text()).fileName());
        button->setToolTip(fileAction->text());
        startPage->recentFilesBox->layout()->addWidget(button);
        connect(button, &QAbstractButton::clicked, fileAction,
                &QAction::trigger);
    }
}

QString MainWindow::getModelNameForTitle()
{
    return m_model->HasDefaultName() ? tr("Unnamed Model")
                                     : QString::fromStdString(m_model->name());
}

void MainWindow::createNewModel()
{
    if (isWindowModified()) {
        QMessageBox::StandardButton answer = QMessageBox::question(
            this, tr("Save Model?"),
            tr("Save changes to model '%1' before closing?")
                .arg(getModelNameForTitle()),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);

        if (answer == QMessageBox::Cancel)
            return;
        if (answer == QMessageBox::Save && !saveModel())
            return;
    }

    m_inputFiles.clear();
    m_model = std::make_unique<mef::Model>();

    emit configChanged();
}

void MainWindow::openFiles(QString directory)
{
    QStringList filenames = QFileDialog::getOpenFileNames(
        this, tr("Open Model Files"), directory,
        QStringLiteral("%1 (*.mef *.opsa *.opsa-mef *.xml);;%2 (*.*)")
            .arg(tr("Model Exchange Format"), tr("All files")));
    if (filenames.empty())
        return;
    std::vector<std::string> inputFiles;
    for (const auto &filename : filenames)
        inputFiles.push_back(filename.toStdString());
    if (addInputFiles(inputFiles))
        updateRecentFiles(filenames);
}

void MainWindow::autoSaveModel()
{
    if (!isWindowModified() || m_inputFiles.empty() || m_inputFiles.size() > 1)
        return;
    saveToFile(m_inputFiles.front());
}

bool MainWindow::saveModel()
{
    if (m_inputFiles.empty() || m_inputFiles.size() > 1)
        return saveModelAs();
    return saveToFile(m_inputFiles.front());
}

bool MainWindow::saveModelAs()
{
    QString filename = QFileDialog::getSaveFileName(
        this, tr("Save Model As"), QDir::homePath(),
        QStringLiteral("%1 (*.mef *.opsa *.opsa-mef *.xml);;%2 (*.*)")
            .arg(tr("Model Exchange Format"), tr("All files")));
    if (filename.isNull())
        return false;
    return saveToFile(filename.toStdString());
}

bool MainWindow::saveToFile(std::string destination)
{
    GUI_ASSERT(!destination.empty(), false);
    GUI_ASSERT(m_model, false);

    namespace fs = boost::filesystem;
    fs::path temp_file = destination + "." + fs::unique_path().string();

    try {
        mef::Serialize(*m_model, temp_file.string());
        try {
            fs::rename(temp_file, destination);
        } catch (const fs::filesystem_error &err) {
            SCRAM_THROW(IOError(err.what()))
                << boost::errinfo_file_name(destination)
                << boost::errinfo_errno(err.code().value());
        }
    } catch (const IOError &err) {
        displayError(err, tr("Save error", "error on saving to file"), this);
        return false;
    }
    m_undoStack->setClean();
    m_inputFiles.clear();
    m_inputFiles.push_back(std::move(destination));
    return true;
}

void MainWindow::updateRecentFiles(QStringList filePaths)
{
    ui->menuRecentFiles->setEnabled(!filePaths.empty());
    if (filePaths.empty()) {
        for (QAction *fileAction : m_recentFileActions)
            fileAction->setVisible(false);
        return;
    }

    int remainingCapacity = m_recentFileActions.size() - filePaths.size();
    for (QAction *fileAction : m_recentFileActions) {
        if (remainingCapacity <= 0)
            break;
        if (!fileAction->isVisible())
            break;
        if (filePaths.contains(fileAction->text()))
            continue;
        filePaths.push_back(fileAction->text());
        --remainingCapacity;
    }
    auto it = m_recentFileActions.begin();
    const auto &constFilePaths = filePaths; // Detach prevention w/ for-each.
    for (const QString &filePath : constFilePaths) {
        if (it == m_recentFileActions.end())
            break;
        (*it)->setText(filePath);
        (*it)->setVisible(true);
        ++it;
    }
    for (; it != m_recentFileActions.end(); ++it)
        (*it)->setVisible(false);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    savePreferences();

    if (!isWindowModified())
        return event->accept();

    QMessageBox::StandardButton answer = QMessageBox::question(
        this, tr("Save Model?"),
        tr("Save changes to model '%1' before closing?")
            .arg(getModelNameForTitle()),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (answer == QMessageBox::Cancel)
        return event->ignore();
    if (answer == QMessageBox::Discard)
        return event->accept();

    return saveModel() ? event->accept() : event->ignore();
}

void MainWindow::closeTab(int index)
{
    if (index < 0)
        return;
    // Ensure show/hide order.
    if (index == ui->tabWidget->currentIndex()) {
        int num_tabs = ui->tabWidget->count();
        if (num_tabs > 1) {
            ui->tabWidget->setCurrentIndex(index == (num_tabs - 1) ? index - 1
                                                                   : index + 1);
        }
    }
    auto *widget = ui->tabWidget->widget(index);
    ui->tabWidget->removeTab(index);
    delete widget;
}

void MainWindow::runAnalysis()
{
    GUI_ASSERT(m_model, );
    if (m_settings.probability_analysis()
        && ext::any_of(m_model->basic_events(),
                       [](const mef::BasicEvent &basicEvent) {
                           return !basicEvent.HasExpression();
                       })) {
        QMessageBox::critical(this, tr("Validation Error"),
                              tr("Not all basic events have expressions "
                                 "for probability analysis."));
        return;
    }
    WaitDialog progress(this);
    //: This is a message shown during the analysis run.
    progress.setLabelText(tr("Running analysis..."));
    auto analysis =
        std::make_unique<core::RiskAnalysis>(m_model.get(), m_settings);
    QFutureWatcher<void> futureWatcher;
    connect(&futureWatcher, SIGNAL(finished()), &progress, SLOT(reset()));
    futureWatcher.setFuture(
        QtConcurrent::run([&analysis] { analysis->Analyze(); }));
    progress.exec();
    futureWatcher.waitForFinished();
    resetReportTree(std::move(analysis));
}

void MainWindow::exportReportAs()
{
    GUI_ASSERT(m_analysis, );
    QString filename = QFileDialog::getSaveFileName(
        this, tr("Export Report As"), QDir::homePath(),
        QStringLiteral("%1 (*.mef *.opsa *.opsa-mef *.xml);;%2 (*.*)")
            .arg(tr("Model Exchange Format"), tr("All files")));
    if (filename.isNull())
        return;
    try {
        Reporter().Report(*m_analysis, filename.toStdString());
    } catch (const IOError &err) {
        displayError(err, tr("Reporting error"), this);
    }
}

void MainWindow::setupZoomableView(ZoomableView *view)
{
    struct ZoomFilter : public QObject
    {
        ZoomFilter(ZoomableView *zoomable, MainWindow *window)
            : QObject(zoomable), m_window(window), m_zoomable(zoomable)
        {
        }
        bool eventFilter(QObject *object, QEvent *event) override
        {
            auto setEnabled = [this](bool state) {
                m_window->m_zoomBox->setEnabled(state);
                m_window->ui->actionZoomIn->setEnabled(state);
                m_window->ui->actionZoomIn->setEnabled(state);
                m_window->ui->actionZoomOut->setEnabled(state);
                m_window->ui->actionBestFit->setEnabled(state);
                m_window->ui->menuZoom->setEnabled(state);
            };

            if (event->type() == QEvent::Show) {
                setEnabled(true);
                m_window->m_zoomBox->setCurrentText(
                    QStringLiteral("%1%").arg(m_zoomable->getZoom()));

                connect(m_zoomable, &ZoomableView::zoomChanged,
                        m_window->m_zoomBox, [this](int level) {
                            m_window->m_zoomBox->setCurrentText(
                                QStringLiteral("%1%").arg(level));
                        });
                connect(m_window->m_zoomBox, &QComboBox::currentTextChanged,
                        m_zoomable, [this](QString text) {
                            // Check if the user editing the box.
                            if (m_window->m_zoomBox->lineEdit()->isModified())
                                return;
                            text.remove(QLatin1Char('%'));
                            m_zoomable->setZoom(text.toInt());
                        });
                connect(m_window->m_zoomBox->lineEdit(),
                        &QLineEdit::editingFinished, m_zoomable, [this] {
                            QString text = m_window->m_zoomBox->currentText();
                            text.remove(QLatin1Char('%'));
                            m_zoomable->setZoom(text.toInt());
                        });
                connect(m_window->ui->actionZoomIn, &QAction::triggered,
                        m_zoomable, [this] { m_zoomable->zoomIn(5); });
                connect(m_window->ui->actionZoomOut, &QAction::triggered,
                        m_zoomable, [this] { m_zoomable->zoomOut(5); });
                connect(m_window->ui->actionBestFit, &QAction::triggered,
                        m_zoomable, &ZoomableView::zoomBestFit);
            } else if (event->type() == QEvent::Hide) {
                setEnabled(false);
                disconnect(m_window->m_zoomBox->lineEdit(), 0, m_zoomable, 0);
                disconnect(m_zoomable, 0, m_window->m_zoomBox, 0);
                disconnect(m_window->m_zoomBox, 0, m_zoomable, 0);
                disconnect(m_window->ui->actionZoomIn, 0, m_zoomable, 0);
                disconnect(m_window->ui->actionZoomOut, 0, m_zoomable, 0);
                disconnect(m_window->ui->actionBestFit, 0, m_zoomable, 0);
            }
            return QObject::eventFilter(object, event);
        }
        MainWindow *m_window;
        ZoomableView *m_zoomable;
    };
    view->installEventFilter(new ZoomFilter(view, this));
}

template <class T>
void MainWindow::setupPrintableView(T *view)
{
    static_assert(std::is_base_of_v<QObject, T>, "Missing QObject");
    struct PrintFilter : public QObject
    {
        PrintFilter(T *printable, MainWindow *window)
            : QObject(printable), m_window(window), m_printable(printable)
        {
        }
        bool eventFilter(QObject *object, QEvent *event) override
        {
            auto setEnabled = [this](bool state) {
                m_window->ui->actionPrint->setEnabled(state);
                m_window->ui->actionPrintPreview->setEnabled(state);
            };
            if (event->type() == QEvent::Show) {
                setEnabled(true);
                connect(m_window->ui->actionPrint, &QAction::triggered,
                        m_printable, [this] { m_printable->print(); });
                connect(m_window->ui->actionPrintPreview, &QAction::triggered,
                        m_printable, [this] { m_printable->printPreview(); });
            } else if (event->type() == QEvent::Hide) {
                setEnabled(false);
                disconnect(m_window->ui->actionPrint, 0, m_printable, 0);
                disconnect(m_window->ui->actionPrintPreview, 0, m_printable, 0);
            }
            return QObject::eventFilter(object, event);
        }
        MainWindow *m_window;
        T *m_printable;
    };

    view->installEventFilter(new PrintFilter(view, this));
}

template <class T>
void MainWindow::setupExportableView(T *view)
{
    struct ExportFilter : public QObject
    {
        ExportFilter(T *exportable, MainWindow *window)
            : QObject(exportable), m_window(window), m_exportable(exportable)
        {
        }

        bool eventFilter(QObject *object, QEvent *event) override
        {
            if (event->type() == QEvent::Show) {
                m_window->ui->actionExportAs->setEnabled(true);
                connect(m_window->ui->actionExportAs, &QAction::triggered,
                        m_exportable, [this] { m_exportable->exportAs(); });
            } else if (event->type() == QEvent::Hide) {
                m_window->ui->actionExportAs->setEnabled(false);
                disconnect(m_window->ui->actionExportAs, 0, m_exportable, 0);
            }

            return QObject::eventFilter(object, event);
        }

        MainWindow *m_window;
        T *m_exportable;
    };
    view->installEventFilter(new ExportFilter(view, this));
}

template <class T>
void MainWindow::setupSearchable(QObject *view, T *model)
{
    struct SearchFilter : public QObject
    {
        SearchFilter(T *searchable, MainWindow *window)
            : QObject(searchable), m_window(window), m_searchable(searchable)
        {
        }

        bool eventFilter(QObject *object, QEvent *event) override
        {
            if (event->type() == QEvent::Show) {
                m_window->m_searchBar->setHidden(false);
                m_window->m_searchBar->setText(
                    m_searchable->filterRegExp().pattern());
                connect(m_window->m_searchBar, &QLineEdit::editingFinished,
                        object, [this] {
                            m_searchable->setFilterRegExp(
                                m_window->m_searchBar->text());
                        });
            } else if (event->type() == QEvent::Hide) {
                m_window->m_searchBar->setHidden(true);
                disconnect(m_window->m_searchBar, 0, object, 0);
            }

            return QObject::eventFilter(object, event);
        }

        MainWindow *m_window;
        T *m_searchable;
    };
    view->installEventFilter(new SearchFilter(model, this));
}

/// Specialization to find the fault tree container of a gate.
///
/// @param[in] gate  The gate belonging exactly to one fault tree.
///
/// @returns The fault tree container with the given gate.
template <>
mef::FaultTree *MainWindow::getFaultTree(mef::Gate *gate)
{
    /// @todo Duplicate code from EventDialog.
    auto it = boost::find_if(m_model->table<mef::FaultTree>(),
                             [&gate](const mef::FaultTree &faultTree) {
                                 return faultTree.gates().count(gate->name());
                             });
    GUI_ASSERT(it != m_model->table<mef::FaultTree>().end(), nullptr);
    return &*it;
}

template <class T>
void MainWindow::removeEvent(T *event, mef::FaultTree *faultTree)
{
    m_undoStack->push(
        new model::Model::RemoveEvent<T>(event, m_guiModel.get(), faultTree));
}

/// Specialization to deal with complexities of gate/fault-tree removal.
template <>
void MainWindow::removeEvent(model::Gate *event, mef::FaultTree *faultTree)
{
    GUI_ASSERT(faultTree->top_events().empty() == false, );
    GUI_ASSERT(faultTree->gates().empty() == false, );
    if (faultTree->top_events().front() != event->data()) {
        m_undoStack->push(new model::Model::RemoveEvent<model::Gate>(
            event, m_guiModel.get(), faultTree));
        return;
    }
    QString faultTreeName = QString::fromStdString(faultTree->name());
    if (faultTree->gates().size() > 1) {
        QMessageBox::information(
            this,
            //: The container w/ dependents still in the model.
            tr("Dependency Container Removal"),
            tr("Fault tree '%1' with root '%2' is not removable because"
               " it has dependent non-root gates."
               " Remove the gates from the fault tree"
               " before this operation.")
                .arg(faultTreeName, event->id()));
        return;
    }
    m_undoStack->beginMacro(tr("Remove fault tree '%1' with root '%2'")
                                .arg(faultTreeName, event->id()));
    m_undoStack->push(new model::Model::RemoveEvent<model::Gate>(
        event, m_guiModel.get(), faultTree));
    m_undoStack->push(
        new model::Model::RemoveFaultTree(faultTree, m_guiModel.get()));
    m_undoStack->endMacro();
}

template <class T>
void MainWindow::setupRemovable(QAbstractItemView *view)
{
    struct RemoveFilter : public QObject
    {
        RemoveFilter(QAbstractItemView *removable, MainWindow *window)
            : QObject(removable), m_window(window), m_removable(removable)
        {
        }

        void react(const QModelIndexList &indexes)
        {
            m_window->ui->actionRemoveElement->setEnabled(
                !(indexes.empty() || indexes.front().parent().isValid()));
        }

        bool eventFilter(QObject *object, QEvent *event) override
        {
            if (event->type() == QEvent::Show) {
                react(m_removable->selectionModel()->selectedIndexes());
                connect(
                    m_removable->model(), &QAbstractItemModel::modelReset,
                    m_removable, [this] {
                        react(m_removable->selectionModel()->selectedIndexes());
                    });
                connect(m_removable->selectionModel(),
                        &QItemSelectionModel::selectionChanged,
                        m_window->ui->actionRemoveElement,
                        [this](const QItemSelection &selected) {
                            react(selected.indexes());
                        });
                connect(
                    m_window->ui->actionRemoveElement, &QAction::triggered,
                    m_removable, [this] {
                        auto currentIndexes =
                            m_removable->selectionModel()->selectedIndexes();
                        GUI_ASSERT(currentIndexes.empty() == false, );
                        auto index = currentIndexes.front();
                        GUI_ASSERT(index.parent().isValid() == false, );
                        auto *element = static_cast<T *>(
                            index.data(Qt::UserRole).value<void *>());
                        GUI_ASSERT(element, );
                        auto parents =
                            m_window->m_guiModel->parents(element->data());
                        if (!parents.empty()) {
                            QMessageBox::information(
                                m_window,
                                //: The event w/ dependents in the model.
                                MainWindow::tr("Dependency Event Removal"),
                                MainWindow::tr(
                                    "Event '%1' is not removable because"
                                    " it has dependents."
                                    " Remove the event from the dependents"
                                    " before this operation.")
                                    .arg(element->id()));
                            return;
                        }
                        m_window->removeEvent(
                            element, m_window->getFaultTree(element->data()));
                    });
            } else if (event->type() == QEvent::Hide) {
                m_window->ui->actionRemoveElement->setEnabled(false);
                disconnect(m_window->ui->actionRemoveElement, 0, m_removable,
                           0);
            }

            return QObject::eventFilter(object, event);
        }

        MainWindow *m_window;
        QAbstractItemView *m_removable;
    };
    view->installEventFilter(new RemoveFilter(view, this));
}

/// Specialization to construct formula out of event editor data.
///
/// @param[in] dialog  The valid event dialog with data for a gate formula.
///
/// @returns A new formula with arguments from the event dialog.
template <>
std::unique_ptr<mef::Formula> MainWindow::extract(const EventDialog &dialog)
{
    auto getEvent = [this](const std::string &arg) -> mef::Formula::ArgEvent {
        try {
            return m_model->GetEvent(arg);
        } catch (const mef::UndefinedElement &) {
            auto argEvent = std::make_unique<mef::BasicEvent>(arg);
            argEvent->AddAttribute({"flavor", "undeveloped", ""});
            auto *address = argEvent.get();
            /// @todo Add into the parent undo.
            m_undoStack->push(new model::Model::AddEvent<model::BasicEvent>(
                std::move(argEvent), m_guiModel.get()));
            return address;
        }
    };

    mef::Formula::ArgSet arg_set;
    for (const std::string &arg : dialog.arguments())
        arg_set.Add(getEvent(arg));

    mef::Connective connective = dialog.connective();
    auto minNumber = [&connective, &dialog]() -> std::optional<int> {
        if (connective == mef::kAtleast)
            dialog.minNumber();
        return {};
    }();
    return std::make_unique<mef::Formula>(connective, std::move(arg_set),
                                          minNumber);
}

/// Specialization to construct basic event out of event editor data.
template <>
std::unique_ptr<mef::BasicEvent> MainWindow::extract(const EventDialog &dialog)
{
    auto basicEvent =
        std::make_unique<mef::BasicEvent>(dialog.name().toStdString());
    basicEvent->label(dialog.label().toStdString());
    switch (dialog.currentType()) {
    case EventDialog::BasicEvent:
        break;
    case EventDialog::Undeveloped:
        basicEvent->AddAttribute({"flavor", "undeveloped", ""});
        break;
    default:
        GUI_ASSERT(false && "unexpected event type", nullptr);
    }
    if (auto p_expression = dialog.expression()) {
        basicEvent->expression(p_expression.get());
        m_model->Add(std::move(p_expression));
    }
    return basicEvent;
}

/// Specialization to construct house event out of event editor data.
template <>
std::unique_ptr<mef::HouseEvent> MainWindow::extract(const EventDialog &dialog)
{
    GUI_ASSERT(dialog.currentType() == EventDialog::HouseEvent, nullptr);
    auto houseEvent =
        std::make_unique<mef::HouseEvent>(dialog.name().toStdString());
    houseEvent->label(dialog.label().toStdString());
    houseEvent->state(dialog.booleanConstant());
    return houseEvent;
}

/// Specialization to construct gate out of event editor data.
template <>
std::unique_ptr<mef::Gate> MainWindow::extract(const EventDialog &dialog)
{
    GUI_ASSERT(dialog.currentType() == EventDialog::Gate, nullptr);
    auto gate = std::make_unique<mef::Gate>(dialog.name().toStdString());
    gate->label(dialog.label().toStdString());
    gate->formula(extract<mef::Formula>(dialog));
    return gate;
}

void MainWindow::addElement()
{
    EventDialog dialog(m_model.get(), this);
    if (dialog.exec() == QDialog::Rejected)
        return;
    switch (dialog.currentType()) {
    case EventDialog::HouseEvent:
        m_undoStack->push(new model::Model::AddEvent<model::HouseEvent>(
            extract<mef::HouseEvent>(dialog), m_guiModel.get()));
        break;
    case EventDialog::BasicEvent:
    case EventDialog::Undeveloped:
        m_undoStack->push(new model::Model::AddEvent<model::BasicEvent>(
            extract<mef::BasicEvent>(dialog), m_guiModel.get()));
        break;
    case EventDialog::Gate: {
        m_undoStack->beginMacro(
            //: Addition of a fault by defining its root event first.
            tr("Add fault tree '%1' with gate '%2'")
                .arg(QString::fromStdString(dialog.faultTree()),
                     dialog.name()));
        auto faultTree = std::make_unique<mef::FaultTree>(dialog.faultTree());
        auto *faultTreeAddress = faultTree.get();
        m_undoStack->push(new model::Model::AddFaultTree(std::move(faultTree),
                                                         m_guiModel.get()));
        m_undoStack->push(new model::Model::AddEvent<model::Gate>(
            extract<mef::Gate>(dialog), m_guiModel.get(), faultTreeAddress));
        faultTreeAddress->CollectTopEvents();
        m_undoStack->endMacro();
    } break;
    default:
        GUI_ASSERT(false && "unexpected event type", );
    }
}

mef::FaultTree *MainWindow::getFaultTree(const EventDialog &dialog)
{
    if (dialog.faultTree().empty())
        return nullptr;
    auto it = m_model->table<mef::FaultTree>().find(dialog.faultTree());
    GUI_ASSERT(it != m_model->table<mef::FaultTree>().end(), nullptr);
    return &*it;
}

template <class T>
void MainWindow::editElement(EventDialog *dialog, model::Element *element)
{
    if (dialog->name() != element->id()) {
        m_undoStack->push(new model::Element::SetId<T>(
            static_cast<T *>(element), dialog->name(), m_model.get(),
            getFaultTree(*dialog)));
    }
    if (dialog->label() != element->label())
        m_undoStack->push(
            new model::Element::SetLabel(element, dialog->label()));
}

void MainWindow::editElement(EventDialog *dialog, model::BasicEvent *element)
{
    editElement<model::BasicEvent>(dialog, element);
    switch (dialog->currentType()) {
    case EventDialog::HouseEvent:
        m_undoStack->push(new model::Model::ChangeEventType<model::BasicEvent,
                                                            model::HouseEvent>(
            element, extract<mef::HouseEvent>(*dialog), m_guiModel.get(),
            getFaultTree(*dialog)));
        return;
    case EventDialog::BasicEvent:
    case EventDialog::Undeveloped:
        break;
    case EventDialog::Gate:
        m_undoStack->push(
            new model::Model::ChangeEventType<model::BasicEvent, model::Gate>(
                element, extract<mef::Gate>(*dialog), m_guiModel.get(),
                getFaultTree(*dialog)));
        return;
    default:
        GUI_ASSERT(false && "Unexpected event type", );
    }
    std::unique_ptr<mef::Expression> expression = dialog->expression();
    auto isEqual = [](mef::Expression *lhs, mef::Expression *rhs) {
        if (lhs == rhs) // Assumes immutable expressions.
            return true;
        if (!lhs || !rhs)
            return false;

        auto *const_lhs = dynamic_cast<mef::ConstantExpression *>(lhs);
        auto *const_rhs = dynamic_cast<mef::ConstantExpression *>(rhs);
        if (const_lhs && const_rhs && const_lhs->value() == const_rhs->value())
            return true;

        auto *exp_lhs = dynamic_cast<mef::Exponential *>(lhs);
        auto *exp_rhs = dynamic_cast<mef::Exponential *>(rhs);
        if (exp_lhs && exp_rhs
            && exp_lhs->args().front()->value()
                   == exp_rhs->args().front()->value())
            return true;

        return false;
    };

    if (!isEqual(expression.get(), element->expression())) {
        m_undoStack->push(
            new model::BasicEvent::SetExpression(element, expression.get()));
        m_model->Add(std::move(expression));
    }

    auto flavorToType = [](model::BasicEvent::Flavor flavor) {
        switch (flavor) {
        case model::BasicEvent::Basic:
            return EventDialog::BasicEvent;
        case model::BasicEvent::Undeveloped:
            return EventDialog::Undeveloped;
        }
        assert(false);
    };

    if (dialog->currentType() != flavorToType(element->flavor())) {
        m_undoStack->push([&dialog, &element]() -> QUndoCommand * {
            switch (dialog->currentType()) {
            case EventDialog::BasicEvent:
                return new model::BasicEvent::SetFlavor(
                    element, model::BasicEvent::Basic);
            case EventDialog::Undeveloped:
                return new model::BasicEvent::SetFlavor(
                    element, model::BasicEvent::Undeveloped);
            default:
                GUI_ASSERT(false && "Unexpected event type", nullptr);
            }
        }());
    }
}

void MainWindow::editElement(EventDialog *dialog, model::HouseEvent *element)
{
    editElement<model::HouseEvent>(dialog, element);
    switch (dialog->currentType()) {
    case EventDialog::HouseEvent:
        break;
    case EventDialog::BasicEvent:
    case EventDialog::Undeveloped:
        m_undoStack->push(new model::Model::ChangeEventType<model::HouseEvent,
                                                            model::BasicEvent>(
            element, extract<mef::BasicEvent>(*dialog), m_guiModel.get(),
            getFaultTree(*dialog)));
        return;
    case EventDialog::Gate:
        m_undoStack->push(
            new model::Model::ChangeEventType<model::HouseEvent, model::Gate>(
                element, extract<mef::Gate>(*dialog), m_guiModel.get(),
                getFaultTree(*dialog)));
        return;
    default:
        GUI_ASSERT(false && "Unexpected event type", );
    }
    if (dialog->booleanConstant() != element->state())
        m_undoStack->push(new model::HouseEvent::SetState(
            element, dialog->booleanConstant()));
}

void MainWindow::editElement(EventDialog *dialog, model::Gate *element)
{
    editElement<model::Gate>(dialog, element);
    switch (dialog->currentType()) {
    case EventDialog::HouseEvent:
        m_undoStack->push(
            new model::Model::ChangeEventType<model::Gate, model::HouseEvent>(
                element, extract<mef::HouseEvent>(*dialog), m_guiModel.get(),
                getFaultTree(*dialog)));
        return;
    case EventDialog::BasicEvent:
    case EventDialog::Undeveloped:
        m_undoStack->push(
            new model::Model::ChangeEventType<model::Gate, model::BasicEvent>(
                element, extract<mef::BasicEvent>(*dialog), m_guiModel.get(),
                getFaultTree(*dialog)));
        return;
    case EventDialog::Gate:
        break;
    default:
        GUI_ASSERT(false && "Unexpected event type", );
    }

    bool formulaChanged = [&dialog, &element] {
        if (dialog->connective() != element->type())
            return true;
        if (element->minNumber()
            && dialog->minNumber() != *element->minNumber())
            return true;
        std::vector<std::string> dialogArgs = dialog->arguments();
        if (element->numArgs() != dialogArgs.size())
            return true;
        auto it = dialogArgs.begin();
        for (const mef::Formula::Arg &arg : element->args()) {
            if (*it != ext::as<const mef::Event *>(arg.event)->id())
                return true;
            ++it;
        }
        return false;
    }();
    if (formulaChanged)
        m_undoStack->push(new model::Gate::SetFormula(
            element, extract<mef::Formula>(*dialog)));
}

template <class ContainerModel, typename... Ts>
QTableView *MainWindow::constructTableView(QWidget *parent, Ts &&... modelArgs)
{
    auto *table = new QTableView(parent);
    auto *tableModel =
        new ContainerModel(std::forward<Ts>(modelArgs)..., table);
    auto *proxyModel = new model::SortFilterProxyModel(table);
    proxyModel->setSourceModel(tableModel);
    table->setModel(proxyModel);
    table->setWordWrap(false);
    table->horizontalHeader()->setSortIndicatorShown(true);
    table->resizeColumnsToContents();
    table->setSortingEnabled(true);
    setupSearchable(table, proxyModel);
    return table;
}

template <class ContainerModel>
QAbstractItemView *MainWindow::constructElementTable(model::Model *guiModel,
                                                     QWidget *parent)
{
    QTableView *table = constructTableView<ContainerModel>(parent, guiModel);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    setupRemovable<typename ContainerModel::ItemModel>(table);
    connect(table, &QAbstractItemView::activated, this,
            [this](const QModelIndex &index) {
                GUI_ASSERT(index.isValid(), );
                EventDialog dialog(m_model.get(), this);
                auto *item = static_cast<typename ContainerModel::ItemModel *>(
                    index.data(Qt::UserRole).value<void *>());
                GUI_ASSERT(item, );
                dialog.setupData(*item);
                if (dialog.exec() == QDialog::Accepted)
                    editElement(&dialog, item);
            });
    return table;
}

/// Specialization to show gates as trees in tables.
template <>
QAbstractItemView *MainWindow::constructElementTable<model::GateContainerModel>(
    model::Model *guiModel, QWidget *parent)
{
    auto *tree = new QTreeView(parent);
    auto *tableModel = new model::GateContainerModel(guiModel, tree);
    auto *proxyModel = new model::GateSortFilterProxyModel(tree);
    proxyModel->setSourceModel(tableModel);
    tree->setModel(proxyModel);
    tree->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree->setSelectionMode(QAbstractItemView::SingleSelection);
    tree->setWordWrap(false);
    tree->header()->setSortIndicatorShown(true);
    tree->header()->setDefaultAlignment(Qt::AlignCenter);
    tree->resizeColumnToContents(0);
    tree->setColumnWidth(0, 2 * tree->columnWidth(0));
    tree->setAlternatingRowColors(true);
    tree->setSortingEnabled(true);

    setupSearchable(tree, proxyModel);
    setupRemovable<model::Gate>(tree);
    connect(tree, &QAbstractItemView::activated, this,
            [this](const QModelIndex &index) {
                GUI_ASSERT(index.isValid(), );
                if (index.parent().isValid())
                    return;
                EventDialog dialog(m_model.get(), this);
                auto *item = static_cast<model::Gate *>(
                    index.data(Qt::UserRole).value<void *>());
                GUI_ASSERT(item, );
                dialog.setupData(*item);
                if (dialog.exec() == QDialog::Accepted)
                    editElement(&dialog, item);
            });
    return tree;
}

void MainWindow::resetModelTree()
{
    while (ui->tabWidget->count()) {
        auto *widget = ui->tabWidget->widget(0);
        ui->tabWidget->removeTab(0);
        delete widget;
    }
    m_guiModel = std::make_unique<model::Model>(m_model.get());
    auto *oldModel = ui->modelTree->model();
    ui->modelTree->setModel(new ModelTree(m_guiModel.get(), this));
    delete oldModel;

    connect(m_guiModel.get(), &model::Model::modelNameChanged, this, [this] {
        setWindowTitle(QStringLiteral("%1[*]").arg(getModelNameForTitle()));
    });
}

void MainWindow::activateModelTree(const QModelIndex &index)
{
    GUI_ASSERT(index.isValid(), );
    if (index.parent().isValid() == false) {
        switch (static_cast<ModelTree::Row>(index.row())) {
        case ModelTree::Row::Gates: {
            auto *table = constructElementTable<model::GateContainerModel>(
                m_guiModel.get(), this);
            //: The tab for the table of gates.
            ui->tabWidget->addTab(table, tr("Gates"));
            ui->tabWidget->setCurrentWidget(table);
            return;
        }
        case ModelTree::Row::BasicEvents: {
            auto *table =
                constructElementTable<model::BasicEventContainerModel>(
                    m_guiModel.get(), this);
            //: The tab for the table of basic events.
            ui->tabWidget->addTab(table, tr("Basic Events"));
            ui->tabWidget->setCurrentWidget(table);
            return;
        }
        case ModelTree::Row::HouseEvents: {
            auto *table =
                constructElementTable<model::HouseEventContainerModel>(
                    m_guiModel.get(), this);
            //: The tab for the table of house events.
            ui->tabWidget->addTab(table, tr("House Events"));
            ui->tabWidget->setCurrentWidget(table);
            return;
        }
        case ModelTree::Row::FaultTrees:
            return;
        }
        GUI_ASSERT(false, );
    }
    GUI_ASSERT(index.parent().parent().isValid() == false, );
    GUI_ASSERT(index.parent().row()
                   == static_cast<int>(ModelTree::Row::FaultTrees), );
    auto faultTree =
        static_cast<mef::FaultTree *>(index.data(Qt::UserRole).value<void *>());
    GUI_ASSERT(faultTree, );
    activateFaultTreeDiagram(faultTree);
}

void MainWindow::activateReportTree(const QModelIndex &index)
{
    GUI_ASSERT(m_analysis, );
    GUI_ASSERT(index.isValid(), );
    QModelIndex parentIndex = index.parent();
    if (!parentIndex.isValid())
        return;
    GUI_ASSERT(parentIndex.parent().isValid() == false, );
    QString name = parentIndex.data(Qt::DisplayRole).toString();
    GUI_ASSERT(parentIndex.row() < m_analysis->results().size(), );
    const core::RiskAnalysis::Result &result =
        m_analysis->results()[parentIndex.row()];

    QWidget *widget = nullptr;
    switch (static_cast<ReportTree::Row>(index.row())) {
    case ReportTree::Row::Products: {
        bool withProbability = result.probability_analysis != nullptr;
        auto *table = constructTableView<model::ProductTableModel>(
            this, result.fault_tree_analysis->products(), withProbability);
        ui->tabWidget->addTab(table, tr("Products: %1").arg(name));
        table->sortByColumn(withProbability ? 2 : 1, withProbability
                                                         ? Qt::DescendingOrder
                                                         : Qt::AscendingOrder);
        table->setSortingEnabled(true);
        widget = table;
        break;
    }
    case ReportTree::Row::Probability:
        break;
    case ReportTree::Row::Importance: {
        widget = constructTableView<model::ImportanceTableModel>(
            this, &result.importance_analysis->importance());
        ui->tabWidget->addTab(widget, tr("Importance: %1").arg(name));
        break;
    }
    default:
        GUI_ASSERT(false && "Unexpected analysis report data", );
    }

    if (!widget)
        return;
    ui->tabWidget->setCurrentWidget(widget);
    connect(ui->reportTree->model(), &QObject::destroyed, widget,
            [this, widget] { closeTab(ui->tabWidget->indexOf(widget)); });
}

void MainWindow::activateFaultTreeDiagram(mef::FaultTree *faultTree)
{
    GUI_ASSERT(faultTree, );
    GUI_ASSERT(faultTree->top_events().size() == 1, );
    auto *topGate = faultTree->top_events().front();
    auto *view = new DiagramView(this);
    auto *scene = new diagram::DiagramScene(
        m_guiModel->gates().find(topGate)->get(), m_guiModel.get(), view);
    view->setScene(scene);
    view->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
    view->setRenderHints(QPainter::Antialiasing
                         | QPainter::SmoothPixmapTransform);
    view->setAlignment(Qt::AlignTop);
    view->ensureVisible(0, 0, 0, 0);
    setupZoomableView(view);
    setupPrintableView(view);
    setupExportableView(view);
    ui->tabWidget->addTab(
        view,
        //: The tab for a fault tree diagram.
        tr("Fault Tree: %1").arg(QString::fromStdString(faultTree->name())));
    ui->tabWidget->setCurrentWidget(view);

    connect(scene, &diagram::DiagramScene::activated, this,
            [this](model::Element *element) {
                EventDialog dialog(m_model.get(), this);
                auto action = [this, &dialog](auto *target) {
                    dialog.setupData(*target);
                    if (dialog.exec() == QDialog::Accepted) {
                        editElement(&dialog, target);
                    }
                };
                /// @todo Redesign/remove the RAII!
                if (auto *basic = dynamic_cast<model::BasicEvent *>(element)) {
                    action(basic);
                } else if (auto *gate = dynamic_cast<model::Gate *>(element)) {
                    action(gate);
                } else {
                    auto *house = dynamic_cast<model::HouseEvent *>(element);
                    GUI_ASSERT(house, );
                    action(house);
                }
            });
    connect(m_guiModel.get(), OVERLOAD(model::Model, removed, mef::FaultTree *),
            view, [this, faultTree, view](mef::FaultTree *removedTree) {
                if (removedTree == faultTree)
                    closeTab(ui->tabWidget->indexOf(view));
            });
}

void MainWindow::resetReportTree(std::unique_ptr<core::RiskAnalysis> analysis)
{
    ui->actionExportReportAs->setEnabled(static_cast<bool>(analysis));

    auto *oldModel = ui->reportTree->model();
    ui->reportTree->setModel(
        analysis ? new ReportTree(&analysis->results(), this) : nullptr);
    delete oldModel;
    m_analysis = std::move(analysis);
}

} // namespace scram::gui

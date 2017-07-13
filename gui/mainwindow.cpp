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
#include "ui_startpage.h"

#include <algorithm>
#include <type_traits>

#include <QApplication>
#include <QCoreApplication>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QPrinter>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QSvgGenerator>
#include <QTableWidget>
#include <QtConcurrent>
#include <QtOpenGL>

#include <libxml++/libxml++.h>

#include "src/config.h"
#include "src/env.h"
#include "src/error.h"
#include "src/ext/find_iterator.h"
#include "src/initializer.h"
#include "src/reporter.h"
#include "src/serialization.h"
#include "src/xml.h"

#include "elementcontainermodel.h"
#include "diagram.h"
#include "guiassert.h"
#include "printable.h"
#include "settingsdialog.h"

namespace scram {
namespace gui {

class StartPage : public QWidget, public Ui::StartPage
{
public:
    explicit StartPage(QWidget *parent = nullptr) : QWidget(parent)
    {
        setupUi(this);
    }
};

class WaitDialog : public QProgressDialog
{
public:
    explicit WaitDialog(QWidget *parent) : QProgressDialog(parent)
    {
        setFixedSize(size());
        setWindowFlags(static_cast<Qt::WindowFlags>(
            windowFlags() | Qt::MSWindowsFixedSizeDialogHint
            | Qt::FramelessWindowHint));
        setCancelButton(nullptr);
        setRange(0, 0);
        setMinimumDuration(0);
    }

private:
    void keyPressEvent(QKeyEvent *event) override
    {
        if (event->key() == Qt::Key_Escape)
            return event->accept();
        QProgressDialog::keyPressEvent(event);
    }
};

class DiagramView : public ZoomableView, public Printable
{
public:
    using ZoomableView::ZoomableView;

    void exportAs()
    {
        QString filename = QFileDialog::getSaveFileName(
            this, tr("Export As"), QDir::homePath(),
            tr("SVG files (*.svg);;All files (*.*)"));
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

namespace {

/// @returns A new table item for data tables.
QTableWidgetItem *constructTableItem(QVariant data)
{
    auto *item = new QTableWidgetItem;
    item->setData(Qt::EditRole, std::move(data));
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
      m_undoStack(new QUndoStack(this)),
      m_percentValidator(QRegularExpression(QStringLiteral(R"([1-9]\d+%?)"))),
      m_zoomBox(new QComboBox)
{
    ui->setupUi(this);

    m_zoomBox->setEditable(true);
    m_zoomBox->setEnabled(false);
    m_zoomBox->setInsertPolicy(QComboBox::NoInsert);
    m_zoomBox->setValidator(&m_percentValidator);
    for (QAction *action : ui->menuZoom->actions()) {
        m_zoomBox->addItem(action->text());
        connect(action, &QAction::triggered, m_zoomBox, [action, this] {
            m_zoomBox->setCurrentText(action->text());
        });
    }
    m_zoomBox->setCurrentText(QStringLiteral("100%"));
    ui->zoomToolBar->addWidget(m_zoomBox); // Transfer the ownership.

    setupStatusBar();
    setupActions();

    auto *startPage = new StartPage;
    connect(startPage->newModelButton, &QAbstractButton::clicked,
            ui->actionNewModel, &QAction::trigger);
    connect(startPage->openModelButton, &QAbstractButton::clicked,
            ui->actionOpenFiles, &QAction::trigger);
    connect(startPage->exampleModelsButton, &QAbstractButton::clicked, this,
            [this]() {
                openFiles(QString::fromStdString(Env::install_dir()
                                                 + "/share/scram/input"));
            });
    ui->tabWidget->addTab(startPage, startPage->windowIcon(),
                          startPage->windowTitle());

    connect(ui->treeWidget, &QTreeWidget::itemActivated, this,
            [this](QTreeWidgetItem *item) {
                if (auto it = ext::find(m_treeActions, item))
                    it->second();
            });
    connect(ui->reportTreeWidget, &QTreeWidget::itemActivated, this,
            [this](QTreeWidgetItem *item) {
                if (auto it = ext::find(m_reportActions, item))
                    it->second();
            });
    connect(ui->tabWidget, &QTabWidget::tabCloseRequested, this,
            [this](int index) {
                // Ensure show/hide order.
                if (index == ui->tabWidget->currentIndex()) {
                    int num_tabs = ui->tabWidget->count();
                    if (num_tabs > 1) {
                        ui->tabWidget->setCurrentIndex(
                            index == (num_tabs - 1) ? index - 1 : index + 1);
                    }
                }
                auto *widget = ui->tabWidget->widget(index);
                ui->tabWidget->removeTab(index);
                delete widget;
            });

    connect(ui->actionSettings, &QAction::triggered, this, [this] {
        SettingsDialog dialog(m_settings, this);
        if (dialog.exec() == QDialog::Accepted)
            m_settings = dialog.settings();
    });
    connect(ui->actionRun, &QAction::triggered, this, [this] {
        GUI_ASSERT(m_model, );
        WaitDialog progress(this);
        progress.setLabelText(tr("Running analysis..."));
        auto analysis
            = std::make_unique<core::RiskAnalysis>(m_model, m_settings);
        QFutureWatcher<void> futureWatcher;
        connect(&futureWatcher, SIGNAL(finished()), &progress, SLOT(reset()));
        futureWatcher.setFuture(
            QtConcurrent::run([&analysis] { analysis->Analyze(); }));
        progress.exec();
        futureWatcher.waitForFinished();
        resetReportWidget(std::move(analysis));
    });

    connect(this, &MainWindow::configChanged, [this] {
        m_undoStack->clear();
        setWindowTitle(QString::fromLatin1("%1[*]").arg(
            QString::fromStdString(m_model->name())));
        ui->actionSaveAs->setEnabled(true);
        ui->actionAddElement->setEnabled(true);
        ui->actionRun->setEnabled(true);
        resetTreeWidget();
        resetReportWidget(nullptr);
    });
    connect(m_undoStack, &QUndoStack::indexChanged, ui->reportTreeWidget,
            [this] {
                if (m_analysis)
                    resetReportWidget(nullptr);
            });
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
        m_settings = config.settings();
    } catch (scram::Error &err) {
        QMessageBox::critical(this, tr("Configuration Error"),
                              QString::fromUtf8(err.what()));
    }
}

void MainWindow::addInputFiles(const std::vector<std::string> &inputFiles)
{
    static xmlpp::RelaxNGValidator validator(Env::install_dir()
                                             + "/share/scram/gui.rng");

    if (inputFiles.empty())
        return;

    auto validateWithGuiSchema = [](const std::string &xmlFile) {
        std::unique_ptr<xmlpp::DomParser> parser
            = ConstructDomParser(xmlFile);
        try {
            validator.validate(parser->get_document());
        } catch (const xmlpp::validity_error &) {
            throw ValidationError(
                "Document failed validation against the GUI schema:\n"
                + xmlpp::format_xml_error());
        }
    };

    try {
        std::vector<std::string> all_input = m_inputFiles;
        all_input.insert(all_input.end(), inputFiles.begin(),
                         inputFiles.end());
        std::shared_ptr<mef::Model> new_model
            = mef::Initializer(all_input, m_settings).model();

        for (const std::string &inputFile : inputFiles)
            validateWithGuiSchema(inputFile);

        m_model = std::move(new_model);
        m_inputFiles = std::move(all_input);
    } catch (scram::Error &err) {
        QMessageBox::critical(this, tr("Initialization Error"),
                              QString::fromUtf8(err.what()));
        return;
    }

    emit configChanged();
}

void MainWindow::setupStatusBar()
{
    m_searchBar = new QLineEdit;
    m_searchBar->setHidden(true);
    m_searchBar->setFrame(false);
    m_searchBar->setMaximumHeight(m_searchBar->fontMetrics().height());
    m_searchBar->setSizePolicy(QSizePolicy::MinimumExpanding,
                               QSizePolicy::Fixed);
    m_searchBar->setPlaceholderText(tr("Find/Filter (Perl Regex)"));
    ui->statusBar->addPermanentWidget(m_searchBar);
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

    // File menu actions.
    ui->actionExit->setShortcut(QKeySequence::Quit);

    ui->actionNewModel->setShortcut(QKeySequence::New);
    connect(ui->actionNewModel, &QAction::triggered, this,
            &MainWindow::createNewModel);

    ui->actionOpenFiles->setShortcut(QKeySequence::Open);
    connect(ui->actionOpenFiles, &QAction::triggered, this,
            [this]() { openFiles(); });

    ui->actionSave->setShortcut(QKeySequence::Save);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveModel);

    ui->actionSaveAs->setShortcut(QKeySequence::SaveAs);
    connect(ui->actionSaveAs, &QAction::triggered, this,
            &MainWindow::saveModelAs);

    ui->actionPrint->setShortcut(QKeySequence::Print);

    connect(ui->actionExportReportAs, &QAction::triggered, this,
            &MainWindow::exportReportAs);

    // View menu actions.
    ui->actionZoomIn->setShortcut(QKeySequence::ZoomIn);
    ui->actionZoomOut->setShortcut(QKeySequence::ZoomOut);

    // Edit menu actions.
    connect(ui->actionAddElement, &QAction::triggered, this, [this] {
                EventDialog dialog(m_model.get(), this);
                if (dialog.exec() == QDialog::Rejected)
                    return;
                auto addBasicEvent = [&]() -> decltype(auto) {
                    auto basicEvent
                        = std::make_shared<mef::BasicEvent>(dialog.name());
                    basicEvent->label(dialog.label().toStdString());
                    if (auto p_expression = dialog.expression()) {
                        basicEvent->expression(p_expression.get());
                        m_model->Add(std::move(p_expression));
                    }
                    auto &result = *basicEvent;
                    m_undoStack->push(new model::Model::AddBasicEvent(
                        std::move(basicEvent), m_guiModel.get()));
                    return result;
                };
                switch (dialog.currentType()) {
                case EventDialog::HouseEvent: {
                    auto houseEvent
                        = std::make_shared<mef::HouseEvent>(dialog.name());
                    houseEvent->label(dialog.label().toStdString());
                    houseEvent->state(dialog.booleanConstant());
                    m_undoStack->push(new model::Model::AddHouseEvent(
                        std::move(houseEvent), m_guiModel.get()));
                    break;
                }
                case EventDialog::BasicEvent:
                    addBasicEvent();
                    break;
                case EventDialog::Undeveloped:
                    addBasicEvent().AddAttribute({"flavor", "undeveloped", ""});
                    break;
                case EventDialog::Conditional:
                    addBasicEvent().AddAttribute({"flavor", "conditional", ""});
                    break;
                default:
                    GUI_ASSERT(false && "unexpected event type", );
                }
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
    connect(searchAction, &QAction::triggered,
            [this] {
        if (m_searchBar->isHidden())
            return;
        m_searchBar->setFocus();
        m_searchBar->selectAll();
    });
}

void MainWindow::createNewModel()
{
    if (isWindowModified()) {
        QMessageBox::StandardButton answer = QMessageBox::question(
            this, tr("Save Model?"),
            tr("Save changes to model '%1' before closing?")
            .arg(QString::fromStdString(m_model->name())),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
            QMessageBox::Save);

        if (answer == QMessageBox::Cancel)
            return;
        if (answer == QMessageBox::Save) {
            saveModel();
            if (isWindowModified())
                return;
        }
    }

    m_inputFiles.clear();
    m_model = std::make_shared<mef::Model>();

    emit configChanged();
}

void MainWindow::openFiles(QString directory)
{
    QStringList filenames = QFileDialog::getOpenFileNames(
        this, tr("Open Model Files"), directory,
        QString::fromLatin1("%1 (*.mef *.opsa *.opsa-mef *.xml);;%2 (*.*)")
            .arg(tr("Model Exchange Format"), tr("All files")));
    if (filenames.empty())
        return;
    std::vector<std::string> inputFiles;
    for (const auto &filename : filenames)
        inputFiles.push_back(filename.toStdString());
    addInputFiles(inputFiles);
}

void MainWindow::saveModel()
{
    if (m_inputFiles.empty() || m_inputFiles.size() > 1)
        return saveModelAs();
    saveToFile(m_inputFiles.front());
}

void MainWindow::saveModelAs()
{
    QString filename = QFileDialog::getSaveFileName(
        this, tr("Save Model As"), QDir::homePath(),
        QString::fromLatin1("%1 (*.mef *.opsa *.opsa-mef *.xml);;%2 (*.*)")
            .arg(tr("Model Exchange Format"), tr("All files")));
    if (filename.isNull())
        return;
    saveToFile(filename.toStdString());
}

void MainWindow::saveToFile(std::string destination)
{
    GUI_ASSERT(!destination.empty(), );
    GUI_ASSERT(m_model, );
    try {
        mef::Serialize(*m_model, destination);
    } catch (Error &err) {
        QMessageBox::critical(this, tr("Save Error"),
                              QString::fromUtf8(err.what()));
        return;
    }
    m_undoStack->setClean();
    m_inputFiles.clear();
    m_inputFiles.push_back(std::move(destination));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!isWindowModified())
        return event->accept();

    QMessageBox::StandardButton answer = QMessageBox::question(
        this, tr("Save Model?"),
        tr("Save changes to model '%1' before closing?")
            .arg(QString::fromStdString(m_model->name())),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);

    if (answer == QMessageBox::Cancel)
        return event->ignore();
    if (answer == QMessageBox::Discard)
        return event->accept();

    saveModel();
    return isWindowModified() ? event->ignore() : event->accept();
}

void MainWindow::exportReportAs()
{
    GUI_ASSERT(m_analysis, );
    QString filename = QFileDialog::getSaveFileName(
        this, tr("Export Report As"), QDir::homePath(),
        QString::fromLatin1("%1 (*.mef *.opsa *.opsa-mef *.xml);;%2 (*.*)")
            .arg(tr("Model Exchange Format"), tr("All files")));
    if (filename.isNull())
        return;
    try {
        Reporter().Report(*m_analysis, filename.toStdString());
    } catch (Error &err) {
        QMessageBox::critical(this, tr("Reporting Error"),
                              QString::fromUtf8(err.what()));
    }
}

void MainWindow::setupZoomableView(ZoomableView *view)
{
    struct ZoomFilter : public QObject {
        ZoomFilter(ZoomableView *zoomable, MainWindow *window)
            : QObject(zoomable), m_window(window), m_zoomable(zoomable)
        {
        }
        bool eventFilter(QObject *object, QEvent *event) override
        {
            auto setEnabled = [this] (bool state) {
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
                    QString::fromLatin1("%1%").arg(m_zoomable->getZoom()));

                connect(m_zoomable, &ZoomableView::zoomChanged,
                        m_window->m_zoomBox, [this](int level) {
                            m_window->m_zoomBox->setCurrentText(
                                QString::fromLatin1("%1%").arg(level));
                        });
                connect(m_window->m_zoomBox, &QComboBox::currentTextChanged,
                        m_zoomable, [this](QString text) {
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
    static_assert(std::is_base_of<QObject, T>::value, "Missing QObject");
    struct PrintFilter : public QObject {
        PrintFilter(T *printable, MainWindow *window)
            : QObject(printable), m_window(window), m_printable(printable)
        {
        }
        bool eventFilter(QObject *object, QEvent *event) override
        {
            auto setEnabled = [this] (bool state) {
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
    struct ExportFilter : public QObject {
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
    struct SearchFilter : public QObject {
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

void MainWindow::editElement(EventDialog *dialog, model::Element *element)
{
    if (dialog->label() != element->label())
        m_undoStack->push(
            new model::Element::SetLabel(element, dialog->label()));
}

void MainWindow::editElement(EventDialog *dialog, model::BasicEvent *element)
{
    editElement(dialog, static_cast<model::Element *>(element));
}

void MainWindow::editElement(EventDialog *dialog, model::HouseEvent *element)
{
    editElement(dialog, static_cast<model::Element *>(element));
    if (dialog->booleanConstant() != element->state())
        m_undoStack->push(new model::HouseEvent::SetState(
            element, dialog->booleanConstant()));
}

template <class ContainerModel>
QTableView *MainWindow::constructElementTable(model::Model *guiModel,
                                              QWidget *parent)
{
    auto *table = new QTableView(parent);
    auto *tableModel = new ContainerModel(guiModel, table);
    auto *proxyModel = new model::SortFilterProxyModel(table);
    proxyModel->setSourceModel(tableModel);
    table->setModel(proxyModel);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setWordWrap(false);
    table->resizeColumnsToContents();
    table->setSortingEnabled(true);
    setupSearchable(table, proxyModel);
    connect(table, &QAbstractItemView::activated,
            [this, proxyModel](const QModelIndex &index) {
                GUI_ASSERT(index.isValid(), );
                EventDialog dialog(m_model.get(), this);
                auto *item = static_cast<typename ContainerModel::ItemModel *>(
                    proxyModel->mapToSource(index).internalPointer());
                dialog.setupData(*item);
                if (dialog.exec() == QDialog::Accepted) {
                    /// @todo Type change
                    /// @todo Name change
                    /// @todo Expression change
                    editElement(&dialog, item);
                }
            });
    return table;
}

void MainWindow::resetTreeWidget()
{
    while (ui->tabWidget->count()) {
        auto *widget = ui->tabWidget->widget(0);
        ui->tabWidget->removeTab(0);
        delete widget;
    }

    m_treeActions.clear();
    ui->treeWidget->clear();
    ui->treeWidget->setHeaderLabel(
        tr("Model: %1").arg(QString::fromStdString(m_model->name())));

    m_guiModel = std::make_unique<model::Model>(m_model.get());

    auto *faultTrees = new QTreeWidgetItem({tr("Fault Trees")});
    for (const mef::FaultTreePtr &faultTree : m_model->fault_trees()) {
        auto *widgetItem
            = new QTreeWidgetItem({QString::fromStdString(faultTree->name())});
        faultTrees->addChild(widgetItem);

        m_treeActions.emplace(widgetItem, [this, &faultTree] {
            auto *scene = new QGraphicsScene(this);
            std::unordered_map<const mef::Gate *, diagram::Gate *> transfer;
            auto *root = new diagram::Gate(*faultTree->top_events().front(),
                                           &transfer);
            scene->addItem(root);
            auto *view = new DiagramView(scene, this);
            view->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
            view->setRenderHints(QPainter::Antialiasing
                                 | QPainter::SmoothPixmapTransform);
            view->setAlignment(Qt::AlignTop);
            view->ensureVisible(root);
            setupZoomableView(view);
            setupPrintableView(view);
            setupExportableView(view);
            ui->tabWidget->addTab(
                view, tr("Fault Tree: %1")
                          .arg(QString::fromStdString(faultTree->name())));
            ui->tabWidget->setCurrentWidget(view);
        });
    }

    auto *modelData = new QTreeWidgetItem({tr("Model Data")});
    auto *basicEvents = new QTreeWidgetItem({tr("Basic Events")});
    modelData->addChild(basicEvents);
    m_treeActions.emplace(basicEvents, [this] {
        auto *table = constructElementTable<model::BasicEventContainerModel>(
            m_guiModel.get(), this);
        ui->tabWidget->addTab(table, tr("Basic Events"));
        ui->tabWidget->setCurrentWidget(table);
    });
    auto *houseEvents = new QTreeWidgetItem({tr("House Events")});
    modelData->addChild(houseEvents);
    m_treeActions.emplace(houseEvents, [this] {
        auto *table = constructElementTable<model::HouseEventContainerModel>(
            m_guiModel.get(), this);
        ui->tabWidget->addTab(table, tr("House Events"));
        ui->tabWidget->setCurrentWidget(table);
    });
    ui->treeWidget->addTopLevelItems({faultTrees, modelData});
}

void MainWindow::resetReportWidget(std::unique_ptr<core::RiskAnalysis> analysis)
{
    ui->reportTreeWidget->clear();
    m_reportActions.clear();
    m_analysis = std::move(analysis);
    ui->actionExportReportAs->setEnabled(static_cast<bool>(m_analysis));
    if (!m_analysis)
        return;

    struct {
        QString operator()(const mef::Gate *gate)
        {
            return QString::fromStdString(gate->id());
        }

        QString operator()(const std::pair<const mef::InitiatingEvent &,
                                           const mef::Sequence &> &)
        {
            GUI_ASSERT(false && "unexpected analysis target", {});
            return {};
        }
    } nameExtractor;
    for (const core::RiskAnalysis::Result &result : m_analysis->results()) {
        QString name = boost::apply_visitor(nameExtractor, result.id);
        auto *widgetItem = new QTreeWidgetItem({name});
        ui->reportTreeWidget->addTopLevelItem(widgetItem);

        GUI_ASSERT(result.fault_tree_analysis,);
        auto *productItem = new QTreeWidgetItem(
            {tr("Products: %L1")
                 .arg(result.fault_tree_analysis->products().size())});
        widgetItem->addChild(productItem);
        m_reportActions.emplace(productItem, [this, &result, name] {
            auto *table = new QTableWidget(nullptr);
            const auto &products = result.fault_tree_analysis->products();
            double sum = 0;
            if (result.probability_analysis) {
                table->setColumnCount(4);
                table->setHorizontalHeaderLabels({tr("Product"), tr("Order"),
                                                  tr("Probability"),
                                                  tr("Contribution")});
                for (const core::Product& product : products)
                    sum += product.p();
            } else {
                table->setColumnCount(2);
                table->setHorizontalHeaderLabels({tr("Product"), tr("Order")});
            }
            table->setRowCount(products.size());
            int row = 0;
            for (const core::Product &product : products) {
                QStringList members;
                for (const core::Literal &literal : product) {
                    members.push_back(QString::fromStdString(
                        (literal.complement ? "\u00AC" : "")
                        + literal.event.id()));
                }
                table->setItem(row, 0, constructTableItem(members.join(
                                           QStringLiteral(" \u22C5 "))));
                table->setItem(row, 1, constructTableItem(product.order()));
                if (result.probability_analysis) {
                    table->setItem(row, 2, constructTableItem(product.p()));
                    table->setItem(row, 3,
                                   constructTableItem(product.p() / sum));
                }
                ++row;
            }
            table->setWordWrap(false);
            table->resizeColumnsToContents();
            table->setSortingEnabled(true);
            ui->tabWidget->addTab(table, tr("Products: %1").arg(name));
            ui->tabWidget->setCurrentWidget(table);
        });

        if (result.probability_analysis) {
            widgetItem->addChild(new QTreeWidgetItem(
                {tr("Probability: %1")
                     .arg(result.probability_analysis->p_total())}));
        }

        if (result.importance_analysis) {
            auto *importanceItem = new QTreeWidgetItem(
                {tr("Importance Factors: %L1")
                     .arg(result.importance_analysis->importance().size())});
            widgetItem->addChild(importanceItem);
            m_reportActions.emplace(importanceItem, [this, &result, name] {
                auto *table = new QTableWidget(nullptr);
                table->setColumnCount(8);
                table->setHorizontalHeaderLabels(
                    {tr("Id"), tr("Occurrence"), tr("Probability"), tr("MIF"),
                     tr("CIF"), tr("DIF"), tr("RAW"), tr("RRW")});
                auto &records = result.importance_analysis->importance();
                table->setRowCount(records.size());
                int row = 0;
                for (const core::ImportanceRecord &record : records) {
                    table->setItem(
                        row, 0, constructTableItem(
                                    QString::fromStdString(record.event.id())));
                    table->setItem(
                        row, 1, constructTableItem(record.factors.occurrence));
                    table->setItem(row, 2,
                                   constructTableItem(record.event.p()));
                    table->setItem(row, 3,
                                   constructTableItem(record.factors.mif));
                    table->setItem(row, 4,
                                   constructTableItem(record.factors.cif));
                    table->setItem(row, 5,
                                   constructTableItem(record.factors.dif));
                    table->setItem(row, 6,
                                   constructTableItem(record.factors.raw));
                    table->setItem(row, 7,
                                   constructTableItem(record.factors.rrw));
                    ++row;
                }

                table->setWordWrap(false);
                table->resizeColumnsToContents();
                table->setSortingEnabled(true);
                ui->tabWidget->addTab(table, tr("Importance: %1").arg(name));
                ui->tabWidget->setCurrentWidget(table);
            });
        }
    }
}

} // namespace gui
} // namespace scram

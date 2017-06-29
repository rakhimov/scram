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

#include <QApplication>
#include <QCoreApplication>
#include <QFileDialog>
#include <QGraphicsScene>
#include <QMessageBox>
#include <QPrinter>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QSvgGenerator>
#include <QTableView>
#include <QTableWidget>
#include <QtConcurrent>
#include <QtOpenGL>

#include <libxml++/libxml++.h>

#include "src/config.h"
#include "src/env.h"
#include "src/error.h"
#include "src/ext/find_iterator.h"
#include "src/initializer.h"
#include "src/serialization.h"
#include "src/xml.h"

#include "elementcontainermodel.h"
#include "event.h"
#include "eventdialog.h"
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
      m_nameValidator(
          QRegularExpression(QStringLiteral(R"([[:alpha:]]\w*(-\w+)*)"))),
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
                auto *widget = ui->tabWidget->widget(index);
                if (dynamic_cast<Printable *>(widget)) {
                    ui->actionPrint->setEnabled(false);
                    ui->actionPrintPreview->setEnabled(false);
                }
                if (dynamic_cast<DiagramView *>(widget))
                    ui->actionExportAs->setEnabled(false);
                ui->tabWidget->removeTab(index);
                delete widget;
            });
    connect(ui->tabWidget, &QTabWidget::currentChanged, this,
            [this](int index) {
                auto *widget = ui->tabWidget->widget(index);
                if (dynamic_cast<Printable *>(widget)) {
                    ui->actionPrint->setEnabled(true);
                    ui->actionPrintPreview->setEnabled(true);
                }
                if (dynamic_cast<DiagramView *>(widget))
                    ui->actionExportAs->setEnabled(true);
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
        return;
    }

    emit configChanged();
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
    connect(ui->actionPrint, &QAction::triggered, this, [this] {
        auto *printable
            = dynamic_cast<Printable *>(ui->tabWidget->currentWidget());
        GUI_ASSERT(printable, );
        printable->print();
    });
    connect(ui->actionPrintPreview, &QAction::triggered, this, [this] {
        auto *printable
            = dynamic_cast<Printable *>(ui->tabWidget->currentWidget());
        GUI_ASSERT(printable, );
        printable->printPreview();
    });

    connect(ui->actionExportAs, &QAction::triggered, this,
            &MainWindow::exportAs);

    // View menu actions.
    ui->actionZoomIn->setShortcut(QKeySequence::ZoomIn);
    ui->actionZoomOut->setShortcut(QKeySequence::ZoomOut);

    // Edit menu actions.
    connect(ui->actionAddElement, &QAction::triggered, this, [this] {
                EventDialog dialog;
                dialog.nameLine->setValidator(&m_nameValidator);
                connect(dialog.buttonBox, &QDialogButtonBox::accepted, &dialog,
                        [&dialog, this] {
                    QString name = dialog.nameLine->text();
                    if (name.isEmpty()) {
                        QMessageBox::critical(&dialog, tr("Missing Data"),
                                              tr("The name string is empty."));
                        return;
                    }
                    try {
                        m_model->GetEvent(name.toStdString(), "");
                        QMessageBox::critical(
                            &dialog, tr("Duplicate Name"),
                            tr("The event with name '%1' already exists.")
                                .arg(name));
                        return;
                    } catch (std::out_of_range &) {
                    }
                    dialog.accept();
                });
                if (dialog.exec() == QDialog::Accepted) {
                    QString type = dialog.typeBox->currentText();
                    std::string name = dialog.nameLine->text().toStdString();
                    std::string label = dialog.labelText->toPlainText()
                                            .simplified()
                                            .toStdString();
                    if (type == tr("House event")) {
                        auto houseEvent = std::make_shared<mef::HouseEvent>(
                            std::move(name));
                        houseEvent->label(std::move(label));
                        m_undoStack->push(new model::AddHouseEventCommand(
                            std::move(houseEvent), m_guiModel.get()));
                    } else {
                        auto basicEvent = std::make_shared<mef::BasicEvent>(
                            std::move(name));
                        basicEvent->label(std::move(label));
                        if (type == tr("Conditional")) {
                            basicEvent->AddAttribute(
                                {"flavor", "conditional", ""});
                        } else if (type == tr("Undeveloped")) {
                            basicEvent->AddAttribute(
                                {"flavor", "undeveloped", ""});
                        } else {
                            GUI_ASSERT(type == tr("Basic event"), );
                        }
                        m_undoStack->push(new model::AddBasicEventCommand(
                            std::move(basicEvent), m_guiModel.get()));
                    }
                }
            });

    // Undo/Redo actions
    m_undoAction = m_undoStack->createUndoAction(this, tr("Undo:"));
    m_undoAction->setShortcuts(QKeySequence::Undo);
    m_undoAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));

    m_redoAction = m_undoStack->createRedoAction(this, tr("Redo:"));
    m_redoAction->setShortcuts(QKeySequence::Redo);
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

    resetTreeWidget();

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

void MainWindow::exportAs()
{
    QString filename = QFileDialog::getSaveFileName(
        this, tr("Export As"), QDir::homePath(),
        tr("SVG files (*.svg);;All files (*.*)"));
    QWidget *widget = ui->tabWidget->currentWidget();
    GUI_ASSERT(widget, );
    QGraphicsView *view = qobject_cast<QGraphicsView *>(widget);
    GUI_ASSERT(view, );
    QGraphicsScene *scene = view->scene();
    QSize sceneSize = scene->sceneRect().size().toSize();

    QSvgGenerator generator;
    generator.setFileName(filename);
    generator.setSize(sceneSize);
    generator.setViewBox(QRect(0, 0, sceneSize.width(), sceneSize.height()));
    generator.setTitle(filename);
    QPainter painter;
    painter.begin(&generator);
    scene->render(&painter);
    painter.end();
}

void MainWindow::activateZoom(int level)
{
    GUI_ASSERT(level > 0,);
    m_zoomBox->setEnabled(true);
    m_zoomBox->setCurrentText(QString::fromLatin1("%1%").arg(level));
    ui->actionZoomIn->setEnabled(true);
    ui->actionZoomIn->setEnabled(true);
    ui->actionZoomOut->setEnabled(true);
    ui->actionBestFit->setEnabled(true);
    ui->menuZoom->setEnabled(true);
}

void MainWindow::deactivateZoom()
{
    m_zoomBox->setEnabled(false);
    ui->actionZoomIn->setEnabled(false);
    ui->actionZoomOut->setEnabled(false);
    ui->actionBestFit->setEnabled(false);
    ui->menuZoom->setEnabled(false);
}

void MainWindow::setupZoomableView(ZoomableView *view)
{
    connect(view, &ZoomableView::zoomEnabled, this, &MainWindow::activateZoom);
    connect(view, &ZoomableView::zoomDisabled, this,
            &MainWindow::deactivateZoom);
    connect(view, &ZoomableView::zoomChanged, this, [this](int level) {
        m_zoomBox->setCurrentText(QString::fromLatin1("%1%").arg(level));
    });
    connect(ui->actionZoomIn, &QAction::triggered, view,
            [view] { view->zoomIn(5); });
    connect(ui->actionZoomOut, &QAction::triggered, view,
            [view] { view->zoomOut(5); });
    connect(m_zoomBox, &QComboBox::currentTextChanged, view,
            [view](QString text) {
                text.remove(QLatin1Char('%'));
                view->setZoom(text.toInt());
            });
    connect(ui->actionBestFit, &QAction::triggered, view, [view] {
        QSize viewSize = view->size();
        QSize sceneSize = view->scene()->sceneRect().size().toSize();
        double ratioHeight
            = static_cast<double>(viewSize.height()) / sceneSize.height();
        double ratioWidth
            = static_cast<double>(viewSize.width()) / sceneSize.width();
        view->setZoom(std::min(ratioHeight, ratioWidth) * 100);
    });
}

template <typename ContainerModel>
QTableView *constructElementTable(model::Model *guiModel, QWidget *parent)
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
    return table;
}

void MainWindow::resetTreeWidget()
{
    while (ui->tabWidget->count()) {
        auto *widget = ui->tabWidget->widget(0);
        ui->tabWidget->removeTab(0);
        delete widget;
    }

    ui->reportTreeWidget->clear();
    m_reportActions.clear();
    m_analysis.reset();

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

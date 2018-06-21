/*
 * Copyright (C) 2017-2018 Olzhas Rakhimov
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

#include "eventdialog.h"

#include <QCompleter>
#include <QListView>
#include <QObject>
#include <QPushButton>
#include <QShortcut>
#include <QStatusBar>

#include <boost/range/algorithm.hpp>

#include "src/element.h"
#include "src/expression/constant.h"
#include "src/expression/exponential.h"
#include "src/ext/bits.h"
#include "src/ext/find_iterator.h"
#include "src/ext/variant.h"

#include "guiassert.h"
#include "translate.h"
#include "validator.h"

namespace scram::gui {

QString EventDialog::redBackground(QStringLiteral("background : red;"));
QString EventDialog::yellowBackground(QStringLiteral("background : yellow;"));

EventDialog::EventDialog(mef::Model *model, QWidget *parent)
    : QDialog(parent), m_model(model), m_errorBar(new QStatusBar(this))
{
    setupUi(this);
    gridLayout->addWidget(m_errorBar, gridLayout->rowCount(), 0,
                          gridLayout->rowCount(), gridLayout->columnCount());

    nameLine->setValidator(Validator::name());
    constantValue->setValidator(Validator::probability());
    exponentialRate->setValidator(Validator::nonNegative());
    addArgLine->setValidator(Validator::name());
    containerFaultTreeName->setValidator(Validator::name());

    connect(typeBox, qOverload<int>(&QComboBox::currentIndexChanged),
            [this](int index) {
                switch (static_cast<EventType>(1 << index)) {
                case HouseEvent:
                    GUI_ASSERT(typeBox->currentText() == _("House event"), );
                    stackedWidgetType->setCurrentWidget(tabBoolean);
                    break;
                case BasicEvent:
                case Undeveloped:
                    stackedWidgetType->setCurrentWidget(tabExpression);
                    break;
                case Gate:
                    stackedWidgetType->setCurrentWidget(tabFormula);
                    break;
                default:
                    GUI_ASSERT(false, );
                }
                /// @todo Implement container change.
                if (index == ext::one_bit_index(EventType::Gate)) {
                    containerFaultTree->setEnabled(true);
                    containerFaultTree->setChecked(true);
                    containerModel->setEnabled(false);
                    if (m_fixContainerName)
                        containerFaultTreeName->setEnabled(false);
                } else {
                    containerFaultTree->setEnabled(false);
                    containerModel->setEnabled(true);
                    containerModel->setChecked(true);
                }
                validate();
            });
    connect(expressionType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &EventDialog::validate);
    connect(expressionBox, &QGroupBox::toggled, this, &EventDialog::validate);
    connectLineEdits(
        {nameLine, constantValue, exponentialRate, containerFaultTreeName});
    connect(connectiveBox, qOverload<int>(&QComboBox::currentIndexChanged),
            [this](int index) {
                minNumberBox->setEnabled(index == mef::kAtleast);
                validate();
            });
    connect(this, &EventDialog::formulaArgsChanged, [this] {
        int numArgs = argsList->count();
        int newMax = numArgs > 2 ? (numArgs - 1) : 2;
        if (minNumberBox->value() > newMax)
            minNumberBox->setValue(newMax);
        minNumberBox->setMaximum(newMax);
        validate();
    });
    connect(addArgLine, &QLineEdit::returnPressed, this, [this] {
        QString name = addArgLine->text();
        if (name.isEmpty())
            return;
        addArgLine->setStyleSheet(yellowBackground);
        if (hasFormulaArg(name)) {
            m_errorBar->showMessage(
                //: Duplicate arguments are not allowed in a formula.
                _("The argument '%1' is already in formula.").arg(name));
            return;
        }
        if (name == nameLine->text()) {
            m_errorBar->showMessage(
                //: Self-cycle is also called a loop in a graph.
                _("The argument '%1' would introduce a self-cycle.").arg(name));
            return;
        } else if (m_event) {
            auto it =
                ext::find(m_model->table<mef::Gate>(), name.toStdString());
            if (it && checkCycle(&*it)) {
                m_errorBar->showMessage(
                    //: Fault trees are acyclic graphs.
                    _("The argument '%1' would introduce a cycle.").arg(name));
                return;
            }
        }
        addArgLine->setStyleSheet({});
        argsList->addItem(name);
        emit formulaArgsChanged();
    });
    connect(addArgLine, &QLineEdit::textChanged,
            [this] { addArgLine->setStyleSheet({}); });
    stealTopFocus(addArgLine);
    setupArgCompleter();
    connect(addArgButton, &QPushButton::clicked, addArgLine,
            &QLineEdit::returnPressed);
    connect(removeArgButton, &QPushButton::clicked, argsList, [this] {
        int rows = argsList->count();
        if (rows == 0)
            return;
        if (argsList->currentItem())
            delete argsList->currentItem();
        else
            delete argsList->takeItem(rows - 1);
        emit formulaArgsChanged();
    });
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::Key_Delete), argsList);
    connect(shortcut, &QShortcut::activated, argsList, [this] {
        if (argsList->currentItem()) {
            delete argsList->currentItem();
            emit formulaArgsChanged();
        }
    });

    /// @todo Enable fault-tree as a container for events.
    containerFaultTree->setEnabled(false);

    // Ensure proper defaults.
    GUI_ASSERT(typeBox->currentIndex() == 0, );
    GUI_ASSERT(stackedWidgetType->currentIndex() == 0, );
    GUI_ASSERT(expressionType->currentIndex() == 0, );
    GUI_ASSERT(stackedWidgetExpressionData->currentIndex() == 0, );

    // Validation triggers.
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    GUI_ASSERT(okButton, );
    okButton->setEnabled(false);
    connect(this, &EventDialog::validated, okButton, &QPushButton::setEnabled);
}

std::vector<std::string> EventDialog::arguments() const
{
    std::vector<std::string> result;
    for (int i = 0; i < argsList->count(); ++i)
        result.push_back(
            argsList->item(i)->data(Qt::DisplayRole).toString().toStdString());
    return result;
}

bool EventDialog::hasFormulaArg(const QString &name)
{
    for (int i = 0; i < argsList->count(); ++i) {
        if (argsList->item(i)->data(Qt::DisplayRole) == name)
            return true;
    }
    return false;
}

bool EventDialog::checkCycle(const mef::Gate *gate)
{
    struct
    {
        bool operator()(const mef::Event *) const { return false; }
        bool operator()(const mef::Gate *arg) const
        {
            return m_self->checkCycle(arg);
        }
        EventDialog *m_self;
    } visitor{this};

    for (const mef::Formula::Arg &arg : gate->formula().args()) {
        if (ext::as<const mef::Element *>(arg.event) == m_event)
            return true;
        if (std::visit(visitor, arg.event))
            return true;
    }
    return false;
}

template <class T>
mef::FaultTree *EventDialog::getFaultTree(const T *event) const
{
    // Find the fault tree of the first parent gate.
    auto it = boost::find_if(m_model->gates(), [&event](const mef::Gate &gate) {
        auto it_arg = boost::find_if(
            gate.formula().args(), [&event](const mef::Formula::Arg &arg) {
                return arg.event
                       == mef::Formula::ArgEvent(const_cast<T *>(event));
            });
        return it_arg != gate.formula().args().end();
    });
    if (it == m_model->gates().end())
        return nullptr;
    return getFaultTree(&*it);
}

/// Finds fault tree container of the gate.
template <>
mef::FaultTree *EventDialog::getFaultTree(const mef::Gate *event) const
{
    auto it = boost::find_if(m_model->table<mef::FaultTree>(),
                             [&event](const mef::FaultTree &faultTree) {
                                 return faultTree.gates().count(event->name());
                             });
    GUI_ASSERT(it != m_model->table<mef::FaultTree>().end(), nullptr);
    return &*it;
}

template <class T>
void EventDialog::setupData(const model::Element &element, const T *origin)
{
    m_event = origin;
    m_initName = element.id();
    nameLine->setText(m_initName);
    labelText->setPlainText(element.label());
    m_fixContainerName = true;
    mef::FaultTree *faultTree = getFaultTree(origin);
    if (faultTree)
        containerFaultTreeName->setText(
            QString::fromStdString(faultTree->name()));
    else
        static_cast<QListView *>(typeBox->view())
            ->setRowHidden(ext::one_bit_index(Gate), true);
    /// @todo Allow type change w/ new fault tree creation.
}

void EventDialog::setupData(const model::HouseEvent &element)
{
    setupData(element, element.data());
    typeBox->setCurrentIndex(ext::one_bit_index(HouseEvent));
    stateBox->setCurrentIndex(element.state());
}

void EventDialog::setupData(const model::BasicEvent &element)
{
    setupData(element, element.data());
    typeBox->setCurrentIndex(ext::one_bit_index(BasicEvent) + element.flavor());
    const mef::BasicEvent &basicEvent = *element.data();
    if (basicEvent.HasExpression()) {
        expressionBox->setChecked(true);
        if (auto *constExpr = dynamic_cast<mef::ConstantExpression *>(
                &basicEvent.expression())) {
            expressionType->setCurrentIndex(0);
            constantValue->setText(QString::number(constExpr->value()));
        } else {
            auto *exponentialExpr =
                dynamic_cast<mef::Exponential *>(&basicEvent.expression());
            GUI_ASSERT(exponentialExpr, );
            expressionType->setCurrentIndex(1);
            exponentialRate->setText(
                QString::number(exponentialExpr->args().front()->value()));
        }
    } else {
        expressionBox->setChecked(false);
    }
}

void EventDialog::setupData(const model::Gate &element)
{
    setupData(element, element.data());
    typeBox->setCurrentIndex(ext::one_bit_index(Gate));

    /// @todo Deal with type changes of the top gate.
    if (getFaultTree(element.data())->top_events().front() == element.data()) {
        static_cast<QListView *>(typeBox->view())
            ->setRowHidden(ext::one_bit_index(HouseEvent), true);
        static_cast<QListView *>(typeBox->view())
            ->setRowHidden(ext::one_bit_index(BasicEvent), true);
        static_cast<QListView *>(typeBox->view())
            ->setRowHidden(ext::one_bit_index(Undeveloped), true);
    }

    connectiveBox->setCurrentIndex(element.type());
    if (element.minNumber())
        minNumberBox->setValue(*element.minNumber());
    for (const mef::Formula::Arg &arg : element.args())
        argsList->addItem(QString::fromStdString(
            ext::as<const mef::Event *>(arg.event)->id()));
    emit formulaArgsChanged(); ///< @todo Bogus signal order conflicts.
}

std::unique_ptr<mef::Expression> EventDialog::expression() const
{
    GUI_ASSERT(tabExpression->isHidden() == false, nullptr);
    if (expressionBox->isChecked() == false)
        return nullptr;
    switch (stackedWidgetExpressionData->currentIndex()) {
    case 0:
        GUI_ASSERT(constantValue->hasAcceptableInput(), nullptr);
        return std::make_unique<mef::ConstantExpression>(
            constantValue->text().toDouble());
    case 1: {
        GUI_ASSERT(exponentialRate->hasAcceptableInput(), nullptr);
        std::unique_ptr<mef::Expression> rate(
            new mef::ConstantExpression(exponentialRate->text().toDouble()));
        auto *rate_arg = rate.get();
        m_model->Add(std::move(rate));
        return std::make_unique<mef::Exponential>(rate_arg,
                                                  &m_model->mission_time());
    }
    default:
        GUI_ASSERT(false && "unexpected expression", nullptr);
    }
}

void EventDialog::validate()
{
    m_errorBar->clearMessage();
    emit validated(false);

    if (nameLine->hasAcceptableInput() == false)
        return;
    QString name = nameLine->text();
    nameLine->setStyleSheet(yellowBackground);
    try {
        if (name != m_initName) {
            m_model->GetEvent(name.toStdString());
            m_errorBar->showMessage(
                //: Duplicate event definition in the model.
                _("The event with name '%1' already exists.").arg(name));
            return;
        }
    } catch (const mef::UndefinedElement &) {
    }

    if (!tabFormula->isHidden() && hasFormulaArg(name)) {
        m_errorBar->showMessage(
            _("Name '%1' would introduce a self-cycle.").arg(name));
        return;
    }
    nameLine->setStyleSheet({});

    if (!tabExpression->isHidden() && expressionBox->isChecked()) {
        switch (stackedWidgetExpressionData->currentIndex()) {
        case 0:
            if (constantValue->hasAcceptableInput() == false)
                return;
            break;
        case 1:
            if (exponentialRate->hasAcceptableInput() == false)
                return;
            break;
        default:
            GUI_ASSERT(false && "unexpected expression", );
        }
    }

    if (!tabFormula->isHidden()) {
        int numArgs = argsList->count();
        switch (static_cast<mef::Connective>(connectiveBox->currentIndex())) {
        case mef::kNot:
        case mef::kNull:
            if (numArgs != 1) {
                m_errorBar->showMessage(
                    _("%1 connective requires a single argument.")
                        .arg(connectiveBox->currentText()));
                return;
            }
            break;
        case mef::kAnd:
        case mef::kOr:
        case mef::kNand:
        case mef::kNor:
            if (numArgs < 2) {
                m_errorBar->showMessage(
                    _("%1 connective requires 2 or more arguments.")
                        .arg(connectiveBox->currentText()));
                return;
            }
            break;
        case mef::kXor:
            if (numArgs != 2) {
                m_errorBar->showMessage(
                    _("%1 connective requires exactly 2 arguments.")
                        .arg(connectiveBox->currentText()));
                return;
            }
            break;
        case mef::kAtleast:
            if (numArgs <= minNumberBox->value()) {
                int numReqArgs = minNumberBox->value() + 1;
                m_errorBar->showMessage(
                    //: The number of required arguments is always more than 2.
                    _("%1 connective requires at-least %n arguments.", "",
                      numReqArgs)
                        .arg(connectiveBox->currentText()));
                return;
            }
            break;
        default:
            GUI_ASSERT(false && "unexpected connective", );
        }
    }

    if (containerFaultTreeName->isEnabled()) {
        if (containerFaultTreeName->hasAcceptableInput() == false)
            return;
        GUI_ASSERT(typeBox->currentIndex() == ext::one_bit_index(Gate), );
        QString faultTreeName = containerFaultTreeName->text();
        if (auto it = ext::find(m_model->fault_trees(),
                                faultTreeName.toStdString())) {
            GUI_ASSERT(it->top_events().empty() == false, );
            m_errorBar->showMessage(
                //: Fault tree redefinition.
                _("Fault tree '%1' is already defined with a top gate.")
                    .arg(faultTreeName));
            containerFaultTreeName->setStyleSheet(yellowBackground);
            return;
        }
    }
    emit validated(true);
}

void EventDialog::connectLineEdits(std::initializer_list<QLineEdit *> lineEdits)
{
    for (QLineEdit *lineEdit : lineEdits) {
        lineEdit->setStyleSheet(redBackground);
        connect(lineEdit, &QLineEdit::textChanged, [this, lineEdit] {
            if (lineEdit->hasAcceptableInput())
                lineEdit->setStyleSheet({});
            else
                lineEdit->setStyleSheet(redBackground);
            validate();
        });
    }
}

void EventDialog::stealTopFocus(QLineEdit *lineEdit)
{
    struct FocusGrabber : public QObject
    {
        FocusGrabber(QObject *parent, QPushButton *okButton)
            : QObject(parent), m_ok(okButton)
        {
        }
        bool eventFilter(QObject *object, QEvent *event) override
        {
            if (event->type() == QEvent::FocusIn) {
                m_ok->setDefault(false);
                m_ok->setAutoDefault(false);
            } else if (event->type() == QEvent::FocusOut) {
                m_ok->setDefault(true);
                m_ok->setAutoDefault(true);
            }
            return QObject::eventFilter(object, event);
        }
        QPushButton *m_ok;
    };
    lineEdit->installEventFilter(
        new FocusGrabber(lineEdit, buttonBox->button(QDialogButtonBox::Ok)));
}

void EventDialog::setupArgCompleter()
{
    /// @todo Optimize the completion model.
    QStringList allEvents;
    allEvents.reserve(m_model->gates().size() + m_model->basic_events().size()
                      + m_model->house_events().size());
    auto addEvents = [&allEvents](const auto &eventContainer) {
        for (const auto &event : eventContainer)
            allEvents.push_back(QString::fromStdString(event.id()));
    };
    addEvents(m_model->gates());
    addEvents(m_model->basic_events());
    addEvents(m_model->house_events());
    auto *completer = new QCompleter(std::move(allEvents), this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    addArgLine->setCompleter(completer);
}

} // namespace scram::gui

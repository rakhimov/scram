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
/// Dialog to edit and validate events.

#pragma once

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include <QDialog>
#include <QStatusBar>
#include <QString>

#include "ui_eventdialog.h"

#include "src/event.h"
#include "src/expression.h"
#include "src/model.h"

#include "model.h"

namespace scram::gui {

/// The Dialog to create, present, and manipulate event data.
///
/// Only valid data is accepted by this dialog.
/// That is, the dialog constrains the user input to be valid,
/// and upon the acceptance, it guarantees that the data is valid
/// for usage by the Model classes.
///
/// However, the requested data must be relevant to the current type.
///
/// @pre The model is normalized.
///
/// @todo Generalize to all model element types.
class EventDialog : public QDialog, private Ui::EventDialog
{
    Q_OBJECT

public:
    /// Event types as listed in the drop-down.
    enum EventType {
        HouseEvent = 1 << 0,
        BasicEvent = 1 << 1,
        Undeveloped = 1 << 2,
        Gate = 1 << 4
    };

    /// Creates a dialog for the definition of a new event.
    ///
    /// @param[in,out] model  The model with valid data.
    /// @param[in,out] parent  The optional owner of this object.
    explicit EventDialog(mef::Model *model, QWidget *parent = nullptr);

    /// Sets up the dialog with element data.
    ///
    /// @param[in] element  The existing valid element in the model.
    ///
    /// @pre Only one element type is requested for the setup.
    ///
    /// @todo Consider providing a template constructor instead.
    ///
    /// @{
    void setupData(const model::Gate &element);
    void setupData(const model::HouseEvent &element);
    void setupData(const model::BasicEvent &element);
    /// @}

    /// @returns The type being defined by this dialog.
    EventType currentType() const
    {
        return static_cast<EventType>(1 << typeBox->currentIndex());
    }

    /// @returns The name data.
    QString name() const { return nameLine->text(); }
    /// @returns The label data.
    QString label() const { return labelText->toPlainText().simplified(); }

    /// @returns The Boolean constant data.
    bool booleanConstant() const { return stateBox->currentIndex(); }

    /// @returns The probability expression for basic events.
    ///          nullptr if no expression is defined.
    std::unique_ptr<mef::Expression> expression() const;

    /// @returns The connective for the formula.
    mef::Connective connective() const
    {
        return static_cast<mef::Connective>(connectiveBox->currentIndex());
    }

    /// @returns The value for the min number for formulas.
    int minNumber() const { return minNumberBox->value(); }

    /// @returns The set of formula argument ids.
    std::vector<std::string> arguments() const;

    /// @returns The fault tree container name.
    std::string faultTree() const
    {
        return containerFaultTreeName->text().toStdString();
    }

signals:
    /// @param[in] valid  True if the current changes and data are valid.
    void validated(bool valid);

    /// Indicates changes in the list of formula arguments.
    void formulaArgsChanged();

public slots:
    /// Triggers validation of the current data.
    void validate();

private:
    static QString redBackground;    ///< CSS color for error/empty.
    static QString yellowBackground; ///< CSS color for partial/invalid.

    /// Checks for duplicates in the formula arguments.
    ///
    /// @param[in] name  The event name to be added to the formula.
    ///
    /// @returns true if the args list already contains the string name.
    bool hasFormulaArg(const QString &name);

    /// Checks for cycles before addition of gate arguments.
    ///
    /// @param[in] gate  The argument gate to be added into the formula list.
    ///
    /// @returns true if the arg would introduce a cycle.
    ///
    /// @pre The check is performed only for existing elements.
    /// @pre The argument is not a self-cycle.
    ///
    /// @todo Optimize to be linear.
    /// @todo Optimize with memoization.
    bool checkCycle(const mef::Gate *gate);

    /// Finds the fault tree container of the event being defined.
    ///
    /// @tparam T  The MEF type.
    ///
    /// @param[in] event  The fully initialized MEF event.
    ///
    /// @returns The fault tree the event belongs to.
    ///          nullptr if the event is unused in fault trees.
    ///
    /// @note Only gates are guaranteed to be in fault trees.
    template <class T>
    mef::FaultTree *getFaultTree(const T *event) const;

    /// Performs the setup common to all the event types.
    ///
    /// @tparam T  The MEF event type.
    ///
    /// @param[in] element  The proxy managing the element.
    /// @param[in] origin  The original MEF event.
    template <class T>
    void setupData(const model::Element &element, const T *origin);

    /// Connects the editing widgets with the dialog validation logic.
    void connectLineEdits(std::initializer_list<QLineEdit *> lineEdits);

    void stealTopFocus(QLineEdit *lineEdit); ///< Intercepts the auto-default.

    /// Sets up the formula argument completer.
    void setupArgCompleter();

    mef::Model *m_model;    ///< The main model w/ the data.
    QStatusBar *m_errorBar; ///< The bar for error/validation messages.
    QString m_initName;     ///< The name not validated for duplicates.
    const mef::Element *m_event = nullptr; ///< Set only for existing events.
    bool m_fixContainerName = false; ///< @todo Implement fault tree change.
};

} // namespace scram::gui

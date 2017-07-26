/*
 * Copyright (C) 2017 Olzhas Rakhimov
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

#ifndef EVENTDIALOG_H
#define EVENTDIALOG_H

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

namespace scram {
namespace gui {

/// The Dialog to create, present, and manipulate event data.
///
/// Only valid data is accepted by this dialog.
/// That is, the dialog constrains the user input to be valid,
/// and upon the acceptance, it guarantees that the data is valid
/// for usage by the Model classes.
class EventDialog : public QDialog, private Ui::EventDialog
{
    Q_OBJECT

public:
    enum EventType {
        HouseEvent = 1 << 0,
        BasicEvent = 1 << 1,
        Undeveloped = 1 << 2,
        Conditional = 1 << 3,
        Gate = 1 << 4
    };

    explicit EventDialog(mef::Model *model, QWidget *parent = nullptr);

    void setupData(const model::HouseEvent &element);
    void setupData(const model::BasicEvent &element);
    void setupData(const model::Gate &element);

    /// @returns The type being defined by this dialog.
    EventType currentType() const
    {
        return static_cast<EventType>(1 << typeBox->currentIndex());
    }

    /// @returns The name data.
    std::string name() const { return nameLine->text().toStdString(); }
    /// @returns The label data.
    QString label() const
    {
        return labelText->toPlainText().simplified();
    }

    /// @returns The Boolean constant data.
    bool booleanConstant() const { return stateBox->currentIndex(); }

    /// @returns The probability expression for basic events.
    ///          nullptr if no expression is defined.
    std::unique_ptr<mef::Expression> expression() const;

    /// @returns The operator type for the formula.
    mef::Operator connective() const
    {
        return static_cast<mef::Operator>(connectiveBox->currentIndex());
    }

    /// @returns The value for the vote number for formulas.
    int voteNumber() const { return voteNumberBox->value(); }

    /// @returns The set of formula argument ids.
    std::vector<std::string> arguments() const;

    /// @returns The fault tree container name.
    std::string faultTree() const
    {
        return containerFaultTreeName->text().toStdString();
    }

signals:
    void validated(bool valid);
    void formulaArgsChanged();

public slots:
    void validate();

private:
    static QString redBackground;
    static QString yellowBackground;

    /// @returns true if the arg already list contains the string name.
    bool hasFormulaArg(const QString &name);

    /// @returns true if the arg would introduce a cycle.
    ///
    /// @pre The check is performed only for existing elements.
    /// @pre The argument is not a self-cycle.
    ///
    /// @todo Optimize to be linear.
    /// @todo Optimize with memoization.
    bool checkCycle(const mef::Gate *gate);

    void setupData(const model::Element &element);
    void connectLineEdits(std::initializer_list<QLineEdit *> lineEdits);
    void stealTopFocus(QLineEdit *lineEdit);  ///< Intercept the auto-default.

    /// Sets up the formula argument completer.
    void setupArgCompleter();

    mef::Model *m_model;
    QStatusBar *m_errorBar;
    QString m_initName;  ///< The name not validated for duplicates.
    const mef::Element *m_event = nullptr;  ///< Set only for existing events.
};

} // namespace gui
} // namespace scram

#endif // EVENTDIALOG_H

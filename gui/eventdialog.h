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

#include <QDialog>
#include <QStatusBar>
#include <QString>

#include "ui_eventdialog.h"

#include "src/expression.h"
#include "src/model.h"

#include "model.h"

namespace scram {
namespace gui {

class EventDialog : public QDialog, private Ui::EventDialog
{
    Q_OBJECT

public:
    enum EventType {
        HouseEvent = 1 << 0,
        BasicEvent = 1 << 1,
        Undeveloped = 1 << 2,
        Conditional = 1 << 3
    };

    explicit EventDialog(mef::Model *model, QWidget *parent = nullptr);

    void setupData(const model::HouseEvent &element);
    void setupData(const model::BasicEvent &element);

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

signals:
    void validated(bool valid);

public slots:
    void validate();

private:
    void setupData(const model::Element &element);

    static QString redBackground;
    static QString yellowBackground;

    void connectLineEdits(std::initializer_list<QLineEdit *> lineEdits);

    mef::Model *m_model;
    QStatusBar *m_errorBar;
    QString m_initName;  ///< The name not validated for duplicates.
};

} // namespace gui
} // namespace scram

#endif // EVENTDIALOG_H

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

#include "eventdialog.h"

#include <limits>

#include <QDoubleValidator>
#include <QObject>
#include <QPushButton>
#include <QStatusBar>
#include <QRegularExpressionValidator>

#include "src/expression/constant.h"
#include "src/expression/exponential.h"

#include "guiassert.h"

namespace scram {
namespace gui {

QString EventDialog::redBackground(
    QStringLiteral("QLineEdit { background : red; }"));
QString EventDialog::yellowBackground(
    QStringLiteral("QLineEdit { background : yellow; }"));

#define OVERLOAD(type, name, ...)                                              \
    static_cast<void (type::*)(__VA_ARGS__)>(&type::name)

EventDialog::EventDialog(mef::Model *model, QWidget *parent)
    : QDialog(parent), m_model(model), m_errorBar(new QStatusBar(this))
{
    static QRegularExpressionValidator nameValidator(
        QRegularExpression(QStringLiteral(R"([[:alpha:]]\w*(-\w+)*)")));
    static QDoubleValidator nonNegativeValidator(
        0, std::numeric_limits<double>::max(), 1000);
    static QDoubleValidator probabilityValidator(0, 1, 1000);

    setupUi(this);
    gridLayout->addWidget(m_errorBar, gridLayout->rowCount(), 0,
                          gridLayout->rowCount(), gridLayout->columnCount());

    nameLine->setValidator(&nameValidator);
    constantValue->setValidator(&probabilityValidator);
    exponentialRate->setValidator(&nonNegativeValidator);

    connect(typeBox, OVERLOAD(QComboBox, currentIndexChanged, int),
            [this](int index) {
                switch (index) {
                case 0:
                    GUI_ASSERT(typeBox->currentText() == tr("House event"), );
                    stackedWidgetType->setCurrentWidget(tabBoolean);
                    break;
                case 1:
                case 2:
                case 3:
                    stackedWidgetType->setCurrentWidget(tabExpression);
                    break;
                default:
                    GUI_ASSERT(false, );
                }
                validate();
            });
    connect(expressionBox, &QGroupBox::toggled, this, &EventDialog::validate);
    connectLineEdits(nameLine, constantValue, exponentialRate);

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

std::unique_ptr<mef::Expression> EventDialog::expression() const
{
    GUI_ASSERT(tabExpression->isHidden() == false, nullptr);
    if (expressionBox->isChecked() == false)
        return nullptr;
    switch (stackedWidgetExpressionData->currentIndex()) {
    case 0:
        return std::make_unique<mef::ConstantExpression>(
            constantValue->text().toDouble());
    case 1: {
        std::unique_ptr<mef::Expression> rate(
            new mef::ConstantExpression(exponentialRate->text().toDouble()));
        auto *rate_arg = rate.get();
        m_model->Add(std::move(rate));
        return std::make_unique<mef::Exponential>(
            rate_arg, m_model->mission_time().get());
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
    try {
        QString name = nameLine->text();
        m_model->GetEvent(name.toStdString(), "");
        m_errorBar->showMessage(
            tr("The event with name '%1' already exists.").arg(name));
        return nameLine->setStyleSheet(yellowBackground);
    } catch (std::out_of_range &) {
    }

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


    emit validated(true);
}

template <class T, class... Ts>
void EventDialog::connectLineEdits(T *lineEdit, Ts*... lineEdits)
{
    lineEdit->setStyleSheet(redBackground);
    connect(lineEdit, &QLineEdit::textChanged, [this, lineEdit] {
        if (lineEdit->hasAcceptableInput())
            lineEdit->setStyleSheet({});
        else
            lineEdit->setStyleSheet(redBackground);
        validate();
    });
    connectLineEdits(lineEdits...);
}

} // namespace gui
} // namespace scram

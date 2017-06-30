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

#include "guiassert.h"

namespace scram {
namespace gui {

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
    static QString redBackround(
        QStringLiteral("QLineEdit { background : red; }"));

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
            });

    // Ensure proper defaults.
    GUI_ASSERT(typeBox->currentIndex() == 0, );
    GUI_ASSERT(stackedWidgetType->currentIndex() == 0, );
    GUI_ASSERT(expressionType->currentIndex() == 0, );
    GUI_ASSERT(stackedWidgetExpressionData->currentIndex() == 0, );

    // Validation triggers.
    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    GUI_ASSERT(okButton, );
    okButton->setEnabled(false);

    nameLine->setStyleSheet(redBackround);

    connect(nameLine, &QLineEdit::textChanged, this, &EventDialog::validate);
    connect(this, &EventDialog::validated, okButton, &QPushButton::setEnabled);
}

void EventDialog::validate()
{
    static QString redBackround(
        QStringLiteral("QLineEdit { background : red; }"));
    static QString yellowBackground(
        QStringLiteral("QLineEdit { background : yellow; }"));

    emit validated(false);

    if (nameLine->hasAcceptableInput() == false)
        return nameLine->setStyleSheet(redBackround);
    QString name = nameLine->text();
    try {
        m_model->GetEvent(name.toStdString(), "");
        m_errorBar->showMessage(
            tr("The event with name '%1' already exists.").arg(name), 5000);
        return nameLine->setStyleSheet(yellowBackground);
    } catch (std::out_of_range &) {
        nameLine->setStyleSheet({});
    }

    emit validated(true);
}

} // namespace gui
} // namespace scram

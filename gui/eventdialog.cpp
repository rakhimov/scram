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

#include "guiassert.h"

namespace scram {
namespace gui {

#define OVERLOAD(type, name, ...)                                              \
    static_cast<void (type::*)(__VA_ARGS__)>(&type::name)

EventDialog::EventDialog(QWidget *parent) : QDialog(parent) {
    setupUi(this);

    constantValue->setValidator(new QDoubleValidator(0, 1, 100, this));
    exponentialRate->setValidator(
        new QDoubleValidator(0, std::numeric_limits<double>::max(), 100, this));

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
}

} // namespace gui
} // namespace scram

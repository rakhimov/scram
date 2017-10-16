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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include "overload.h"

namespace scram {
namespace gui {

PreferencesDialog::PreferencesDialog(QSettings *preferences,
                                     QUndoStack *undoStack,
                                     QTimer *autoSaveTimer,
                                     QWidget *parent)
    : QDialog(parent), ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    if (undoStack->undoLimit()) {
        ui->checkUndoLimit->setChecked(true);
        ui->undoLimitBox->setValue(undoStack->undoLimit());
    }
    auto setUndoLimit = [preferences, undoStack](int undoLimit) {
        undoStack->setUndoLimit(undoLimit);
        preferences->setValue(QStringLiteral("undoLimit"), undoLimit);
    };
    connect(ui->undoLimitBox, OVERLOAD(QSpinBox, valueChanged, int), undoStack,
            setUndoLimit);
    connect(ui->checkUndoLimit, &QCheckBox::toggled, undoStack,
            [this, setUndoLimit](bool checked) {
                setUndoLimit(checked ? ui->undoLimitBox->value() : 0);
            });

    if (autoSaveTimer->isActive()) {
        ui->checkAutoSave->setChecked(true);
        ui->autoSaveBox->setValue(autoSaveTimer->interval() / 60000);
    }
    auto setAutoSave = [preferences, autoSaveTimer](int intervalMin) {
        if (!intervalMin) {
            autoSaveTimer->stop();
            preferences->remove(QStringLiteral("autoSave"));
        } else {
            int intervalMs = intervalMin * 60000;
            autoSaveTimer->start(intervalMs);
            preferences->setValue(QStringLiteral("autoSave"), intervalMs);
        }
    };
    connect(ui->autoSaveBox, OVERLOAD(QSpinBox, valueChanged, int),
            autoSaveTimer, setAutoSave);
    connect(ui->checkAutoSave, &QCheckBox::toggled, autoSaveTimer,
            [this, setAutoSave](bool checked) {
                setAutoSave(checked ? ui->autoSaveBox->value() : 0);
            });
}

PreferencesDialog::~PreferencesDialog() = default;

} // namespace gui
} // namespace scram

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

#include "settingsdialog.h"
#include "ui_settingsdialog.h"

namespace scram {
namespace gui {

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    connect(ui->probability, &QAbstractButton::toggled, [this](bool checked) {
        if (!checked)
            ui->importance->setChecked(false);
    });
    connect(ui->importance, &QAbstractButton::toggled, [this](bool checked) {
        if (checked)
            ui->probability->setChecked(true);
    });
    connect(ui->bdd, &QAbstractButton::toggled, [this](bool checked) {
        if (!checked) {
            ui->approximationsBox->setChecked(true);
            ui->primeImplicants->setChecked(false);
        }
    });
    connect(ui->primeImplicants, &QAbstractButton::toggled,
            [this](bool checked) {
                if (checked) {
                    ui->bdd->setChecked(true);
                    ui->approximationsBox->setChecked(false);
                }
            });
    connect(ui->approximationsBox, &QGroupBox::toggled, [this](bool checked) {
        if (checked) {
            ui->primeImplicants->setChecked(false);
        } else {
            ui->bdd->setChecked(true);
        }
    });
}

SettingsDialog::~SettingsDialog() = default;

} // namespace gui
} // namespace scram

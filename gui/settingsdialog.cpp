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

#include "src/error.h"

#include "guiassert.h"

namespace scram {
namespace gui {

SettingsDialog::SettingsDialog(const core::Settings &initSettings,
                               QWidget *parent)
    : QDialog(parent), ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);
    setupState(initSettings);
    setupConnections();
}

SettingsDialog::~SettingsDialog() = default;

core::Settings SettingsDialog::settings() const
{
    core::Settings result;
    try {
        if (ui->bdd->isChecked()) {
            result.algorithm(core::Algorithm::kBdd);
        } else if (ui->zbdd->isChecked()) {
            result.algorithm(core::Algorithm::kZbdd);
        } else {
            GUI_ASSERT(ui->mocus->isChecked(), result);
            result.algorithm(core::Algorithm::kMocus);
        }

        result.prime_implicants(ui->primeImplicants->isChecked());
        result.probability_analysis(ui->probability->isChecked());
        result.importance_analysis(ui->importance->isChecked());

        if (ui->approximationsBox->isChecked() == false) {
            result.approximation(core::Approximation::kNone);
        } else if (ui->rareEvent->isChecked()) {
            result.approximation(core::Approximation::kRareEvent);
        } else {
            GUI_ASSERT(ui->mcub->isChecked(), result);
            result.approximation(core::Approximation::kMcub);
        }

        result.limit_order(ui->productOrder->value());
        result.mission_time(ui->missionTime->value());
    } catch (Error& err) {
        GUI_ASSERT(false && err.what(), result);
    }
    return result;
}

void SettingsDialog::setupState(const core::Settings &initSettings)
{
    ui->primeImplicants->setChecked(initSettings.prime_implicants());
    ui->probability->setChecked(initSettings.probability_analysis());
    ui->importance->setChecked(initSettings.importance_analysis());
    ui->missionTime->setValue(initSettings.mission_time());
    ui->productOrder->setValue(initSettings.limit_order());

    switch (initSettings.algorithm()) {
    case core::Algorithm::kBdd:
        ui->bdd->setChecked(true);
        break;
    case core::Algorithm::kZbdd:
        ui->zbdd->setChecked(true);
        break;
    case core::Algorithm::kMocus:
        ui->mocus->setChecked(true);
        break;
    }

    ui->approximationsBox->setChecked(true);
    switch (initSettings.approximation()) {
    case core::Approximation::kNone:
        ui->approximationsBox->setChecked(false);
        break;
    case core::Approximation::kRareEvent:
        ui->rareEvent->setChecked(true);
        break;
    case core::Approximation::kMcub:
        ui->mcub->setChecked(true);
        break;
    }
}

void SettingsDialog::setupConnections()
{
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

} // namespace gui
} // namespace scram

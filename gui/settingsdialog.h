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

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <memory>

#include <QDialog>

#include "src/settings.h"

namespace Ui {
class SettingsDialog;
}

namespace scram {
namespace gui {

class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /// @param[in] initSettings  The initial settings to setup the dialog.
    explicit SettingsDialog(const core::Settings &initSettings,
                            QWidget *parent = nullptr);
    ~SettingsDialog();

    /// @returns Analysis settings derived from the dialog state.
    core::Settings settings() const;

private:
    void setupState(const core::Settings &initSettings);
    void setupConnections();

    std::unique_ptr<Ui::SettingsDialog> ui;
};

} // namespace gui
} // namespace scram

#endif // SETTINGSDIALOG_H

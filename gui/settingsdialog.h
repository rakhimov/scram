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
/// Dialog to manage analysis settings.

#pragma once

#include <memory>

#include <QDialog>

#include "src/settings.h"

namespace Ui {
class SettingsDialog;
}

namespace scram::gui {

/// The dialog to present and set analysis settings.
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    /// @param[in] initSettings  The initial settings to setup the dialog.
    /// @param[in,out] parent  The optional owner of the object.
    explicit SettingsDialog(const core::Settings &initSettings,
                            QWidget *parent = nullptr);
    ~SettingsDialog();

    /// @returns Analysis settings derived from the dialog state.
    core::Settings settings() const;

private:
    /// Initializes the dialog state with the settings data.
    void setupState(const core::Settings &initSettings);

    /// Initializes the connections in the UI elements.
    void setupConnections();

    std::unique_ptr<Ui::SettingsDialog> ui; ///< The dialog UI.
};

} // namespace scram::gui

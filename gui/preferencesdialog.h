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

/// @file
/// Dialog to manage the application's persistent preferences.

#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <memory>

#include <QDialog>
#include <QSettings>
#include <QTimer>
#include <QUndoStack>

namespace Ui {
class PreferencesDialog;
}

namespace scram {
namespace gui {

/// The dialog to present and manage GUI application preferences.
///
/// @note Some changes apply immediately,
///       while others require an application restart.
class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    /// @param[in,out] preferences  The persistent application preferences.
    /// @param[in,out] undoStack  The main undo stack.
    /// @param[in,out] autoSaveTimer  The timer to auto-save documents.
    /// @param[in,out] parent  The optional owner of the object.
    explicit PreferencesDialog(QSettings *preferences, QUndoStack *undoStack,
                               QTimer *autoSaveTimer,
                               QWidget *parent = nullptr);
    ~PreferencesDialog();

private:
    /// Queries available languages,
    /// and initializes the connections and the current language choice.
    ///
    /// @note The interface language changes take effect only after restart.
    void setupLanguage();

    /// Initializes the dialog with the undo stack data and connections.
    void setupUndoStack(QUndoStack *undoStack);

    /// Initializes the dialog with auto-save timer data and connections.
    void setupAutoSave(QTimer *autoSaveTimer);

    std::unique_ptr<Ui::PreferencesDialog> ui; ///< The Preferences UI.
    QSettings *m_preferences; ///< The persistent preferences to be saved.
};

} // namespace gui
} // namespace scram

#endif // PREFERENCESDIALOG_H

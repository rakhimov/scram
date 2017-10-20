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

#include <string>

#include <QMessageBox>

#include <boost/range/algorithm.hpp>

#include "guiassert.h"
#include "overload.h"

namespace scram {
namespace gui {

const char *const PreferencesDialog::m_languageToLocale[]
    = {"en", "ru_RU", "de_DE"};

PreferencesDialog::PreferencesDialog(QSettings *preferences,
                                     QUndoStack *undoStack,
                                     QTimer *autoSaveTimer,
                                     QWidget *parent)
    : QDialog(parent), ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);

    GUI_ASSERT(std::distance(std::begin(m_languageToLocale),
                             std::end(m_languageToLocale))
                   == ui->languageBox->count(), );
    auto it = boost::find(m_languageToLocale,
                          preferences->value(QStringLiteral("language"))
                              .toString()
                              .toStdString());
    auto it_end = std::end(m_languageToLocale);
    if (it != it_end)
        ui->languageBox->setCurrentIndex(std::distance(m_languageToLocale, it));
    connect(ui->languageBox, OVERLOAD(QComboBox, currentIndexChanged, int),
            preferences, [this, preferences](int index) {
                QMessageBox::information(
                    this, tr("Restart Required"),
                    tr("The language change will take effect after an "
                       "application restart."));
                preferences->setValue(
                    QStringLiteral("language"),
                    QString::fromLatin1(m_languageToLocale[index]));
            });

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
        int intervalMs = intervalMin * 60000;
        preferences->setValue(QStringLiteral("autoSave"), intervalMs);
        if (intervalMin)
            autoSaveTimer->start(intervalMs);
        else
            autoSaveTimer->stop();

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

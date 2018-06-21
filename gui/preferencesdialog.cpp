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

#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include <string>
#include <vector>

#include <QMessageBox>
#include <QSortFilterProxyModel>
#include <QStringListModel>

#include <boost/range/algorithm.hpp>

#include "guiassert.h"
#include "language.h"
#include "translate.h"

namespace scram::gui {

PreferencesDialog::PreferencesDialog(QSettings *preferences,
                                     QUndoStack *undoStack,
                                     QTimer *autoSaveTimer, QWidget *parent)
    : QDialog(parent), ui(new Ui::PreferencesDialog), m_preferences(preferences)
{
    ui->setupUi(this);
    setupLanguage();
    setupUndoStack(undoStack);
    setupAutoSave(autoSaveTimer);
}

PreferencesDialog::~PreferencesDialog() = default;

void PreferencesDialog::setupLanguage()
{
    // The available locales excluding the default English.
    static const std::vector<std::string> locales = gui::translations();
    // Native language representations including default English as the last.
    static const QStringList nativeLanguages = [] {
        QStringList result;
        result.reserve(locales.size());
        for (const std::string &locale : locales)
            result.push_back(gui::nativeLanguageName(locale));

        result.push_back(QStringLiteral("English"));
        return result;
    }();

    auto *listModel = new QStringListModel(nativeLanguages, this);
    auto *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(listModel);
    proxyModel->sort(0);
    ui->languageBox->setModel(proxyModel);
    int currentIndex = [this]() -> int {
        QString language =
            m_preferences->value(QStringLiteral("language")).toString();
        if (language == QStringLiteral("en")) // Assume the common case is
            return locales.size();            // the default language.

        auto it = boost::find(locales, language.toStdString());
        return std::distance(locales.begin(), it); // The default if it == end.
    }();
    ui->languageBox->setCurrentIndex(
        proxyModel->mapFromSource(listModel->index(currentIndex, 0)).row());
    connect(
        ui->languageBox, qOverload<int>(&QComboBox::currentIndexChanged), this,
        [this, proxyModel](int proxyIndex) {
            QMessageBox::information(
                this, _("Restart Required"),
                _("The language change will take effect after an "
                  "application restart."));
            int index =
                proxyModel->mapToSource(proxyModel->index(proxyIndex, 0)).row();
            QString locale = index == locales.size()
                                 ? QStringLiteral("en")
                                 : QString::fromStdString(locales[index]);
            m_preferences->setValue(QStringLiteral("language"), locale);
        });
}

void PreferencesDialog::setupUndoStack(QUndoStack *undoStack)
{
    if (undoStack->undoLimit()) {
        ui->checkUndoLimit->setChecked(true);
        ui->undoLimitBox->setValue(undoStack->undoLimit());
    }
    auto setUndoLimit = [this, undoStack](int undoLimit) {
        undoStack->setUndoLimit(undoLimit);
        m_preferences->setValue(QStringLiteral("undoLimit"), undoLimit);
    };
    connect(ui->undoLimitBox, qOverload<int>(&QSpinBox::valueChanged),
            undoStack, setUndoLimit);
    connect(ui->checkUndoLimit, &QCheckBox::toggled, undoStack,
            [this, setUndoLimit](bool checked) {
                setUndoLimit(checked ? ui->undoLimitBox->value() : 0);
            });
}

void PreferencesDialog::setupAutoSave(QTimer *autoSaveTimer)
{
    if (autoSaveTimer->isActive()) {
        ui->checkAutoSave->setChecked(true);
        ui->autoSaveBox->setValue(autoSaveTimer->interval() / 60000);
    }
    auto setAutoSave = [this, autoSaveTimer](int intervalMin) {
        int intervalMs = intervalMin * 60000;
        m_preferences->setValue(QStringLiteral("autoSave"), intervalMs);
        if (intervalMin)
            autoSaveTimer->start(intervalMs);
        else
            autoSaveTimer->stop();
    };
    connect(ui->autoSaveBox, qOverload<int>(&QSpinBox::valueChanged),
            autoSaveTimer, setAutoSave);
    connect(ui->checkAutoSave, &QCheckBox::toggled, autoSaveTimer,
            [this, setAutoSave](bool checked) {
                setAutoSave(checked ? ui->autoSaveBox->value() : 0);
            });
}

} // namespace scram::gui

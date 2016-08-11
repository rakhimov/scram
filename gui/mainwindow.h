/*
 * Copyright (C) 2015-2016 Olzhas Rakhimov
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <memory>

#include <QMainWindow>

#include "src/model.h"

namespace Ui {
class MainWindow;
}

namespace scram {
namespace gui {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    /**
     * @brief Sets the model to display in the window.
     *
     * @param model  Fully initialized and validated model.
     *               nullptr if no model present,
     *               or to remove the current model.
     */
    void setModel(std::shared_ptr<mef::Model> model)
    {
        m_model = std::move(model);
    }

private:
    Ui::MainWindow *ui;
    std::shared_ptr<mef::Model> m_model; ///< The main model to display.
};

} // namespace gui
} // namespace scram

#endif // MAINWINDOW_H

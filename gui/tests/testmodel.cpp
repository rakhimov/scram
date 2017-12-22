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

#include "gui/model.h"

#include <QSignalSpy>
#include <QtTest>

#include "help.h"

using namespace scram;

class TestModel : public QObject
{
    Q_OBJECT

private slots:
    void testElementLabelChange();
};

void TestModel::testElementLabelChange()
{
    const char *name = "pump";
    mef::BasicEvent event(name);
    gui::model::BasicEvent proxy(&event);
    QSignalSpy spy(&proxy, SIGNAL(labelChanged(const QString &)));
    QVERIFY(spy.isValid());

    TEST_EQ(event.name(), name);
    TEST_EQ(event.id(), name);
    TEST_EQ(proxy.id(), name);
    QVERIFY(spy.empty());
    QVERIFY(event.label().empty());
    QVERIFY(proxy.label().isEmpty());

    const char *label = "the label of the pump";
    gui::model::Element::SetLabel setter(&proxy, label);
    setter.redo();
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.front().size(), 1);
    TEST_EQ(spy.front().front().toString(), label);
    TEST_EQ(proxy.label(), label);
    TEST_EQ(event.label(), label);
    spy.clear();

    gui::model::Element::SetLabel(&proxy, label).redo();
    QVERIFY(spy.empty());
    TEST_EQ(proxy.label(), label);
    TEST_EQ(event.label(), label);

    setter.undo();
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.front().size(), 1);
    QVERIFY(spy.front().front().toString().isEmpty());
    QVERIFY(event.label().empty());
    QVERIFY(proxy.label().isEmpty());
}

QTEST_MAIN(TestModel)

#include "testmodel.moc"

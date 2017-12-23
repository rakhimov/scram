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
    void testModelSetName();
    void testAddFaultTree();
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

void TestModel::testModelSetName()
{
    mef::Model model;
    gui::model::Model proxy(&model);
    QVERIFY(model.HasDefaultName());
    QVERIFY(model.GetOptionalName().empty());
    QVERIFY(!model.name().empty());

    const char *name = "model";
    QSignalSpy spy(&proxy, SIGNAL(modelNameChanged(QString)));
    QVERIFY(spy.isValid());

    gui::model::Model::SetName setter(name, &proxy);
    setter.redo();
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.front().size(), 1);
    TEST_EQ(spy.front().front().toString(), name);
    TEST_EQ(proxy.id(), name);
    TEST_EQ(model.name(), name);
    TEST_EQ(model.GetOptionalName(), name);
    spy.clear();

    gui::model::Model::SetName(name, &proxy).redo();
    QVERIFY(spy.empty());
    TEST_EQ(proxy.id(), name);
    TEST_EQ(model.name(), name);

    setter.undo();
    QCOMPARE(spy.size(), 1);
    QCOMPARE(spy.front().size(), 1);
    QVERIFY(spy.front().front().toString().isEmpty());
    QVERIFY(model.HasDefaultName());
    QVERIFY(model.GetOptionalName().empty());
    QVERIFY(!model.name().empty());
    QVERIFY(proxy.id() != name);
}

void TestModel::testAddFaultTree()
{
    mef::Model model;
    gui::model::Model proxyModel(&model);
    auto faultTree = std::make_unique<mef::FaultTree>("FT");
    QVERIFY(model.fault_trees().empty());
    QVERIFY(proxyModel.faultTrees().empty());

    QSignalSpy spyAdd(&proxyModel, SIGNAL(added(mef::FaultTree *)));
    QSignalSpy spyRemove(&proxyModel, SIGNAL(removed(mef::FaultTree *)));
    QVERIFY(spyAdd.isValid());
    QVERIFY(spyRemove.isValid());

    auto *address = faultTree.get();
    gui::model::Model::AddFaultTree adder(std::move(faultTree), &proxyModel);
    adder.redo();
    QVERIFY(spyRemove.empty());
    QCOMPARE(spyAdd.size(), 1);
    QCOMPARE(spyAdd.front().size(), 1);
    TEST_EQ(model.fault_trees().size(), 1);
    QCOMPARE(model.fault_trees().begin()->get(), address);
    TEST_EQ(proxyModel.faultTrees().size(), 1);
    spyAdd.clear();

    adder.undo();
    QVERIFY(spyAdd.empty());
    QCOMPARE(spyRemove.size(), 1);
    QCOMPARE(spyRemove.front().size(), 1);
    QVERIFY(model.fault_trees().empty());
    QVERIFY(proxyModel.faultTrees().empty());
}

QTEST_MAIN(TestModel)

#include "testmodel.moc"

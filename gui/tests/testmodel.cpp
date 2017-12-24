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

#include <QtTest>

#include "gui/overload.h"
#include "help.h"

using namespace scram;

class TestModel : public QObject
{
    Q_OBJECT

private slots:
    void testElementLabelChange();
    void testModelSetName();
    void testAddFaultTree();
    void testRemoveFaultTree();
    void testAddBasicEvent();
    void testRemoveBasicEvent();
};

void TestModel::testElementLabelChange()
{
    const char *name = "pump";
    mef::BasicEvent event(name);
    gui::model::BasicEvent proxy(&event);
    auto spy = ext::make_spy(&proxy, &gui::model::Element::labelChanged);

    TEST_EQ(event.name(), name);
    TEST_EQ(event.id(), name);
    TEST_EQ(proxy.id(), name);
    QVERIFY(spy.empty());
    QVERIFY(event.label().empty());
    QVERIFY(proxy.label().isEmpty());

    const char *label = "the label of the pump";
    gui::model::Element::SetLabel setter(&proxy, label);
    setter.redo();
    TEST_EQ(spy.size(), 1);
    TEST_EQ(std::get<0>(spy.front()), label);

    TEST_EQ(proxy.label(), label);
    TEST_EQ(event.label(), label);
    spy.clear();

    gui::model::Element::SetLabel(&proxy, label).redo();
    QVERIFY(spy.empty());
    TEST_EQ(proxy.label(), label);
    TEST_EQ(event.label(), label);

    setter.undo();
    TEST_EQ(spy.size(), 1);
    QVERIFY(std::get<0>(spy.front()).isEmpty());
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
    auto spy = ext::make_spy(&proxy, &gui::model::Model::modelNameChanged);

    gui::model::Model::SetName setter(name, &proxy);
    setter.redo();
    TEST_EQ(spy.size(), 1);
    TEST_EQ(std::get<0>(spy.front()), name);
    TEST_EQ(proxy.id(), name);
    TEST_EQ(model.name(), name);
    TEST_EQ(model.GetOptionalName(), name);
    spy.clear();

    gui::model::Model::SetName(name, &proxy).redo();
    QVERIFY(spy.empty());
    TEST_EQ(proxy.id(), name);
    TEST_EQ(model.name(), name);

    setter.undo();
    TEST_EQ(spy.size(), 1);
    QVERIFY(std::get<0>(spy.front()).isEmpty());
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

    auto spyAdd = ext::make_spy(
        &proxyModel, OVERLOAD(gui::model::Model, added, mef::FaultTree *));
    auto spyRemove = ext::make_spy(
        &proxyModel, OVERLOAD(gui::model::Model, removed, mef::FaultTree *));

    auto *address = faultTree.get();
    gui::model::Model::AddFaultTree adder(std::move(faultTree), &proxyModel);
    adder.redo();
    QVERIFY(spyRemove.empty());
    TEST_EQ(spyAdd.size(), 1);
    QCOMPARE(std::get<0>(spyAdd.front()), address);
    TEST_EQ(model.fault_trees().size(), 1);
    QCOMPARE(model.fault_trees().begin()->get(), address);
    TEST_EQ(proxyModel.faultTrees().size(), 1);
    spyAdd.clear();

    adder.undo();
    QVERIFY(spyAdd.empty());
    TEST_EQ(spyRemove.size(), 1);
    QCOMPARE(std::get<0>(spyRemove.front()), address);
    QVERIFY(model.fault_trees().empty());
    QVERIFY(proxyModel.faultTrees().empty());
}

void TestModel::testRemoveFaultTree()
{
    mef::Model model;
    auto faultTree = std::make_unique<mef::FaultTree>("FT");
    auto *address = faultTree.get();
    QVERIFY(model.fault_trees().empty());
    model.Add(std::move(faultTree));
    TEST_EQ(model.fault_trees().size(), 1);
    QCOMPARE(model.fault_trees().begin()->get(), address);

    gui::model::Model proxyModel(&model);
    TEST_EQ(proxyModel.faultTrees().size(), 1);

    auto spyAdd = ext::make_spy(
        &proxyModel, OVERLOAD(gui::model::Model, added, mef::FaultTree *));
    auto spyRemove = ext::make_spy(
        &proxyModel, OVERLOAD(gui::model::Model, removed, mef::FaultTree *));

    gui::model::Model::RemoveFaultTree remover(address, &proxyModel);
    remover.redo();
    QVERIFY(spyAdd.empty());
    TEST_EQ(spyRemove.size(), 1);
    QCOMPARE(std::get<0>(spyRemove.front()), address);
    QVERIFY(model.fault_trees().empty());
    QVERIFY(proxyModel.faultTrees().empty());
    spyRemove.clear();

    remover.undo();
    QVERIFY(spyRemove.empty());
    TEST_EQ(spyAdd.size(), 1);
    QCOMPARE(std::get<0>(spyAdd.front()), address);
    TEST_EQ(model.fault_trees().size(), 1);
    QCOMPARE(model.fault_trees().begin()->get(), address);
    TEST_EQ(proxyModel.faultTrees().size(), 1);
}

void TestModel::testAddBasicEvent()
{
    mef::Model model;
    model.Add(std::make_unique<mef::FaultTree>("FT"));
    gui::model::Model proxyModel(&model);
    auto *faultTree = proxyModel.faultTrees().begin()->get();
    QVERIFY(model.basic_events().empty());
    QVERIFY(faultTree->basic_events().empty());
    QVERIFY(proxyModel.basicEvents().empty());

    auto spyAdd =
        ext::make_spy(&proxyModel, OVERLOAD(gui::model::Model, added,
                                            gui::model::BasicEvent *));
    auto spyRemove =
        ext::make_spy(&proxyModel, OVERLOAD(gui::model::Model, removed,
                                            gui::model::BasicEvent *));

    auto event = std::make_unique<mef::BasicEvent>("pump");
    auto *address = event.get();

    gui::model::Model::AddEvent<gui::model::BasicEvent> adder(
        std::move(event), &proxyModel, faultTree);
    adder.redo();
    QVERIFY(spyRemove.empty());
    TEST_EQ(spyAdd.size(), 1);
    gui::model::BasicEvent *proxyEvent = std::get<0>(spyAdd.front());
    QCOMPARE(proxyEvent->data(), address);

    TEST_EQ(model.basic_events().size(), 1);
    TEST_EQ(model.basic_events().begin()->get(), address);
    TEST_EQ(faultTree->basic_events().size(), /*expected=1, but normalized=*/0);
    /* TEST_EQ(*faultTree->basic_events().begin(), address); */
    TEST_EQ(proxyModel.basicEvents().size(), 1);
    TEST_EQ(proxyModel.basicEvents().begin()->get(), proxyEvent);
    spyAdd.clear();

    adder.undo();
    QVERIFY(spyAdd.empty());
    TEST_EQ(spyRemove.size(), 1);
    QCOMPARE(std::get<0>(spyRemove.front()), proxyEvent);
    QVERIFY(model.basic_events().empty());
    QVERIFY(faultTree->basic_events().empty());
    QVERIFY(proxyModel.basicEvents().empty());
}

void TestModel::testRemoveBasicEvent()
{
    mef::Model model;
    model.Add(std::make_unique<mef::FaultTree>("FT"));
    auto *faultTree = model.fault_trees().begin()->get();
    auto event = std::make_unique<mef::BasicEvent>("pump");
    auto *address = event.get();
    model.Add(std::move(event));
    faultTree->Add(address);
    gui::model::Model proxyModel(&model);

    TEST_EQ(model.basic_events().size(), 1);
    TEST_EQ(faultTree->basic_events().size(), /*expected=1, but normalized=*/0);
    TEST_EQ(proxyModel.basicEvents().size(), 1);
    auto *proxyEvent = proxyModel.basicEvents().begin()->get();
    QCOMPARE(proxyEvent->data(), address);

    auto spyAdd =
        ext::make_spy(&proxyModel, OVERLOAD(gui::model::Model, added,
                                            gui::model::BasicEvent *));
    auto spyRemove =
        ext::make_spy(&proxyModel, OVERLOAD(gui::model::Model, removed,
                                            gui::model::BasicEvent *));

    gui::model::Model::RemoveEvent<gui::model::BasicEvent> remover(
        proxyEvent, &proxyModel, faultTree);
    remover.redo();
    QVERIFY(spyAdd.empty());
    TEST_EQ(spyRemove.size(), 1);
    QCOMPARE(std::get<0>(spyRemove.front()), proxyEvent);
    QVERIFY(model.basic_events().empty());
    QVERIFY(faultTree->basic_events().empty());
    QVERIFY(proxyModel.basicEvents().empty());
    spyRemove.clear();

    remover.undo();
    QVERIFY(spyRemove.empty());
    TEST_EQ(spyAdd.size(), 1);
    QCOMPARE(std::get<0>(spyAdd.front()), proxyEvent);
    TEST_EQ(model.basic_events().size(), 1);
    QCOMPARE(model.basic_events().begin()->get(), address);
    TEST_EQ(faultTree->basic_events().size(), /*expected=1, but normalized=*/0);
    TEST_EQ(proxyModel.basicEvents().size(), 1);
    QCOMPARE(proxyModel.basicEvents().begin()->get(), proxyEvent);
}

QTEST_MAIN(TestModel)

#include "testmodel.moc"

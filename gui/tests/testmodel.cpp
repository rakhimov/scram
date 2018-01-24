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

#include "gui/model.h"

#include <QtTest>

#include "src/expression/constant.h"

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
    void testAddBasicEvent() { testAddEvent<gui::model::BasicEvent>(); }
    void testAddHouseEvent() { testAddEvent<gui::model::HouseEvent>(); }
    void testAddGate() { testAddEvent<gui::model::Gate>(); }
    void testRemoveBasicEvent() { testRemoveEvent<gui::model::BasicEvent>(); }
    void testRemoveHouseEvent() { testRemoveEvent<gui::model::HouseEvent>(); }
    void testRemoveGate() { testRemoveEvent<gui::model::Gate>(); }
    void testHouseEventState();
    void testBasicEventFlavorToString();
    void testBasicEventSetFlavor();
    void testBasicEventConstructWithFlavor();
    void testBasicEventSetExpression();
    void testGateType();
    void testGateSetFormula();
    void testBasicEventParents() { testEventParents<gui::model::BasicEvent>(); }
    void testHouseEventParents() { testEventParents<gui::model::HouseEvent>(); }
    void testGateParents() { testEventParents<gui::model::Gate>(); }
    void testBasicEventSetId() { testEventSetId<gui::model::BasicEvent>(); }
    void testHouseEventSetId() { testEventSetId<gui::model::HouseEvent>(); }
    void testGateSetId() { testEventSetId<gui::model::Gate>(); }

private:
    /// @tparam T  Proxy type.
    template <class T>
    void testAddEvent();

    /// @tparam T  Proxy type.
    template <class T>
    void testRemoveEvent();

    /// @tparam T  Proxy type.
    template <class T>
    void testEventParents();

    /// @tparam T  Proxy type.
    template <class T>
    void testEventSetId();
};

void TestModel::testElementLabelChange()
{
    const char *name = "pump";
    mef::BasicEvent event(name);
    gui::model::BasicEvent proxy(&event);
    auto spy = ext::SignalSpy(&proxy, &gui::model::Element::labelChanged);

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
    auto spy = ext::SignalSpy(&proxy, &gui::model::Model::modelNameChanged);

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

    auto spyAdd = ext::SignalSpy(
        &proxyModel, OVERLOAD(gui::model::Model, added, mef::FaultTree *));
    auto spyRemove = ext::SignalSpy(
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

    auto spyAdd = ext::SignalSpy(
        &proxyModel, OVERLOAD(gui::model::Model, added, mef::FaultTree *));
    auto spyRemove = ext::SignalSpy(
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

namespace {

template <class T>
decltype(auto) table(const mef::FaultTree &faultTree);

template <class T>
decltype(auto) table(const mef::Model &model);

template <>
decltype(auto) table<mef::BasicEvent>(const mef::FaultTree &faultTree)
{
    return faultTree.basic_events();
}

template <>
decltype(auto) table<mef::BasicEvent>(const mef::Model &model)
{
    return model.basic_events();
}

template <>
decltype(auto) table<mef::HouseEvent>(const mef::FaultTree &faultTree)
{
    return faultTree.house_events();
}

template <>
decltype(auto) table<mef::HouseEvent>(const mef::Model &model)
{
    return model.house_events();
}

template <>
decltype(auto) table<mef::Gate>(const mef::FaultTree &faultTree)
{
    return faultTree.gates();
}

template <>
decltype(auto) table<mef::Gate>(const mef::Model &model)
{
    return model.gates();
}

/// @tparam T  Proxy type.
template <class T>
struct IsNormalized : std::true_type
{
};

template <>
struct IsNormalized<mef::Gate> : std::false_type
{
};

template <class T>
void testNormalized(const mef::FaultTree &faultTree, const T *address,
                    const char *info)
{
    qWarning("%s", info);
    if (IsNormalized<T>::value) {
        TEST_EQ(table<T>(faultTree).size(), 0);
    } else {
        TEST_EQ(table<T>(faultTree).size(), 1);
        TEST_EQ(*table<T>(faultTree).begin(), address);
    }
}

template <class T>
auto makeDefaultEvent(std::string name)
{
    return std::make_unique<T>(name);
}

template <>
auto makeDefaultEvent<mef::Gate>(std::string name)
{
    auto gate = std::make_unique<mef::Gate>(name);
    gate->formula(std::make_unique<mef::Formula>(mef::kNull));
    gate->formula().Add(&mef::HouseEvent::kTrue);
    return gate;
}

} // namespace

template <class T>
void TestModel::testAddEvent()
{
    using E = typename T::Origin;

    mef::Model model;
    model.Add(std::make_unique<mef::FaultTree>("FT"));
    gui::model::Model proxyModel(&model);
    auto *faultTree = proxyModel.faultTrees().begin()->get();
    QVERIFY(table<E>(model).empty());
    QVERIFY(table<E>(*faultTree).empty());
    QVERIFY(proxyModel.table<T>().empty());

    auto spyAdd =
        ext::SignalSpy(&proxyModel, OVERLOAD(gui::model::Model, added, T *));
    auto spyRemove =
        ext::SignalSpy(&proxyModel, OVERLOAD(gui::model::Model, removed, T *));

    auto event = std::make_unique<E>("pump");
    auto *address = event.get();

    gui::model::Model::AddEvent<T> adder(std::move(event), &proxyModel,
                                         faultTree);
    adder.redo();
    QVERIFY(spyRemove.empty());
    TEST_EQ(spyAdd.size(), 1);
    T *proxyEvent = std::get<0>(spyAdd.front());
    QCOMPARE(proxyEvent->data(), address);

    TEST_EQ(table<E>(model).size(), 1);
    TEST_EQ(table<E>(model).begin()->get(), address);
    testNormalized(*faultTree, address, "Add event into fault tree.");
    TEST_EQ(proxyModel.table<T>().size(), 1);
    TEST_EQ(proxyModel.table<T>().begin()->get(), proxyEvent);
    spyAdd.clear();

    adder.undo();
    QVERIFY(spyAdd.empty());
    TEST_EQ(spyRemove.size(), 1);
    QCOMPARE(std::get<0>(spyRemove.front()), proxyEvent);
    QVERIFY(table<E>(model).empty());
    QVERIFY(table<E>(*faultTree).empty());
    QVERIFY(proxyModel.table<T>().empty());
}

template <class T>
void TestModel::testRemoveEvent()
{
    using E = typename T::Origin;

    mef::Model model;
    model.Add(std::make_unique<mef::FaultTree>("FT"));
    auto *faultTree = model.fault_trees().begin()->get();
    auto event = std::make_unique<E>("pump");
    auto *address = event.get();
    model.Add(std::move(event));
    faultTree->Add(address);
    gui::model::Model proxyModel(&model);

    TEST_EQ(table<E>(model).size(), 1);
    testNormalized(*faultTree, address, "Add event into fault tree.");
    TEST_EQ(proxyModel.table<T>().size(), 1);
    auto *proxyEvent = proxyModel.table<T>().begin()->get();
    QCOMPARE(proxyEvent->data(), address);

    auto spyAdd =
        ext::SignalSpy(&proxyModel, OVERLOAD(gui::model::Model, added, T *));
    auto spyRemove =
        ext::SignalSpy(&proxyModel, OVERLOAD(gui::model::Model, removed, T *));

    gui::model::Model::RemoveEvent<T> remover(proxyEvent, &proxyModel,
                                              faultTree);
    remover.redo();
    QVERIFY(spyAdd.empty());
    TEST_EQ(spyRemove.size(), 1);
    QCOMPARE(std::get<0>(spyRemove.front()), proxyEvent);
    QVERIFY(table<E>(model).empty());
    QVERIFY(table<E>(*faultTree).empty());
    QVERIFY(proxyModel.table<T>().empty());
    spyRemove.clear();

    remover.undo();
    QVERIFY(spyRemove.empty());
    TEST_EQ(spyAdd.size(), 1);
    QCOMPARE(std::get<0>(spyAdd.front()), proxyEvent);
    TEST_EQ(table<E>(model).size(), 1);
    TEST_EQ(table<E>(model).begin()->get(), address);
    testNormalized(*faultTree, address, "Undo event removal.");
    TEST_EQ(proxyModel.table<T>().size(), 1);
    QCOMPARE(proxyModel.table<T>().begin()->get(), proxyEvent);
}

void TestModel::testHouseEventState()
{
    mef::HouseEvent event("Flood");
    gui::model::HouseEvent proxy(&event);
    QCOMPARE(event.state(), false);
    QCOMPARE(proxy.state(), false);
    TEST_EQ(proxy.state<QString>(), "False");

    auto spy = ext::SignalSpy(&proxy, &gui::model::HouseEvent::stateChanged);
    gui::model::HouseEvent::SetState setter(&proxy, true);
    setter.redo();
    TEST_EQ(spy.size(), 1);
    QCOMPARE(std::get<0>(spy.front()), true);
    QCOMPARE(proxy.state(), true);
    QCOMPARE(event.state(), true);
    TEST_EQ(proxy.state<QString>(), "True");
    spy.clear();

    setter.undo();
    TEST_EQ(spy.size(), 1);
    QCOMPARE(std::get<0>(spy.front()), false);
    QCOMPARE(event.state(), false);
    QCOMPARE(proxy.state(), false);
    TEST_EQ(proxy.state<QString>(), "False");
}

void TestModel::testBasicEventFlavorToString()
{
    using namespace gui::model;
    TEST_EQ(BasicEvent::flavorToString(BasicEvent::Basic), "Basic");
    TEST_EQ(BasicEvent::flavorToString(BasicEvent::Undeveloped), "Undeveloped");
    TEST_EQ(BasicEvent::flavorToString(BasicEvent::Conditional), "Conditional");
}

void TestModel::testBasicEventSetFlavor()
{
    mef::BasicEvent event("pump");
    gui::model::BasicEvent proxy(&event);
    QVERIFY(event.attributes().empty());
    QCOMPARE(proxy.flavor(), gui::model::BasicEvent::Basic);

    auto spy = ext::SignalSpy(&proxy, &gui::model::BasicEvent::flavorChanged);
    auto value = gui::model::BasicEvent::Undeveloped;
    gui::model::BasicEvent::SetFlavor setter(&proxy, value);
    setter.redo();
    TEST_EQ(spy.size(), 1);
    QCOMPARE(std::get<0>(spy.front()), value);
    QCOMPARE(proxy.flavor(), value);
    QVERIFY(event.HasAttribute("flavor"));
    spy.clear();

    setter.undo();
    TEST_EQ(spy.size(), 1);
    QCOMPARE(std::get<0>(spy.front()), gui::model::BasicEvent::Basic);
    QVERIFY(event.attributes().empty());
    QCOMPARE(proxy.flavor(), gui::model::BasicEvent::Basic);
}

void TestModel::testBasicEventConstructWithFlavor()
{
    mef::BasicEvent event("pump");
    {
        QVERIFY(event.attributes().empty());
        QCOMPARE(gui::model::BasicEvent(&event).flavor(),
                 gui::model::BasicEvent::Basic);
    }
    {
        event.SetAttribute({"flavor", "undeveloped"});
        QCOMPARE(gui::model::BasicEvent(&event).flavor(),
                 gui::model::BasicEvent::Undeveloped);
    }
    {
        event.SetAttribute({"flavor", "conditional"});
        QCOMPARE(gui::model::BasicEvent(&event).flavor(),
                 gui::model::BasicEvent::Conditional);
    }
}

void TestModel::testBasicEventSetExpression()
{
    mef::BasicEvent event("pump");
    gui::model::BasicEvent proxy(&event);
    QVERIFY(!event.HasExpression());
    QVERIFY(!proxy.expression());
    QCOMPARE(proxy.probability<QVariant>(), QVariant());

    double value = 0.1;
    mef::ConstantExpression prob(value);
    auto spy =
        ext::SignalSpy(&proxy, &gui::model::BasicEvent::expressionChanged);
    gui::model::BasicEvent::SetExpression setter(&proxy, &prob);
    setter.redo();
    QCOMPARE(prob.value(), value);

    TEST_EQ(spy.size(), 1);
    TEST_EQ(std::get<0>(spy.front()), &prob);
    TEST_EQ(&event.expression(), &prob);
    QCOMPARE(event.p(), value);
    TEST_EQ(proxy.expression(), &prob);
    QCOMPARE(proxy.probability(), value);
    spy.clear();

    setter.undo();
    TEST_EQ(spy.size(), 1);
    TEST_EQ(std::get<0>(spy.front()), nullptr);
    QVERIFY(!event.HasExpression());
    QVERIFY(!proxy.expression());
    QCOMPARE(proxy.probability<QVariant>(), QVariant());
}

void TestModel::testGateType()
{
    mef::Gate gate("pump");
    gui::model::Gate proxy(&gate);
    gate.formula(std::make_unique<mef::Formula>(mef::kNull));
    TEST_EQ(proxy.type<QString>(), "null");
    gate.formula(std::make_unique<mef::Formula>(mef::kAnd));
    TEST_EQ(proxy.type<QString>(), "and");
    gate.formula(std::make_unique<mef::Formula>(mef::kOr));
    TEST_EQ(proxy.type<QString>(), "or");
    gate.formula(std::make_unique<mef::Formula>(mef::kXor));
    TEST_EQ(proxy.type<QString>(), "xor");
    gate.formula(std::make_unique<mef::Formula>(mef::kNor));
    TEST_EQ(proxy.type<QString>(), "nor");
    gate.formula(std::make_unique<mef::Formula>(mef::kNot));
    TEST_EQ(proxy.type<QString>(), "not");
    gate.formula(std::make_unique<mef::Formula>(mef::kNand));
    TEST_EQ(proxy.type<QString>(), "nand");

    auto vote = std::make_unique<mef::Formula>(mef::kAtleast);
    vote->min_number(2);
    gate.formula(std::move(vote));
    TEST_EQ(proxy.type<QString>(), "at-least 2");
    QCOMPARE(proxy.minNumber(), 2);
}

void TestModel::testGateSetFormula()
{
    mef::Gate gate("pump");
    QVERIFY(!gate.HasFormula());
    gate.formula(std::make_unique<mef::Formula>(mef::kNot));
    auto *initFormula = &gate.formula();
    gui::model::Gate proxy(&gate);
    QCOMPARE(proxy.type(), mef::kNot);

    auto formula = std::make_unique<mef::Formula>(mef::kNull);
    auto *address = formula.get();
    auto spy = ext::SignalSpy(&proxy, &gui::model::Gate::formulaChanged);
    gui::model::Gate::SetFormula setter(&proxy, std::move(formula));
    setter.redo();
    TEST_EQ(spy.size(), 1);
    QCOMPARE(proxy.type(), mef::kNull);
    QVERIFY(gate.HasFormula());
    QCOMPARE(&gate.formula(), address);
    QCOMPARE(proxy.numArgs(), 0);
    QVERIFY(proxy.args().empty());
    spy.clear();

    setter.undo();
    TEST_EQ(spy.size(), 1);
    QVERIFY(gate.HasFormula());
    QCOMPARE(&gate.formula(), initFormula);
    QCOMPARE(proxy.type(), mef::kNot);
}

template <class T>
void TestModel::testEventParents()
{
    mef::Model model;
    gui::model::Model proxy(&model);
    auto event = makeDefaultEvent<typename T::Origin>("pump");
    auto *address = event.get();
    gui::model::Model::AddEvent<T>(std::move(event), &proxy).redo();

    auto gate = std::make_unique<mef::Gate>("parent");
    auto *parent = gate.get();
    gate->formula(std::make_unique<mef::Formula>(mef::kNull));
    gate->formula().Add(address);

    QVERIFY(proxy.parents(address).empty());
    gui::model::Model::AddEvent<gui::model::Gate>(std::move(gate), &proxy)
        .redo();
    auto *proxyParent = proxy.gates().find(parent)->get();
    QVERIFY(proxy.parents(address).size() == 1);
    QCOMPARE(proxy.parents(address).front(), proxyParent);

    gui::model::Model::RemoveEvent<gui::model::Gate>(proxyParent, &proxy)
        .redo();
    QVERIFY(proxy.parents(address).empty());
}

template <class T>
void TestModel::testEventSetId()
{
    using E = typename T::Origin;

    mef::Model model;
    model.Add(std::make_unique<mef::FaultTree>("FT"));
    auto *faultTree = model.fault_trees().begin()->get();
    const char oldName[] = "pump";
    auto event = std::make_unique<E>(oldName);
    auto *address = event.get();
    model.Add(std::move(event));
    faultTree->Add(address);
    gui::model::Model proxyModel(&model);
    auto *proxyEvent = proxyModel.table<T>().find(address)->get();
    TEST_EQ(proxyEvent->id(), oldName);

    const char newName[] = "valve";
    auto spy = ext::SignalSpy(proxyEvent, &gui::model::Element::idChanged);
    gui::model::Element::SetId<T> setter(proxyEvent, newName, &model,
                                         faultTree);
    setter.redo();
    TEST_EQ(spy.size(), 1);
    TEST_EQ(std::get<0>(spy.front()), newName);
    TEST_EQ(proxyEvent->id(), newName);
    TEST_EQ(address->id(), newName);
    spy.clear();

    setter.undo();
    TEST_EQ(spy.size(), 1);
    TEST_EQ(std::get<0>(spy.front()), oldName);
    TEST_EQ(proxyEvent->id(), oldName);
    TEST_EQ(address->id(), oldName);
}

QTEST_APPLESS_MAIN(TestModel)

#include "testmodel.moc"

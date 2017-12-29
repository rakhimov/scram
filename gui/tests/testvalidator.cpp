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

#include "gui/validator.h"

#include <QtTest>

#include "data.h"

Q_DECLARE_METATYPE(QValidator::State)

namespace QTest {

/// @warning This is a C string API in Qt C++ code!!!
///          The caller must deallocate the returned string.
///          This is only specialized for QCOMPARE to print better messages.
template <>
char *toString(const QValidator::State &state)
{
    switch (state) {
    case QValidator::Invalid:
        return qstrdup("Invalid");
    case QValidator::Intermediate:
        return qstrdup("Intermediate");
    case QValidator::Acceptable:
        return qstrdup("Acceptable");
    }
    Q_UNREACHABLE();
}

} // namespace QTest

class TestValidator : public QObject
{
    Q_OBJECT

private slots:
    void testName_data()
    {
        populateData<QString, QValidator::State>(
            {"value", "state"},
            {{"letter", "a", QValidator::Acceptable},
             {"number", "1", QValidator::Invalid},
             {"hyphen", "-", QValidator::Invalid},
             {"underscore", "_", QValidator::Invalid},
             {"space", " ", QValidator::Invalid},
             {"dot", ".", QValidator::Invalid},
             {"w/ space", "pump one", QValidator::Invalid},
             {"w/ tab", "pump\tone", QValidator::Invalid},
             {"w/ dot", "pump.one", QValidator::Invalid},
             {"w/ colon", "pump:one", QValidator::Invalid},
             {"w/ hyphen", "pump-one", QValidator::Acceptable},
             {"w/ leading hyphen", "-pump", QValidator::Invalid},
             {"trailing hyphen", "pump-", QValidator::Intermediate},
             {"w/ under", "pump_one", QValidator::Acceptable},
             {"w/ leading under", "_pump", QValidator::Invalid},
             {"w/ trailing under", "pump_", QValidator::Acceptable},
             {"w/ number", "pump1", QValidator::Acceptable},
             {"w/ leading number", "1pump", QValidator::Invalid},
             {"w/ leading !", "!pump1", QValidator::Invalid},
             {"w/ leading ~", "~pump1", QValidator::Invalid},
             {"w/ trailing ~", "pump1~", QValidator::Invalid},
             {"w/ ~ separator", "pump~one", QValidator::Invalid},
             {"w/ double hyphen", "pump--one", QValidator::Invalid},
             {"w/ dunder", "pump__one", QValidator::Acceptable},
             {"hyphen/under", "pump-_one", QValidator::Acceptable},
             {"under/hyphen", "pump_-one", QValidator::Acceptable},
             {"dunder/dunder", "__pump__", QValidator::Invalid},
             {"end dunder", "pump__", QValidator::Acceptable},
             {"w/ hyphen num", "pump-1", QValidator::Acceptable},
             {"w/ under num", "pump_1", QValidator::Acceptable},
             {"capital", "PUMP", QValidator::Acceptable},
             {"lower", "pump", QValidator::Acceptable},
             {"mixed", "PumpOne", QValidator::Acceptable},
             {"camel", "pumpOne", QValidator::Acceptable},
             {"non-English", u8"Помпа", QValidator::Acceptable}});
    }
    void testName() { test(scram::gui::Validator::name()); }

    void testPercent_data()
    {
        populateData<QString, QValidator::State>(
            {"value", "state"},
            {{"valid", "5", QValidator::Acceptable},
             {"zero", "0", QValidator::Invalid},
             {"negative", "-1", QValidator::Invalid},
             {"one", "1", QValidator::Acceptable},
             {"hundred", "100", QValidator::Acceptable},
             {"large", "1010", QValidator::Acceptable},
             {"positive exponent", "1e6", QValidator::Invalid},
             {"negative exponent", "1e-6", QValidator::Invalid},
             {"double", "0.1", QValidator::Invalid},
             {"bare dot", ".1", QValidator::Invalid},
             {"string", "one", QValidator::Invalid},
             {"expression", "4*2", QValidator::Invalid},
             {"whitespace", " 10", QValidator::Invalid},
             {"percent sign", "1%", QValidator::Acceptable},
             {"bare percent", "%", QValidator::Invalid},
             {"space percent", "1 %", QValidator::Invalid},
             {"leading percent", "%1", QValidator::Invalid}});
    }
    void testPercent() { test(scram::gui::Validator::percent()); }

    void testProbability_data()
    {
        populateData<QString, QValidator::State>(
            {"value", "state"},
            {{"empty", "", QValidator::Intermediate},
             {"valid", "0.5", QValidator::Acceptable},
             {"zero", "0", QValidator::Acceptable},
             {"negative", "-1", QValidator::Invalid},
             {"one", "1", QValidator::Acceptable},
             {"outside range", "1.1", QValidator::Intermediate},
             {"positive exponent", "1e6", QValidator::Intermediate},
             {"negative exponent", "1e-6", QValidator::Acceptable},
             {"bare exponent", "e-6", QValidator::Intermediate},
             {"bare dot", ".1", QValidator::Acceptable},
             {"bare dot exponent", ".1e-6", QValidator::Acceptable},
             {"string", "one", QValidator::Invalid},
             {"expression", "0.4*2", QValidator::Invalid},
             {"whitespace", " 10", QValidator::Invalid}});
    }
    void testProbability() { test(scram::gui::Validator::probability()); }

    void testNonNegative_data()
    {
        populateData<QString, QValidator::State>(
            {"value", "state"},
            {{"empty", "", QValidator::Intermediate},
             {"zero", "0", QValidator::Acceptable},
             {"negative", "-1", QValidator::Invalid},
             {"double", "1.2", QValidator::Acceptable},
             {"positive exponent", "1e6", QValidator::Acceptable},
             {"negative exponent", "1e-6", QValidator::Acceptable},
             {"bare exponent", "e-6", QValidator::Intermediate},
             {"bare dot", ".1", QValidator::Acceptable},
             {"bare dot exponent", ".1e6", QValidator::Acceptable},
             {"string", "one", QValidator::Invalid},
             {"expression", "4*2", QValidator::Invalid},
             {"whitespace", " 10", QValidator::Invalid}});
    }
    void testNonNegative() { test(scram::gui::Validator::nonNegative()); }

private:
    /// Runs the given validator with test data.
    void test(const QValidator *validator)
    {
        QVERIFY2(validator, "Uninitialized validator.");

        QFETCH(QString, value);
        QFETCH(QValidator::State, state);

        int pos = 0;
        QCOMPARE(validator->validate(value, pos), state);
    }
};

QTEST_APPLESS_MAIN(TestValidator)

#include "testvalidator.moc"

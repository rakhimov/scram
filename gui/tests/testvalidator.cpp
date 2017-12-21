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

Q_DECLARE_METATYPE(QValidator::State)

class TestValidator : public QObject
{
    Q_OBJECT

private slots:
    void testName_data()
    {
        QTest::addColumn<QString>("value");
        QTest::addColumn<QValidator::State>("state");

#if QT_VERSION >= 0x050500 ///< @todo Upgrade Qt.
        QTest::newRow("empty") << "" << QValidator::Intermediate;
#endif
        QTest::newRow("letter") << "a" << QValidator::Acceptable;
        QTest::newRow("number") << "1" << QValidator::Invalid;
        QTest::newRow("hyphen") << "-" << QValidator::Invalid;
        QTest::newRow("underscore") << "_" << QValidator::Invalid;
        QTest::newRow("space") << " " << QValidator::Invalid;
        QTest::newRow("dot") << "." << QValidator::Invalid;
        QTest::newRow("w/ space") << "pump one" << QValidator::Invalid;
        QTest::newRow("w/ tab") << "pump\tone" << QValidator::Invalid;
        QTest::newRow("w/ dot") << "pump.one" << QValidator::Invalid;
        QTest::newRow("w/ colon") << "pump:one" << QValidator::Invalid;
        QTest::newRow("w/ hyphen") << "pump-one" << QValidator::Acceptable;
        QTest::newRow("w/ leading hyphen") << "-pump" << QValidator::Invalid;
        QTest::newRow("trailing hyphen") << "pump-" << QValidator::Intermediate;
        QTest::newRow("w/ under") << "pump_one" << QValidator::Acceptable;
        QTest::newRow("w/ leading under") << "_pump" << QValidator::Invalid;
        QTest::newRow("w/ trailing under") << "pump_" << QValidator::Acceptable;
        QTest::newRow("w/ number") << "pump1" << QValidator::Acceptable;
        QTest::newRow("w/ leading number") << "1pump" << QValidator::Invalid;
        QTest::newRow("w/ leading !") << "!pump1" << QValidator::Invalid;
        QTest::newRow("w/ leading ~") << "~pump1" << QValidator::Invalid;
        QTest::newRow("w/ trailing ~") << "pump1~" << QValidator::Invalid;
        QTest::newRow("w/ ~ separator") << "pump~one" << QValidator::Invalid;
        QTest::newRow("w/ double hyphen") << "pump--one" << QValidator::Invalid;
        QTest::newRow("w/ dunder") << "pump__one" << QValidator::Acceptable;
        QTest::newRow("hyphen/under") << "pump-_one" << QValidator::Acceptable;
        QTest::newRow("under/hyphen") << "pump_-one" << QValidator::Acceptable;
        QTest::newRow("dunder/dunder") << "__pump__" << QValidator::Invalid;
        QTest::newRow("end dunder") << "pump__" << QValidator::Acceptable;
        QTest::newRow("w/ hyphen num") << "pump-1" << QValidator::Acceptable;
        QTest::newRow("w/ under num") << "pump_1" << QValidator::Acceptable;
        QTest::newRow("capital") << "PUMP" << QValidator::Acceptable;
        QTest::newRow("lower") << "pump" << QValidator::Acceptable;
        QTest::newRow("mixed") << "PumpOne" << QValidator::Acceptable;
        QTest::newRow("camel") << "pumpOne" << QValidator::Acceptable;
        QTest::newRow("non-English") << u8"Помпа" << QValidator::Acceptable;
    }
    void testName() { test(scram::gui::Validator::name()); }

    void testPercent_data()
    {
        QTest::addColumn<QString>("value");
        QTest::addColumn<QValidator::State>("state");

#if QT_VERSION >= 0x050500 ///< @todo Upgrade Qt.
        QTest::newRow("empty") << "" << QValidator::Intermediate;
#endif
        QTest::newRow("valid") << "5" << QValidator::Acceptable;
        QTest::newRow("zero") << "0" << QValidator::Invalid;
        QTest::newRow("negative") << "-1" << QValidator::Invalid;
        QTest::newRow("one") << "1" << QValidator::Acceptable;
        QTest::newRow("hundred") << "100" << QValidator::Acceptable;
        QTest::newRow("large") << "1010" << QValidator::Acceptable;
        QTest::newRow("positive exponent") << "1e6" << QValidator::Invalid;
        QTest::newRow("negative exponent") << "1e-6" << QValidator::Invalid;
        QTest::newRow("double") << "0.1" << QValidator::Invalid;
        QTest::newRow("bare dot") << ".1" << QValidator::Invalid;
        QTest::newRow("string") << "one" << QValidator::Invalid;
        QTest::newRow("expression") << "4*2" << QValidator::Invalid;
        QTest::newRow("whitespace") << " 10" << QValidator::Invalid;
        QTest::newRow("percent sign") << "1%" << QValidator::Acceptable;
        QTest::newRow("bare percent") << "%" << QValidator::Invalid;
        QTest::newRow("space percent") << "1 %" << QValidator::Invalid;
        QTest::newRow("leading percent") << "%1" << QValidator::Invalid;
    }
    void testPercent() { test(scram::gui::Validator::percent()); }

    void testProbability_data()
    {
        QTest::addColumn<QString>("value");
        QTest::addColumn<QValidator::State>("state");

        QTest::newRow("empty") << "" << QValidator::Intermediate;
        QTest::newRow("valid") << "0.5" << QValidator::Acceptable;
        QTest::newRow("zero") << "0" << QValidator::Acceptable;
        QTest::newRow("negative") << "-1" << QValidator::Invalid;
        QTest::newRow("one") << "1" << QValidator::Acceptable;
        QTest::newRow("outside range") << "1.1" << QValidator::Intermediate;
        QTest::newRow("positive exponent") << "1e6" << QValidator::Intermediate;
        QTest::newRow("negative exponent") << "1e-6" << QValidator::Acceptable;
        QTest::newRow("bare exponent") << "e-6" << QValidator::Intermediate;
        QTest::newRow("bare dot") << ".1" << QValidator::Acceptable;
        QTest::newRow("bare dot exponent") << ".1e-6" << QValidator::Acceptable;
        QTest::newRow("string") << "one" << QValidator::Invalid;
        QTest::newRow("expression") << "0.4*2" << QValidator::Invalid;
        QTest::newRow("whitespace") << " 10" << QValidator::Invalid;
    }
    void testProbability() { test(scram::gui::Validator::probability()); }

    void testNonNegative_data()
    {
        QTest::addColumn<QString>("value");
        QTest::addColumn<QValidator::State>("state");

        QTest::newRow("empty") << "" << QValidator::Intermediate;
        QTest::newRow("zero") << "0" << QValidator::Acceptable;
        QTest::newRow("negative") << "-1" << QValidator::Invalid;
        QTest::newRow("double") << "1.2" << QValidator::Acceptable;
        QTest::newRow("positive exponent") << "1e6" << QValidator::Acceptable;
        QTest::newRow("negative exponent") << "1e-6" << QValidator::Acceptable;
        QTest::newRow("bare exponent") << "e-6" << QValidator::Intermediate;
        QTest::newRow("bare dot") << ".1" << QValidator::Acceptable;
        QTest::newRow("bare dot exponent") << ".1e6" << QValidator::Acceptable;
        QTest::newRow("string") << "one" << QValidator::Invalid;
        QTest::newRow("expression") << "4*2" << QValidator::Invalid;
        QTest::newRow("whitespace") << " 10" << QValidator::Invalid;
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

QTEST_MAIN(TestValidator)

#include "testvalidator.moc"

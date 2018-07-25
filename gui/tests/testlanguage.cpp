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

#include "gui/language.h"

#include <boost/range/algorithm.hpp>

#include <QtTest>

#include "data.h"
#include "help.h"

class TestLanguage : public QObject
{
    Q_OBJECT

private slots:
    void testTranslations();
    void testNativeName_data();
    void testNativeName();
};

void TestLanguage::testTranslations()
{
    try {
        std::vector<std::string> translations = scram::gui::translations();
        boost::sort(translations);
        decltype(translations) expected = {"de_DE", "es_ES", "id_ID",
                                           "it_IT", "nl_NL", "pl_PL",
                                           "ru_RU", "tr_TR", "zh_CN"};
        QCOMPARE(translations, expected);
    } catch (const std::exception &err) {
        qFatal("%s", err.what());
    }
}

void TestLanguage::testNativeName_data()
{
    populateData<QByteArray, QString>({"code", "name"},
                                      {{"French", "fr", u8"Français"},
                                       {"German", "de_DE", "Deutsch"},
                                       {"Italian", "it_IT", "Italiano"},
                                       {"Dutch", "nl_NL", "Nederlands"},
                                       {"Russian", "ru_RU", u8"Русский"},
                                       {"Japanese", "ja_JP", u8"日本語"}});
}

void TestLanguage::testNativeName()
{
    QFETCH(QByteArray, code);
    QFETCH(QString, name);
    QCOMPARE(scram::gui::nativeLanguageName(code.data()), name);
}

QTEST_APPLESS_MAIN(TestLanguage)

#include "testlanguage.moc"

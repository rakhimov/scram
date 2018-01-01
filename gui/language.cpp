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

/// @file

#include "language.h"

#include <cstring>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

#include <QLocale>

#include "src/env.h"

#include "guiassert.h"

namespace scram {
namespace gui {

const std::string &translationsPath()
{
    static const std::string tsPath(scram::Env::install_dir()
                                    + "/share/scram/translations");
    return tsPath;
}

std::vector<std::string> translations()
{
    namespace fs = boost::filesystem;

    std::vector<std::string> result;
    for (fs::directory_iterator it(translationsPath()), it_end; it != it_end;
         ++it) {
        if (!fs::is_regular_file(it->status()))
            continue;

        std::string filename = it->path().filename().string();
        const char suffix[] = ".qm";
        const char prefix[] = "scramgui_";

        if (!boost::starts_with(filename, prefix)
            || !boost::ends_with(filename, suffix))
            continue;

        filename.erase(filename.size() - std::strlen(suffix));
        filename.erase(0, std::strlen(prefix));
        if (filename != "en")
            result.push_back(std::move(filename));
    }
    return result;
}

QString nativeLanguageName(const std::string &locale)
{
    QLocale nativeLocale(QString::fromStdString(locale));
    GUI_ASSERT(nativeLocale != QLocale::c(), {});
    QString language = nativeLocale.nativeLanguageName();
    GUI_ASSERT(!language.isEmpty(), {});
    if (language[0].isLower())
        language[0] = language[0].toUpper();
    return language;
}

} // namespace gui
} // namespace scram

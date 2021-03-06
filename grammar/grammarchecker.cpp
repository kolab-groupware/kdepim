/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "grammarchecker.h"
#include "grammarloader.h"

#include <KGlobal>
#include <KLocale>

#include <QStringList>

namespace Grammar {

class GrammarCheckerPrivate
{
public:
    GrammarCheckerPrivate(GrammarChecker *qq)
        : q(qq)
    {

    }
    GrammarChecker *q;
};


GrammarChecker::GrammarChecker(const QString &lang)
    : d(new GrammarCheckerPrivate(this))
{
}

GrammarChecker::~GrammarChecker()
{
    delete d;
}

QMap<QString, QString> GrammarChecker::availableLanguage() const
{
    GrammarLoader *l = GrammarLoader::openGrammarLoader();
    const QStringList lst = l->languages();
    QMap<QString, QString> langs;
    foreach(const QString &tag, lst) {
        const QString description= QString::fromLatin1("%1 (%2)")
                     .arg(KGlobal::locale()->languageCodeToName(tag))
                     .arg(KGlobal::locale()->countryCodeToName(tag));
        langs.insert(description, tag);
    }

    return langs;
}

}

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


#include "grantleeutil.h"

#include <KSharedConfig>
#include <grantleetheme/grantleethememanager.h>
#include <grantleetheme/globalsettings_base.h>

QString KAddressBookGrantlee::GrantleeUtil::kaddressBookAbsoluteThemePath()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig( QLatin1String("kaddressbookrc") );
    GrantleeTheme::GrantleeSettings::self()->setSharedConfig(config);
    GrantleeTheme::GrantleeSettings::self()->readConfig();
    QString themeName = GrantleeTheme::GrantleeSettings::self()->grantleeThemeName();
    if (themeName.isEmpty()) {
        themeName = QLatin1String("default");
    }
    const QString absolutePath = GrantleeTheme::GrantleeThemeManager::pathFromThemes(QLatin1String("kaddressbook/viewertemplates/"), themeName, QLatin1String( "theme.desktop" ));
    return absolutePath;
}

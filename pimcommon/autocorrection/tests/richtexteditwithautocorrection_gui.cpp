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

#include "pimcommon/autocorrection/widgets/richtexteditwithautocorrection.h"

#include <kdebug.h>
#include <kapplication.h>
#include <KCmdLineArgs>
#include <KLocalizedString>

int main (int argc, char **argv)
{
    KCmdLineArgs::init(argc, argv, "richtexteditwithautocorrection_gui", 0, ki18n("richtexteditwithautocorrectionTest_Gui"),
                       "1.0", ki18n("Test for richtexteditwithautocorrection"));

    KApplication app;

    PimCommon::RichTextEditWithAutoCorrection *w = new PimCommon::RichTextEditWithAutoCorrection;

    w->show();
    app.exec();
    delete w;
    return 0;
}


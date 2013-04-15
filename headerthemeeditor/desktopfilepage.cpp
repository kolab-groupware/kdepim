/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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


#include "desktopfilepage.h"
#include "pimcommon/simplestringlisteditor.h"

#include <KLineEdit>
#include <KLocale>
#include <KDesktopFile>
#include <KConfigGroup>

#include <QGridLayout>
#include <QLabel>

DesktopFilePage::DesktopFilePage(QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *lay = new QGridLayout;
    QLabel *lab = new QLabel(i18n("Name:"));
    mName = new KLineEdit;
    mName->setReadOnly(true);
    lay->addWidget(lab,0,0);
    lay->addWidget(mName,0,1);

    lab = new QLabel(i18n("Description:"));
    mDescription = new KLineEdit;
    lay->addWidget(lab,1,0);
    lay->addWidget(mDescription,1,1);

    lab = new QLabel(i18n("Filename:"));
    mFilename = new KLineEdit;
    lay->addWidget(lab,2,0);
    lay->addWidget(mFilename,2,1);

    lab = new QLabel(i18n("Extract Headers:"));
    lay->addWidget(lab,3,0);

    mExtraDisplayHeaders = new PimCommon::SimpleStringListEditor;
    lay->addWidget(mExtraDisplayHeaders, 4, 0, 4, 2);
    setLayout(lay);
}

DesktopFilePage::~DesktopFilePage()
{
}

void DesktopFilePage::setThemeName(const QString &themeName)
{
    mName->setText(themeName);
}

QString DesktopFilePage::filename() const
{
    return mFilename->text();
}

void DesktopFilePage::saveTheme(const QString &path)
{
    QString filename;
    KDesktopFile desktopFile(filename);
    KConfigGroup grp = desktopFile.actionGroup(QLatin1String("Desktop Entry"));
    grp.writeEntry(QLatin1String("Name"), mName->text());
    grp.writeEntry(QLatin1String("Description"), mDescription->text());
    grp.writeEntry(QLatin1String("FileName"), mFilename->text());
    grp.writeEntry(QLatin1String("DisplayExtraHeaders"), mExtraDisplayHeaders->stringList());
    grp.sync();
}

#include "desktopfilepage.moc"

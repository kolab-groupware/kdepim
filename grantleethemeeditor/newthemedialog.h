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

#ifndef NEWTHEMEDIALOG_H
#define NEWTHEMEDIALOG_H

#include "grantleethemeeditor_export.h"
#include <KDialog>

class KLineEdit;
class KUrlRequester;
namespace GrantleeThemeEditor {
class GRANTLEETHEMEEDITOR_EXPORT NewThemeDialog : public KDialog
{
    Q_OBJECT
public:
    explicit NewThemeDialog(QWidget *parent = 0);
    ~NewThemeDialog();

    QString themeName() const;
    QString directory() const;

private Q_SLOTS:
    void slotUpdateOkButton();

private:
    void readConfig();
    KLineEdit *mThemeName;
    KUrlRequester *mUrlRequester;
};
}
#endif // NEWTHEMEDIALOG_H

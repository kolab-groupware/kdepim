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


#ifndef PREVIEWWIDGET_H
#define PREVIEWWIDGET_H

#include "grantleethemeeditor/previewwidget.h"
class ContactPreviewWidget;
class PreviewWidget : public GrantleeThemeEditor::PreviewWidget
{
    Q_OBJECT
public:
    explicit PreviewWidget(const QString &projectDirectory, QWidget *parent = 0);
    ~PreviewWidget();

    void createScreenShot(const QStringList &fileList);
    void loadConfig();
    void setThemePath(const QString &projectDirectory, const QString &mainPageFileName);
    void updateViewer();

private:
    ContactPreviewWidget *mPreview;
};

#endif // PREVIEWWIDGET_H

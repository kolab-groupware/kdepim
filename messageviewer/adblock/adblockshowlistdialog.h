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

#ifndef ADBLOCKSHOWLISTDIALOG_H
#define ADBLOCKSHOWLISTDIALOG_H

#include <KDialog>

class KJob;
class KTemporaryFile;

namespace KPIMUtils {
class ProgressIndicatorLabel;
}

namespace PimCommon {
class PlainTextEditorWidget;
}

namespace MessageViewer {
class AdBlockShowListDialog : public KDialog
{
    Q_OBJECT
public:
    explicit AdBlockShowListDialog(QWidget *parent = 0);
    ~AdBlockShowListDialog();

    void setAdBlockListPath(const QString &localPath, const QString &url);

private Q_SLOTS:
    void slotFinished(KJob *job);

private:
    void readConfig();
    void writeConfig();
    void downLoadList(const QString &url);

    PimCommon::PlainTextEditorWidget *mTextEdit;
    KTemporaryFile *mTemporaryFile;
    KPIMUtils::ProgressIndicatorLabel *mProgress;
};
}

#endif // ADBLOCKSHOWLISTDIALOG_H

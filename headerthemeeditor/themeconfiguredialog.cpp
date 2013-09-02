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

#include "themeconfiguredialog.h"
#include "themeeditorutil.h"

#include <KLocale>
#include <KUrlRequester>
#include <KConfig>
#include <KGlobal>
#include <KConfigGroup>
#include <KTextEdit>
#include <KLineEdit>

#include <QVBoxLayout>
#include <QLabel>
#include <QTabWidget>

ThemeConfigureDialog::ThemeConfigureDialog(QWidget *parent)
    : KDialog(parent)
{
    setCaption( i18n( "Configure" ) );
    setButtons( Default|Ok|Cancel );
    setButtonFocus( Ok );

    QTabWidget *tab = new QTabWidget;

    QWidget *w = new QWidget;

    QVBoxLayout *lay = new QVBoxLayout;
    w->setLayout(lay);

    QHBoxLayout *hbox = new QHBoxLayout;
    lay->addLayout(hbox);

    QLabel *lab = new QLabel(i18n("Default theme path:"));
    hbox->addWidget(lab);

    mDefaultUrl = new KUrlRequester;
    mDefaultUrl->setMode(KFile::Directory);
    hbox->addWidget(mDefaultUrl);

    hbox = new QHBoxLayout;
    lay->addLayout(hbox);

    lab = new QLabel(i18n("Author email:"));
    hbox->addWidget(lab);
    mAuthorEmail = new KLineEdit;
    hbox->addWidget(mAuthorEmail);

    lab = new QLabel(i18n("Default email:"));
    lay->addWidget(lab);

    mDefaultEmail = new KTextEdit;
    mDefaultEmail->setAcceptRichText(false);
    lay->addWidget(mDefaultEmail);
    tab->addTab(w, i18n("General"));

    mDefaultTemplate = new KTextEdit;
    mDefaultTemplate->setAcceptRichText(false);
    tab->addTab(mDefaultTemplate, i18n("Default Template"));

    setMainWidget(tab);
    connect(this, SIGNAL(defaultClicked()), this, SLOT(slotDefaultClicked()));
    connect(this, SIGNAL(okClicked()), this, SLOT(slotOkClicked()));
    readConfig();
}

ThemeConfigureDialog::~ThemeConfigureDialog()
{
    KSharedConfig::Ptr config = KGlobal::config();

    KConfigGroup group = config->group( QLatin1String("ThemeConfigureDialog") );
    group.writeEntry( "Size", size() );
}

void ThemeConfigureDialog::slotDefaultClicked()
{
    mDefaultUrl->setUrl(KUrl());
    mDefaultEmail->setPlainText(themeeditorutil::defaultMail());
    mAuthorEmail->clear();
    mDefaultTemplate->clear();
}

void ThemeConfigureDialog::slotOkClicked()
{
    writeConfig();
}

void ThemeConfigureDialog::readConfig()
{
    KSharedConfig::Ptr config = KGlobal::config();
    if (config->hasGroup(QLatin1String("Global"))) {
        KConfigGroup group = config->group(QLatin1String("Global"));
        mDefaultUrl->setUrl(group.readEntry("path", KUrl()));
        mDefaultEmail->setPlainText(group.readEntry("defaultEmail",themeeditorutil::defaultMail()));
        mDefaultTemplate->setPlainText(group.readEntry("defaultTemplate",QString()));
        mAuthorEmail->setText(group.readEntry("authorEmail"));
    } else {
        mDefaultEmail->setPlainText(themeeditorutil::defaultMail());
    }

    KConfigGroup group = KConfigGroup( config, "ThemeConfigureDialog" );
    const QSize sizeDialog = group.readEntry( "Size", QSize() );
    if ( sizeDialog.isValid() ) {
        resize( sizeDialog );
    } else {
        resize( 600,400);
    }
}

void ThemeConfigureDialog::writeConfig()
{
    KSharedConfig::Ptr config = KGlobal::config();
    KConfigGroup group = config->group(QLatin1String("Global"));
    group.writeEntry("path", mDefaultUrl->url());
    group.writeEntry("defaultEmail", mDefaultEmail->toPlainText());
    group.writeEntry("defaultTemplate", mDefaultTemplate->toPlainText());
    group.writeEntry("authorEmail", mAuthorEmail->text());
}

#include "themeconfiguredialog.moc"

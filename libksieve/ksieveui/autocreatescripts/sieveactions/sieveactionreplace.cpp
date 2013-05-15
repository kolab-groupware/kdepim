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


#include "sieveactionreplace.h"
#include "widgets/multilineedit.h"
#include "autocreatescripts/autocreatescriptutil_p.h"

#include <KLocale>
#include <KLineEdit>

#include <QLabel>
#include <QHBoxLayout>

using namespace KSieveUi;
SieveActionReplace::SieveActionReplace(QObject *parent)
    : SieveAction(QLatin1String("replace"), i18n("Replace"), parent)
{
}

SieveAction* SieveActionReplace::newAction()
{
  return new SieveActionReplace;
}

QWidget *SieveActionReplace::createParamWidget( QWidget *parent ) const
{
    QWidget *w = new QWidget(parent);
    QHBoxLayout *lay = new QHBoxLayout;
    lay->setMargin(0);
    w->setLayout(lay);

    QLabel *lab = new QLabel(i18n("Subject:"));
    lay->addWidget(lab);

    KLineEdit *subject = new KLineEdit;
    subject->setObjectName(QLatin1String("subject"));
    lay->addWidget(subject);

    lab = new QLabel(i18n("from:"));
    lay->addWidget(lab);

    KLineEdit *headers = new KLineEdit;
    headers->setObjectName(QLatin1String("from"));
    lay->addWidget(headers);

    lab = new QLabel(i18n("text:"));
    lay->addWidget(lab);

    MultiLineEdit *text = new MultiLineEdit;
    text->setObjectName(QLatin1String("text"));
    lay->addWidget(text);

    return w;
}

QString SieveActionReplace::code(QWidget *w) const
{
    QString result = QLatin1String("replace ");
    const KLineEdit *subject = w->findChild<KLineEdit*>(QLatin1String("subject"));
    const QString subjectStr = subject->text();
    if (!subjectStr.isEmpty()) {
        result += QString::fromLatin1(":subject \"%1\" ").arg(subjectStr);
    }

    const KLineEdit *headers = w->findChild<KLineEdit*>(QLatin1String("from"));
    const QString headersStr = headers->text();
    if (!headersStr.isEmpty()) {
        result += QString::fromLatin1(":from \"%1\" ").arg(headersStr);
    }


    const MultiLineEdit *edit = w->findChild<MultiLineEdit*>( QLatin1String("text") );
    const QString text = edit->toPlainText();
    if (!text.isEmpty()) {
        result += QString::fromLatin1("text:%1").arg(AutoCreateScriptUtil::createMultiLine(text));
    }

    return result;
}


QStringList SieveActionReplace::needRequires() const
{
    return QStringList() << QLatin1String("replace");
}

bool SieveActionReplace::needCheckIfServerHasCapability() const
{
    return true;
}

QString SieveActionReplace::serverNeedsCapability() const
{
    return QLatin1String("replace");
}


#include "sieveactionreplace.moc"

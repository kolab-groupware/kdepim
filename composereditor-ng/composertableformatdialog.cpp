/*
  Copyright (c) 2012 Montel Laurent <montel@kde.org>

  This library is free software; you can redistribute it and/or modify it
  under the terms of the GNU Library General Public License as published by
  the Free Software Foundation; either version 2 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
  License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to the
  Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

*/

#include "composertableformatdialog.h"

#include <KPIMTextEdit/InsertTableWidget>

#include <KLocale>
#include <KColorButton>

#include <QWebElement>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QDebug>

namespace ComposerEditorNG
{
class ComposerTableFormatDialogPrivate
{
public:
    ComposerTableFormatDialogPrivate(ComposerTableFormatDialog *qq)
        :q(qq)
    {
    }

    void initialize( const QWebElement& element);

    void updateTableHtml();

    QWebElement webElement;
    KColorButton *backgroundColor;
    QCheckBox *useBackgroundColor;
    KPIMTextEdit::InsertTableWidget *insertTableWidget;
    ComposerTableFormatDialog *q;
};

void ComposerTableFormatDialogPrivate::updateTableHtml()
{
    if(!webElement.isNull()) {
        webElement.setAttribute(QLatin1String("border"),QString::number(insertTableWidget->border()));
        const QString width = QString::fromLatin1("%1%2").arg(insertTableWidget->length()).arg(insertTableWidget->typeOfLength() == QTextLength::PercentageLength ? QLatin1String("%") : QString());
        webElement.setAttribute(QLatin1String("width"),width);
        if(useBackgroundColor->isChecked()) {
            const QColor col = backgroundColor->color();
            if(col.isValid()) {
                webElement.setAttribute(QLatin1String("bgcolor"),backgroundColor->color().name());
            }
        } else {
            webElement.removeAttribute(QLatin1String("bgcolor"));
        }
        //TODO update column/row
    }
}

void ComposerTableFormatDialogPrivate::initialize(const QWebElement &element)
{
    webElement = element;
    q->setCaption( i18n( "Table Format" ) );
    q->setButtons( KDialog::Ok|KDialog::Cancel );
    q->setButtonText( KDialog::Ok, i18n( "Edit" ) );
    QWidget *page = new QWidget( q );
    q->setMainWidget( page );

    QVBoxLayout *lay = new QVBoxLayout( page );
    insertTableWidget = new KPIMTextEdit::InsertTableWidget( q );
    lay->addWidget(insertTableWidget);

    QHBoxLayout *hbox = new QHBoxLayout;
    useBackgroundColor = new QCheckBox( i18n( "Background Color:" ) );
    hbox->addWidget(useBackgroundColor);
    backgroundColor = new KColorButton;
    backgroundColor->setEnabled(false);
    hbox->addWidget(backgroundColor);

    lay->addLayout(hbox);

    q->connect(q,SIGNAL(okClicked()),q,SLOT(slotOkClicked()));
    q->connect(useBackgroundColor,SIGNAL(toggled(bool)),backgroundColor,SLOT(setEnabled(bool)));
    if(!webElement.isNull()) {
        if(webElement.hasAttribute(QLatin1String("border"))) {
            insertTableWidget->setBorder(webElement.attribute(QLatin1String("border")).toInt());
        }
        if(webElement.hasAttribute(QLatin1String("width"))) {
            QString width = webElement.attribute(QLatin1String("width"));
            if(width.endsWith(QLatin1Char('%'))) {
                insertTableWidget->setTypeOfLength(QTextLength::PercentageLength);
                width.chop(1);
                insertTableWidget->setLength(width.toInt());
            } else {
                insertTableWidget->setTypeOfLength(QTextLength::FixedLength);
                insertTableWidget->setLength(width.toInt());
            }
        }
        if(webElement.hasAttribute(QLatin1String("bgcolor"))) {
            useBackgroundColor->setChecked(true);
            const QColor color = QColor(webElement.attribute(QLatin1String("bgcolor")));
            backgroundColor->setColor(color);
        }
        QWebElementCollection allRows = webElement.findAll(QLatin1String("tr"));
        insertTableWidget->setRows(allRows.count());
        QWebElementCollection allCol = webElement.findAll(QLatin1String("td"));
        insertTableWidget->setColumns(allCol.count()/allRows.count());
    }
}

ComposerTableFormatDialog::ComposerTableFormatDialog(const QWebElement& element, QWidget *parent)
    : KDialog(parent), d(new ComposerTableFormatDialogPrivate(this))
{
    d->initialize(element);
}

ComposerTableFormatDialog::~ComposerTableFormatDialog()
{
    delete d;
}


void ComposerTableFormatDialog::slotOkClicked()
{
    if(!d->webElement.isNull()) {
        d->updateTableHtml();
    }
    accept();
}

}

#include "composertableformatdialog.moc"

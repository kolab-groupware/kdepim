/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

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


#include "quicksearchlinetest.h"
#include "messagelist/core/quicksearchline.h"
#include <qtest_kde.h>
#include <qtestkeyboard.h>
#include <qtestmouse.h>
#include <KLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <KPushButton>
#include <KComboBox>


using namespace MessageList::Core;
QuickSearchLineTest::QuickSearchLineTest()
{
}

void QuickSearchLineTest::shouldHaveDefaultValueOnCreation()
{
    QuickSearchLine searchLine;
    QVERIFY(searchLine.searchEdit()->text().isEmpty());
    QVERIFY(!searchLine.lockSearch()->isChecked());
    QWidget *widget = qFindChild<QWidget *>(&searchLine, QLatin1String("extraoptions"));
    QVERIFY(widget);
    QVERIFY(widget->isHidden());
    KPushButton *moreButton = qFindChild<KPushButton *>(&searchLine, QLatin1String("moreoptions"));
    QVERIFY(moreButton);
    QCOMPARE(moreButton->icon().name(), QLatin1String("arrow-down-double"));
    QWidget *quickSearchFilterWidget = qFindChild<QWidget *>(&searchLine, QLatin1String("quicksearchfilterwidget"));
    QVERIFY(quickSearchFilterWidget);
    QVERIFY(quickSearchFilterWidget->isHidden());
    QCOMPARE(searchLine.containsOutboundMessages(), false);
}

void QuickSearchLineTest::shouldEmitTextChanged()
{
    QuickSearchLine searchLine;
    QSignalSpy spy(&searchLine, SIGNAL(searchEditTextEdited(QString)));
    QTest::keyClick(searchLine.searchEdit(), 'F');
    QCOMPARE(spy.count(),1);

    QSignalSpy spy2(&searchLine, SIGNAL(searchEditTextEdited(QString)));
    QTest::keyClicks(searchLine.searchEdit(), QLatin1String("FOO"));
    QCOMPARE(spy2.count(), 3);
}

void QuickSearchLineTest::shouldShowExtraOptionWidget()
{
    QuickSearchLine searchLine;
    searchLine.show();
    QTest::keyClick(searchLine.searchEdit(), 'F');
    QTest::qWaitForWindowShown(&searchLine);
    QWidget *widget = qFindChild<QWidget *>(&searchLine, QLatin1String("extraoptions"));
    QVERIFY(widget->isVisible());
}

void QuickSearchLineTest::shouldHideExtraOptionWidgetWhenClearLineEdit()
{
    QuickSearchLine searchLine;
    searchLine.show();
    QTest::keyClick(searchLine.searchEdit(), 'F');
    QTest::qWaitForWindowShown(&searchLine);
    QWidget *widget = qFindChild<QWidget *>(&searchLine, QLatin1String("extraoptions"));

    searchLine.searchEdit()->clear();
    QVERIFY(!widget->isVisible());
}

void QuickSearchLineTest::shouldHideExtraOptionWidgetWhenResetFilter()
{
    QuickSearchLine searchLine;
    searchLine.show();
    QTest::keyClick(searchLine.searchEdit(), 'F');
    QTest::qWaitForWindowShown(&searchLine);
    QWidget *widget = qFindChild<QWidget *>(&searchLine, QLatin1String("extraoptions"));

    searchLine.resetFilter();
    QVERIFY(!widget->isVisible());
}

void QuickSearchLineTest::shouldEmitSearchOptionChanged()
{
    QuickSearchLine searchLine;
    searchLine.show();
    QSignalSpy spy(&searchLine, SIGNAL(searchOptionChanged()));
    QPushButton *button = qFindChild<QPushButton *>(&searchLine, QLatin1String("subject"));
    QTest::mouseClick(button, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
}

void QuickSearchLineTest::shouldEmitSearchOptionChangedWhenUseTabPress()
{
    QuickSearchLine searchLine;
    searchLine.show();
    QTest::qWaitForWindowShown(&searchLine);
    QPushButton *button = qFindChild<QPushButton *>(&searchLine, QLatin1String("full_message"));
    QTest::mouseClick(button, Qt::LeftButton);
    QTest::keyClick(button,Qt::Key_Right);
    QSignalSpy spy(&searchLine, SIGNAL(searchOptionChanged()));
    button = qFindChild<QPushButton *>(&searchLine, QLatin1String("body"));
    QTest::mouseClick(button, Qt::LeftButton);
    QCOMPARE(spy.count(), 1);
}

void QuickSearchLineTest::shouldResetAllWhenResetFilter()
{
    QuickSearchLine searchLine;
    searchLine.show();
    searchLine.resetFilter();
    QCOMPARE(searchLine.status().count(), 0);
    QCOMPARE(searchLine.lockSearch()->isChecked(), false);
    QCOMPARE(searchLine.tagFilterComboBox()->currentIndex(), -1);
    QuickSearchLine::SearchOptions options;
    options = QuickSearchLine::SearchEveryWhere;
    QCOMPARE(searchLine.searchOptions(), options);
}

void QuickSearchLineTest::shouldShowTagComboBox()
{
    QuickSearchLine searchLine;
    searchLine.show();
    QTest::qWaitForWindowShown(&searchLine);
    QCOMPARE(searchLine.tagFilterComboBox()->isVisible(), false);
    searchLine.tagFilterComboBox()->addItems(QStringList()<<QLatin1String("1")<<QLatin1String("2"));
    searchLine.updateComboboxVisibility();
    QCOMPARE(searchLine.tagFilterComboBox()->isVisible(), true);
}

void QuickSearchLineTest::shouldResetComboboxWhenResetFilter()
{
    QuickSearchLine searchLine;
    searchLine.show();
    QTest::qWaitForWindowShown(&searchLine);
    QCOMPARE(searchLine.tagFilterComboBox()->isVisible(), false);
    searchLine.tagFilterComboBox()->addItems(QStringList()<<QLatin1String("1")<<QLatin1String("2"));
    searchLine.updateComboboxVisibility();
    QCOMPARE(searchLine.tagFilterComboBox()->isVisible(), true);
    searchLine.tagFilterComboBox()->setCurrentIndex(1);
    QCOMPARE(searchLine.tagFilterComboBox()->currentIndex(), 1);
    searchLine.resetFilter();
    QCOMPARE(searchLine.tagFilterComboBox()->currentIndex(), 0);
}

void QuickSearchLineTest::shouldNotEmitTextChangedWhenTextTrimmedIsEmpty()
{
    QuickSearchLine searchLine;
    QSignalSpy spy(&searchLine, SIGNAL(searchEditTextEdited(QString)));
    QTest::keyClicks(searchLine.searchEdit(), QLatin1String("      "));
    QCOMPARE(spy.count(),0);

    QSignalSpy spy2(&searchLine, SIGNAL(searchEditTextEdited(QString)));
    QTest::keyClicks(searchLine.searchEdit(), QLatin1String(" FOO"));
    QCOMPARE(spy2.count(), 3);
}

void QuickSearchLineTest::shouldShowExtraOptionWidgetWhenTextTrimmedIsNotEmpty()
{
    QuickSearchLine searchLine;
    searchLine.show();
    QTest::keyClick(searchLine.searchEdit(), ' ');
    QTest::qWaitForWindowShown(&searchLine);
    QWidget *widget = qFindChild<QWidget *>(&searchLine, QLatin1String("extraoptions"));
    QVERIFY(!widget->isVisible());
    QTest::keyClick(searchLine.searchEdit(), ' ');
    QVERIFY(!widget->isVisible());
    QTest::keyClick(searchLine.searchEdit(), 'F');
    QVERIFY(widget->isVisible());

}

void QuickSearchLineTest::shouldShowMoreOptionWhenClickOnMoreButton()
{
    QuickSearchLine searchLine;
    searchLine.show();
    QTest::qWaitForWindowShown(&searchLine);
    KPushButton *moreButton = qFindChild<KPushButton *>(&searchLine, QLatin1String("moreoptions"));
    QTest::mouseClick(moreButton, Qt::LeftButton);
    QWidget *quickSearchFilterWidget = qFindChild<QWidget *>(&searchLine, QLatin1String("quicksearchfilterwidget"));
    QVERIFY(quickSearchFilterWidget->isVisible());
    QCOMPARE(moreButton->icon().name(), QLatin1String("arrow-up-double"));

    QTest::mouseClick(moreButton, Qt::LeftButton);
    QVERIFY(!quickSearchFilterWidget->isVisible());
    QCOMPARE(moreButton->icon().name(), QLatin1String("arrow-down-double"));
}

void QuickSearchLineTest::shouldChangeFromButtonLabelWhenChangeOutboundMessagesValue()
{
    QuickSearchLine searchLine;
    QPushButton *button = qFindChild<QPushButton *>(&searchLine, QLatin1String("fromorto"));
    const QString buttonName = button->text();
    searchLine.setContainsOutboundMessages(true);
    QVERIFY(button->text()!=buttonName);
    searchLine.setContainsOutboundMessages(false);
    QCOMPARE(button->text(), buttonName);
}

void QuickSearchLineTest::shouldSearchToOrFrom()
{
    QuickSearchLine searchLine;
    QPushButton *button = qFindChild<QPushButton *>(&searchLine, QLatin1String("fromorto"));
    QTest::mouseClick(button, Qt::LeftButton);
    searchLine.setContainsOutboundMessages(true);
    QuickSearchLine::SearchOptions options;
    options = QuickSearchLine::SearchAgainstTo;
    QCOMPARE(searchLine.searchOptions(), options);

    searchLine.setContainsOutboundMessages(false);
    options = QuickSearchLine::SearchAgainstFrom;
    QCOMPARE(searchLine.searchOptions(), options);
}

QTEST_KDEMAIN( QuickSearchLineTest, GUI )

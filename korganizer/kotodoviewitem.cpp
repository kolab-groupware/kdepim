/*
    This file is part of KOrganizer.
    Copyright (c) 2000,2001 Cornelius Schumacher <schumacher@kde.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include <klocale.h>
#include <kdebug.h>

#include "kotodoviewitem.h"
#include "kotodoview.h"
#include "koprefs.h"

KOTodoViewItem::KOTodoViewItem( QListView *parent, Todo *todo, KOTodoView *kotodo)
  : QCheckListItem( parent , "", CheckBox ), mTodo( todo ), mTodoView( kotodo )
{
  construct();
}

KOTodoViewItem::KOTodoViewItem( KOTodoViewItem *parent, Todo *todo, KOTodoView *kotodo )
  : QCheckListItem( parent, "", CheckBox ), mTodo( todo ), mTodoView( kotodo )
{
  construct();
}

QString KOTodoViewItem::key(int column,bool) const
{
  QMap<int,QString>::ConstIterator it = mKeyMap.find(column);
  if (it == mKeyMap.end()) {
    return text(column);
  } else {
    return *it;
  }
}

void KOTodoViewItem::setSortKey(int column,const QString &key)
{
  mKeyMap.insert(column,key);
}

#if QT_VERSION >= 300
void KOTodoViewItem::paintBranches(QPainter *p,const QColorGroup & cg,int w,
                                   int y,int h)
{
  QListViewItem::paintBranches(p,cg,w,y,h);
}
#else
#endif

void KOTodoViewItem::construct()
{
  m_init = true;
  QString keyd = "==";
  QString keyt = "==";

  setOn(mTodo->isCompleted());
  setText(0,mTodo->summary());
  setText(1,QString::number(mTodo->priority()));
  setPixmap(2, progressImg(mTodo->percentComplete()));
  if (mTodo->percentComplete()<100) {
    if (mTodo->isCompleted()) setSortKey(2,QString::number(999));
    else setSortKey(2,QString::number(mTodo->percentComplete()));
  }
  else {
    if (mTodo->isCompleted()) setSortKey(2,QString::number(999));
    else setSortKey(2,QString::number(99));
  }
  if (mTodo->hasDueDate()) {
    setText(3, mTodo->dtDueDateStr());
    QDate d = mTodo->dtDue().date();
    keyd.sprintf("%04d%02d%02d",d.year(),d.month(),d.day());
    setSortKey(3,keyd);
    if (mTodo->doesFloat()) {
      setText(4,"");
    }
    else {
      setText(4,mTodo->dtDueTimeStr());
      QTime t = mTodo->dtDue().time();
      keyt.sprintf("%02d%02d",t.hour(),t.minute());
      setSortKey(4,keyt);
    }
  } else {
    setText(3,"");
    setText(4,"");
  }
  setSortKey(3,keyd);
  setSortKey(4,keyt);

  QString priorityKey = QString::number( mTodo->priority() ) + keyd + keyt;
  if ( mTodo->isCompleted() ) setSortKey( 1, "1" + priorityKey );
  else setSortKey( 1, "0" + priorityKey );

  setText(5,mTodo->categoriesStr());

#if 0
  // Find sort id in description. It's the text behind the last '#' character
  // found in the description. White spaces are removed from beginning and end
  // of sort id.
  int pos = mTodo->description().findRev('#');
  if (pos < 0) {
    setText(6,"");
  } else {
    QString str = mTodo->description().mid(pos+1);
    str.stripWhiteSpace();
    setText(6,str);
  }
#endif

  m_known = false;
  m_init = false;
}

void KOTodoViewItem::stateChange(bool state)
{
  // do not change setting on startup
  if ( m_init ) return;

  kdDebug(5850) << "State changed, modified " << state << endl;
  QString keyd = "==";
  QString keyt = "==";

  Todo*oldTodo = mTodo->clone();

  if (state) mTodo->setCompleted(state);
  else mTodo->setPercentComplete(0);
  if (isOn()!=state) {
    setOn(state);
  }

  if (mTodo->hasDueDate()) {
    setText(3, mTodo->dtDueDateStr());
    QDate d = mTodo->dtDue().date();
    keyd.sprintf("%04d%02d%02d",d.year(),d.month(),d.day());
    setSortKey(3,keyd);
    if (mTodo->doesFloat()) {
      setText(4,"");
    }
    else {
      setText(4,mTodo->dtDueTimeStr());
      QTime t = mTodo->dtDue().time();
      keyt.sprintf("%02d%02d",t.hour(),t.minute());
      setSortKey(4,keyt);
    }
  }

  QString priorityKey = QString::number( mTodo->priority() ) + keyd + keyt;
  if ( mTodo->isCompleted() ) setSortKey( 1, "1" + priorityKey );
  else setSortKey( 1, "0" + priorityKey );

  setPixmap(2, progressImg(mTodo->percentComplete()));
  if (mTodo->percentComplete()<100) {
    if (mTodo->isCompleted()) setSortKey(2,QString::number(999));
    else setSortKey(2,QString::number(mTodo->percentComplete()));
  }
  else {
    if (mTodo->isCompleted()) setSortKey(2,QString::number(999));
    else setSortKey(2,QString::number(99));
  }
  QListViewItem *myChild = firstChild();
  KOTodoViewItem *item;
  while( myChild ) {
    item = static_cast<KOTodoViewItem*>(myChild);
    item->stateChange(state);
    myChild = myChild->nextSibling();
  }
  mTodoView->modified(true);
  mTodoView->setTodoModified( oldTodo, mTodo );
  delete oldTodo;
}

QPixmap KOTodoViewItem::progressImg(int progress)
{
  QImage img(64, 11, 32, 16);
  QPixmap progr;
  int x, y;

  /* White Background */
  img.fill(KGlobalSettings::baseColor().rgb());


  /* Check wether progress is in valid range */
  if(progress > 100) progress = 100;
  else if (progress < 0) progress=0;

  /* Calculating the number of pixels to fill */
  progress=(int) (((float)progress)/100 * 62 + 0.5);

  /* Drawing the border */
  for(x = 0; x < 64; x++) {
    img.setPixel(x, 0, KGlobalSettings::textColor().rgb());
    img.setPixel(x, 10, KGlobalSettings::textColor().rgb());
  }

  for(y = 0; y < 11; y++) {
    img.setPixel(0, y, KGlobalSettings::textColor().rgb());
    img.setPixel(63, y, KGlobalSettings::textColor().rgb());
  }

  /* Drawing the progress */
  for(y = 1; y <= 9; ++y)
    for(x = 1; x <= progress; ++x)
      img.setPixel(x, y, KGlobalSettings::highlightColor().rgb());

  /* Converting to a pixmap */
  progr.convertFromImage(img);

  return progr;
}

bool KOTodoViewItem::isAlternate()
{
#ifndef KORG_NOLVALTERNATION
  KOTodoListView *lv = static_cast<KOTodoListView *>(listView());
  if (lv && lv->alternateBackground().isValid())
  {
    KOTodoViewItem *above = 0;
    above = dynamic_cast<KOTodoViewItem *>(itemAbove());
    m_known = above ? above->m_known : true;
    if (m_known)
    {
       m_odd = above ? !above->m_odd : false;
    }
    else
    {
       KOTodoViewItem *item;
       bool previous = true;
       if (QListViewItem::parent())
       {
          item = dynamic_cast<KOTodoViewItem *>(QListViewItem::parent());
          if (item)
             previous = item->m_odd;
          item = dynamic_cast<KOTodoViewItem *>(QListViewItem::parent()->firstChild());
       }
       else
       {
          item = dynamic_cast<KOTodoViewItem *>(lv->firstChild());
       }

       while(item)
       {
          item->m_odd = previous = !previous;
          item->m_known = true;
          item = dynamic_cast<KOTodoViewItem *>(item->nextSibling());
       }
    }
    return m_odd;
  }
  return false;
#else
  return false;
#endif
}

void KOTodoViewItem::paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment)
{
  QColorGroup _cg = cg;
#ifndef KORG_NOLVALTERNATION
  if (isAlternate())
        _cg.setColor(QColorGroup::Base, static_cast< KOTodoListView* >(listView())->alternateBackground());
  if (mTodo->hasDueDate()) {
    if (mTodo->dtDue().date()==QDate::currentDate() &&
        !mTodo->isCompleted()) {
      _cg.setColor(QColorGroup::Base, KOPrefs::instance()->mTodoDueTodayColor);
    }
    if (mTodo->dtDue().date() < QDate::currentDate() &&
        !mTodo->isCompleted()) {
      _cg.setColor(QColorGroup::Base, KOPrefs::instance()->mTodoOverdueColor);
    }
  }
#endif

  QCheckListItem::paintCell(p, _cg, column, width, alignment);
}

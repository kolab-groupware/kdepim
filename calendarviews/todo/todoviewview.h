/*
  This file is part of KOrganizer.

  Copyright (c) 2008 Thomas Thrainer <tom_t@gmx.at>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

  As a special exception, permission is given to link this program
  with any edition of Qt, and distribute the resulting executable,
  without including the source code for Qt in the source distribution.
*/

#ifndef CALENDARVIEWS_TODOVIEWVIEW_H
#define CALENDARVIEWS_TODOVIEWVIEW_H

#include <QTreeView>
#include <QTimer>

class KMenu;

class TodoViewView : public QTreeView
{
  Q_OBJECT

  public:
    explicit TodoViewView( QWidget *parent = 0 );

    bool isEditing( const QModelIndex &index ) const;

    virtual bool eventFilter( QObject *watched, QEvent *event );

  protected:
    virtual QModelIndex moveCursor( CursorAction cursorAction, Qt::KeyboardModifiers modifiers );
    virtual void mousePressEvent( QMouseEvent * );
    virtual void mouseReleaseEvent( QMouseEvent * );
    virtual void mouseMoveEvent( QMouseEvent * );

  private:
    QModelIndex getNextEditableIndex( const QModelIndex &cur, int inc );

    KMenu *mHeaderPopup;
    QList<QAction *> mColumnActions;
    QTimer mExpandTimer;
    bool mIgnoreNextMouseRelease;

  signals:
    void visibleColumnCountChanged();

  private slots:
    void toggleColumnHidden( QAction *action );
    void expandParent();
};

#endif

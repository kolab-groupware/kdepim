/*
    This file is part of KOrganizer.
    Copyright (c) 2001 Cornelius Schumacher <schumacher@kde.org>
    Copyright (C) 2003-2004 Reinhold Kainhofer <reinhold@kainhofer.com>

    Marcus Bains line.
    Copyright (c) 2001 Ali Rahimi

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.

    As a special exception, permission is given to link this program
    with any edition of Qt, and distribute the resulting executable,
    without including the source code for Qt in the source distribution.
*/
#include <assert.h>

#include <q3intdict.h>
#include <qdatetime.h>
#include <qapplication.h>
#include <q3popupmenu.h>
#include <qcursor.h>
#include <qpainter.h>
#include <qlabel.h>
//Added by qt3to4:
#include <QWheelEvent>
#include <QPixmap>
#include <QVector>
#include <QList>
#include <QEvent>
#include <QKeyEvent>
#include <QFrame>
#include <QDropEvent>
#include <QResizeEvent>
#include <QMouseEvent>

#include <kdebug.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <kmessagebox.h>

#include "koagendaitem.h"
#include "koprefs.h"
#include "koglobals.h"
#include "komessagebox.h"
#include "incidencechanger.h"
#include "kohelper.h"

#include "koagenda.h"
#include "koagenda.moc"
#include <korganizer/baseview.h>

#include <libkcal/event.h>
#include <libkcal/todo.h>
#include <libkcal/dndfactory.h>
#include <libkcal/icaldrag.h>
#include <libkcal/vcaldrag.h>
#include <libkcal/calendar.h>
#include <libkcal/calendarresources.h>
#include <math.h>

////////////////////////////////////////////////////////////////////////////
MarcusBains::MarcusBains( KOAgenda *_agenda )
    : QFrame( _agenda->viewport() ), agenda( _agenda )
{
  setLineWidth(0);
//  setMargin(0);
  QPalette pal;
  pal.setColor( backgroundRole(), Qt::red );
  setPalette( pal );
  minutes = new QTimer(this);
  connect(minutes, SIGNAL(timeout()), this, SLOT(updateLocation()));
  minutes->start(0, true);

  mTimeBox = new QLabel(this);
  mTimeBox->setAlignment(Qt::AlignRight | Qt::AlignBottom);
  QPalette pal1 = mTimeBox->palette();
  pal1.setColor(QColorGroup::Foreground, Qt::red);
  mTimeBox->setPalette(pal1);
#warning "kde4: porting ?"
  //mTimeBox->setAutoMask(true);

  agenda->addChild(mTimeBox);

  oldToday = -1;
}

MarcusBains::~MarcusBains()
{
  delete minutes;
}

int MarcusBains::todayColumn()
{
  QDate currentDate = QDate::currentDate();

  DateList dateList = agenda->dateList();
  DateList::ConstIterator it;
  int col = 0;
  for(it = dateList.begin(); it != dateList.end(); ++it) {
    if((*it) == currentDate)
      return KOGlobals::self()->reverseLayout() ?
             agenda->columns() - 1 - col : col;
      ++col;
  }

  return -1;
}

void MarcusBains::updateLocation(bool recalculate)
{
  QTime tim = QTime::currentTime();
  if((tim.hour() == 0) && (oldTime.hour()==23))
    recalculate = true;

  int mins = tim.hour()*60 + tim.minute();
  int minutesPerCell = 24 * 60 / agenda->rows();
  int y = int( mins * agenda->gridSpacingY() / minutesPerCell );
  int today = recalculate ? todayColumn() : oldToday;
  int x = int( agenda->gridSpacingX() * today );
  bool disabled = !(KOPrefs::instance()->mMarcusBainsEnabled);

  oldTime = tim;
  oldToday = today;

  if(disabled || (today<0)) {
    hide();
    mTimeBox->hide();
    return;
  } else {
    show();
    mTimeBox->show();
  }

  if ( recalculate ) setFixedSize( int( agenda->gridSpacingX() ), 1 );
  agenda->moveChild( this, x, y );
  raise();

  if(recalculate)
    mTimeBox->setFont(KOPrefs::instance()->mMarcusBainsFont);

  mTimeBox->setText(KGlobal::locale()->formatTime(tim, KOPrefs::instance()->mMarcusBainsShowSeconds));
  mTimeBox->adjustSize();
  if (y-mTimeBox->height()>=0) y-=mTimeBox->height(); else y++;
  if (x-mTimeBox->width()+agenda->gridSpacingX() > 0)
    x += int( agenda->gridSpacingX() - mTimeBox->width() - 1 );
  else x++;
  agenda->moveChild(mTimeBox,x,y);
  mTimeBox->raise();
#warning "kde4: porting ?"
  //mTimeBox->setAutoMask(true);

  minutes->start(1000,true);
}


////////////////////////////////////////////////////////////////////////////


/*
  Create an agenda widget with rows rows and columns columns.
*/
KOAgenda::KOAgenda( int columns, int rows, int rowSize, QWidget *parent,
                    Qt::WFlags f )
// TODO_QT4: Use constructor without *name=0 param
  : Q3ScrollView( parent, /*name*/0, f ), mChanger( 0 )
{
  mColumns = columns;
  mRows = rows;
  mGridSpacingY = rowSize;
  mAllDayMode = false;

  init();

  viewport()->setMouseTracking(true);
}

/*
  Create an agenda widget with columns columns and one row. This is used for
  all-day events.
*/
KOAgenda::KOAgenda( int columns, QWidget *parent, Qt::WFlags f )
// TODO_QT4: Use constructor without *name=0 param
  : Q3ScrollView( parent, /*name*/0, f )
{
  mColumns = columns;
  mRows = 1;
  mGridSpacingY = 24;
  mAllDayMode = true;

  init();
}


KOAgenda::~KOAgenda()
{
  delete mMarcusBains;
}


Incidence *KOAgenda::selectedIncidence() const
{
  return ( mSelectedItem ? mSelectedItem->incidence() : 0 );
}


QDate KOAgenda::selectedIncidenceDate() const
{
  return ( mSelectedItem ? mSelectedItem->itemDate() : QDate() );
}

const QString KOAgenda::lastSelectedUid() const
{
  return mSelectedUid;
}


void KOAgenda::init()
{
  mGridSpacingX = 100;

  mResizeBorderWidth = 8;
  mScrollBorderWidth = 8;
  mScrollDelay = 30;
  mScrollOffset = 10;

  enableClipper( true );

  // Grab key strokes for keyboard navigation of agenda. Seems to have no
  // effect. Has to be fixed.
  setFocusPolicy( Qt::WheelFocus );

  connect( &mScrollUpTimer, SIGNAL( timeout() ), SLOT( scrollUp() ) );
  connect( &mScrollDownTimer, SIGNAL( timeout() ), SLOT( scrollDown() ) );

  mStartCell = QPoint( 0, 0 );
  mEndCell = QPoint( 0, 0 );

  mHasSelection = false;
  mSelectionStartPoint = QPoint( 0, 0 );
  mSelectionStartCell = QPoint( 0, 0 );
  mSelectionEndCell = QPoint( 0, 0 );

  mOldLowerScrollValue = -1;
  mOldUpperScrollValue = -1;

  mClickedItem = 0;

  mActionItem = 0;
  mActionType = NOP;
  mItemMoved = false;

  mSelectedItem = 0;
  mSelectedUid.clear();

  setAcceptDrops( true );
  installEventFilter( this );
#warning "make sure we can really remove this"
//   mItems.setAutoDelete( true );
//   mItemsToDelete.setAutoDelete( true );

  resizeContents( int( mGridSpacingX * mColumns ),
                  int( mGridSpacingY * mRows ) );

  viewport()->update();
  viewport()->setBackgroundMode( Qt::NoBackground );
  viewport()->setFocusPolicy( Qt::WheelFocus );

  setMinimumSize( 30, int( mGridSpacingY + 1 ) );
//  setMaximumHeight(mGridSpacingY * mRows + 5);

  // Disable horizontal scrollbar. This is a hack. The geometry should be
  // controlled in a way that the contents horizontally always fits. Then it is
  // not necessary to turn off the scrollbar.
  setHScrollBarMode( AlwaysOff );

  setStartTime( KOPrefs::instance()->mDayBegins.time() );

  calculateWorkingHours();

  connect( verticalScrollBar(), SIGNAL( valueChanged( int ) ),
           SLOT( checkScrollBoundaries( int ) ) );

  // Create the Marcus Bains line.
  if( mAllDayMode ) {
    mMarcusBains = 0;
  } else {
    mMarcusBains = new MarcusBains( this );
    addChild( mMarcusBains );
  }

  mTypeAhead = false;
  mTypeAheadReceiver = 0;

  mReturnPressed = false;
}


void KOAgenda::clear()
{
//  kDebug(5850) << "KOAgenda::clear()" << endl;

  foreach( KOAgendaItem *item, mItems ) { removeChild(item); }
  qDeleteAll( mItems );
  qDeleteAll( mItemsToDelete );
  mItems.clear();
  mItemsToDelete.clear();

  mSelectedItem = 0;

  clearSelection();
}


void KOAgenda::clearSelection()
{
  mHasSelection = false;
  mActionType = NOP;
  updateContents();
}

void KOAgenda::marcus_bains()
{
    if(mMarcusBains) mMarcusBains->updateLocation(true);
}


void KOAgenda::changeColumns(int columns)
{
  if (columns == 0) {
    kDebug(5850) << "KOAgenda::changeColumns() called with argument 0" << endl;
    return;
  }

  clear();
  mColumns = columns;
//  setMinimumSize(mColumns * 10, mGridSpacingY + 1);
//  init();
//  update();

  QResizeEvent event( size(), size() );

  QApplication::sendEvent( this, &event );
}

/*
  This is the eventFilter function, which gets all events from the KOAgendaItems
  contained in the agenda. It has to handle moving and resizing for all items.
*/
bool KOAgenda::eventFilter ( QObject *object, QEvent *event )
{
//  kDebug(5850) << "KOAgenda::eventFilter() " << int( event->type() ) << endl;

  switch( event->type() ) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonDblClick:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseMove:
      return eventFilter_mouse( object, static_cast<QMouseEvent *>( event ) );
#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
      return eventFilter_wheel( object, static_cast<QWheelEvent *>( event ) );
#endif
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
      return eventFilter_key( object, static_cast<QKeyEvent *>( event ) );

    case ( QEvent::Leave ):
      if ( !mActionItem )
        setCursor( Qt::ArrowCursor );
      if ( object == viewport() )
        emit leaveAgenda();
      return true;

    case QEvent::Enter:
      emit enterAgenda();
      return Q3ScrollView::eventFilter( object, event );

#ifndef KORG_NODND
    case QEvent::DragEnter:
    case QEvent::DragMove:
    case QEvent::DragLeave:
    case QEvent::Drop:
 //   case QEvent::DragResponse:
      return eventFilter_drag(object, static_cast<QDropEvent*>(event));
#endif

    default:
      return Q3ScrollView::eventFilter( object, event );
  }
}

bool KOAgenda::eventFilter_drag( QObject *object, QDropEvent *de )
{
#ifndef KORG_NODND
  QPoint viewportPos;
  if ( object != viewport() && object != this ) {
    viewportPos = static_cast<QWidget *>( object )->mapToParent( de->pos() );
  } else {
    viewportPos = de->pos();
  }

  switch ( de->type() ) {
    case QEvent::DragEnter:
    case QEvent::DragMove:
      if ( ICalDrag::canDecode( de ) || VCalDrag::canDecode( de ) ) {

        DndFactory factory( mCalendar );
        Todo *todo = factory.createDropTodo( de );
        if ( todo ) {
          de->accept();
          delete todo;
        } else {
          de->ignore();
        }
        return true;
      } else return false;
      break;
    case QEvent::DragLeave:
      return false;
      break;
    case QEvent::Drop:
      {
        if ( !ICalDrag::canDecode( de ) && !VCalDrag::canDecode( de ) ) {
          return false;
        }

        DndFactory factory( mCalendar );
        Todo *todo = factory.createDropTodo( de );

        if ( todo ) {
          de->acceptAction();
          QPoint pos;
          // FIXME: This is a bad hack, as the viewportToContents seems to be off by
          // 2000 (which is the left upper corner of the viewport). It works correctly
          // for agendaItems.
          if ( object == this  ) {
            pos = viewportPos + QPoint( contentsX(), contentsY() );
          } else {
            pos = viewportToContents( viewportPos );
          }
          QPoint gpos = contentsToGrid( pos );
          emit droppedToDo( todo, gpos, mAllDayMode );
          return true;
        }
      }
      break;

    case QEvent::DragResponse:
    default:
      break;
  }
#endif

  return false;
}

bool KOAgenda::eventFilter_key( QObject *, QKeyEvent *ke )
{
  // kDebug(5850) << "KOAgenda::eventFilter_key() " << ke->type() << endl;

  // If Return is pressed bring up an editor for the current selected time span.
  if ( ke->key() == Qt::Key_Return ) {
    if ( ke->type() == QEvent::KeyPress ) mReturnPressed = true;
    else if ( ke->type() == QEvent::KeyRelease ) {
      if ( mReturnPressed ) {
        emitNewEventForSelection();
        mReturnPressed = false;
        return true;
      } else {
        mReturnPressed = false;
      }
    }
  }

  // Ignore all input that does not produce any output
  if ( ke->text().isEmpty() ) return false;

  if ( ke->type() == QEvent::KeyPress || ke->type() == QEvent::KeyRelease ) {
    switch ( ke->key() ) {
      case Qt::Key_Escape:
      case Qt::Key_Return:
      case Qt::Key_Enter:
      case Qt::Key_Tab:
      case Qt::Key_Backtab:
      case Qt::Key_Left:
      case Qt::Key_Right:
      case Qt::Key_Up:
      case Qt::Key_Down:
      case Qt::Key_Backspace:
      case Qt::Key_Delete:
      case Qt::Key_PageUp:
      case Qt::Key_PageDown:
      case Qt::Key_Home:
      case Qt::Key_End:
      case Qt::Key_Control:
      case Qt::Key_Meta:
      case Qt::Key_Alt:
        break;
      default:
        mTypeAheadEvents.append( new QKeyEvent( ke->type(), ke->key(),
                                                ke->ascii(), ke->state(),
                                                ke->text(), ke->isAutoRepeat(),
                                                ke->count() ) );
        if ( !mTypeAhead ) {
          mTypeAhead = true;
          emitNewEventForSelection();
        }
        return true;
    }
  }
  return false;
}

void KOAgenda::emitNewEventForSelection()
{
  emit newEventSignal();
}

void KOAgenda::finishTypeAhead()
{
//  kDebug(5850) << "KOAgenda::finishTypeAhead()" << endl;
  if ( typeAheadReceiver() ) {
    foreach( QEvent *e, mTypeAheadEvents ) {
//      kDebug(5850) << "sendEvent() " << int( typeAheadReceiver() ) << endl;
      QApplication::sendEvent( typeAheadReceiver(), e );
    }
  }
  qDeleteAll( mTypeAheadEvents );
  mTypeAheadEvents.clear();
  mTypeAhead = false;
}
#ifndef QT_NO_WHEELEVENT
bool KOAgenda::eventFilter_wheel ( QObject *object, QWheelEvent *e )
{
  QPoint viewportPos;
  bool accepted=false;
  if  ( ( e->state() & Qt::ShiftModifier) == Qt::ShiftModifier ) {
    if ( object != viewport() ) {
      viewportPos = ( (QWidget *) object )->mapToParent( e->pos() );
    } else {
      viewportPos = e->pos();
    }
    //kDebug(5850)<< "KOAgenda::eventFilter_wheel: type:"<<
    //  e->type()<<" delta: "<< e->delta()<< endl;
    emit zoomView( -e->delta() ,
      contentsToGrid( viewportToContents( viewportPos ) ),
      Qt::Horizontal );
    accepted=true;
  }

  if  ( ( e->state() & Qt::ControlModifier ) == Qt::ControlModifier ){
    if ( object != viewport() ) {
      viewportPos = ( (QWidget *)object )->mapToParent( e->pos() );
    } else {
      viewportPos = e->pos();
    }
    emit zoomView( -e->delta() ,
      contentsToGrid( viewportToContents( viewportPos ) ),
      Qt::Vertical );
    emit mousePosSignal(gridToContents(contentsToGrid(viewportToContents( viewportPos ))));
    accepted=true;
  }
  if (accepted ) e->accept();
  return accepted;
}
#endif
bool KOAgenda::eventFilter_mouse(QObject *object, QMouseEvent *me)
{
  QPoint viewportPos;
  if (object != viewport()) {
    viewportPos = ((QWidget *)object)->mapToParent(me->pos());
  } else {
    viewportPos = me->pos();
  }

  switch (me->type())  {
    case QEvent::MouseButtonPress:
//        kDebug(5850) << "koagenda: filtered button press" << endl;
      if (object != viewport()) {
        if (me->button() == Qt::RightButton) {
          mClickedItem = dynamic_cast<KOAgendaItem *>(object);
          if (mClickedItem) {
            selectItem(mClickedItem);
            emit showIncidencePopupSignal( mClickedItem->incidence(),
                                           mClickedItem->itemDate() );
          }
        } else {
          KOAgendaItem* item = dynamic_cast<KOAgendaItem *>(object);
          if (item) {
            Incidence *incidence = item->incidence();
            if ( incidence->isReadOnly() ) {
              mActionItem = 0;
            } else {
              mActionItem = item;
              startItemAction(viewportPos);
            }
            // Warning: do selectItem() as late as possible, since all
            // sorts of things happen during this call. Some can lead to
            // this filter being run again and mActionItem being set to
            // null.
            selectItem( item );
          }
        }
      } else {
        if (me->button() == Qt::RightButton)
        {
          // if mouse pointer is not in selection, select the cell below the cursor
          QPoint gpos = contentsToGrid( viewportToContents( viewportPos ) );
          if ( !ptInSelection( gpos ) ) {
            mSelectionStartCell = gpos;
            mSelectionEndCell = gpos;
            mHasSelection = true;
            emit newStartSelectSignal();
            emit newTimeSpanSignal( mSelectionStartCell, mSelectionEndCell );
            updateContents();
          }
          showNewEventPopupSignal();
        }
        else
        {
          // if mouse pointer is in selection, don't change selection
          QPoint gpos = contentsToGrid( viewportToContents( viewportPos ) );
          if ( !ptInSelection( gpos ) ) {
            selectItem(0);
            mActionItem = 0;
            setCursor(Qt::ArrowCursor);
            startSelectAction(viewportPos);
          }
        }
      }
      break;

    case QEvent::MouseButtonRelease:
      if (mActionItem) {
        endItemAction();
      } else if ( mActionType == SELECT ) {
        endSelectAction( viewportPos );
      }
      // This nasty gridToContents(contentsToGrid(..)) is needed to
      // avoid an offset of a few pixels. Don't ask me why...
      emit mousePosSignal( gridToContents(contentsToGrid(
                           viewportToContents( viewportPos ) ) ));
      break;

    case QEvent::MouseMove: {
      // This nasty gridToContents(contentsToGrid(..)) is needed to
      // avoid an offset of a few pixels. Don't ask me why...
      QPoint indicatorPos = gridToContents(contentsToGrid(
                                          viewportToContents( viewportPos )));
      if (object != viewport()) {
        KOAgendaItem *moveItem = dynamic_cast<KOAgendaItem *>(object);
        if (moveItem && !moveItem->incidence()->isReadOnly() ) {
          if (!mActionItem)
            setNoActionCursor(moveItem,viewportPos);
          else {
            performItemAction(viewportPos);

            if ( mActionType == MOVE ) {
              // show cursor at the current begin of the item
              KOAgendaItem *firstItem = mActionItem->firstMultiItem();
              if (!firstItem) firstItem = mActionItem;
              indicatorPos = gridToContents( QPoint( firstItem->cellXLeft(),
                                                     firstItem->cellYTop() ) );

            } else if ( mActionType == RESIZEBOTTOM ) {
              // RESIZETOP is handled correctly, only resizebottom works differently
              indicatorPos = gridToContents( QPoint( mActionItem->cellXLeft(),
                                                     mActionItem->cellYBottom()+1 ) );
            }

          } // If we have an action item
        } // If move item && !read only
      } else {
          if ( mActionType == SELECT ) {
            performSelectAction( viewportPos );

            // show cursor at end of timespan
            if ( ((mStartCell.y() < mEndCell.y()) && (mEndCell.x() >= mStartCell.x())) ||
                 (mEndCell.x() > mStartCell.x()) )
              indicatorPos = gridToContents( QPoint(mEndCell.x(), mEndCell.y()+1) );
            else
              indicatorPos = gridToContents( mEndCell );
          }
        }
      emit mousePosSignal( indicatorPos );
      break; }

    case QEvent::MouseButtonDblClick:
      if (object == viewport()) {
        selectItem(0);
        emit newEventSignal();
      } else {
        KOAgendaItem *doubleClickedItem = dynamic_cast<KOAgendaItem *>(object);
        if (doubleClickedItem) {
          selectItem(doubleClickedItem);
          emit editIncidenceSignal(doubleClickedItem->incidence());
        }
      }
      break;

    default:
      break;
  }

  return true;
}

bool KOAgenda::ptInSelection( QPoint gpos ) const
{
  if ( !mHasSelection ) {
    return false;
  } else if ( gpos.x()<mSelectionStartCell.x() || gpos.x()>mSelectionEndCell.x() ) {
    return false;
  } else if ( (gpos.x()==mSelectionStartCell.x()) && (gpos.y()<mSelectionStartCell.y()) ) {
    return false;
  } else if ( (gpos.x()==mSelectionEndCell.x()) && (gpos.y()>mSelectionEndCell.y()) ) {
    return false;
  }
  return true;
}

void KOAgenda::startSelectAction( const QPoint &viewportPos )
{
  emit newStartSelectSignal();

  mActionType = SELECT;
  mSelectionStartPoint = viewportPos;
  mHasSelection = true;

  QPoint pos = viewportToContents( viewportPos );
  QPoint gpos = contentsToGrid( pos );

  // Store new selection
  mStartCell = gpos;
  mEndCell = gpos;
  mSelectionStartCell = gpos;
  mSelectionEndCell = gpos;

  updateContents();
}

void KOAgenda::performSelectAction(const QPoint& viewportPos)
{
  QPoint pos = viewportToContents( viewportPos );
  QPoint gpos = contentsToGrid( pos );

  QPoint clipperPos = clipper()->
                      mapFromGlobal(viewport()->mapToGlobal(viewportPos));

  // Scroll if cursor was moved to upper or lower end of agenda.
  if (clipperPos.y() < mScrollBorderWidth) {
    mScrollUpTimer.start(mScrollDelay);
  } else if (visibleHeight() - clipperPos.y() <
             mScrollBorderWidth) {
    mScrollDownTimer.start(mScrollDelay);
  } else {
    mScrollUpTimer.stop();
    mScrollDownTimer.stop();
  }

  if ( gpos != mEndCell ) {
    mEndCell = gpos;
    if ( mStartCell.x()>mEndCell.x() ||
         ( mStartCell.x()==mEndCell.x() && mStartCell.y()>mEndCell.y() ) ) {
      // backward selection
      mSelectionStartCell = mEndCell;
      mSelectionEndCell = mStartCell;
    } else {
      mSelectionStartCell = mStartCell;
      mSelectionEndCell = mEndCell;
    }

    updateContents();
  }
}

void KOAgenda::endSelectAction( const QPoint &currentPos )
{
  mScrollUpTimer.stop();
  mScrollDownTimer.stop();

  mActionType = NOP;

  emit newTimeSpanSignal( mSelectionStartCell, mSelectionEndCell );

  if ( KOPrefs::instance()->mSelectionStartsEditor ) {
    if ( ( mSelectionStartPoint - currentPos ).manhattanLength() >
         QApplication::startDragDistance() ) {
       emitNewEventForSelection();
    }
  }
}

KOAgenda::MouseActionType KOAgenda::isInResizeArea( bool horizontal,
    const QPoint &pos, KOAgendaItem*item )
{
  if (!item) return NOP;
  QPoint gridpos = contentsToGrid( pos );
  QPoint contpos = gridToContents( gridpos +
      QPoint( (KOGlobals::self()->reverseLayout())?1:0, 0 ) );

//kDebug(5850)<<"contpos="<<contpos<<", pos="<<pos<<", gpos="<<gpos<<endl;
//kDebug(5850)<<"clXLeft="<<clXLeft<<", clXRight="<<clXRight<<endl;

  if ( horizontal ) {
    int clXLeft = item->cellXLeft();
    int clXRight = item->cellXRight();
    if ( KOGlobals::self()->reverseLayout() ) {
      int tmp = clXLeft;
      clXLeft = clXRight;
      clXRight = tmp;
    }
    int gridDistanceX = int( pos.x() - contpos.x() );
    if (gridDistanceX < mResizeBorderWidth && clXLeft == gridpos.x() ) {
      if ( KOGlobals::self()->reverseLayout() ) return RESIZERIGHT;
      else return RESIZELEFT;
    } else if ((mGridSpacingX - gridDistanceX) < mResizeBorderWidth &&
               clXRight == gridpos.x() ) {
      if ( KOGlobals::self()->reverseLayout() ) return RESIZELEFT;
      else return RESIZERIGHT;
    } else {
      return MOVE;
    }
  } else {
    int gridDistanceY = int( pos.y() - contpos.y() );
    if (gridDistanceY < mResizeBorderWidth &&
        item->cellYTop() == gridpos.y() &&
        !item->firstMultiItem() ) {
      return RESIZETOP;
    } else if ((mGridSpacingY - gridDistanceY) < mResizeBorderWidth &&
               item->cellYBottom() == gridpos.y() &&
               !item->lastMultiItem() )  {
      return RESIZEBOTTOM;
    } else {
      return MOVE;
    }
  }
}

void KOAgenda::startItemAction(const QPoint& viewportPos)
{
  QPoint pos = viewportToContents( viewportPos );
  mStartCell = contentsToGrid( pos );
  mEndCell = mStartCell;

  bool noResize = ( mActionItem->incidence()->type() == "Todo");

  mActionType = MOVE;
  if ( !noResize ) {
    mActionType = isInResizeArea( mAllDayMode, pos, mActionItem );
  }


  mActionItem->startMove();
  setActionCursor( mActionType, true );
}

void KOAgenda::performItemAction(const QPoint& viewportPos)
{
//  kDebug(5850) << "viewportPos: " << viewportPos.x() << "," << viewportPos.y() << endl;
//  QPoint point = viewport()->mapToGlobal(viewportPos);
//  kDebug(5850) << "Global: " << point.x() << "," << point.y() << endl;
//  point = clipper()->mapFromGlobal(point);
//  kDebug(5850) << "clipper: " << point.x() << "," << point.y() << endl;
//  kDebug(5850) << "visible height: " << visibleHeight() << endl;
  QPoint pos = viewportToContents( viewportPos );
//  kDebug(5850) << "contents: " << x << "," << y << "\n" << endl;
  QPoint gpos = contentsToGrid( pos );
  QPoint clipperPos = clipper()->
                      mapFromGlobal(viewport()->mapToGlobal(viewportPos));

  // Cursor left active agenda area.
  // This starts a drag.
  if ( clipperPos.y() < 0 || clipperPos.y() > visibleHeight() ||
       clipperPos.x() < 0 || clipperPos.x() > visibleWidth() ) {
    if ( mActionType == MOVE ) {
      mScrollUpTimer.stop();
      mScrollDownTimer.stop();
      mActionItem->resetMove();
      placeSubCells( mActionItem );
      emit startDragSignal( mActionItem->incidence() );
      setCursor( Qt::ArrowCursor );
      mActionItem = 0;
      mActionType = NOP;
      mItemMoved = false;
      if ( mItemMoved && mChanger )
        mChanger->endChange( mActionItem->incidence() );
      return;
    }
  } else {
    setActionCursor( mActionType );
  }

  // Scroll if item was moved to upper or lower end of agenda.
  if (clipperPos.y() < mScrollBorderWidth) {
    mScrollUpTimer.start(mScrollDelay);
  } else if (visibleHeight() - clipperPos.y() <
             mScrollBorderWidth) {
    mScrollDownTimer.start(mScrollDelay);
  } else {
    mScrollUpTimer.stop();
    mScrollDownTimer.stop();
  }

  // Move or resize item if necessary
  if ( mEndCell != gpos ) {
    if ( !mItemMoved ) {
      if ( !mChanger || !mChanger->beginChange( mActionItem->incidence() ) ) {
        KMessageBox::information( this, i18n("Unable to lock item for "
                             "modification. You cannot change make any changes."),
                             i18n("Locking Failed"), "AgendaLockingFailed" );
        mScrollUpTimer.stop();
        mScrollDownTimer.stop();
        mActionItem->resetMove();
        placeSubCells( mActionItem );
        setCursor( Qt::ArrowCursor );
        mActionItem = 0;
        mActionType = NOP;
        mItemMoved = false;
        return;
      }
      mItemMoved = true;
    }
    mActionItem->raise();
    if (mActionType == MOVE) {
      // Move all items belonging to a multi item
      KOAgendaItem *firstItem = mActionItem->firstMultiItem();
      if (!firstItem) firstItem = mActionItem;
      KOAgendaItem *lastItem = mActionItem->lastMultiItem();
      if (!lastItem) lastItem = mActionItem;
      QPoint deltapos = gpos - mEndCell;
      KOAgendaItem *moveItem = firstItem;
      while (moveItem) {
        bool changed=false;
        if ( deltapos.x()!=0 ) {
          moveItem->moveRelative( deltapos.x(), 0 );
          changed=true;
        }
        // in agenda's all day view don't try to move multi items, since there are none
        if ( moveItem==firstItem && !mAllDayMode ) { // is the first item
          int newY = deltapos.y() + moveItem->cellYTop();
          // If event start moved earlier than 0:00, it starts the previous day
          if ( newY<0 ) {
            moveItem->expandTop( -moveItem->cellYTop() );
            // prepend a new item at ( x-1, rows()+newY to rows() )
            KOAgendaItem *newFirst = firstItem->prevMoveItem();
            // cell's y values are first and last cell of the bar, so if newY=-1, they need to be the same
            if (newFirst) {
              newFirst->setCellXY(moveItem->cellXLeft()-1, rows()+newY, rows()-1);
              mItems.append( newFirst );
              moveItem->resize( int( mGridSpacingX * newFirst->cellWidth() ),
                                int( mGridSpacingY * newFirst->cellHeight() ));
              QPoint cpos = gridToContents( QPoint( newFirst->cellXLeft(), newFirst->cellYTop() ) );
              addChild( newFirst, cpos.x(), cpos.y() );
            } else {
              newFirst = insertItem( moveItem->incidence(), moveItem->itemDate(),
                moveItem->cellXLeft()-1, rows()+newY, rows()-1 ) ;
            }
            if (newFirst) newFirst->show();
            moveItem->prependMoveItem(newFirst);
            firstItem=newFirst;
          } else if ( newY>=rows() ) {
            // If event start is moved past 24:00, it starts the next day
            // erase current item (i.e. remove it from the multiItem list)
            firstItem = moveItem->nextMultiItem();
            moveItem->hide();
            mItems.removeAll( moveItem );
            removeChild( moveItem );
            mActionItem->removeMoveItem(moveItem);
            moveItem=firstItem;
            // adjust next day's item
            if (moveItem) moveItem->expandTop( rows()-newY );
          } else {
            moveItem->expandTop(deltapos.y());
          }
          changed=true;
        }
        if ( moveItem && !moveItem->lastMultiItem() && !mAllDayMode ) { // is the last item
          int newY = deltapos.y()+moveItem->cellYBottom();
          if (newY<0) {
            // erase current item
            lastItem = moveItem->prevMultiItem();
            moveItem->hide();
            mItems.removeAll( moveItem );
            removeChild( moveItem );
            moveItem->removeMoveItem( moveItem );
            moveItem = lastItem;
            moveItem->expandBottom(newY+1);
          } else if (newY>=rows()) {
            moveItem->expandBottom( rows()-moveItem->cellYBottom()-1 );
            // append item at ( x+1, 0 to newY-rows() )
            KOAgendaItem *newLast = lastItem->nextMoveItem();
            if (newLast) {
              newLast->setCellXY( moveItem->cellXLeft()+1, 0, newY-rows()-1 );
              mItems.append(newLast);
              moveItem->resize( int( mGridSpacingX * newLast->cellWidth() ),
                                int( mGridSpacingY * newLast->cellHeight() ));
              QPoint cpos = gridToContents( QPoint( newLast->cellXLeft(), newLast->cellYTop() ) ) ;
              addChild( newLast, cpos.x(), cpos.y() );
            } else {
              newLast = insertItem( moveItem->incidence(), moveItem->itemDate(),
                moveItem->cellXLeft()+1, 0, newY-rows()-1 ) ;
            }
            moveItem->appendMoveItem( newLast );
            newLast->show();
            lastItem = newLast;
          } else {
            moveItem->expandBottom( deltapos.y() );
          }
          changed=true;
        }
        if (changed) {
          adjustItemPosition( moveItem );
        }
        moveItem = moveItem->nextMultiItem();
      }
    } else if (mActionType == RESIZETOP) {
      if (mEndCell.y() <= mActionItem->cellYBottom()) {
        mActionItem->expandTop(gpos.y() - mEndCell.y());
        adjustItemPosition( mActionItem );
      }
    } else if (mActionType == RESIZEBOTTOM) {
      if (mEndCell.y() >= mActionItem->cellYTop()) {
        mActionItem->expandBottom(gpos.y() - mEndCell.y());
        adjustItemPosition( mActionItem );
      }
    } else if (mActionType == RESIZELEFT) {
      if (mEndCell.x() <= mActionItem->cellXRight()) {
        mActionItem->expandLeft( gpos.x() - mEndCell.x() );
        adjustItemPosition( mActionItem );
      }
    } else if (mActionType == RESIZERIGHT) {
      if (mEndCell.x() >= mActionItem->cellXLeft()) {
        mActionItem->expandRight(gpos.x() - mEndCell.x());
        adjustItemPosition( mActionItem );
      }
    }
    mEndCell = gpos;
  }
}

void KOAgenda::endItemAction()
{
//  kDebug(5850) << "KOAgenda::endItemAction() " << endl;
  mScrollUpTimer.stop();
  mScrollDownTimer.stop();
  setCursor( Qt::ArrowCursor );
  bool multiModify = false;
  // FIXME: do the cloning here...

  if ( mItemMoved ) {
    bool modify = true;
    if ( mActionItem->incidence()->doesRecur() ) {
      int res = KOMessageBox::fourBtnMsgBox( this, QMessageBox::Question,
          i18n("The item you try to change is a recurring item. Shall the changes "
               "be applied only to this single occurrence, only to the future items, "
               "or to all items in the recurrence?"),
          i18n("Changing Recurring Item"),
          i18n("Only &This Item"), i18n("Only &Future Items"), i18n("&All Occurrences") );
      switch ( res ) {
        case KMessageBox::Ok: // All occurrences
            // Moving the whole sequene of events is handled by the itemModified below.
            modify = true;
            break;
        case KMessageBox::Yes: { // Just this occurrence
            // Dissociate this occurrence:
            // create clone of event, set relation to old event, set cloned event
            // for mActionItem, add exception date to old event, changeIncidence
            // for the old event, remove the recurrence from the new copy and then just
            // go on with the newly adjusted mActionItem and let the usual code take
            // care of the new time!
            modify = true;
            multiModify = true;
            emit startMultiModify( i18n("Dissociate event from recurrence") );
            Incidence* oldInc = mActionItem->incidence();
            Incidence* oldIncSaved = mActionItem->incidence()->clone();
            Incidence* newInc = mCalendar->dissociateOccurrence(
                oldInc, mActionItem->itemDate() );
            if ( newInc ) {
              // don't recreate items, they already have the correct position
              emit enableAgendaUpdate( false );
              mChanger->changeIncidence( oldIncSaved, oldInc );
              mActionItem->setIncidence( newInc );
              mActionItem->dissociateFromMultiItem();
              mChanger->addIncidence( newInc );
              emit enableAgendaUpdate( true );
            } else {
              KMessageBox::sorry( this, i18n("Unable to add the exception item to the "
                  "calendar. No change will be done."), i18n("Error Occurred") );
            }
            delete oldIncSaved;
            break; }
        case KMessageBox::No/*Future*/: { // All future occurrences
            // Dissociate this occurrence:
            // create clone of event, set relation to old event, set cloned event
            // for mActionItem, add recurrence end date to old event, changeIncidence
            // for the old event, adjust the recurrence for the new copy and then just
            // go on with the newly adjusted mActionItem and let the usual code take
            // care of the new time!
            modify = true;
            multiModify = true;
            emit startMultiModify( i18n("Split future recurrences") );
            Incidence* oldInc = mActionItem->incidence();
            Incidence* oldIncSaved = mActionItem->incidence()->clone();
            Incidence* newInc = mCalendar->dissociateOccurrence(
                oldInc, mActionItem->itemDate(), false );
            if ( newInc ) {
              emit enableAgendaUpdate( false );
              mActionItem->dissociateFromMultiItem();
              mActionItem->setIncidence( newInc );
              mChanger->addIncidence( newInc );
              emit enableAgendaUpdate( true );
              mChanger->changeIncidence( oldIncSaved, oldInc );
            } else {
              KMessageBox::sorry( this, i18n("Unable to add the future items to the "
                  "calendar. No change will be done."), i18n("Error Occurred") );
            }
            delete oldIncSaved;
            break; }
        default:
          modify = false;
          mActionItem->resetMove();
          placeSubCells( mActionItem );
      }
    }

    if ( modify ) {
      mActionItem->endMove();
      KOAgendaItem *placeItem = mActionItem->firstMultiItem();
      // FIXME: A mChanger->changeIncidence is missing here!
      if  ( !placeItem ) {
        placeItem = mActionItem;
      }

      KOAgendaItem *modif = placeItem;

      QList<KOAgendaItem*> oldconflictItems = placeItem->conflictItems();
      QList<KOAgendaItem*>::iterator it;
      for ( it = oldconflictItems.begin(); it != oldconflictItems.end(); ++it ) {
        placeSubCells( *it );
      }
      while ( placeItem ) {
        placeSubCells( placeItem );
        placeItem = placeItem->nextMultiItem();
      }

      // Notify about change, so that agenda view can update the event data
      emit itemModified( modif );
    }
    // FIXME: If the change failed, we need to update the view!
    mChanger->endChange( mActionItem->incidence() );
  }

  mActionItem = 0;
  mActionType = NOP;
  mItemMoved = false;

  if ( multiModify ) emit endMultiModify();

  kDebug(5850) << "KOAgenda::endItemAction() done" << endl;
}

void KOAgenda::setActionCursor( int actionType, bool acting )
{
  switch ( actionType ) {
    case MOVE:
      if (acting) setCursor( Qt::SizeAllCursor );
      else setCursor( Qt::ArrowCursor );
      break;
    case RESIZETOP:
    case RESIZEBOTTOM:
      setCursor( Qt::SizeHorCursor );
      break;
    case RESIZELEFT:
    case RESIZERIGHT:
      setCursor( Qt::SizeHorCursor );
      break;
    default:
      setCursor( Qt::ArrowCursor );
  }
}

void KOAgenda::setNoActionCursor( KOAgendaItem *moveItem, const QPoint& viewportPos )
{
//  kDebug(5850) << "viewportPos: " << viewportPos.x() << "," << viewportPos.y() << endl;
//  QPoint point = viewport()->mapToGlobal(viewportPos);
//  kDebug(5850) << "Global: " << point.x() << "," << point.y() << endl;
//  point = clipper()->mapFromGlobal(point);
//  kDebug(5850) << "clipper: " << point.x() << "," << point.y() << endl;

  QPoint pos = viewportToContents( viewportPos );
  bool noResize = (moveItem && moveItem->incidence() &&
      moveItem->incidence()->type() == "Todo");

  KOAgenda::MouseActionType resizeType = MOVE;
  if ( !noResize ) resizeType = isInResizeArea( mAllDayMode, pos , moveItem);
  setActionCursor( resizeType );
}


/** calculate the width of the column subcells of the given item
*/
double KOAgenda::calcSubCellWidth( KOAgendaItem *item )
{
  QPoint pt, pt1;
  pt = gridToContents( QPoint( item->cellXLeft(), item->cellYTop() ) );
  pt1 = gridToContents( QPoint( item->cellXLeft(), item->cellYTop() ) +
                        QPoint( 1, 1 ) );
  pt1 -= pt;
  int maxSubCells = item->subCells();
  double newSubCellWidth;
  if ( mAllDayMode ) {
    newSubCellWidth = double( pt1.y() ) / maxSubCells;
  } else {
    newSubCellWidth = double( pt1.x() ) / maxSubCells;
  }
  return newSubCellWidth;
}

void KOAgenda::adjustItemPosition( KOAgendaItem *item )
{
  if (!item) return;
  item->resize( int( mGridSpacingX * item->cellWidth() ),
                int( mGridSpacingY * item->cellHeight() ) );
  int clXLeft = item->cellXLeft();
  if ( KOGlobals::self()->reverseLayout() )
    clXLeft = item->cellXRight() + 1;
  QPoint cpos = gridToContents( QPoint( clXLeft, item->cellYTop() ) );
  moveChild( item, cpos.x(), cpos.y() );
}

void KOAgenda::placeAgendaItem( KOAgendaItem *item, double subCellWidth )
{
//  kDebug(5850) << "KOAgenda::placeAgendaItem(): " << item->incidence()->summary()
//            << " subCellWidth: " << subCellWidth << endl;

  // "left" upper corner, no subcells yet, RTL layouts have right/left switched, widths are negative then
  QPoint pt = gridToContents( QPoint( item->cellXLeft(), item->cellYTop() ) );
  // right lower corner
  QPoint pt1 = gridToContents( QPoint( item->cellXLeft() + item->cellWidth(),
      item->cellYBottom()+1 ) );

  double subCellPos = item->subCell() * subCellWidth;

  // we need to add 0.01 to make sure we don't loose one pixed due to
  // numerics (i.e. if it would be x.9998, we want the integer, not rounded down.
  double delta=0.01;
  if (subCellWidth<0) delta=-delta;
  int height, width, xpos, ypos;
  if (mAllDayMode) {
    width = pt1.x()-pt.x();
    height = int( subCellPos + subCellWidth + delta ) - int( subCellPos );
    xpos = pt.x();
    ypos = pt.y() + int( subCellPos );
  } else {
    width = int( subCellPos + subCellWidth + delta ) - int( subCellPos );
    height = pt1.y()-pt.y();
    xpos = pt.x() + int( subCellPos );
    ypos = pt.y();
  }
  if ( KOGlobals::self()->reverseLayout() ) { // RTL language/layout
    xpos += width;
    width = -width;
  }
  if ( height<0 ) { // BTT (bottom-to-top) layout ?!?
    ypos += height;
    height = -height;
  }
  item->resize( width, height );
  moveChild( item, xpos, ypos );
}

/*
  Place item in cell and take care that multiple items using the same cell do
  not overlap. This method is not yet optimal. It doesn't use the maximum space
  it can get in all cases.
  At the moment the method has a bug: When an item is placed only the sub cell
  widths of the items are changed, which are within the Y region the item to
  place spans. When the sub cell width change of one of this items affects a
  cell, where other items are, which do not overlap in Y with the item to place,
  the display gets corrupted, although the corruption looks quite nice.
*/
void KOAgenda::placeSubCells( KOAgendaItem *placeItem )
{
#if 0
  kDebug(5850) << "KOAgenda::placeSubCells()" << endl;
  if ( placeItem ) {
    Incidence *event = placeItem->incidence();
    if ( !event ) {
      kDebug(5850) << "  event is 0" << endl;
    } else {
      kDebug(5850) << "  event: " << event->summary() << endl;
    }
  } else {
    kDebug(5850) << "  placeItem is 0" << endl;
  }
  kDebug(5850) << "KOAgenda::placeSubCells()..." << endl;
#endif

  QList<KOrg::CellItem*> cells;
  foreach( KOrg::CellItem* item, mItems ) { cells.append(item); }

  QList<KOrg::CellItem*> items = KOrg::CellItem::placeItem( cells, placeItem );

  placeItem->setConflictItems( QList<KOAgendaItem*>() );
  double newSubCellWidth = calcSubCellWidth( placeItem );
  QList<KOrg::CellItem*>::iterator it;
  for ( it = items.begin(); it != items.end(); ++it ) {
    KOAgendaItem *item = static_cast<KOAgendaItem *>( *it );
    placeAgendaItem( item, newSubCellWidth );
    item->addConflictItem( placeItem );
    placeItem->addConflictItem( item );
  }
  if ( items.isEmpty() ) {
    placeAgendaItem( placeItem, newSubCellWidth );
  }
  placeItem->update();
}

int KOAgenda::columnWidth( int column )
{
  int start = gridToContents( QPoint( column, 0 ) ).x();
  if (KOGlobals::self()->reverseLayout() )
    column--;
  else
    column++;
  int end = gridToContents( QPoint( column, 0 ) ).x();
  return end - start;
}
/*
  Draw grid in the background of the agenda.
*/
void KOAgenda::drawContents(QPainter* p, int cx, int cy, int cw, int ch)
{
  QPixmap db(cw, ch);
  db.fill(KOPrefs::instance()->mAgendaBgColor);
  QPainter dbp(&db);
  dbp.translate(-cx,-cy);

//  kDebug(5850) << "KOAgenda::drawContents()" << endl;
  double lGridSpacingY = mGridSpacingY*2;

  // Highlight working hours
  if (mWorkingHoursEnable) {
    QPoint pt1( cx, mWorkingHoursYTop );
    QPoint pt2( cx+cw, mWorkingHoursYBottom );
    if ( pt2.x() >= pt1.x() /*&& pt2.y() >= pt1.y()*/) {
      int gxStart = contentsToGrid( pt1 ).x();
      int gxEnd = contentsToGrid( pt2 ).x();
      // correct start/end for rtl layouts
      if ( gxStart > gxEnd ) {
        int tmp = gxStart;
        gxStart = gxEnd;
        gxEnd = tmp;
      }
      int xoffset = ( KOGlobals::self()->reverseLayout()?1:0 );
      while( gxStart <= gxEnd ) {
        int xStart = gridToContents( QPoint( gxStart+xoffset, 0 ) ).x();
        int xWidth = columnWidth( gxStart ) + 1;
        if ( pt2.y() < pt1.y() ) {
          // overnight working hours
          if ( ( (gxStart==0) && !mHolidayMask->at(mHolidayMask->count()-1) ) ||
               ( (gxStart>0) && (gxStart<int(mHolidayMask->count())) && (!mHolidayMask->at(gxStart-1) ) ) ) {
            if ( pt2.y() > cy ) {
              dbp.fillRect( xStart, cy, xWidth, pt2.y() - cy + 1,
                            KOPrefs::instance()->mWorkingHoursColor);
            }
          }
          if ( (gxStart < int(mHolidayMask->count()-1)) && (!mHolidayMask->at(gxStart)) ) {
            if ( pt1.y() < cy + ch - 1 ) {
              dbp.fillRect( xStart, pt1.y(), xWidth, cy + ch - pt1.y() + 1,
                            KOPrefs::instance()->mWorkingHoursColor);
            }
          }
        } else {
          // last entry in holiday mask denotes the previous day not visible (needed for overnight shifts)
          if ( gxStart < int(mHolidayMask->count()-1) && !mHolidayMask->at(gxStart)) {
            dbp.fillRect( xStart, pt1.y(), xWidth, pt2.y() - pt1.y() + 1,
                          KOPrefs::instance()->mWorkingHoursColor );
          }
        }
        ++gxStart;
      }
    }
  }

  // draw selection
  if ( mHasSelection ) {
    QPoint pt, pt1;

    if ( mSelectionEndCell.x() > mSelectionStartCell.x() ) { // multi day selection
      // draw start day
      pt = gridToContents( mSelectionStartCell );
      pt1 = gridToContents( QPoint( mSelectionStartCell.x() + 1, mRows + 1 ) );
      dbp.fillRect( QRect( pt, pt1 ), KOPrefs::instance()->mHighlightColor );
      // draw all other days between the start day and the day of the selection end
      for ( int c = mSelectionStartCell.x() + 1; c < mSelectionEndCell.x(); ++c ) {
        pt = gridToContents( QPoint( c, 0 ) );
        pt1 = gridToContents( QPoint( c + 1, mRows + 1 ) );
        dbp.fillRect( QRect( pt, pt1 ), KOPrefs::instance()->mHighlightColor );
      }
      // draw end day
      pt = gridToContents( QPoint( mSelectionEndCell.x(), 0 ) );
      pt1 = gridToContents( mSelectionEndCell + QPoint(1,1) );
      dbp.fillRect( QRect( pt, pt1), KOPrefs::instance()->mHighlightColor );
    }  else { // single day selection
      pt = gridToContents( mSelectionStartCell );
      pt1 = gridToContents( mSelectionEndCell + QPoint(1,1) );
      dbp.fillRect( QRect( pt, pt1 ), KOPrefs::instance()->mHighlightColor );
    }
  }

  QPen hourPen( KOPrefs::instance()->mAgendaBgColor.dark( 150 ) );
  QPen halfHourPen( KOPrefs::instance()->mAgendaBgColor.dark( 125 ) );
  dbp.setPen( hourPen );

  // Draw vertical lines of grid, start with the last line not yet visible
  //  kDebug(5850) << "drawContents cx: " << cx << " cy: " << cy << " cw: " << cw << " ch: " << ch << endl;
  double x = ( int( cx / mGridSpacingX ) ) * mGridSpacingX;
  while (x < cx + cw) {
    dbp.drawLine( int( x ), cy, int( x ), cy + ch );
    x+=mGridSpacingX;
  }

  // Draw horizontal lines of grid
  double y = ( int( cy / (2*lGridSpacingY) ) ) * 2 * lGridSpacingY;
  while (y < cy + ch) {
//    kDebug(5850) << " y: " << y << endl;
    dbp.drawLine( cx, int( y ), cx + cw, int( y ) );
    y += 2 * lGridSpacingY;
  }
  y = ( 2 * int( cy / (2*lGridSpacingY) ) + 1) * lGridSpacingY;
  dbp.setPen( halfHourPen );
  while (y < cy + ch) {
//    kDebug(5850) << " y: " << y << endl;
    dbp.drawLine( cx, int( y ), cx + cw, int( y ) );
    y+=2*lGridSpacingY;
  }
  p->drawPixmap(cx,cy, db);
}

/*
  Convert srcollview contents coordinates to agenda grid coordinates.
*/
QPoint KOAgenda::contentsToGrid ( const QPoint &pos ) const
{
  int gx = int( KOGlobals::self()->reverseLayout() ?
        mColumns - pos.x()/mGridSpacingX : pos.x()/mGridSpacingX );
  int gy = int( pos.y()/mGridSpacingY );
  return QPoint( gx, gy );
}

/*
  Convert agenda grid coordinates to scrollview contents coordinates.
*/
QPoint KOAgenda::gridToContents( const QPoint &gpos ) const
{
  int x = int( KOGlobals::self()->reverseLayout() ?
             (mColumns - gpos.x())*mGridSpacingX : gpos.x()*mGridSpacingX );
  int y = int( gpos.y()*mGridSpacingY );
  return QPoint( x, y );
}


/*
  Return Y coordinate corresponding to time. Coordinates are rounded to fit into
  the grid.
*/
int KOAgenda::timeToY(const QTime &time)
{
//  kDebug(5850) << "Time: " << time.toString() << endl;
  int minutesPerCell = 24 * 60 / mRows;
//  kDebug(5850) << "minutesPerCell: " << minutesPerCell << endl;
  int timeMinutes = time.hour() * 60 + time.minute();
//  kDebug(5850) << "timeMinutes: " << timeMinutes << endl;
  int Y = (timeMinutes + (minutesPerCell / 2)) / minutesPerCell;
//  kDebug(5850) << "y: " << Y << endl;
//  kDebug(5850) << "\n" << endl;
  return Y;
}


/*
  Return time corresponding to cell y coordinate. Coordinates are rounded to
  fit into the grid.
*/
QTime KOAgenda::gyToTime(int gy)
{
//  kDebug(5850) << "gyToTime: " << gy << endl;
  int secondsPerCell = 24 * 60 * 60/ mRows;

  int timeSeconds = secondsPerCell * gy;

  QTime time( 0, 0, 0 );
  if ( timeSeconds < 24 * 60 * 60 ) {
    time = time.addSecs(timeSeconds);
  } else {
    time.setHMS( 23, 59, 59 );
  }
//  kDebug(5850) << "  gyToTime: " << time.toString() << endl;

  return time;
}

QVector<int> KOAgenda::minContentsY()
{
  QVector<int> minArray;
  minArray.fill( timeToY( QTime(23, 59) ), mSelectedDates.count() );
  foreach( KOAgendaItem *item, mItems ) {
    int ymin = item->cellYTop();
    int index = item->cellXLeft();
    if ( index>=0 && index<(int)(mSelectedDates.count()) ) {
      if ( ymin < minArray[index] && !mItemsToDelete.contains( item ) )
        minArray[index] = ymin;
    }
  }

  return minArray;
}

QVector<int> KOAgenda::maxContentsY()
{
  QVector<int> maxArray;
  maxArray.fill( timeToY( QTime(0, 0) ), mSelectedDates.count() );
  foreach( KOAgendaItem *item, mItems ) {
    int ymax = item->cellYBottom();
    int index = item->cellXLeft();
    if ( index>=0 && index<(int)(mSelectedDates.count()) ) {
      if ( ymax > maxArray[index] && !mItemsToDelete.contains( item )  )
        maxArray[index] = ymax;
    }
  }

  return maxArray;
}

void KOAgenda::setStartTime( const QTime &startHour )
{
  double startPos = ( startHour.hour()/24. + startHour.minute()/1440. +
                      startHour.second()/86400. ) * mRows * gridSpacingY();
  setContentsPos( 0, int( startPos ) );
}


/*
  Insert KOAgendaItem into agenda.
*/
KOAgendaItem *KOAgenda::insertItem( Incidence *incidence, const QDate &qd, int X,
                                    int YTop, int YBottom )
{
#if 0
  kDebug(5850) << "KOAgenda::insertItem:" << event->summary() << "-"
                << qd.toString() << " ;top, bottom:" << YTop << "," << YBottom
                << endl;
#endif

  if ( mAllDayMode ) {
    kDebug(5850) << "KOAgenda: calling insertItem in all-day mode is illegal." << endl;
    return 0;
  }


  mActionType = NOP;

  KOAgendaItem *agendaItem = new KOAgendaItem( incidence, qd, viewport() );
  connect( agendaItem, SIGNAL( removeAgendaItem( KOAgendaItem * ) ),
           SLOT( removeAgendaItem( KOAgendaItem * ) ) );
  connect( agendaItem, SIGNAL( showAgendaItem( KOAgendaItem * ) ),
           SLOT( showAgendaItem( KOAgendaItem * ) ) );

  if ( YBottom <= YTop ) {
    kDebug(5850) << "KOAgenda::insertItem(): Text: " << agendaItem->text() << " YSize<0" << endl;
    YBottom = YTop;
  }

  agendaItem->resize( int( ( X + 1 ) * mGridSpacingX ) -
                      int( X * mGridSpacingX ),
                      int( YTop * mGridSpacingY ) -
                      int( ( YBottom + 1 ) * mGridSpacingY ) );
  agendaItem->setCellXY( X, YTop, YBottom );
  agendaItem->setCellXRight( X );
  agendaItem->setResourceColor( KOHelper::resourceColor( mCalendar, incidence ) );
  agendaItem->installEventFilter( this );

  addChild( agendaItem, int( X * mGridSpacingX ), int( YTop * mGridSpacingY ) );
  mItems.append( agendaItem );

  placeSubCells( agendaItem );

  agendaItem->show();

  marcus_bains();

  return agendaItem;
}

/*
  Insert all-day KOAgendaItem into agenda.
*/
KOAgendaItem *KOAgenda::insertAllDayItem( Incidence *event, const QDate &qd,
                                          int XBegin, int XEnd )
{
  if ( !mAllDayMode ) {
    kDebug(5850) << "KOAgenda: calling insertAllDayItem in non all-day mode is illegal." << endl;
    return 0;
  }

  mActionType = NOP;

  KOAgendaItem *agendaItem = new KOAgendaItem( event, qd, viewport() );
  connect( agendaItem, SIGNAL( removeAgendaItem( KOAgendaItem* ) ),
           SLOT( removeAgendaItem( KOAgendaItem* ) ) );
  connect( agendaItem, SIGNAL( showAgendaItem( KOAgendaItem* ) ),
           SLOT( showAgendaItem( KOAgendaItem* ) ) );

  agendaItem->setCellXY( XBegin, 0, 0 );
  agendaItem->setCellXRight( XEnd );

  double startIt = mGridSpacingX * ( agendaItem->cellXLeft() );
  double endIt = mGridSpacingX * ( agendaItem->cellWidth() +
                                   agendaItem->cellXLeft() );

  agendaItem->resize( int( endIt ) - int( startIt ), int( mGridSpacingY ) );

  agendaItem->installEventFilter( this );
  agendaItem->setResourceColor( KOHelper::resourceColor( mCalendar, event ) );
  addChild( agendaItem, int( XBegin * mGridSpacingX ), 0 );
  mItems.append( agendaItem );

  placeSubCells( agendaItem );

  agendaItem->show();

  return agendaItem;
}


void KOAgenda::insertMultiItem (Event *event,const QDate &qd,int XBegin,int XEnd,
                                int YTop,int YBottom)
{
  if (mAllDayMode) {
    kDebug(5850) << "KOAgenda: calling insertMultiItem in all-day mode is illegal." << endl;
    return;
  }
  mActionType = NOP;

  int cellX,cellYTop,cellYBottom;
  QString newtext;
  int width = XEnd - XBegin + 1;
  int count = 0;
  KOAgendaItem *current = 0;
  QList<KOAgendaItem*> multiItems;
  int visibleCount = mSelectedDates.first().daysTo(mSelectedDates.last());
  for ( cellX = XBegin; cellX <= XEnd; ++cellX ) {
    ++count;
    //Only add the items that are visible.
    if( cellX >=0 && cellX <= visibleCount ) {
      if ( cellX == XBegin ) cellYTop = YTop;
      else cellYTop = 0;
      if ( cellX == XEnd ) cellYBottom = YBottom;
      else cellYBottom = rows() - 1;
      newtext = QString("(%1/%2): ").arg( count ).arg( width );
      newtext.append( event->summary() );

      current = insertItem( event, qd, cellX, cellYTop, cellYBottom );
      current->setText( newtext );
      multiItems.append( current );
    }
  }

  QList<KOAgendaItem*>::iterator it;
  QList<KOAgendaItem*>::iterator b = multiItems.begin();
  QList<KOAgendaItem*>::iterator e = multiItems.end();
  KOAgendaItem *first = multiItems.first();
  KOAgendaItem *last = multiItems.last();
  KOAgendaItem *prev = 0, *next = 0;

  while ( it != e ) {
    KOAgendaItem *item = *it;
    ++it;
    next = (it==e)?0:(*it);
    if ( item )
      item->setMultiItem( (item == first)?0:first, prev, next, (item==last)?0:last  );
    prev = item;
  }

  marcus_bains();
}

void KOAgenda::removeIncidence( Incidence *incidence )
{
  // First find all items to be deleted and store them
  // in its own list. Otherwise removeAgendaItem will reset
  // the current position in the iterator-loop and mess the logic up.
  QList<KOAgendaItem*> itemsToRemove;
  KOAgendaItem *item;

  foreach( item, mItems ) {
    if ( item && item->incidence() == incidence ) {
      itemsToRemove.append( item );
    }
  }

  foreach( item, itemsToRemove ) {
    removeAgendaItem( item );
  }
}

void KOAgenda::showAgendaItem( KOAgendaItem *agendaItem )
{
  if ( !agendaItem ) return;
  agendaItem->hide();
  addChild( agendaItem );
  if ( !mItems.contains( agendaItem ) )
    mItems.append( agendaItem );
  placeSubCells( agendaItem );

  agendaItem->show();
}

bool KOAgenda::removeAgendaItem( KOAgendaItem *item )
{
  // we found the item. Let's remove it and update the conflicts
  bool taken = false;
  KOAgendaItem *thisItem = item;
  QList<KOAgendaItem*> conflictItems = thisItem->conflictItems();
  removeChild( thisItem );

  taken = ( mItems.removeAll( thisItem ) > 0 );

  QList<KOAgendaItem*>::iterator it;
  for ( it = conflictItems.begin(); it != conflictItems.end(); ++it ) {
    // the item itself is also in its own conflictItems list!
    if ( *it != thisItem ) placeSubCells( *it );

  }
  mItemsToDelete.append( thisItem );
  QTimer::singleShot( 0, this, SLOT( deleteItemsToDelete() ) );
  return taken;
}

void KOAgenda::deleteItemsToDelete()
{
  qDeleteAll( mItemsToDelete );
  mItemsToDelete.clear();
}

/*QSizePolicy KOAgenda::sizePolicy() const
{
  // Thought this would make the all-day event agenda minimum size and the
  // normal agenda take the remaining space. But it doesnt work. The QSplitter
  // dont seem to think that an Expanding widget needs more space than a
  // Preferred one.
  // But it doesnt hurt, so it stays.
  if (mAllDayMode) {
    return QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Preferred);
  } else {
    return QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
  }
}
*/

/*
  Overridden from QScrollView to provide proper resizing of KOAgendaItems.
*/
void KOAgenda::resizeEvent ( QResizeEvent *ev )
{
//  kDebug(5850) << "KOAgenda::resizeEvent" << endl;

  QSize newSize( ev->size() );
  if (mAllDayMode) {
    mGridSpacingX = double( newSize.width() - 2 * frameWidth() ) / (double)mColumns;
    mGridSpacingY = newSize.height() - 2 * frameWidth();
  } else {
    mGridSpacingX = double( newSize.width() - verticalScrollBar()->width() - 2 * frameWidth()) / double(mColumns);
    // make sure that there are not more than 24 per day
    mGridSpacingY = double(newSize.height() - 2 * frameWidth()) / double(mRows);
    if ( mGridSpacingY < mDesiredGridSpacingY )
      mGridSpacingY = mDesiredGridSpacingY;
  }
  calculateWorkingHours();
  QTimer::singleShot( 0, this, SLOT( resizeAllContents() ) );
  Q3ScrollView::resizeEvent(ev);
}

void KOAgenda::resizeAllContents()
{
  double subCellWidth;
  KOAgendaItem *item;
  if (mAllDayMode) {
    foreach( item, mItems ) {
      subCellWidth = calcSubCellWidth( item );
      placeAgendaItem( item, subCellWidth );
    }
  } else {
    foreach( item, mItems ) {
      subCellWidth = calcSubCellWidth( item );
      placeAgendaItem( item, subCellWidth );
    }
  }
  checkScrollBoundaries();
  marcus_bains();
}


void KOAgenda::scrollUp()
{
  scrollBy(0,-mScrollOffset);
}


void KOAgenda::scrollDown()
{
  scrollBy(0,mScrollOffset);
}


/*
  Calculates the minimum width
*/
int KOAgenda::minimumWidth() const
{
  // FIXME:: develop a way to dynamically determine the minimum width
  int min = 100;

  return min;
}

void KOAgenda::updateConfig()
{
  double oldGridSpacingY = mGridSpacingY;
  mDesiredGridSpacingY = KOPrefs::instance()->mHourSize;
 // make sure that there are not more than 24 per day
  mGridSpacingY = (double)height()/(double)mRows;
  if (mGridSpacingY<mDesiredGridSpacingY) mGridSpacingY=mDesiredGridSpacingY;

  //can be two doubles equal?, it's better to compare them with an epsilon
  if ( fabs( oldGridSpacingY - mGridSpacingY ) > 0.1 )
    resizeContents( int( mGridSpacingX * mColumns ),
                  int( mGridSpacingY * mRows ) );

  calculateWorkingHours();

  marcus_bains();
}

void KOAgenda::checkScrollBoundaries()
{
  // Invalidate old values to force update
  mOldLowerScrollValue = -1;
  mOldUpperScrollValue = -1;

  checkScrollBoundaries(verticalScrollBar()->value());
}

void KOAgenda::checkScrollBoundaries( int v )
{
  int yMin = int( (v) / mGridSpacingY );
  int yMax = int( ( v + visibleHeight() ) / mGridSpacingY );

//  kDebug(5850) << "--- yMin: " << yMin << "  yMax: " << yMax << endl;

  if ( yMin != mOldLowerScrollValue ) {
    mOldLowerScrollValue = yMin;
    emit lowerYChanged(yMin);
  }
  if ( yMax != mOldUpperScrollValue ) {
    mOldUpperScrollValue = yMax;
    emit upperYChanged(yMax);
  }
}

int KOAgenda::visibleContentsYMin()
{
  int v = verticalScrollBar()->value();
  return int( v / mGridSpacingY );
}

int KOAgenda::visibleContentsYMax()
{
  int v = verticalScrollBar()->value();
  return int( ( v + visibleHeight() ) / mGridSpacingY );
}

void KOAgenda::deselectItem()
{
  if (mSelectedItem.isNull()) return;
  mSelectedItem->select(false);
  mSelectedItem = 0;
}

void KOAgenda::selectItem(KOAgendaItem *item)
{
  if ((KOAgendaItem *)mSelectedItem == item) return;
  deselectItem();
  if (item == 0) {
    emit incidenceSelected( 0 );
    return;
  }
  mSelectedItem = item;
  mSelectedItem->select();
  assert( mSelectedItem->incidence() );
  mSelectedUid = mSelectedItem->incidence()->uid();
  emit incidenceSelected( mSelectedItem->incidence() );
}

void KOAgenda::selectItemByUID( const QString& uid )
{
  KOAgendaItem *item;
  foreach( item, mItems ) {
    if( item->incidence() && item->incidence()->uid() == uid ) {
      selectItem( item );
      break;
    }
  }
}

// This function seems never be called.
void KOAgenda::keyPressEvent( QKeyEvent *kev )
{
  switch(kev->key()) {
    case Qt::Key_PageDown:
      verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepAdd);
      break;
    case Qt::Key_PageUp:
      verticalScrollBar()->triggerAction(QAbstractSlider::SliderPageStepSub);
      break;
    case Qt::Key_Down:
      verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepAdd);
      break;
    case Qt::Key_Up:
      verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
      break;
    default:
      ;
  }
}

void KOAgenda::calculateWorkingHours()
{
  mWorkingHoursEnable = !mAllDayMode;

  QTime tmp = KOPrefs::instance()->mWorkingHoursStart.time();
  mWorkingHoursYTop = int( 4 * mGridSpacingY *
                           ( tmp.hour() + tmp.minute() / 60. +
                             tmp.second() / 3600. ) );
  tmp = KOPrefs::instance()->mWorkingHoursEnd.time();
  mWorkingHoursYBottom = int( 4 * mGridSpacingY *
                              ( tmp.hour() + tmp.minute() / 60. +
                                tmp.second() / 3600. ) - 1 );
}


DateList KOAgenda::dateList() const
{
    return mSelectedDates;
}

void KOAgenda::setDateList(const DateList &selectedDates)
{
    mSelectedDates = selectedDates;
    marcus_bains();
}

void KOAgenda::setHolidayMask(QVector<bool> *mask)
{
  mHolidayMask = mask;

}

void KOAgenda::contentsMousePressEvent ( QMouseEvent *event )
{
  kDebug(5850) << "KOagenda::contentsMousePressEvent(): type: " << event->type() << endl;
  Q3ScrollView::contentsMousePressEvent(event);
}

void KOAgenda::setTypeAheadReceiver( QObject *o )
{
  mTypeAheadReceiver = o;
}

QObject *KOAgenda::typeAheadReceiver() const
{
  return mTypeAheadReceiver;
}

#include <qfile.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qptrlist.h>
#include <qptrstack.h>
#include <qtextstream.h>
#include <qlistview.h>
#include <qtimer.h>

#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>            // i18n
#include <kmessagebox.h>
#include <klineeditdlg.h>
#include <kmessagebox.h>

#include "desktoptracker.h"
#include "edittaskdialog.h"
#include "idletimedetector.h"
#include "preferences.h"
#include "task.h"
#include "taskview.h"
#include "karmstorage.h"

#define T_LINESIZE 1023
#define HIDDEN_COLUMN -10

class DesktopTracker;

TaskView::TaskView(QWidget *parent, const char *name):KListView(parent,name)
{
  _preferences = Preferences::instance();
  _storage = KarmStorage::instance();

  connect(this, SIGNAL( doubleClicked( QListViewItem * )),
          this, SLOT( changeTimer( QListViewItem * )));

  // setup default values
  previousColumnWidths[0] = previousColumnWidths[1]
  = previousColumnWidths[2] = previousColumnWidths[3] = HIDDEN_COLUMN;

  addColumn( i18n("Task Name") );
  addColumn( i18n("Session Time") );
  addColumn( i18n("Time") );
  addColumn( i18n("Total Session Time") );
  addColumn( i18n("Total Time") );
  // setColumnAlignment( 1, Qt::AlignRight );
  // setColumnAlignment( 2, Qt::AlignRight );
  // setColumnAlignment( 3, Qt::AlignRight );
  adaptColumns();
  setAllColumnsShowFocus( true );

  // set up the minuteTimer
  _minuteTimer = new QTimer(this);
  connect( _minuteTimer, SIGNAL( timeout() ), this, SLOT( minuteUpdate() ));
  _minuteTimer->start(1000 * secsPerMinute);

  // React when user changes iCalFile
  connect(_preferences, SIGNAL(iCalFile(QString)), 
      this, SLOT(iCalFileChanged(QString)));

  // resize columns when config is changed
  connect(_preferences, SIGNAL( setupChanged() ), this,SLOT( adaptColumns() ));

  _minuteTimer->start(1000 * secsPerMinute);

  // Set up the idle detection.
  _idleTimeDetector = new IdleTimeDetector( _preferences->idlenessTimeout() );
  connect( _idleTimeDetector, SIGNAL( extractTime(int) ),
           this, SLOT( extractTime(int) ));
  connect( _idleTimeDetector, SIGNAL( stopAllTimers() ),
           this, SLOT( stopAllTimers() ));
  connect( _preferences, SIGNAL( idlenessTimeout(int) ),
           _idleTimeDetector, SLOT( setMaxIdle(int) ));
  connect( _preferences, SIGNAL( detectIdleness(bool) ),
           _idleTimeDetector, SLOT( toggleOverAllIdleDetection(bool) ));
  if (!_idleTimeDetector->isIdleDetectionPossible())
    _preferences->disableIdleDetection();

  // Setup auto save timer
  _autoSaveTimer = new QTimer(this);
  connect( _preferences, SIGNAL( autoSave(bool) ),
           this, SLOT( autoSaveChanged(bool) ));
  connect( _preferences, SIGNAL( autoSavePeriod(int) ),
           this, SLOT( autoSavePeriodChanged(int) ));
  connect( _autoSaveTimer, SIGNAL( timeout() ), this, SLOT( save() ));

  // Connect desktop tracker events to task starting/stopping
  _desktopTracker = new DesktopTracker();
  connect( _desktopTracker, SIGNAL( reachedtActiveDesktop( Task* ) ),
           this, SLOT( startTimerFor(Task*) ));
  connect( _desktopTracker, SIGNAL( leftActiveDesktop( Task* ) ),
           this, SLOT( stopTimerFor(Task*) ));
}

TaskView::~TaskView()
{
  _preferences->save();
}

Task* TaskView::first_child() const
{
  return static_cast<Task*>(firstChild());
}

Task* TaskView::current_item() const
{
  return static_cast<Task*>(currentItem());
}

Task* TaskView::item_at_index(int i)
{
  return static_cast<Task*>(itemAtIndex(i));
}

void TaskView::load()
{

  QString err = _storage->load(this, _preferences);

  if (!err.isEmpty())
  {
    KMessageBox::error(this, err);
    return;
  }

  // TODO: If empty, ask if user wants to import tasks from another file
  //if (_storage.isEmpty())
  //{
  //}

  // Register tasks with desktop tracker
  int task_idx = 0;
  for (Task* task = item_at_index(task_idx);
      task; 
      task = item_at_index(++task_idx))
  {
    _desktopTracker->registerForDesktops( task, task->getDesktops() );
  }

  setSelected(first_child(), true);
  setCurrentItem(first_child());
  _desktopTracker->startTracking();
}

void TaskView::loadFromFlatFile()
{
  kdDebug() << "TaskView::loadFromFlatFile()" << endl;

  // FIXME: replace with a file open dialog
  QString msg = QString(QString::fromLatin1("You are going to add all tasks from %1 (defined in the KArm preferences) to the current task view.  Are you sure you want to do this?")).arg(_preferences->flatFile());

  int answer = KMessageBox::warningContinueCancel(this, msg);
  if (answer == KMessageBox::Continue)
  {
    QString err = _storage->loadFromFlatFile(this, _preferences);
    if (!err.isEmpty())
    {
      KMessageBox::error(this, err);
      return;
    }

    // Register tasks with desktop tracker
    int task_idx = 0;
    Task* task = item_at_index(task_idx++);
    while (task)
    {
      // item_at_index returns 0 where no more items.
      _desktopTracker->registerForDesktops( task, task->getDesktops() );
      task = item_at_index(task_idx++);
    }

    setSelected(first_child(), true);
    setCurrentItem(first_child());

    _desktopTracker->startTracking();
  }
}

void TaskView::save()
{
  _storage->save(this);
}

void TaskView::startCurrentTimer()
{
  startTimerFor( current_item() );
}

long TaskView::count()
{
  long n = 0;
  for (Task* t = item_at_index(n); t; t=item_at_index(++n));
  return n;
}

void TaskView::startTimerFor(Task* task)
{
  if (task != 0 && activeTasks.findRef(task) == -1) {
    _idleTimeDetector->startIdleDetection();
    task->setRunning(true, _storage);
    activeTasks.append(task);
    emit updateButtons();
    if ( activeTasks.count() == 1 )
        emit timersActive();

    emit tasksChanged( activeTasks);
  }
}

void TaskView::stopAllTimers()
{
  for (unsigned int i=0; i<activeTasks.count();i++) {
    activeTasks.at(i)->setRunning(false, _storage);
  }
  _idleTimeDetector->stopIdleDetection();
  activeTasks.clear();
  emit updateButtons();
  emit timersInactive();
  emit tasksChanged( activeTasks);
}

void TaskView::startNewSession()
{
  QListViewItemIterator item( first_child());
  for ( ; item.current(); ++item ) {
    Task * task = (Task *) item.current();
    task->startNewSession();
  }
}

void TaskView::resetTimeForAllTasks()
{
  QListViewItemIterator item( first_child());
  for ( ; item.current(); ++item ) {
    Task * task = (Task *) item.current();
    task->resetTimes();
  }
}

void TaskView::stopTimerFor(Task* task)
{
  if (task != 0 && activeTasks.findRef(task) != -1) {
    activeTasks.removeRef(task);
    task->setRunning(false, _storage);
    if (activeTasks.count()== 0) {
      _idleTimeDetector->stopIdleDetection();
      emit timersInactive();
    }
    emit updateButtons();
  }
  emit tasksChanged( activeTasks);
}

void TaskView::stopCurrentTimer()
{
  stopTimerFor( current_item());
}


void TaskView::changeTimer(QListViewItem *)
{
  Task *task = current_item();
  if (task != 0 && activeTasks.findRef(task) == -1) {
    // Stop all the other timers.
    for (unsigned int i=0; i<activeTasks.count();i++) {
      (activeTasks.at(i))->setRunning(false, _storage);
    }
    activeTasks.clear();

    // Start the new timer.
    startCurrentTimer();
  }
  else {
    stopCurrentTimer();
  }
}

void TaskView::minuteUpdate()
{
  addTimeToActiveTasks(1, false);
}

void TaskView::addTimeToActiveTasks(int minutes, bool do_logging)
{
  for(unsigned int i=0; i<activeTasks.count();i++)
    activeTasks.at(i)->changeTime(minutes, do_logging, _storage);
}

void TaskView::newTask()
{
  newTask(i18n("New Task"), 0);
}

void TaskView::newTask(QString caption, Task *parent)
{
  EditTaskDialog *dialog = new EditTaskDialog(caption, false);
  long total, totalDiff, session, sessionDiff;
  DesktopList desktopList;
  Task *task;

  int result = dialog->exec();
  if (result == QDialog::Accepted) {
    QString taskName = i18n("Unnamed Task");
    if (!dialog->taskName().isEmpty()) {
      taskName = dialog->taskName();
    }

    total = totalDiff = session = sessionDiff = 0;
    dialog->status( &total, &totalDiff, &session, &sessionDiff, &desktopList);

    // If all available desktops are checked, disable auto tracking,
    // since it makes no sense to track for every desktop.
    if (desktopList.size() == (unsigned int)_desktopTracker->desktopCount())
      desktopList.clear();

    if (parent == 0)
    {
      task = new Task(taskName, total, session, desktopList, this);
      task->setUid(_storage->addTask(task, 0));
    }
    else
    {
      task = new Task(taskName, total, session, desktopList, parent);
      task->setUid(_storage->addTask(task, parent));
    }

    if (task->uid())
    {
      _desktopTracker->registerForDesktops( task, desktopList );

      setCurrentItem( task );
      setSelected( task, true );

      save();
    }
    else
    {
      delete task;
      KMessageBox::error(0,i18n(
            "Error storing new task--your changes were not saved."));
    }
  }

  delete dialog;
}

void TaskView::newSubTask()
{
  Task* task = current_item();
  if(!task)
    return;
  newTask(i18n("New Sub Task"), task);
  task->setOpen(true);
  setRootIsDecorated(true);
}

void TaskView::editTask()
{
  Task *task = current_item();
  if (!task)
    return;

  DesktopList desktopList = task->getDesktops();
  EditTaskDialog *dialog = new EditTaskDialog(i18n("Edit Task"), true, &desktopList);
  dialog->setTask( task->name(),
                   task->time(),
                   task->sessionTime() );
  int result = dialog->exec();
  if (result == QDialog::Accepted) {
    QString taskName = i18n("Unnamed Task");
    if (!dialog->taskName().isEmpty()) {
      taskName = dialog->taskName();
    }
    // setName only does something if the new name is different
    task->setName(taskName, _storage);

    // update session time as well if the time was changed
    long total, session, totalDiff, sessionDiff;
    total = totalDiff = session = sessionDiff = 0;
    DesktopList desktopList;
    dialog->status( &total, &totalDiff, &session, &sessionDiff, &desktopList);

    if( totalDiff != 0 || sessionDiff != 0)
      task->changeTimes( sessionDiff ,totalDiff, true, _storage );

    // If all available desktops are checked, disable auto tracking,
    // since it makes no sense to track for every desktop.
    if (desktopList.size() == (unsigned int)_desktopTracker->desktopCount())
      desktopList.clear();

    task->setDesktopList(desktopList);

    _desktopTracker->registerForDesktops( task, desktopList );

    emit updateButtons();
  }
  delete dialog;
}

void TaskView::addCommentToTask()
{
  Task *task = current_item();
  if (!task)
    return;

  bool ok;
  QString comment = KLineEditDlg::getText(
                       i18n("Log comment for task %1").arg(task->name()),
                       QString(), &ok, this);
  if ( ok )
    task->addComment( comment, _storage );
}


void TaskView::deleteTask()
{
  Task *task = current_item();
  if (task == 0) {
    KMessageBox::information(0,i18n("No task selected"));
    return;
  }

  int response = KMessageBox::Yes;
  if ( _preferences->promptDelete() ) {
    if (task->childCount() == 0) {
      response = KMessageBox::questionYesNo( 0,
                  i18n( "Are you sure you want to delete "
                        "the task named\n\"%1\"").arg(task->name()),
                  i18n( "Deleting Task"));
    }
    else {
      response = KMessageBox::questionYesNo( 0,
                  i18n( "Are you sure you want to delete the task named"
                        "\n\"%1\"\n" "NOTE: all its subtasks will also "
                        "be deleted!").arg(task->name()),
                  i18n( "Deleting Task"));
    }
  }

  if (response == KMessageBox::Yes) {

    if (task->remove( activeTasks, _storage))
    {
      
      // remove root decoration if there is no more children.
      bool anyChilds = false;
      for(Task* child = first_child();
                child;
                child = child->nextSibling()) {
        if (child->childCount() != 0) {
          anyChilds = true;
          break;
        }
      }
      if (!anyChilds) {
        setRootIsDecorated(false);
      }

      // Stop idle detection if no more counters are running
      if (activeTasks.count() == 0) {
        _idleTimeDetector->stopIdleDetection();
        emit timersInactive();
      }
      emit tasksChanged( activeTasks );

      save();
    }
    else
      KMessageBox::error(0,i18n(
            "Error deleting task--libkcal doesn't support this yet."));
  }

}

void TaskView::extractTime(int minutes)
{
  addTimeToActiveTasks(-minutes, true);
}

void TaskView::autoSaveChanged(bool on)
{
  if (on) {
    if (!_autoSaveTimer->isActive()) {
      _autoSaveTimer->start(_preferences->autoSavePeriod()*1000*secsPerMinute);
    }
  }
  else {
    if (_autoSaveTimer->isActive()) {
      _autoSaveTimer->stop();
    }
  }
}

void TaskView::autoSavePeriodChanged(int /*minutes*/)
{
  autoSaveChanged(_preferences->autoSave());
}

void TaskView::adaptColumns()
{
  // to hide a column X we set it's width to 0
  // at that moment we'll remember the original column within
  // previousColumnWidths[X]
  //
  // When unhiding a previously hidden column
  // (previousColumnWidths[X] != HIDDEN_COLUMN !)
  // we restore it's width from the saved value and set
  // previousColumnWidths[X] to HIDDEN_COLUMN

  for( int x=1; x <= 4; x++) {
    // the column was invisible before and were switching it on now
    if(   _preferences->displayColumn(x-1)
       && previousColumnWidths[x-1] != HIDDEN_COLUMN )
    {
      setColumnWidth( x, previousColumnWidths[x-1] );
      previousColumnWidths[x-1] = HIDDEN_COLUMN;
      setColumnWidthMode( x, QListView::Maximum );
    }
    // the column was visible before and were switching it off now
    else
    if( ! _preferences->displayColumn(x-1)
       && previousColumnWidths[x-1] == HIDDEN_COLUMN )
    {
      setColumnWidthMode( x, QListView::Manual ); // we don't want update()
                                                  // to resize/unhide the col
      previousColumnWidths[x-1] = columnWidth( x );
      setColumnWidth( x, 0 );
    }
  }
}

void TaskView::deletingTask(Task* deletedTask)
{
  DesktopList desktopList;

  _desktopTracker->registerForDesktops( deletedTask, desktopList );
  activeTasks.removeRef( deletedTask );

  emit tasksChanged( activeTasks);
}

void TaskView::iCalFileChanged(QString file)
{
  kdDebug() << "TaskView:iCalFileChanged: " << file << endl;
  load();
}
#include "taskview.moc"

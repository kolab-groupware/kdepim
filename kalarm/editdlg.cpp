/*
 *  editdlg.cpp  -  dialogue to create or modify an alarm message
 *  Program:  kalarm
 *  (C) 2001 by David Jarvie  software@astrojar.org.uk
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 */

#include "kalarm.h"

#include <limits.h>

#include <qlayout.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qmultilinedit.h>
#include <qlabel.h>
#include <qmsgbox.h>
#include <qvalidator.h>
#include <qwhatsthis.h>

#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <kmessagebox.h>
#include <kdebug.h>

#include "kalarmapp.h"
#include "prefsettings.h"
#include "editdlg.h"
#include "editdlg.moc"


EditAlarmDlg::EditAlarmDlg(const QString& caption, QWidget* parent, const char* name, const MessageEvent* event)
	: KDialogBase(parent, name, true, caption, Ok|Cancel, Ok, true)
{
	QWidget* page = new QWidget(this);
	setMainWidget(page);
	QVBoxLayout* topLayout = new QVBoxLayout(page, marginHint(), spacingHint());

	// Message label + multi-line editor

	QLabel* lbl = new QLabel(page);
	lbl->setText(i18n("Message:"));
	lbl->setFixedSize(lbl->sizeHint());
	topLayout->addWidget(lbl, 0, AlignLeft);
	topLayout->addSpacing(fontMetrics().lineSpacing()/2 - spacingHint());

	messageEdit = new QMultiLineEdit(page);
	QSize size = messageEdit->sizeHint();
	size.setHeight(messageEdit->fontMetrics().lineSpacing()*13/4 + 2*messageEdit->frameWidth());
	messageEdit->setMinimumSize(size);
	topLayout->addWidget(messageEdit, 6);
	QWhatsThis::add(messageEdit,
	      i18n("Enter the text of the alarm message.\n"
	           "It may be multi-line."));

	// Date label + spin box

	QGridLayout* grid = new QGridLayout(1, 4);
	topLayout->addLayout(grid);

	lbl = new QLabel(page);
	lbl->setText(i18n("Date:"));
	lbl->setFixedSize(lbl->sizeHint());
	grid->addWidget(lbl, 0, 0, AlignLeft);

	dateEdit = new DateSpinBox(page);
	size = dateEdit->sizeHint();
	dateEdit->setFixedSize(size);
	grid->addWidget(dateEdit, 0, 1, AlignLeft);
	QWhatsThis::add(dateEdit, i18n("Enter the date to schedule the alarm message."));

	// Time label + spin box

	lbl = new QLabel(page);
	lbl->setText(i18n("Time:"));
	lbl->setFixedSize(lbl->sizeHint());
	grid->addWidget(lbl, 0, 2, AlignRight);

	timeEdit = new TimeSpinBox(page);
	timeEdit->setValue(2399);
	size = timeEdit->sizeHint();
	timeEdit->setFixedSize(size);
	grid->addWidget(timeEdit, 0, 3, AlignRight);
	QWhatsThis::add(timeEdit, i18n("Enter the time to schedule the alarm message."));

	// Repeating alarm

	QGroupBox* group = new QGroupBox(4, Qt::Horizontal, i18n("Repetition"), page);
	topLayout->addWidget(group);
	lbl = new QLabel(group);
	lbl->setText(i18n("Count:"));
	lbl->setFixedSize(lbl->sizeHint());

	repeatCount = new QSpinBox(0, 9999, 1, group);
	repeatCount->setFixedSize(repeatCount->sizeHint());
	QWhatsThis::add(repeatCount,
	      i18n("Enter the number of times to repeat the alarm,\n"
	           "after its initial display."));
	connect(repeatCount, SIGNAL(valueChanged(int)), this, SLOT(slotRepeatCountChanged(int)));

	lbl = new QLabel(group);
	lbl->setText(i18n("Interval:"));
	lbl->setFixedSize(lbl->sizeHint());

	repeatInterval = new TimeSpinBox(1, 99*60+59, group);
	repeatInterval->setValue(2399);
	size = repeatInterval->sizeHint();
	repeatInterval->setFixedSize(size);
	QWhatsThis::add(repeatInterval,
	      i18n("Enter the time (in hours and minutes)\n"
	           "between repetitions of the alarm."));

	// Late display checkbox - default = allow late display

	grid = new QGridLayout(1, 2);
	topLayout->addLayout(grid);

	lateCancel = new QCheckBox(page);
	lateCancel->setText(i18n("Cancel if late"));
	lateCancel->setFixedSize(lateCancel->sizeHint());
	lateCancel->setChecked(false);
	grid->addWidget(lateCancel, 0, 0, AlignLeft);
	QWhatsThis::add(lateCancel,
	      i18n("If checked, the message will be cancelled if it\n"
	           "cannot be displayed within 1 minute of the specified\n"
	           "time. Possible reasons for non-display include your\n"
	           "being logged off, X not running, or the alarm daemon\n"
	           "not running.\n\n"
	           "If unchecked, the message will be displayed at the\n"
	           "first opportunity after the specified time, regardless\n"
	           "of how late it is."));

	// Beep checkbox - default = no beep

	beep = new QCheckBox(page);
	beep->setText(i18n("Beep"));
	beep->setFixedSize(beep->sizeHint());
	beep->setChecked(false);
	grid->addWidget(beep, 0, 1, AlignLeft);
	QWhatsThis::add(beep,
	      i18n("If checked, a beep will sound when the message is\n"
	           "displayed."));

#ifdef SELECT_FONT
	// Font and colour choice drop-down list

	fontColour = new FontColourChooser(page, 0L, false, QStringList(), true, i18n("Font and background colour"), false);
	size = fontColour->sizeHint();
	fontColour->setMinimumHeight(size.height() + 4);
	topLayout->addWidget(fontColour, 6);
	QWhatsThis::add(fontColour,
	      i18n("Choose the font and background colour for the alarm message."));
#else
	// Colour choice drop-down list

	bgColourChoose = new ColourCombo(page);
	size = bgColourChoose->sizeHint();
	bgColourChoose->setMinimumHeight(size.height() + 4);
	topLayout->addWidget(bgColourChoose, 6);
	QWhatsThis::add(bgColourChoose,
	      i18n("Choose the background colour for the alarm message."));
#endif

	setButtonText(Ok, i18n("&Set Alarm"));
	setButtonWhatsThis(Ok, i18n("Schedule the message for display at the specified time."));

	topLayout->activate();

	KConfig* config = KGlobal::config();
	config->setGroup(QString::fromLatin1("EditDialog"));
	size = minimumSize();
	QWidget* desktop = KApplication::desktop();
	size.setWidth(config->readNumEntry(QString::fromLatin1("Width %1").arg(desktop->width()), size.width()));
	size.setHeight(config->readNumEntry(QString::fromLatin1("Height %1").arg(desktop->height()), size.height()));
	resize(size);

	// Set up initial values
	if (event)
	{
		messageEdit->setText(event->message());
//		messageEdit->insertLine(event->message());
		timeEdit->setValue(event->time().hour()*60 + event->time().minute());
		dateEdit->setDate(event->date());
		lateCancel->setChecked(event->lateCancel());
		beep->setChecked(event->beep());
		repeatCount->setValue(event->repeatCount());
		repeatInterval->setValue(event->repeatMinutes());
#ifdef SELECT_FONT
		fontColour->setColour(event->colour());
		fontColour->setFont(?);
#else
		bgColourChoose->setColour(event->colour());
#endif
	}
	else
	{
		messageEdit->setText("");
		QDateTime now = QDateTime::currentDateTime();
		timeEdit->setValue(now.time().hour()*60 + now.time().minute());
		dateEdit->setDate(now.date());
		repeatCount->setValue(0);
		repeatInterval->setValue(0);
#ifdef SELECT_FONT
		fontColour->setColour(theApp()->generalSettings()->defaultBgColour());
		fontColour->setFont(theApp()->generalSettings()->messageFont());
#else
		bgColourChoose->setColour(theApp()->generalSettings()->defaultBgColour());
#endif
	}
	repeatInterval->setEnabled(repeatCount->value());
	messageEdit->setFocus();
}


EditAlarmDlg::~EditAlarmDlg()
{
}


/******************************************************************************
 * Get the currently entered message data.
 * The data is returned in the supplied Event instance.
 */
void EditAlarmDlg::getEvent(MessageEvent& event)
{
	int flags = (lateCancel->isChecked() ? MessageEvent::LATE_CANCEL : 0)
	          | (beep->isChecked() ? MessageEvent::BEEP : 0);
	event.set(alarmDateTime, flags, bgColourChoose->color(), alarmMessage);
	event.setRepetition(repeatInterval->value(), repeatCount->value());
}

/******************************************************************************
*  Called when the window's size has changed (before it is painted).
*  Sets the last column in the list view to extend at least to the right
*  hand edge of the list view.
*/
void EditAlarmDlg::resizeEvent(QResizeEvent* re)
{
	KConfig* config = KGlobal::config();
	config->setGroup(QString::fromLatin1("EditDialog"));
	config->writeEntry("Size", re->size());
	QWidget* desktop = KApplication::desktop();
	config->writeEntry(QString::fromLatin1("Width %1").arg(desktop->width()), re->size().width());
	config->writeEntry(QString::fromLatin1("Height %1").arg(desktop->height()), re->size().height());
	KDialog::resizeEvent(re);
}


void EditAlarmDlg::slotOk()
{
	alarmDateTime.setTime(QTime(timeEdit->value()/60, timeEdit->value()%60));
	alarmDateTime.setDate(dateEdit->getDate());
	if (alarmDateTime < QDateTime::currentDateTime())
		KMessageBox::sorry(this, i18n("Message time has already expired"));
	else
	{
		alarmMessage = messageEdit->text();
		alarmMessage.stripWhiteSpace();
		if (alarmMessage.isEmpty())
			alarmMessage = "Alarm";
		accept();
	}
}

void EditAlarmDlg::slotCancel()
{
	reject();
}

/******************************************************************************
*  Called when the repeat count edit box value has changed..
*/
void EditAlarmDlg::slotRepeatCountChanged(int count)
{
	repeatInterval->setEnabled(count);
}

/*=============================================================================
=  class TimeSpinBox
=============================================================================*/
class TimeSpinBox::TimeValidator : public QValidator
{
	public:
		TimeValidator(int minMin, int maxMin, QWidget* parent, const char* name = 0L)
			: QValidator(parent, name), minMinute(minMin), maxMinute(maxMin) { }
		virtual State validate(QString&, int&) const;
		int  minMinute, maxMinute;
};

/******************************************************************************
 * Validate the time spin box input.
 * The entered time must contain a colon, but hours and/or minutes may be blank.
 */
QValidator::State TimeSpinBox::TimeValidator::validate(QString& text, int& /*cursorPos*/) const
{
	QValidator::State state = QValidator::Acceptable;
	QString hour;
	int hr;
	int mn = 0;
	int colon = text.find(':');
	if (colon >= 0)
	{
		QString minute = text.mid(colon + 1).stripWhiteSpace();
		if (minute.isEmpty())
			state = QValidator::Intermediate;
		else
		{
			bool ok;
			if ((mn = minute.toUInt(&ok)) >= 60  ||  !ok)
				return QValidator::Invalid;
		}

		hour = text.left(colon).stripWhiteSpace();
	}
	else
	{
		state = QValidator::Intermediate;
		hour = text;
	}

	if (hour.isEmpty())
		return QValidator::Intermediate;
	bool ok;
	if ((hr = hour.toUInt(&ok)) > maxMinute/60  ||  !ok)
		return QValidator::Invalid;
	if (state == QValidator::Acceptable)
	{
		int t = hr * 60 + mn;
		if (t < minMinute  ||  t > maxMinute)
			return QValidator::Invalid;
	}
	return state;
}


// Construct a wrapping 00:00 - 23:59 time spin box
TimeSpinBox::TimeSpinBox(QWidget* parent, const char* name)
	: SpinBox2(0, 1439, 1, 60, parent, name)
{
	validator = new TimeValidator(0, 1439, this, "TimeSpinBox validator");
	setValidator(validator);
	setWrapping(true);
}

// Construct a non-wrapping time spin box
TimeSpinBox::TimeSpinBox(int minMinute, int maxMinute, QWidget* parent, const char* name)
	: SpinBox2(minMinute, maxMinute, 1, 60, parent, name)
{
	validator = new TimeValidator(minMinute, maxMinute, this, "TimeSpinBox validator");
	setValidator(validator);
}

QString TimeSpinBox::mapValueToText(int v)
{
	QString s;
	s.sprintf("%02d:%02d", v/60, v%60);
	return s;
}

/******************************************************************************
 * Convert the user-entered text to a value in minutes.
 * The allowed format is [hour]:[minute], where hour and
 * minute must be non-blank.
 */
int TimeSpinBox::mapTextToValue(bool* ok)
{
	QString text = cleanText();
	int colon = text.find(':');
	if (colon >= 0)
	{
		QString hour   = text.left(colon).stripWhiteSpace();
		QString minute = text.mid(colon + 1).stripWhiteSpace();
		if (!hour.isEmpty()  &&  !minute.isEmpty())
		{
			bool okhour, okmin;
			int m = minute.toUInt(&okmin);
			int t = hour.toUInt(&okhour) * 60 + m;
			if (okhour  &&  okmin  &&  m < 60  &&  t >= minValue()  &&  t <= maxValue())
			{
				*ok = true;
				return t;
			}
		}
	}
	*ok = false;
	return 0;
}


QDate  DateSpinBox::baseDate(2000, 1, 1);


DateSpinBox::DateSpinBox(QWidget* parent, const char* name)
	: QSpinBox(0, 0, 1, parent, name)
{
	QDate now = QDate::currentDate();
	QDate maxDate(now.year() + 100, 12, 31);
	setRange(0, baseDate.daysTo(maxDate));
}

QDate DateSpinBox::getDate()
{
	return baseDate.addDays(value());
}

void DateSpinBox::setDate(const QDate& date)
{
	setValue(baseDate.daysTo(date));
}

QString DateSpinBox::mapValueToText(int v)
{
	QDate date = baseDate.addDays(v);
	return KGlobal::locale()->formatDate(date, true);
}

/*
 * Convert the user-entered text to a value in days.
 * The date must be in the range
 */
int DateSpinBox::mapTextToValue(bool* ok)
{
	QDate date = KGlobal::locale()->readDate(cleanText());
	int days = baseDate.daysTo(date);
	int minval = baseDate.daysTo(QDate::currentDate());
	if (days >= minval  &&  days <= maxValue())
	{
		*ok = true;
		return days;
	}
	*ok = false;
	return 0;
}

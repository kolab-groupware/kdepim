/*
 *  main.cpp
 *  Program:  kalarm
 *  (C) 2001, 2002, 2003 by David Jarvie <software@astrojar.org.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 *  As a special exception, permission is given to link this program
 *  with any edition of Qt, and distribute the resulting executable,
 *  without including the source code for Qt in the source distribution.
 */

#include "kalarm.h"

#include <stdlib.h>

#include <kcmdlineargs.h>
#include <kaboutdata.h>
#include <klocale.h>
#include <kdebug.h>

#include "kalarmapp.h"

#define PROGRAM_NAME "kalarm"

QCString execArguments;    // argument to --exec option

static KCmdLineOptions options[] =
{
	{ "a", 0, 0 },
	{ "ack-confirm", I18N_NOOP("Prompt for confirmation when alarm is acknowledged"), 0 },
	{ "A", 0, 0 },
	{ "attach <url>", I18N_NOOP("Attach file to email (repeat as needed)"), 0 },
	{ "bcc", I18N_NOOP("Blind copy email to self"), 0 },
	{ "b", 0, 0 },
	{ "beep", I18N_NOOP("Beep when message is displayed"), 0 },
	{ "colour", 0, 0 },
	{ "c", 0, 0 },
	{ "color <color>", I18N_NOOP("Message background color (name or hex 0xRRGGBB)"), 0 },
	{ "colourfg", 0, 0 },
	{ "C", 0, 0 },
	{ "colorfg <color>", I18N_NOOP("Message foreground color (name or hex 0xRRGGBB)"), 0 },
	{ "calendarURL <url>", I18N_NOOP("URL of calendar file"), 0 },
	{ "cancelEvent <eventID>", I18N_NOOP("Cancel alarm with the specified event ID"), 0 },
	{ "e", 0, 0 },
	{ "exec <commandline>", I18N_NOOP("Execute a shell command line"), 0 },
	{ "f", 0, 0 },
	{ "file <url>", I18N_NOOP("File to display"), 0 },
	{ "handleEvent <eventID>", I18N_NOOP("Trigger or cancel alarm with the specified event ID"), 0 },
	{ "i", 0, 0 },
	{ "interval <period>", I18N_NOOP("Interval between alarm recurrences"), 0 },
	{ "l", 0, 0 },
	{ "late-cancel", I18N_NOOP("Cancel alarm if it cannot be triggered on time"), 0 },
	{ "L", 0, 0 },
	{ "login", I18N_NOOP("Repeat alarm at every login"), 0 },
	{ "m", 0, 0 },
	{ "mail <address>", I18N_NOOP("Send an email to the given address (repeat as needed)"), 0 },
	{ "recurrence <spec>", I18N_NOOP("Specify alarm recurrence using iCalendar syntax"), 0 },
	{ "R", 0, 0 },
	{ "reminder <period>", I18N_NOOP("Display reminder in advance of alarm"), 0 },
	{ "r", 0, 0 },
	{ "repeat <count>", I18N_NOOP("Number of times to repeat alarm (after the initial occasion)"), 0 },
	{ "reset", I18N_NOOP("Reset the alarm scheduling daemon"), 0 },
	{ "s", 0, 0 },
	{ "sound <url>", I18N_NOOP("Audio file to play"), 0 },
	{ "stop", I18N_NOOP("Stop the alarm scheduling daemon"), 0 },
	{ "S", 0, 0 },
	{ "subject ", I18N_NOOP("Email subject line"), 0 },
	{ "t", 0, 0 },
	{ "time <time>", I18N_NOOP("Trigger alarm at time [[[yyyy-]mm-]dd-]hh:mm, or date yyyy-mm-dd"), 0 },
	{ "tray", I18N_NOOP("Display system tray icon"), 0 },
	{ "u", 0, 0 },
	{ "until <time>", I18N_NOOP("Repeat until time [[[yyyy-]mm-]dd-]hh:mm, or date yyyy-mm-dd"), 0 },
	{ "displayEvent <eventID>", I18N_NOOP("Obsolete: use --triggerEvent instead"), 0 },
	{ "triggerEvent <eventID>", I18N_NOOP("Trigger alarm with the specified event ID"), 0 },
	{ "+[message]", I18N_NOOP("Message text to display"), 0 },
	KCmdLineLastOption
};


int main(int argc, char *argv[])
{
	KAboutData aboutData(PROGRAM_NAME, I18N_NOOP("KAlarm"), KALARM_VERSION,
		I18N_NOOP("Personal alarm message, command and email scheduler for KDE"),
		KAboutData::License_GPL,
		"(c) 2001 - 2003, David Jarvie", 0, "http://www.astrojar.org.uk/linux/kalarm.html");
	aboutData.addAuthor("David Jarvie", 0, "software@astrojar.org.uk");

	// Fetch all command line options/arguments after --exec and concatenate
	// them into a single argument. Then change the leading '-'.
	// This is necessary because the "!" indicator in the 'options'
	// array above doesn't work (on KDE2, at least)
	for (int i = 0;  i < argc;  ++i)
	{
		if (!strcmp(argv[i], "-e") || !strcmp(argv[i], "--exec"))
		{
			while (++i < argc)
			{
				execArguments += argv[i];
				if (i < argc - 1)
					execArguments += ' ';
				argv[i][0] = 'x';     // in case it's an option
			}
			break;
		}
	}
	KCmdLineArgs::init(argc, argv, &aboutData);
	KCmdLineArgs::addCmdLineOptions(options);
	KUniqueApplication::addCmdLineOptions();

	if (!KAlarmApp::start())
	{
		// An instance of the application is already running
		exit(0);
	}

	// This is the first time through
	kdDebug(5950) << "main(): initialising\n";
	KAlarmApp* app = KAlarmApp::getInstance();
	app->restoreSession();
	return app->exec();
}

/*
	Empath - Mailer for KDE
	
	Copyright (C) 1998 Rik Hemsley rik@kde.org
	
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
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <qstring.h>

#include <RMM_Group.h>
#include <RMM_Token.h>

RGroup::RGroup()
{
	rmmDebug("ctor");
}

RGroup::RGroup(const RGroup & g)
	:	RAddress()
{
	rmmDebug("ctor");
}

RGroup::~RGroup()
{
	rmmDebug("dtor");
}

	RGroup &
RGroup::operator = (const RGroup & g)
{
	rmmDebug("operator =");
	
	if (this == &g) return *this;
	
	mailboxList_	= g.mailboxList_;
	name_			= g.name_;
	phrase_			= g.phrase_;
	
	RAddress::operator = (g);

	return *this;
}

	QDataStream &
operator >> (QDataStream & s, RGroup & group)
{
	s	>> group.name_
		>> group.phrase_;
	return s;
}
	
	QDataStream &
operator << (QDataStream & s, RGroup & group)
{
	s	<< group.name_
		<< group.phrase_;
	return s;
}



	const QCString &
RGroup::name()
{
	return name_;
}

	const QCString &
RGroup::phrase()
{
	return phrase_;
}

	void
RGroup::setName(const QCString & s)
{
	name_ = s;
}

	void
RGroup::setPhrase(const QCString & s)
{
	phrase_ = s;
}

	RMailboxList &
RGroup::mailboxList()
{
	return mailboxList_;
}

	void
RGroup::parse()
{
	rmmDebug("parse() called");
}

	void
RGroup::assemble()
{
	rmmDebug("assemble() called");
}

	void
RGroup::createDefault()
{
	rmmDebug("createDefault() called");
	name_ = "unnamed";
}


/***************************************************************************
                          kngrouppropdlg.cpp  -  description
                             -------------------

    copyright            : (C) 1999 by Christian Thurner
    email                : cthurner@freepage.de
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <klocale.h>

#include "kngrouppropdlg.h"
#include "utilities.h"
#include "kngroup.h"
#include <qlayout.h>


KNGroupPropDlg::KNGroupPropDlg(KNGroup *group, QWidget *parent, const char *name )
	: QTabDialog(parent,name, true)
{
	grp=group;
	nChanged=false;
	setCancelButton(i18n("Cancel"));
	setOkButton(i18n("OK"));
	
	int v[5];
	v[0]=grp->count();
	v[1]=grp->count()-grp->readCount();
	v[2]=grp->newCount();
	v[3]=grp->statThrWithUnread();
	v[4]=grp->statThrWithNew();
	
	sta=new statistics(this, 0, v);
	set=new settings(this);
	
	if(grp->hasName()) set->nick->setText(grp->name());
	
	if(grp->user()) set->uw->setData(grp->user());
	
	addTab(sta, i18n("&Statistics"));
	addTab(set, i18n("Se&ttings"));
	
	QString tmp=i18n("Properties of ");
	tmp+=grp->name();
	
	setCaption(tmp);	
  restoreWindowSize("groupPropDLG", this, sizeHint());
}



KNGroupPropDlg::~KNGroupPropDlg()
{
	saveWindowSize("groupPropDLG", size());
}



void KNGroupPropDlg::apply()
{
	if( !(grp->name()==set->nick->text()) ) {
		grp->setName(set->nick->text());
		nChanged=true;
	}
	set->uw->applyData();	
}


//=============================================================================

KNGroupPropDlg::statistics::statistics(QWidget *parent, const char *name, int *values)
	: QWidget(parent, name)
{
	char tmp[12];
	
	gb1=new QGroupBox(i18n("articles"), this);
	
	QLabel *l1=new QLabel(i18n("total :"), gb1);
	QLabel *l2=new QLabel(i18n("unread :"), gb1);
	QLabel *l3=new QLabel(i18n("new :"), gb1);
	QLabel *l4=new QLabel(i18n("threads with unread :"), gb1);
	QLabel *l5=new QLabel(i18n("threads with new :"), gb1);
		
	sprintf(tmp, "%d", values[0]);
	t_otal=new QLabel(tmp, gb1);
	
	sprintf(tmp, "%d", values[1]);
	u_nread=new QLabel(tmp, gb1);

	sprintf(tmp, "%d", values[2]);
	n_ew=new QLabel(tmp, gb1);
		
	sprintf(tmp, "%d", values[3]);
	u_nrThr=new QLabel(tmp, gb1);
		
	sprintf(tmp, "%d", values[4]);
	n_ewThr=new QLabel(tmp, gb1);
		
	SIZE(t_otal); SIZE(u_nread); SIZE(n_ew);
	SIZE(u_nrThr); SIZE(n_ewThr);
	 	
	
	QGridLayout *topL=new QGridLayout(gb1, 5,2, 20,10);
	
	topL->addWidget(l1, 0,0);
	topL->addWidget(t_otal, 0,1);
	topL->addWidget(l2, 1,0);
	topL->addWidget(u_nread, 1,1);
	topL->addWidget(l3, 2,0);
	topL->addWidget(n_ew, 2,1);
	topL->addWidget(l4, 3,0);
	topL->addWidget(u_nrThr, 3,1);
	topL->addWidget(l5, 4,0);
	topL->addWidget(n_ewThr, 4,1);
	topL->activate();
}


KNGroupPropDlg::statistics::~statistics()
{
}

				
void KNGroupPropDlg::statistics::resizeEvent(QResizeEvent*)
{
	gb1->setGeometry(10,10, width()-20, height()-20);
}

//=============================================================================


KNGroupPropDlg::settings::settings(QWidget *parent, const char *name)
	: QWidget(parent, name)
{
	QGroupBox *ngb=new QGroupBox(i18n("nickname"), this);
		
	QLabel *l1=new QLabel(i18n("name:"), ngb);
	nick=new QLineEdit(ngb);
	uw=new KNUserWidget(i18n("specific Identity"), this);
		
	QVBoxLayout *topL=new QVBoxLayout(this, 10);
	QHBoxLayout *nicL=new QHBoxLayout(ngb, 20);
	
	nicL->addWidget(l1);
	nicL->addWidget(nick, 1);
	
	topL->addWidget(ngb);
	topL->addWidget(uw, 1);
	topL->activate();
}

	
KNGroupPropDlg::settings::~settings()
{
}

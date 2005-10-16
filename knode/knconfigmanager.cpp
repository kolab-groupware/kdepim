/*
    KNode, the KDE newsreader
    Copyright (c) 1999-2005 the KNode authors.
    See file AUTHORS for details

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, US
*/

#include "knconfigmanager.h"

#include <kcmultidialog.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kwin.h>

#include <q3hbox.h>

#include "utilities.h"
#include "knglobals.h"
#include "articlewidget.h"
#include "knarticlefactory.h"
#include "knmainwidget.h"
#include "settings.h"


KNConfigManager::KNConfigManager( QObject *parent )
    : QObject( parent ), d_ialog(0)
{
  i_dentity           = new KNode::Identity();
  a_ppearance         = new KNode::Appearance();
  d_isplayedHeaders   = new KNode::DisplayedHeaders();
  p_ostNewsTechnical  = new KNode::PostNewsTechnical();
  c_leanup            = new KNode::Cleanup();
}


KNConfigManager::~KNConfigManager()
{
  delete i_dentity;
  delete a_ppearance;
  delete d_isplayedHeaders;
  delete p_ostNewsTechnical;
  delete c_leanup;
}


void KNConfigManager::configure()
{
  if(!d_ialog) {
    d_ialog = new KNConfigDialog( knGlobals.topWidget );
    d_ialog->setObjectName( "Preferences_Dlg" );
    connect(d_ialog, SIGNAL(finished()), this, SLOT(slotDialogDone()));
    d_ialog->show();
  }
  else
    KWin::activateWindow(d_ialog->winId());
}


void KNConfigManager::syncConfig()
{
  a_ppearance->save();
  d_isplayedHeaders->save();
  p_ostNewsTechnical->save();
  c_leanup->save();
  //c_ache->save();
  knGlobals.settings()->writeConfig();
}


void KNConfigManager::slotDialogDone()
{
  d_ialog->delayedDestruct();
  d_ialog=0;
}


//===================================================================================================


KNConfigDialog::KNConfigDialog( QWidget *parent )
  : KCMultiDialog( parent )
{
  addModule ( "knode_config_identity", false );
  addModule ( "knode_config_accounts", false );
  addModule ( "knode_config_appearance", false );
  addModule ( "knode_config_read_news", false );
  addModule ( "knode_config_post_news", false );
  addModule ( "knode_config_privacy", false );
  addModule ( "knode_config_cleanup", false );

  setHelp("anc-setting-your-identity");

  connect(this, SIGNAL(configCommitted()), this, SLOT(slotConfigCommitted()));
}


void KNConfigDialog::slotConfigCommitted()
{
  knGlobals.configManager()->syncConfig();

  KNode::ArticleWidget::configChanged();
  if(knGlobals.top)
    knGlobals.top->configChanged();
  if(knGlobals.artFactory)
    knGlobals.artFactory->configChanged();
}


//-----------------------------
#include "knconfigmanager.moc"

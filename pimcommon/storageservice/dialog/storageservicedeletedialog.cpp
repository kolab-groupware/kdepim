/*
  Copyright (c) 2014 Montel Laurent <montel@kde.org>

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License, version 2, as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License along
  with this program; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "storageservicedeletedialog.h"

#include "storageservice/widgets/storageservicetreewidget.h"
#include "storageservice/widgets/storageserviceprogresswidget.h"
#include "storageservice/widgets/storageserviceprogressindicator.h"
#include "storageservice/storageserviceabstract.h"

#include <KLocalizedString>
#include <KGlobal>
#include <KSharedConfig>
#include <KMessageBox>
#include <KFileDialog>

#include <QGridLayout>
#include <QLabel>
#include <QTreeWidget>
#include <QFileInfo>
#include <QDebug>
#include <QHeaderView>

using namespace PimCommon;

StorageServiceDeleteDialog::StorageServiceDeleteDialog(PimCommon::StorageServiceAbstract *storage, QWidget *parent)
    : KDialog(parent),
      mStorage(storage)
{
    setCaption( i18n( "Delete File" ) );

    mStorageServiceProgressIndicator = new PimCommon::StorageServiceProgressIndicator(this);
    connect(mStorageServiceProgressIndicator, SIGNAL(updatePixmap(QPixmap)), this, SLOT(slotUpdatePixmap(QPixmap)));

    setButtons( User1 | Close );
    setButtonText(User1, i18n("Delete"));

    QWidget *w = new QWidget;
    QVBoxLayout *vbox = new QVBoxLayout;

    QHBoxLayout *hbox = new QHBoxLayout;
    vbox->addLayout(hbox);
    QLabel *lab = new QLabel(i18n("Select file to delete:"));
    hbox->addWidget(lab);
    mLabelProgressIncator = new QLabel;
    hbox->addWidget(mLabelProgressIncator);
    hbox->setAlignment(mLabelProgressIncator, Qt::AlignLeft);
    mTreeWidget = new StorageServiceTreeWidget(storage);


    vbox->addWidget(mTreeWidget);
    connect(mTreeWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(slotItemDoubleClicked(QTreeWidgetItem*,int)));
    w->setLayout(vbox);
    setMainWidget(w);
    enableButton(User1, false);
    connect(this, SIGNAL(user1Clicked()), this, SLOT(slotDownloadFile()));
    connect(mStorage, SIGNAL(listFolderDone(QString,QString)), this, SLOT(slotListFolderDone(QString,QString)));
    connect(mStorage, SIGNAL(actionFailed(QString,QString)), this, SLOT(slotActionFailed(QString,QString)));
    connect(mTreeWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(slotItemActivated(QTreeWidgetItem*,int)));
    mStorageServiceProgressIndicator->startAnimation();
    mTreeWidget->setEnabled(false);
    mTreeWidget->refreshList();
    readConfig();
}

StorageServiceDeleteDialog::~StorageServiceDeleteDialog()
{
    writeConfig();
}

void StorageServiceDeleteDialog::slotActionFailed(const QString &serviceName, const QString &data)
{
    Q_UNUSED(serviceName);
    Q_UNUSED(data);
    mStorageServiceProgressIndicator->stopAnimation();
    mTreeWidget->setEnabled(true);
    enableButton(User1, true);
}

void StorageServiceDeleteDialog::slotListFolderDone(const QString &serviceName, const QString &data)
{
    mTreeWidget->setEnabled(true);
    mStorageServiceProgressIndicator->stopAnimation();
    mTreeWidget->slotListFolderDone(serviceName, data);
}

void StorageServiceDeleteDialog::slotUpdatePixmap(const QPixmap &pix)
{
    mLabelProgressIncator->setPixmap(pix);
}

void StorageServiceDeleteDialog::readConfig()
{
    KConfigGroup group( KGlobal::config(), "StorageServiceDeleteDialog" );
    const QSize size = group.readEntry( "Size", QSize(600, 400) );
    mTreeWidget->header()->restoreState( group.readEntry( mStorage->storageServiceName(), QByteArray() ) );
    if ( size.isValid() ) {
        resize( size );
    }
}

void StorageServiceDeleteDialog::writeConfig()
{
    KSharedConfig::Ptr config = KGlobal::config();

    KConfigGroup group = config->group( QLatin1String("StorageServiceDeleteDialog") );
    group.writeEntry( "Size", size() );
    group.writeEntry(mStorage->storageServiceName(), mTreeWidget->header()->saveState());
}

void StorageServiceDeleteDialog::slotItemActivated(QTreeWidgetItem *item, int)
{
    enableButton(User1, (item && (mTreeWidget->type(item) == StorageServiceTreeWidget::File)));
}

void StorageServiceDeleteDialog::slotItemDoubleClicked(QTreeWidgetItem *item, int)
{
    StorageServiceTreeWidgetItem *storageServiceItem = dynamic_cast<StorageServiceTreeWidgetItem*>(item);
    if (storageServiceItem) {
        if (mTreeWidget->type(storageServiceItem) == StorageServiceTreeWidget::File) {
            //downloadItem(storageServiceItem);
        }
    }
}

#include "moc_storageservicedeletedialog.cpp"

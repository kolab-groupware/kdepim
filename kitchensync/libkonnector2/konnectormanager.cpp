#include <qdir.h>

#include <kapplication.h>
#include <kdebug.h>
#include <kparts/componentfactory.h>
#include <kstandarddirs.h>

#include "configpart.h"
#include "konnectorinfo.h"


#include "konnectorplugin.h"
#include "konnectormanager.h"

using namespace KSync;

static KStaticDeleter<KonnectorManager> deleter;
KonnectorManager* KonnectorManager::m_self = 0;

KonnectorManager::KonnectorManager()
{
    m_auto = false;
    m_filter.setAutoDelete( true );
    m_konnectors.setAutoDelete( true );
}

KonnectorManager::~KonnectorManager()
{
}

KonnectorManager* KonnectorManager::self()
{
    if ( !m_self )
        deleter.setObject( m_self,  new KonnectorManager() );

    return m_self;
}

Device::ValueList KonnectorManager::query()
{
    return allDevices();
}

Konnector *KonnectorManager::load( const Device& dev )
{
    Konnector *plugin = KParts::ComponentFactory::
        createInstanceFromLibrary<Konnector>( dev.library().local8Bit(), this );
    if ( !plugin ) return 0;

    connect( plugin, SIGNAL( sync( Konnector *, Syncee::PtrList ) ),
             SLOT( slotSync( Konnector *, Syncee::PtrList) ) );
    connect( plugin, SIGNAL( sig_progress( Konnector *, const Progress & ) ),
             SLOT( slotProgress( Konnector *, const Progress & ) ) );
    connect( plugin, SIGNAL( sig_error( Konnector *, const Error & ) ),
             SLOT( slotError( Konnector *, const Error& ) ) );
    connect( plugin, SIGNAL( sig_downloaded( Konnector *, Syncee::PtrList ) ),
             SLOT( slotDownloaded( Konnector *, Syncee::PtrList ) ) );

    m_konnectors.append( plugin );

    return plugin;
}

Konnector *KonnectorManager::load( const QString& deviceName )
{
    return load( find( deviceName ) );
}

bool KonnectorManager::unload( Konnector *k )
{
    return m_konnectors.remove( k );
}

ConfigWidget *KonnectorManager::configWidget( Konnector *konnector,
                                              QWidget *parent,
                                              const char *name )
{
    if ( kapp->type() == QApplication::Tty ) return 0;

    if ( !konnector ) return 0;

    ConfigWidget *wid = konnector->configWidget( parent, name );
    if ( !wid ) wid = new ConfigPart( konnector->capabilities(), parent, name );

    return wid;
}

ConfigWidget *KonnectorManager::configWidget( Konnector *konnector,
                                              const Kapabilities &caps,
                                              QWidget *parent,
                                              const char *name )
{
    if ( kapp->type() == QApplication::Tty ) return 0;

    if ( !konnector ) return 0;

    ConfigWidget *wid = konnector->configWidget( caps, parent, name );
    if ( !wid ) wid = new ConfigPart( konnector->capabilities(), caps, parent, name );

    return wid;
}


bool KonnectorManager::autoLoadFilter() const
{
    return m_auto;
}

void KonnectorManager::setAutoLoadFilter( bool aut )
{
    m_auto = aut;
}

void KonnectorManager::add( Filter* filter)
{
    m_filAdded.append( filter );
}

void KonnectorManager::deleteFilter( Filter* filter)
{
    m_filAdded.remove( filter ); // autoDelete is on!
}

const Filter::PtrList KonnectorManager::filters()
{
    return m_filAdded;
}

void KonnectorManager::write( Konnector *plugin, const Syncee::PtrList &lst )
{
    kdDebug(5201) << "KonnectorManager::write" << endl;
    if ( !plugin ) {
        kdDebug(5201) << " Did not contain the plugin " << endl;
        emit error( plugin, StdError::konnectorDoesNotExist() );
        emit progress( plugin, StdProgress::done() );
        return;
    }
    kdDebug(5201) << "Konnector: " << plugin->info().name() << endl;
    plugin->doWrite( lst );
}

/*
 * find all available desktop files
 * we'll find the kitchensync dir
 * and then parse each .desktop file
 */
Device::ValueList KonnectorManager::allDevices()
{
    m_devices.clear(); // clean up first

    QStringList list = KGlobal::dirs()->findDirs("data", "kitchensync" );

    /* for each dir */
    for (QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
        QDir dir( (*it), "*.desktop" ); // data dir of kitchensync + .desktop as a filter
        QStringList files = dir.entryList();

        QStringList::Iterator fileIt;
        /* for each file */
        for (fileIt = files.begin(); fileIt != files.end(); ++fileIt )
            m_devices.append( parseDevice( (*it) + (*fileIt  ) ) );
    }
    return m_devices;
}

Device KonnectorManager::parseDevice( const QString &path )
{
    KService service( path );

    QString name  = service.name();
    QString lib   = service.library();
    QString group = service.property( QString::fromLatin1("Group" ) ).toString();
    QString vendo = service.property( QString::fromLatin1("Vendor") ).toString();
    QString id    = service.property( QString::fromLatin1("Id"    ) ).toString();

    kdDebug(5201) << "Id " << id << " " << name << endl;

    return Device(name, group, vendo, lib, id );
}

Device KonnectorManager::find( const QString& device )
{
    Device dev;
    if ( m_devices.isEmpty() ) return dev;

    Device::ValueList::Iterator it;
    for ( it = m_devices.begin(); it != m_devices.end(); ++it ) {
        if ( (*it).identify() == device ) {
            dev = (*it);
            break;
        }
    }
    return dev;
}

void KonnectorManager::slotSync( Konnector *k, Syncee::PtrList list )
{
    Syncee::PtrList unknown = findUnknown( list );
    filter( unknown, list );
    emit sync( k, list );
}

void KonnectorManager::slotProgress( Konnector *k, const Progress &pro )
{
    emit progress( k, pro );
}

void KonnectorManager::slotError( Konnector *k, const Error &err )
{
    emit error( k, err );
}

void KonnectorManager::slotDownloaded( Konnector *k, Syncee::PtrList list)
{
    Syncee::PtrList unknown = findUnknown( list );
    filter( unknown, list );
    emit downloaded( k, list );
}

/*
 * FIXME Cornelius take a look here when you want to implement
 * a generic KIO <-> Konnector FileBridge
 * The KIO Konnector only retrieves data and the Filter
 * filters for example the AddressBook or any other data...
 *
 * FIXME use filters!!!!
 */
void KonnectorManager::filter( Syncee::PtrList lst , Syncee::PtrList& real)
{
    Syncee* syncee= 0;
    for ( syncee = lst.first(); syncee; syncee = lst.next() ) {
        real.append( syncee );
    }
}

Syncee::PtrList KonnectorManager::findUnknown( Syncee::PtrList& lst )
{
    lst.setAutoDelete( false );
    Syncee::PtrList list;
    Syncee* syn;
    for ( syn = lst.first(); syn; syn = lst.next() ) {
        if ( syn->type() == QString::fromLatin1("UnknownSyncEntry") ) {
            lst.remove( syn ); // setAutoDelete should be false
            list.append( syn );
        }
    }
    return list;
}

#include "konnectormanager.moc"

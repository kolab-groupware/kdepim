
#include <kdebug.h>

#include "helper.h"

using namespace OpieHelper;

Base::Base( CategoryEdit* edit,
            KSync::KonnectorUIDHelper* helper,
            const QString &tz,
            bool metaSyncing )
{
    m_metaSyncing = metaSyncing;
    m_edit = edit;
    m_helper = helper;
    m_tz = tz;
}
Base::~Base()
{

}
QDateTime Base::fromUTC( time_t time )
{
   struct tm *lt;
   // set tz
   QString real_TZ = QString::fromLocal8Bit( getenv("TZ") );
   if (!m_tz.isEmpty() )
       setenv( "TZ", m_tz, true );
// let's set the tz
#if defined(_OS_WIN32) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64)
    _tzset();
#else
    tzset();
#endif
    lt = localtime( &time );
    QDateTime dt;
    kdDebug() << "tz " << m_tz << " year " << lt->tm_year  << " month " << lt->tm_mon << " day " << lt->tm_mday << endl;
    dt.setDate( QDate( lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday ) );
    dt.setTime( QTime( lt->tm_hour, lt->tm_min, lt->tm_sec ) );
    kdDebug() << "From " << time << " To " << dt.toString() << endl;
    // unset tz
    if (!m_tz.isEmpty() ) {
        unsetenv("TZ");
        setenv("TZ",  real_TZ, true );
    }
    // done
    return dt;
}
time_t Base::toUTC( const QDateTime& dt )
{
    time_t tmp;
    struct tm *lt;
    QString real_TZ = QString::fromLocal8Bit( getenv("TZ") );
    setenv( "TZ", m_tz, true );
#if defined(_OS_WIN32) || defined (Q_OS_WIN32) || defined (Q_OS_WIN64)
    _tzset();
#else
    tzset();
#endif

    // get a tm structure from the system to get the correct tz_name
    tmp = time( 0 );
    lt = localtime( &tmp );

    lt->tm_sec = dt.time().second();
    lt->tm_min = dt.time().minute();
    lt->tm_hour = dt.time().hour();
    lt->tm_mday = dt.date().day();
    lt->tm_mon = dt.date().month() - 1; // 0-11 instead of 1-12
    lt->tm_year = dt.date().year() - 1900; // year - 1900
    //lt->tm_wday = dt.date().dayOfWeek(); ignored anyway
    //lt->tm_yday = dt.date().dayOfYear(); ignored anyway
    lt->tm_wday = -1;
    lt->tm_yday = -1;
    // tm_isdst negative -> mktime will find out about DST
    lt->tm_isdst = -1;
    // keep tm_zone and tm_gmtoff
    tmp = mktime( lt );
    unsetenv("TZ");
    setenv("TZ",  real_TZ, true );
    return tmp;
}
bool Base::isMetaSyncingEnabled()const
{
    return m_metaSyncing;
}
void Base::setMetaSyncingEnabled(bool meta )
{
    m_metaSyncing = meta;
}
QString Base::categoriesToNumber( const QStringList &list, const QString &app )
{
    QString dummy;
    QValueList<OpieCategories>::ConstIterator catIt;
    QValueList<OpieCategories> categories = m_edit->categories();
    bool found = false;
    for ( QStringList::ConstIterator listIt = list.begin(); listIt != list.end(); ++listIt ) {
        for ( catIt = categories.begin(); catIt != categories.end(); ++catIt ) {
            if ( (*catIt).name() == (*listIt) ) { // the same name
                found= true;
                dummy.append( (*catIt).id() + ";");
            }
        }
        if ( !found )
         dummy.append( QString::number(m_edit->addCategory( app, (*listIt) ) ) + ";" );  // generate a new category
    }
    if ( !dummy.isEmpty() )
        dummy.remove(dummy.length() -1,  1 ); //remove the last ;
    return dummy;
}
QStringList Base::categoriesToNumberList( const QStringList &list, const QString &app )
{
    QStringList dummy;
    QValueList<OpieCategories>::ConstIterator catIt;
    QValueList<OpieCategories> categories = m_edit->categories();
    bool found = false;

    for ( QStringList::ConstIterator listIt = list.begin(); listIt != list.end(); ++listIt ) {
        for ( catIt = categories.begin(); catIt != categories.end(); ++catIt ) {
            if ( (*catIt).name() == (*listIt) ) { // the same name
                found= true;
                dummy <<  (*catIt).id();
            }
        }
        if ( !found ) {
            dummy << QString::number( m_edit->addCategory(app, (*listIt) ) );
        }
    }
    return dummy;
}
QString Base::konnectorId( const QString &appName,  const QString &uid )
{
    QString id;
    QString id2;
    // Konnector-.length() ==  10
    if ( uid.startsWith( "Konnector-" ) ) { // not converted
        id2 =  uid.mid( 10 );
    }else if ( m_helper) {
        id =  m_helper->konnectorId( appName,  uid );
        //                        konnector kde
        if (id.isEmpty() ) { // generate new id
            id2 = QString::number( newId() );
            id = QString::fromLatin1("Konnector-") + id2;
        }else if ( id.startsWith( "Konnector-" ) ) { // not converted
            id2 =  id.mid( 10 );
        }
        m_kde2opie.append( Kontainer( id,     uid ) );
    }
    return id2;
}
QString Base::kdeId( const QString &appName,  const QString &uid )
{
    QString ret;
    if ( m_helper == 0 ) {
        ret = QString::fromLatin1("Konnector-")  + uid;
    }
    else{ // only if meta
        ret = m_helper->kdeId( appName, "Konnector-"+uid,  "Konnector-"+uid);
    }
    return ret;
}
// code copyrighted by tt FIXME
int Base::newId()
{
    static QMap<int,  bool> ids;
    int id = -1 * (int) ::time(NULL );
    while ( ids.contains( id ) ){
        id += -1;
        if ( id > 0 )
            id = -1;
    }
    ids.insert( id, true );
    return id;
}

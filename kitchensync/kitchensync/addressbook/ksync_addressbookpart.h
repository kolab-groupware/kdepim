#ifndef ksync_addressbookpart_h
#define ksync_addressbookpart_h

#include <kglobal.h>

#include <manipulatorpart.h>

class KAboutData;
class KConfig;
class AddressBookConfigBase;
class KSimpleConfig;

namespace KABC {
    class AddressBook;
}

namespace KSync {

    class Syncee;
    class SyncEntry;
    class AddressBookSyncee;
    class AddressBookPart : public ManipulatorPart{
        Q_OBJECT
    public:
        AddressBookPart(QWidget* parent,  const char* name,
                        QObject* obj = 0, const char* name2 =0,
                        const QStringList& = QStringList() );
        virtual ~AddressBookPart();
        static KAboutData *createAboutData();

        QString type()const { return QString::fromLatin1("Addressbook"); };
        int progress()const { return 0; };
        QString name()const { return i18n("Addressbook"); };
        QString description()const { return i18n("The addressbook part"); };
        QPixmap *pixmap();
        bool partIsVisible() const { return false; };
        QWidget* widget();
        QWidget* configWidget();
        void processEntry( const Syncee::PtrList&,  Syncee::PtrList& );
    public:
        void slotConfigOk();
    private:
        AddressBookSyncee* load( const QString& path );
        void doMeta( Syncee*,  const QString& path );
        void writeMeta( Syncee*, const QString& path );
        void save( AddressBookSyncee*, const QString& path );
        bool pathIsDefault( const QString& path );
        AddressBookSyncee* book2syncee( KABC::AddressBook* );
        QPixmap m_pixmap;
        AddressBookConfigBase* m_widget;
        QString m_path;
        bool m_evo:1;
        bool m_configured:1;
        KConfig *m_config;
    };
};

#endif

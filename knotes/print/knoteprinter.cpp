#include "knoteprinter.h"
#include "print/knoteprintobject.h"

#include <QPainter>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QAbstractTextDocumentLayout>
#include <QPrinter>
#include <QPrintDialog>

#include <kcal/journal.h>
#include <kdeprintdialog.h>
#include <KMessageBox>

#include <klocale.h>
#include <kdebug.h>
#include <KPrintPreview>

#include <grantlee/context.h>
#include <grantlee/engine.h>
#include <grantlee/templateloader.h>


KNotePrinter::KNotePrinter()
    : mEngine(0)
{
}

KNotePrinter::~KNotePrinter()
{
    if (mEngine)
        mEngine->deleteLater();
}

void KNotePrinter::setDefaultFont( const QFont &font )
{
    m_defaultFont = font;
}

QFont KNotePrinter::defaultFont() const
{
    return m_defaultFont;
}

void KNotePrinter::doPrintPreview(const QString &htmlText)
{
    QPrinter printer( QPrinter::HighResolution );
    printer.setOutputFormat( QPrinter::PdfFormat );
    printer.setCollateCopies( true );

    KPrintPreview previewdlg( &printer, 0 );
    print(printer, htmlText);
    previewdlg.exec();
}

void KNotePrinter::doPrint( const QString &htmlText,
                            const QString &dialogCaption )
{
    QPrinter printer( QPrinter::HighResolution );
    //printer.setFullPage( true );  //disabled, causes asymmetric margins
    QPrintDialog printDialog(/*KdePrint::createPrintDialog*/(&printer));
    printDialog.setWindowTitle( dialogCaption );
    if ( !printDialog.exec() ) {
        return;
    }
    print(printer, htmlText);
}

void KNotePrinter::print(QPrinter &printer, const QString &htmlText)
{
    const int margin = 30; //pt     //set to 40 when setFullPage() works again
    int marginX = margin * printer.logicalDpiX() / 72;
    int marginY = margin * printer.logicalDpiY() / 72;

    QRect typeArea( marginX, marginY,
                    printer.width() - marginX * 2,
                    printer.height() - marginY * 2 );

    QTextDocument textDoc;
    textDoc.setHtml( htmlText );
    textDoc.documentLayout()->setPaintDevice( &printer );
    textDoc.setPageSize( typeArea.size() );
    textDoc.setDefaultFont( m_defaultFont );

    QPainter painter( &printer );
    QRect clip( typeArea );
    painter.translate( marginX, marginY );
    clip.translate( -marginX, -marginY );

    for ( int page = 1; page <= textDoc.pageCount(); ++page ) {
        textDoc.drawContents( &painter, clip );
        clip.translate( 0, typeArea.height() );
        painter.translate( 0, -typeArea.height() );

        painter.setFont( m_defaultFont );
        painter.drawText(
                    clip.right() - painter.fontMetrics().width( QString::number( page ) ),
                    clip.bottom() + painter.fontMetrics().ascent() + 5,
                    QString::number( page ) );

        if ( page < textDoc.pageCount() ) {
            printer.newPage();
        }
    }
}

void KNotePrinter::printNote( const QString &name,
                              const QString &htmlText, bool preview )
{
    const QString dialogCaption = i18n( "Print %1", name );
    if (preview)
        doPrintPreview(htmlText);
    else
        doPrint( htmlText, dialogCaption );
}

void KNotePrinter::printNotes(const QList<KNotePrintObject *> lst, const QString &themePath, bool preview)
{
    mEngine = new Grantlee::Engine;
    mTemplateLoader = Grantlee::FileSystemTemplateLoader::Ptr( new Grantlee::FileSystemTemplateLoader );

    mTemplateLoader->setTemplateDirs( QStringList() << themePath );
    mEngine->addTemplateLoader( mTemplateLoader );

    mSelfcontainedTemplate = mEngine->loadByName( QLatin1String("theme.html") );
    QString mErrorMessage;
    if ( mSelfcontainedTemplate->error() ) {
         mErrorMessage += mSelfcontainedTemplate->errorString() + QLatin1String("<br>");
    }

    if (mErrorMessage.isEmpty()) {
        QVariantList notes;
        Q_FOREACH(KNotePrintObject *n, lst)
            notes << QVariant::fromValue(static_cast<QObject*>(n));
        Grantlee::Context c;
        c.insert(QLatin1String("notes"), notes);

        const QString htmlText = mSelfcontainedTemplate->render(&c);
        const QString dialogCaption = i18np( "Print Note", "Print %1 notes",
                                             lst.count() );
        if (preview)
            doPrintPreview(htmlText);
        else
            doPrint( htmlText, dialogCaption );
    } else {
        KMessageBox::error(0, i18n("Printing theme was not found."), i18n("Printing error"));
    }
}

inline QString KNotePrinter::ensureHtmlText( const QString &maybeRichText ) const
{
    if ( Qt::mightBeRichText( maybeRichText ) ) {
        return maybeRichText; //... now probablyRichText
    } else {
        return Qt::convertFromPlainText( maybeRichText );
    }
}


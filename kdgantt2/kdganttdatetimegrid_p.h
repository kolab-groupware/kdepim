/****************************************************************************
 ** Copyright (C) 2001-2006 Klarälvdalens Datakonsult AB.  All rights reserved.
 **
 ** This file is part of the KD Gantt library.
 **
 ** This file may be distributed and/or modified under the terms of the
 ** GNU General Public License version 2 as published by the Free Software
 ** Foundation and appearing in the file LICENSE.GPL included in the
 ** packaging of this file.
 **
 ** Licensees holding valid commercial KD Gantt licenses may use this file in
 ** accordance with the KD Gantt Commercial License Agreement provided with
 ** the Software.
 **
 ** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 ** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 **
 ** See http://www.kdab.net/kdgantt for
 **   information about KD Gantt Commercial License Agreements.
 **
 ** Contact info@kdab.net if any conditions of this
 ** licensing are not clear to you.
 **
 **********************************************************************/
#ifndef KDGANTTDATETIMEGRID_P_H
#define KDGANTTDATETIMEGRID_P_H

#include "kdganttdatetimegrid.h"
#include "kdganttabstractgrid_p.h"

#include <QDateTime>
#include <QBrush>

namespace KDGantt {
    class DateTimeScaleFormatter::Private
    {
    public:
        Private( DateTimeScaleFormatter::Range _range,
                 const QString& _format,
                 const QString& _templ,
                 Qt::Alignment _alignment )
            : range( _range ),
              format( _format ),
              templ( _templ ),
              alignment( _alignment )
        {
        }

        const DateTimeScaleFormatter::Range range;
        const QString format;
        const QString templ;
        const Qt::Alignment alignment;
    };

    class DateTimeGrid::Private : public AbstractGrid::Private {
    public:
        Private()
            : startDateTime( QDateTime::currentDateTime().addDays( -3 ) ),
              dayWidth( 100. ),
              scale( ScaleAuto ),
              weekStart( Qt::Monday ),
              freeDays( QSet<Qt::DayOfWeek>() << Qt::Saturday << Qt::Sunday ),
              rowSeparators( false ),
              noInformationBrush( Qt::red, Qt::DiagCrossPattern ),
	      upper( new DateTimeScaleFormatter( DateTimeScaleFormatter::Week, QString::fromLatin1( "w" ) ) ),
	      lower( new DateTimeScaleFormatter( DateTimeScaleFormatter::Day, QString::fromLatin1( "ddd" ) ) ),
              year_upper( DateTimeScaleFormatter::Year, QString::fromLatin1("yyyy" ) ),
              year_lower( DateTimeScaleFormatter::Month, QString::fromLatin1("MMM" ) ),
              month_upper( DateTimeScaleFormatter::Month, QString::fromLatin1("MMMM" ) ),
              month_lower( DateTimeScaleFormatter::Week, QString::fromLatin1("w" ) ),
              week_upper( DateTimeScaleFormatter::Week, QString::fromLatin1("w" ) ),
              week_lower( DateTimeScaleFormatter::Day, QString::fromLatin1("ddd" ) ),
              day_upper( DateTimeScaleFormatter::Day, QString::fromLatin1("dddd" ) ),
              day_lower( DateTimeScaleFormatter::Hour, QString::fromLatin1("hh" ) ),
              hour_upper( DateTimeScaleFormatter::Hour, QString::fromLatin1("hh" ) ),
              hour_lower( DateTimeScaleFormatter::Minute, QString::fromLatin1("m" ) ),
              minute_upper( DateTimeScaleFormatter::Minute, QString::fromLatin1("m" ) ),
              minute_lower( DateTimeScaleFormatter::Second, QString::fromLatin1("s" ) )
        {
        }
        ~Private()
        {
            delete lower;
            delete upper;
        }

        qreal dateTimeToChartX( const QDateTime& dt ) const;
        QDateTime chartXtoDateTime( qreal x ) const;


	int tabHeight( const QString& txt, QWidget* widget = 0 ) const;
	void getAutomaticFormatters( DateTimeScaleFormatter** lower, DateTimeScaleFormatter** upper);

        void paintVerticalDayLines( QPainter* painter,
                                    const QRectF& sceneRect,
                                    const QRectF& exposedRect,
                                    QWidget* widget );
        void paintVerticalHourLines( QPainter* painter,
                                    const QRectF& sceneRect,
                                    const QRectF& exposedRect,
                                    QWidget* widget );
        void paintVerticalUserDefinedLines( QPainter* painter,
                                            const QRectF& sceneRect,
                                            const QRectF& exposedRect,
                                            const DateTimeScaleFormatter* formatter,
                                            QWidget* widget );

        QDateTime startDateTime;
        QDateTime endDateTime;
        qreal dayWidth;
	Scale scale;
        Qt::DayOfWeek weekStart;
        QSet<Qt::DayOfWeek> freeDays;
        bool rowSeparators;
        QBrush noInformationBrush;

        DateTimeScaleFormatter* upper;
        DateTimeScaleFormatter* lower;

	DateTimeScaleFormatter year_upper;
	DateTimeScaleFormatter year_lower;
	DateTimeScaleFormatter month_upper;
	DateTimeScaleFormatter month_lower;
	DateTimeScaleFormatter week_upper;
	DateTimeScaleFormatter week_lower;
	DateTimeScaleFormatter day_upper;
	DateTimeScaleFormatter day_lower;
	DateTimeScaleFormatter hour_upper;
	DateTimeScaleFormatter hour_lower;
	DateTimeScaleFormatter minute_upper;
	DateTimeScaleFormatter minute_lower;
    };

    inline DateTimeGrid::DateTimeGrid( DateTimeGrid::Private* d ) : AbstractGrid( d ) {}

    inline DateTimeGrid::Private* DateTimeGrid::d_func() {
        return static_cast<Private*>( AbstractGrid::d_func() );
    }
    inline const DateTimeGrid::Private* DateTimeGrid::d_func() const {
        return static_cast<const Private*>( AbstractGrid::d_func() );
    }
}

#endif /* KDGANTTDATETIMEGRID_P_H */


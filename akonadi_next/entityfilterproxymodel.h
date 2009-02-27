/*
    Copyright (c) 2007 Bruno Virlet <bruno.virlet@gmail.com>
    Copyright (c) 2009 Stephen Kelly <steveire@gmail.com>

    This library is free software; you can redistribute it and/or modify it
    under the terms of the GNU Library General Public License as published by
    the Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    This library is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Library General Public
    License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to the
    Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.
*/

#ifndef AKONADI_ENTITYFILTERPROXYMODEL_H
#define AKONADI_ENTITYFILTERPROXYMODEL_H

// #include "akonadi_export.h"
#include <QtGui/QSortFilterProxyModel>

namespace Akonadi {

/**
 * @short A proxy model that filters entities by mime type.
 *
 * This class can be used on top of an EntityTreeModel to exclude entities by mimetype
 * or to include only certain mimetypes.
 *
 * @code
 *
 *   Akonadi::EntityTreeModel *model = new Akonadi::EntityTreeModel( this );
 *
 *   Akonadi::EntityFilterProxyModel *proxy = new Akonadi::EntityFilterProxyModel();
 *   proxy->addMimeTypeInclusionFilter( "message/rfc822" );
 *   proxy->setSourceModel( model );
 *
 *   Akonadi::EntityTreeView *view = new Akonadi::EntityTreeView( this );
 *   view->setModel( proxy );
 *
 * @endcode
 *
 * @li If a mimetype is in both the exclusion list and the inclusion list, it is excluded.
 * @li If the mimeTypeInclusionFilter is empty, all mimetypes are
 *     accepted (except if they are in the exclusion filter of course).
 *
 *
 * @author Bruno Virlet <bruno.virlet@gmail.com>
 * @author Stephen Kelly <steveire@gmail.com>
 */
class /*AKONADI_EXPORT*/ EntityFilterProxyModel : public QSortFilterProxyModel
{
  Q_OBJECT

  public:
    /**
     * Creates a new proxy filter model.
     *
     * @param parent The parent object.
     */
    explicit EntityFilterProxyModel( QObject *parent = 0 );

    /**
     * Destroys the proxy filter model.
     */
    virtual ~EntityFilterProxyModel();

    /**
     * Add mime types to be shown by the filter.
     *
     * @param mimeTypes A list of mime types to be included.
     */
    void addMimeTypeInclusionFilters( const QStringList &mimeTypes );

    /**
    Add mimetypes to filter out

    @param mimeTypes A list to exclude from the model.
    */
    void addMimeTypeExclusionFilters( const QStringList &mimeTypes );

    /**
     * Add mime type to be shown by the filter.
     *
     * @param mimeType A mime type to be shown.
     */
    void addMimeTypeInclusionFilter( const QString &mimeType );

    /**
     * Add mime type to be excluded by the filter.
     *
     * @param mimeType A mime type to be excluded.
     */
    void addMimeTypeExclusionFilter( const QString &mimeType );

    /**
     * Returns the list of mime type inclusion filters.
     */
    QStringList mimeTypeInclusionFilters() const;

    /**
     * Returns the list of mime type exclusion filters.
     */
    QStringList mimeTypeExclusionFilters() const;

    /**
     * Clear all mime type filters.
     */
    void clearFilters();

    void setRootIndex(const QModelIndex &srcIndex);

  protected:
    virtual bool filterAcceptsRow( int sourceRow, const QModelIndex &sourceParent) const;

  private:
    //@cond PRIVATE
    class Private;
    Private* const d;
    //@endcond
};

}

#endif

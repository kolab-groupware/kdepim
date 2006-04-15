/*
 * This file is part of libsyndication
 *
 * Copyright (C) 2006 Frank Osterfeld <frank.osterfeld@kdemail.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef LIBSYNDICATION_RDF_PROPERTY_H
#define LIBSYNDICATION_RDF_PROPERTY_H

#include <libsyndication/rdf/resource.h>

class QString;

namespace Syndication {
namespace RDF {

class Property;

typedef SharedPtr<Property> PropertyPtr;

class KDE_EXPORT Property : public Resource
{
    public:
        
        Property();
        Property(const QString& uri);
        virtual ~Property();
        
        virtual void accept(NodeVisitor* visitor, NodePtr ptr);
        
        virtual bool isProperty() const;
        virtual Property* clone() const;
};

} // namespace RDF
} // namespace Syndication

#endif // LIBSYNDICATION_RDF_PROPERTY_H

/*
  Copyright (c) 2013, 2014 Montel Laurent <montel@kde.org>
  
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

#ifndef EXPORTBLOGILOJOB_H
#define EXPORTBLOGILOJOB_H

#include "abstractimportexportjob.h"

class QWidget;
class ArchiveStorage;

class ExportBlogiloJob : public AbstractImportExportJob
{
    Q_OBJECT
public:
    explicit ExportBlogiloJob(QWidget *parent, Utils::StoredTypes typeSelected, ArchiveStorage *archiveStorage, int numberOfStep);
    ~ExportBlogiloJob();

    void start();

private:
    void backupConfig();
    void backupData();
};

#endif // EXPORTBLOGILOJOB_H

/*
  Copyright (c) 2013 Montel Laurent <montel@kde.org>

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


#include "imagescalingutils.h"
#include "messagecomposersettings.h"

using namespace MessageComposer;

bool Util::resizeImage(MessageCore::AttachmentPart::Ptr part)
{
    const QString filename = part->fileName();
    const QString pattern = MessageComposer::MessageComposerSettings::self()->filterSourcePattern();

    if (!pattern.isEmpty()) {
        switch (MessageComposer::MessageComposerSettings::self()->filterSourceType()) {
        case MessageComposer::MessageComposerSettings::EnumFilterSourceType::NoFilter:
            break;
        case MessageComposer::MessageComposerSettings::EnumFilterSourceType::IncludeFilesWithPattern:
            if (!filename.startsWith(pattern)) {
                return false;
            }
            break;
        case MessageComposer::MessageComposerSettings::EnumFilterSourceType::ExcludeFilesWithPattern:
            if (filename.startsWith(pattern)) {
                return false;
            }
            break;
        }
    }

    if (MessageComposer::MessageComposerSettings::self()->skipImageLowerSizeEnabled() &&
            (part->size() > MessageComposer::MessageComposerSettings::self()->skipImageLowerSize() *1024)) {
        if (part->mimeType() == "image/gif" ||
                part->mimeType() == "image/jpeg" ||
                part->mimeType() == "image/png" ) {
            return true;
        }
    }
    return false;
}


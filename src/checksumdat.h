/*
 *  This file is part of skyscraper.
 *  Copyright 2026 rsporteman @ GitHub
 *
 *  skyscraper is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  skyscraper is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with skyscraper; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 */

#ifndef CHECKSUMDAT_H
#define CHECKSUMDAT_H

#include <QByteArray>
#include <QMultiHash>
#include <QString>

/*
 Precalculated checksums of ROM files, read from a Logiqx XML DAT file (the
 format used by No-Intro, Redump, ClrMamePro, RomVault, ...).

 Each <rom name="..." size="..." crc="..." md5="..." sha1="..."/> element is
 one entry. A ROM file on disk matches an entry when the filename (without
 directory) and the size in bytes are equal. Entries lacking one of the three
 checksums are skipped. If more than one entry with the same filename and size
 but different checksums exists, the lookup is considered ambiguous and fails.
*/
class ChecksumDat {
public:
    struct Checksums {
        // all uppercase hex, as expected by the ScreenScraper API
        QString crc;
        QString md5;
        QString sha1;
    };

    // Parses the DAT in the file, returns false and sets error on failure
    bool load(const QString &filePath, QString &error);
    // Parses the DAT content, returns false and sets error on failure
    bool loadFromData(const QByteArray &xml, QString &error);

    // Returns true and sets out if exactly one set of checksums is known for
    // the filename (without directory) and size
    bool lookup(const QString &fileName, qint64 size, Checksums &out) const;

    bool isEmpty() const { return entries.isEmpty(); }
    int size() const { return entries.size(); }
    // number of <rom> elements that were skipped due to missing attributes
    int skipped() const { return skippedEntries; }

private:
    struct Entry {
        qint64 size;
        Checksums sums;
    };
    QMultiHash<QString, Entry> entries;
    int skippedEntries = 0;
};

#endif // CHECKSUMDAT_H

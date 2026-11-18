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

#include "checksumdat.h"

#include <QFile>
#include <QRegularExpression>
#include <QXmlStreamReader>

static bool isHex(const QString &s, int len) {
    static const QRegularExpression re("^[0-9A-Fa-f]+$");
    return s.length() == len && re.match(s).hasMatch();
}

bool ChecksumDat::load(const QString &filePath, QString &error) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        error = QString("cannot open checksum DAT '%1': %2")
                    .arg(filePath, f.errorString());
        return false;
    }
    QByteArray data = f.readAll();
    f.close();
    return loadFromData(data, error);
}

bool ChecksumDat::loadFromData(const QByteArray &xml, QString &error) {
    entries.clear();
    skippedEntries = 0;

    QXmlStreamReader r(xml);
    while (!r.atEnd()) {
        r.readNext();
        if (!r.isStartElement() || r.name() != QStringLiteral("rom")) {
            continue;
        }
        const QXmlStreamAttributes a = r.attributes();
        const QString name = a.value("name").toString();
        bool sizeOk = false;
        const qint64 size = a.value("size").toLongLong(&sizeOk);
        const QString crc = a.value("crc").toString();
        const QString md5 = a.value("md5").toString();
        const QString sha1 = a.value("sha1").toString();

        if (name.isEmpty() || !sizeOk || size < 0 || !isHex(crc, 8) ||
            !isHex(md5, 32) || !isHex(sha1, 40)) {
            skippedEntries++;
            continue;
        }
        // DATs may store names with a subfolder, match on filename only
        QString fileName = name;
        fileName.replace('\\', '/');
        fileName = fileName.mid(fileName.lastIndexOf('/') + 1);

        Entry e;
        e.size = size;
        e.sums.crc = crc.toUpper();
        e.sums.md5 = md5.toUpper();
        e.sums.sha1 = sha1.toUpper();
        entries.insert(fileName, e);
    }
    if (r.hasError()) {
        error = QString("invalid checksum DAT at line %1: %2")
                    .arg(r.lineNumber())
                    .arg(r.errorString());
        entries.clear();
        return false;
    }
    return true;
}

bool ChecksumDat::lookup(const QString &fileName, qint64 size,
                         Checksums &out) const {
    bool found = false;
    Checksums match;
    for (auto it = entries.constFind(fileName);
         it != entries.cend() && it.key() == fileName; ++it) {
        if (it.value().size != size) {
            continue;
        }
        const Checksums &s = it.value().sums;
        if (found && (s.crc != match.crc || s.md5 != match.md5 ||
                      s.sha1 != match.sha1)) {
            // same name and size, but different content: ambiguous
            return false;
        }
        match = s;
        found = true;
    }
    if (found) {
        out = match;
    }
    return found;
}

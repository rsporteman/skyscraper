#include "checksumdat.h"

#include <QTemporaryDir>
#include <QTest>

/*
 Tests the parsing of Logiqx XML DAT files and the lookup of precalculated
 checksums by filename and size.
*/
class TestChecksumDat : public QObject {
    Q_OBJECT

private:
    const QByteArray datHeader =
        "<?xml version=\"1.0\"?>\n"
        "<!DOCTYPE datafile PUBLIC \"-//Logiqx//DTD ROM Management "
        "Datafile//EN\" "
        "\"http://www.logiqx.com/Dats/datafile.dtd\">\n"
        "<datafile>\n"
        "  <header><name>Test</name></header>\n";
    const QByteArray datFooter = "</datafile>\n";

    QByteArray dat(const QByteArray &games) {
        return datHeader + games + datFooter;
    }

private slots:
    void testLookupHit() {
        ChecksumDat cd;
        QString err;
        QVERIFY(cd.loadFromData(
            dat("<game name=\"Bully (USA)\">"
                "<rom name=\"Bully (USA).chd\" size=\"2182974250\" "
                "crc=\"0a1b2c3d\" md5=\"0123456789abcdef0123456789abcdef\" "
                "sha1=\"d801adc0ef54c8094283276f88ea84869c57b182\"/>"
                "</game>"),
            err));
        QCOMPARE(cd.size(), 1);
        QCOMPARE(cd.skipped(), 0);
        QVERIFY(!cd.isEmpty());

        ChecksumDat::Checksums s;
        QVERIFY(cd.lookup("Bully (USA).chd", 2182974250LL, s));
        // always uppercase, as required by the ScreenScraper API
        QCOMPARE(s.crc, "0A1B2C3D");
        QCOMPARE(s.md5, "0123456789ABCDEF0123456789ABCDEF");
        QCOMPARE(s.sha1, "D801ADC0EF54C8094283276F88EA84869C57B182");
    }

    void testLookupMiss() {
        ChecksumDat cd;
        QString err;
        QVERIFY(cd.loadFromData(
            dat("<game name=\"g\"><rom name=\"game.iso\" size=\"100\" "
                "crc=\"00000001\" md5=\"00000000000000000000000000000001\" "
                "sha1=\"0000000000000000000000000000000000000001\"/></game>"),
            err));
        ChecksumDat::Checksums s;
        s.crc = "untouched";
        // size differs: entry is stale or belongs to another dump
        QVERIFY(!cd.lookup("game.iso", 101, s));
        // unknown filename
        QVERIFY(!cd.lookup("other.iso", 100, s));
        // filename matching is case sensitive, as filesystems usually are
        QVERIFY(!cd.lookup("GAME.ISO", 100, s));
        QCOMPARE(s.crc, "untouched");
    }

    void testNameWithFolder() {
        ChecksumDat cd;
        QString err;
        QVERIFY(cd.loadFromData(
            dat("<game name=\"g\">"
                "<rom name=\"sub\\dir\\a.bin\" size=\"1\" crc=\"00000001\" "
                "md5=\"00000000000000000000000000000001\" "
                "sha1=\"0000000000000000000000000000000000000001\"/>"
                "<rom name=\"sub/b.bin\" size=\"2\" crc=\"00000002\" "
                "md5=\"00000000000000000000000000000002\" "
                "sha1=\"0000000000000000000000000000000000000002\"/>"
                "</game>"),
            err));
        ChecksumDat::Checksums s;
        QVERIFY(cd.lookup("a.bin", 1, s));
        QCOMPARE(s.crc, "00000001");
        QVERIFY(cd.lookup("b.bin", 2, s));
        QCOMPARE(s.crc, "00000002");
    }

    void testSameNameDifferentSize() {
        // e.g. two DATs merged, different dumps share a filename
        ChecksumDat cd;
        QString err;
        QVERIFY(cd.loadFromData(
            dat("<game name=\"g1\"><rom name=\"disc.iso\" size=\"10\" "
                "crc=\"0000000a\" md5=\"0000000000000000000000000000000a\" "
                "sha1=\"000000000000000000000000000000000000000a\"/></game>"
                "<game name=\"g2\"><rom name=\"disc.iso\" size=\"20\" "
                "crc=\"00000014\" md5=\"00000000000000000000000000000014\" "
                "sha1=\"0000000000000000000000000000000000000014\"/></game>"),
            err));
        QCOMPARE(cd.size(), 2);
        ChecksumDat::Checksums s;
        QVERIFY(cd.lookup("disc.iso", 10, s));
        QCOMPARE(s.crc, "0000000A");
        QVERIFY(cd.lookup("disc.iso", 20, s));
        QCOMPARE(s.crc, "00000014");
    }

    void testDuplicateEntries() {
        const QByteArray rom1 =
            "<rom name=\"x.bin\" size=\"5\" crc=\"0000000a\" "
            "md5=\"0000000000000000000000000000000a\" "
            "sha1=\"000000000000000000000000000000000000000a\"/>";
        // same checksums, only differing in letter case
        const QByteArray rom1Upper =
            "<rom name=\"x.bin\" size=\"5\" crc=\"0000000A\" "
            "md5=\"0000000000000000000000000000000A\" "
            "sha1=\"000000000000000000000000000000000000000A\"/>";
        const QByteArray rom2 =
            "<rom name=\"x.bin\" size=\"5\" crc=\"00000006\" "
            "md5=\"00000000000000000000000000000006\" "
            "sha1=\"0000000000000000000000000000000000000006\"/>";
        ChecksumDat::Checksums s;
        QString err;

        // identical duplicates are fine
        ChecksumDat same;
        QVERIFY(same.loadFromData(dat("<game name=\"a\">" + rom1 +
                                      "</game><game name=\"b\">" + rom1Upper +
                                      "</game>"),
                                  err));
        QVERIFY(same.lookup("x.bin", 5, s));
        QCOMPARE(s.crc, "0000000A");

        // same name and size but different content cannot be resolved
        ChecksumDat ambiguous;
        QVERIFY(ambiguous.loadFromData(dat("<game name=\"a\">" + rom1 +
                                           "</game><game name=\"b\">" + rom2 +
                                           "</game>"),
                                       err));
        QVERIFY(!ambiguous.lookup("x.bin", 5, s));
    }

    void testIncompleteEntriesSkipped() {
        ChecksumDat cd;
        QString err;
        QVERIFY(cd.loadFromData(
            dat("<game name=\"g\">"
                // valid
                "<rom name=\"ok.bin\" size=\"1\" crc=\"00000001\" "
                "md5=\"00000000000000000000000000000001\" "
                "sha1=\"0000000000000000000000000000000000000001\"/>"
                // no md5
                "<rom name=\"nomd5.bin\" size=\"1\" crc=\"00000001\" "
                "sha1=\"0000000000000000000000000000000000000001\"/>"
                // crc too short
                "<rom name=\"shortcrc.bin\" size=\"1\" crc=\"1\" "
                "md5=\"00000000000000000000000000000001\" "
                "sha1=\"0000000000000000000000000000000000000001\"/>"
                // sha1 not hex
                "<rom name=\"badsha1.bin\" size=\"1\" crc=\"00000001\" "
                "md5=\"00000000000000000000000000000001\" "
                "sha1=\"zz00000000000000000000000000000000000001\"/>"
                // size missing
                "<rom name=\"nosize.bin\" crc=\"00000001\" "
                "md5=\"00000000000000000000000000000001\" "
                "sha1=\"0000000000000000000000000000000000000001\"/>"
                // size not a number
                "<rom name=\"badsize.bin\" size=\"abc\" crc=\"00000001\" "
                "md5=\"00000000000000000000000000000001\" "
                "sha1=\"0000000000000000000000000000000000000001\"/>"
                // no name
                "<rom size=\"1\" crc=\"00000001\" "
                "md5=\"00000000000000000000000000000001\" "
                "sha1=\"0000000000000000000000000000000000000001\"/>"
                "</game>"),
            err));
        QCOMPARE(cd.size(), 1);
        QCOMPARE(cd.skipped(), 6);
        ChecksumDat::Checksums s;
        QVERIFY(cd.lookup("ok.bin", 1, s));
        QVERIFY(!cd.lookup("nomd5.bin", 1, s));
    }

    void testInvalidXml() {
        ChecksumDat cd;
        QString err;
        QVERIFY(!cd.loadFromData("<datafile><game><rom name=\"a\"", err));
        QVERIFY(!err.isEmpty());
        QVERIFY(cd.isEmpty());
    }

    void testReloadReplacesEntries() {
        ChecksumDat cd;
        QString err;
        QVERIFY(cd.loadFromData(
            dat("<game name=\"g\"><rom name=\"old.bin\" size=\"1\" "
                "crc=\"00000001\" md5=\"00000000000000000000000000000001\" "
                "sha1=\"0000000000000000000000000000000000000001\"/></game>"),
            err));
        QVERIFY(cd.loadFromData(dat(""), err));
        QVERIFY(cd.isEmpty());
        ChecksumDat::Checksums s;
        QVERIFY(!cd.lookup("old.bin", 1, s));
    }

    void testLoadFromFile() {
        QTemporaryDir tmp;
        QVERIFY(tmp.isValid());
        const QString path = tmp.filePath("test.dat");
        QFile f(path);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(dat("<game name=\"g\"><rom name=\"file.chd\" size=\"42\" "
                    "crc=\"deadbeef\" md5=\"00000000000000000000000000000042\" "
                    "sha1=\"0000000000000000000000000000000000000042\"/>"
                    "</game>"));
        f.close();

        ChecksumDat cd;
        QString err;
        QVERIFY(cd.load(path, err));
        ChecksumDat::Checksums s;
        QVERIFY(cd.lookup("file.chd", 42, s));
        QCOMPARE(s.crc, "DEADBEEF");

        ChecksumDat missing;
        QVERIFY(!missing.load(tmp.filePath("does-not-exist.dat"), err));
        QVERIFY(err.contains("does-not-exist.dat"));
    }
};

QTEST_MAIN(TestChecksumDat)
#include "test_checksumdat.moc"

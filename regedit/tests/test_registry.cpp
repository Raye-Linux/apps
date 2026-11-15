// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The window's model on a ~/.config of its own: the tree, the values, an
// edit and its undo, search, export and import.

#include "registry.h"

#include <QDir>
#include <QFile>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QUrl>

class TestRegistry : public QObject
{
    Q_OBJECT

    QTemporaryDir m_tmp;
    QString m_config;

    void put(const QString &rel, const QByteArray &data)
    {
        QDir().mkpath(QFileInfo(m_config + "/" + rel).path());
        QFile f(m_config + "/" + rel);
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(data);
    }

    QByteArray get(const QString &rel)
    {
        QFile f(m_config + "/" + rel);
        return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
    }

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        m_config = m_tmp.filePath("config");
        put("kdeglobals", "# keep me\n[General]\nColorScheme=BreezeDark\nAccentColor=123,60,255\n[KDE]\nSingleClick=false\n");
        put("gtk-3.0/settings.ini", "[Settings]\ngtk-font-name=Inter 10\n");
        put("locked", "[A]\nk[$i]=1\n");
    }

    void treeAndValues()
    {
        Registry r;
        r.tree()->setPlaces(m_config, m_tmp.filePath("root"));
        QVERIFY(r.indexOfPath("Computer\\HKEY_CURRENT_USER\\Config\\gtk-3.0\\settings.ini\\Settings").isValid());
        QVERIFY(!r.indexOfPath("Computer\\HKEY_CURRENT_USER\\Config\\nope").isValid());
        r.select(r.indexOfPath("HKEY_CURRENT_USER\\Config\\kdeglobals\\General"));
        QCOMPARE(r.currentPath(), QStringLiteral("Computer\\HKEY_CURRENT_USER\\Config\\kdeglobals\\General"));
        QVERIFY(r.currentWritable());
        QVERIFY(!r.currentSystem());
        QCOMPARE(r.values()->count(), 2);
        QCOMPARE(r.values()->rows().at(1).kind, QStringLiteral("List"));
        QCOMPARE(r.checkValue("AccentColor", "anything"), QString());

        r.select(r.indexOfPath("HKEY_CURRENT_USER\\Config\\kdeglobals\\KDE"));
        QVERIFY(!r.checkValue("SingleClick", "yes").isEmpty());
        QVERIFY(r.checkValue("SingleClick", "true").isEmpty());

        r.select(r.indexOfPath("HKEY_CURRENT_USER\\Config\\locked\\A"));
        QVERIFY(r.values()->rows().at(0).locked);
    }

    void editAndUndo()
    {
        Registry r;
        r.tree()->setPlaces(m_config, m_tmp.filePath("root"));
        const QByteArray before = get("kdeglobals");
        r.select(r.indexOfPath("HKEY_CURRENT_USER\\Config\\kdeglobals\\General"));
        r.setValue("ColorScheme", "RayeNight");
        QCOMPARE(get("kdeglobals"), QByteArray(before).replace("BreezeDark", "RayeNight"));
        r.newValue("Text", "Name", "Raye");
        r.deleteValue("AccentColor");
        QCOMPARE(r.values()->count(), 2);
        QVERIFY(r.canUndo());
        r.undo();
        r.undo();
        r.undo();
        QVERIFY(!r.canUndo());
        QCOMPARE(get("kdeglobals"), before);
        // every write kept a copy
        QDir backups(QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) + "/raye-regedit/backups");
        QVERIFY(backups.exists());
    }

    void lockedIsNotWritten()
    {
        Registry r;
        r.tree()->setPlaces(m_config, m_tmp.filePath("root"));
        QSignalSpy failed(&r, &Registry::failed);
        r.select(r.indexOfPath("HKEY_CURRENT_USER\\Config\\locked\\A"));
        r.setValue("k", "2");
        QCOMPARE(failed.count(), 1);
        QCOMPARE(get("locked"), QByteArray("[A]\nk[$i]=1\n"));
    }

    void search()
    {
        Registry r;
        r.tree()->setPlaces(m_config, m_tmp.filePath("root"));
        r.select(r.indexOfPath("HKEY_CURRENT_USER"));
        QVERIFY(r.find("inter", false, false, true, false));
        QCOMPARE(r.currentPath(), QStringLiteral("Computer\\HKEY_CURRENT_USER\\Config\\gtk-3.0\\settings.ini\\Settings"));
        QVERIFY(r.find("singleclick", false, true, false, false));
        QCOMPARE(r.currentPath(), QStringLiteral("Computer\\HKEY_CURRENT_USER\\Config\\kdeglobals\\KDE"));
        QVERIFY(!r.find("singleclick", false, true, false, false));
        r.select(r.indexOfPath("HKEY_CURRENT_USER"));
        QVERIFY(r.find("General", true, false, false, true));
        QCOMPARE(r.currentPath(), QStringLiteral("Computer\\HKEY_CURRENT_USER\\Config\\kdeglobals\\General"));
    }

    void exportImport()
    {
        Registry r;
        r.tree()->setPlaces(m_config, m_tmp.filePath("root"));
        r.select(r.indexOfPath("HKEY_CURRENT_USER\\Config\\kdeglobals"));
        const QUrl file = QUrl::fromLocalFile(m_tmp.filePath("out.reg"));
        QVERIFY(r.exportTo(file, true));
        {
            QFile f(file.toLocalFile());
            QVERIFY(f.open(QIODevice::ReadOnly));
            QVERIFY(f.readAll().contains("\n[HKEY_CURRENT_USER\\Config\\kdeglobals\\General]\n"));
        }
        const QByteArray before = get("kdeglobals");
        r.select(r.indexOfPath("HKEY_CURRENT_USER\\Config\\kdeglobals\\General"));
        r.setValue("ColorScheme", "Other");
        r.deleteValue("AccentColor");
        QCOMPARE(r.prepareImport(file), 3);
        QCOMPARE(r.importSkipped(), 0);
        r.applyImport();
        QCOMPARE(get("kdeglobals").count("ColorScheme=BreezeDark"), 1);
        QCOMPARE(get("kdeglobals").count("AccentColor=123,60,255"), 1);
        Q_UNUSED(before);

        QFile bad(m_tmp.filePath("bad.reg"));
        QVERIFY(bad.open(QIODevice::WriteOnly));
        bad.write("Raye Registry Editor 1\n\n[HKEY_CURRENT_USER\\Config\\nothere\\G]\n\"a\"=\"b\"\n");
        bad.close();
        QCOMPARE(r.prepareImport(QUrl::fromLocalFile(bad.fileName())), 0);
        QCOMPARE(r.importSkipped(), 1);
    }

    void favourites()
    {
        Registry r;
        r.tree()->setPlaces(m_config, m_tmp.filePath("root"));
        r.select(r.indexOfPath("HKEY_CURRENT_USER\\Config\\kdeglobals\\KDE"));
        r.addFavourite("Clicks");
        Registry again;
        again.tree()->setPlaces(m_config, m_tmp.filePath("root"));
        QCOMPARE(again.favouriteNames(), QStringList{"Clicks"});
        again.openFavourite("Clicks");
        QCOMPARE(again.currentPath(), QStringLiteral("Computer\\HKEY_CURRENT_USER\\Config\\kdeglobals\\KDE"));
        again.removeFavourite("Clicks");
        QVERIFY(again.favouriteNames().isEmpty());
    }

    // a conffile rewritten here would keep every later update's defaults out
    void packageFilesAreReadOnly()
    {
        const QString root = m_tmp.filePath("pkgroot");
        auto write = [&](const QString &rel, const QByteArray &data) {
            QDir().mkpath(QFileInfo(root + rel).path());
            QFile f(root + rel);
            QVERIFY(f.open(QIODevice::WriteOnly));
            f.write(data);
        };
        write("/etc/xdg/kwinrc", "[Windows]\nBorderSnapZone=10\n");
        write("/etc/xdg/mine", "[A]\nb=1\n");
        write("/etc/xdg/kwinrc.dpkg-dist", "[Windows]\nBorderSnapZone=12\n");
        write("/etc/systemd/system.conf", "[Manager]\nLogLevel=info\nDefaultEnvironment=\n");
        write("/var/lib/dpkg/info/raye-theme.conffiles", "/etc/xdg/kwinrc\n");
        Registry r;
        r.tree()->setPlaces(m_config, root);
        QVERIFY(!r.indexOfPath("HKEY_LOCAL_MACHINE\\Defaults\\kwinrc.dpkg-dist").isValid());
        // straight to the file, never opened: still read only
        r.select(r.indexOfPath("HKEY_LOCAL_MACHINE\\Defaults\\kwinrc"));
        QVERIFY(!r.currentWritable());
        QVERIFY(r.currentNote().contains("raye-theme"));
        r.select(r.indexOfPath("HKEY_LOCAL_MACHINE\\Defaults\\kwinrc\\Windows"));
        QVERIFY(r.currentNote().contains("raye-theme"));
        QVERIFY(r.values()->lockedOf("BorderSnapZone"));
        r.select(r.indexOfPath("HKEY_LOCAL_MACHINE\\Defaults\\mine\\A"));
        QCOMPARE(r.currentNote(), QString());
        QVERIFY(!r.values()->lockedOf("b"));
        r.select(r.indexOfPath("HKEY_LOCAL_MACHINE\\System\\systemd\\system.conf\\Manager"));
        QVERIFY(r.values()->lockedOf("DefaultEnvironment"));
        QVERIFY(!r.values()->lockedOf("LogLevel"));
    }

    // a change that cannot be taken back stays on the undo list
    void failedUndoIsKept()
    {
        Registry r;
        r.tree()->setPlaces(m_config, m_tmp.filePath("root"));
        r.select(r.indexOfPath("HKEY_CURRENT_USER\\Config\\gtk-3.0\\settings.ini\\Settings"));
        r.setValue("gtk-font-name", "Inter 11");
        QVERIFY(r.canUndo());
        const QByteArray saved = get("gtk-3.0/settings.ini");
        QVERIFY(QFile::remove(m_config + "/gtk-3.0/settings.ini"));
        QVERIFY(QDir().mkdir(m_config + "/gtk-3.0/settings.ini"));
        QSignalSpy failed(&r, &Registry::failed);
        r.undo();
        QCOMPARE(failed.count(), 1);
        // said in the window's words, not the file layer's
        QVERIFY(!failed.at(0).at(0).toString().contains("not a file"));
        QVERIFY(r.canUndo());
        QVERIFY(QDir().rmdir(m_config + "/gtk-3.0/settings.ini"));
        put("gtk-3.0/settings.ini", saved);
        r.undo();
        QVERIFY(!r.canUndo());
        QCOMPARE(get("gtk-3.0/settings.ini"), QByteArray("[Settings]\ngtk-font-name=Inter 10\n"));
    }
};

QTEST_GUILESS_MAIN(TestRegistry)
#include "test_registry.moc"

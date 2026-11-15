// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

// The search box: src/search.js, run as the window runs it.

#include <QFile>
#include <QJSEngine>
#include <QTest>

class TestSearch : public QObject
{
    Q_OBJECT

    QJSEngine m_js;

    QJSValue call(const QString &function, const QJSValueList &args)
    {
        QJSValue result = m_js.globalObject().property(function).call(args);
        if (result.isError())
            qWarning("%s", qPrintable(result.toString()));
        return result;
    }

    // [{ item, tasks }] the way the window hands them over
    QJSValue entries()
    {
        return m_js.evaluate(QStringLiteral(R"([
            { item: { name: "Disk Encryption", summary: "Whether your disk is encrypted",
                      keywords: "bitlocker, recovery key",
                      tasks: [{ text: "Check disk encryption" }, { text: "Back up your recovery key" }] },
              tasks: [{ text: "Check disk encryption" }, { text: "Back up your recovery key" }] },
            { item: { name: "Affichage", summary: "Résolution de l’écran", keywords: "",
                      tasks: [{ text: "Ajuster la résolution" }] },
              tasks: [{ text: "Ajuster la résolution" }] },
            { item: { name: "Sound", summary: "Speakers", keywords: "",
                      tasks: [{ text: "One" }, { text: "Two" }, { text: "Three" }, { text: "Four" }] },
              tasks: [{ text: "One" }, { text: "Two" }, { text: "Three" }, { text: "Four" }] },
            { item: { name: "Network", summary: "", keywords: "",
                      tasks: [{ text: "Connect to a Wi-Fi network" }, { text: "Set up a VPN" }] },
              tasks: [{ text: "Set up a VPN" }] }
        ])"));
    }

    QStringList names(const QJSValue &results)
    {
        QStringList out;
        for (int i = 0; i < results.property(QStringLiteral("length")).toInt(); ++i)
            out << results.property(i).property(QStringLiteral("item")).property(QStringLiteral("name")).toString();
        return out;
    }

    QStringList tasks(const QJSValue &results, int index)
    {
        QStringList out;
        const QJSValue list = results.property(index).property(QStringLiteral("tasks"));
        for (int i = 0; i < list.property(QStringLiteral("length")).toInt(); ++i)
            out << list.property(i).property(QStringLiteral("text")).toString();
        return out;
    }

    QJSValue search(const QString &query)
    {
        return call(QStringLiteral("results"), {query, entries()});
    }

private Q_SLOTS:
    void initTestCase()
    {
        QFile f(QStringLiteral(SEARCH_JS));
        QVERIFY(f.open(QIODevice::ReadOnly));
        // the pragma is for QML; the rest is plain script
        const QString code = QString::fromUtf8(f.readAll()).replace(QStringLiteral(".pragma library"), QString());
        const QJSValue loaded = m_js.evaluate(code, QStringLiteral(SEARCH_JS));
        QVERIFY2(!loaded.isError(), qPrintable(loaded.toString()));
    }

    void emptySearchFindsNothing()
    {
        QVERIFY(names(search(QString())).isEmpty());
        QVERIFY(names(search(QStringLiteral("   "))).isEmpty());
    }

    void showsOnlyTheTasksThatMatch()
    {
        const QJSValue r = search(QStringLiteral("recovery"));
        QCOMPARE(names(r), QStringList{QStringLiteral("Disk Encryption")});
        QCOMPARE(tasks(r, 0), QStringList{QStringLiteral("Back up your recovery key")});
    }

    void nameOnlyMatchShowsFirstTasks()
    {
        const QJSValue r = search(QStringLiteral("sound"));
        QCOMPARE(names(r), QStringList{QStringLiteral("Sound")});
        QCOMPARE(tasks(r, 0).size(), 3);
    }

    void findsByKeywords()
    {
        QCOMPARE(names(search(QStringLiteral("bitlocker"))), QStringList{QStringLiteral("Disk Encryption")});
    }

    void ignoresAccentsAndCase()
    {
        QCOMPARE(names(search(QStringLiteral("RESOLUTION ecran"))), QStringList{QStringLiteral("Affichage")});
        QCOMPARE(names(search(QStringLiteral("écran"))), QStringList{QStringLiteral("Affichage")});
    }

    void everyWordHasToBeThere()
    {
        QVERIFY(names(search(QStringLiteral("disk speakers"))).isEmpty());
        QCOMPARE(names(search(QStringLiteral("check disk"))), QStringList{QStringLiteral("Disk Encryption")});
    }

    void neverShowsATaskThisPcCannotOpen()
    {
        // Wi-Fi is in the item's words but not in what this PC can open
        const QJSValue r = search(QStringLiteral("wi-fi"));
        QCOMPARE(names(r), QStringList{QStringLiteral("Network")});
        QCOMPARE(tasks(r, 0), QStringList{QStringLiteral("Set up a VPN")});
    }
};

QTEST_GUILESS_MAIN(TestSearch)
#include "test_search.moc"

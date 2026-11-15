// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "edits.h"

#include <QDirIterator>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace Reg;

namespace
{
void put(const QString &path, const QByteArray &data)
{
    QFile f(path);
    QVERIFY(f.open(QIODevice::WriteOnly));
    f.write(data);
}

QByteArray get(const QString &path)
{
    QFile f(path);
    return f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray();
}

int countFiles(const QString &dir)
{
    int n = 0;
    QDirIterator it(dir, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        ++n;
    }
    return n;
}
}

class TestEdits : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void setKeepsACopyAndTheMode()
    {
        QTemporaryDir tmp;
        const QString file = tmp.filePath("kdeglobals");
        put(file, "# mine\n[General]\nColorScheme=BreezeDark\n");
        QFile::setPermissions(file, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        const Change c{Change::Set, file, Format::KConfig, {"General"}, "ColorScheme", "RayeNight"};
        const Outcome o = applyChange(c, tmp.filePath("backups"));
        QVERIFY2(o.ok, qPrintable(o.error));
        QVERIFY(o.changed);
        QVERIFY(o.hadValue);
        QCOMPARE(o.oldText, QStringLiteral("BreezeDark"));
        QCOMPARE(get(file), QByteArray("# mine\n[General]\nColorScheme=RayeNight\n"));
        QCOMPARE(get(o.backup), QByteArray("# mine\n[General]\nColorScheme=BreezeDark\n"));
        const auto others = QFileDevice::ReadGroup | QFileDevice::WriteGroup | QFileDevice::ReadOther | QFileDevice::WriteOther;
        QCOMPARE(QFile::permissions(file) & others, QFileDevice::Permissions());
        QCOMPARE(QFile::permissions(o.backup) & (QFileDevice::ReadOther | QFileDevice::ReadGroup), QFileDevice::Permissions());
    }

    void sameValueWritesNothing()
    {
        QTemporaryDir tmp;
        const QString file = tmp.filePath("kwinrc");
        put(file, "[A]\nk=1\n");
        const Outcome o = applyChange({Change::Set, file, Format::KConfig, {"A"}, "k", "1"}, tmp.filePath("b"));
        QVERIFY(o.ok);
        QVERIFY(!o.changed);
        QCOMPARE(countFiles(tmp.filePath("b")), 0);
    }

    void undoBringsTheFileBack()
    {
        QTemporaryDir tmp;
        const QString file = tmp.filePath("x.conf");
        const QByteArray before = "[A]\nk = 1\n";
        put(file, before);
        const Change add{Change::Set, file, Format::Ini, {"A"}, "new", "2"};
        const Outcome a = applyChange(add, tmp.filePath("b"));
        QVERIFY(a.ok && !a.hadValue);
        const Change edit{Change::Set, file, Format::Ini, {"A"}, "k", "5"};
        const Outcome e = applyChange(edit, tmp.filePath("b"));
        QVERIFY(e.ok && e.hadValue);
        QCOMPARE(get(file), QByteArray("[A]\nk = 5\nnew=2\n"));
        QVERIFY(applyChange(inverseOf(edit, e), tmp.filePath("b")).ok);
        QVERIFY(applyChange(inverseOf(add, a), tmp.filePath("b")).ok);
        QCOMPARE(get(file), before);
    }

    void refusalLeavesTheFile()
    {
        QTemporaryDir tmp;
        const QString file = tmp.filePath("kdeglobals");
        const QByteArray before = "[A]\nk[$i]=1\n";
        put(file, before);
        const Outcome o = applyChange({Change::Set, file, Format::KConfig, {"A"}, "k", "2"}, tmp.filePath("b"));
        QVERIFY(!o.ok);
        QVERIFY(!o.error.isEmpty());
        QCOMPARE(get(file), before);
        QCOMPARE(countFiles(tmp.filePath("b")), 0);
        QVERIFY(!applyChange({Change::Set, tmp.filePath("none"), Format::Ini, {"A"}, "k", "2"}, tmp.filePath("b")).ok);
    }

    void aLinkStaysALink()
    {
        QTemporaryDir tmp;
        const QString real = tmp.filePath("real.conf");
        put(real, "[A]\nk=1\n");
        const QString link = tmp.filePath("link.conf");
        QVERIFY(QFile::link(real, link));
        QVERIFY(applyChange({Change::Set, link, Format::Ini, {"A"}, "k", "2"}, tmp.filePath("b")).ok);
        QVERIFY(QFileInfo(link).isSymLink());
        QCOMPARE(get(real), QByteArray("[A]\nk=2\n"));
    }

    void oldCopiesGo()
    {
        QTemporaryDir tmp;
        const QString file = tmp.filePath("c.conf");
        put(file, "[A]\nk=0\n");
        for (int i = 1; i <= 7; ++i)
            QVERIFY(applyChange({Change::Set, file, Format::Ini, {"A"}, "k", QString::number(i)}, tmp.filePath("b"), 3).ok);
        QCOMPARE(countFiles(tmp.filePath("b")), 3);
    }

    void types()
    {
        QCOMPARE(guessType("true", Format::KConfig), ValueType::YesNo);
        QCOMPARE(guessType("yes", Format::Ini), ValueType::YesNo);
        QCOMPARE(guessType("yes", Format::KConfig), ValueType::Text);
        QCOMPARE(guessType("-12", Format::Shell), ValueType::Number);
        QCOMPARE(guessType("123,60,255", Format::KConfig), ValueType::List);
        QCOMPARE(guessType("123,60,255", Format::Ini), ValueType::Text);
        QVERIFY(fitsType("true", "false", ValueType::YesNo));
        QVERIFY(!fitsType("true", "no", ValueType::YesNo));
        QVERIFY(fitsType("no", "yes", ValueType::YesNo));
        QVERIFY(!fitsType("5", "five", ValueType::Number));
        QVERIFY(fitsType("5", "10", ValueType::Number));
    }
};

QTEST_GUILESS_MAIN(TestEdits)
#include "test_edits.moc"

// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// The share picker: its logic (PickerLogic.js), then the window itself
// with fake screens and windows, loaded the way the backend loads it.
// The window part needs org.raye.ui (raye-style, or the theme's source
// tree next to this repo); without it those tests are skipped.

#include "fakesources.h"
#include "pickerloader.h"

#include <QJSValue>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSignalSpy>
#include <QTest>

#include <memory>

Q_IMPORT_QML_PLUGIN(org_raye_portal_pickerPlugin)

using namespace Qt::StringLiterals;

namespace
{
constexpr uint Monitor = 1;
constexpr uint Window = 2;
constexpr uint Virtual = 4;

QVariant plain(const QVariant &value)
{
    if (value.metaType() == QMetaType::fromType<QJSValue>()) {
        return value.value<QJSValue>().toVariant();
    }
    return value;
}

struct Picker {
    std::unique_ptr<QQmlApplicationEngine> engine;
    // goes first, before the engine and the models (as in QuickDialog)
    std::unique_ptr<QQuickWindow> owner;
    QQuickWindow *window = nullptr;
    FakeOutputs *outputs = nullptr;
    FakeWindows *windows = nullptr;

    QVariant prop(const char *name) const
    {
        return plain(window->property(name));
    }
    QQuickItem *item(const QString &name) const
    {
        return window->findChild<QQuickItem *>(name);
    }
    QVariant call(const char *function, const QVariant &arg = {}) const
    {
        QVariant result;
        if (arg.isValid()) {
            QMetaObject::invokeMethod(window, function, Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, arg));
        } else {
            QMetaObject::invokeMethod(window, function, Q_RETURN_ARG(QVariant, result));
        }
        return plain(result);
    }
    QQuickItem *card(int index) const
    {
        // the view makes its cards once the window is on screen
        if (!window->isExposed()) {
            window->show();
            if (!QTest::qWaitForWindowExposed(window)) {
                qWarning() << "the picker never showed";
            }
        }
        QQuickItem *cell = nullptr;
        QMetaObject::invokeMethod(item(u"grid"_s), "forceLayout");
        QMetaObject::invokeMethod(item(u"grid"_s), "itemAtIndex", Q_RETURN_ARG(QQuickItem *, cell), Q_ARG(int, index));
        QQuickItem *found = cell ? cell->findChild<QQuickItem *>(u"card"_s) : nullptr;
        if (!found) {
            qWarning() << "no card" << index << "cell" << cell;
        }
        return found;
    }
    int gridCount() const
    {
        return item(u"grid"_s)->property("count").toInt();
    }
};
}

class TestSharePicker : public QObject
{
    Q_OBJECT

    std::unique_ptr<QQmlEngine> m_logicEngine;
    QObject *m_logic = nullptr;
    QString m_rayeUi;

    QVariant logic(const QString &function, const QVariantList &args)
    {
        QVariant result;
        QMetaObject::invokeMethod(m_logic, "call", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, function), Q_ARG(QVariant, args));
        return plain(result);
    }

    struct Options {
        QList<QSize> screens = {{2560, 1440}};
        int windows = 3;
        QStringList windowTitles; // instead of `windows` when set
        uint sourceTypes = Monitor | Window;
        bool multiple = false;
        uint cursorMode = 2;
        bool persist = false;
        QString appName = u"Discord"_s;
        bool appKnown = true;
    };

    Picker open(const Options &o)
    {
        Picker p;
        p.engine = std::make_unique<QQmlApplicationEngine>();
        PickerLoader::findRayeUi(*p.engine);
        const bool screens = (o.sourceTypes & (Monitor | Virtual)) != 0;
        const bool windows = (o.sourceTypes & Window) != 0;
        if (screens) {
            p.outputs = new FakeOutputs(o.screens, o.sourceTypes, p.engine.get());
        }
        if (windows) {
            p.windows = o.windowTitles.isEmpty() ? new FakeWindows(o.windows, p.engine.get()) : new FakeWindows(o.windowTitles, p.engine.get());
        }
        const QVariantMap props = {
            {u"title"_s, u"Share screen with %1"_s.arg(o.appName)},
            {u"mainText"_s, u"Choose what to share with %1:"_s.arg(o.appName)},
            {u"appId"_s, u"com.discordapp.Discord"_s},
            {u"appName"_s, o.appName},
            {u"appIconName"_s, u"com.discordapp.Discord"_s},
            {u"appKnown"_s, o.appKnown},
            {u"outputsModel"_s, QVariant::fromValue<QObject *>(p.outputs)},
            {u"windowsModel"_s, QVariant::fromValue<QObject *>(p.windows)},
            {u"multiple"_s, o.multiple},
            {u"sourceTypes"_s, o.sourceTypes},
            {u"cursorMode"_s, o.cursorMode},
            {u"persistRequested"_s, o.persist},
            {u"previewComponent"_s, QVariant::fromValue(PickerLoader::fakePreview(*p.engine))},
        };
        QString error;
        p.window = PickerLoader::load(*p.engine, props, &error);
        p.owner.reset(p.window);
        if (!p.window) {
            qWarning().noquote() << error;
            return p;
        }
        // as ScreenChooserDialog does
        if (p.outputs) {
            connect(p.window, SIGNAL(clearSelection()), p.outputs, SLOT(clearSelection()));
        }
        if (p.windows) {
            connect(p.window, SIGNAL(clearSelection()), p.windows, SLOT(clearSelection()));
        }
        return p;
    }

#define NEED_RAYE_UI()                                                                                                                                         \
    if (m_rayeUi.isEmpty())                                                                                                                                    \
    QSKIP("org.raye.ui is not installed and the theme's source tree is not next to this repo")

private Q_SLOTS:
    void initTestCase()
    {
        PickerLoader::useCatalog(u"en"_s, {});
        m_logicEngine = std::make_unique<QQmlEngine>();
        QQmlComponent component(m_logicEngine.get());
        component.setData(
            "import QtQuick\n"
            "import \"qrc:/qt/qml/org/raye/portal/picker/PickerLogic.js\" as L\n"
            "QtObject { function call(name, args) { return L[name].apply(null, args) } }\n",
            QUrl(u"logic.qml"_s));
        m_logic = component.create();
        QVERIFY2(m_logic, qPrintable(component.errorString()));
        m_logic->setParent(m_logicEngine.get());

        QQmlApplicationEngine probe;
        m_rayeUi = PickerLoader::findRayeUi(probe);
        qInfo() << "org.raye.ui:" << (m_rayeUi.isEmpty() ? u"not found"_s : m_rayeUi);
    }

    // --- logic ----------------------------------------------------------

    void syntheticKinds()
    {
        QCOMPARE(logic(u"syntheticKinds"_s, {Monitor, 1, 2}).toStringList(), (QStringList{u"virtual"_s, u"region"_s}));
        QCOMPARE(logic(u"syntheticKinds"_s, {Monitor | Window, 2, 3}).toStringList(),
                 (QStringList{u"workspace"_s, u"virtual"_s, u"region"_s}));
        QCOMPARE(logic(u"syntheticKinds"_s, {Virtual, 0, 1}).toStringList(), QStringList{u"virtual"_s});
        QCOMPARE(logic(u"syntheticKinds"_s, {Window, 0, 0}).toStringList(), QStringList{});
        // a backend that changed its rows: nothing is guessed
        QCOMPARE(logic(u"syntheticKinds"_s, {Monitor, 1, 3}).toStringList(), (QStringList{u"other"_s, u"other"_s, u"other"_s}));
    }

    void outputKind()
    {
        // two screens, all screens, virtual, region
        QCOMPARE(logic(u"outputKind"_s, {0, 5, 3, Monitor}).toString(), u"screen"_s);
        QCOMPARE(logic(u"outputKind"_s, {1, 5, 3, Monitor}).toString(), u"screen"_s);
        QCOMPARE(logic(u"outputKind"_s, {2, 5, 3, Monitor}).toString(), u"workspace"_s);
        QCOMPARE(logic(u"outputKind"_s, {3, 5, 3, Monitor}).toString(), u"virtual"_s);
        QCOMPARE(logic(u"outputKind"_s, {4, 5, 3, Monitor}).toString(), u"region"_s);
        QCOMPARE(logic(u"tabOfKind"_s, {u"region"_s}).toString(), u"area"_s);
        QCOMPARE(logic(u"tabOfKind"_s, {u"workspace"_s}).toString(), u"screen"_s);
    }

    void tabs()
    {
        QCOMPARE(logic(u"tabs"_s, {Monitor, true, false, 3, 2}).toStringList(), (QStringList{u"screen"_s, u"area"_s}));
        QCOMPARE(logic(u"tabs"_s, {Monitor | Window, true, true, 3, 2}).toStringList(), (QStringList{u"screen"_s, u"window"_s, u"area"_s}));
        QCOMPARE(logic(u"tabs"_s, {Window, false, true, 0, 0}).toStringList(), QStringList{u"window"_s});
        QCOMPARE(logic(u"tabs"_s, {Virtual, true, false, 1, 1}).toStringList(), QStringList{u"screen"_s});
        QCOMPARE(logic(u"tabs"_s, {Monitor, false, false, 0, 0}).toStringList(), QStringList{});
    }

    void requestKind()
    {
        QCOMPARE(logic(u"requestKind"_s, {Window}).toString(), u"window"_s);
        QCOMPARE(logic(u"requestKind"_s, {Virtual}).toString(), u"virtual"_s);
        QCOMPARE(logic(u"requestKind"_s, {Monitor}).toString(), u"screen"_s);
        QCOMPARE(logic(u"requestKind"_s, {Monitor | Window}).toString(), u"screen"_s);
    }

    void pixelSize()
    {
        const QVariantMap scaled = logic(u"pixelSize"_s, {1280, 720, 1.25}).toMap();
        QCOMPARE(scaled.value(u"width"_s).toInt(), 1600);
        QCOMPARE(scaled.value(u"height"_s).toInt(), 900);
        const QVariantMap noScale = logic(u"pixelSize"_s, {1920, 1080, 0}).toMap();
        QCOMPARE(noScale.value(u"width"_s).toInt(), 1920);
        QCOMPARE(logic(u"screenNumber"_s, {0}).toInt(), 1);
    }

    void preselectedRow()
    {
        QCOMPARE(logic(u"preselectedRow"_s, {Monitor, 3, 2}).toInt(), 0);
        QCOMPARE(logic(u"preselectedRow"_s, {Monitor, 5, 3}).toInt(), -1);
        QCOMPARE(logic(u"preselectedRow"_s, {Monitor | Window, 3, 2}).toInt(), -1);
        QCOMPARE(logic(u"preselectedRow"_s, {Virtual, 1, 1}).toInt(), 0);
        QCOMPARE(logic(u"preselectedRow"_s, {Virtual | Window, 1, 1}).toInt(), -1);
        QCOMPARE(logic(u"preselectedRow"_s, {Window, 0, 0}).toInt(), -1);
    }

    void offersPointerAndColumns()
    {
        QVERIFY(!logic(u"offersPointer"_s, {0}).toBool());
        QVERIFY(!logic(u"offersPointer"_s, {1}).toBool());
        QVERIFY(logic(u"offersPointer"_s, {2}).toBool());
        QVERIFY(logic(u"offersPointer"_s, {4}).toBool());
        QCOMPARE(logic(u"columns"_s, {712, 232}).toInt(), 3);
        QCOMPARE(logic(u"columns"_s, {100, 232}).toInt(), 1);
        QCOMPARE(logic(u"columns"_s, {100, 0}).toInt(), 1);
    }

    void escapeHtmlAndTitles()
    {
        QCOMPARE(logic(u"escapeHtml"_s, {u"<b>A & \"B\" 'C'</b>"_s}).toString(), u"&lt;b&gt;A &amp; &quot;B&quot; &#39;C&#39;&lt;/b&gt;"_s);
        QCOMPARE(logic(u"escapeHtml"_s, {QVariant()}).toString(), QString());
        QCOMPARE(logic(u"windowTitle"_s, {u"  Notes  "_s}).toString(), u"Notes"_s);
        QCOMPARE(logic(u"windowTitle"_s, {QVariant()}).toString(), QString());
    }

    void nextTab()
    {
        const QStringList list = {u"screen"_s, u"window"_s, u"area"_s};
        QCOMPARE(logic(u"nextTab"_s, {list, u"screen"_s, 1}).toString(), u"window"_s);
        QCOMPARE(logic(u"nextTab"_s, {list, u"area"_s, 1}).toString(), u"screen"_s);
        QCOMPARE(logic(u"nextTab"_s, {list, u"screen"_s, -1}).toString(), u"area"_s);
        QCOMPARE(logic(u"nextTab"_s, {list, u"gone"_s, 1}).toString(), u"screen"_s);
        QCOMPARE(logic(u"nextTab"_s, {QStringList{}, u"screen"_s, 1}).toString(), QString());
    }

    // --- the window -----------------------------------------------------

    void loadsWithTheContract()
    {
        NEED_RAYE_UI();
        Picker p = open({});
        QVERIFY(p.window);
        QCOMPARE(p.window->width(), 760);
        QCOMPARE(p.window->height(), 540);
        QVERIFY(p.window->flags() & Qt::Dialog);
        // what the backend reads back
        QCOMPARE(p.prop("allowRestore").toBool(), false);
        QCOMPARE(p.prop("showPointer").toBool(), true);
        QVERIFY(p.window->metaObject()->indexOfSignal("accept()") >= 0);
        QVERIFY(p.window->metaObject()->indexOfSignal("reject()") >= 0);
        QVERIFY(p.window->metaObject()->indexOfSignal("clearSelection()") >= 0);
        QVERIFY(p.window->metaObject()->indexOfMethod("present()") >= 0);
        QCOMPARE(p.prop("tabList").toStringList(), (QStringList{u"screen"_s, u"window"_s, u"area"_s}));
    }

    void headline()
    {
        NEED_RAYE_UI();
        {
            Picker p = open({});
            QVERIFY(p.window);
            QCOMPARE(p.item(u"headline"_s)->property("text").toString(), u"<b>Discord</b> wants to share your screen"_s);
        }
        {
            Options o;
            o.appName = u"A<i>pp</i> & co"_s;
            Picker p = open(o);
            QCOMPARE(p.item(u"headline"_s)->property("text").toString(), u"<b>A&lt;i&gt;pp&lt;/i&gt; &amp; co</b> wants to share your screen"_s);
        }
        {
            Options o;
            o.appName = u"An app"_s;
            o.appKnown = false;
            o.sourceTypes = Window;
            Picker p = open(o);
            QCOMPARE(p.item(u"headline"_s)->property("text").toString(), u"An app wants to share a window"_s);
            QCOMPARE(p.item(u"headline"_s)->property("plainText").toString(), u"An app wants to share a window"_s);
        }
    }

    void cardsAndLabels()
    {
        NEED_RAYE_UI();
        Options o;
        o.screens = {{2560, 1440}, {1920, 1080}};
        Picker p = open(o);
        QVERIFY(p.window);
        // two screens, all screens, a virtual screen
        QCOMPARE(p.gridCount(), 4);
        QCOMPARE(p.card(0)->property("label").toString(), u"Screen 1 · 2560 × 1440"_s);
        QCOMPARE(p.card(1)->property("label").toString(), u"Screen 2 · 1920 × 1080"_s);
        QCOMPARE(p.card(2)->property("label").toString(), u"All screens"_s);
        QCOMPARE(p.card(2)->property("kind").toString(), u"workspace"_s);
        QCOMPARE(p.card(3)->property("label").toString(), u"New virtual screen"_s);
        QCOMPARE(p.item(u"grid"_s)->property("columns").toInt(), 3);

        p.window->setProperty("currentTab", u"window"_s);
        QCOMPARE(p.gridCount(), 3);
        QCOMPARE(p.card(0)->property("label").toString(), u"Notes for the meeting - Notepad"_s);
        QCOMPARE(p.card(0)->property("kind").toString(), u"window"_s);

        p.window->setProperty("currentTab", u"area"_s);
        QCOMPARE(p.gridCount(), 1);
        QCOMPARE(p.card(0)->property("label").toString(), u"Select an area"_s);
    }

    void manyWindows()
    {
        NEED_RAYE_UI();
        Options o;
        o.windows = 30;
        o.screens = {{1920, 1080}, {1920, 1080}, {1920, 1080}, {1920, 1080}, {1920, 1080}, {1920, 1080}};
        Picker p = open(o);
        QVERIFY(p.window);
        QCOMPARE(p.gridCount(), 8);
        p.window->setProperty("currentTab", u"window"_s);
        QCOMPARE(p.gridCount(), 30);
    }

    // window titles come from any app or web page: shown as they are,
    // never as markup (rich text could even fetch a picture from the web)
    void titlesAreNotMarkup()
    {
        NEED_RAYE_UI();
        Options o;
        o.windowTitles = {u"<img src=\"https://example.com/x.png\"><b>Bold</b>"_s, u"   "_s};
        o.appName = u"<b>Evil</b>"_s;
        o.persist = true;
        Picker p = open(o);
        QVERIFY(p.window);
        p.window->setProperty("currentTab", u"window"_s);
        QQuickItem *card = p.card(0);
        QCOMPARE(card->property("label").toString(), o.windowTitles.first());
        QQuickItem *caption = card->findChild<QQuickItem *>(u"caption"_s);
        QVERIFY(caption);
        QCOMPARE(caption->property("textFormat").toInt(), 0); // Text.PlainText
        // a title of spaces is no title
        QCOMPARE(p.card(1)->property("label").toString(), u"Untitled window"_s);
        QCOMPARE(p.item(u"rememberOption"_s)->property("text").toString(), u"Remember this choice for <b>Evil</b>"_s);
        QQuickItem *remember = p.item(u"rememberOption"_s)->property("contentItem").value<QQuickItem *>();
        QCOMPARE(remember->property("textFormat").toInt(), 0);
    }

    void noWindows()
    {
        NEED_RAYE_UI();
        Options o;
        o.sourceTypes = Window;
        o.windows = 0;
        Picker p = open(o);
        QVERIFY(p.window);
        QCOMPARE(p.prop("tabList").toStringList(), QStringList{u"window"_s});
        QVERIFY(!p.item(u"tabs"_s)->isVisible());
        QCOMPARE(p.gridCount(), 0);
        QVERIFY(p.item(u"emptyState"_s)->isVisible());
        QVERIFY(!p.item(u"shareButton"_s)->isEnabled());
    }

    void singleChoice()
    {
        NEED_RAYE_UI();
        Options o;
        o.screens = {{2560, 1440}, {1920, 1080}};
        Picker p = open(o);
        QVERIFY(p.window);
        QVERIFY(p.outputs->selectedRows().isEmpty());
        QVERIFY(!p.item(u"shareButton"_s)->isEnabled());

        p.call("choose", 0);
        p.call("choose", 1);
        QCOMPARE(p.outputs->selectedRows(), QList<int>{1});
        QVERIFY(p.item(u"shareButton"_s)->isEnabled());
        QCOMPARE(p.card(1)->property("selected").toBool(), true);
        QCOMPARE(p.card(0)->property("selected").toBool(), false);

        // a window instead: the screen is no longer chosen
        p.window->setProperty("currentTab", u"window"_s);
        p.call("choose", 2);
        QVERIFY(p.outputs->selectedRows().isEmpty());
        QCOMPARE(p.windows->selectedRows(), QList<int>{2});
    }

    void severalChoices()
    {
        NEED_RAYE_UI();
        Options o;
        o.screens = {{2560, 1440}, {1920, 1080}};
        o.multiple = true;
        Picker p = open(o);
        QVERIFY(p.window);
        p.call("choose", 0);
        p.call("choose", 1);
        QCOMPARE(p.outputs->selectedRows(), (QList<int>{0, 1}));
        p.call("choose", 1);
        QCOMPARE(p.outputs->selectedRows(), QList<int>{0});
        p.window->setProperty("currentTab", u"window"_s);
        p.call("choose", 1);
        QCOMPARE(p.outputs->selectedRows(), QList<int>{0});
        QCOMPARE(p.windows->selectedRows(), QList<int>{1});
        QCOMPARE(p.card(1)->property("multiple").toBool(), true);

        // a double click shares just that one
        QSignalSpy accepted(p.window, SIGNAL(accept()));
        p.call("chooseAndShare", 2);
        QCOMPARE(accepted.count(), 1);
        QVERIFY(p.outputs->selectedRows().isEmpty());
        QCOMPARE(p.windows->selectedRows(), QList<int>{2});
    }

    void preselectsTheOnlyScreen()
    {
        NEED_RAYE_UI();
        Options o;
        o.sourceTypes = Monitor;
        Picker p = open(o);
        QVERIFY(p.window);
        QCOMPARE(p.outputs->selectedRows(), QList<int>{0});
        QCOMPARE(p.prop("tabList").toStringList(), (QStringList{u"screen"_s, u"area"_s}));

        Options two = o;
        two.screens = {{2560, 1440}, {1920, 1080}};
        Picker q = open(two);
        QVERIFY(q.outputs->selectedRows().isEmpty());

        Options virtualOnly;
        virtualOnly.sourceTypes = Virtual;
        Picker v = open(virtualOnly);
        QCOMPARE(v.outputs->selectedRows(), QList<int>{0});
        QCOMPARE(v.prop("tabList").toStringList(), QStringList{u"screen"_s});
        QCOMPARE(v.card(0)->property("kind").toString(), u"virtual"_s);
        QCOMPARE(v.item(u"headline"_s)->property("text").toString(), u"<b>Discord</b> wants to share a new virtual screen"_s);
    }

    void shareAndCancel()
    {
        NEED_RAYE_UI();
        Picker p = open({});
        QVERIFY(p.window);
        QSignalSpy accepted(p.window, SIGNAL(accept()));
        QSignalSpy rejected(p.window, SIGNAL(reject()));

        p.call("share");
        QCOMPARE(accepted.count(), 0);

        p.call("choose", 0);
        p.call("share");
        p.call("share");
        QCOMPARE(accepted.count(), 1);
        QVERIFY(!p.window->isVisible());

        // the user gave up picking an area: the backend shows it again
        p.call("present");
        QVERIFY(p.window->isVisible());
        p.call("share");
        QCOMPARE(accepted.count(), 2);

        p.call("present");
        p.window->close();
        QCOMPARE(rejected.count(), 1);
        p.call("cancel");
        QCOMPARE(rejected.count(), 1);
    }

    void keyboard()
    {
        NEED_RAYE_UI();
        Options o;
        o.screens = {{2560, 1440}, {1920, 1080}};
        Picker p = open(o);
        QVERIFY(p.window);
        QSignalSpy accepted(p.window, SIGNAL(accept()));
        QSignalSpy rejected(p.window, SIGNAL(reject()));
        p.call("present");
        QVERIFY(QTest::qWaitForWindowExposed(p.window));
        QQuickItem *grid = p.item(u"grid"_s);
        QTRY_VERIFY(grid->hasActiveFocus());

        QTest::keyClick(p.window, Qt::Key_Right);
        QCOMPARE(grid->property("currentIndex").toInt(), 1);
        QVERIFY(p.prop("keyboardUsed").toBool());
        QVERIFY(p.card(1)->property("showFocus").toBool());
        QTest::keyClick(p.window, Qt::Key_Space);
        QCOMPARE(p.outputs->selectedRows(), QList<int>{1});
        QTest::keyClick(p.window, Qt::Key_Left);
        QTest::keyClick(p.window, Qt::Key_Return);
        QCOMPARE(accepted.count(), 1);
        QCOMPARE(p.outputs->selectedRows(), QList<int>{0});

        // Tab goes up to the tabs, where the arrows change the tab
        p.call("present");
        QTest::qWait(50); // present() gives the cards the focus a moment later
        QTRY_VERIFY(grid->hasActiveFocus());
        QTest::keyClick(p.window, Qt::Key_Backtab);
        QTRY_VERIFY2(p.item(u"tabs"_s)->hasActiveFocus(), qPrintable(QDebug::toString(p.window->activeFocusItem())));
        QTest::keyClick(p.window, Qt::Key_Right);
        QCOMPARE(p.prop("currentTab").toString(), u"window"_s);
        // and Tab from the tabs to the cards, then past them to the options
        QTest::keyClick(p.window, Qt::Key_Tab);
        QTRY_VERIFY(grid->hasActiveFocus());
        QTest::keyClick(p.window, Qt::Key_Tab);
        QTRY_VERIFY2(p.item(u"pointerOption"_s)->hasActiveFocus(), qPrintable(QDebug::toString(p.window->activeFocusItem())));
        QTest::keyClick(p.window, Qt::Key_Escape);
        QCOMPARE(rejected.count(), 1);
    }

    void options()
    {
        NEED_RAYE_UI();
        {
            Options o;
            o.cursorMode = 1;
            Picker p = open(o);
            QVERIFY(!p.item(u"pointerRow"_s)->isVisible());
            QVERIFY(!p.item(u"rememberOption"_s)->isVisible());
            QCOMPARE(p.prop("allowRestore").toBool(), false);
        }
        {
            Options o;
            o.cursorMode = 4;
            o.persist = true;
            Picker p = open(o);
            QVERIFY(p.item(u"pointerRow"_s)->isVisible());
            QVERIFY(p.item(u"rememberOption"_s)->isVisible());
            QCOMPARE(p.item(u"rememberOption"_s)->property("text").toString(), u"Remember this choice for Discord"_s);
            // sharing a screen is not remembered unless the person says so
            QCOMPARE(p.item(u"rememberOption"_s)->property("checked").toBool(), false);
            QCOMPARE(p.prop("allowRestore").toBool(), false);
            p.item(u"rememberOption"_s)->setProperty("checked", true);
            QCOMPARE(p.prop("allowRestore").toBool(), true);
            p.window->setProperty("showPointer", false);
            QCOMPARE(p.item(u"pointerOption"_s)->property("checked").toBool(), false);
        }
    }

    // last: it changes the language of the process
    void french()
    {
        NEED_RAYE_UI();
        if (!PickerLoader::useCatalog(u"fr"_s, QStringLiteral(RAYE_PICKER_MO))) {
            QSKIP("no compiled French catalog, or no UTF-8 locale installed for gettext");
        }
        Options o;
        o.screens = {{2560, 1440}, {1920, 1080}};
        o.persist = true;
        Picker p = open(o);
        QVERIFY(p.window);
        QCOMPARE(p.item(u"headline"_s)->property("text").toString(), u"<b>Discord</b> veut partager votre écran"_s);
        QCOMPARE(p.card(0)->property("label").toString(), u"Écran 1 · 2560 × 1440"_s);
        QCOMPARE(p.card(2)->property("label").toString(), u"Tous les écrans"_s);
        QCOMPARE(p.call("tabText", u"screen"_s).toString(), u"Écran entier"_s);
        QCOMPARE(p.call("tabText", u"window"_s).toString(), u"Fenêtre"_s);
        QCOMPARE(p.call("tabText", u"area"_s).toString(), u"Zone"_s);
        QCOMPARE(p.item(u"shareButton"_s)->property("text").toString(), u"Partager"_s);
        QCOMPARE(p.item(u"rememberOption"_s)->property("text").toString(), u"Mémoriser ce choix pour Discord"_s);
        PickerLoader::useCatalog(u"en"_s, {});
    }
};

QTEST_MAIN(TestSharePicker)
#include "test_sharepicker.moc"

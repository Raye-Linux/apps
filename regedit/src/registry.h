// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "dconfstore.h"
#include "edits.h"
#include "settingsfile.h"

#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QProcess>
#include <QQmlEngine>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

struct RegNode;

// The tree on the left: Computer, the two hives, and under them folders,
// files, groups and dconf folders, each read the first time it is opened.
class RegistryModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    enum Roles { NameRole = Qt::UserRole + 1, KindRole, PathRole };

    explicit RegistryModel(QObject *parent = nullptr);
    ~RegistryModel() override;

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    int columnCount(const QModelIndex &parent = {}) const override;
    bool hasChildren(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    RegNode *node(const QModelIndex &index) const;
    QModelIndex indexOf(RegNode *node) const;
    RegNode *nodeAt(const QString &path) const;
    QString pathOf(const RegNode *node) const;
    RegNode *root() const { return m_root.get(); }
    void load(RegNode *node) const;
    // read a file or a dconf folder again, after a write or on F5
    void reload(RegNode *node);
    void reloadAll();

    // for tests: where ~/.config and / are
    void setPlaces(const QString &configDir, const QString &systemRoot);

private:
    void build();
    std::unique_ptr<RegNode> m_root;
    QString m_configDir;
    QString m_systemRoot = QStringLiteral("/");
};

// The values of the selected key, on the right.
class ValuesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles { NameRole = Qt::UserRole + 1, TypeRole, DataRole, RawRole, LockedRole, KindRole };

    struct Row {
        QString name;
        QString type; // what the window shows: "Text", "Number", "YesNo", "List", or a GVariant type
        QString data; // shown
        QString raw; // edited
        bool locked = false;
        QString kind; // Text Number YesNo List Dconf
    };

    using QAbstractListModel::QAbstractListModel;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    void setRows(const QList<Row> &rows);
    const QList<Row> &rows() const { return m_rows; }
    int count() const { return m_rows.size(); }
    Q_INVOKABLE int indexOfName(const QString &name) const;
    Q_INVOKABLE QString rawOf(const QString &name) const;
    Q_INVOKABLE bool lockedOf(const QString &name) const;

Q_SIGNALS:
    void countChanged();

private:
    QList<Row> m_rows;
};

// What QML talks to: selection, edits, undo, search, export, import and
// favourites.
class Registry : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(RegistryModel *tree READ tree CONSTANT)
    Q_PROPERTY(ValuesModel *values READ values CONSTANT)
    Q_PROPERTY(QString currentPath READ currentPath NOTIFY currentChanged)
    Q_PROPERTY(bool currentWritable READ currentWritable NOTIFY currentChanged)
    Q_PROPERTY(bool currentSystem READ currentSystem NOTIFY currentChanged)
    Q_PROPERTY(bool currentDconf READ currentDconf NOTIFY currentChanged)
    // why the selected key cannot be changed, when there is more to say than "read only"
    Q_PROPERTY(QString currentNote READ currentNote NOTIFY currentChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY undoChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QStringList favouriteNames READ favouriteNames NOTIFY favouritesChanged)

public:
    explicit Registry(QObject *parent = nullptr);

    RegistryModel *tree() { return &m_tree; }
    ValuesModel *values() { return &m_values; }
    QString currentPath() const;
    bool currentWritable() const;
    bool currentSystem() const;
    bool currentDconf() const;
    QString currentNote() const;
    bool canUndo() const { return !m_undo.isEmpty(); }
    bool busy() const { return m_busy; }
    QStringList favouriteNames() const;

    Q_INVOKABLE void select(const QModelIndex &index);
    Q_INVOKABLE QModelIndex indexOfPath(const QString &path);
    // a value of the selected key; kind of a new one: Text Number YesNo List
    Q_INVOKABLE void setValue(const QString &name, const QString &text);
    Q_INVOKABLE void newValue(const QString &kind, const QString &name, const QString &text);
    Q_INVOKABLE void deleteValue(const QString &name);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void refresh();
    // checks the text against the value's type, "" when it fits
    Q_INVOKABLE QString checkValue(const QString &name, const QString &text) const;
    Q_INVOKABLE QString uniqueName(const QString &base) const;

    // the next key or value after the selection that matches; false at the end
    Q_INVOKABLE bool find(const QString &text, bool keys, bool names, bool data, bool wholeWord);

    Q_INVOKABLE bool exportTo(const QUrl &file, bool branch);
    // reads a file and keeps its changes until applyImport(); returns how many
    Q_INVOKABLE int prepareImport(const QUrl &file);
    Q_INVOKABLE void applyImport();
    Q_INVOKABLE int importSkipped() const { return m_importSkipped; }

    Q_INVOKABLE void addFavourite(const QString &name);
    Q_INVOKABLE void removeFavourite(const QString &name);
    Q_INVOKABLE void openFavourite(const QString &name);

Q_SIGNALS:
    void currentChanged();
    void undoChanged();
    void busyChanged();
    void favouritesChanged();
    // QML moves the tree's selection there, and the value list's to valueName
    void revealRequested(const QModelIndex &index, const QString &valueName);
    void failed(const QString &message);
    void done(const QString &message);

private:
    struct Edit {
        RegNode *node = nullptr;
        QString name;
        bool remove = false;
        QString text; // file text, or GVariant text for dconf
        QString dconfType;
        bool undoing = false;
    };
    struct UndoItem {
        QString path; // of the key
        QString name;
        bool remove = false;
        QString text;
        QString dconfType;
    };

    void run(const Edit &edit, std::function<void(bool)> then = {});
    void runQueue();
    void finishEdit(const Edit &edit, bool ok, bool hadValue, const QString &oldText, const QString &oldType,
                    const QString &error);
    void showValues();
    QString backupRoot() const;
    void loadFavourites();
    void saveFavourites();

    RegistryModel m_tree;
    ValuesModel m_values;
    Reg::DconfStore m_dconf;
    RegNode *m_current = nullptr;
    QList<UndoItem> m_undo;
    QList<Edit> m_queue;
    QList<std::function<void(bool)>> m_thens;
    bool m_busy = false;
    QProcess *m_helper = nullptr;
    QList<Edit> m_import;
    int m_importSkipped = 0;
    QStringList m_favNames;
    QStringList m_favPaths;
    // where find() stops: the node and value it last matched
    QString m_foundPath;
    QString m_foundName;
};

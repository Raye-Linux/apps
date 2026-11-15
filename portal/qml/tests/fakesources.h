// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: LGPL-2.1-or-later

// Screens and windows for the share picker without KWin or PipeWire:
// the same roles, the same selection and the same synthetic rows as the
// backend's models (src/outputsmodel.cpp and the window proxy in
// src/screenchooserdialog.cpp).

#pragma once

#include <QAbstractListModel>
#include <QRect>
#include <QSet>
#include <QSize>
#include <QStringList>

class FakeOutputs : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY hasSelectionChanged)
    Q_PROPERTY(qsizetype syntheticCount READ syntheticCount CONSTANT)
public:
    enum Roles {
        ScreenRole = Qt::UserRole,
        NameRole,
        IsSyntheticRole,
        DescriptionRole,
        GeometryRole,
    };

    // sizes: the screens, side by side; sourceTypes as in the portal
    FakeOutputs(const QList<QSize> &sizes, uint sourceTypes, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    bool hasSelection() const
    {
        return !m_selected.isEmpty();
    }
    qsizetype syntheticCount() const;
    QList<int> selectedRows() const;

public Q_SLOTS:
    void clearSelection();

Q_SIGNALS:
    void hasSelectionChanged();

private:
    struct Row {
        QString display;
        QString icon;
        QString name;
        QRect geometry;
        bool synthetic = false;
    };
    QList<Row> m_rows;
    QSet<int> m_selected;
};

class FakeWindows : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(bool hasSelection READ hasSelection NOTIFY hasSelectionChanged)
public:
    // the roles of KWayland's PlasmaWindowModel the picker reads
    enum Roles {
        AppId = Qt::UserRole + 1,
        Uuid = Qt::UserRole + 24,
    };

    explicit FakeWindows(int count, QObject *parent = nullptr);
    // titles of your own (one window each)
    FakeWindows(const QStringList &titles, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    bool hasSelection() const
    {
        return !m_selected.isEmpty();
    }
    QList<int> selectedRows() const;

public Q_SLOTS:
    void clearSelection();

Q_SIGNALS:
    void hasSelectionChanged();

private:
    struct Row {
        QString title;
        QString icon;
        QString appId;
    };
    QList<Row> m_rows;
    QSet<int> m_selected;
};

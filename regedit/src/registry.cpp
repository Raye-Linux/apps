// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "registry.h"

#include "helperargs.h"
#include "locations.h"
#include "regfile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>

#include <KLocalizedString>

using Reg::Format;

struct RegNode {
    enum Kind { Computer, Hive, Store, Folder, File, Group, Dconf };
    enum StoreKind { None, UserFiles, Defaults, System, Information, Desktop };

    Kind kind = Computer;
    StoreKind store = None;
    QString name;
    RegNode *parent = nullptr;
    std::vector<std::unique_ptr<RegNode>> children;
    bool loaded = false;
    QString path; // file or folder; for Dconf the dconf folder ("/org/gnome/")
    QStringList group;
    Format format = Format::KConfig;
    bool system = false; // written through the helper
    bool readOnly = false;
    QString etcPath; // a system file's real path under /etc, whatever the root
    QString package; // the package that ships it as a conffile

    RegNode *add(Kind k, const QString &n)
    {
        auto child = std::make_unique<RegNode>();
        child->kind = k;
        child->name = n;
        child->parent = this;
        child->store = store;
        child->system = system;
        child->readOnly = readOnly;
        children.push_back(std::move(child));
        return children.back().get();
    }

    RegNode *child(const QString &n) const
    {
        for (const auto &c : children) {
            if (c->name == n)
                return c.get();
        }
        return nullptr;
    }

    int row() const
    {
        if (!parent)
            return 0;
        for (size_t i = 0; i < parent->children.size(); ++i) {
            if (parent->children[i].get() == this)
                return int(i);
        }
        return 0;
    }

    const RegNode *fileNode() const
    {
        const RegNode *n = this;
        while (n && n->kind == Group)
            n = n->parent;
        return n && n->kind == File ? n : nullptr;
    }
};

namespace
{

std::optional<Reg::SettingsFile> readFile(const RegNode *file)
{
    QFile f(file->path);
    if (f.size() > Reg::MaxFileSize || !f.open(QIODevice::ReadOnly))
        return std::nullopt;
    return Reg::SettingsFile::parse(f.readAll(), file->format);
}

QString typeName(Reg::ValueType t)
{
    switch (t) {
    case Reg::ValueType::Number: return QStringLiteral("Number");
    case Reg::ValueType::YesNo: return QStringLiteral("YesNo");
    case Reg::ValueType::List: return QStringLiteral("List");
    default: return QStringLiteral("Text");
    }
}

// a path as files are named in the tree: "gtk-3.0/settings.ini" -> folders then a file
void addFiles(RegNode *under, const QString &base, const QStringList &relative)
{
    for (const QString &rel : relative) {
        const QStringList parts = rel.split(QLatin1Char('/'));
        RegNode *at = under;
        for (int i = 0; i + 1 < parts.size(); ++i) {
            RegNode *next = at->child(parts.at(i));
            if (!next) {
                next = at->add(RegNode::Folder, parts.at(i));
                next->path = base + QLatin1Char('/') + parts.mid(0, i + 1).join(QLatin1Char('/'));
                next->loaded = true;
            }
            at = next;
        }
        RegNode *file = at->add(RegNode::File, parts.constLast());
        file->path = base + QLatin1Char('/') + rel;
        file->format = Reg::formatFor(file->path);
    }
}

}

// ---- RegistryModel ---------------------------------------------------

RegistryModel::RegistryModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_configDir(QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation))
{
    build();
}

RegistryModel::~RegistryModel() = default;

void RegistryModel::setPlaces(const QString &configDir, const QString &systemRoot)
{
    beginResetModel();
    m_configDir = configDir;
    m_systemRoot = systemRoot;
    build();
    endResetModel();
}

void RegistryModel::build()
{
    m_root = std::make_unique<RegNode>();
    m_root->name = i18nc("@item the top of the registry tree", "Computer");
    m_root->loaded = true;

    RegNode *user = m_root->add(RegNode::Hive, QStringLiteral("HKEY_CURRENT_USER"));
    user->loaded = true;
    RegNode *config = user->add(RegNode::Store, QStringLiteral("Config"));
    config->store = RegNode::UserFiles;
    config->path = m_configDir;
    RegNode *desktop = user->add(RegNode::Store, QStringLiteral("Desktop"));
    desktop->store = RegNode::Desktop;
    desktop->kind = RegNode::Dconf;
    desktop->path = QStringLiteral("/");

    RegNode *machine = m_root->add(RegNode::Hive, QStringLiteral("HKEY_LOCAL_MACHINE"));
    machine->loaded = true;
    machine->system = true;
    RegNode *defaults = machine->add(RegNode::Store, QStringLiteral("Defaults"));
    defaults->store = RegNode::Defaults;
    defaults->path = QDir::cleanPath(m_systemRoot + QStringLiteral("/etc/xdg"));
    RegNode *system = machine->add(RegNode::Store, QStringLiteral("System"));
    system->store = RegNode::System;
    RegNode *info = machine->add(RegNode::Store, QStringLiteral("Information"));
    info->store = RegNode::Information;
    info->readOnly = true;
}

void RegistryModel::load(RegNode *node) const
{
    if (node->loaded)
        return;
    node->loaded = true;
    switch (node->kind) {
    case RegNode::Store: {
        // a file a package ships is read only from the start, even when it
        // is reached by the address bar before it was ever opened
        const auto markPackageFiles = [this](RegNode *store) {
            const QString root = QDir::cleanPath(m_systemRoot.isEmpty() ? QStringLiteral("/") : m_systemRoot);
            std::function<void(RegNode *)> mark = [&](RegNode *n) {
                if (n->kind == RegNode::File) {
                    n->etcPath = root == QLatin1String("/") ? n->path : n->path.mid(root.size());
                    n->package = Reg::packageOf(n->etcPath, m_systemRoot);
                    n->readOnly = n->readOnly || !n->package.isEmpty();
                }
                for (auto &c : n->children)
                    mark(c.get());
            };
            mark(store);
        };
        if (node->store == RegNode::UserFiles) {
            addFiles(node, node->path, Reg::userFiles(node->path, 3));
        } else if (node->store == RegNode::Defaults) {
            QStringList files;
            const auto all = Reg::userFiles(node->path, 2);
            for (const QString &rel : all) {
                if (Reg::systemWritable(QStringLiteral("/etc/xdg/") + rel))
                    files.append(rel);
            }
            addFiles(node, node->path, files);
            markPackageFiles(node);
        } else {
            const QString etc = QDir::cleanPath(m_systemRoot + QStringLiteral("/etc"));
            const QStringList paths = node->store == RegNode::System ? Reg::systemFiles(m_systemRoot)
                                                                     : Reg::informationFiles(m_systemRoot);
            QStringList relative;
            for (const QString &p : paths)
                relative.append(p.mid(etc.size() + 1));
            addFiles(node, etc, relative);
            // the format and the list are those of the real /etc path
            std::function<void(RegNode *)> fix = [&](RegNode *n) {
                if (n->kind == RegNode::File)
                    n->format = Reg::formatFor(QStringLiteral("/etc/") + n->path.mid(etc.size() + 1));
                for (auto &c : n->children)
                    fix(c.get());
            };
            fix(node);
            if (node->store == RegNode::System)
                markPackageFiles(node);
        }
        break;
    }
    case RegNode::File: {
        const auto file = readFile(node);
        if (!file)
            break;
        if (file->locked() || node->format == Format::Plain)
            node->readOnly = true;

        const auto groups = file->groups();
        for (const QStringList &g : groups) {
            RegNode *at = node;
            for (int i = 0; i < g.size(); ++i) {
                RegNode *next = at->child(g.at(i));
                if (!next) {
                    next = at->add(RegNode::Group, g.at(i));
                    next->path = node->path;
                    next->format = node->format;
                    next->group = g.mid(0, i + 1);
                    next->loaded = true;
                    next->readOnly = node->readOnly || file->groupLocked(next->group);
                }
                at = next;
            }
        }
        break;
    }
    case RegNode::Dconf: {
        static Reg::DconfStore dconf;
        const auto items = dconf.list(node->path);
        for (const QString &item : items) {
            if (!item.endsWith(QLatin1Char('/')))
                continue;
            RegNode *dir = node->add(RegNode::Dconf, item.chopped(1));
            dir->path = node->path + item;
        }
        break;
    }
    default:
        break;
    }
}

void RegistryModel::reloadAll()
{
    beginResetModel();
    build();
    endResetModel();
}

void RegistryModel::reload(RegNode *node)
{
    const QModelIndex idx = indexOf(node);
    if (!node->children.empty()) {
        beginRemoveRows(idx, 0, int(node->children.size()) - 1);
        node->children.clear();
        endRemoveRows();
    }
    node->loaded = false;
    // load() adds the children in silence; tell the view afterwards
    load(node);
    if (!node->children.empty()) {
        std::vector<std::unique_ptr<RegNode>> kids;
        kids.swap(node->children);
        beginInsertRows(idx, 0, int(kids.size()) - 1);
        node->children.swap(kids);
        endInsertRows();
    }
}

RegNode *RegistryModel::node(const QModelIndex &index) const
{
    return index.isValid() ? static_cast<RegNode *>(index.internalPointer()) : nullptr;
}

QModelIndex RegistryModel::indexOf(RegNode *n) const
{
    if (!n || !n->parent)
        return n ? createIndex(0, 0, n) : QModelIndex();
    return createIndex(n->row(), 0, n);
}

QModelIndex RegistryModel::index(int row, int column, const QModelIndex &parent) const
{
    if (column != 0)
        return {};
    if (!parent.isValid())
        return row == 0 ? createIndex(0, 0, m_root.get()) : QModelIndex();
    RegNode *p = node(parent);
    load(p);
    if (row < 0 || row >= int(p->children.size()))
        return {};
    return createIndex(row, 0, p->children[row].get());
}

QModelIndex RegistryModel::parent(const QModelIndex &child) const
{
    RegNode *n = node(child);
    if (!n || !n->parent)
        return {};
    return indexOf(n->parent);
}

int RegistryModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return 1;
    RegNode *p = node(parent);
    load(p);
    return int(p->children.size());
}

int RegistryModel::columnCount(const QModelIndex &) const
{
    return 1;
}

bool RegistryModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return true;
    RegNode *p = node(parent);
    if (!p->loaded)
        return p->kind != RegNode::Group;
    return !p->children.empty();
}

QVariant RegistryModel::data(const QModelIndex &index, int role) const
{
    RegNode *n = node(index);
    if (!n)
        return {};
    switch (role) {
    case Qt::DisplayRole:
    case NameRole:
        return n->name;
    case KindRole:
        switch (n->kind) {
        case RegNode::Computer: return QStringLiteral("computer");
        case RegNode::File: return QStringLiteral("file");
        default: return n->readOnly ? QStringLiteral("lockedfolder") : QStringLiteral("folder");
        }
    case PathRole:
        return pathOf(n);
    }
    return {};
}

QHash<int, QByteArray> RegistryModel::roleNames() const
{
    return {{Qt::DisplayRole, "display"}, {NameRole, "name"}, {KindRole, "kind"}, {PathRole, "path"}};
}

QString RegistryModel::pathOf(const RegNode *n) const
{
    QStringList parts;
    for (; n; n = n->parent)
        parts.prepend(n->name);
    return parts.join(QLatin1Char('\\'));
}

RegNode *RegistryModel::nodeAt(const QString &path) const
{
    QStringList parts = path.split(QLatin1Char('\\'), Qt::SkipEmptyParts);
    if (parts.isEmpty())
        return nullptr;
    // "HKEY_CURRENT_USER\..." without "Computer" works too, as in Windows
    // and the English word in a file exported in another language
    if (parts.constFirst().compare(m_root->name, Qt::CaseInsensitive) == 0
        || parts.constFirst().compare(QLatin1String("Computer"), Qt::CaseInsensitive) == 0)
        parts.removeFirst();
    RegNode *at = m_root.get();
    for (const QString &p : std::as_const(parts)) {
        load(at);
        RegNode *next = at->child(p);
        if (!next) {
            for (const auto &c : at->children) {
                if (c->name.compare(p, Qt::CaseInsensitive) == 0)
                    next = c.get();
            }
        }
        if (!next)
            return nullptr;
        at = next;
    }
    return at;
}

// ---- ValuesModel -----------------------------------------------------

int ValuesModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_rows.size();
}

QVariant ValuesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_rows.size())
        return {};
    const Row &r = m_rows.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case NameRole: return r.name;
    case TypeRole: return r.type;
    case DataRole: return r.data;
    case RawRole: return r.raw;
    case LockedRole: return r.locked;
    case KindRole: return r.kind;
    }
    return {};
}

QHash<int, QByteArray> ValuesModel::roleNames() const
{
    return {{NameRole, "name"}, {TypeRole, "type"}, {DataRole, "valueData"},
            {RawRole, "raw"}, {LockedRole, "locked"}, {KindRole, "kind"}};
}

void ValuesModel::setRows(const QList<Row> &rows)
{
    beginResetModel();
    m_rows = rows;
    endResetModel();
    Q_EMIT countChanged();
}

int ValuesModel::indexOfName(const QString &name) const
{
    for (int i = 0; i < m_rows.size(); ++i) {
        if (m_rows.at(i).name == name)
            return i;
    }
    return -1;
}

QString ValuesModel::rawOf(const QString &name) const
{
    const int i = indexOfName(name);
    return i < 0 ? QString() : m_rows.at(i).raw;
}

bool ValuesModel::lockedOf(const QString &name) const
{
    const int i = indexOfName(name);
    return i < 0 || m_rows.at(i).locked;
}

// ---- Registry --------------------------------------------------------

namespace
{

QList<ValuesModel::Row> rowsOf(const RegNode *n, Reg::DconfStore &dconf)
{
    QList<ValuesModel::Row> rows;
    if (!n)
        return rows;
    if (n->kind == RegNode::File || n->kind == RegNode::Group) {
        const RegNode *file = n->fileNode();
        const auto parsed = readFile(file);
        if (!parsed)
            return rows;
        const auto entries = parsed->entries(n->kind == RegNode::File ? QStringList() : n->group);
        for (const Reg::Entry &e : entries) {
            ValuesModel::Row r;
            r.name = e.key;
            r.raw = e.text;
            r.data = e.text;
            r.kind = typeName(Reg::guessType(e.text, file->format));
            r.type = r.kind;
            r.locked = e.locked || n->readOnly || (file->system && Reg::isCodeKey(file->etcPath, e.key));
            rows.append(r);
        }
    } else if (n->kind == RegNode::Dconf) {
        const auto items = dconf.list(n->path);
        for (const QString &item : items) {
            if (item.endsWith(QLatin1Char('/')))
                continue;
            const auto v = dconf.read(n->path + item);
            if (!v)
                continue;
            ValuesModel::Row r;
            r.name = item;
            r.type = v->type;
            r.raw = v->text;
            r.data = Reg::dconfDisplay(*v);
            r.kind = QStringLiteral("Dconf");
            r.locked = !dconf.writable(n->path + item);
            rows.append(r);
        }
    }
    return rows;
}

}

Registry::Registry(QObject *parent)
    : QObject(parent)
{
    loadFavourites();
}

QString Registry::currentPath() const
{
    return m_current ? m_tree.pathOf(m_current) : m_tree.root()->name;
}

bool Registry::currentWritable() const
{
    if (!m_current || m_current->readOnly)
        return false;
    if (m_current->kind == RegNode::Dconf)
        return true;
    const RegNode *file = m_current->fileNode();
    if (!file)
        return false;
    return file->system ? Reg::systemWritable(file->path) : QFileInfo(file->path).isWritable();
}

QString Registry::currentNote() const
{
    const RegNode *file = m_current ? m_current->fileNode() : nullptr;
    if (!file || file->package.isEmpty())
        return QString();
    return i18n("Read only: this file comes with the %1 package, and an edit here would stop updates from changing it.",
                file->package);
}

bool Registry::currentSystem() const
{
    return m_current && m_current->system;
}

bool Registry::currentDconf() const
{
    return m_current && m_current->kind == RegNode::Dconf;
}

void Registry::select(const QModelIndex &index)
{
    m_current = m_tree.node(index);
    showValues();
    Q_EMIT currentChanged();
}

QModelIndex Registry::indexOfPath(const QString &path)
{
    return m_tree.indexOf(m_tree.nodeAt(path));
}

void Registry::showValues()
{
    m_values.setRows(rowsOf(m_current, m_dconf));
}

void Registry::refresh()
{
    if (m_busy)
        return;
    const QString path = currentPath();
    m_tree.reloadAll();
    m_current = m_tree.nodeAt(path);
    showValues();
    Q_EMIT currentChanged();
    if (m_current)
        Q_EMIT revealRequested(m_tree.indexOf(m_current), QString());
}

QString Registry::backupRoot() const
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericStateLocation) + QStringLiteral("/raye-regedit/backups");
}

QString Registry::checkValue(const QString &name, const QString &text) const
{
    const int row = m_values.indexOfName(name);
    if (row < 0)
        return QString();
    const auto &r = m_values.rows().at(row);
    if (r.kind == QLatin1String("Dconf")) {
        QString error;
        QString src = text;
        if (r.type == QLatin1String("s"))
            src = Reg::dconfQuote(text);
        return Reg::dconfCanonical(src, r.type, &error) ? QString() : i18n("This value must be of the type %1.", r.type);
    }
    if (r.kind == QLatin1String("Number") && !Reg::fitsType(r.raw, text, Reg::ValueType::Number))
        return i18n("Type a whole number.");
    if (r.kind == QLatin1String("YesNo") && !Reg::fitsType(r.raw, text, Reg::ValueType::YesNo)) {
        const bool yes = r.raw == QLatin1String("yes") || r.raw == QLatin1String("no");
        return yes ? i18n("Type yes or no.") : i18n("Type true or false.");
    }
    const RegNode *file = m_current ? m_current->fileNode() : nullptr;
    if (file && file->format != Format::KConfig && text.contains(QLatin1Char('\n')))
        return i18n("This value must stay on one line.");
    return QString();
}

QString Registry::uniqueName(const QString &base) const
{
    for (int n = 1;; ++n) {
        const QString name = base + QLatin1Char(' ') + QString::number(n);
        if (m_values.indexOfName(name) < 0)
            return name;
    }
}

void Registry::setValue(const QString &name, const QString &text)
{
    if (!currentWritable())
        return;
    Edit e;
    e.node = m_current;
    e.name = name;
    const int row = m_values.indexOfName(name);
    if (m_current->kind == RegNode::Dconf) {
        if (row < 0)
            return;
        e.dconfType = m_values.rows().at(row).type;
        e.text = e.dconfType == QLatin1String("s") ? Reg::dconfQuote(text) : text;
    } else {
        e.text = text;
    }
    run(e);
}

void Registry::newValue(const QString &kind, const QString &name, const QString &text)
{
    if (!currentWritable() || name.isEmpty())
        return;
    if (m_values.indexOfName(name) >= 0) {
        Q_EMIT failed(i18n("There is already a value named %1.", name));
        return;
    }
    Edit e;
    e.node = m_current;
    e.name = name;
    if (m_current->kind == RegNode::Dconf) {
        if (!m_current->path.startsWith(QLatin1Char('/')) || m_current->path == QLatin1String("/")) {
            Q_EMIT failed(i18n("Open a folder first, then add the value there."));
            return;
        }
        QString error;
        const auto v = Reg::dconfNewValue(text, kind, &error);
        if (!v) {
            Q_EMIT failed(i18n("This is not a valid value: %1", error));
            return;
        }
        e.text = v->text;
        e.dconfType = v->type;
    } else {
        e.text = text;
    }
    run(e);
}

void Registry::deleteValue(const QString &name)
{
    if (!currentWritable() || m_values.indexOfName(name) < 0)
        return;
    Edit e;
    e.node = m_current;
    e.name = name;
    e.remove = true;
    run(e);
}

void Registry::undo()
{
    if (m_undo.isEmpty() || m_busy)
        return;
    const UndoItem u = m_undo.takeLast();
    Q_EMIT undoChanged();
    RegNode *n = m_tree.nodeAt(u.path);
    if (!n) {
        Q_EMIT failed(i18n("The key %1 is no longer there.", u.path));
        return;
    }
    Edit e;
    e.node = n;
    e.name = u.name;
    e.remove = u.remove;
    e.text = u.text;
    e.dconfType = u.dconfType;
    e.undoing = true;
    run(e, [this, n, u](bool ok) {
        if (ok) {
            Q_EMIT revealRequested(m_tree.indexOf(n), u.name);
            return;
        }
        // a canceled password or a failed write: it can still be taken back later
        m_undo.append(u);
        Q_EMIT undoChanged();
    });
}

void Registry::run(const Edit &edit, std::function<void(bool)> then)
{
    m_queue.append(edit);
    m_thens.append(std::move(then));
    if (!m_busy)
        runQueue();
}

void Registry::runQueue()
{
    while (!m_queue.isEmpty()) {
        const Edit e = m_queue.takeFirst();
        const auto then = m_thens.takeFirst();
        RegNode *n = e.node;

        if (n->kind == RegNode::Dconf) {
            const QString key = n->path + e.name;
            const auto old = m_dconf.read(key);
            QString error;
            const bool ok = e.remove ? m_dconf.reset(key, &error) : m_dconf.write(key, {e.dconfType, e.text}, &error);
            finishEdit(e, ok, old.has_value(), old ? old->text : QString(), old ? old->type : QString(), error);
            if (then)
                then(ok);
            continue;
        }

        const RegNode *file = n->fileNode();
        Reg::Change c;
        c.op = e.remove ? Reg::Change::Remove : Reg::Change::Set;
        c.path = file->path;
        c.format = file->format;
        c.group = n->kind == RegNode::File ? QStringList() : n->group;
        c.key = e.name;
        c.text = e.text;

        if (!file->system) {
            const Reg::Outcome o = Reg::applyChange(c, backupRoot());
            // the reason is in English and for the log; the window says it plainly
            if (!o.ok)
                qWarning("raye-regedit: %s", qPrintable(o.error));
            finishEdit(e, o.ok, o.hadValue, o.oldText, QString(),
                       o.ok ? QString() : i18n("%1 could not be changed.", QFileInfo(c.path).fileName()));
            if (then)
                then(o.ok);
            continue;
        }

        // the root part: one pkexec per change, and the password asked for each one
        m_busy = true;
        Q_EMIT busyChanged();
        m_helper = new QProcess(this);
        m_helper->setProgram(QStringLiteral("pkexec"));
        m_helper->setArguments(QStringList{QStringLiteral(RAYE_REGEDIT_HELPER)} + Reg::helperArguments(c));
        connect(m_helper, &QProcess::finished, this, [this, e, then](int code, QProcess::ExitStatus status) {
            const QByteArray out = m_helper->readAllStandardOutput();
            const QString err = QString::fromUtf8(m_helper->readAllStandardError()).trimmed();
            m_helper->deleteLater();
            m_helper = nullptr;
            const bool ok = status == QProcess::NormalExit && code == 0;
            const bool had = out.startsWith("had 1\n");
            QString error;
            if (code == 126 || code == 127)
                error = i18n("The change was not allowed.");
            else if (code == Reg::HelperCodeKey)
                error = i18n("%1 can't be changed in Registry Editor: a program running as root would load what it names.", e.name);
            else if (code == Reg::HelperPackageFile)
                error = i18n("This file comes with a package and is read only here.");
            else if (code == Reg::HelperRefused)
                error = i18n("This file can't be changed in Registry Editor.");
            if (!ok)
                qWarning("raye-regedit-helper: %s", qPrintable(err));
            finishEdit(e, ok, had, QString::fromUtf8(out.mid(6)), QString(), error);
            m_busy = false;
            Q_EMIT busyChanged();
            if (then)
                then(ok);
            runQueue();
        });
        connect(m_helper, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart && m_helper)
                Q_EMIT m_helper->finished(127, QProcess::NormalExit);
        });
        m_helper->start();
        m_helper->write(e.text.toUtf8());
        m_helper->closeWriteChannel();
        return;
    }
}

void Registry::finishEdit(const Edit &edit, bool ok, bool hadValue, const QString &oldText, const QString &oldType,
                          const QString &error)
{
    if (!ok) {
        Q_EMIT failed(error.isEmpty() ? i18n("The value could not be changed.") : error);
        return;
    }
    if (!edit.undoing) {
        UndoItem u;
        u.path = m_tree.pathOf(edit.node);
        u.name = edit.name;
        u.remove = !hadValue;
        u.text = oldText;
        u.dconfType = oldType;
        m_undo.append(u);
        while (m_undo.size() > 100)
            m_undo.removeFirst();
        Q_EMIT undoChanged();
    }
    if (edit.node == m_current)
        showValues();
}

bool Registry::find(const QString &text, bool keys, bool names, bool data, bool wholeWord)
{
    if (text.isEmpty())
        return false;
    auto matches = [&](const QString &s) {
        return wholeWord ? s.compare(text, Qt::CaseInsensitive) == 0 : s.contains(text, Qt::CaseInsensitive);
    };
    // the search starts after what was found last, or at the selected key
    const QString fromPath = currentPath();
    const QString fromName = m_foundPath == fromPath ? m_foundName : QString();
    bool started = false;
    bool skippedStart = false;
    RegNode *hitNode = nullptr;
    QString hitName;

    std::function<bool(RegNode *)> walk = [&](RegNode *n) -> bool {
        const bool here = m_tree.pathOf(n) == fromPath;
        if (started && keys && matches(n->name)) {
            hitNode = n;
            return true;
        }
        if (here && !started) {
            started = true;
            skippedStart = fromName.isEmpty();
        }
        if (started && (names || data)) {
            const auto rows = rowsOf(n, m_dconf);
            for (const auto &r : rows) {
                if (here && !skippedStart) {
                    if (r.name == fromName)
                        skippedStart = true;
                    continue;
                }
                if ((names && matches(r.name)) || (data && matches(r.data))) {
                    hitNode = n;
                    hitName = r.name;
                    return true;
                }
            }
        }
        m_tree.load(n);
        for (size_t i = 0; i < n->children.size(); ++i) {
            if (walk(n->children[i].get()))
                return true;
        }
        return false;
    };
    walk(m_tree.root());
    if (!hitNode) {
        m_foundPath.clear();
        m_foundName.clear();
        return false;
    }
    m_foundPath = m_tree.pathOf(hitNode);
    m_foundName = hitName;
    m_current = hitNode;
    showValues();
    Q_EMIT currentChanged();
    Q_EMIT revealRequested(m_tree.indexOf(hitNode), hitName);
    return true;
}

bool Registry::exportTo(const QUrl &url, bool branch)
{
    QList<Reg::RegSection> sections;
    std::function<void(RegNode *)> walk = [&](RegNode *n) {
        const auto rows = rowsOf(n, m_dconf);
        if (!rows.isEmpty()) {
            Reg::RegSection s;
            // from the hive down: the word for Computer depends on the language
            s.path = m_tree.pathOf(n).section(QLatin1Char('\\'), 1);
            for (const auto &r : rows)
                s.items.append(Reg::RegItem{r.name, false, r.kind == QLatin1String("Dconf"), r.raw});
            sections.append(s);
        }
        if (!branch)
            return;
        m_tree.load(n);
        for (size_t i = 0; i < n->children.size(); ++i)
            walk(n->children[i].get());
    };
    walk(m_current ? m_current : m_tree.root());
    QFile out(url.toLocalFile());
    if (!out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        Q_EMIT failed(i18n("The file could not be saved: %1", out.errorString()));
        return false;
    }
    out.write(Reg::writeRegFile(sections).toUtf8());
    Q_EMIT done(i18np("Exported one key.", "Exported %1 keys.", sections.size()));
    return true;
}

int Registry::prepareImport(const QUrl &url)
{
    m_import.clear();
    m_importSkipped = 0;
    QFile in(url.toLocalFile());
    if (!in.open(QIODevice::ReadOnly) || in.size() > 16 * 1024 * 1024) {
        Q_EMIT failed(i18n("The file could not be opened."));
        return -1;
    }
    QString error;
    int line = 0;
    const auto sections = Reg::readRegFile(QString::fromUtf8(in.readAll()), &error, &line);
    if (!sections) {
        Q_EMIT failed(i18n("This is not a file the Registry Editor made (line %1).", line));
        return -1;
    }
    for (const auto &s : *sections) {
        RegNode *n = m_tree.nodeAt(s.path);
        RegNode *was = m_current;
        m_current = n;
        const bool ok = n && currentWritable();
        const auto rows = n ? rowsOf(n, m_dconf) : QList<ValuesModel::Row>();
        m_current = was;
        if (!ok) {
            m_importSkipped += s.items.size();
            continue;
        }
        for (const auto &item : s.items) {
            Edit e;
            e.node = n;
            e.name = item.name;
            e.remove = item.remove;
            if (n->kind == RegNode::Dconf) {
                if (!item.remove) {
                    if (!item.gvariant) {
                        ++m_importSkipped;
                        continue;
                    }
                    QString type;
                    for (const auto &r : rows) {
                        if (r.name == item.name)
                            type = r.type;
                    }
                    if (type.isEmpty())
                        type = Reg::dconfGuessType(item.text).value_or(QString());
                    if (type.isEmpty() || !Reg::dconfCanonical(item.text, type)) {
                        ++m_importSkipped;
                        continue;
                    }
                    e.dconfType = type;
                    e.text = item.text;
                }
            } else {
                if (item.gvariant) {
                    ++m_importSkipped;
                    continue;
                }
                e.text = item.text;
            }
            m_import.append(e);
        }
    }
    return m_import.size();
}

void Registry::applyImport()
{
    const QList<Edit> edits = m_import;
    m_import.clear();
    const int total = edits.size();
    auto failures = std::make_shared<int>(0);
    auto left = std::make_shared<int>(total);
    for (const Edit &e : edits) {
        run(e, [this, failures, left, total](bool ok) {
            if (!ok)
                ++*failures;
            if (--*left == 0)
                Q_EMIT done(i18np("Imported one value.", "Imported %1 values.", total - *failures));
        });
    }
}

void Registry::loadFavourites()
{
    QSettings s(QStringLiteral("raye"), QStringLiteral("raye-regedit"));
    m_favNames = s.value(QStringLiteral("Favourites/names")).toStringList();
    m_favPaths = s.value(QStringLiteral("Favourites/paths")).toStringList();
    while (m_favPaths.size() < m_favNames.size())
        m_favNames.removeLast();
}

void Registry::saveFavourites()
{
    QSettings s(QStringLiteral("raye"), QStringLiteral("raye-regedit"));
    s.setValue(QStringLiteral("Favourites/names"), m_favNames);
    s.setValue(QStringLiteral("Favourites/paths"), m_favPaths);
    Q_EMIT favouritesChanged();
}

QStringList Registry::favouriteNames() const
{
    return m_favNames;
}

void Registry::addFavourite(const QString &name)
{
    if (name.trimmed().isEmpty())
        return;
    const int at = m_favNames.indexOf(name);
    if (at >= 0) {
        m_favPaths[at] = currentPath();
    } else {
        m_favNames.append(name);
        m_favPaths.append(currentPath());
    }
    saveFavourites();
}

void Registry::removeFavourite(const QString &name)
{
    const int at = m_favNames.indexOf(name);
    if (at < 0)
        return;
    m_favNames.removeAt(at);
    m_favPaths.removeAt(at);
    saveFavourites();
}

void Registry::openFavourite(const QString &name)
{
    const int at = m_favNames.indexOf(name);
    if (at < 0)
        return;
    RegNode *n = m_tree.nodeAt(m_favPaths.at(at));
    if (!n) {
        Q_EMIT failed(i18n("The key %1 is no longer there.", m_favPaths.at(at)));
        return;
    }
    m_current = n;
    showValues();
    Q_EMIT currentChanged();
    Q_EMIT revealRequested(m_tree.indexOf(n), QString());
}

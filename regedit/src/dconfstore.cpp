// SPDX-FileCopyrightText: 2026 Raye Linux
// SPDX-License-Identifier: GPL-3.0-or-later

#include "dconfstore.h"

#include <QRegularExpression>

#include <dconf.h>

namespace Reg
{

namespace
{

struct Variant {
    GVariant *v = nullptr;
    explicit Variant(GVariant *p) : v(p) {}
    ~Variant() { if (v) g_variant_unref(v); }
    Variant(const Variant &) = delete;
    Variant &operator=(const Variant &) = delete;
};

QString printed(GVariant *v)
{
    gchar *s = g_variant_print(v, FALSE);
    const QString out = QString::fromUtf8(s);
    g_free(s);
    return out;
}

}

std::optional<QString> dconfCanonical(const QString &text, const QString &type, QString *error)
{
    const QByteArray t = type.toUtf8();
    if (!g_variant_type_string_is_valid(t.constData())) {
        if (error)
            *error = QStringLiteral("bad type");
        return std::nullopt;
    }
    GVariantType *vt = g_variant_type_new(t.constData());
    GError *err = nullptr;
    const QByteArray src = text.toUtf8();
    GVariant *v = g_variant_parse(vt, src.constData(), nullptr, nullptr, &err);
    g_variant_type_free(vt);
    if (!v) {
        if (error)
            *error = QString::fromUtf8(err ? err->message : "parse error");
        if (err)
            g_error_free(err);
        return std::nullopt;
    }
    Variant hold(v); // parse hands back a full reference
    return printed(hold.v);
}

std::optional<QString> dconfGuessType(const QString &text)
{
    const QByteArray src = text.toUtf8();
    GVariant *v = g_variant_parse(nullptr, src.constData(), nullptr, nullptr, nullptr);
    if (!v)
        return std::nullopt;
    Variant hold(v);
    return QString::fromUtf8(g_variant_get_type_string(hold.v));
}

QString dconfQuote(const QString &text)
{
    const QByteArray s = text.toUtf8();
    Variant hold(g_variant_ref_sink(g_variant_new_string(s.constData())));
    return printed(hold.v);
}

QString dconfDisplay(const DconfValue &value)
{
    if (value.type != QLatin1String("s"))
        return value.text;
    const QByteArray src = value.text.toUtf8();
    GVariant *v = g_variant_parse(G_VARIANT_TYPE_STRING, src.constData(), nullptr, nullptr, nullptr);
    if (!v)
        return value.text;
    Variant hold(v);
    return QString::fromUtf8(g_variant_get_string(hold.v, nullptr));
}

std::optional<DconfValue> dconfNewValue(const QString &text, const QString &kind, QString *error)
{
    QString type;
    QString src = text;
    if (kind == QLatin1String("Text")) {
        type = QStringLiteral("s");
        src = dconfQuote(text);
    } else if (kind == QLatin1String("Number")) {
        static const QRegularExpression integer(QStringLiteral("^-?[0-9]+$"));
        type = integer.match(text.trimmed()).hasMatch() ? QStringLiteral("i") : QStringLiteral("d");
    } else if (kind == QLatin1String("YesNo")) {
        type = QStringLiteral("b");
    } else if (kind == QLatin1String("List")) {
        type = QStringLiteral("as");
    } else {
        if (error)
            *error = QStringLiteral("bad kind");
        return std::nullopt;
    }
    auto canonical = dconfCanonical(src, type, error);
    if (!canonical)
        return std::nullopt;
    return DconfValue{type, *canonical};
}

DconfStore::DconfStore()
    : m_client(dconf_client_new())
{
}

DconfStore::~DconfStore()
{
    if (m_client)
        g_object_unref(m_client);
}

QStringList DconfStore::list(const QString &dir) const
{
    QStringList out;
    const QByteArray d = dir.toUtf8();
    gint n = 0;
    gchar **items = dconf_client_list(m_client, d.constData(), &n);
    for (gint i = 0; i < n; ++i)
        out.append(QString::fromUtf8(items[i]));
    g_strfreev(items);
    out.sort(Qt::CaseInsensitive);
    return out;
}

std::optional<DconfValue> DconfStore::read(const QString &key) const
{
    const QByteArray k = key.toUtf8();
    GVariant *v = dconf_client_read(m_client, k.constData());
    if (!v)
        return std::nullopt;
    Variant hold(v);
    return DconfValue{QString::fromUtf8(g_variant_get_type_string(v)), printed(v)};
}

bool DconfStore::writable(const QString &key) const
{
    const QByteArray k = key.toUtf8();
    return dconf_client_is_writable(m_client, k.constData());
}

bool DconfStore::write(const QString &key, const DconfValue &value, QString *error)
{
    if (!dconf_is_key(key.toUtf8().constData(), nullptr)) {
        if (error)
            *error = QStringLiteral("bad key");
        return false;
    }
    const auto canonical = dconfCanonical(value.text, value.type, error);
    if (!canonical)
        return false;
    const QByteArray t = value.type.toUtf8();
    const QByteArray src = canonical->toUtf8();
    GVariantType *vt = g_variant_type_new(t.constData());
    Variant hold(g_variant_parse(vt, src.constData(), nullptr, nullptr, nullptr));
    g_variant_type_free(vt);
    GError *err = nullptr;
    const QByteArray k = key.toUtf8();
    const bool ok = dconf_client_write_sync(m_client, k.constData(), hold.v, nullptr, nullptr, &err);
    if (!ok) {
        if (error)
            *error = QString::fromUtf8(err ? err->message : "write failed");
        if (err)
            g_error_free(err);
    }
    return ok;
}

bool DconfStore::reset(const QString &key, QString *error)
{
    GError *err = nullptr;
    const QByteArray k = key.toUtf8();
    const bool ok = dconf_client_write_sync(m_client, k.constData(), nullptr, nullptr, nullptr, &err);
    if (!ok) {
        if (error)
            *error = QString::fromUtf8(err ? err->message : "reset failed");
        if (err)
            g_error_free(err);
    }
    return ok;
}

}

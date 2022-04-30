// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QByteArray>
#include <QSettings>
#include <QString>
#include <QTextCodec>

template <typename T>
class Setting;

class Settings
{
    template <typename T>
    friend class Setting;  // for GetQSettings

public:
    static QString GetDataPath();

    static QTextCodec* GetLoadCodec(const QByteArray& data);
    static QTextCodec* GetSaveCodec();

    static Setting<qreal> timing_text_font_size;
    static Setting<qreal> raw_font_size;

    static Setting<int> audio_latency;
    static Setting<int> video_latency;

    static Setting<qreal>* const REAL_SETTINGS[2];
    static Setting<int>* const INT_SETTINGS[2];

private:
    static QSettings* GetQSettings()
    {
        static auto settings = std::make_unique<QSettings>(QSettings::Format::IniFormat,
                QSettings::Scope::UserScope, "MoonCat Karaoke", "Hibikase");

        return settings.get();
    }
};

template <typename T>
class Setting
{
public:
    Setting(QString name, QString friendly_name, T default_value)
        : name(name), friendly_name(friendly_name), default_value(default_value)
    {
        m_cached_value = Settings::GetQSettings()->value(name, default_value).template value<T>();
    }

    Setting(const Setting&) = delete;
    Setting& operator=(const Setting&) = delete;

    T Get() const
    {
        return m_cached_value;
    }

    void Set(T value)
    {
        Settings::GetQSettings()->setValue(name, value);

        m_cached_value = value;

        if (m_callback)
            m_callback(value);
    }

    void Reset()
    {
        Set(default_value);
    }

    void SetCallback(std::function<void(T)> callback)
    {
        m_callback = callback;
    }

    const QString name;
    const QString friendly_name;
    const T default_value;

private:
    std::function<void(T)> m_callback;
    T m_cached_value;
};

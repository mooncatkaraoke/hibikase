// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>

#include <QBrush>
#include <QColor>
#include <QObject>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QPen>
#include <QPoint>
#include <QRect>
#include <QTextCursor>
#include <QTextDocument>
#include <QWidget>

#include "KaraokeData/Song.h"

#include "LineTimingDecorations.h"

static constexpr int VERTICAL_OFFSET = -2;
static constexpr int SYLLABLE_MARKER_WIDTH = 5;
static constexpr int SYLLABLE_MARKER_HEIGHT = 5;
static constexpr int PROGRESS_LINE_HEIGHT = 1;

using Milliseconds = std::chrono::milliseconds;

static TimingState GetTimingState(Milliseconds current, Milliseconds start, Milliseconds end)
{
    if (start > current)
        return TimingState::NotPlayed;
    else if (end > current)
        return TimingState::Playing;
    else
        return TimingState::Played;
}

static QColor GetPlayingColor()
{
    return QPalette().color(QPalette::Text);
}

static QColor GetNotPlayedColor()
{
    QColor col(0x77, 0x55, 0x77);
    // Adjust our colours if the system has a light text colour
    // (The assumption is the system is using a light-on-dark grayscale scheme)
    int diff = QPalette().color(QPalette::Text).lightness() - col.lightness();
    if (diff > 0)
        col = col.lighter(diff);
    return col;
}

static QColor GetPlayedColor()
{
    QColor col(0x55, 0x77, 0x55);
    int diff = QPalette().color(QPalette::Text).lightness() - col.lightness();
    if (diff > 0)
        col = col.lighter(diff);
    return col;
}

static void SetColor(QTextDocument* document, int start_index, int end_index, QColor color)
{
    QTextCursor cursor(document);
    cursor.setPosition(start_index, QTextCursor::MoveAnchor);
    cursor.setPosition(end_index, QTextCursor::KeepAnchor);

    QTextCharFormat format;
    format.setForeground(color);
    cursor.setCharFormat(format);
}

static void SetColor(QTextDocument* document, int start_index, int end_index, TimingState state)
{
    QColor color;
    if (state == TimingState::NotPlayed)
        color = GetNotPlayedColor();
    else if (state == TimingState::Playing)
        color = GetPlayingColor();
    else
        color = GetPlayedColor();

    return SetColor(document, start_index, end_index, color);
}

SyllableDecorations::SyllableDecorations(const QPlainTextEdit* text_edit, int start_index,
                                         int end_index, Milliseconds start_time,
                                         Milliseconds end_time, bool show_end_marker,
                                         TimingState state)
    : QWidget(text_edit->viewport()), m_text_edit(text_edit), m_start_index(start_index),
      m_end_index(end_index), m_start_time(start_time), m_end_time(end_time),
      m_show_end_marker(show_end_marker), m_state(state)
{
    Relayout();

    setPalette(Qt::transparent);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setVisible(true);
}

void SyllableDecorations::Update(Milliseconds time, bool line_is_active)
{
    const TimingState state = GetTimingState(time, m_start_time, m_end_time);
    if (!line_is_active && !m_line_is_active && state == m_state)
        return;
    m_line_is_active = line_is_active;

    if (line_is_active && m_start_time <= time)
    {
        m_progress = std::min<qreal>(1.0, static_cast<qreal>((time - m_start_time).count()) /
                                          (m_end_time - m_start_time).count());
    }
    else
    {
        m_progress = 0;
    }

    update();

    if (state == m_state)
        return;
    m_state = state;
    SetColor(m_text_edit->document(), m_start_index, m_end_index, state);
}

int SyllableDecorations::GetPosition() const
{
    return m_start_index;
}

void SyllableDecorations::AddToPosition(int diff)
{
    m_start_index += diff;
    m_end_index += diff;

    Relayout();
}

static QPainterPath GetStartMarkerPath()
{
    QPainterPath path;

    path.moveTo(QPoint(0, 0));
    path.lineTo(QPoint(SYLLABLE_MARKER_WIDTH, SYLLABLE_MARKER_HEIGHT));
    path.lineTo(QPoint(0, SYLLABLE_MARKER_HEIGHT));
    path.lineTo(QPoint(0, 0));

    return path;
}

static QPainterPath GetEndMarkerPath()
{
    QPainterPath path;

    path.moveTo(QPoint(0, 0));
    path.lineTo(QPoint(-SYLLABLE_MARKER_WIDTH, SYLLABLE_MARKER_HEIGHT));
    path.lineTo(QPoint(0, SYLLABLE_MARKER_HEIGHT));
    path.lineTo(QPoint(0, 0));

    return path;
}

void SyllableDecorations::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::gray));

    if (m_progress != 0)
    {
        painter.drawRect(QRectF(0, SYLLABLE_MARKER_HEIGHT,
                                width() * m_progress, PROGRESS_LINE_HEIGHT));
    }

    painter.fillPath(GetStartMarkerPath(), QBrush(Qt::gray));
    if (m_show_end_marker)
        painter.fillPath(GetEndMarkerPath().translated(width(), 0), QBrush(Qt::gray));
}

void SyllableDecorations::moveEvent(QMoveEvent*)
{
    Relayout();
}

void SyllableDecorations::Relayout()
{
    QTextCursor cursor(m_text_edit->document());
    cursor.setPosition(m_start_index);
    const QRect start_rect = m_text_edit->cursorRect(cursor);
    cursor.setPosition(m_end_index);
    const QRect end_rect = m_text_edit->cursorRect(cursor);

    const int left = std::min(start_rect.left(), end_rect.left());
    const int top = std::max(start_rect.bottom(), end_rect.bottom()) + VERTICAL_OFFSET;
    const int width = std::abs(end_rect.left() - start_rect.left());
    const int height = SYLLABLE_MARKER_HEIGHT + PROGRESS_LINE_HEIGHT;
    setGeometry(QRect(left, top, width, height));
}

LineTimingDecorations::LineTimingDecorations(const KaraokeData::Line& line, int position,
                                             QPlainTextEdit* text_edit, Milliseconds time,
                                             QObject* parent)
    : QObject(parent), m_line(line), m_start_index(position)
{
    m_state = GetTimingState(time, m_line.GetStart(), m_line.GetEnd());


    const QVector<const KaraokeData::Syllable*> syllables = m_line.GetSyllables();
    m_syllables.reserve(syllables.size());
    const int prefix_end_index = m_start_index + m_line.GetPrefix().size();
    int text_index = prefix_end_index;
    for (int i = 0; i < syllables.size(); ++i)
    {
        const KaraokeData::Syllable* syllable = syllables[i];

        const int text_start_index = text_index;
        text_index += syllable->GetText().size();

        const bool last_syllable = i == syllables.size() - 1;
        const bool show_end_marker = syllable->GetEnd() !=
                (last_syllable ? KaraokeData::PLACEHOLDER_TIME : syllables[i + 1]->GetStart());

        m_syllables.emplace_back(std::make_unique<SyllableDecorations>(text_edit,
                                         text_start_index, text_index, syllable->GetStart(),
                                         syllable->GetEnd(), show_end_marker, m_state));
    }

    m_end_index = text_index;
    SetColor(text_edit->document(), m_start_index, prefix_end_index, GetPlayingColor());
    SetColor(text_edit->document(), prefix_end_index, m_end_index, m_state);
}

void LineTimingDecorations::Update(Milliseconds time)
{
    const TimingState state = GetTimingState(time, m_line.GetStart(), m_line.GetEnd());
    if (state == m_state && state != TimingState::Playing)
        return;
    m_state = state;

    for (std::unique_ptr<SyllableDecorations>& syllable : m_syllables)
        syllable->Update(time, state == TimingState::Playing);
}

int LineTimingDecorations::GetPosition() const
{
    return m_start_index;
}

int LineTimingDecorations::GetSyllableCount() const
{
    return m_syllables.size();
}

void LineTimingDecorations::AddToPosition(int diff)
{
    m_start_index += diff;
    m_end_index += diff;

    for (std::unique_ptr<SyllableDecorations>& syllable : m_syllables)
        syllable->AddToPosition(diff);
}

int LineTimingDecorations::TextPositionToSyllable(int position) const
{
    const auto it = std::lower_bound(m_syllables.cbegin(), m_syllables.cend(), position,
                    [](const std::unique_ptr<SyllableDecorations>& syllable, int position) {
        return syllable->GetPosition() < position;
    });

    return it - m_syllables.cbegin();
}

int LineTimingDecorations::TextPositionFromSyllable(int position) const
{
    if (m_syllables.size() <= position)
        return m_end_index;
    else
        return m_syllables[position]->GetPosition();
}

void LineTimingDecorations::Relayout()
{
    for (std::unique_ptr<SyllableDecorations>& syllable : m_syllables)
    {
        syllable->Relayout();
    }
}

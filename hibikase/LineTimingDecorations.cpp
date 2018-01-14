// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <memory>

#include <QBrush>
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

static constexpr int SYLLABLE_MARKER_WIDTH = 4;
static constexpr int SYLLABLE_MARKER_HEIGHT = 4;
static constexpr int PROGRESS_LINE_HEIGHT = 1;

static QTextCharFormat ActiveColor()
{
    QTextCharFormat color;
    color.setBackground(Qt::green);
    return color;
}

static QTextCharFormat InactiveColor()
{
    QTextCharFormat color;
    color.setBackground(Qt::white);
    return color;
}

SyllableDecorations::SyllableDecorations(QPlainTextEdit* text_edit, size_t start_index,
        size_t end_index, Milliseconds start_time, Milliseconds end_time)
    : QWidget(text_edit->viewport()), m_start_index(start_index), m_end_index(end_index),
      m_start_time(start_time), m_end_time(end_time)
{
    QTextCursor cursor(text_edit->document());
    cursor.setPosition(start_index);
    const QRect start_rect = text_edit->cursorRect(cursor);
    cursor.setPosition(end_index);
    const QRect end_rect = text_edit->cursorRect(cursor);

    const int left = std::min(start_rect.left(), end_rect.left()) - SYLLABLE_MARKER_WIDTH;
    const int top = std::max(start_rect.bottom(), end_rect.bottom());
    const int width = std::abs(end_rect.left() - start_rect.left()) + SYLLABLE_MARKER_WIDTH;
    const int height = SYLLABLE_MARKER_HEIGHT + PROGRESS_LINE_HEIGHT;
    setGeometry(QRect(left, top, width, height));
    setPalette(Qt::transparent);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setVisible(true);
}

void SyllableDecorations::Update(Milliseconds time, QTextDocument* document,
                                 bool line_is_inactivating)
{
    const bool is_active = m_start_time <= time && m_end_time > time;
    if (!line_is_inactivating && !is_active && !m_was_active)
        return;

    if (!line_is_inactivating && m_start_time <= time)
    {
        const qreal progress = static_cast<qreal>((time - m_start_time).count()) /
                               (m_end_time - m_start_time).count();
        const int max_width = width() - SYLLABLE_MARKER_WIDTH;
        m_progress_line_width = max_width * std::min<qreal>(1.0, progress);
    }
    else
    {
        m_progress_line_width = 0;
    }

    update();

    if (is_active == m_was_active)
        return;
    m_was_active = is_active;

    QTextCursor cursor(document);
    cursor.setPosition(m_start_index, QTextCursor::MoveAnchor);
    cursor.setPosition(m_end_index, QTextCursor::KeepAnchor);
    cursor.setCharFormat(is_active ? ActiveColor() : InactiveColor());
}

void SyllableDecorations::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(Qt::gray));

    if (m_progress_line_width != 0)
    {
        painter.drawRect(QRectF(SYLLABLE_MARKER_WIDTH, SYLLABLE_MARKER_HEIGHT,
                                m_progress_line_width, PROGRESS_LINE_HEIGHT));
    }

    QPainterPath path;
    path.moveTo(QPoint(SYLLABLE_MARKER_WIDTH, 0));
    path.lineTo(QPoint(0, SYLLABLE_MARKER_HEIGHT));
    path.lineTo(QPoint(SYLLABLE_MARKER_WIDTH, SYLLABLE_MARKER_HEIGHT));
    path.lineTo(QPoint(SYLLABLE_MARKER_WIDTH, 0));

    painter.fillPath(path, QBrush(Qt::gray));
}

LineTimingDecorations::LineTimingDecorations(KaraokeData::Line* line, size_t position,
                                             QPlainTextEdit* text_edit, QObject* parent)
    : QObject(parent), m_line(line), m_position(position), m_text_edit(text_edit)
{
    auto syllables = m_line->GetSyllables();
    m_syllables.reserve(syllables.size());
    size_t i = m_position + m_line->GetPrefix().size();
    for (KaraokeData::Syllable* syllable : syllables)
    {
        const size_t start_index = i;
        i += syllable->GetText().size();
        m_syllables.emplace_back(std::make_unique<SyllableDecorations>(
                        text_edit, start_index, i, syllable->GetStart(), syllable->GetEnd()));
    }
}

void LineTimingDecorations::Update(std::chrono::milliseconds time)
{
    const bool is_active = m_line->GetStart() <= time && m_line->GetEnd() > time;
    if (!is_active && !m_was_active)
        return;
    m_was_active = is_active;

    for (std::unique_ptr<SyllableDecorations>& syllable : m_syllables)
        syllable->Update(time, m_text_edit->document(), !is_active);
}

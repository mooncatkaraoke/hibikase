// SPDX-License-Identifier: GPL-2.0-or-later

#include "LyricsEditor.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <memory>
#include <utility>

#include <QAction>
#include <QEvent>
#include <QFont>
#include <QInputDialog>
#include <QLocale>
#include <QMenu>
#include <QPair>
#include <QPoint>
#include <QRect>
#include <QTextCursor>
#include <QVBoxLayout>

#include "KaraokeData/ReadOnlySong.h"
#include "KaraokeData/Song.h"
#include "Settings.h"
#include "TextTransform/RomanizeHangul.h"
#include "TextTransform/Syllabify.h"

static QFont WithPointSize(QFont font, qreal size)
{
    // For historical reasons,[0] points are one-third bigger on Windows than
    // on macOS. Qt doesn't account for this, so we must.
    // [0] https://blogs.msdn.microsoft.com/fontblog/2005/11/08/where-does-96-dpi-come-from-in-windows/
#ifdef TARGET_OS_MAC
    size *= 96.0 / 72.0;
#endif
    font.setPointSizeF(size);
    return font;
}

TimingEventFilter::TimingEventFilter(QObject* parent) : QObject(parent)
{
}

bool TimingEventFilter::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        // TODO: We never seem to get any mouse button press events for some reason?

        QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
        if (mouse_event->button() & Qt::MouseButton::LeftButton)
        {
            emit GoToPosition(mouse_event->pos());
            return true;
        }
        else
        {
            return QObject::eventFilter(obj, event);
        }
    }
    else if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* key_event = static_cast<QKeyEvent*>(event);

        switch (key_event->key())
        {
        case Qt::Key_Space:
            if (!key_event->isAutoRepeat())
                emit SetSyllableStart();
            return true;
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (!key_event->isAutoRepeat())
                emit SetSyllableEnd();
            return true;
        case Qt::Key_Left:
            emit GoToPreviousSyllable();
            return true;
        case Qt::Key_Right:
            emit GoToNextSyllable();
            return true;
        case Qt::Key_Up:
            emit GoToPreviousLine();
            return true;
        case Qt::Key_Down:
            emit GoToNextLine();
            return true;
        default:
            return QObject::eventFilter(obj, event);
        }
    }
    else
    {
        return QObject::eventFilter(obj, event);
    }
}

TextEventFilter::TextEventFilter(QObject* parent) : QObject(parent)
{
}

bool TextEventFilter::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::KeyPress)
    {
        QKeyEvent* key_event = static_cast<QKeyEvent*>(event);

        if (key_event->key() == Qt::Key_Space &&
#if defined(Q_OS_MACOS) || defined(Q_OS_OSX)
            // on macOS, ControlModifier is mapped to ⌘ (Command), but ⌘+Space
            // is the default input method switching shortcut, which we can't
            // override, so let's use MetaModifier which /actually/ maps to Ctrl
            // on macOS. Ctrl+Space seems to be fine as a shortcut.
            key_event->modifiers().testFlag(Qt::MetaModifier))
#else
            // on other platforms the shortcut is also Ctrl+Space
            key_event->modifiers().testFlag(Qt::ControlModifier))
#endif
        {
            emit ToggleSyllable();
            return true;
        }
    }

    return QObject::eventFilter(obj, event);
}

LyricsEditor::LyricsEditor(QWidget* parent) : QWidget(parent)
{
    m_raw_text_edit = new QPlainTextEdit();
    m_rich_text_edit = new QPlainTextEdit();

    connect(m_rich_text_edit, &QPlainTextEdit::cursorPositionChanged,
            this, &LyricsEditor::OnCursorPositionChanged);

    connect(m_raw_text_edit->document(), &QTextDocument::contentsChange,
            this, &LyricsEditor::OnRawContentsChange);

    m_rich_text_edit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_rich_text_edit, &QPlainTextEdit::customContextMenuRequested,
            [this](const QPoint& point) { ShowContextMenu(point, m_rich_text_edit); });
    m_raw_text_edit->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_raw_text_edit, &QPlainTextEdit::customContextMenuRequested,
            [this](const QPoint& point) { ShowContextMenu(point, m_raw_text_edit); });

    connect(&m_timing_event_filter, &TimingEventFilter::SetSyllableStart,
            this, &LyricsEditor::SetSyllableStart);
    connect(&m_timing_event_filter, &TimingEventFilter::SetSyllableEnd,
            this, &LyricsEditor::SetSyllableEnd);
    connect(&m_timing_event_filter, &TimingEventFilter::GoToPreviousSyllable,
            [this]() { GoTo(GetPreviousSyllable()); });
    connect(&m_timing_event_filter, &TimingEventFilter::GoToNextSyllable,
            [this]() { GoTo(GetNextSyllable()); });
    connect(&m_timing_event_filter, &TimingEventFilter::GoToPreviousLine,
            [this]() { GoTo(GetPreviousLine()); });
    connect(&m_timing_event_filter, &TimingEventFilter::GoToNextLine,
            [this]() { GoTo(GetNextLine()); });

    connect(&m_text_event_filter, &TextEventFilter::ToggleSyllable,
            this, &LyricsEditor::ToggleSyllable);

    m_rich_text_edit->setReadOnly(true);

    m_raw_text_edit->setTabChangesFocus(true);
    m_rich_text_edit->setTabChangesFocus(true);

    m_raw_text_edit->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_rich_text_edit->setLineWrapMode(QPlainTextEdit::NoWrap);

    m_rich_text_edit->setFont(WithPointSize(QFont(), Settings::timing_text_font_size.Get()));
    Settings::timing_text_font_size.SetCallback([this](qreal new_value) {
        m_rich_text_edit->setFont(WithPointSize(m_rich_text_edit->font(), new_value));
        for (auto &decorations : m_line_timing_decorations)
        {
            decorations->Relayout();
        }
    });
    m_raw_text_edit->setFont(WithPointSize(QFont(), Settings::raw_font_size.Get()));
    Settings::raw_font_size.SetCallback([this](qreal new_value) {
        m_raw_text_edit->setFont(WithPointSize(m_raw_text_edit->font(), new_value));
    });

    SetMode(Mode::Text);

    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->setMargin(0);
    main_layout->addWidget(m_raw_text_edit);
    main_layout->addWidget(m_rich_text_edit);

    setLayout(main_layout);
}

void LyricsEditor::ReloadSong(KaraokeData::Song* song)
{
    m_song_ref = song;

    m_raw_updates_disabled = true;
    m_raw_text_edit->setPlainText(song->GetRaw());
    m_raw_updates_disabled = false;
    m_rich_updates_disabled = true;
    m_rich_text_edit->setPlainText(song->GetText());
    m_rich_updates_disabled = false;

    const QVector<KaraokeData::Line*> lines = song->GetLines();
    m_line_timing_decorations.clear();
    m_line_timing_decorations.reserve(lines.size());
    int i = 0;
    for (const KaraokeData::Line* line : lines)
    {
        auto decorations = std::make_unique<LineTimingDecorations>(*line, i, m_rich_text_edit, m_time);
        decorations->Update(m_time);
        m_line_timing_decorations.emplace_back(std::move(decorations));

        i += line->GetText().size() + sizeof('\n');
    }

    connect(m_song_ref, &KaraokeData::Song::LinesChanged, this, &LyricsEditor::OnLinesChanged);
}

void LyricsEditor::UpdateTime(std::chrono::milliseconds time)
{
    for (auto& decorations : m_line_timing_decorations)
        decorations->Update(time);

    m_time = time;
}

void LyricsEditor::UpdateSpeed(double speed)
{
    m_speed = speed;
}

void LyricsEditor::SetMode(Mode mode)
{
    if (mode == Mode::Timing)
    {
        m_rich_text_edit->removeEventFilter(&m_text_event_filter);
        m_rich_text_edit->installEventFilter(&m_timing_event_filter);
    }
    else
    {
        m_rich_text_edit->removeEventFilter(&m_timing_event_filter);
        m_rich_text_edit->installEventFilter(&m_text_event_filter);
    }

    switch (mode)
    {
    case Mode::Timing:
    case Mode::Text:
        m_raw_text_edit->setVisible(false);
        m_rich_text_edit->setVisible(true);
        m_rich_text_edit->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        break;
    case Mode::Raw:
        m_raw_text_edit->setVisible(true);
        m_rich_text_edit->setVisible(false);
        break;
    }

    // The visibility changes above must happen before the cursor position updates below,
    // otherwise the SyllableDecorations (see LineTimingDecorations.h) won't update correctly.
    // (It seems like SyllableDecorations don't receive move events if m_rich_text_edit
    // scrolls as a result of changing the cursor position while it is invisible.)

    if (mode == Mode::Raw && m_mode != Mode::Raw)
    {
        const QTextCursor cursor = m_rich_text_edit->textCursor();
        QTextCursor end_cursor = m_rich_text_edit->textCursor();
        end_cursor.movePosition(QTextCursor::End);

        QTextCursor raw_cursor = m_raw_text_edit->textCursor();
        if (cursor == end_cursor)
        {
            raw_cursor.movePosition(QTextCursor::End);
        }
        else
        {
            const int position = m_rich_text_edit->textCursor().position();
            const KaraokeData::SongPosition song_position = ToSongPosition(position, m_mode);
            const int raw_position = m_song_ref->PositionToRaw(song_position);

            raw_cursor.setPosition(raw_position);
        }
        m_raw_text_edit->setTextCursor(raw_cursor);
    }
    if (mode != Mode::Raw && m_mode == Mode::Raw)
    {
        const int position = m_raw_text_edit->textCursor().position();
        const KaraokeData::SongPosition song_position = ToSongPosition(position, m_mode);
        QTextCursor cursor = m_rich_text_edit->textCursor();
        if (static_cast<size_t>(song_position.line) < m_line_timing_decorations.size())
        {
            const int line_position = m_line_timing_decorations[song_position.line]->GetPosition();
            cursor.setPosition(line_position + song_position.position_in_line);
        }
        else
        {
            cursor.movePosition(QTextCursor::End);
        }
        m_rich_cursor_updates_disabled = true;
        m_rich_text_edit->setTextCursor(cursor);
        m_rich_cursor_updates_disabled = false;
    }

    m_mode = mode;

    OnCursorPositionChanged();
}

void LyricsEditor::OnLinesChanged(int line_position, int lines_removed, int lines_added,
                                  int raw_position, int raw_chars_removed, int raw_chars_added)
{
    (void)raw_chars_added;

    emit Modified();

    if (!m_raw_updates_disabled)
    {
        m_raw_updates_disabled = true;

        const int raw_char_count = m_raw_text_edit->document()->characterCount();
        Q_ASSERT(raw_position <= raw_char_count + 1);
        if (raw_position > raw_char_count)
            m_raw_text_edit->appendPlainText(QStringLiteral("\n"));  // Add line break between lines

        if (raw_position > 0 && lines_removed > 0 && lines_added == 0)
        {
            // Erase the previous line's trailing newline character
            raw_position -= 1;
            raw_chars_removed += 1;
        }

        QTextCursor cursor(m_raw_text_edit->document());
        cursor.setPosition(raw_position);
        cursor.setPosition(raw_position + raw_chars_removed, QTextCursor::MoveMode::KeepAnchor);
        cursor.insertText(m_song_ref->GetRaw(line_position, line_position + lines_added));

        m_raw_updates_disabled = false;
    }

    if (!m_rich_updates_disabled)
    {
        const size_t size = m_line_timing_decorations.size();
        const int start_position = size == 0 ? 0 :
                    m_line_timing_decorations[line_position]->GetPosition();
        const int end_position = line_position + lines_removed >= size ?
                    m_rich_text_edit->document()->characterCount() :
                    m_line_timing_decorations[line_position + lines_removed]->GetPosition();

        const QVector<KaraokeData::Line*> lines = m_song_ref->GetLines();

        QTextCursor cursor(m_rich_text_edit->document());
        cursor.setPosition(start_position);
        cursor.setPosition(end_position - sizeof('\n'), QTextCursor::MoveMode::KeepAnchor);
        cursor.insertText(m_song_ref->GetText(line_position, line_position + lines_added));

        auto replace_it = m_line_timing_decorations.begin() + line_position;
        if (lines_removed > lines_added)
        {
            m_line_timing_decorations.erase(replace_it, replace_it + (lines_removed - lines_added));
        }
        else if (lines_removed < lines_added)
        {
            std::vector<std::unique_ptr<LineTimingDecorations>> lines_to_insert(lines_added - lines_removed);
            m_line_timing_decorations.insert(replace_it, std::make_move_iterator(lines_to_insert.begin()),
                                                         std::make_move_iterator(lines_to_insert.end()));
        }

        int current_position = start_position;
        for (int i = line_position; i < line_position + lines_added; ++i)
        {
            auto decorations = std::make_unique<LineTimingDecorations>(
                        *lines[i], current_position, m_rich_text_edit, m_time);
            decorations->Update(m_time);
            m_line_timing_decorations[i] = std::move(decorations);

            current_position += lines[i]->GetText().size() + sizeof('\n');
        }

        if (current_position != end_position)
        {
            const int position_diff = current_position - end_position;
            for (size_t i = line_position + lines_added; i < m_line_timing_decorations.size(); ++i)
                m_line_timing_decorations[i]->AddToPosition(position_diff);
        }
    }
}

void LyricsEditor::OnRawContentsChange(int position, int chars_removed, int chars_added)
{
    if (m_raw_updates_disabled)
        return;

    m_raw_updates_disabled = true;
    m_song_ref->UpdateRawText(position, chars_removed,
                              m_raw_text_edit->toPlainText().midRef(position, chars_added));
    m_raw_updates_disabled = false;
}

void LyricsEditor::OnCursorPositionChanged()
{
    if (m_mode != Mode::Timing || m_rich_cursor_updates_disabled)
        return;

    const int position = m_rich_text_edit->textCursor().position();
    const SyllablePosition syllable = TextPositionToSyllable(position);
    const SyllablePosition previous_syllable = GetPreviousSyllable(syllable);
    const int line = TextPositionToLine(position);
    if (syllable.IsValid() && line != TextPositionToLine(position + 1))
    {
        // Clicked on the end of a line. Go to the start of the next line,
        // since that's the position you have to be at to edit the end time of a line.
        GoTo(syllable);
    }
    else if (!syllable.IsValid() && !previous_syllable.IsValid())
    {
    }
    else if (!syllable.IsValid() || (syllable.line != line && previous_syllable.line == line))
    {
        GoTo(previous_syllable);
    }
    else if (!previous_syllable.IsValid() || syllable.line != previous_syllable.line)
    {
        GoTo(syllable);
    }
    else
    {
        const int position_1 = TextPositionFromSyllable(previous_syllable);
        const int position_2 = TextPositionFromSyllable(syllable);

        if (position - position_1 < position_2 - position)
            GoTo(previous_syllable);
        else
            GoTo(syllable);
    }
}

void LyricsEditor::GoToPosition(QPoint pos)
{
    QTextCursor cursor = m_rich_text_edit->cursorForPosition(pos);
    const int position = cursor.position();
    const SyllablePosition syllable = TextPositionToSyllable(position);
    const SyllablePosition previous_syllable = GetPreviousSyllable(syllable);
    const int line = TextPositionToLine(position);
    if (syllable.IsValid() && line != TextPositionToLine(position + 1))
    {
        // Clicked on the end of a line. Go to the start of the next line,
        // since that's the position you have to be at to edit the end time of a line.
        GoTo(syllable);
    }
    else if (!syllable.IsValid() && !previous_syllable.IsValid())
    {
    }
    else if (!syllable.IsValid() || (syllable.line != line && previous_syllable.line == line))
    {
        GoTo(previous_syllable);
    }
    else if (!previous_syllable.IsValid() || syllable.line != previous_syllable.line)
    {
        GoTo(syllable);
    }
    else
    {
        cursor.setPosition(TextPositionFromSyllable(previous_syllable));
        const QRect rect_1 = m_rich_text_edit->cursorRect(cursor);
        cursor.setPosition(TextPositionFromSyllable(syllable));
        const QRect rect_2 = m_rich_text_edit->cursorRect(cursor);

        if (pos.x() - rect_1.right() < rect_2.left() - pos.x())
            GoTo(previous_syllable);
        else
            GoTo(syllable);
    }
}

void LyricsEditor::AddLyricsActionsToMenu(QMenu* menu, QPlainTextEdit* text_edit)
{
    const bool has_selection = text_edit->textCursor().hasSelection();

    menu->addSeparator();

    QMenu* syllabify = menu->addMenu(QStringLiteral("Syllabify"));
    syllabify->setEnabled(has_selection);
    syllabify->addAction(QStringLiteral("Basic"), [this]{ Syllabify(QString()); });
    syllabify->addSeparator();
    AddSyllabificationLanguages(syllabify);

    menu->addAction(QStringLiteral("Romanize Hangul"), this,
                    SLOT(RomanizeHangul()))->setEnabled(has_selection);

    menu->addAction(QStringLiteral("Shift Timings..."), this,
                    SLOT(ShiftTimings()))->setEnabled(has_selection);
}

void LyricsEditor::ShowContextMenu(const QPoint& point, QPlainTextEdit* text_edit)
{
    std::unique_ptr<QMenu> menu(text_edit->createStandardContextMenu(point));
    AddLyricsActionsToMenu(menu.get(), text_edit);
    menu->exec(text_edit->mapToGlobal(point));
}

void LyricsEditor::AddActionsToMenu(QMenu* menu)
{
    QPlainTextEdit* text_edit = GetActiveTextEdit();

    std::unique_ptr<QMenu> menu_template(text_edit->createStandardContextMenu());
    for (QAction* action : menu_template->actions())
    {
        menu->addAction(action);
        action->setParent(menu);
    }

    AddLyricsActionsToMenu(menu, text_edit);
}

void LyricsEditor::AddSyllabificationLanguages(QMenu* menu)
{
    QVector<QPair<QString, QString>> languages;

    for (const QString& language_code : TextTransform::Syllabifier::AvailableLanguages())
    {
        const QLocale locale(language_code);

        if (locale.language() <= QLocale::Language::C && language_code != QStringLiteral("la"))
        {
            languages.push_back(QPair<QString, QString>(language_code, language_code));
        }
        else
        {
            QString language_name = QLocale::languageToString(locale.language());

            // Without this extra check, the language code "no" will give us the language name
            // "Norwegian Bokmal". This not only inappropriately uses a instead of å, but is
            // also overly specific ("nb" is the bokmål-specific code).
            if (language_code == QStringLiteral("no"))
                language_name = QStringLiteral("Norwegian");

            // Qt doesn't seem to be able to set the language of a QLocale to Latin.
            // Let's handle Latin manually instead. (The locale will be the C locale.)
            if (language_code == QStringLiteral("la"))
                language_name = QStringLiteral("Latin");

            for (const QString& component : language_code.split(QStringLiteral("_")))
            {
                if (component.size() == 2 && component[0].isUpper() && component[1].isUpper())
                {
                    language_name += QStringLiteral(" (%1)").arg(QLocale::countryToString(locale.country()));
                }
                else if (component.size() == 4)
                {
                    if (component == QStringLiteral("Latn") && locale.script() != QLocale::Script::LatinScript)
                        language_name += QStringLiteral(" (Romanized)");
                    else
                        language_name += QStringLiteral(" (%1)").arg(QLocale::scriptToString(locale.script()));
                }
            }

            languages.push_back(QPair<QString, QString>(language_code, language_name));
        }
    }

    std::sort(languages.begin(), languages.end(),
              [](const QPair<QString, QString>& a, const QPair<QString, QString>& b) {
                    return a.second < b.second;
    });

    for (const QPair<QString, QString>& language : languages)
        menu->addAction(language.second, [this, language]{ Syllabify(language.first); });
}

void LyricsEditor::ApplyLineTransformation(KaraokeData::SongPosition start_position,
            KaraokeData::SongPosition end_position, bool split_syllables_at_start_and_end,
            std::function<std::unique_ptr<KaraokeData::Line>(const KaraokeData::Line&)> f)
{
    KaraokeData::ReadOnlyLine first_line_first_half;
    KaraokeData::ReadOnlyLine first_line_last_half;
    KaraokeData::ReadOnlyLine last_line_first_half;
    KaraokeData::ReadOnlyLine last_line_last_half;
    bool syllable_boundary_at_start;
    bool syllable_boundary_at_end;

    const QVector<const KaraokeData::Line*> old_lines = m_song_ref->GetLines(start_position,
                end_position, &first_line_first_half, &first_line_last_half, &last_line_first_half,
                &last_line_last_half, &syllable_boundary_at_start, &syllable_boundary_at_end);

    std::vector<std::unique_ptr<KaraokeData::Line>> new_lines;
    QVector<const KaraokeData::Line*> new_line_pointers;
    new_lines.reserve(old_lines.size());
    new_line_pointers.reserve(old_lines.size());

    for (const KaraokeData::Line* old_line : old_lines)
    {
        new_lines.push_back(f(*old_line));
        new_line_pointers.push_back(new_lines.back().get());
    }

    if (split_syllables_at_start_and_end)
    {
        syllable_boundary_at_start = true;
        syllable_boundary_at_end = true;
    }

    m_song_ref->ReplaceLines(start_position, end_position, new_line_pointers,
                             &first_line_first_half, &last_line_last_half,
                             syllable_boundary_at_start, syllable_boundary_at_end);
}

void LyricsEditor::ApplyLineTransformation(bool split_syllables_at_start_and_end,
            std::function<std::unique_ptr<KaraokeData::Line>(const KaraokeData::Line&)> f)
{
    const QTextCursor cursor = GetActiveTextEdit()->textCursor();
    int start = cursor.position();
    int end = cursor.anchor();
    if (start > end)
        std::swap(start, end);

    const KaraokeData::SongPosition start_position = ToSongPosition(start, m_mode);
    const KaraokeData::SongPosition end_position = ToSongPosition(end, m_mode);

    return ApplyLineTransformation(start_position, end_position,
                                   split_syllables_at_start_and_end, std::move(f));
}

void LyricsEditor::Syllabify(const QString& language_code)
{
    const TextTransform::Syllabifier syllabifier(language_code);
    ApplyLineTransformation(true, [&syllabifier](const KaraokeData::Line& line) {
        return syllabifier.Syllabify(line);
    });
}

void LyricsEditor::RomanizeHangul()
{
    ApplyLineTransformation(false, [](const KaraokeData::Line& line) {
        return TextTransform::RomanizeHangul(line.GetSyllables(), line.GetPrefix());
    });
}

void LyricsEditor::ShiftTimings()
{
    // Calculate what range the offset can be in without moving anything
    // outside of the safe range.

    KaraokeData::Centiseconds min_safe_offset = -KaraokeData::MAXIMUM_TIME;
    KaraokeData::Centiseconds max_safe_offset = KaraokeData::MAXIMUM_TIME;

    KaraokeData::ReadOnlyLine partial_first_line;
    KaraokeData::ReadOnlyLine partial_last_line;
    const QVector<const KaraokeData::Line*> selected_lines =
            GetSelectedLines(&partial_first_line, &partial_last_line);

    for (const KaraokeData::Line* line : selected_lines)
    {
        for (const KaraokeData::Syllable* syllable : line->GetSyllables())
        {
            if (syllable->GetStart() != KaraokeData::PLACEHOLDER_TIME)
            {
                min_safe_offset = std::max(min_safe_offset, KaraokeData::MINIMUM_TIME - syllable->GetStart());
                max_safe_offset = std::min(max_safe_offset, KaraokeData::MAXIMUM_TIME - syllable->GetStart());
            }
            if (syllable->GetEnd() != KaraokeData::PLACEHOLDER_TIME)
            {
                min_safe_offset = std::max(min_safe_offset, KaraokeData::MINIMUM_TIME - syllable->GetEnd());
                max_safe_offset = std::min(max_safe_offset, KaraokeData::MAXIMUM_TIME - syllable->GetEnd());
            }
        }
    }

    bool ok;
    double offset = QInputDialog::getDouble(
        this,
        "Shift Timings",
        "Enter an offset in seconds. A positive offset shifts timings to "
        "be later; a negative offset shifts timings to be earlier.",
        0.05, // MP3 adds approximately this offset when compressing for 44.1kHz
        static_cast<double>(min_safe_offset.count()) / 100,
        static_cast<double>(max_safe_offset.count()) / 100,
        2, // at most centisecond precision
        &ok
    );
    if (!ok)
        return;

    KaraokeData::Centiseconds offset_cs(static_cast<int>(offset * 100));
    ApplyLineTransformation(false, [offset_cs](const KaraokeData::Line& line) {
        const QVector<const KaraokeData::Syllable*> syllables = line.GetSyllables();

        auto shifted_line = std::make_unique<KaraokeData::ReadOnlyLine>();
        shifted_line->m_prefix = line.GetPrefix();
        shifted_line->m_syllables.reserve(syllables.size());

        for (const KaraokeData::Syllable* syllable : syllables)
        {
            KaraokeData::Centiseconds new_start = syllable->GetStart();
            if (new_start != KaraokeData::PLACEHOLDER_TIME)
                new_start += offset_cs;
            KaraokeData::Centiseconds new_end = syllable->GetEnd();
            if (new_end != KaraokeData::PLACEHOLDER_TIME)
                new_end += offset_cs;

            shifted_line->m_syllables.emplace_back(std::make_unique<KaraokeData::ReadOnlySyllable>(
                    syllable->GetText(), new_start, new_end));
        }

        return shifted_line;
    });
}

void LyricsEditor::SetSyllableStart()
{
    const SyllablePosition current_syllable_pos = GetCurrentSyllable();
    KaraokeData::Syllable* current_syllable = GetSyllable(current_syllable_pos);

    if (!current_syllable)
        return;

    const SyllablePosition previous_syllable_pos = GetPreviousSyllable();
    const SyllablePosition next_syllable_pos = GetNextSyllable();

    const KaraokeData::Centiseconds time = GetLatencyCompensatedTime();

    if (current_syllable_pos != previous_syllable_pos)
    {
        KaraokeData::Syllable* previous_syllable = GetSyllable(previous_syllable_pos);
        if (previous_syllable && previous_syllable->GetEnd() == current_syllable->GetStart())
            previous_syllable->SetEnd(time);
    }

    current_syllable->SetStart(time);

    GoTo(next_syllable_pos);
}

void LyricsEditor::SetSyllableEnd()
{
    KaraokeData::Syllable* syllable = GetSyllable(GetPreviousSyllable());

    if (!syllable)
        return;

    QTextCursor cursor = m_rich_text_edit->textCursor();
    const int cursor_position = cursor.position();

    syllable->SetEnd(GetLatencyCompensatedTime());

    cursor.setPosition(cursor_position);
    m_rich_text_edit->setTextCursor(cursor);
}

void LyricsEditor::GoTo(SyllablePosition position)
{
    if (!position.IsValid())
        return;

    const bool updates_disabled = m_rich_cursor_updates_disabled;
    m_rich_cursor_updates_disabled = true;

    QTextCursor cursor = m_rich_text_edit->textCursor();
    cursor.setPosition(TextPositionFromSyllable(position));
    m_rich_text_edit->setTextCursor(cursor);

    m_rich_cursor_updates_disabled = updates_disabled;
}

LyricsEditor::SyllablePosition LyricsEditor::SkipLinesWithoutSyllablesForward(SyllablePosition pos) const
{
    if (m_line_timing_decorations.empty())
        return SyllablePosition{-1, 0};

    if (!pos.IsValid())
    {
        const int last_line = m_line_timing_decorations.size() - 1;
        pos = SyllablePosition{last_line, m_line_timing_decorations[last_line]->GetSyllableCount()};
    }

    SyllablePosition new_pos = pos;

    // Seek forward until we find a line with syllables.
    while (new_pos.line < m_line_timing_decorations.size() &&
           m_line_timing_decorations[new_pos.line]->GetSyllableCount() <= new_pos.syllable)
    {
        new_pos.line++;
        new_pos.syllable = 0;
    }

    if (new_pos.line >= m_line_timing_decorations.size())
    {
        // No lines with syllables were found when seeking forwards.
        // Seek backwards instead to find the last line of the song that has syllables.

        new_pos = pos;

        if (new_pos.line >= m_line_timing_decorations.size())
            new_pos.line = m_line_timing_decorations.size() - 1;

        while (new_pos.line >= 0 &&
               m_line_timing_decorations[new_pos.line]->GetSyllableCount() == 0)
        {
            new_pos.line--;
            new_pos.syllable = 0;
        }

        if (new_pos.line >= 0 && new_pos.line < m_line_timing_decorations.size())
        {
            // It needs to be possible to place the cursor "after" the last line that has a syllable,
            // because otherwise it's impossible to set the end time of the last syllable of that line.
            // Because of this, treat the very end of that line as if it's a separate line.
            // (If there is a line with no syllables after the line, it would be valid to pick
            // that as the position instead, but this doesn't work when there are no such lines.)
            new_pos.syllable = m_line_timing_decorations[new_pos.line]->GetSyllableCount();
        }
    }

    if (new_pos.line < 0)
    {
        // No lines with syllables were found when seeking backwards either.
        // In other words, the song contains no syllables. Give up and return an invalid position.
        return SyllablePosition{-1, 0};
    }

    return new_pos;
}

LyricsEditor::SyllablePosition LyricsEditor::SkipLinesWithoutSyllablesBackward(SyllablePosition pos) const
{
    if (m_line_timing_decorations.empty())
        return SyllablePosition{-1, 0};

    if (!pos.IsValid())
        pos = SyllablePosition{0, 0};

    SyllablePosition new_pos = pos;

    // It needs to be possible to place the cursor "after" the last line that has a syllable,
    // because otherwise it's impossible to set the end time of the last syllable of that line.
    // Because of this, treat the very end of that line as if it's a separate line.
    const int last_line = GetLastLineWithSyllables();
    if (pos.line == last_line && pos.syllable != 0 &&
        pos.syllable == m_line_timing_decorations[last_line]->GetSyllableCount())
    {
        return SyllablePosition{last_line, 0};
    }

    // Seek backward until we find a line with syllables.
    while (new_pos.line >= 0 && m_line_timing_decorations[new_pos.line]->GetSyllableCount() == 0)
    {
        new_pos.line--;
        new_pos.syllable = 0;
    }

    if (new_pos.line < 0)
    {
        // No lines with syllables were found when seeking backwards.
        // Seek forwards instead to find the last line of the song that has syllables.

        new_pos = SyllablePosition{pos.line, 0};

        while (new_pos.line < m_line_timing_decorations.size() &&
               m_line_timing_decorations[new_pos.line]->GetSyllableCount() == 0)
        {
            new_pos.line++;
            new_pos.syllable = 0;
        }
    }

    if (new_pos.line >= m_line_timing_decorations.size())
    {
        // No lines with syllables were found when seeking forwards either.
        // In other words, the song contains no syllables. Give up and return an invalid position.
        return SyllablePosition{-1, 0};
    }

    return new_pos;
}

int LyricsEditor::TextPositionToLine(int position) const
{
    if (m_line_timing_decorations.empty())
        return 0;

    const auto it = std::upper_bound(m_line_timing_decorations.cbegin(), m_line_timing_decorations.cend(),
                                     position, [](int pos, const auto& line) {
        return pos < line->GetPosition();
    });

    return it - 1 - m_line_timing_decorations.cbegin();
}

LyricsEditor::SyllablePosition LyricsEditor::TextPositionToSyllable(int position) const
{
    if (m_line_timing_decorations.empty())
        return SyllablePosition{-1, 0};

    int line = TextPositionToLine(position);
    int syllable = m_line_timing_decorations[line]->TextPositionToSyllable(position);

    // If we're at the end of a line, skip to the next line that has a syllable
    return SkipLinesWithoutSyllablesForward(SyllablePosition{line, syllable});
}

int LyricsEditor::TextPositionFromSyllable(SyllablePosition position) const
{
    return m_line_timing_decorations[position.line]->TextPositionFromSyllable(position.syllable);
}

LyricsEditor::SyllablePosition LyricsEditor::GetPreviousSyllable() const
{
    return GetPreviousSyllable(TextPositionToSyllable(m_rich_text_edit->textCursor().position()));
}

LyricsEditor::SyllablePosition LyricsEditor::GetPreviousSyllable(SyllablePosition pos) const
{
    if (pos.syllable == 0)
    {
        const SyllablePosition new_pos =
                SkipLinesWithoutSyllablesBackward(SyllablePosition{pos.line - 1, 0});

        if (!new_pos.IsValid() || new_pos.line >= pos.line)
            return new_pos;

        return SyllablePosition{new_pos.line,
                    m_line_timing_decorations[new_pos.line]->GetSyllableCount() - 1};
    }
    else
    {
        return SyllablePosition{pos.line, pos.syllable - 1};
    }
}

LyricsEditor::SyllablePosition LyricsEditor::GetCurrentSyllable() const
{
    return TextPositionToSyllable(m_rich_text_edit->textCursor().position());
}

LyricsEditor::SyllablePosition LyricsEditor::GetNextSyllable() const
{
    return TextPositionToSyllable(m_rich_text_edit->textCursor().position() + 1);
}

int LyricsEditor::GetLastLineWithSyllables() const
{
    int line = m_line_timing_decorations.size() - 1;

    while (line >= 0 && m_line_timing_decorations[line]->GetSyllableCount() == 0)
        line--;

    return line;
}

LyricsEditor::SyllablePosition LyricsEditor::GetPreviousLine() const
{
    const SyllablePosition pos = TextPositionToSyllable(m_rich_text_edit->textCursor().position());

    if (pos.IsValid())
    {
        // It needs to be possible to place the cursor "after" the last line that has a syllable,
        // because otherwise it's impossible to set the end time of the last syllable of that line.
        // Because of this, treat the very end of that line as if it's a valid position.
        const int last_line = GetLastLineWithSyllables();
        if (pos.line == last_line && pos.syllable != 0 &&
            pos.syllable == m_line_timing_decorations[last_line]->GetSyllableCount())
        {
            return SyllablePosition{pos.line, 0};
        }
    }

    return SkipLinesWithoutSyllablesBackward(SyllablePosition{pos.line - 1, 0});
}

LyricsEditor::SyllablePosition LyricsEditor::GetNextLine() const
{
    int line = TextPositionToSyllable(m_rich_text_edit->textCursor().position()).line + 1;
    return SkipLinesWithoutSyllablesForward(SyllablePosition{line, 0});
}

KaraokeData::Syllable* LyricsEditor::GetSyllable(SyllablePosition position) const
{
    if (!position.IsValid())
        return nullptr;

    const QVector<KaraokeData::Line*> lines = m_song_ref->GetLines();
    if (lines.size() <= position.line)
        return nullptr;

    const QVector<KaraokeData::Syllable*> syllables = lines[position.line]->GetSyllables();
    if (syllables.size() <= position.syllable)
        return nullptr;

    return syllables[position.syllable];
}

KaraokeData::SongPosition LyricsEditor::ToSongPosition(int position, Mode mode) const
{
    if (mode == Mode::Raw)
    {
        return m_song_ref->PositionFromRaw(position);
    }
    else
    {
        const int line = TextPositionToLine(position);
        if (line >= m_line_timing_decorations.size())
            return KaraokeData::SongPosition{line, 0};

        const int line_pos = m_line_timing_decorations[line]->GetPosition();
        return KaraokeData::SongPosition{line, position - line_pos};
    }
}

QVector<const KaraokeData::Line*> LyricsEditor::GetSelectedLines(
        KaraokeData::ReadOnlyLine* partial_first_line_out,
        KaraokeData::ReadOnlyLine* partial_last_line_out)
{
    QTextCursor cursor = GetActiveTextEdit()->textCursor();
    int start = cursor.position();
    int end = cursor.anchor();
    if (start > end)
        std::swap(start, end);

    const KaraokeData::SongPosition start_position = ToSongPosition(start, m_mode);
    const KaraokeData::SongPosition end_position = ToSongPosition(end, m_mode);

    KaraokeData::ReadOnlyLine first_line_first_half;
    KaraokeData::ReadOnlyLine last_line_last_half;
    bool syllable_boundary_at_start;
    bool syllable_boundary_at_end;

    return m_song_ref->GetLines(start_position, end_position, &first_line_first_half,
                                partial_first_line_out, partial_last_line_out, &last_line_last_half,
                                &syllable_boundary_at_start, &syllable_boundary_at_end);
}

QPlainTextEdit* LyricsEditor::GetActiveTextEdit()
{
    return m_mode == Mode::Raw ? m_raw_text_edit : m_rich_text_edit;
}

KaraokeData::Centiseconds LyricsEditor::GetLatencyCompensatedTime() const
{
    const std::chrono::milliseconds latency(static_cast<int>(Settings::video_latency.Get() / m_speed));

    return std::chrono::duration_cast<KaraokeData::Centiseconds>(m_time - latency);
}

void LyricsEditor::ToggleSyllable()
{
    QTextCursor cursor = m_rich_text_edit->textCursor();
    const int cursor_position = cursor.position();
    const int line_index = TextPositionToLine(cursor_position);
    if (line_index >= m_line_timing_decorations.size())
        return;

    const KaraokeData::Line* line = m_song_ref->GetLines()[line_index];
    const int index_in_line = cursor_position - m_line_timing_decorations[line_index]->GetPosition();
    if (index_in_line >= line->GetText().size())
        return;

    QVector<const KaraokeData::Syllable*> old_syllables = line->GetSyllables();
    std::vector<std::unique_ptr<KaraokeData::ReadOnlySyllable>> new_syllables;
    new_syllables.reserve(old_syllables.size() + 1);
    for (const KaraokeData::Syllable* syllable : line->GetSyllables())
        new_syllables.push_back(std::make_unique<KaraokeData::ReadOnlySyllable>(*syllable));

    QString prefix = line->GetPrefix();

    if (index_in_line < prefix.size())
    {
        new_syllables.insert(new_syllables.begin(), std::make_unique<KaraokeData::ReadOnlySyllable>(
                                 prefix.right(prefix.size() - index_in_line),
                                 KaraokeData::PLACEHOLDER_TIME, KaraokeData::PLACEHOLDER_TIME));
        prefix = prefix.left(index_in_line);
    }
    else if (index_in_line == prefix.size())
    {
        prefix += new_syllables[0]->GetText();
        new_syllables.erase(new_syllables.begin(), new_syllables.begin() + 1);
    }
    else
    {
        int text_length = prefix.size();
        for (size_t i = 0; i < new_syllables.size(); ++i)
        {
            const QString& text = new_syllables[i]->m_text;
            text_length += text.size();

            if (index_in_line < text_length)
            {
                new_syllables.insert(new_syllables.begin() + i + 1,
                        std::make_unique<KaraokeData::ReadOnlySyllable>(text.right(text_length - index_in_line),
                                KaraokeData::PLACEHOLDER_TIME, new_syllables[i]->GetEnd()));
                new_syllables[i]->m_text = text.left(index_in_line - (text_length - text.size()));
                new_syllables[i]->m_end = KaraokeData::PLACEHOLDER_TIME;
                break;
            }
            else if (index_in_line == text_length)
            {
                new_syllables[i]->m_text += new_syllables[i + 1]->GetText();
                new_syllables[i]->m_end = new_syllables[i + 1]->GetEnd();
                new_syllables.erase(new_syllables.begin() + i + 1, new_syllables.begin() + i + 2);
                break;
            }
        }
    }

    const auto new_line = std::make_unique<KaraokeData::ReadOnlyLine>(std::move(new_syllables), prefix);
    m_song_ref->ReplaceLine(line_index, new_line.get());

    cursor.setPosition(cursor_position);
    m_rich_text_edit->setTextCursor(cursor);
}

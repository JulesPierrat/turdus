#pragma once

#include <turdus/core/Pitch.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/core/Velocity.hpp>
#include <turdus/model/Note.hpp>
#include <turdus/model/Pattern.hpp>

namespace turdus::ui {

// Sink for editing gestures. Implementations route to AppController.
//
// The PianoRoll only emits these on gesture COMMIT (mouse-up for drags, click
// release for taps) — it doesn't spam during ongoing drags. Listener calls are
// expected to mutate the underlying Pattern; the PianoRoll repaints whenever the
// observed pattern reference changes.
class PianoRollListener {
public:
    virtual ~PianoRollListener() = default;

    virtual void roll_add_note(model::Note note) = 0;
    virtual void roll_remove_note(model::NoteId id) = 0;
    virtual void roll_move_note(model::NoteId id,
                                core::Tick new_start,
                                core::Pitch new_pitch) = 0;
    virtual void roll_resize_note(model::NoteId id, core::Tick new_length) = 0;
};

}  // namespace turdus::ui

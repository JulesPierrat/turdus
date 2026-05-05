#pragma once

#include <nlohmann/json.hpp>

#include <turdus/core/Beats.hpp>
#include <turdus/core/Bpm.hpp>
#include <turdus/core/Channel.hpp>
#include <turdus/core/Id.hpp>
#include <turdus/core/Pitch.hpp>
#include <turdus/core/Tick.hpp>
#include <turdus/core/Velocity.hpp>
#include <turdus/model/Note.hpp>
#include <turdus/model/Pattern.hpp>
#include <turdus/model/Project.hpp>
#include <turdus/model/TimeSignature.hpp>
#include <turdus/model/Track.hpp>

// ADL-resolvable to_json / from_json overloads for every core and model type.
// Including this header is what plugs the model into nlohmann/json — keeping JSON
// out of the model and core headers themselves.

namespace turdus::core {

void to_json(nlohmann::json& j, const Tick& t);
void from_json(const nlohmann::json& j, Tick& t);

void to_json(nlohmann::json& j, const Beats& b);
void from_json(const nlohmann::json& j, Beats& b);

void to_json(nlohmann::json& j, const Bpm& b);
void from_json(const nlohmann::json& j, Bpm& b);

void to_json(nlohmann::json& j, const Pitch& p);
void from_json(const nlohmann::json& j, Pitch& p);

void to_json(nlohmann::json& j, const Velocity& v);
void from_json(const nlohmann::json& j, Velocity& v);

void to_json(nlohmann::json& j, const Channel& c);
void from_json(const nlohmann::json& j, Channel& c);

template <typename Tag>
inline void to_json(nlohmann::json& j, const Id<Tag>& id) {
    j = id.raw();
}

template <typename Tag>
inline void from_json(const nlohmann::json& j, Id<Tag>& id) {
    id = Id<Tag>::from_raw(j.get<typename Id<Tag>::value_type>());
}

}  // namespace turdus::core

namespace turdus::model {

void to_json(nlohmann::json& j, const Note& n);
void from_json(const nlohmann::json& j, Note& n);

void to_json(nlohmann::json& j, const TimeSignature& ts);
void from_json(const nlohmann::json& j, TimeSignature& ts);

void to_json(nlohmann::json& j, const Pattern& p);
void from_json(const nlohmann::json& j, Pattern& p);

void to_json(nlohmann::json& j, const Track& t);
void from_json(const nlohmann::json& j, Track& t);

void to_json(nlohmann::json& j, const PatternPlacement& p);
void from_json(const nlohmann::json& j, PatternPlacement& p);

void to_json(nlohmann::json& j, const MidiPortMapping& m);
void from_json(const nlohmann::json& j, MidiPortMapping& m);

void to_json(nlohmann::json& j, const Project& p);
void from_json(const nlohmann::json& j, Project& p);

}  // namespace turdus::model

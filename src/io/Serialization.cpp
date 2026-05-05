#include <turdus/io/Serialization.hpp>

#include <utility>

namespace turdus::core {

void to_json(nlohmann::json& j, const Tick& t) { j = t.value(); }
void from_json(const nlohmann::json& j, Tick& t) { t = Tick{j.get<Tick::value_type>()}; }

void to_json(nlohmann::json& j, const Beats& b) { j = b.value(); }
void from_json(const nlohmann::json& j, Beats& b) { b = Beats{j.get<Beats::value_type>()}; }

void to_json(nlohmann::json& j, const Bpm& b) { j = b.value(); }
void from_json(const nlohmann::json& j, Bpm& b) { b = Bpm{j.get<Bpm::value_type>()}; }

void to_json(nlohmann::json& j, const Pitch& p) { j = static_cast<int>(p.value()); }
void from_json(const nlohmann::json& j, Pitch& p) { p = Pitch{j.get<int>()}; }

void to_json(nlohmann::json& j, const Velocity& v) { j = static_cast<int>(v.value()); }
void from_json(const nlohmann::json& j, Velocity& v) { v = Velocity{j.get<int>()}; }

void to_json(nlohmann::json& j, const Channel& c) { j = static_cast<int>(c.value()); }
void from_json(const nlohmann::json& j, Channel& c) { c = Channel{j.get<int>()}; }

}  // namespace turdus::core

namespace turdus::model {

void to_json(nlohmann::json& j, const Note& n) {
    j = nlohmann::json{
        {"pitch", n.pitch},
        {"start", n.start},
        {"length", n.length},
        {"velocity", n.velocity},
    };
}

void from_json(const nlohmann::json& j, Note& n) {
    j.at("pitch").get_to(n.pitch);
    j.at("start").get_to(n.start);
    j.at("length").get_to(n.length);
    j.at("velocity").get_to(n.velocity);
}

void to_json(nlohmann::json& j, const TimeSignature& ts) {
    j = nlohmann::json{
        {"numerator", ts.numerator()},
        {"denominator", ts.denominator()},
    };
}

void from_json(const nlohmann::json& j, TimeSignature& ts) {
    ts = TimeSignature{j.at("numerator").get<int>(), j.at("denominator").get<int>()};
}

void to_json(nlohmann::json& j, const Pattern& p) {
    auto notes = nlohmann::json::array();
    for (const auto& entry : p.notes()) {
        nlohmann::json note_json = entry.note;
        note_json["id"] = entry.id;
        notes.push_back(std::move(note_json));
    }
    j = nlohmann::json{
        {"name", p.name()},
        {"length", p.length()},
        {"default_channel", p.default_channel()},
        {"notes", std::move(notes)},
    };
}

void from_json(const nlohmann::json& j, Pattern& p) {
    Pattern result{
        j.at("name").get<std::string>(),
        j.at("length").get<core::Tick>(),
        j.at("default_channel").get<core::Channel>(),
    };
    for (const auto& note_json : j.at("notes")) {
        const auto id = note_json.at("id").get<NoteId>();
        Note note;
        from_json(note_json, note);
        result.add_note_with_id(id, note);
    }
    p = std::move(result);
}

void to_json(nlohmann::json& j, const Track& t) {
    auto patterns = nlohmann::json::array();
    for (const auto& entry : t.patterns()) {
        nlohmann::json p_json = entry.pattern;
        p_json["id"] = entry.id;
        patterns.push_back(std::move(p_json));
    }
    j = nlohmann::json{
        {"name", t.name()},
        {"port_label", t.port_label()},
        {"channel", t.channel()},
        {"muted", t.muted()},
        {"soloed", t.soloed()},
        {"transpose", t.transpose()},
        {"patterns", std::move(patterns)},
    };
}

void from_json(const nlohmann::json& j, Track& t) {
    Track result{
        j.at("name").get<std::string>(),
        j.at("port_label").get<std::string>(),
        j.at("channel").get<core::Channel>(),
    };
    result.set_muted(j.at("muted").get<bool>());
    result.set_soloed(j.at("soloed").get<bool>());
    result.set_transpose(j.at("transpose").get<int>());
    for (const auto& p_json : j.at("patterns")) {
        const auto id = p_json.at("id").get<PatternId>();
        Pattern pattern;
        from_json(p_json, pattern);
        result.add_pattern_with_id(id, std::move(pattern));
    }
    t = std::move(result);
}

void to_json(nlohmann::json& j, const PatternPlacement& p) {
    j = nlohmann::json{
        {"track_id", p.track_id},
        {"pattern_id", p.pattern_id},
        {"start", p.start},
    };
}

void from_json(const nlohmann::json& j, PatternPlacement& p) {
    j.at("track_id").get_to(p.track_id);
    j.at("pattern_id").get_to(p.pattern_id);
    j.at("start").get_to(p.start);
}

void to_json(nlohmann::json& j, const MidiPortMapping& m) {
    j = nlohmann::json{
        {"label", m.label},
        {"device_name", m.device_name},
        {"send_clock", m.send_clock},
    };
}

void from_json(const nlohmann::json& j, MidiPortMapping& m) {
    j.at("label").get_to(m.label);
    j.at("device_name").get_to(m.device_name);
    j.at("send_clock").get_to(m.send_clock);
}

void to_json(nlohmann::json& j, const LoopRegion& r) {
    j = nlohmann::json{
        {"start", r.start},
        {"end", r.end},
    };
}

void from_json(const nlohmann::json& j, LoopRegion& r) {
    j.at("start").get_to(r.start);
    j.at("end").get_to(r.end);
}

void to_json(nlohmann::json& j, const Project& p) {
    auto tracks = nlohmann::json::array();
    for (const auto& entry : p.tracks()) {
        nlohmann::json t_json = entry.track;
        t_json["id"] = entry.id;
        tracks.push_back(std::move(t_json));
    }
    j = nlohmann::json{
        {"name", p.name()},
        {"tempo", p.tempo()},
        {"time_signature", p.time_signature()},
        {"tracks", std::move(tracks)},
        {"arrangement", p.arrangement()},
        {"port_mappings", p.port_mappings()},
        {"loop", p.loop()},
    };
}

void from_json(const nlohmann::json& j, Project& p) {
    Project result;
    result.set_name(j.at("name").get<std::string>());
    result.set_tempo(j.at("tempo").get<core::Bpm>());
    result.set_time_signature(j.at("time_signature").get<TimeSignature>());

    for (const auto& t_json : j.at("tracks")) {
        const auto id = t_json.at("id").get<TrackId>();
        Track track;
        from_json(t_json, track);
        result.add_track_with_id(id, std::move(track));
    }

    for (const auto& a_json : j.at("arrangement")) {
        result.add_placement(a_json.get<PatternPlacement>());
    }

    for (const auto& m_json : j.at("port_mappings")) {
        result.add_port_mapping(m_json.get<MidiPortMapping>());
    }

    // Loop region is optional for backward compatibility with v1 schema files.
    if (j.contains("loop")) {
        result.set_loop(j.at("loop").get<LoopRegion>());
    }

    p = std::move(result);
}

}  // namespace turdus::model

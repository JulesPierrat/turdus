#include <turdus/io/ProjectIO.hpp>

#include <fstream>
#include <sstream>
#include <utility>

#include <nlohmann/json.hpp>

#include <turdus/core/Id.hpp>
#include <turdus/io/Serialization.hpp>
#include <turdus/model/Pattern.hpp>
#include <turdus/model/Track.hpp>

namespace turdus::io {

const char* describe(ProjectIOError e) noexcept {
    switch (e) {
        case ProjectIOError::None: return "ok";
        case ProjectIOError::FileNotFound: return "file not found";
        case ProjectIOError::FileNotReadable: return "file not readable";
        case ProjectIOError::FileNotWritable: return "file not writable";
        case ProjectIOError::InvalidJson: return "invalid JSON";
        case ProjectIOError::UnsupportedSchemaVersion: return "unsupported schema version";
        case ProjectIOError::MalformedData: return "malformed project data";
    }
    return "unknown error";
}

namespace {

// After loading, bump every Id counter past whatever ids the file contained, so future
// next() calls don't collide with deserialized ids.
void bump_id_counters(const model::Project& p) {
    for (const auto& te : p.tracks()) {
        core::Id<model::TrackTag>::ensure_next_at_least(te.id.raw() + 1);
        for (const auto& pe : te.track.patterns()) {
            core::Id<model::PatternTag>::ensure_next_at_least(pe.id.raw() + 1);
            for (const auto& ne : pe.pattern.notes()) {
                core::Id<model::NoteTag>::ensure_next_at_least(ne.id.raw() + 1);
            }
        }
    }
}

}  // namespace

LoadResult ProjectIO::from_json_string(std::string_view json_text) {
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(json_text);
    } catch (const nlohmann::json::parse_error& e) {
        return {std::nullopt, ProjectIOError::InvalidJson, e.what()};
    }

    if (!j.is_object()) {
        return {std::nullopt, ProjectIOError::MalformedData, "top-level value is not an object"};
    }

    const int version = j.value("schema_version", 1);
    if (version > kCurrentSchemaVersion) {
        return {std::nullopt, ProjectIOError::UnsupportedSchemaVersion,
                "schema version " + std::to_string(version) + " is newer than supported ("
                    + std::to_string(kCurrentSchemaVersion) + ")"};
    }
    // Future migrations would run here, mutating `j` from `version` up to kCurrentSchemaVersion.

    model::Project p;
    try {
        j.get_to(p);
    } catch (const nlohmann::json::exception& e) {
        return {std::nullopt, ProjectIOError::MalformedData, e.what()};
    }

    bump_id_counters(p);
    return {std::move(p), ProjectIOError::None, {}};
}

std::string ProjectIO::to_json_string(const model::Project& project) {
    nlohmann::json j = project;
    j["schema_version"] = kCurrentSchemaVersion;
    return j.dump(2);
}

LoadResult ProjectIO::load(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        return {std::nullopt, ProjectIOError::FileNotFound, path.string()};
    }
    std::ifstream file(path);
    if (!file) {
        return {std::nullopt, ProjectIOError::FileNotReadable, path.string()};
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return from_json_string(buffer.str());
}

SaveResult ProjectIO::save(const model::Project& project, const std::filesystem::path& path) {
    std::ofstream file(path);
    if (!file) {
        return {ProjectIOError::FileNotWritable, path.string()};
    }
    file << to_json_string(project) << '\n';
    if (!file) {
        return {ProjectIOError::FileNotWritable, path.string()};
    }
    return {ProjectIOError::None, {}};
}

}  // namespace turdus::io

#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include <turdus/model/Project.hpp>

namespace turdus::io {

enum class ProjectIOError {
    None,
    FileNotFound,
    FileNotReadable,
    FileNotWritable,
    InvalidJson,
    UnsupportedSchemaVersion,
    MalformedData,
};

const char* describe(ProjectIOError e) noexcept;

struct LoadResult {
    std::optional<model::Project> project;
    ProjectIOError error{ProjectIOError::None};
    std::string detail;

    bool ok() const noexcept { return error == ProjectIOError::None; }
};

struct SaveResult {
    ProjectIOError error{ProjectIOError::None};
    std::string detail;

    bool ok() const noexcept { return error == ProjectIOError::None; }
};

class ProjectIO {
public:
    static constexpr int kCurrentSchemaVersion = 1;

    // No exceptions cross this boundary — all failures surface via the result types.
    static LoadResult load(const std::filesystem::path& path);
    static SaveResult save(const model::Project& project, const std::filesystem::path& path);

    static std::string to_json_string(const model::Project& project);
    static LoadResult from_json_string(std::string_view json_text);
};

}  // namespace turdus::io

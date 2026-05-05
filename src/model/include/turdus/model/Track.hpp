#pragma once

#include <string>
#include <vector>

#include <turdus/core/Channel.hpp>
#include <turdus/core/Id.hpp>
#include <turdus/model/Pattern.hpp>

namespace turdus::model {

struct TrackTag;
using TrackId = core::Id<TrackTag>;

class Track {
public:
    static constexpr int kMinTranspose = -64;
    static constexpr int kMaxTranspose = 64;

    struct PatternEntry {
        PatternId id;
        Pattern pattern;

        bool operator==(const PatternEntry&) const = default;
    };

    Track();
    Track(std::string name, std::string port_label, core::Channel channel);

    const std::string& name() const noexcept { return name_; }
    void set_name(std::string name);

    const std::string& port_label() const noexcept { return port_label_; }
    void set_port_label(std::string label);

    core::Channel channel() const noexcept { return channel_; }
    void set_channel(core::Channel ch);

    bool muted() const noexcept { return muted_; }
    void set_muted(bool m) noexcept { muted_ = m; }

    bool soloed() const noexcept { return soloed_; }
    void set_soloed(bool s) noexcept { soloed_ = s; }

    int transpose() const noexcept { return transpose_; }
    void set_transpose(int semitones);

    const std::vector<PatternEntry>& patterns() const noexcept { return patterns_; }
    PatternId add_pattern(Pattern p);

    // Insert a pattern with a caller-supplied id (for deserialization). Returns false
    // if the id is invalid or already present on this track.
    bool add_pattern_with_id(PatternId id, Pattern p);

    bool remove_pattern(PatternId id);
    Pattern* find_pattern(PatternId id);
    const Pattern* find_pattern(PatternId id) const;

    bool operator==(const Track&) const = default;

private:
    std::string name_;
    std::string port_label_;
    core::Channel channel_;
    bool muted_{false};
    bool soloed_{false};
    int transpose_{0};
    std::vector<PatternEntry> patterns_;
};

}  // namespace turdus::model

#include <turdus/model/TimeSignature.hpp>

#include <algorithm>

namespace turdus::model {

TimeSignature::TimeSignature(int numerator, int denominator)
    : numerator_(std::clamp(numerator, kMinNumerator, kMaxNumerator)),
      denominator_(is_valid_denominator(denominator) ? denominator : 4) {}

bool TimeSignature::is_valid_denominator(int d) noexcept {
    return d == 1 || d == 2 || d == 4 || d == 8 || d == 16 || d == 32;
}

}  // namespace turdus::model

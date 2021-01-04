#pragma once

#include <array>
#include <cmath>
#include <chrono>


using FPSeconds = std::chrono::duration<double, std::chrono::seconds::period>;


// Formats durations with appropriate units for nice reading.
class FormattedDuration {
public:
    explicit FormattedDuration(FPSeconds seconds) :
        _seconds{seconds.count()}
    {}

    template<class OStream>
    friend OStream& operator<<(OStream& stream, FormattedDuration const& duration) {
        static constexpr std::array<char const*, 4> UNITS{"s", "ms", "us", "ns"};
        auto value = duration._seconds;
        unsigned i = 0;
        while (true) {
            if (std::abs(value) >= 1.0) {
                break;
            }
            else if (i + 1 < 4) {
                value *= 1000;
                ++i;
            }
            else {
                break;
            }
        }
        stream << value << UNITS.at(i);
        return stream;
    }

private:
    double _seconds;
};


inline FormattedDuration formatDuration(FPSeconds seconds) {
    return FormattedDuration{seconds};
}

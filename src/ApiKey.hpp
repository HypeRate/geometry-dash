#pragma once

#include <array>
#include <cstddef>
#include <string>

// The HypeRate API key is injected at build time (see CMakeLists.txt) and
// XOR-encoded at compile time, so it does not appear as plain text in the
// binary. This only stops casual extraction - the key must still be treated as
// public and restricted server-side (read-only, rate-limited, rotatable).
namespace api_key {
    constexpr unsigned char keyByte(std::size_t i) {
        return static_cast<unsigned char>((i * 0x9Du + 0x5Bu) ^ 0xA7u);
    }

    template <std::size_t N>
    struct Encoded {
        std::array<unsigned char, N> data{};

        consteval Encoded(char const (&str)[N]) {
            for (std::size_t i = 0; i < N; ++i) {
                data[i] = static_cast<unsigned char>(str[i]) ^ keyByte(i);
            }
        }

        std::string decode() const {
            std::string out(N - 1, '\0');
            for (std::size_t i = 0; i + 1 < N; ++i) {
                out[i] = static_cast<char>(data[i] ^ keyByte(i));
            }
            return out;
        }
    };

    inline std::string get() {
        static constexpr Encoded encoded(HYPERATE_API_KEY);
        return encoded.decode();
    }
}

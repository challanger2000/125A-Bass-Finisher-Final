#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <filesystem>
#endif

namespace HighGainGuitarFinisher::Licensing {

inline constexpr const char* kLicenseFileName =
    "125A_Bass_Finisher_V1.license";

inline constexpr std::uint64_t kExpectedLicenseHash =
    0x3c233d812634a09cULL;

inline std::uint64_t fnv1a64(
    const std::string& text) noexcept {
    std::uint64_t hash =
        14695981039346656037ULL;
    for (const unsigned char c : text) {
        hash ^= static_cast<std::uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

inline bool isLicensed() noexcept {
#if defined(_WIN32)
    HMODULE module = nullptr;
    const auto address =
        reinterpret_cast<LPCWSTR>(&isLicensed);

    if (!GetModuleHandleExW(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            address,
            &module) ||
        !module) {
        return false;
    }

    wchar_t modulePath[32768] {};
    constexpr DWORD capacity =
        static_cast<DWORD>(
            sizeof(modulePath) /
            sizeof(modulePath[0]));

    const DWORD length =
        GetModuleFileNameW(
            module,
            modulePath,
            capacity);

    if (length == 0 ||
        length >= capacity) {
        return false;
    }

    try {
        const std::filesystem::path
            binaryPath(modulePath);

        const auto contentsPath =
            binaryPath.parent_path().
                parent_path();

        std::ifstream file(
            contentsPath /
            L"Resources" /
            L"125A_Bass_Finisher_V1.license",
            std::ios::binary);

        if (!file)
            return false;

        std::string token;
        std::getline(file, token);

        if (!token.empty() &&
            token.back() == '\r') {
            token.pop_back();
        }

        return fnv1a64(token) ==
            kExpectedLicenseHash;
    } catch (...) {
        return false;
    }
#else
    return false;
#endif
}

} // namespace HighGainGuitarFinisher::Licensing

#include "AntiTamper.h"

#include <chrono>
#include <fstream>
#include <string>

#ifdef _WIN32
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #ifndef NOMINMAX
        #define NOMINMAX
    #endif
    #include <windows.h>
    #include <intrin.h>
#endif

namespace LicenseManager
{
    static std::uint64_t fnv1a(const unsigned char* data, std::size_t size, std::uint64_t hash = 0xcbf29ce484222325ULL)
    {
        for (std::size_t i = 0; i < size; ++i)
        {
            hash ^= static_cast<std::uint64_t>(data[i]);
            hash *= 0x100000001b3ULL;
        }
        return hash;
    }

    bool AntiTamper::isDebuggerPresent()
    {
#ifdef _WIN32
        if (::IsDebuggerPresent())
            return true;

        BOOL remote = FALSE;
        if (::CheckRemoteDebuggerPresent(::GetCurrentProcess(), &remote) && remote)
            return true;

        // Read the BeingDebugged flag directly from the PEB. On x64 the PEB is at
        // gs:[0x60]; the BeingDebugged byte is at offset 0x02.
    #if defined(_M_X64)
        auto peb = reinterpret_cast<const unsigned char*>(__readgsqword(0x60));
        if (peb && peb[0x02] != 0)
            return true;
    #elif defined(_M_IX86)
        auto peb = reinterpret_cast<const unsigned char*>(__readfsdword(0x30));
        if (peb && peb[0x02] != 0)
            return true;
    #endif
        return false;
#else
        return false;
#endif
    }

    bool AntiTamper::isSingleStepped(std::uint64_t thresholdMicroseconds)
    {
        auto start = std::chrono::steady_clock::now();

        // Cheap, side-effecting work the optimizer cannot elide entirely.
        volatile std::uint64_t acc = 0x1234567;
        for (int i = 0; i < 1000; ++i)
            acc = acc * 6364136223846793005ULL + 1442695040888963407ULL;

        auto end = std::chrono::steady_clock::now();
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        (void)acc;
        return static_cast<std::uint64_t>(us) > thresholdMicroseconds;
    }

    std::uint64_t AntiTamper::checksumRange(const void* begin, std::size_t size)
    {
        if (!begin || size == 0)
            return 0;
        return fnv1a(reinterpret_cast<const unsigned char*>(begin), size);
    }

    std::int64_t AntiTamper::nowUnix()
    {
        return static_cast<std::int64_t>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
    }

    bool AntiTamper::checkAndRecordTime(const std::string& stateFilePath,
                                        const std::string& obfuscationKey,
                                        std::int64_t nowUnix)
    {
        // The stored high-water mark is XOR-obfuscated with a keystream derived
        // from obfuscationKey, so the file is neither human-readable nor portable
        // to another machine (when a hardware id is used as the key).
        auto keystreamByte = [&obfuscationKey](std::size_t i) -> unsigned char {
            if (obfuscationKey.empty())
                return static_cast<unsigned char>(0xA5 + i * 31);
            unsigned char k = static_cast<unsigned char>(obfuscationKey[i % obfuscationKey.size()]);
            return static_cast<unsigned char>(k ^ (0x5A + (i * 7)));
        };

        std::int64_t stored = 0;
        {
            std::ifstream in(stateFilePath, std::ios::binary);
            if (in)
            {
                unsigned char buf[8];
                in.read(reinterpret_cast<char*>(buf), sizeof(buf));
                if (in.gcount() == sizeof(buf))
                {
                    std::uint64_t v = 0;
                    for (std::size_t i = 0; i < sizeof(buf); ++i)
                    {
                        unsigned char b = static_cast<unsigned char>(buf[i] ^ keystreamByte(i));
                        v |= static_cast<std::uint64_t>(b) << (8 * i);
                    }
                    stored = static_cast<std::int64_t>(v);
                }
            }
        }

        bool ok = (nowUnix + 60) >= stored;   // small skew tolerance
        std::int64_t highWater = (nowUnix > stored) ? nowUnix : stored;

        {
            std::ofstream out(stateFilePath, std::ios::binary | std::ios::trunc);
            if (out)
            {
                std::uint64_t v = static_cast<std::uint64_t>(highWater);
                unsigned char buf[8];
                for (std::size_t i = 0; i < sizeof(buf); ++i)
                {
                    unsigned char b = static_cast<unsigned char>((v >> (8 * i)) & 0xFF);
                    buf[i] = static_cast<unsigned char>(b ^ keystreamByte(i));
                }
                out.write(reinterpret_cast<char*>(buf), sizeof(buf));
            }
        }

        return ok;
    }

    std::uint64_t AntiTamper::guardToken(std::uint64_t seed, const std::string& context)
    {
        std::uint64_t h = fnv1a(reinterpret_cast<const unsigned char*>(context.data()), context.size(),
                                0xcbf29ce484222325ULL ^ seed);
        // Fold the live tamper state in. When clean this is a no-op; under a
        // debugger it perturbs the token so downstream users of the token break
        // without an obvious boolean to patch.
        if (isDebuggerPresent())
            h ^= 0x9e3779b97f4a7c15ULL;
        return h;
    }
}

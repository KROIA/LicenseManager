#pragma once
#include "LicenseManager_base.h"
#include <string>
#include <cstdint>

/*
    AntiTamper
    ==========
    Best-effort runtime protections that raise the cost of cracking a locally
    validated license. NONE of these are unbreakable: an attacker who fully
    controls the machine can defeat any of them given enough effort. Their value
    is to make a quick patch-and-crack noticeably harder and to give you signals
    you can react to.

    Guidelines for using these effectively:
      - Call the checks from *several* places, not one central function, so there
        is no single point to patch.
      - Prefer feeding results into computations the program needs (see
        guardToken / License::deriveSecret) instead of a lone `if (cracked)`.
      - Delay and diffuse your reaction to a detected tamper; do not fail loudly
        right where you detected it.
*/

namespace LicenseManager
{
    class LICENSE_MANAGER_API AntiTamper
    {
    public:
        // True if a user-mode debugger is attached to this process.
        // Combines several independent Windows signals.
        static bool isDebuggerPresent();

        // Simple wall-clock timing probe: runs a trivial loop and reports true
        // if it took implausibly long (single-stepping / heavy instrumentation).
        // thresholdMicroseconds: report tampering above this budget.
        static bool isSingleStepped(std::uint64_t thresholdMicroseconds = 50000);

        // FNV-1a checksum over a memory range. Use it to checksum a function or
        // code region at runtime and compare against a value captured on a known
        // good build. See the header notes on how to obtain the reference value.
        static std::uint64_t checksumRange(const void* begin, std::size_t size);

        // Anti-rollback: persists the highest timestamp ever seen (obfuscated
        // with obfuscationKey) in stateFilePath. Returns false if the supplied
        // nowUnix is earlier than the stored high-water mark (clock rolled back),
        // true otherwise. Updates the stored value to max(stored, nowUnix).
        // Pass a per-machine value (e.g. a hardware id) as obfuscationKey so the
        // state file cannot simply be copied between machines.
        static bool checkAndRecordTime(const std::string& stateFilePath,
                                       const std::string& obfuscationKey,
                                       std::int64_t nowUnix);

        // Current time as seconds since the Unix epoch (UTC).
        static std::int64_t nowUnix();

        // Combine an application secret with the live tamper state into an opaque
        // token. When the environment is clean the token equals
        //   hash(seed | context)
        // and when a debugger is detected it is deliberately different, so code
        // that *uses* the token (as a key/offset/seed) silently misbehaves under
        // analysis instead of branching on a boolean the attacker can flip.
        static std::uint64_t guardToken(std::uint64_t seed, const std::string& context);
    };
}

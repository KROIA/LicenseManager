# Hardening — how the library resists cracking

This document describes every mechanism the library uses to make an application
harder to crack, what specific attack each one raises the cost of, its
limitations, and how to use it. Read [Security.md](Security.md) first for the
threat model.

## The golden rule
A license check that runs entirely on a machine the attacker controls can **never
be made uncrackable** — only more expensive to crack. Each mechanism below is a
*layer*. None is a wall. Their value is cumulative: an attacker must defeat all
the layers you enable, and the layers are designed so that the easy attacks
(copying a file, editing a field, replacing a key, flipping a jump) are the ones
that are blocked.

## Layers at a glance

| # | Mechanism | Attack it raises the cost of | Where |
|---|-----------|------------------------------|-------|
| 1 | Full-payload digital signature | Forging or editing a license | `License::signLicense` / `check` |
| 2 | Public-key obfuscation | Replacing the embedded key; `strings` search | `EncryptedConstant` |
| 3 | Node-locking (hardware binding) | Copying one license to many machines | `HardwareId` |
| 4 | Fuzzy node-locking | False lockouts *and* trivial spoofing | `HardwareId::generatePerComponent` |
| 5 | Signed validity window | Using a time-limited license forever | `setValidFrom` / `setValidUntil` |
| 6 | Anti-rollback clock check | Turning the clock back to beat expiry | `AntiTamper::checkAndRecordTime` |
| 7 | Debugger detection | Live reverse engineering | `AntiTamper::isDebuggerPresent` |
| 8 | Timing probe | Single-stepping / instrumentation | `AntiTamper::isSingleStepped` |
| 9 | Code checksum | Patching the binary | `AntiTamper::checksumRange` |
| 10 | Derived-secret (non-boolean) check | Flipping the `if (valid)` jump | `License::deriveSecret` |
| 11 | Guard token | Bypass + patch combined | `AntiTamper::guardToken` |

---

## 1. Full-payload digital signature
**How it works.** Every license is signed with an RSA-2048 private key (SHA-256,
PKCS#1 v1.5) through the modern OpenSSL EVP interface. The application ships only
the matching public key and calls `isVerified()` / `check()` to confirm the
license was produced by the key holder and has not been altered.

The signed payload is a canonical, deterministic JSON representation that now
covers **all** trust-relevant fields — the license data, the name, the node-lock
fingerprint and the validity window — so editing *any* of them invalidates the
signature. (Earlier versions signed only the license data, leaving the name and
other fields unauthenticated.)

**Raises the cost of.** Forging a license, or editing fields (expiry, tier, name,
hardware binding) in an issued license. Without the private key this is
computationally infeasible.

**Limitations.** Does nothing against an attacker who *bypasses* the check rather
than defeating it (see layer 10), or who replaces the embedded public key (layer
2).

**Compatibility note.** Because the signed payload expanded, licenses issued by
pre-hardening versions no longer verify and must be re-generated.

---

## 2. Public-key obfuscation (compile-time string encryption)
**How it works.** The public key must live inside the binary, where a plain
constant is trivially found. `EncryptedConstant::encrypt_string()` XOR-encrypts
the key **at compile time** with a key derived from `__TIME__`; each byte is
chained with its neighbours and the random key so the plaintext never appears in
the binary. `decrypt_string()` recovers it at runtime.

```c++
constexpr auto encKey = LicenseManager::EncryptedConstant::encrypt_string(
    "-----BEGIN PUBLIC KEY-----\n" /* ... */ "-----END PUBLIC KEY-----\n");
std::string publicKey = LicenseManager::EncryptedConstant::decrypt_string(encKey);
```

**Raises the cost of.** A casual `strings`/hex-editor search for the key, and the
naive attack of overwriting the key with the attacker's own so they can sign
their own licenses.

**Limitations — important.** This is *obfuscation, not encryption*. The
decryption key is stored at the end of the ciphertext, and the algorithm is open
source, so an attacker with the source can locate the blob, dump the decrypted
key at runtime, or re-encrypt *their* key the same way and splice it in. Treat it
as a speed bump, not a lock. Never do `decrypt_string(encrypt_string(x))` in one
expression — the optimizer folds it away and the plaintext lands in the binary.

---

## 3. Node-locking (hardware binding)
**How it works.** `HardwareId` builds a stable fingerprint from hardware/OS
identifiers **you choose**, hashed with SHA-256 and an application salt. The
fingerprint is stored in the license and included in the signed payload, so a
license only validates on the machine it was issued for.

```c++
using HW = LicenseManager::HardwareId;
std::uint32_t components = HW::VolumeSerial | HW::MachineGuid | HW::CpuInfo;
std::string salt = "MyApp-2026";

// Issuer: bind the license, then sign.
license.setHardwareId(HW::generate(components, salt));
license.signLicense(privateKey);

// Client: pass the running machine's fingerprint to check().
license.check(publicKey, HW::generate(components, salt));   // -> wrongMachine if copied
```

Available components (choose per your users' stability needs — see `HardwareId.h`
for the full trade-off notes):

| Component | Stability / notes |
|-----------|-------------------|
| `VolumeSerial` | Changes if the system drive is reformatted |
| `MachineGuid`  | Stable across app reinstalls; changes on OS reinstall |
| `ComputerName` | User can rename the machine |
| `MacAddress`   | Changes with adapter / dock / VPN |
| `CpuInfo`      | Model+feature signature, **not** a per-chip serial — one factor only |
| `BiosInfo`     | Baseboard/BIOS identifiers from the registry |
| `UserName`     | Current OS user |

**Raises the cost of.** The zero-skill "crack": copying one valid `.lic` to every
machine. This is the single highest-value offline protection.

**Limitations.** Windows-only. Fingerprints can be spoofed by a determined
attacker who hooks the identifier APIs. Choosing too many volatile components can
lock out legitimate users after routine hardware changes — mitigate with layer 4.

---

## 4. Fuzzy node-locking
**How it works.** Instead of one combined hash, `generatePerComponent()` returns
one hash per component. `countMatching()` reports how many still match a stored
set, so you can accept a license when at least *K of N* components agree.

```c++
auto stored  = /* per-component hashes captured at issue time */;
auto current = HW::generatePerComponent(components, salt);
if (HW::countMatching(stored, current) >= 2) { /* accept */ }
```

**Raises the cost of.** Both false lockouts (a user swaps a NIC) *and* trivial
spoofing (one faked component is not enough to pass a K-of-N threshold).

**Limitations.** Requires storing per-component hashes alongside the license; a
lower threshold trades security for tolerance.

---

## 5. Signed validity window
**How it works.** `setValidFrom()` / `setValidUntil()` take ISO-8601 strings, are
part of the signed payload, and are enforced by `check()`, which returns
`notYetValid` or `expired`. Comparison is lexicographic, which is correct for
zero-padded ISO-8601.

**Raises the cost of.** Editing an expiry date (blocked by layer 1) and running a
time-limited or subscription license past its term.

**Limitations.** An offline app must read the wall clock, which the user
controls — pair this with layer 6.

---

## 6. Anti-rollback clock check
**How it works.** `AntiTamper::checkAndRecordTime()` persists the highest
timestamp ever seen, XOR-obfuscated with a per-machine key (pass a hardware id),
and reports `false` when the current time is earlier than that high-water mark.

```c++
std::string state = /* app data dir */ + "/.lmstate";
bool ok = LicenseManager::AntiTamper::checkAndRecordTime(
              state, hwId, LicenseManager::AntiTamper::nowUnix());
if (!ok) { /* clock rolled back */ }
```

**Raises the cost of.** Setting the system clock back to before an expiry date, or
back inside a validity window.

**Limitations.** Deleting the state file resets the high-water mark; keying it to
a hardware id stops the file being copied between machines but not deleted. Best
combined with occasional trusted-time sources if any are available.

---

## 7. Debugger detection
**How it works.** `AntiTamper::isDebuggerPresent()` combines three independent
Windows signals: `IsDebuggerPresent`, `CheckRemoteDebuggerPresent`, and a direct
read of the PEB `BeingDebugged` flag.

**Raises the cost of.** Live analysis with a user-mode debugger, which is how most
crackers locate the check.

**Limitations.** Defeatable by anti-anti-debug plugins and kernel debuggers. Most
effective when its result feeds a computation (layer 11) rather than a visible
branch.

---

## 8. Timing probe
**How it works.** `AntiTamper::isSingleStepped()` times a short, un-elidable loop
and reports tampering if it took implausibly long — the signature of
single-stepping or heavy instrumentation.

**Raises the cost of.** Step-by-step tracing and some instrumentation frameworks.

**Limitations.** Coarse; a paused-then-resumed process or a very slow machine can
false-positive, so choose the threshold conservatively.

---

## 9. Code checksum
**How it works.** `AntiTamper::checksumRange(begin, size)` computes an FNV-1a hash
over a memory range (e.g. a critical function). Compare it against a value
captured from a known-good build to detect that the code was patched.

**Raises the cost of.** Static binary patching — the classic "NOP the jump" or
"return true" edits.

**Limitations.** You must obtain the reference checksum for the exact release
build (a post-build step), and the comparison site itself can be patched. Use it
to feed a derived value, and checksum from several places.

---

## 10. Derived-secret (non-boolean) verification — the key technique
**How it works.** The most common crack is not defeating the crypto; it is
patching the one conditional that acts on the result:

```c++
if (license.isVerified(key)) { /* flip this jump == cracked */ }
```

`License::deriveSecret(publicKey, context)` returns a cryptographic value that
**only exists when the signature verifies** (it hashes the unforgeable signature
with your context). Use that value as a key or seed for something the program
genuinely needs — decrypt a resource, unlock a feature, seed a code path. If an
attacker bypasses the check by patching a branch, the derived value is wrong and
the dependent feature silently breaks. There is no boolean to flip.

```c++
std::string key = license.deriveSecret(publicKey, "premium-feature");
if (key.empty()) { /* not licensed */ }
// Better: use `key` to decrypt an encrypted resource the feature needs.
```

**Raises the cost of.** Jump/branch patching and "make it return true" — the
cheapest and most popular cracks.

**Limitations.** Only as strong as how you *use* the derived value. If you turn it
straight back into `if (!key.empty())`, you have re-introduced the single boolean.
The gain comes from making correct program behaviour *depend* on the secret.

---

## 11. Guard token
**How it works.** `AntiTamper::guardToken(seed, context)` folds the live tamper
state into an opaque 64-bit value: when the environment is clean it equals a
stable hash of `seed`+`context`; under a debugger it is deliberately different.
Use it the same way as a derived secret — as an offset, index, or key — so code
misbehaves under analysis without exposing a patchable check.

**Raises the cost of.** The combination of dynamic analysis and patching, by
removing the boolean the attacker would target.

**Limitations.** Same as layer 10 — its value depends on the token being *used*,
not compared.

---

## Putting it together — recommended usage
1. **Sign the full payload** including a **node-lock fingerprint** and a
   **validity window** (layers 1, 3, 5).
2. Ship the public key **obfuscated** (layer 2).
3. On the client, call `check()` and gate real functionality through
   **`deriveSecret()`** rather than a bare boolean (layer 10).
4. Enforce the clock with **anti-rollback** (layer 6).
5. Sprinkle **debugger/timing/checksum** probes and fold them in with
   **`guardToken()`** — from *several* places, never one central function
   (layers 7–9, 11).
6. **Diffuse the reaction**: never fail loudly right where you detected the
   problem. Let a wrong derived value cause a distant, subtle failure instead.

## What this library does *not* do
- It cannot stop a determined reverse engineer with unlimited time on hardware
  they control. That is a property of all offline licensing.
- It provides no server-side activation. If you can host a server, moving the
  trust decision online is categorically stronger than everything above.
- The anti-tamper probes are Windows-only and best-effort.

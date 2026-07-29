# Security

> For a mechanism-by-mechanism breakdown of every crack-safety layer, what attack
> each raises the cost of, and how to use it, see
> [Hardening — crack-safety methods](Hardening.md).

## Overview
* [Threat model — read this first](#threat-model--read-this-first)
* [What the library protects](#what-the-library-protects)
* [Node-locking (hardware binding)](#node-locking-hardware-binding)
* [Validity window](#validity-window)
* [Hardening the check itself](#hardening-the-check-itself)
* [Anti-tamper helpers](#anti-tamper-helpers)
* [License validation](#license-validation)
* [Public key problem](#public-key-problem)
* [Public key solution](#public-key-solution)
* [Compile time string encryption](#compile-time-string-encryption)

---
## Threat model — read this first
A license check that runs entirely on a machine the attacker controls can **never
be made uncrackable** — only more expensive to crack. Everything in this library
raises that cost; none of it is absolute. The only categorically stronger model
is online activation, where the private key and the trust decision live on a
server the attacker cannot touch. This library is designed for the **fully
offline** case, so treat every measure below as *defence in depth*, not a wall.

The single most important consequence: the classic crack is **not** forging a
license or replacing the embedded public key. It is patching the one conditional
that acts on the result of the check:

```c++
if (license.isVerified(publicKey)) { /* ... */ }   // <- flip this one jump == cracked
```

Hardening the cryptography while leaving that branch exposed is locking the
windows and leaving the door open. See
[Hardening the check itself](#hardening-the-check-itself) for how to avoid the
single-boolean pattern.

## What the library protects
* **Integrity of the whole license.** The RSA-2048 signature (SHA-256, via the
  modern OpenSSL EVP interface) now covers the entire payload — license data,
  name, node-lock fingerprint and validity window. Editing any of them
  invalidates the license. *(Older versions signed only the license data, so
  licenses issued by them must be re-generated.)*
* **Authenticity.** Only the holder of the private key can produce a license the
  embedded public key accepts.

It does **not**, by itself, stop a determined reverse engineer from patching the
binary. That is what the hardening measures raise the cost of.

## Node-locking (hardware binding)
`LicenseManager::HardwareId` builds a stable machine fingerprint from components
**you** choose, so a license file only works on the machine it was issued for.
Copying the `.lic` to another machine then fails with `wrongMachine`.

```c++
using HW = LicenseManager::HardwareId;

// Pick the components you trust to be stable for your users, plus an app salt.
std::uint32_t components = HW::VolumeSerial | HW::MachineGuid | HW::CpuInfo;
std::string   salt       = "MyApp-2026";

std::string id = HW::generate(components, salt);
license.setHardwareId(id);      // issuer side, before signing
// ... later, on the client:
license.check(publicKey, HW::generate(components, salt));
```

Available components and their trade-offs are documented in `HardwareId.h`.
Combine several so that a single hardware change does not lock a user out; for
tolerance to minor changes use `generatePerComponent()` + `countMatching()` and
accept the license when at least *K of N* components still match.

## Validity window
`setValidFrom()` / `setValidUntil()` take ISO-8601 date or datetime strings and
are part of the signed payload. `check()` enforces them and returns `notYetValid`
or `expired`. Because a fully offline app cannot trust the system clock, combine
this with the anti-rollback helper below.

## Hardening the check itself
Prefer `check()` over `isVerified()` — it returns a typed `VerificationResult`
(signature, node-lock and validity in one call). But the real win is to stop
branching on a single boolean:

* **Derive a value from the verified license and *use* it.**
  `License::deriveSecret(publicKey, context)` returns a cryptographic value that
  only exists when the signature verifies. Use it as a key/seed for something the
  program genuinely needs (decrypt a resource, unlock a feature). If an attacker
  bypasses the check by patching a jump, this value is wrong and the dependent
  feature silently breaks — there is no boolean to flip.
* **Check in several places**, not one central function.
* **Diffuse the reaction**: don't fail loudly right where you detected a problem.

## Anti-tamper helpers
`LicenseManager::AntiTamper` provides best-effort runtime signals (all Windows,
all defeatable by a determined attacker, all most useful when fed into
computations rather than an `if`):

* `isDebuggerPresent()` — combines `IsDebuggerPresent`, `CheckRemoteDebuggerPresent`
  and a direct PEB read.
* `isSingleStepped()` — timing probe for heavy instrumentation.
* `checksumRange()` — FNV-1a over a memory range to detect code patches.
* `checkAndRecordTime()` — anti-rollback: persists an obfuscated high-water-mark
  timestamp (keyed by a per-machine value) and reports clock roll-back.
* `guardToken()` — folds live tamper state into an opaque token so downstream code
  misbehaves under analysis instead of exposing a patchable branch.

## License validation
Each license is signed with a private key. Using the public key, the signed
payload and the signature, OpenSSL checks that the data matches the signature —
i.e. that nothing was modified after signing.

## Public key problem
Since the public key is needed to check a license, it must ship inside the
application. Stored as a plain constant it is trivially found in the binary:

```c++
const std::string publicKey =
        "-----BEGIN PUBLIC KEY-----\n"
        "....\n"
        "-----END PUBLIC KEY-----\n";
```

An attacker could replace it with *their* public key and sign their own licenses.

> ⚠️ **Caveat:** obfuscating the public key only defends against key-replacement.
> It does nothing against the far more common attack of patching the check
> branch, and because this library's obfuscation algorithm is open source, an
> attacker with the source can re-encrypt their own key the same way. Treat it as
> obfuscation, not protection, and rely on [hardening the check
> itself](#hardening-the-check-itself) instead.

## Public key solution
Store the key with the compile-time XOR obfuscation so the plaintext is not
visible in the binary:

```c++
constexpr auto encryptedPublicKey = LicenseManager::EncryptedConstant::encrypt_string(
    "-----BEGIN PUBLIC KEY-----\n"
    "....\n"
    "-----END PUBLIC KEY-----\n");

std::string publicKey = LicenseManager::EncryptedConstant::decrypt_string(encryptedPublicKey);
```

The string is encrypted at compile time; only the encrypted bytes are stored.

> ⚠️ **Do not do this:** the compiler will fold away a decrypt-of-encrypt and the
> plaintext will end up in the binary anyway.
> ```c++
> std::string message = decrypt_string(encrypt_string("Secret message"));
> ```

## Compile time string encryption
The random key is generated from `__TIME__` and appended to the encrypted string.
Each character is XORed with the key and its neighbours, so the plaintext can only
be recovered if the exact location of the message end is known. This raises the
bar for a casual `strings`-style search but, as noted above, is obfuscation only.

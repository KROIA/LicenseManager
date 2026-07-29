#pragma once

#include "UnitTest.h"
#include "LicenseManager.h"
#include <map>
#include <string>

/*
    Tests for the hardening features:
      - composable hardware fingerprint
      - full-payload signing (name / node-lock / validity are tamper-protected)
      - License::check() result codes (node-lock, validity window)
      - deriveSecret anti-patch primitive
*/

class TST_LicenseHardening : public UnitTest::Test
{
    TEST_CLASS(TST_LicenseHardening)
public:
    TST_LicenseHardening()
        : Test("TST_LicenseHardening")
    {
        ADD_TEST(TST_LicenseHardening::hardwareIdStableAndComposable);
        ADD_TEST(TST_LicenseHardening::signAndCheckValid);
        ADD_TEST(TST_LicenseHardening::nodeLockRejectsOtherMachine);
        ADD_TEST(TST_LicenseHardening::validityWindow);
        ADD_TEST(TST_LicenseHardening::tamperInvalidatesSignature);
        ADD_TEST(TST_LicenseHardening::deriveSecret);
    }

private:
    using HW = LicenseManager::HardwareId;
    using VR = LicenseManager::License::VerificationResult;

    static std::uint32_t components()
    {
        return HW::VolumeSerial | HW::MachineGuid | HW::CpuInfo;
    }

    TEST_FUNCTION(hardwareIdStableAndComposable)
    {
        TEST_START;
        std::string a = HW::generate(components(), "salt-A");
        std::string b = HW::generate(components(), "salt-A");
        std::string c = HW::generate(components(), "salt-B");

        TEST_MESSAGE("id(salt-A) = " + a);
        TEST_MESSAGE("id(salt-B) = " + c);

        TEST_ASSERT_M(!a.empty(), "hardware id should not be empty on a real machine");
        TEST_ASSERT_M(a == b, "hardware id must be stable across calls");
        TEST_ASSERT_M(a != c, "different salt must produce a different id");
    }

    TEST_FUNCTION(signAndCheckValid)
    {
        TEST_START;
        std::string priv = LicenseManager::License::generatePrivateKey();
        std::string pub = LicenseManager::License::getPublicKeyFromPrivateKey(priv);
        TEST_ASSERT_M(!priv.empty() && !pub.empty(), "key generation failed");

        std::string hwId = HW::generate(components(), "salt");

        LicenseManager::License lic;
        std::map<std::string, std::string> data;
        data["name"] = "John Doe";
        lic.setLicenseData(data);
        lic.setName("John Doe");
        lic.setHardwareId(hwId);
        lic.setValidFrom("2000-01-01");
        lic.setValidUntil("2999-12-31");
        TEST_ASSERT_M(lic.signLicense(priv), "signing failed");

        VR r = lic.check(pub, hwId);
        TEST_MESSAGE("check() = " + LicenseManager::License::toString(r));
        TEST_ASSERT(r == VR::valid);
        TEST_ASSERT_M(lic.isVerified(pub), "signature-only verification should also pass");
    }

    TEST_FUNCTION(nodeLockRejectsOtherMachine)
    {
        TEST_START;
        std::string priv = LicenseManager::License::generatePrivateKey();
        std::string pub = LicenseManager::License::getPublicKeyFromPrivateKey(priv);

        LicenseManager::License lic;
        lic.setHardwareId("0000000000000000000000000000000000000000000000000000000000000000");
        TEST_ASSERT(lic.signLicense(priv));

        // Running machine fingerprint differs from the bound one.
        VR r = lic.check(pub, HW::generate(components(), "salt"));
        TEST_ASSERT(r == VR::wrongMachine);

        // Empty running fingerprint must fail closed too.
        VR r2 = lic.check(pub, std::string());
        TEST_ASSERT(r2 == VR::wrongMachine);
    }

    TEST_FUNCTION(validityWindow)
    {
        TEST_START;
        std::string priv = LicenseManager::License::generatePrivateKey();
        std::string pub = LicenseManager::License::getPublicKeyFromPrivateKey(priv);

        LicenseManager::License lic;   // not node-locked
        lic.setValidFrom("2025-01-01");
        lic.setValidUntil("2025-12-31");
        TEST_ASSERT(lic.signLicense(priv));

        TEST_ASSERT(lic.check(pub, std::string(), "2024-06-01") == VR::notYetValid);
        TEST_ASSERT(lic.check(pub, std::string(), "2026-06-01") == VR::expired);
        TEST_ASSERT(lic.check(pub, std::string(), "2025-06-01") == VR::valid);
    }

    TEST_FUNCTION(tamperInvalidatesSignature)
    {
        TEST_START;
        std::string priv = LicenseManager::License::generatePrivateKey();
        std::string pub = LicenseManager::License::getPublicKeyFromPrivateKey(priv);

        LicenseManager::License lic;
        std::map<std::string, std::string> data;
        data["tier"] = "basic";
        lic.setLicenseData(data);
        lic.setName("Alice");
        TEST_ASSERT(lic.signLicense(priv));
        TEST_ASSERT(lic.isVerified(pub));

        // Editing the (now signed) name must break verification.
        lic.setName("Attacker");
        TEST_ASSERT_M(lic.check(pub, std::string()) == VR::invalidSignature,
                      "modifying the name must invalidate the signature");

        // Editing license data must break verification too.
        lic.setName("Alice");
        data["tier"] = "enterprise";
        lic.setLicenseData(data);
        TEST_ASSERT(lic.check(pub, std::string()) == VR::invalidSignature);
    }

    TEST_FUNCTION(deriveSecret)
    {
        TEST_START;
        std::string priv = LicenseManager::License::generatePrivateKey();
        std::string pub = LicenseManager::License::getPublicKeyFromPrivateKey(priv);

        std::string otherPriv = LicenseManager::License::generatePrivateKey();
        std::string otherPub = LicenseManager::License::getPublicKeyFromPrivateKey(otherPriv);

        LicenseManager::License lic;
        std::map<std::string, std::string> data;
        data["name"] = "Bob";
        lic.setLicenseData(data);
        TEST_ASSERT(lic.signLicense(priv));

        std::string good = lic.deriveSecret(pub, "context");
        std::string bad = lic.deriveSecret(otherPub, "context");
        std::string good2 = lic.deriveSecret(pub, "context");
        std::string differentCtx = lic.deriveSecret(pub, "other-context");

        TEST_ASSERT_M(!good.empty(), "valid license should yield a secret");
        TEST_ASSERT_M(bad.empty(), "wrong public key must yield no secret");
        TEST_ASSERT_M(good == good2, "derived secret must be deterministic");
        TEST_ASSERT_M(good != differentCtx, "different context must change the secret");
    }
};

TEST_INSTANTIATE(TST_LicenseHardening);

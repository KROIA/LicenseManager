#include <QCoreApplication>
#include <iostream>
#include "LicenseManager.h"
#include <QDate>
#include <array>
#include <type_traits>
#include <utility>
#include <cstdint>

void createKeys();
void createLicense();
void validateLicense();
void correctImplementationInYourApplication();

std::string s_publicKey;

// Choose which hardware components make up the node-lock fingerprint and an
// application-specific salt. The SAME selection and salt must be used when the
// license is created (issuer side) and when it is validated (client side).
static const std::uint32_t s_hwComponents =
	LicenseManager::HardwareId::VolumeSerial |
	LicenseManager::HardwareId::MachineGuid  |
	LicenseManager::HardwareId::CpuInfo;
static const std::string s_hwSalt = "LibraryExample-2026";

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

	Log::UI::createConsoleView(Log::UI::ConsoleViewType::nativeConsoleView);
	Log::UI::getConsoleView<Log::UI::NativeConsoleView>()->show();

	//createKeys();
	//createLicense();
	//validateLicense();
	correctImplementationInYourApplication();

	LicenseManager::Logger::logInfo("done");
	int ret = 0;
	ret = app.exec();
	return ret;
}

void createKeys()
{
	LicenseManager::Logger::logInfo("Generating keys");

	// Generate a random private key
	std::string privateKey = LicenseManager::License::generatePrivateKey();
	LicenseManager::Logger::logInfo("Private Key:\n" + privateKey);

	// Get the public key from the private key
	std::string publicKey = LicenseManager::License::getPublicKeyFromPrivateKey(privateKey);
	LicenseManager::Logger::logInfo("Public Key:\n" + publicKey);
}
void createLicense()
{
	LicenseManager::Logger::logInfo("Creating license");

	// Do not share/store/hardcode the private key in your application!
	std::string privateKey = LicenseManager::License::generatePrivateKey();
	s_publicKey = LicenseManager::License::getPublicKeyFromPrivateKey(privateKey);

	LicenseManager::Logger::logInfo("Populating license data");
	std::map<std::string, std::string> licenseData;
	licenseData["name"]       = "John Doe";
	licenseData["expiryDate"] = "2025-01-01";

	// Create the license
	LicenseManager::License license;

	// Set the license data
	license.setLicenseData(licenseData);

	// Node-lock the license to THIS machine (the issuer would instead use the
	// hardware id sent by the customer during activation). An empty fingerprint
	// would mean "not node-locked".
	std::string hwId = LicenseManager::HardwareId::generate(s_hwComponents, s_hwSalt);
	license.setHardwareId(hwId);
	LicenseManager::Logger::logInfo("Bound to hardware id: " + hwId);

	// Optional validity window (ISO-8601). Empty bounds mean "unbounded".
	license.setValidFrom("2025-01-01");
	license.setValidUntil("2026-12-31");

	// Sign the license using the private key. Everything above (data, name,
	// hardware id and validity window) is covered by the signature.
	license.signLicense(privateKey);
	LicenseManager::Logger::logInfo("License signature:\n"+license.getSignature());

	LicenseManager::Logger::logInfo("Saving license to file");
	license.saveToFile("license.lic");
}
void validateLicense()
{
	LicenseManager::Logger::logInfo("Validating license");

	LicenseManager::License license;
	
	// Load the license from file
	license.loadFromFile("license.lic");

	// Verify the license
	if (license.isVerified(s_publicKey))
	{
		// The license data can be trusted
		LicenseManager::Logger::logInfo("License is verified");

		// Get the license data
		std::map<std::string, std::string> licenseData = license.getLicenseData();
		
		// Check the license data and take appropriate action
		// ...
		const auto& dateIt = licenseData.find("expiryDate");
		if (dateIt != licenseData.end())
		{
			std::string date = dateIt->second;
			if (date > "ThisDate")
			{
				// License expired...
			}
		}
	}
	else
	{
		// If the user does modify the "licenseData" or the "signature" in the license file, the license will not be verifieds
		// In this case, the license can't be trusted
		LicenseManager::Logger::logError("License is not verified");
	}
}

void correctImplementationInYourApplication()
{
	// The public key would normally be stored as constant in your application
	// Do not store the public key as const string in your application, use an encrypted form.
	// If you store the public key without encryption, the user can easily modify the public key in the binary and create a fake license

	// Use an binary encrypted public key
	constexpr auto encryptedPublicKey = LicenseManager::EncryptedConstant::encrypt_string(
		"-----BEGIN PUBLIC KEY-----\n"
		"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA2bHIM+QgoZayZLqwTwfr\n"
		"tEKd5/gXZ66j6eMqSvLN+3TiXWjHoZkLZR3ps8Ceh0bgNC2s45iHBlBcF+hQHA74\n"
		"LheXixwLeaafA4qv+qY9lvBHEbBiTHiTHNZ9XGjtadvdkvfbHCzQBG+rBLUg0kP4\n"
		"WXeHRA+ENe6qdlM6T9tPsErcqQrgWyTKB8VDBr5b6ve7TPu+2nuX7K4+rJmzh9fU\n"
		"1INoQStMwTa6T2O4dTN34+ejn03EkogcU2v6fWjr9OgCTOWgHoPRssBPYk0P6UPR\n"
		"iBOE/pRifAEvW+cR6tt8Ra+yKJd3o4MKWxMpyE/niL2Zq31YmHh3n7tT5+FdJrGf\n"
		"awIDAQAB\n"
		"-----END PUBLIC KEY-----\n");

	LicenseManager::License license;

	// Load the license from file
	license.loadFromFile("license.lic");

	// Use the decrypt function to get the public key
	std::string decryptedPublicKey = LicenseManager::EncryptedConstant::decrypt_string(encryptedPublicKey);

	// Fingerprint of the machine we are running on, using the same selection and
	// salt that were used when the license was issued.
	std::string currentHwId = LicenseManager::HardwareId::generate(s_hwComponents, s_hwSalt);

	// Full check: signature + node-lock + validity window. Prefer this over the
	// signature-only isVerified().
	using VR = LicenseManager::License::VerificationResult;
	VR result = license.check(decryptedPublicKey, currentHwId);

	LicenseManager::Logger::logInfo("License check: " + LicenseManager::License::toString(result));

	if (result == VR::valid)
	{
		// Advanced anti-patch pattern: instead of trusting a boolean that an
		// attacker can flip, derive a value FROM the verified license and use it
		// where the program genuinely needs it. If verification is bypassed, this
		// value is wrong and the dependent feature silently breaks.
		std::string featureKey = license.deriveSecret(decryptedPublicKey, "premium-feature-unlock");

		// Fold in the live anti-tamper state so the derived value is also wrong
		// under a debugger. Call such checks from several places, not just here.
		std::uint64_t guard = LicenseManager::AntiTamper::guardToken(
			LicenseManager::AntiTamper::checksumRange(featureKey.data(), featureKey.size()),
			"premium-feature-unlock");

		LicenseManager::Logger::logInfo("License valid. Feature key (use as decryption key/seed): " + featureKey);
		LicenseManager::Logger::logInfo("Guard token: " + std::to_string(guard));
	}
	else
	{
		// The license data can't be trusted. Do not just log-and-continue in a
		// real app; make the failure feed into the derived-secret path above.
		LicenseManager::Logger::logError("License not valid: " + LicenseManager::License::toString(result));
	}
}
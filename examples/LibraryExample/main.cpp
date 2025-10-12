#include <QCoreApplication>
#include <iostream>
#include "LicenseManager.h"
#include <QDate>
#include <array>
#include <type_traits>
#include <utility>

void createKeys();
void createLicense();
void validateLicense();
void correctImplementationInYourApplication();

std::string s_publicKey;

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

	// Sign the license using the private key
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

	// Verify the license
	if (license.isVerified(decryptedPublicKey))
	{
		// Readout the license data
		LicenseManager::Logger::logInfo("License verified");
	}
	else
	{
		LicenseManager::Logger::logInfo("License not verified");
		// The license is not verified
		// The license data can't be trusted
		// In this example code, the license is not verified because the public key is not the same as the one used to sign the license
	}
}
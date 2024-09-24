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


	std::string t = LicenseManager::EncryptedConstant::decrypt_string(LicenseManager::EncryptedConstant::encrypt_string("TEST"));
	createKeys();
	createLicense();
	validateLicense();
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
		"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1o/tXYhx89lPvwDgePsp\n"
		"6kNyMpUdnZqsrr+9lzbCQ4EVtCjIpbv+sKP5sZ6REi9/dshH4xV/Gbh4RhbdwrC9\n"
		"EtlIuss4zagfpYnuRFIrVDIMV5VDmyU0WOzC75XAa/65z8iXSj8aZCCsq0FJ9y6y\n"
		"5s4VO/wfuZ+DjCB10KGijzXjcxnULKZdhSILq1srTUzw433c+fag3A+l7hrHNaKO\n"
		"rbOX9MrjTuoqNt4qNsIw97NP+Ptu+of4Dl0S89II+fXxATBJLbSKMOlOt4d20YOb\n"
		"SZetkHpCNRNjejBMuO9k5+Uec1v/Ukjolwr3OzkxnBEpxtIm2fwKRP++LPxk4B42\n"
		"RQIDAQAB\n"
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
	}
	else
	{
		// The license is not verified
		// The license data can't be trusted
		// In this example code, the license is not verified because the public key is not the same as the one used to sign the license
	}
}
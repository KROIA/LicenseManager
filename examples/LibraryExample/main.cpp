#include <QCoreApplication>
#include <iostream>
#include "LicenseManager.h"
#include <QDate>



#define CREATE_LICENSE

void createLicense();
void loadLicense();


int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

	Log::UI::createConsoleView(Log::UI::ConsoleViewType::nativeConsoleView);

#ifdef CREATE_LICENSE
	createLicense();
#else
	loadLicense();
#endif

	LicenseManager::Logger::logInfo("done");
	int ret = 0;
	ret = app.exec();
	return ret;
}

void createLicense()
{
	/*
	LicenseManager::Logger::logInfo("Generating private key");
	std::string privateKey = LicenseManager::License::generatePrivateKey();
	std::string publicKey = LicenseManager::License::getPublicKeyFromPrivateKey(privateKey);
	LicenseManager::License::savePrivateKeyToFile(privateKey, "private.pem");
	LicenseManager::License::savePublicKeyToFile(publicKey, "public.pem");*/

	std::string privateKey;
	if(!LicenseManager::License::loadPrivateKeyFromFile("private.pem", privateKey))
	{ 
		LicenseManager::Logger::logInfo("Generating private key");
		privateKey = LicenseManager::License::generatePrivateKey();
		std::string publicKey = LicenseManager::License::getPublicKeyFromPrivateKey(privateKey);
		LicenseManager::License::savePrivateKeyToFile(privateKey, "private.pem");
		LicenseManager::License::savePublicKeyToFile(publicKey, "public.pem");
	}

	LicenseManager::Logger::logInfo("Populating license data");
	std::map<std::string, std::string> licenseData;
	licenseData["name"] = "John Doe";
	licenseData["expiryDate"] = "2025-01-01";

	LicenseManager::License license;
	license.setLicenseData(licenseData);
	license.signLicense(privateKey);

	LicenseManager::Logger::logInfo("Saving license to file");
	license.saveToFile("license.lic");
}
void loadLicense()
{
	
	/*std::string publicKey =
		"-----BEGIN PUBLIC KEY-----\n"
		"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA1o/tXYhx89lPvwDgePsp\n"
		"6kNyMpUdnZqsrr+9lzbCQ4EVtCjIpbv+sKP5sZ6REi9/dshH4xV/Gbh4RhbdwrC9\n"
		"EtlIuss4zagfpYnuRFIrVDIMV5VDmyU0WOzC75XAa/65z8iXSj8aZCCsq0FJ9y6y\n"
		"5s4VO/wfuZ+DjCB10KGijzXjcxnULKZdhSILq1srTUzw433c+fag3A+l7hrHNaKO\n"
		"rbOX9MrjTuoqNt4qNsIw97NP+Ptu+of4Dl0S89II+fXxATBJLbSKMOlOt4d20YOb\n"
		"SZetkHpCNRNjejBMuO9k5+Uec1v/Ukjolwr3OzkxnBEpxtIm2fwKRP++LPxk4B42\n"
		"RQIDAQAB\n"
		"-----END PUBLIC KEY-----\n";
	*/

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
	license.loadFromFile("license.lic");
	LicenseManager::Logger::logInfo("Verifying license");
	LicenseManager::Logger::logInfo("Public Key:\n"+LicenseManager::EncryptedConstant::decrypt_string(encryptedPublicKey));
	if (license.isVerified(LicenseManager::EncryptedConstant::decrypt_string(encryptedPublicKey)))
	{
		LicenseManager::Logger::logInfo("License is verified");
	}
	else
	{
		LicenseManager::Logger::logError("License is not verified");
	}
}
#include <QCoreApplication>
#include <iostream>
#include "LicenseManager.h"
#include <QDate>



//#define CREATE_LICENSE

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
	LicenseManager::Logger::logInfo("Generating private key");
	std::string privateKey = LicenseManager::License::generatePrivateKey();
	std::string publicKey = LicenseManager::License::getPublicKeyFromPrivateKey(privateKey);
	LicenseManager::License::savePrivateKeyToFile(privateKey, "private.pem");
	LicenseManager::License::savePublicKeyToFile(publicKey, "public.pem");

	LicenseManager::Logger::logInfo("Populating license data");
	std::map<std::string, std::string> licenseData;
	licenseData["name"] = "John Doe";
	licenseData["email"] = "mail";
	licenseData["company"] = "company";
	licenseData["date"] = QDate::currentDate().toString("yyyy-MM-dd").toStdString();

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
		"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxFGAg2LApG0bMnfIY9Fv\n"
		"0j04dALB2VlPXd0EIHf9hDrnfOBszpDuI240GYCLyuIfcAh2wLbXUFoCEi3oBL8g\n"
		"9WW533cQ6aKN95lXhFr9m9VQOxeJ1ypsDcd8DYEF+dmDYfCz1VUoz9TyaWDC5mKu\n"
		"hdjebNL7OrYQnkSxdaT1p4ZZlrWGThKTlvgIGVyNM1wa6hznKOZlEzhhtd3p4sOg\n"
		"tFk+f3WCedtoKcTTm7RrMMm5OrtfUKCsCnY90JTQyNwgX0gMphToPd3MdDHEfyzR\n"
		"wJK9cpR0kEjb5Wpn9osGXiVbhgVOafDkdIgihIxR07L5Sr98Kc7lFJEknb/oUhzl\n"
		"OQIDAQAB\n"
		"-----END PUBLIC KEY----- \n";
	*/

	// Use an binary encrypted public key
	constexpr auto encryptedPublicKey = LicenseManager::EncryptedConstant::encrypt_string(
		"-----BEGIN PUBLIC KEY-----\n"
		"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAxFGAg2LApG0bMnfIY9Fv\n"
		"0j04dALB2VlPXd0EIHf9hDrnfOBszpDuI240GYCLyuIfcAh2wLbXUFoCEi3oBL8g\n"
		"9WW533cQ6aKN95lXhFr9m9VQOxeJ1ypsDcd8DYEF+dmDYfCz1VUoz9TyaWDC5mKu\n"
		"hdjebNL7OrYQnkSxdaT1p4ZZlrWGThKTlvgIGVyNM1wa6hznKOZlEzhhtd3p4sOg\n"
		"tFk+f3WCedtoKcTTm7RrMMm5OrtfUKCsCnY90JTQyNwgX0gMphToPd3MdDHEfyzR\n"
		"wJK9cpR0kEjb5Wpn9osGXiVbhgVOafDkdIgihIxR07L5Sr98Kc7lFJEknb/oUhzl\n"
		"OQIDAQAB\n"
		"-----END PUBLIC KEY----- \n");

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
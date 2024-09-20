#pragma once
#include "LicenseManager_base.h"
#include <string>
#include <map>



namespace LicenseManager
{
	class LICENSE_MANAGER_EXPORT License
	{
	public:
		License();
		License(const License& other);

		~License();

		License& operator=(const License& other);
		bool operator==(const License& other) const;
		bool operator!=(const License& other) const;

		void setLicenseData(const std::map<std::string, std::string>& licenseData)
		{
			m_licenseData = licenseData;
		}
		const std::map<std::string, std::string>& getLicenseData() const
		{
			return m_licenseData;
		}

		void setSignature(const std::string& signature)
		{
			m_signature = signature;
		}
		const std::string &getSignature() const
		{
			return m_signature;
		}

		bool saveToFile(const std::string& filename) const;
		bool loadFromFile(const std::string& filename);

		bool isVerified(const std::string &publicKey) const;

		bool signLicense(const std::string& privateKey);
		bool signLicenseFromFile(const std::string& privateKeyFile);

		static std::string generatePrivateKey();
		static std::string getPublicKeyFromPrivateKey(const std::string& privateKeyPEM);
		static std::string signMessage(const std::string& privateKey, const std::string& message);
		static bool savePublicKeyToFile(const std::string& publicKey, const std::string& filename);
		static bool savePrivateKeyToFile(const std::string& privateKey, const std::string& filename);
		static bool loadPublicKeyFromFile(const std::string& filename, std::string& publicKey);
		static bool loadPrivateKeyFromFile(const std::string& filename, std::string& privateKey);


	private:
		static bool writeToFile(const std::string& filename, const std::string& data);
		static bool readFromFile(const std::string& filename, std::string& data);

		std::string getDataString() const;

		std::string m_signature;

		std::map<std::string, std::string> m_licenseData;		
	};
}
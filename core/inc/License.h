#pragma once
#include "LicenseManager_base.h"
#include <string>
#include <map>
#include "EncryptedConstant.h"


namespace LicenseManager
{
	class LICENSE_MANAGER_API License
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

		void setName(const std::string& name)
		{
			m_nameChangedSinceLastSave = true;
			m_name = name;
		}
		const std::string& getName() const
		{
			return m_name;
		}

		bool saveToFile(const std::string& filePath) const;
		bool loadFromFile(const std::string& filePath);

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

		std::string m_name;
		std::map<std::string, std::string> m_licenseData;		

		bool m_nameChangedSinceLastSave = false;
		std::string m_loadedPath;


	// Encrypted strings
	// 
	// 
	
		// Store encrypted strings
		struct EncryptedStrings
		{
			struct JsonKeys
			{
				static constexpr auto version = EncryptedConstant::encrypt_string("version");
				static constexpr auto licenseData = EncryptedConstant::encrypt_string("licenseData");
				static constexpr auto signature = EncryptedConstant::encrypt_string("signature");
				static constexpr auto libraryInfo = EncryptedConstant::encrypt_string("libraryInfo");
				static constexpr auto name = EncryptedConstant::encrypt_string("name");
			};
			struct Messages
			{
				static constexpr auto errReadingFromFile = EncryptedConstant::encrypt_string("Error reading from file: ");
				static constexpr auto errOpenSSL = EncryptedConstant::encrypt_string("OpenSSL error: ");
				static constexpr auto errGettingRSAKeypair = EncryptedConstant::encrypt_string("Error generating RSA keypair!");
				static constexpr auto errDuringEncr = EncryptedConstant::encrypt_string("Error during encryption!");
				static constexpr auto errDuringDecr = EncryptedConstant::encrypt_string("Error during decryption!");
				static constexpr auto errConvPrivToStr = EncryptedConstant::encrypt_string("Error converting private key to string!");
				static constexpr auto errConvPubToStr = EncryptedConstant::encrypt_string("Error converting public key to string!");
				static constexpr auto errInvalidPrivKey = EncryptedConstant::encrypt_string("Invalid private key!");
				static constexpr auto errPubFromPriv = EncryptedConstant::encrypt_string("Error generating public key from private key!");
				static constexpr auto errLoadPriv = EncryptedConstant::encrypt_string("Error loading private key from string!");
				static constexpr auto errLoadPub = EncryptedConstant::encrypt_string("Error loading public key from string!");
				static constexpr auto errSign = EncryptedConstant::encrypt_string("Error signing message!");
				static constexpr auto errSigVerFail = EncryptedConstant::encrypt_string("Signature verification failed!");

			};
			static constexpr auto s = EncryptedConstant::encrypt_string("   ");
		};
	public:
		struct DecryptedStrings
		{
			struct JsonKeys
			{
				std::string version;
				std::string licenseData;
				std::string signature;
				std::string libraryInfo;
				std::string name;
			};
			struct Messages
			{
				std::string errReadingFromFile;
				std::string errOpenSSL;
				std::string errGettingRSAKeypair;
				std::string errDuringEncr;
				std::string errDuringDecr;
				std::string errConvPrivToStr;
				std::string errConvPubToStr;
				std::string errInvalidPrivKey;
				std::string errPubFromPriv;
				std::string errLoadPriv;
				std::string errLoadPub;
				std::string errSign;
				std::string errSigVerFail;
			};
			JsonKeys jsonKeys;
			Messages messages;

			void decrypt()
			{
				jsonKeys.version = EncryptedConstant::decrypt_string(EncryptedStrings::JsonKeys::version);
				jsonKeys.licenseData = EncryptedConstant::decrypt_string(EncryptedStrings::JsonKeys::licenseData);
				jsonKeys.signature = EncryptedConstant::decrypt_string(EncryptedStrings::JsonKeys::signature);
				jsonKeys.libraryInfo = EncryptedConstant::decrypt_string(EncryptedStrings::JsonKeys::libraryInfo);
				jsonKeys.name = EncryptedConstant::decrypt_string(EncryptedStrings::JsonKeys::name);

				messages.errReadingFromFile = EncryptedConstant::decrypt_string(EncryptedStrings::Messages::errReadingFromFile);
				messages.errOpenSSL = EncryptedConstant::decrypt_string(EncryptedStrings::Messages::errOpenSSL);
				messages.errGettingRSAKeypair = EncryptedConstant::decrypt_string(EncryptedStrings::Messages::errGettingRSAKeypair);
				messages.errDuringEncr = EncryptedConstant::decrypt_string(EncryptedStrings::Messages::errDuringEncr);
				messages.errDuringDecr = EncryptedConstant::decrypt_string(EncryptedStrings::Messages::errDuringDecr);
				messages.errConvPrivToStr = EncryptedConstant::decrypt_string(EncryptedStrings::Messages::errConvPrivToStr);
				messages.errConvPubToStr = EncryptedConstant::decrypt_string(EncryptedStrings::Messages::errConvPubToStr);
				messages.errInvalidPrivKey = EncryptedConstant::decrypt_string(EncryptedStrings::Messages::errInvalidPrivKey);
				messages.errPubFromPriv = EncryptedConstant::decrypt_string(EncryptedStrings::Messages::errPubFromPriv);
				messages.errLoadPriv = EncryptedConstant::decrypt_string(EncryptedStrings::Messages::errLoadPriv);
				messages.errLoadPub = EncryptedConstant::decrypt_string(EncryptedStrings::Messages::errLoadPub);
				messages.errSign = EncryptedConstant::decrypt_string(EncryptedStrings::Messages::errSign);
				messages.errSigVerFail = EncryptedConstant::decrypt_string(EncryptedStrings::Messages::errSigVerFail);
			}
		};

		const static DecryptedStrings &decryptedStrings();
	};
}
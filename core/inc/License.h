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
		// Result of a full license check. Only `valid` means the license may be
		// trusted; every other value is a distinct failure reason.
		enum class VerificationResult
		{
			valid = 0,
			invalidSignature,   // signature does not match the signed payload
			wrongMachine,       // node-locked license, running on another machine
			notYetValid,        // current time is before validFrom
			expired,            // current time is after validUntil
			clockTampering,     // system clock appears to have been rolled back
			malformed           // required fields missing / unparseable
		};

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

		// --- Node-locking -----------------------------------------------------
		// Bind the license to a machine fingerprint (see HardwareId). An empty
		// value means the license is not node-locked. The value is part of the
		// signed payload, so it cannot be edited without invalidating the license.
		void setHardwareId(const std::string& hardwareId)
		{
			m_hardwareId = hardwareId;
		}
		const std::string& getHardwareId() const
		{
			return m_hardwareId;
		}
		bool isNodeLocked() const
		{
			return !m_hardwareId.empty();
		}

		// --- Validity window --------------------------------------------------
		// ISO-8601 date ("YYYY-MM-DD") or datetime strings. Empty means unbounded.
		// Both bounds are part of the signed payload.
		void setValidFrom(const std::string& iso)
		{
			m_validFrom = iso;
		}
		const std::string& getValidFrom() const
		{
			return m_validFrom;
		}
		void setValidUntil(const std::string& iso)
		{
			m_validUntil = iso;
		}
		const std::string& getValidUntil() const
		{
			return m_validUntil;
		}

		bool saveToFile(const std::string& filePath) const;
		bool loadFromFile(const std::string& filePath);

		// Signature-only verification (kept for backwards compatibility). Prefer
		// check(), which also enforces node-locking and the validity window.
		bool isVerified(const std::string &publicKey) const;

		// Full verification: signature + node-lock + validity window.
		//   currentHardwareId : fingerprint of the running machine. If empty and
		//                       the license is node-locked, the result is
		//                       wrongMachine (fail closed).
		//   nowIso            : current time as an ISO string. If empty the system
		//                       clock is used.
		VerificationResult check(const std::string& publicKey,
								 const std::string& currentHardwareId = std::string(),
								 const std::string& nowIso = std::string()) const;

		// Human-readable name of a VerificationResult (for logging/UI).
		static std::string toString(VerificationResult result);

		// Anti-patch primitive: returns a value cryptographically derived from the
		// *verified* license (a hash over the signature and `context`). Returns an
		// empty string when the signature does not verify. Use the result as a key
		// or seed for something the application genuinely needs (decrypting a
		// resource, enabling a feature), so that skipping the check by patching a
		// branch does not yield the correct value.
		std::string deriveSecret(const std::string& publicKey, const std::string& context) const;

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

		std::string m_hardwareId;   // node-lock fingerprint ("" = not locked)
		std::string m_validFrom;    // ISO date/datetime ("" = no lower bound)
		std::string m_validUntil;   // ISO date/datetime ("" = no upper bound)

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
				static constexpr auto hardwareId = EncryptedConstant::encrypt_string("hardwareId");
				static constexpr auto validFrom = EncryptedConstant::encrypt_string("validFrom");
				static constexpr auto validUntil = EncryptedConstant::encrypt_string("validUntil");
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
				std::string hardwareId;
				std::string validFrom;
				std::string validUntil;
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
				jsonKeys.hardwareId = EncryptedConstant::decrypt_string(EncryptedStrings::JsonKeys::hardwareId);
				jsonKeys.validFrom = EncryptedConstant::decrypt_string(EncryptedStrings::JsonKeys::validFrom);
				jsonKeys.validUntil = EncryptedConstant::decrypt_string(EncryptedStrings::JsonKeys::validUntil);

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
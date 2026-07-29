#include "License.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QByteArray>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/rsa.h>

#include <vector>



#define ENCRYPTED_STRING(x) EncryptedConstant::decrypt_string(EncryptedConstant::encrypt_string(x))

namespace LicenseManager
{
	static void print_openssl_error();
	static EVP_PKEY* generate_rsa_keypair();
	static std::string private_key_to_string(EVP_PKEY* pkey);
	static std::string public_key_to_string(EVP_PKEY* pkey);
	static EVP_PKEY* load_private_key_from_string(const std::string& private_key_str);
	static EVP_PKEY* load_public_key_from_string(const std::string& public_key_str);
	static std::string sign_message(EVP_PKEY* pkey, const std::string& message);
	static bool verify_signature(EVP_PKEY* pkey, const std::string& message, const std::string& signature);
	static std::string base64_encode(const std::string& binary_data);
	static std::string base64_decode(const std::string& base64_data);
	static std::string signature_to_string(const std::string& signature);
	static std::string string_to_signature(const std::string& base64_signature);


	const License::DecryptedStrings& License::decryptedStrings()
	{
		static License::DecryptedStrings decryptedStrings;
		static bool initialized = false;
		if (!initialized)
		{
			decryptedStrings.decrypt();
			initialized = true;
		}
		return decryptedStrings;
	}

	License::License()
	{
		// OpenSSL 3.x auto-initializes on first use and cleans up at process exit.
		// The legacy OpenSSL_add_all_algorithms / ERR_load_crypto_strings /
		// EVP_cleanup / ERR_free_strings calls are no-ops (and deprecated) — the
		// old Initializer type is gone entirely.
	}
	License::License(const License& other)
	{
		this->operator=(other);
	}

	License::~License()
	{

	}

	License& License::operator=(const License& other)
	{
		m_signature = other.m_signature;
		m_licenseData = other.m_licenseData;
		m_name = other.m_name;
		return *this;
	}
	bool License::operator==(const License& other) const
	{
		if(m_signature != other.m_signature)
			return false;
		if(m_licenseData != other.m_licenseData)
			return false;
		if(m_name != other.m_name)
			return false;
		return true;
	}
	bool License::operator!=(const License& other) const
	{
		return !this->operator==(other);
	}


	bool License::saveToFile(const std::string& filePath) const
	{
		if (m_nameChangedSinceLastSave)
		{
			// Remove file if name has changed
			QFile file(m_loadedPath.c_str());
			if (file.exists())
			{
				file.remove();
			}
		}

		QJsonObject json;

		// Additional info about this library
		QJsonObject libInfo;
		libInfo[decryptedStrings().jsonKeys.version.c_str()] = LibraryInfo::version.toString().c_str();

		QJsonObject licenseData;
		for (auto it = m_licenseData.begin(); it != m_licenseData.end(); ++it)
		{
			licenseData[it->first.c_str()] = it->second.c_str();
		}
		json[decryptedStrings().jsonKeys.licenseData.c_str()] = licenseData;
		json[decryptedStrings().jsonKeys.signature.c_str()] = m_signature.c_str();
		json[decryptedStrings().jsonKeys.libraryInfo.c_str()] = libInfo;
		json[decryptedStrings().jsonKeys.name.c_str()] = m_name.c_str();


		QJsonDocument doc(json);
		QByteArray data = doc.toJson();

		QFile file(filePath.c_str());
		if(file.open(QIODevice::WriteOnly))
		{
			file.write(data);
			file.close();
			return true;
		}
		return false;
	}
	bool License::loadFromFile(const std::string& filePath)
	{
		QFile file(filePath.c_str());
		if(file.open(QIODevice::ReadOnly))
		{
			QByteArray data = file.readAll();
			QJsonDocument doc = QJsonDocument::fromJson(data);
			QJsonObject json = doc.object();

			m_licenseData.clear();
			QJsonObject licenseDataJson = json[decryptedStrings().jsonKeys.licenseData.c_str()].toObject();
			for(auto it = licenseDataJson.begin(); it != licenseDataJson.end(); ++it)
			{
				m_licenseData[it.key().toStdString()] = it.value().toString().toStdString();
			}
			m_signature = json[decryptedStrings().jsonKeys.signature.c_str()].toString().toStdString();
			m_name = json[decryptedStrings().jsonKeys.name.c_str()].toString().toStdString();
			file.close();
			m_nameChangedSinceLastSave = false;
			m_loadedPath = filePath;
			return true;
		}
		return false;
	}
	bool License::isVerified(const std::string& publicKey) const
	{
		EVP_PKEY* pubKey = load_public_key_from_string(publicKey);
		if(!pubKey)
			return false;
		std::string dataString = getDataString();
		bool isVerified = verify_signature(pubKey, dataString, m_signature);
		EVP_PKEY_free(pubKey);
		return isVerified;
	}

	bool License::signLicense(const std::string& privateKey)
	{
		EVP_PKEY* privKey = load_private_key_from_string(privateKey);
		if(!privKey)
			return false;
		m_signature = sign_message(privKey, getDataString());
		EVP_PKEY_free(privKey);
		return true;
	}
	bool License::signLicenseFromFile(const std::string& privateKeyFile)
	{
		std::string privateKey;
		if(loadPrivateKeyFromFile(privateKeyFile, privateKey))
		{
			return signLicense(privateKey);
		}
		return false;
	}
	std::string License::generatePrivateKey()
	{
		EVP_PKEY* keypair = generate_rsa_keypair();
		if(!keypair)
			return "";
		std::string privateKeyPEM = private_key_to_string(keypair);
		EVP_PKEY_free(keypair);
		return privateKeyPEM;
	}
	std::string License::getPublicKeyFromPrivateKey(const std::string& privateKeyPEM)
	{
		EVP_PKEY* privateKey = load_private_key_from_string(privateKeyPEM);
		if(!privateKey)
			return "";
		// EVP_PKEY holds both halves; PEM_write_bio_PUBKEY emits just the public part.
		std::string publicKeyPEM = public_key_to_string(privateKey);
		EVP_PKEY_free(privateKey);
		return publicKeyPEM;
	}
	std::string License::signMessage(const std::string& privateKey, const std::string& message)
	{
		EVP_PKEY* pkey = load_private_key_from_string(privateKey);
		if(!pkey)
			return "";
		std::string signature = sign_message(pkey, message);
		EVP_PKEY_free(pkey);
		return signature;
	}

	bool License::savePublicKeyToFile(const std::string& publicKey, const std::string& filename)
	{
		return writeToFile(filename, publicKey);
	}
	bool License::savePrivateKeyToFile(const std::string& privateKey, const std::string& filename)
	{
		return writeToFile(filename, privateKey);
	}
	bool License::loadPublicKeyFromFile(const std::string& filename, std::string& publicKey)
	{
		return readFromFile(filename, publicKey);
	}
	bool License::loadPrivateKeyFromFile(const std::string& filename, std::string& privateKey)
	{
		return readFromFile(filename, privateKey);
	}
	bool License::writeToFile(const std::string& filename, const std::string& data)
	{
		QFile file(filename.c_str());
		if (file.open(QIODevice::WriteOnly))
		{
			QByteArray arr(data.c_str(), data.length());
			file.write(arr);
			file.close();
			return true;
		}
		Logger::logError(decryptedStrings().messages.errReadingFromFile + filename);
		return false;
	}
	bool License::readFromFile(const std::string& filename, std::string& data)
	{
		QFile file(filename.c_str());
		if (file.open(QIODevice::ReadOnly))
		{
			QByteArray arr = file.readAll();
			data = arr.toStdString();
			file.close();
			return true;
		}
		Logger::logError(decryptedStrings().messages.errReadingFromFile + filename);
		return false;
	}


	std::string License::getDataString() const
	{
		QJsonObject licenseData;
		for(auto it = m_licenseData.begin(); it != m_licenseData.end(); ++it)
		{
			licenseData[it->first.c_str()] = it->second.c_str();
		}
		QJsonDocument doc(licenseData);
		QByteArray data = doc.toJson();
		return data.toStdString();
	}


	void print_openssl_error()
	{
		char err[256];
		ERR_error_string_n(ERR_get_error(), err, sizeof(err));
		Logger::logError(License::decryptedStrings().messages.errOpenSSL + std::string(err));
	}

	// Generate a 2048-bit RSA key pair via the EVP API (RSA_generate_key* is
	// deprecated in OpenSSL 3.x and removed in 4.x).
	EVP_PKEY* generate_rsa_keypair()
	{
		EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
		if(!ctx)
		{
			Logger::logError(License::decryptedStrings().messages.errGettingRSAKeypair);
			print_openssl_error();
			return nullptr;
		}

		EVP_PKEY* pkey = nullptr;
		if(EVP_PKEY_keygen_init(ctx) <= 0
		   || EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 2048) <= 0
		   || EVP_PKEY_keygen(ctx, &pkey) <= 0)
		{
			Logger::logError(License::decryptedStrings().messages.errGettingRSAKeypair);
			print_openssl_error();
			EVP_PKEY_CTX_free(ctx);
			return nullptr;
		}

		EVP_PKEY_CTX_free(ctx);
		return pkey;
	}

	// Write private key as PKCS#8 PEM ("-----BEGIN PRIVATE KEY-----").
	// PEM_write_bio_PrivateKey is the modern replacement for
	// PEM_write_bio_RSAPrivateKey; existing PKCS#1 keys still load via
	// PEM_read_bio_PrivateKey (see load_private_key_from_string) so old keys
	// stay compatible.
	std::string private_key_to_string(EVP_PKEY* pkey)
	{
		BIO* bio = BIO_new(BIO_s_mem());
		if (!PEM_write_bio_PrivateKey(bio, pkey, nullptr, nullptr, 0, nullptr, nullptr))
		{
			Logger::logError(License::decryptedStrings().messages.errConvPrivToStr);
			print_openssl_error();
			BIO_free(bio);
			return "";
		}

		char* key_data = nullptr;
		long key_length = BIO_get_mem_data(bio, &key_data);
		std::string private_key_str(key_data, key_length);

		BIO_free(bio);
		return private_key_str;
	}

	// Write public key as SubjectPublicKeyInfo PEM ("-----BEGIN PUBLIC KEY-----"),
	// same format as the legacy PEM_write_bio_RSA_PUBKEY produced.
	std::string public_key_to_string(EVP_PKEY* pkey)
	{
		BIO* bio = BIO_new(BIO_s_mem());
		if (!PEM_write_bio_PUBKEY(bio, pkey))
		{
			Logger::logError(License::decryptedStrings().messages.errConvPubToStr);
			print_openssl_error();
			BIO_free(bio);
			return "";
		}

		char* key_data = nullptr;
		long key_length = BIO_get_mem_data(bio, &key_data);
		std::string public_key_str(key_data, key_length);

		BIO_free(bio);
		return public_key_str;
	}

	// PEM_read_bio_PrivateKey accepts both PKCS#1 (legacy) and PKCS#8 (modern).
	EVP_PKEY* load_private_key_from_string(const std::string& private_key_str)
	{
		BIO* bio = BIO_new_mem_buf(private_key_str.c_str(), -1);
		EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
		if (!pkey)
		{
			Logger::logError(License::decryptedStrings().messages.errLoadPriv);
			print_openssl_error();
		}
		BIO_free(bio);
		return pkey;
	}

	EVP_PKEY* load_public_key_from_string(const std::string& public_key_str)
	{
		BIO* bio = BIO_new_mem_buf(public_key_str.c_str(), -1);
		EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
		if (!pkey)
		{
			Logger::logError(License::decryptedStrings().messages.errLoadPub);
			print_openssl_error();
		}
		BIO_free(bio);
		return pkey;
	}

	// Sign the message with RSA-SHA256 (PKCS#1 v1.5 — same scheme as the old
	// RSA_sign path, so signatures produced by the legacy 1.1.x code still
	// verify here and vice versa).
	std::string sign_message(EVP_PKEY* pkey, const std::string& message)
	{
		EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
		if(!mdctx)
		{
			print_openssl_error();
			return "";
		}

		if (EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, pkey) <= 0
		    || EVP_DigestSignUpdate(mdctx, message.data(), message.size()) <= 0)
		{
			Logger::logError(License::decryptedStrings().messages.errSign);
			print_openssl_error();
			EVP_MD_CTX_free(mdctx);
			return "";
		}

		size_t sig_len = 0;
		if (EVP_DigestSignFinal(mdctx, nullptr, &sig_len) <= 0)
		{
			Logger::logError(License::decryptedStrings().messages.errSign);
			print_openssl_error();
			EVP_MD_CTX_free(mdctx);
			return "";
		}

		std::vector<unsigned char> signature(sig_len);
		if (EVP_DigestSignFinal(mdctx, signature.data(), &sig_len) <= 0)
		{
			Logger::logError(License::decryptedStrings().messages.errSign);
			print_openssl_error();
			EVP_MD_CTX_free(mdctx);
			return "";
		}

		EVP_MD_CTX_free(mdctx);
		return signature_to_string(std::string(reinterpret_cast<char*>(signature.data()), sig_len));
	}

	bool verify_signature(EVP_PKEY* pkey, const std::string& message, const std::string& signature)
	{
		std::string signature_bin = string_to_signature(signature);

		EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
		if(!mdctx)
		{
			print_openssl_error();
			return false;
		}

		if (EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pkey) <= 0
		    || EVP_DigestVerifyUpdate(mdctx, message.data(), message.size()) <= 0)
		{
			Logger::logError(License::decryptedStrings().messages.errSigVerFail);
			print_openssl_error();
			EVP_MD_CTX_free(mdctx);
			return false;
		}

		int result = EVP_DigestVerifyFinal(mdctx,
			reinterpret_cast<const unsigned char*>(signature_bin.data()),
			signature_bin.size());
		EVP_MD_CTX_free(mdctx);

		if (result != 1)
		{
			Logger::logError(License::decryptedStrings().messages.errSigVerFail);
			if(result < 0) print_openssl_error();
			return false;
		}
		return true;
	}

	std::string base64_encode(const std::string& binary_data)
	{
		BIO* bio = BIO_new(BIO_s_mem());
		BIO* b64 = BIO_new(BIO_f_base64());
		bio = BIO_push(b64, bio);
		BIO_write(bio, binary_data.data(), binary_data.size());
		BIO_flush(bio);

		BUF_MEM* buffer_ptr;
		BIO_get_mem_ptr(bio, &buffer_ptr);

		std::string base64_encoded(buffer_ptr->data, buffer_ptr->length);
		BIO_free_all(bio);

		return base64_encoded;
	}

	std::string base64_decode(const std::string& base64_data)
	{
		BIO* bio = BIO_new_mem_buf(base64_data.data(), base64_data.size());
		BIO* b64 = BIO_new(BIO_f_base64());
		bio = BIO_push(b64, bio);

		std::vector<char> buffer(base64_data.size());
		int length = BIO_read(bio, buffer.data(), buffer.size());
		BIO_free_all(bio);

		return std::string(buffer.data(), length);
	}

	std::string signature_to_string(const std::string& signature)
	{
		return base64_encode(signature);
	}

	std::string string_to_signature(const std::string& base64_signature)
	{
		return base64_decode(base64_signature);
	}
}

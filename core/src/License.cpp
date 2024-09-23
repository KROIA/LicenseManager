#include "License.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QByteArray>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>


namespace LicenseManager
{
	static void print_openssl_error();
	static RSA* generate_RSA_keypair();
	static std::string encrypt_with_private_key(RSA* rsa_private_key, const std::string& plaintext);
	static std::string decrypt_with_public_key(RSA* rsa_public_key, const std::string& encrypted);
	static std::string private_key_to_string(RSA* rsa_private_key);
	static std::string public_key_to_string(RSA* rsa_public_key);
	static RSA* generate_public_key_from_private(RSA* rsa_private_key);
	static RSA* load_private_key_from_string(const std::string& private_key_str);
	static RSA* load_public_key_from_string(const std::string& public_key_str);
	static std::string sign_message(RSA* rsa_private_key, const std::string& message);
	static bool verify_signature(RSA* rsa_public_key, const std::string& message, const std::string& signature);
	static std::string base64_encode(const std::string& binary_data);
	static std::string base64_decode(const std::string& base64_data);
	static std::string signature_to_string(const std::string& signature);
	static std::string string_to_signature(const std::string& base64_signature);



	License::License()
	{
		class Initializer
		{
			public:
			Initializer()
			{
				// Initialize OpenSSL
				OpenSSL_add_all_algorithms();
				ERR_load_crypto_strings();
			}
			~Initializer()
			{
				// Cleanup OpenSSL
				EVP_cleanup();
				ERR_free_strings();
			}
		};
		static Initializer init;

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
		libInfo["version"] = LibraryInfo::version.toString().c_str();
		
		QJsonObject licenseData;
		for (auto it = m_licenseData.begin(); it != m_licenseData.end(); ++it)
		{
			licenseData[it->first.c_str()] = it->second.c_str();
		}
		json["licenseData"] = licenseData;
		json["signature"] = m_signature.c_str();
		json["libraryInfo"] = libInfo;
		json["name"] = m_name.c_str();


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
			QJsonObject licenseDataJson = json["licenseData"].toObject();
			for(auto it = licenseDataJson.begin(); it != licenseDataJson.end(); ++it)
			{
				m_licenseData[it.key().toStdString()] = it.value().toString().toStdString();
			}
			m_signature = json["signature"].toString().toStdString();
			m_name = json["name"].toString().toStdString();
			file.close();
			m_nameChangedSinceLastSave = false;
			m_loadedPath = filePath;
			return true;
		}
		return false;
	}
	bool License::isVerified(const std::string& publicKey) const
	{
		RSA* pubKey = load_public_key_from_string(publicKey);
		if(!pubKey)
			return false;
		std::string dataString = getDataString();
		bool isVerified = verify_signature(pubKey, dataString, m_signature);
		RSA_free(pubKey);
		return isVerified;
	}

	bool License::signLicense(const std::string& privateKey)
	{
		RSA* privKey = load_private_key_from_string(privateKey);
		if(!privKey)
			return false;
		m_signature = sign_message(privKey, getDataString());
		RSA* pubKey = generate_public_key_from_private(privKey);
		RSA_free(privKey);
		RSA_free(pubKey);
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
		// Generate RSA key pair
		RSA* rsa_keypair = generate_RSA_keypair();

		RSA* private_key = RSAPrivateKey_dup(rsa_keypair);
		std::string privateKeyPEM = private_key_to_string(private_key);
		RSA_free(private_key);
		RSA_free(rsa_keypair);
		return privateKeyPEM;
	}
	std::string License::getPublicKeyFromPrivateKey(const std::string& privateKeyPEM)
	{
		RSA* privateKey = load_private_key_from_string(privateKeyPEM);
		RSA* pubKey = generate_public_key_from_private(privateKey);

		std::string publicKeyPEM = public_key_to_string(pubKey);
		RSA_free(pubKey);
		RSA_free(privateKey);
		return publicKeyPEM;
	}
	std::string License::signMessage(const std::string& privateKey, const std::string& message)
	{
		RSA* rsa = load_private_key_from_string(privateKey);
		std::string signature = sign_message(rsa, message);
		RSA_free(rsa);
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
		Logger::logError("Error writing to file: " + filename);
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
		Logger::logError("Error reading from file: " + filename);
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
		char* err = (char*)malloc(130);
		ERR_load_crypto_strings();
		ERR_error_string(ERR_get_error(), err);
		Logger::logError("OpenSSL error: " + std::string(err));
		free(err);
	}

	// Function to generate RSA key pair
	RSA* generate_RSA_keypair()
	{
		int key_length = 2048;
		RSA* rsa = RSA_generate_key(key_length, RSA_F4, NULL, NULL);
		if (!rsa) 
		{
			Logger::logError("Error generating RSA keypair!");
			print_openssl_error();
		}
		return rsa;
	}

	// Function to encrypt text using private key
	std::string encrypt_with_private_key(RSA* rsa_private_key, const std::string& plaintext)
	{
		std::string encrypted;
		int rsa_size = RSA_size(rsa_private_key);
		unsigned char* encrypted_text = (unsigned char*)malloc(rsa_size);

		int result = RSA_private_encrypt(plaintext.length(),
			(unsigned char*)plaintext.c_str(),
			encrypted_text,
			rsa_private_key,
			RSA_PKCS1_PADDING);
		if (result == -1) 
		{
			Logger::logError("Error during encryption!");
			print_openssl_error();
		}
		else 
		{
			encrypted.assign((char*)encrypted_text, result);
		}

		free(encrypted_text);
		return encrypted;
	}

	// Function to decrypt text using public key
	std::string decrypt_with_public_key(RSA* rsa_public_key, const std::string& encrypted) 
	{
		std::string decrypted;
		int rsa_size = RSA_size(rsa_public_key);
		unsigned char* decrypted_text = (unsigned char*)malloc(rsa_size);

		int result = RSA_public_decrypt(encrypted.length(),
			(unsigned char*)encrypted.c_str(),
			decrypted_text,
			rsa_public_key,
			RSA_PKCS1_PADDING);
		if (result == -1) 
		{
			Logger::logError("Error during decryption!");
			print_openssl_error();
		}
		else
		{
			decrypted.assign((char*)decrypted_text, result);
		}

		free(decrypted_text);
		return decrypted;
	}

	// Convert RSA private key to string (PEM format)
	std::string private_key_to_string(RSA* rsa_private_key)
	{
		BIO* bio = BIO_new(BIO_s_mem());
		if (!PEM_write_bio_RSAPrivateKey(bio, rsa_private_key, NULL, NULL, 0, NULL, NULL)) 
		{
			Logger::logError("Error converting private key to string!");
			print_openssl_error();
			return "";
		}

		char* key_data = NULL;
		size_t key_length = BIO_get_mem_data(bio, &key_data);
		std::string private_key_str(key_data, key_length);

		BIO_free(bio);
		return private_key_str;
	}

	// Convert RSA public key to string (PEM format)
	std::string public_key_to_string(RSA* rsa_public_key)
	{
		BIO* bio = BIO_new(BIO_s_mem());
		if (!PEM_write_bio_RSA_PUBKEY(bio, rsa_public_key)) 
		{
			Logger::logError("Error converting public key to string!");
			print_openssl_error();
			return "";
		}

		char* key_data = NULL;
		size_t key_length = BIO_get_mem_data(bio, &key_data);
		std::string public_key_str(key_data, key_length);

		BIO_free(bio);
		return public_key_str;
	}

	// Function to generate RSA public key from private key
	RSA* generate_public_key_from_private(RSA* rsa_private_key)
	{
		// Check if the private key is valid
		if (!rsa_private_key) {
			Logger::logError("Invalid private key!");
			return nullptr;
		}

		// Create a new RSA structure to hold the public key
		RSA* rsa_public_key = RSAPublicKey_dup(rsa_private_key);
		if (!rsa_public_key) {
			Logger::logError("Error generating public key from private key!");
			print_openssl_error();
		}

		return rsa_public_key;
	}

	// Load RSA private key from string (PEM format)
	RSA* load_private_key_from_string(const std::string& private_key_str)
	{
		BIO* bio = BIO_new_mem_buf((void*)private_key_str.c_str(), -1);
		RSA* rsa_private_key = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);
		if (!rsa_private_key) {
			Logger::logError("Error loading private key from string!");
			print_openssl_error();
		}

		BIO_free(bio);
		return rsa_private_key;
	}

	// Load RSA public key from string (PEM format)
	RSA* load_public_key_from_string(const std::string& public_key_str)
	{
		BIO* bio = BIO_new_mem_buf((void*)public_key_str.c_str(), -1);
		RSA* rsa_public_key = PEM_read_bio_RSA_PUBKEY(bio, NULL, NULL, NULL);
		if (!rsa_public_key) {
			Logger::logError("Error loading public key from string!");
			print_openssl_error();
		}

		BIO_free(bio);
		return rsa_public_key;
	}
	// Function to sign a message using RSA private key
	std::string sign_message(RSA* rsa_private_key, const std::string& message) 
	{
		unsigned char hash[SHA256_DIGEST_LENGTH];
		SHA256((unsigned char*)message.c_str(), message.length(), hash);

		unsigned char* signature = (unsigned char*)malloc(RSA_size(rsa_private_key));
		unsigned int signature_len = 0;

		if (RSA_sign(NID_sha256, hash, SHA256_DIGEST_LENGTH, signature, &signature_len, rsa_private_key) != 1) {
			Logger::logError("Error signing message!");
			print_openssl_error();
			free(signature);
			return "";
		}

		std::string signature_str((char*)signature, signature_len);
		free(signature);

		return signature_to_string(signature_str);
	}

	// Function to verify a signature using RSA public key
	bool verify_signature(RSA* rsa_public_key, const std::string& message, const std::string& signature) 
	{
		unsigned char hash[SHA256_DIGEST_LENGTH];
		SHA256((unsigned char*)message.c_str(), message.length(), hash);
		std::string signature_str = string_to_signature(signature);
		int result = RSA_verify(NID_sha256, hash, SHA256_DIGEST_LENGTH, (unsigned char*)signature_str.c_str(), signature_str.length(), rsa_public_key);
		if (result != 1) {
			Logger::logError("Signature verification failed!");
			print_openssl_error();
			return false;
		}

		return true;
	}
	// Encode binary data to Base64
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

	// Decode Base64 to binary data
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

	// Function to convert signature to Base64-encoded string
	std::string signature_to_string(const std::string& signature) 
	{
		return base64_encode(signature);
	}

	// Function to convert Base64-encoded string back to signature
	std::string string_to_signature(const std::string& base64_signature) 
	{
		return base64_decode(base64_signature);
	}
}
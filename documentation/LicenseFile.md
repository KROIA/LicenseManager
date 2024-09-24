# License file

## Overview
* [Description](#description)
* [File structure](#file-structure)
* [Lincense creation](#lincense-creation)
* [Validation check](#validation-check)
* [License data check](#license-data-check)

## Description
The license file contains the informations about the license.<br>
The data can be customized depending on the needs of your licensing model.<br>
For license validation, the signature of the license data is also contained in the file.

## File structure
The license is stored in json format.
* **libraryInfo:** Documents the LicenseManager library version.
* **licenseData:** Stores the application specific license informations.<br>
Only this section is used in the validation process.
* **signature:** The signature is generated using the [LicenseCreator](../examples/LicenseGenerator/README.md) and is needed to check the license for validity. 


```
{
    "libraryInfo": {
        "version": "00.00.0000"
    },
    "licenseData": {
        "expiryDate": "2025-01-01",
        "name": "John Doe"
    },
    "name": "",
    "signature": "QlrjbbBULuuCBLQeXlZhcRTfGPuyU84RgVm/FeEi7Ke4Ie6Hy0YI2Bl8PIH6iilc\nIuCA4IABRzWoszWkY76QzV6FQC5UmAHshH+u8jLnTqIbixA6Q5m/fpTHnC1IWlVz\nUzQOEgzDpdE05VOk/sEAI1PLhBx5PWjb9M7TsKRosO/1tObMzH2hhoQgrGoS2ZmE\nnmcno781pmMei7kAMTpwrgdfNnLETbgKly/uVAP1YRI/ls51WSd6GfN8kGcsPz55\nD5qKYgL2UU1JAc1ck6KG6BLeY5+QPowOdG/5Yzi6HhNkzKHOQoDV0WwJGP56w5jG\nt8ZzzZtW0+5p3wTuZHH/oA==\n"
}


```
## Lincense creation
The license is generated using a external tool. For example you can use the [LicenseCreator](../examples/LicenseGenerator/README.md).<br>
You can implement the same functionality using this example code.

```c++
// Do not share/store/hardcode the private key in your application!
std::string privateKey = LicenseManager::License::generatePrivateKey();

// The public key will be hardcoded in the application that uses the validation
std::string publicKey = LicenseManager::License::getPublicKeyFromPrivateKey(s_privateKey);

std::map<std::string, std::string> licenseData;
licenseData["name"]       = "John Doe";
licenseData["expiryDate"] = "2025-01-01";

// Create the license
LicenseManager::License license;

// Set the license data
license.setLicenseData(licenseData);

// Sign the license using the private key
license.signLicense(privateKey);

license.saveToFile("license.lic");
```


## Validation check
To validate if a license was modified, the **public key** and the **signature** is needed.<br>
The public key is stored in the application and the signature is stored in the license file.<br>

```c++
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
```

## License data check
**The validation check does not check if the user is permitted to use the software!**<br>
You have to implement the code that checks the license data, for example the expiryDate.

```c++
// Verify the license
if (license.isVerified(decryptedPublicKey))
{
	// The license data can be trusted

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
```
#ifndef CRYPT_ECIES_H
#define CRYPT_ECIES_H

#include "oids.h"
#include "osrng.h"
#include "eccrypto.h"
#include "asn.h"
#include "ec2n.h"
#include "ecp.h"
#include "hex.h"
#include "files.h"
#include <iostream>
#include <stdio.h>
#include <ostream>

namespace CRYPT_ECIES
{

using namespace std;
using namespace CryptoPP;

int GenerateKey(const std::string &strSeed, std::string &strPri, std::string &strPub);
std::string Encrypt(const std::string &strPub, const std::string &strSeed, const string &plainText);
std::string Decrypt(const std::string &strPri, const std::string &strSeed, const string &cipherText);

void SavePrivateKeyInString(const PrivateKey& key, string& str);
void SavePublicKeyInString(const PublicKey& key, string& str);

void LoadPrivateKeyFromString(PrivateKey& key, const string& str);
void LoadPublicKeyFromString(PublicKey& key, const string& str);

void SavePrivateKeyInFile(const PrivateKey& key, const string& file = "ecies.private.key");
void SavePublicKeyInFile(const PublicKey& key, const string& file = "ecies.public.key");

void LoadPrivateKeyFromFile(PrivateKey& key, const string& file = "ecies.private.key");
void LoadPublicKeyFromFile(PublicKey& key, const string& file = "ecies.public.key");

void PrintPrivateKey(const DL_PrivateKey_EC<ECP>& key, ostream& out = cout);
void PrintPublicKey(const DL_PublicKey_EC<ECP>& key, ostream& out = cout);

};

#endif

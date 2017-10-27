#include "crypt_ecies.h"

namespace CRYPT_ECIES
{
int GenerateKey(const std::string &strSeed, std::string &strPri, std::string &strPub)
{
    RandomPool randPool;
    randPool.Put((byte *)strSeed.c_str(), strSeed.length());
    ECIES<ECP>::Decryptor d0(randPool, ASN1::secp256r1());
    //PrintPrivateKey(d0.GetKey());

    ECIES<ECP>::Encryptor e0(d0);
    //PrintPublicKey(e0.GetKey());

    SavePrivateKeyInString(d0.GetPrivateKey(), strPri);
    SavePublicKeyInString(e0.GetPublicKey() ,strPub);

    return 0;
}

std::string Encrypt(const std::string &strPub, const std::string &strSeed, const string &strPlainText)
{
    RandomPool randPool;
    randPool.Put((byte *)strSeed.c_str(), strSeed.length());

    string strResult;


    ECIES<ECP>::Encryptor e;
    LoadPublicKeyFromString(e.AccessPublicKey(), strPub);
    e.GetPublicKey().ThrowIfInvalid(randPool, 3); 

    StringSource ss (strPlainText, true, new PK_EncryptorFilter(randPool, e, new StringSink(strResult) ) );

    return strResult;
}

std::string Decrypt(const std::string &strPri, const std::string &strSeed, const string &strCipherText)
{
    RandomPool randPool;
    randPool.Put((byte *)strSeed.c_str(), strSeed.length());

    ECIES<ECP>::Decryptor d;
    LoadPrivateKeyFromString(d.AccessPrivateKey(), strPri);
    d.GetPrivateKey().ThrowIfInvalid(randPool, 3);

    string strResult; // decrypted message
    StringSource ss (strCipherText, true, new PK_DecryptorFilter(randPool, d, new StringSink(strResult) ) );
    return strResult;
}

void PrintPrivateKey(const DL_PrivateKey_EC<ECP>& key, ostream& out)
{
    // Group parameters
    const DL_GroupParameters_EC<ECP>& params = key.GetGroupParameters();
    // Base precomputation (for public key calculation from private key)
    const DL_FixedBasePrecomputation<ECPPoint>& bpc = params.GetBasePrecomputation();
    // Public Key (just do the exponentiation)
    const ECPPoint point = bpc.Exponentiate(params.GetGroupPrecomputation(), key.GetPrivateExponent());

    out << "Modulus: " << std::hex << params.GetCurve().GetField().GetModulus() << endl;
    out << "Cofactor: " << std::hex << params.GetCofactor() << endl;

    out << "Coefficients" << endl;
    out << "  A: " << std::hex << params.GetCurve().GetA() << endl;
    out << "  B: " << std::hex << params.GetCurve().GetB() << endl;

    out << "Base Point" << endl;
    out << "  x: " << std::hex << params.GetSubgroupGenerator().x << endl;
    out << "  y: " << std::hex << params.GetSubgroupGenerator().y << endl;

    out << "Public Point" << endl;
    out << "  x: " << std::hex << point.x << endl;
    out << "  y: " << std::hex << point.y << endl;

    out << "Private Exponent (multiplicand): " << endl;
    out << "  " << std::hex << key.GetPrivateExponent() << endl;
}

void PrintPublicKey(const DL_PublicKey_EC<ECP>& key, ostream& out)
{
    // Group parameters
    const DL_GroupParameters_EC<ECP>& params = key.GetGroupParameters();
    // Public key
    const ECPPoint& point = key.GetPublicElement();

    out << "Modulus: " << std::hex << params.GetCurve().GetField().GetModulus() << endl;
    out << "Cofactor: " << std::hex << params.GetCofactor() << endl;

    out << "Coefficients" << endl;
    out << "  A: " << std::hex << params.GetCurve().GetA() << endl;
    out << "  B: " << std::hex << params.GetCurve().GetB() << endl;

    out << "Base Point" << endl;
    out << "  x: " << std::hex << params.GetSubgroupGenerator().x << endl;
    out << "  y: " << std::hex << params.GetSubgroupGenerator().y << endl;

    out << "Public Point" << endl;
    out << "  x: " << std::hex << point.x << endl;
    out << "  y: " << std::hex << point.y << endl;
}

void SavePrivateKeyInString(const PrivateKey& key, string& str)
{
    StringSink sink(str);
    key.Save(sink);
}

void SavePublicKeyInString(const PublicKey& key, string& str)
{
    StringSink sink(str);
    key.Save(sink);
}

void LoadPrivateKeyFromString(PrivateKey& key, const string& str)
{
    StringSource source(str, true);
    key.Load(source);
}

void LoadPublicKeyFromString(PublicKey& key, const string& str)
{
    StringSource source(str, true);
    key.Load(source);
}

void SavePrivateKeyInFile(const PrivateKey& key, const string& file)
{
    FileSink sink(file.c_str());
    key.Save(sink);
}

void SavePublicKeyInFile(const PublicKey& key, const string& file)
{
    FileSink sink(file.c_str());
    key.Save(sink);
}

void LoadPrivateKeyFromFile(PrivateKey& key, const string& file)
{
    FileSource source(file.c_str(), true);
    key.Load(source);
}

void LoadPublicKeyFromFile(PublicKey& key, const string& file)
{
    FileSource source(file.c_str(), true);
    key.Load(source);
}

};

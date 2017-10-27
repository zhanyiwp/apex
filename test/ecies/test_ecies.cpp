#include "util_crypt_ecies.h"
#include <iostream>

using namespace std;
using namespace CryptoPP;
using namespace CRYPT_ECIES;

int test1()
{
    static const string message("Now is the time for all good men to come to the aide of their country.");

    AutoSeededRandomPool prng;

    /////////////////////////////////////////////////
    // Part one - generate keys

    ECIES<ECP>::Decryptor d0(prng, ASN1::secp256k1());
    PrintPrivateKey(d0.GetKey());

    ECIES<ECP>::Encryptor e0(d0);
    PrintPublicKey(e0.GetKey());

    /////////////////////////////////////////////////
    // Part two - save keys
    //   Get* returns a const reference

    SavePrivateKeyInFile(d0.GetPrivateKey());
    SavePublicKeyInFile(e0.GetPublicKey());

    /////////////////////////////////////////////////
    // Part three - load keys
    //   Access* returns a non-const reference

    ECIES<ECP>::Decryptor d1;
    LoadPrivateKeyFromFile(d1.AccessPrivateKey());
    d1.GetPrivateKey().ThrowIfInvalid(prng, 3);

    ECIES<ECP>::Encryptor e1;
    LoadPublicKeyFromFile(e1.AccessPublicKey());
    e1.GetPublicKey().ThrowIfInvalid(prng, 3);

    /////////////////////////////////////////////////
    // Part four - encrypt/decrypt with e0/d1

    string em0; // encrypted message
    StringSource ss1 (message, true, new PK_EncryptorFilter(prng, e0, new StringSink(em0) ) );
    string dm0; // decrypted message
    StringSource ss2 (em0, true, new PK_DecryptorFilter(prng, d1, new StringSink(dm0) ) );

    cout << dm0 << endl;

    /////////////////////////////////////////////////
    // Part five - encrypt/decrypt with e1/d0

    string em1; // encrypted message
    StringSource ss3 (message, true, new PK_EncryptorFilter(prng, e1, new StringSink(em1) ) );
    string dm1; // decrypted message
    StringSource ss4 (em1, true, new PK_DecryptorFilter(prng, d0, new StringSink(dm1) ) );

    cout << dm1 << endl;

    return 0;
}

int test2()
{
    static const string message("Now is the time for all good men to come to the aide of their country.");

    string strPub, strPri;

    AutoSeededRandomPool prng;

    /////////////////////////////////////////////////
    // Part one - generate keys

    ECIES<ECP>::Decryptor d0(prng, ASN1::secp256k1());
    PrintPrivateKey(d0.GetKey());

    ECIES<ECP>::Encryptor e0(d0);
    PrintPublicKey(e0.GetKey());

    /////////////////////////////////////////////////
    // Part two - save keys
    //   Get* returns a const reference

    SavePrivateKeyInString(d0.GetPrivateKey(), strPri);
    SavePublicKeyInString(e0.GetPublicKey(), strPub);

    /////////////////////////////////////////////////
    // Part three - load keys
    //   Access* returns a non-const reference

    ECIES<ECP>::Decryptor d1;
    LoadPrivateKeyFromString(d1.AccessPrivateKey(), strPri);
    d1.GetPrivateKey().ThrowIfInvalid(prng, 3);

    ECIES<ECP>::Encryptor e1;
    LoadPublicKeyFromString(e1.AccessPublicKey(), strPub);
    e1.GetPublicKey().ThrowIfInvalid(prng, 3);

    /////////////////////////////////////////////////
    // Part four - encrypt/decrypt with e0/d1

    string em0; // encrypted message
    StringSource ss1 (message, true, new PK_EncryptorFilter(prng, e0, new StringSink(em0) ) );
    string dm0; // decrypted message
    StringSource ss2 (em0, true, new PK_DecryptorFilter(prng, d1, new StringSink(dm0) ) );

    cout << dm0 << endl;

    /////////////////////////////////////////////////
    // Part five - encrypt/decrypt with e1/d0

    string em1; // encrypted message
    StringSource ss3 (message, true, new PK_EncryptorFilter(prng, e1, new StringSink(em1) ) );
    string dm1; // decrypted message
    StringSource ss4 (em1, true, new PK_DecryptorFilter(prng, d0, new StringSink(dm1) ) );

    cout << dm1 << endl;

    return 0;
}

int test3()
{
    string strSeed("hello");
    string strPub;
    string strPri;
    string strPlainText("Now is the time for all good men to come to the aide of their country.");
    string strCipherText;
    string strResult;

    GenerateKey(strSeed, strPri, strPub);
    strCipherText = Encrypt(strPub, strSeed, strPlainText);
    strResult = Decrypt(strPri, strSeed, strCipherText);

    cout << strResult << endl;
    return 0;

    return 0;
}

int main(int argc, char* argv[])
{
    //test1();
    //test2();
    test3();
    return 0;
}

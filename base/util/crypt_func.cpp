#include "crypt_func.h"

#define CRYPTOPP_ENABLE_NAMESPACE_WEAK 1

#include "files.h"
#include "hex.h"
#include "filters.h"
#include "osrng.h"
#include "pubkey.h"
#include "eccrypto.h"
#include "pubkey.h"
#include "asn.h"
#include "oids.h"
#include "cryptlib.h"
#include "base64.h"
#include "md5.h"
#include "modes.h"
#include "aes.h"
#include "filters.h"

using namespace CryptoPP;

static uint16_t keysize(Gaz::Crypt::KeyBitsNum kbn)
{
	switch (kbn)
	{
	case Gaz::Crypt::KBN_NONE:
		//ASSERT(FALSE);
		return 0;
	case Gaz::Crypt::KBN_AES_128:
		return 16;
	case Gaz::Crypt::KBN_AES_192:
		return 24;
	case Gaz::Crypt::KBN_AES_256:
		return 32;
	case Gaz::Crypt::KBN_ECC_160:
		return 20;
	case Gaz::Crypt::KBN_ECC_224:
		return 38;
	case Gaz::Crypt::KBN_ECC_256:
		return 32;
	case Gaz::Crypt::KBN_ECC_384:
		return 48;
	case Gaz::Crypt::KBN_ECC_512:
		return 64;
	default:
		//ASSERT(FALSE);
		return 0;
	}
}

//Base16编码
bool	Gaz::Crypt::Base16::encode(const std::string& inbuf, std::string& outstr)
{
	outstr.clear();
	try
	{
		StringSource ss(inbuf, true,
			new HexEncoder(
			new StringSink(outstr)
			) // HexEncoder
			); // StringSource
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Base16::decode(const std::string& instr, std::string& outbuf)
{
	outbuf.clear();
	try
	{
		StringSource ss(instr, true,
			new HexDecoder(
			new StringSink(outbuf)
			) // HexDecoder
			); // StringSource
	}
	catch (...)
	{
		return false;
	}
	return true;
}

//Base64编码
bool	Gaz::Crypt::Base64::encode(const std::string& inbuf, std::string& outstr)
{
	outstr.clear();
	try
	{
		StringSource ss(inbuf, true,
			new Base64Encoder(
			new StringSink(outstr)
			) // Base64Encoder
			); // StringSource
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Base64::decode(const std::string& instr, std::string& outbuf)
{
	outbuf.clear();
	try
	{
		StringSource ss(instr, true,
			new Base64Decoder(
			new StringSink(outbuf)
			) // Base64Decoder
			); // StringSource
	}
	catch (...)
	{
		return false;
	}
	return true;
}

//摘要函数
//此处file指加解密的文件对象
bool	Gaz::Crypt::Md5::hash(const std::string& inbuf, std::string& outbuf)
{
	outbuf.clear();
	try
	{
		Weak1::MD5 md;

		StringSource(inbuf, true,
			new HashFilter(md,
			new StringSink(outbuf)
			)//HashFilter
			);//FileSource
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Md5::file_hash(const std::string& filepath_instr, std::string& outbuf)
{
	outbuf.clear();
	try
	{
		Weak1::MD5 md;

		FileSource(filepath_instr.c_str(), true,
			new HashFilter(md,
			new StringSink(outbuf)
			)//HashFilter
			);//FileSource
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Sha256::hash(const std::string& inbuf, std::string& outbuf)
{
	outbuf.clear();
	try
	{
		SHA256 sha;

		StringSource(inbuf, true,
			new HashFilter(sha,
			new StringSink(outbuf)
			)//HashFilter
			);//FileSource
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Sha256::file_hash(const std::string& filepath_instr, std::string& outbuf)
{
	outbuf.clear();
	try
	{
		SHA256 sha;

		FileSource(filepath_instr.c_str(), true,
			new HashFilter(sha,
			new StringSink(outbuf)
			)//HashFilter
			);//FileSource
	}
	catch (...)
	{
		return false;
	}
	return true;
}

//对称加密
bool	Gaz::Crypt::Aes::CbcMode::gen_key(std::string& outbuf, KeyBitsNum kbn/* = KBN_AES_256*/)
{
	outbuf.clear();

	if (KBN_AES_128 != kbn && KBN_AES_192 != kbn && KBN_AES_256 != kbn)
	{
		return false;
	}
	try
	{
		AutoSeededRandomPool rnd;

		uint16_t key_size = keysize(kbn);
		uint8_t key[32];
		//ASSERT(key_size <= sizeof(key));
		rnd.GenerateBlock(key, key_size);

		outbuf = std::string((const char*)(char*)key, key_size);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Aes::CbcMode::encrypt(const std::string& plain_inbuf, std::string& chiper_outbuf, const std::string& key_inbuf, const std::string& iv_inbuf/* = ""*/)
{
	std::string iv = iv_inbuf;
	iv.resize(AES::BLOCKSIZE, 0);

	chiper_outbuf.clear();

	try
	{
		CBC_Mode<AES>::Encryption encrypt((const uint8_t*)key_inbuf.data(), key_inbuf.size(), (const uint8_t*)iv.data());
		StringSource(plain_inbuf, true,
			new StreamTransformationFilter(encrypt,
			new StringSink(chiper_outbuf)
			)
			);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Aes::CbcMode::decrypt(const std::string& chiper_inbuf, std::string& plain_outbuf, const std::string& key_inbuf, const std::string& iv_inbuf/* = ""*/)
{
	std::string iv = iv_inbuf;
	iv.resize(AES::BLOCKSIZE, 0);

	plain_outbuf.clear();

	try
	{
		CBC_Mode<AES>::Decryption decrypt((const uint8_t*)key_inbuf.data(), key_inbuf.size(), (const uint8_t*)iv.data());
		StringSource(chiper_inbuf, true,
			new StreamTransformationFilter(decrypt,
			new StringSink(plain_outbuf)
			)
			);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

//非对称加密
//使用secp256k1曲线
bool	Gaz::Crypt::Ecies::MemKey::gen_key(std::string& pri_outbuf, std::string& pub_outbuf)
{
	pri_outbuf.clear();
	pub_outbuf.clear();

	try
	{
		AutoSeededRandomPool rnd(false, keysize(KBN_ECC_256));

		ECIES<ECP>::PrivateKey  pri_key;
		ECIES<ECP>::PublicKey   pub_key;
		
		pri_key.Initialize(rnd, ASN1::secp256k1());
		pri_key.MakePublicKey(pub_key);

		//验证
		pri_key.ThrowIfInvalid(rnd, 3);
		pub_key.ThrowIfInvalid(rnd, 3);

		const Integer priv_exp = pri_key.GetPrivateExponent();
		size_t pri_len = priv_exp.MinEncodedSize();
		pri_outbuf.resize(pri_len);
		priv_exp.Encode((uint8_t *)pri_outbuf.data(), pri_outbuf.size(), Integer::UNSIGNED);

		ECP::Point pub_point = pub_key.GetGroupParameters().ExponentiateBase(priv_exp);
		SecByteBlock y(pub_key.GetGroupParameters().GetCurve().EncodedPointSize(true /*compressed*/));
		pub_key.GetGroupParameters().GetCurve().EncodePoint(y, pub_point, true /*compressed*/);
		pub_outbuf.assign((const char*)y.data(), y.size());
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Ecies::MemKey::encrypt(const std::string& plain_inbuf, std::string& chiper_outbuf, const std::string& pubkey_inbuf)
{
	chiper_outbuf.clear();

	try
	{
		AutoSeededRandomPool rnd;

		StringSource pub_source(pubkey_inbuf, true);
		ECIES<ECP>::Encryptor encryptor;

		encryptor.AccessKey().AccessGroupParameters().Initialize(ASN1::secp256k1());
		ECP::Point point;
		encryptor.GetKey().GetGroupParameters().GetCurve().DecodePoint(point, pub_source, (size_t)pub_source.MaxRetrievable());
		encryptor.AccessKey().SetPublicElement(point);
		encryptor.AccessKey().ThrowIfInvalid(rnd, 3);

		StringSource source(plain_inbuf, true,
			new PK_EncryptorFilter(rnd, encryptor,
			new StringSink(chiper_outbuf)
			)
			);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Ecies::MemKey::decrypt(const std::string& chiper_inbuf, std::string& plain_outbuf, const std::string& prikey_inbuf)
{
	plain_outbuf.clear();

	try
	{
		AutoSeededRandomPool rnd;

		StringSource pri_source(prikey_inbuf, true);
		Integer priv_exp;
		priv_exp.Decode(pri_source, (size_t)pri_source.MaxRetrievable(), Integer::UNSIGNED);

		ECIES<ECP>::Decryptor decryptor;
		decryptor.AccessKey().Initialize(ASN1::secp256k1(), priv_exp);
		decryptor.AccessKey().ThrowIfInvalid(rnd, 3);

		StringSource source(chiper_inbuf, true,
			new PK_DecryptorFilter(rnd, decryptor,
			new StringSink(plain_outbuf)
			)
			);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Ecies::FileKey::gen_key(const std::string& pri_filepath_instr, const std::string pub_filepath_instr)
{
	try
	{
		AutoSeededRandomPool rnd(false, keysize(KBN_ECC_256));

		ECIES<ECP>::PrivateKey  pri_key;
		ECIES<ECP>::PublicKey   pub_key;

		pri_key.Initialize(rnd, ASN1::secp256k1());
		pri_key.MakePublicKey(pub_key);

		//验证
		pri_key.ThrowIfInvalid(rnd, 3);
		pub_key.ThrowIfInvalid(rnd, 3);

		pri_key.AccessGroupParameters().SetEncodeAsOID(true);
		pri_key.AccessGroupParameters().SetPointCompression(true);

		pub_key.AccessGroupParameters().SetEncodeAsOID(true);
		pub_key.AccessGroupParameters().SetPointCompression(true);

		FileSink pri_sink(pri_filepath_instr.c_str());
		pri_key.Save(pri_sink);

		FileSink pub_sink(pub_filepath_instr.c_str());
		pub_key.Save(pub_sink);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Ecies::FileKey::encrypt(const std::string& plain_inbuf, std::string& chiper_outbuf, const std::string& pubkey_filepath_instr)
{
	chiper_outbuf.clear();
	try
	{
		AutoSeededRandomPool rnd;

		FileSource pub_source(pubkey_filepath_instr.c_str(), true);
		ECIES<ECP>::Encryptor encryptor;
		encryptor.AccessKey().Load(pub_source);
		encryptor.AccessKey().ThrowIfInvalid(rnd, 3);

		StringSource source(plain_inbuf, true,
			new PK_EncryptorFilter(rnd, encryptor,
			new StringSink(chiper_outbuf)
			)
			);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Ecies::FileKey::decrypt(const std::string& chiper_inbuf, std::string& plain_outbuf, const std::string& prikey_filepath_instr)
{
	plain_outbuf.clear();
	try
	{
		AutoSeededRandomPool rnd;

		FileSource pri_source(prikey_filepath_instr.c_str(), true);
		ECIES<ECP>::Decryptor decryptor;
		decryptor.AccessKey().Load(pri_source);
		decryptor.AccessKey().ThrowIfInvalid(rnd, 3);

		StringSource source(chiper_inbuf, true,
			new PK_DecryptorFilter(rnd, decryptor,
			new StringSink(plain_outbuf)
			)
			);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

//验证
//使用secp256k1曲线
bool	Gaz::Crypt::Ecdsa::MemKey::gen_key(std::string& pri_outbuf, std::string& pub_outbuf)
{
	pri_outbuf.clear();
	pub_outbuf.clear();
	try
	{
		AutoSeededRandomPool rnd(false, keysize(KBN_ECC_256));

		ECDSA<ECP, SHA256>::PrivateKey  pri_key;
		ECDSA<ECP, SHA256>::PublicKey   pub_key;

		pri_key.Initialize(rnd, ASN1::secp256k1());
		pri_key.MakePublicKey(pub_key);

		//验证
		pri_key.ThrowIfInvalid(rnd, 3);
		pub_key.ThrowIfInvalid(rnd, 3);

		const Integer priv_exp = pri_key.GetPrivateExponent();
		size_t pri_len = priv_exp.MinEncodedSize();
		pri_outbuf.resize(pri_len);
		priv_exp.Encode((uint8_t *)pri_outbuf.data(), pri_outbuf.size(), Integer::UNSIGNED);

		ECP::Point pub_point = pub_key.GetGroupParameters().ExponentiateBase(priv_exp);
		SecByteBlock y(pub_key.GetGroupParameters().GetCurve().EncodedPointSize(true /*compressed*/));
		pub_key.GetGroupParameters().GetCurve().EncodePoint(y, pub_point, true /*compressed*/);
		pub_outbuf.assign((const char*)y.data(), y.size());
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Ecdsa::MemKey::sign(const std::string& plain_inbuf, std::string& sign_inbuf, const std::string& prikey_inbuf)
{
	sign_inbuf.clear();
	try
	{
		AutoSeededRandomPool rnd;

		StringSource pri_source(prikey_inbuf, true);
		Integer priv_exp;
		priv_exp.Decode(pri_source, (size_t)pri_source.MaxRetrievable(), Integer::UNSIGNED);
			
		ECDSA<ECP, SHA256>::Signer signer;
		signer.AccessKey().Initialize(ASN1::secp256k1(), priv_exp);
		signer.AccessKey().ThrowIfInvalid(rnd, 3);
		
		StringSource source(plain_inbuf, true,
			new SignerFilter(rnd, signer,
			new StringSink(sign_inbuf)
			) // SignerFilter
			); // StringSource
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Ecdsa::MemKey::verify(const std::string& chiper_inbuf, const std::string& sign_inbuf, const std::string& pubkey_inbuf)
{
	bool result = false;

	try
	{
		AutoSeededRandomPool rnd;

		ECDSA<ECP, SHA256>::Verifier verifier;

		verifier.AccessKey().AccessGroupParameters().Initialize(ASN1::secp256k1());
		ECP::Point point;
		StringSource pub_source(pubkey_inbuf, true);
		verifier.AccessKey().GetGroupParameters().GetCurve().DecodePoint(point, pub_source, (size_t)pub_source.MaxRetrievable());
		verifier.AccessKey().SetPublicElement(point);
		verifier.AccessKey().ThrowIfInvalid(rnd, 3);

		StringSource ssources(sign_inbuf + chiper_inbuf, true,
			new SignatureVerificationFilter(verifier,
			new ArraySink((byte*)&result, sizeof(result)),
			SignatureVerificationFilter::DEFAULT_FLAGS
			) // SignatureVerificationFilter
			);
	}
	catch (...)
	{
		return false;
	}
	return result;	
}

bool	Gaz::Crypt::Ecdsa::FileKey::gen_key(const std::string& pri_filepath_instr, const std::string pub_filepath_instr)
{
	try
	{
		AutoSeededRandomPool rnd(false, keysize(KBN_ECC_256));

		ECIES<ECP, SHA256>::PrivateKey  pri_key;
		ECIES<ECP, SHA256>::PublicKey   pub_key;

		pri_key.Initialize(rnd, ASN1::secp256k1());
		pri_key.MakePublicKey(pub_key);

		//验证
		pri_key.ThrowIfInvalid(rnd, 3);
		pub_key.ThrowIfInvalid(rnd, 3);

		pri_key.AccessGroupParameters().SetEncodeAsOID(true);
		pri_key.AccessGroupParameters().SetPointCompression(true);

		pub_key.AccessGroupParameters().SetEncodeAsOID(true);
		pub_key.AccessGroupParameters().SetPointCompression(true);

		FileSink pri_sink(pri_filepath_instr.c_str());
		pri_key.Save(pri_sink);

		FileSink pub_sink(pub_filepath_instr.c_str());
		pub_key.Save(pub_sink);
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Ecdsa::FileKey::sign(const std::string& plain_inbuf, std::string& sign_inbuf, const std::string& prikey_filepath_instr)
{
	try
	{
		AutoSeededRandomPool rnd;

		FileSource pri_source(prikey_filepath_instr.c_str(), true);
		ECDSA<ECP, SHA256>::PrivateKey   pri_key;
		pri_key.Load(pri_source);

		ECDSA<ECP, SHA256>::Signer signer(pri_key);

		StringSource source(plain_inbuf, true,
			new SignerFilter(rnd, signer,
			new StringSink(sign_inbuf)
			) // SignerFilter
			); // StringSource
	}
	catch (...)
	{
		return false;
	}
	return true;
}

bool	Gaz::Crypt::Ecdsa::FileKey::verify(const std::string& chiper_inbuf, const std::string& sign_inbuf, const std::string& pubkey_filepath_instr)
{
	bool result = false;
	
	try
	{
		FileSource pub_source(pubkey_filepath_instr.c_str(), true);
		ECDSA<ECP, SHA256>::PublicKey   pub_key;
		pub_key.Load(pub_source);

		ECDSA<ECP, SHA256>::Verifier verifier(pub_key);

		StringSource ssources(sign_inbuf + chiper_inbuf, true,
			new SignatureVerificationFilter(verifier,
			new ArraySink((byte*)&result, sizeof(result)),
			SignatureVerificationFilter::DEFAULT_FLAGS
			) // SignatureVerificationFilter
			);
	}
	catch (...)
	{
		return false;
	}
	return result;
}

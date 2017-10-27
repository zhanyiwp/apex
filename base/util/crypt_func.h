#ifndef CRYPT_FUNC_H_
#define CRYPT_FUNC_H_

#include <stdint.h>
#include <string>

namespace Gaz
{
	//不仅仅是加密，还包括了16/64编码转换
	//注意：参数使用std::string，并不一定意味着是str字符串，其有可能是buff数据，请参照变量名
	//特别提醒，如果是char*自然隐式转型为std::string的，如果原先char*实际是一个byte*的，会自动按照\0截断
	//默认对内存数据进行操作，作为对照的是加file前缀德针对文件进行的操作
	namespace Crypt
	{
		enum KeyBitsNum
		{
			KBN_NONE = 0,

			KBN_AES_128 = 1,
			KBN_AES_192 = 2,
			KBN_AES_256 = 3,

			KBN_ECC_160 = 11,//保留
			KBN_ECC_224 = 12,//保留
			KBN_ECC_256 = 13,//保留
			KBN_ECC_384 = 14,//保留
			KBN_ECC_512 = 15,//保留
		};

		namespace Base16	//Base16编码
		{
			bool	encode(const std::string& inbuf, std::string& outstr);
			bool	decode(const std::string& instr, std::string& outbuf);
		}
		namespace Base64	//Base64编码
		{
			//特殊的不添加后缀“==”以及Web安全字母请参照https://www.cryptopp.com/wiki/Base64Encoder			
			bool	encode(const std::string& inbuf, std::string& outstr);
			bool	decode(const std::string& instr, std::string& outbuf);
		}

		namespace Md5	
		{
			//此处file指加解密的文件对象
			bool	hash(const std::string& inbuf, std::string& outbuf);
			bool	file_hash(const std::string& filepath_instr, std::string& outbuf);
		}

		namespace Sha256
		{
			//此处file指加解密的文件对象
			bool	hash(const std::string& inbuf, std::string& outbuf);
			bool	file_hash(const std::string& filepath_instr, std::string& outbuf);	
		}
		
		namespace Aes	//对称加密
		{
			namespace CbcMode
			{
				//iv如果需要随机生成，也可以通过generate_key生成；iv将会调整长度为128位：不够的补0，多的截断
				bool	gen_key(std::string& outbuf, KeyBitsNum kbn = KBN_AES_256);
				bool	encrypt(const std::string& plain_inbuf, std::string& chiper_outbuf, const std::string& key_inbuf, const std::string& iv_inbuf = "");
				bool	decrypt(const std::string& chiper_inbuf, std::string& plain_outbuf, const std::string& key_inbuf, const std::string& iv_inbuf = "");
			}
		}

		namespace Ecies		//非对称加密
		{
			//使用secp256k1曲线
			//MemKey使用 “真正的”公钥私钥，私钥长度为32字节，公钥长度为32+1字节（压缩）
			//FileKey使用“Dump”的公钥私钥，包括keyinfo + “真正的”公钥私钥

			namespace MemKey
			{
				bool	gen_key(std::string& pri_outbuf, std::string& pub_outbuf);
				bool	encrypt(const std::string& plain_inbuf, std::string& chiper_outbuf, const std::string& pubkey_inbuf);
				bool	decrypt(const std::string& chiper_inbuf, std::string& plain_outbuf, const std::string& prikey_inbuf);
			}

			namespace FileKey
			{
				bool	gen_key(const std::string& pri_filepath_instr, const std::string pub_filepath_instr);
				bool	encrypt(const std::string& plain_inbuf, std::string& chiper_outbuf, const std::string& pubkey_filepath_instr);
				bool	decrypt(const std::string& chiper_inbuf, std::string& plain_outbuf, const std::string& prikey_filepath_instr);
			}
		}
		
		namespace Ecdsa		//验证
		{
			//使用secp256k1曲线
			//MemKey使用 “真正的”公钥私钥，私钥长度为32字节，公钥长度为32+1字节（压缩）
			//FileKey使用“Dump”的公钥私钥，包括keyinfo + “真正的”公钥私钥

			namespace MemKey
			{
				bool	gen_key(std::string& pri_outbuf, std::string& pub_outbuf);
				bool	sign(const std::string& plain_inbuf, std::string& sign_outbuf, const std::string& prikey_inbuf);
				bool	verify(const std::string& chiper_inbuf, const std::string& sign_inbuf, const std::string& pubkey_inbuf);
			}

			namespace FileKey
			{
				bool	gen_key(const std::string& pri_filepath_instr, const std::string pub_filepath_instr);
				bool	sign(const std::string& plain_inbuf, std::string& sign_outbuf, const std::string& prikey_filepath_instr);
				bool	verify(const std::string& chiper_inbuf, const std::string& sign_inbuf, const std::string& pubkey_filepath_instr);
			}
		}
	}
}

#endif // CRYPT_FUNC_H_ 

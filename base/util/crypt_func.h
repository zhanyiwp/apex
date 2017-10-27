#ifndef CRYPT_FUNC_H_
#define CRYPT_FUNC_H_

#include <stdint.h>
#include <string>

namespace Gaz
{
	//�������Ǽ��ܣ���������16/64����ת��
	//ע�⣺����ʹ��std::string������һ����ζ����str�ַ��������п�����buff���ݣ�����ձ�����
	//�ر����ѣ������char*��Ȼ��ʽת��Ϊstd::string�ģ����ԭ��char*ʵ����һ��byte*�ģ����Զ�����\0�ض�
	//Ĭ�϶��ڴ����ݽ��в�������Ϊ���յ��Ǽ�fileǰ׺������ļ����еĲ���
	namespace Crypt
	{
		enum KeyBitsNum
		{
			KBN_NONE = 0,

			KBN_AES_128 = 1,
			KBN_AES_192 = 2,
			KBN_AES_256 = 3,

			KBN_ECC_160 = 11,//����
			KBN_ECC_224 = 12,//����
			KBN_ECC_256 = 13,//����
			KBN_ECC_384 = 14,//����
			KBN_ECC_512 = 15,//����
		};

		namespace Base16	//Base16����
		{
			bool	encode(const std::string& inbuf, std::string& outstr);
			bool	decode(const std::string& instr, std::string& outbuf);
		}
		namespace Base64	//Base64����
		{
			//����Ĳ���Ӻ�׺��==���Լ�Web��ȫ��ĸ�����https://www.cryptopp.com/wiki/Base64Encoder			
			bool	encode(const std::string& inbuf, std::string& outstr);
			bool	decode(const std::string& instr, std::string& outbuf);
		}

		namespace Md5	
		{
			//�˴�fileָ�ӽ��ܵ��ļ�����
			bool	hash(const std::string& inbuf, std::string& outbuf);
			bool	file_hash(const std::string& filepath_instr, std::string& outbuf);
		}

		namespace Sha256
		{
			//�˴�fileָ�ӽ��ܵ��ļ�����
			bool	hash(const std::string& inbuf, std::string& outbuf);
			bool	file_hash(const std::string& filepath_instr, std::string& outbuf);	
		}
		
		namespace Aes	//�ԳƼ���
		{
			namespace CbcMode
			{
				//iv�����Ҫ������ɣ�Ҳ����ͨ��generate_key���ɣ�iv�����������Ϊ128λ�������Ĳ�0����Ľض�
				bool	gen_key(std::string& outbuf, KeyBitsNum kbn = KBN_AES_256);
				bool	encrypt(const std::string& plain_inbuf, std::string& chiper_outbuf, const std::string& key_inbuf, const std::string& iv_inbuf = "");
				bool	decrypt(const std::string& chiper_inbuf, std::string& plain_outbuf, const std::string& key_inbuf, const std::string& iv_inbuf = "");
			}
		}

		namespace Ecies		//�ǶԳƼ���
		{
			//ʹ��secp256k1����
			//MemKeyʹ�� �������ġ���Կ˽Կ��˽Կ����Ϊ32�ֽڣ���Կ����Ϊ32+1�ֽڣ�ѹ����
			//FileKeyʹ�á�Dump���Ĺ�Կ˽Կ������keyinfo + �������ġ���Կ˽Կ

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
		
		namespace Ecdsa		//��֤
		{
			//ʹ��secp256k1����
			//MemKeyʹ�� �������ġ���Կ˽Կ��˽Կ����Ϊ32�ֽڣ���Կ����Ϊ32+1�ֽڣ�ѹ����
			//FileKeyʹ�á�Dump���Ĺ�Կ˽Կ������keyinfo + �������ġ���Կ˽Կ

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

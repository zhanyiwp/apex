/*
 * util_crypt_aes.h
 *
 *  Created on: 2011-6-16
 *      Author: echo
 *      Brief: a simple implement for AES algorithm. note: block size is 16byte(128bit) only.
 *      	The function aes_set_key(), aes_encrypt(), aes_decrypt() are open-source code from Christophe Devine.
 *      	The ecb/cbc/cbc-md5 method are written by myself.
 */

/*
 *  FIPS-197 compliant AES implementation
 *
 *  Copyright (C) 2001-2004  Christophe Devine
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef CRYPT_AES_H_
#define CRYPT_AES_H_


#include <stdint.h>
//commet this macro defination if you compile on IOS
//#define AES_USING_B128_OPTIMIZATION
namespace CRYPT_AES
{

typedef struct
{
    uint32_t erk[64];     /* encryption round keys */
    uint32_t drk[64];     /* decryption round keys */
    int nr;             /* number of rounds */
}
aes_context;

int  aes_set_key( aes_context *ctx, uint8_t *key, int nbits );
void aes_encrypt( aes_context *ctx, uint8_t input[16], uint8_t output[16] );
void aes_decrypt( aes_context *ctx, uint8_t input[16], uint8_t output[16] );

/**
 * a ecb mode implementation for aes encrypting work flow.
 * @param	key			encrypt key
 * @param	nbits		bits length of key. supported values are 128,192,256.
 * @param	input		the content to be encrypted
 * @param	input_len	the length of the input content. it can be any value which is not a multiple of 16.
 * @param	output		store the encrypted content
 * @param	output_len	specified the max length of output. it must be 32 bytes greater than input_len.
 * return the length of encrypted data bytes on success. otherwise, -1 was returned.
 * note: when success, the return value is always a mutiple of 16byte, it's 32 at least
 * note: padding will happen when the input_len is not a multiple of 16byte, so output_len is requested to be greater than input_len.
 */
int aes_encrypt_ecb(uint8_t *key, int nbits, uint8_t* input, uint32_t input_len, uint8_t* output, uint32_t output_len);

/**
 * a ecb mode implementation for aes decrypting work flow.
 * @param	key			encrypt key
 * @param	nbits		bits length of key. supported values are 128,192,256.
 * @param	input		the content to be decrypted
 * @param	input_len	the length of the input content, it's 32 at least and must be a multiple of 16byte.
 * @param	output		store the decrypted content
 * @param	output_len	specified the max length of output. it can be 16 bytes less than input_len.
 * return the length of decrypted plain data bytes on success. otherwise, -1 was returned.
 */
int aes_decrypt_ecb(uint8_t *key, int nbits, uint8_t* input, uint32_t input_len, uint8_t* output, uint32_t output_len);

/**
 * a cbc mode implementation for aes encrypting work flow.
 * @param	key			encrypt key
 * @param	nbits		bits length of key. supported values are 128,192,256.
 * @param	input		the content to be encrypted
 * @param	input_len	the length of the input content. it can be any value which is not a multiple of 16 and must be greater the 0.
 * @param	output		store the encrypted content
 * @param	output_len	specified the max length of output.
 * return the length of encrypted data bytes on success. otherwise, -1 was returned.
 * note: when success, the return value is always a mutiple of 16byte.
 * note: padding will happen when the input_len is not a multiple of 16byte.
 * note: a 16 byte IV(Initial Vector) of zero value was used.
 */
int aes_encrypt_cbc(uint8_t *key, int nbits, uint8_t* input, uint32_t input_len, uint8_t* output, uint32_t output_len);

/**
 * a cbc mode implementation for aes decrypting work flow.
 * @param	key			encrypt key
 * @param	nbits		bits length of key. supported values are 128,192,256.
 * @param	input		the content to be decrypted
 * @param	input_len	the length of the input content, it's 48 at least and must be a multiple of 16byte.
 * @param	output		store the decrypted content
 * @param	output_len	specified the max length of output. it can be 16 bytes less than input_len.
 * return the length of decrypted plain data bytes on success. otherwise, -1 was returned.
 */
int aes_decrypt_cbc(uint8_t *key, int nbits, uint8_t* input, uint32_t input_len, uint8_t* output, uint32_t output_len);


};

#endif /* CRYPT_AES_H_ */


#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include "util_crypt_aes.h"

using namespace CRYPT_AES;

typedef struct tagKey
{
    uint64_t u64_a;
    uint64_t u64_b;
    uint64_t u64_c;
    uint64_t u64_d;
}tagKey;

int test_str_aes()
{
	char szKey[17] = "abcdefghigklmnop";
	char szInput1[33] = "12345678901234567890123456789012";
	char szOutput1[64] = {0};
	uint32_t uOutputLen1 = 0;

	char szInput2[64] = {0};
	char szOutput2[64] = {0};
	uint32_t uOutputLen2 = 0;
	uint32_t uMaxOutputlen = sizeof(szOutput1);
	int nRet = 0;
	printf("szKey:%s, uKeyLen:%d, szInput:%s, uInputLen:%d\n", szKey, (int)strlen(szKey), szInput1, (int)strlen(szInput1));

	nRet = aes_encrypt_cbc((uint8_t *)szKey, 128, (uint8_t *)szInput1, 32, (uint8_t*)szOutput1, uMaxOutputlen);
	if (nRet == -1)
	{
		printf("aes_encrypt_cbc exec failed, nRet:%d\n", nRet);
		return -1;
	}
	else
	{
		printf("aes_encrypt_cbc exec succeed, nRet:%d\n", nRet);
		uOutputLen1 = nRet;
	}

	memcpy(szInput2, szOutput1, uOutputLen1);
	nRet = aes_decrypt_cbc((uint8_t *)szKey, 128, (uint8_t *)szInput2, uOutputLen1, (uint8_t*)szOutput2, uMaxOutputlen);
	if (nRet == -1)
	{
		printf("aes_decrypt_cbc exec failed, nRet:%d\n", nRet);
		return -1;
	}
	else
	{
            printf("aes_decrypt_cbc exec succeed, nRet:%d, szOutPut2:%s\n", nRet, szOutput2);
		uOutputLen2 = nRet;
	}

    return 0;
}

int test_str_aes_pressure()
{
	char szKey[17] = "abcdefghigklmnop";
	char szInput1[33] = "12345678901234567890123456789012";
	char szOutput1[64] = {0};
	uint32_t uOutputLen1 = 0;

	char szInput2[64] = {0};
	char szOutput2[64] = {0};
	uint32_t uOutputLen2 = 0;
	uint32_t uMaxOutputlen = sizeof(szOutput1);
	int nRet = 0;
    struct timeval tpstart, tpend;
    float timeuse;

    
	printf("szKey:%s, uKeyLen:%d, szInput1:%s, uInputLen1:%d\n", szKey, (int)strlen(szKey), szInput1, (int)strlen(szInput1));
    srand(time(0));
    gettimeofday(&tpstart, NULL);

    for (int i = 0; i < 10000; i++)
    {
        int *pn = (int *)(szInput1 + 28);
        *pn += rand() % 1000;

        nRet = aes_encrypt_cbc((uint8_t *)szKey, 128, (uint8_t *)szInput1, 32, (uint8_t*)szOutput1, uMaxOutputlen);
        if (nRet == -1)
        {
            printf("aes_encrypt_cbc exec failed, nRet:%d\n", nRet);
            return -1;
        }
        else
        {
            printf("aes_encrypt_cbc exec succeed, nRet:%d\n", nRet);
            uOutputLen1 = nRet;
        }

        memcpy(szInput2, szOutput1, uOutputLen1);
        nRet = aes_decrypt_cbc((uint8_t *)szKey, 128, (uint8_t *)szInput2, uOutputLen1, (uint8_t*)szOutput2, uMaxOutputlen);
        if (nRet == -1)
        {
            printf("aes_decrypt_cbc exec failed, nRet:%d\n", nRet);
            return -1;
        }
        else
        {
            printf("aes_decrypt_cbc exec succeed, nRet:%d\n", nRet);
            uOutputLen2 = nRet;
        }

    }

    gettimeofday(&tpend, NULL);

    timeuse = 1000000* (tpend.tv_sec - tpstart.tv_sec) + tpend.tv_usec - tpstart.tv_usec; 
    timeuse /= 1000000;
    printf("Used Time:%f\n", timeuse);

    return 0;
}

int main()
{
    test_str_aes_pressure();
    return 0;
}

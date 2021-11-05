#include <string.h>
#include <stdio.h>

#include "mbedtls/aes.h"

const unsigned char plaintext[16] = {"ON"};
unsigned char encrypted [32] = {0x4A, 0x8C, 0x8C, 0xE6, 0xBB, 0x9F, 0x83, 0x91, 0xCB, 0x96, 0xF8, 0x64, 0x97, 0x68, 0x0E, 0xFA};
unsigned char decrypted[32];
const unsigned char key[] = ("ALL DOGS ARE NOT");

void app_main()
{
	int i;
	mbedtls_aes_context ctx;
	mbedtls_aes_init(&ctx);
	mbedtls_aes_setkey_enc( &ctx, key, 128);
	mbedtls_aes_setkey_dec( &ctx, key, 128);
	
/*	mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, plaintext, encrypted);
       for( i = 0; i < 128; i++ )
	{
		printf( "%02x[%c]%c", encrypted[i], (encrypted[i]>31)?encrypted[i]:' ', ((i&0xf)!=0xf)?' ':'\n' );
	}              	
*/	
	
	mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_DECRYPT, encrypted, decrypted);
	 for( i = 0; i < 128; i++ )
	{
		printf(" \n decrypted[%d] = %d \n", i, decrypted[i]);
	}   

}

//#include <openssl/md5.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "./MD5.h"
#include "../util/logwriter.h"

#define TEA_LOOP 4

static void GMD5(const unsigned char *data, uint32_t len, unsigned char *out)
{
    class MD5 md5(data, len);
    md5.output((char *)out);
}

static int code(unsigned char *in, unsigned char *out, uint32_t Pos, uint32_t key[])
{
    uint32_t i, y, z, sum, n, delta = 0x9e3779b9;
    uint32_t *inNow, *outPre, *outNow, *inPre;

	inNow = (uint32_t *)(in + Pos);
	outNow = (uint32_t *)(out + Pos);
	if (Pos > 0) {
		outPre = (uint32_t *)(out + Pos - 8);
		inNow[0] = inNow[0] ^ outPre[0];
		inNow[1] = inNow[1] ^ outPre[1];
	}
	y = ntohl(inNow[0]);
	z = ntohl(inNow[1]);
	sum = 0;
	n = 1 << TEA_LOOP;
	for (i = 0; i < n; i++) {
		sum += delta;
		y += ((z << 4) + key[0]) ^ (z + sum) ^ ((z >> 5) + key[1]); 
		z += ((y << 4) + key[2]) ^ (y + sum) ^ ((y >> 5) + key[3]);  
	}
	outNow[0] = htonl(y);
	outNow[1] = htonl(z);
	if (Pos > 0) {
		inPre = (uint32_t *)(in + Pos - 8);
		outNow[0] = outNow[0] ^ inPre[0];
		outNow[1] = outNow[1] ^ inPre[1];
	}
	return 0;
}

static int decode(unsigned char *in, unsigned char *out, uint32_t Pos, uint32_t key[])
{
    uint32_t i, y, z, sum, n, delta = 0x9e3779b9;
    uint32_t *inNow, *outPre, *outNow;

	inNow = (uint32_t *)(in + Pos);
	outNow = (uint32_t *)(out + Pos);
	if (Pos > 0) {
		outPre = (uint32_t *)(out + Pos - 8);
		outNow[0] = inNow[0] ^ outPre[0];
		outNow[1] = inNow[1] ^ outPre[1];
	}
	else
		memcpy(out, in, 8);
	y = ntohl(outNow[0]);
	z = ntohl(outNow[1]);
	sum = delta << TEA_LOOP;
	n = 1 << TEA_LOOP;
	for (i = 0; i < n; i++) {
		z -= ((y << 4) + key[2]) ^ (y + sum) ^ ((y >> 5) + key[3]);  
		y -= ((z << 4) + key[0]) ^ (z + sum) ^ ((z >> 5) + key[1]); 
		sum -= delta;
	}
	outNow[0] = htonl(y);
	outNow[1] = htonl(z);
	return 0;
}

//out buffer size should be at lease len + 17
int tea_enc(char *in, uint32_t len, char *out, char *key_str, uint32_t key_len)
{
    uint32_t i, pos, outlen, key[4];
    unsigned char *plain, md[16];

	pos = (len + 10) % 8;
	if (pos) pos = 8 - pos;
	outlen = pos + len + 10;
    
	//MD5((const unsigned char*) key_str, strlen(key_str), md);
    //MD5((const unsigned char*) key_str, key_len, md);
    GMD5((const unsigned char*) key_str, key_len, md);
	for (i = 0; i < 4; i++)
		key[i] = ntohl(*((uint32_t *)md + i));
	plain = (unsigned char *) malloc(outlen);
	//srand(1);
	//plain[0] = (rand() & 0xf8) | pos;
	for (i = 0; i < pos + 3; i++)
		plain[i] = rand() & 0xff;
	memcpy(plain + pos + 3, in, len);
	for (i = len + pos + 3; i < outlen - 1; i++)
		plain[i] = 0;
	plain[outlen - 1] = pos;
	for (i = 0; i < outlen; i += 8)
		code(plain, (unsigned char*) out, i, key);
	free(plain);
	return outlen;
}

// out buffer size should be same as len.
int tea_dec(char *in, uint32_t len, char *out, char *key_str, uint32_t key_len)
{
    uint32_t i, pos, outlen, key[4];
    //unsigned char *plain;
    unsigned char md[16];

	if (len % 8 != 0 || len < 16) {
		::writelog(InfoType,"tea_dec len error,len = %lu",len);
		return -1;
	}
    
    //MD5((const unsigned char*) key_str, strlen(key_str), md);
    //MD5((const unsigned char*) key_str, key_len, md);
    GMD5((const unsigned char*) key_str, key_len, md);
	for (i = 0; i < 4; i++)
		key[i] = ntohl(*((uint32_t *)md + i));
	for (i = 0; i < len; i += 8)
		decode((unsigned char*) in, (unsigned char*) out, i, key);
	for (i = 8; i < len; i += 4)
		*(uint32_t *)(out + i) = *(uint32_t *)(out + i) ^ *(uint32_t *)(in + i - 8);
	//HexShow(out, len);
	pos = out[len - 1];
	if (pos > 7) return -2;
	for (i = len - 7; i < len - 1; i++) if (out[i]) return -2;
	outlen = len - pos - 10;
	memmove(out, out + pos + 3, outlen + 1);
	return outlen;
}


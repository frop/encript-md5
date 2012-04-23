#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

// 64 bytes = 512 bits
#define TAM_BLOCO 64

// indice do primeiro byte do tamanho da mensagem dentro de um bloco
#define MSG_LENGTH_CHUNK_INDEX ((512 - 64) / 8)

#define initH(h) {\
	h[0] = 0x67452301;\
	h[1] = 0xEFCDAB89;\
	h[2] = 0x98BADCFE;\
 	h[3] = 0x10325476;\
}

#define initK(k) {\
	unsigned int i;\
	for(i=0; i<64; i++)\
		k[i] = floor(fabs(sin(i+1)) * pow(2, 32));\
}

#define appendBitOne(v, i) ( v[i] = 0x80 )
#define leftRotate(x, c) ((x << c) | (x >> (32 - c)))

typedef unsigned short int boolean;
#define TRUE 1
#define FALSE 0

unsigned int h[4];

unsigned int r[64] =
	{
		7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22, 
		5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
		4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
		6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21
	};
	
/*unsigned int k[64] =
	{
		0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
		0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
		0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x2441453,0xd8a1e681,0xe7d3fbc8,
		0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
		0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
		0x289b7ec6,0xeaa127fa,0xd4ef3085,0x4881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
		0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
		0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391
	};
*/
unsigned int k[64];

void appendBitsZero(unsigned char *v, unsigned int i, unsigned int j){
	for(i; i<j; i++)
		v[i] = 0x0;
}

void appendMsgLength(unsigned char *v, unsigned long int n){
	int i;
	
	for(i=56; i<64; i++, n = n >> 8)
		v[i] = n & 0xFF;
}

void printMsgChunk(unsigned char *v){
	int i;
	
	for(i=63; i>-1; i--)
		printf("%x-", v[i]);
	puts("");
}

void printOutputMD5(unsigned int *h){
	int i,j;
	
	for(i=0;i<4;i++)
		for(j=0;j<8;j++)
			if ( j%2 == 0 )
				printf("%x", ((h[i] >> 4*j) & 0xF0) >> 4);
			else
				printf("%x", (h[i] >> 4*(j-1)) & 0x0F);
	puts("");
}

void breakChunk(unsigned char *chunk, unsigned int *w){
	int i,j;

	for(i=0, j=0; i<16; i++, j+=4){
		w[i] = chunk[j+3] << 24 | chunk[j+2] << 16 | chunk[j+1] << 8 | chunk[j];
	}
	
// 	printMsgChunk(chunk);
}

void md5_bloco(unsigned char *chunk){
	unsigned int i;
	
	unsigned int a,b,c,d, f,g, temp;
	unsigned int w[16];
	
	breakChunk(chunk, w);

	a = h[0];
	b = h[1];
	c = h[2];
	d = h[3];
	
	for(i=0; i<64; i++){
		if (i < 16){
			f = (b & c) | ((~b) & d );
			g = i;
		}else if(i < 32){
			f = (d & b) | (~(d) & c);
			g = (5*i + 1) % 16;
		}else if (i < 48){
			f = b ^ c ^ d;
			g = (3*i + 5) % 16;
		}else{
			f = c ^(b | ~(d));
			g = (7*i) % 16;
		}
		
		temp = d;
		d = c;
		c = b;
		b = b + leftRotate((a + f + k[i] + w[g]), r[i]);
		a = temp;
	}
	
	h[0] += a;
	h[1] += b;
	h[2] += c;
	h[3] += d;

}

void main(int argc, char *argv[]){
	int i, fd;
	unsigned int chunkBitSize;
	unsigned long int n, msgBitSize=0;
	unsigned char chunk[TAM_BLOCO+1];
	boolean padded = FALSE;	

	FILE *arq_entrada;

	if ( argc != 2 ){
		if ( argc == 1 )
			arq_entrada = stdin;
		else{
			printf ("Uso correto:\n\t%s <ARQUIVO_ENTRADA>\n", argv[0]);
			exit(1);
		}
	}

	if ( argc != 1 )
		if ( !(arq_entrada = fopen (argv[1], "r")) ){
			printf ("O arquivo de entrada <%s> nao pode ser aberto.\n", argv[1]);
			exit(2);
		}

	fd = fileno(arq_entrada);

	bzero (chunk, TAM_BLOCO+1);
	
	initH(h);
	initK(k);

	while ( (n = fread(chunk, 1, TAM_BLOCO, arq_entrada)) ){
	//while ((n = read (fd, chunk, TAM_BLOCO))){
		msgBitSize += (n*8);
		if (n == 64){

			md5_bloco(chunk);

		}else{
			chunkBitSize = n*8;
			padded = TRUE;

			if(chunkBitSize <= (512 - (64 + 1))){

				appendBitOne(chunk, n);
				appendBitsZero(chunk, n+1, MSG_LENGTH_CHUNK_INDEX);
				appendMsgLength(chunk, msgBitSize);
				md5_bloco(chunk);

			}else{ // caso em que o preenchimento resulta em 2 blocos de 512 bits
				unsigned char extraChunk[TAM_BLOCO];
				
				appendBitOne(chunk, n);
				appendBitsZero(chunk, n+1, 64);
				
				appendBitsZero(extraChunk, 0, MSG_LENGTH_CHUNK_INDEX);
				appendMsgLength(extraChunk, msgBitSize);
				
				md5_bloco(chunk);
				md5_bloco(extraChunk);
				
			}
		}
		
	}

	// Ocorre quando a entrada em bytes for vazia ou multipla de 64	(512 bits)
	if (!padded){
		appendBitOne(chunk, 0);
		appendBitsZero(chunk, 1, MSG_LENGTH_CHUNK_INDEX);
		appendMsgLength(chunk, msgBitSize);
		md5_bloco(chunk);
		padded = TRUE;
	}
	
	printOutputMD5(h);
	fclose(arq_entrada);
}

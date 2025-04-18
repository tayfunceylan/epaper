/*    SHA implements secure hash functions that can be used for cryptography, data integrity and security purposes.    */
/*    https://github.com/Cathedrow/Cryptosuite/tree/master/Sha                                                         */

#include <string.h>
//#include <avr/io.h>
#include "SHA1.h"
//
#define SHA1_K0 0x5a827999
#define SHA1_K20 0x6ed9eba1
#define SHA1_K40 0x8f1bbcdc
#define SHA1_K60 0xca62c1d6

const uint8_t sha1InitState[]  = {
  0x01,0x23,0x45,0x67, // H0
  0x89,0xab,0xcd,0xef, // H1
  0xfe,0xdc,0xba,0x98, // H2
  0x76,0x54,0x32,0x10, // H3
  0xf0,0xe1,0xd2,0xc3  // H4
};

const static char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void Sha1Class::init(void) {
  memcpy_P(state.b,sha1InitState,HASH_LENGTH);
  byteCount = 0;
  bufferOffset = 0;
}

uint32_t Sha1Class::rol32(uint32_t number, uint8_t bits) {
  return ((number << bits) | (number >> (32-bits)));
}

void Sha1Class::hashBlock() {
  // SHA1 only for now
  uint8_t i;
  uint32_t a,b,c,d,e,t;

  a=state.w[0];
  b=state.w[1];
  c=state.w[2];
  d=state.w[3];
  e=state.w[4];
  for (i=0; i<80; i++) {
    if (i>=16) {
      t = buffer.w[(i+13)&15] ^ buffer.w[(i+8)&15] ^ buffer.w[(i+2)&15] ^ buffer.w[i&15];
      buffer.w[i&15] = rol32(t,1);
    }
    if (i<20) {
      t = (d ^ (b & (c ^ d))) + SHA1_K0;
    } else if (i<40) {
      t = (b ^ c ^ d) + SHA1_K20;
    } else if (i<60) {
      t = ((b & c) | (d & (b | c))) + SHA1_K40;
    } else {
      t = (b ^ c ^ d) + SHA1_K60;
    }
    t+=rol32(a,5) + e + buffer.w[i&15];
    e=d;
    d=c;
    c=rol32(b,30);
    b=a;
    a=t;
  }
  state.w[0] += a;
  state.w[1] += b;
  state.w[2] += c;
  state.w[3] += d;
  state.w[4] += e;
}

void Sha1Class::addUncounted(uint8_t data) {
  buffer.b[bufferOffset ^ 3] = data;
  bufferOffset++;
  if (bufferOffset == BLOCK_LENGTH) {
    hashBlock();
    bufferOffset = 0;
  }
}

size_t Sha1Class::write(uint8_t data) {
  ++byteCount;
  addUncounted(data);
  return 1;
}

void Sha1Class::pad() {
  // Implement SHA-1 padding (fips180-2 ?5.1.1)
  // Pad with 0x80 followed by 0x00 until the end of the block
  addUncounted(0x80);
  while (bufferOffset != 56) addUncounted(0x00);

  // Append length in the last 8 bytes
  addUncounted(0); // We're only using 32 bit lengths
  addUncounted(0); // But SHA-1 supports 64 bit lengths
  addUncounted(0); // So zero pad the top bits
  addUncounted(byteCount >> 29); // Shifting to multiply by 8
  addUncounted(byteCount >> 21); // as SHA-1 supports bitstreams as well as
  addUncounted(byteCount >> 13); // byte.
  addUncounted(byteCount >> 5);
  addUncounted(byteCount << 3);
}

uint8_t* Sha1Class::result(void) {
  // Pad to complete the last block
  pad();

  // Swap byte order back
  for (int i=0; i<5; i++) {
    uint32_t a,b;
    a=state.w[i];
    b=a<<24;
    b|=(a<<8) & 0x00ff0000;
    b|=(a>>8) & 0x0000ff00;
    b|=a>>24;
    state.w[i]=b;
  }
  // Return pointer to hash (20 characters)
  return state.b;
}

#define HMAC_IPAD 0x36
#define HMAC_OPAD 0x5c

void Sha1Class::initHmac(const uint8_t* key, int keyLength) {
  uint8_t i;
  memset(keyBuffer,0,BLOCK_LENGTH);
  if (keyLength > BLOCK_LENGTH) {
    // Hash long keys
    init();
    for (;keyLength--;) write(*key++);
    memcpy(keyBuffer,result(),HASH_LENGTH);
  } else {
    // Block length keys are used as is
    memcpy(keyBuffer,key,keyLength);
  }
  //for (i=0; i<BLOCK_LENGTH; i++) debugHH(keyBuffer[i]);
  // Start inner hash
  init();
  for (i=0; i<BLOCK_LENGTH; i++) {
    write(keyBuffer[i] ^ HMAC_IPAD);
  }
}

char* base64Encode(char *buffer, const uint8_t *data, size_t data_len) {
  int i, len;

  for (i = 0, len = data_len; len > 0; i += 3, len -= 3)
    {
      unsigned long val;

      val = (unsigned long) data[i] << 16;
      if (len > 1)
        val |= (unsigned long) data[i + 1] << 8;
      if (len > 2)
        val |= data[i + 2];

      *buffer++ = (char) *(base64_table + (val >> 18));
      *buffer++ = (char) *(base64_table + ((val >> 12) & 0x3f));
      *buffer++ = (char) *(base64_table + ((val >> 6) & 0x3f));
      *buffer++ = (char) *(base64_table + (val & 0x3f));
    }

  for (; len < 0; len++)
    buffer[len] = '=';

  *buffer = '\0';
  return buffer;
}

void Sha1Class::HmacBase64(char* hmac, char* input) {
    uint8_t i;

    print(input);
    memcpy(innerHash,result(),HASH_LENGTH);
    init();
    for (i=0; i<BLOCK_LENGTH; i++) write(keyBuffer[i] ^ HMAC_OPAD);
    for (i=0; i<HASH_LENGTH; i++) write(innerHash[i]);

    base64Encode(hmac, result(), 20);
}

Sha1Class Sha1;
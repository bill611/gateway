/*
 * Copyright (c) 2014-2015 Alibaba Group. All rights reserved.
 *
 * Alibaba Group retains all right, title and interest (including all
 * intellectual property rights) in and to this computer program, which is
 * protected by applicable intellectual property laws.  Unless you have
 * obtained a separate written license from Alibaba Group., you are not
 * authorized to utilize all or a part of this computer program for any
 * purpose (including reproduction, distribution, modification, and
 * compilation into object code), and you must immediately destroy or
 * return to Alibaba Group all copies of this computer program.  If you
 * are licensed by Alibaba Group, your rights to utilize this computer
 * program are limited by the terms of that license.  To obtain a license,
 * please contact Alibaba Group.
 *
 * This computer program contains trade secrets owned by Alibaba Group.
 * and, unless unauthorized by Alibaba Group in writing, you agree to
 * maintain the confidentiality of this computer program and related
 * information and to not disclose this computer program and related
 * information to any other person or entity.
 *
 * THIS COMPUTER PROGRAM IS PROVIDED AS IS WITHOUT ANY WARRANTIES, AND
 * Alibaba Group EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * INCLUDING THE WARRANTIES OF MERCHANTIBILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE, AND NONINFRINGEMENT.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "base64.h"

static const unsigned char encoding_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};

static const unsigned char decoding_table[] = {
    0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff,

    0x3e, 0xff, 0xff, 0xff, 0x3f, 0x34, 0x35, 0x36,
    0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0xff,
    0xff, 0xff, 0xfe, 0xff, 0xff, 0xff, 0x00, 0x01,
    0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11,
    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1a, 0x1b,
    0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23,
    0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b,
    0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0x32, 0x33,

                      0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
};

static const unsigned char mod_table[] = { 0, 2, 1 };

#define BASE64_ENCODE_SIZE(x)  (((x)+2) / 3 * 4 + 1)

char *ABase64_Encode(uint8_t *in, int in_len, char *out, int out_len)
{
    int o_len = BASE64_ENCODE_SIZE(in_len);
    if (out_len < o_len)
        return NULL;

    char *dst = out;

    int i;
    for (i = 0; i < in_len;) {
        unsigned int octet_a = i < in_len ? in[i++] : 0;
        unsigned int octet_b = i < in_len ? in[i++] : 0;
        unsigned int octet_c = i < in_len ? in[i++] : 0;

        unsigned int triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        *dst++ = encoding_table[(triple >> 18) & 0x3F];
        *dst++ = encoding_table[(triple >> 12) & 0x3F];
        *dst++ = encoding_table[(triple >>  6) & 0x3F];
        *dst++ = encoding_table[(triple >>  0) & 0x3F];
    }

    for (i = 0; i < mod_table[in_len % 3]; i++)
        out[o_len - 2 - i] = '=';
    out[o_len - 1] = '\0';

    return out;
}

#define BASE64_DECODE_SIZE(x) ((x) * 3LL / 4)

int ABase64_Decode(char *in, uint8_t *out, int out_len)
{
    int o_len = ABase64_DecodeLen(in);
    if (o_len > out_len)
        return -1;

    int in_len = strlen(in);

    int i, j;
    for (i = 0, j = 0; i < in_len;) {
        unsigned int sextet_a = decoding_table[(int)in[i++]];
        unsigned int sextet_b = decoding_table[(int)in[i++]];
        unsigned int sextet_c = decoding_table[(int)in[i++]];
        unsigned int sextet_d = decoding_table[(int)in[i++]];

        unsigned int triple = (sextet_a << 18)
                            + (sextet_b << 12)
                            + (sextet_c <<  6)
                            + (sextet_d <<  0);

        if (j < o_len) out[j++] = (triple >> 16) & 0xFF;
        if (j < o_len) out[j++] = (triple >>  8) & 0xFF;
        if (j < o_len) out[j++] = (triple >>  0) & 0xFF;
    }

    return o_len;
}

int ABase64_DecodeLen(char *encoded)
{
    int encoded_len = strlen(encoded);

    int len = BASE64_DECODE_SIZE(encoded_len);

    len -= encoded[encoded_len - 1] == '=' ? 1 : 0;
    len -= encoded[encoded_len - 2] == '=' ? 1 : 0;

    return len;
}

int ABase64_EncodeLen(int decode_len)
{
    return BASE64_ENCODE_SIZE(decode_len);
}

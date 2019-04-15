/*******************************************************************************
* Copyright 2014-2019 Intel Corporation
* All Rights Reserved.
*
* If this  software was obtained  under the  Intel Simplified  Software License,
* the following terms apply:
*
* The source code,  information  and material  ("Material") contained  herein is
* owned by Intel Corporation or its  suppliers or licensors,  and  title to such
* Material remains with Intel  Corporation or its  suppliers or  licensors.  The
* Material  contains  proprietary  information  of  Intel or  its suppliers  and
* licensors.  The Material is protected by  worldwide copyright  laws and treaty
* provisions.  No part  of  the  Material   may  be  used,  copied,  reproduced,
* modified, published,  uploaded, posted, transmitted,  distributed or disclosed
* in any way without Intel's prior express written permission.  No license under
* any patent,  copyright or other  intellectual property rights  in the Material
* is granted to  or  conferred  upon  you,  either   expressly,  by implication,
* inducement,  estoppel  or  otherwise.  Any  license   under such  intellectual
* property rights must be express and approved by Intel in writing.
*
* Unless otherwise agreed by Intel in writing,  you may not remove or alter this
* notice or  any  other  notice   embedded  in  Materials  by  Intel  or Intel's
* suppliers or licensors in any way.
*
*
* If this  software  was obtained  under the  Apache License,  Version  2.0 (the
* "License"), the following terms apply:
*
* You may  not use this  file except  in compliance  with  the License.  You may
* obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
*
*
* Unless  required  by   applicable  law  or  agreed  to  in  writing,  software
* distributed under the License  is distributed  on an  "AS IS"  BASIS,  WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*
* See the   License  for the   specific  language   governing   permissions  and
* limitations under the License.
*******************************************************************************/

/* 
// 
//  Purpose:
//     Cryptography Primitive.
//     SMS4 EBC decryption
// 
//  Contents:
//     cpSMS4_CTR_aesni_x16()
// 
// 
*/

#include "owndefs.h"
#include "owncp.h"
#include "pcpsms4.h"

#if (_IPP>=_IPP_H9) || (_IPP32E>=_IPP32E_L9)

static __ALIGN32 Ipp8u inpMaskLO[] = {0x65,0x41,0xfd,0xd9,0x0a,0x2e,0x92,0xb6,0x0f,0x2b,0x97,0xb3,0x60,0x44,0xf8,0xdc,
                                      0x65,0x41,0xfd,0xd9,0x0a,0x2e,0x92,0xb6,0x0f,0x2b,0x97,0xb3,0x60,0x44,0xf8,0xdc};
static __ALIGN32 Ipp8u inpMaskHI[] = {0x00,0xc9,0x67,0xae,0x80,0x49,0xe7,0x2e,0x4a,0x83,0x2d,0xe4,0xca,0x03,0xad,0x64,
                                      0x00,0xc9,0x67,0xae,0x80,0x49,0xe7,0x2e,0x4a,0x83,0x2d,0xe4,0xca,0x03,0xad,0x64};
static __ALIGN32 Ipp8u outMaskLO[] = {0xd3,0x59,0x38,0xb2,0xcc,0x46,0x27,0xad,0x36,0xbc,0xdd,0x57,0x29,0xa3,0xc2,0x48,
                                      0xd3,0x59,0x38,0xb2,0xcc,0x46,0x27,0xad,0x36,0xbc,0xdd,0x57,0x29,0xa3,0xc2,0x48};
static __ALIGN32 Ipp8u outMaskHI[] = {0x00,0x50,0x14,0x44,0x89,0xd9,0x9d,0xcd,0xde,0x8e,0xca,0x9a,0x57,0x07,0x43,0x13,
                                      0x00,0x50,0x14,0x44,0x89,0xd9,0x9d,0xcd,0xde,0x8e,0xca,0x9a,0x57,0x07,0x43,0x13};

static __ALIGN32 Ipp8u maskSrows[] = {0x00,0x0d,0x0a,0x07,0x04,0x01,0x0e,0x0b,0x08,0x05,0x02,0x0f,0x0c,0x09,0x06,0x03,
                                      0x00,0x0d,0x0a,0x07,0x04,0x01,0x0e,0x0b,0x08,0x05,0x02,0x0f,0x0c,0x09,0x06,0x03};

static __ALIGN32 Ipp8u lowBits4[] = {0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,
                                     0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f};

static __ALIGN32 Ipp8u swapBytes[] = {3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12,
                                      3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12};

static __ALIGN32 Ipp8u permMask[] = {0x00,0x00,0x00,0x00, 0x04,0x00,0x00,0x00, 0x01,0x00,0x00,0x00, 0x05,0x00,0x00,0x00,
                                     0x02,0x00,0x00,0x00, 0x06,0x00,0x00,0x00, 0x03,0x00,0x00,0x00, 0x07,0x00,0x00,0x00};

static __ALIGN32 Ipp8u endiannes_swap[] = {12,13,14,15, 8,9,10,11, 4,5,6,7, 0,1,2,3,
                                           12,13,14,15, 8,9,10,11, 4,5,6,7, 0,1,2,3};

static __ALIGN32 Ipp8u endiannes[] = {15,14,13,12, 11,10,9,8, 7,6,5,4, 3,2,1,0,
                                      15,14,13,12, 11,10,9,8, 7,6,5,4, 3,2,1,0};

static __ALIGN32 Ipp8u two256[] = {2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static __ALIGN16 Ipp8u one256[] = {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                                   1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static __ALIGN16 Ipp8u encKey[] = {0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63};

#define M256(mem)    (*((__m256i*)((Ipp8u*)(mem))))
#define M128(mem)    (*((__m128i*)((Ipp8u*)(mem))))


__INLINE __m256i affine(__m256i x, __m256i maskLO, __m256i maskHI)
{
   __m256i T1 = _mm256_and_si256(_mm256_srli_epi64(x, 4), M256(lowBits4));
   __m256i T0 = _mm256_and_si256(x, M256(lowBits4));
   T0 = _mm256_shuffle_epi8(maskLO, T0);
   T1 = _mm256_shuffle_epi8(maskHI, T1);
   return _mm256_xor_si256(T0, T1);
}

__INLINE __m256i L(__m256i x)
{
   __m256i T = _mm256_xor_si256(_mm256_slli_epi32(x, 2), _mm256_srli_epi32(x,30));

   T = _mm256_xor_si256(T, _mm256_slli_epi32 (x,10));
   T = _mm256_xor_si256(T, _mm256_srli_epi32 (x,22));

   T = _mm256_xor_si256(T, _mm256_slli_epi32 (x,18));
   T = _mm256_xor_si256(T, _mm256_srli_epi32 (x,14));

   T = _mm256_xor_si256(T, _mm256_slli_epi32 (x,24));
   T = _mm256_xor_si256(T, _mm256_srli_epi32 (x, 8));
   return T;
}

__INLINE __m256i AES_ENC_LAST(__m256i x, __m128i key)
{
   __m128i t0 = _mm256_extracti128_si256(x, 0);
   __m128i t1 = _mm256_extracti128_si256(x, 1);
   t0 = _mm_aesenclast_si128(t0, key);
   t1 = _mm_aesenclast_si128(t1, key);
   x = _mm256_inserti128_si256(x, t0, 0);
   x = _mm256_inserti128_si256(x, t1, 1);
   return x;
}

/*
// inp: T0, T1, T2, T3
// out: K0, K1, K2, K3
*/
#define TRANSPOSE_INP(K0,K1,K2,K3, T0,T1,T2,T3) \
   K0 = _mm256_unpacklo_epi32(T0, T1); \
   K1 = _mm256_unpacklo_epi32(T2, T3); \
   K2 = _mm256_unpackhi_epi32(T0, T1); \
   K3 = _mm256_unpackhi_epi32(T2, T3); \
   \
   T0 = _mm256_unpacklo_epi64(K0, K1); \
   T1 = _mm256_unpacklo_epi64(K2, K3); \
   T2 = _mm256_unpackhi_epi64(K0, K1); \
   T3 = _mm256_unpackhi_epi64(K2, K3); \
   \
   K2 = _mm256_permutevar8x32_epi32(T1, M256(permMask)); \
   K1 = _mm256_permutevar8x32_epi32(T2, M256(permMask)); \
   K3 = _mm256_permutevar8x32_epi32(T3, M256(permMask)); \
   K0 = _mm256_permutevar8x32_epi32(T0, M256(permMask))

/*
// inp: K0, K1, K2, K3
// out: T0, T1, T2, T3
*/
#define TRANSPOSE_OUT(T0,T1,T2,T3, K0,K1,K2,K3) \
   T0 = _mm256_unpacklo_epi32(K1, K0); \
   T1 = _mm256_unpacklo_epi32(K3, K2); \
   T2 = _mm256_unpackhi_epi32(K1, K0); \
   T3 = _mm256_unpackhi_epi32(K3, K2); \
   \
   K0 = _mm256_unpacklo_epi64(T1, T0); \
   K1 = _mm256_unpacklo_epi64(T3, T2); \
   K2 = _mm256_unpackhi_epi64(T1, T0); \
   K3 = _mm256_unpackhi_epi64(T3, T2); \
   \
   T0 = _mm256_permute2x128_si256(K0, K2, 0x20); \
   T1 = _mm256_permute2x128_si256(K1, K3, 0x20); \
   T2 = _mm256_permute2x128_si256(K0, K2, 0x31); \
   T3 = _mm256_permute2x128_si256(K1, K3, 0x31)

__INLINE __m128i inc128(__m128i x)
{
   __m128i t = _mm_add_epi64(x,  M128(one256));
   x = _mm_cmpeq_epi64(t,  _mm_setzero_si128());
   t = _mm_sub_epi64(t, _mm_slli_si128(x, sizeof(Ipp64u)));
   return t;
}

__INLINE __m256i inc256(__m256i x)
{
   __m256i t = _mm256_add_epi64(x,  M256(two256));
   x = _mm256_cmpeq_epi64(t,  _mm256_setzero_si256());
   t = _mm256_sub_epi64(t, _mm256_slli_si256(x, sizeof(Ipp64u)));
   return t;
}
__INLINE __m256i inc256_2(__m256i x)
{
   __m256i t = _mm256_add_epi64(x,  M256(one256));
   x = _mm256_cmpeq_epi64(t,  _mm256_setzero_si256());
   t = _mm256_sub_epi64(t, _mm256_slli_si256(x, sizeof(Ipp64u)));

   t = _mm256_add_epi64(t,  M256(one256));
   x = _mm256_cmpeq_epi64(t,  _mm256_setzero_si256());
   t = _mm256_sub_epi64(t, _mm256_slli_si256(x, sizeof(Ipp64u)));
   return t;
}

int cpSMS4_CTR_aesni(Ipp8u* pOut, const Ipp8u* pInp, int len, const Ipp32u* pRKey, const Ipp8u* pCtrMask, Ipp8u* pCtr)
{
   int processedLen = len & -(16*MBS_SMS4);

   if(processedLen) {
      int n;

      /* read string counter and convert to numerical */
      __m256i ctr  = _mm256_shuffle_epi8(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pCtr)), M256(endiannes));

      /* read string mask and convert to numerical */
      __m256i mask = _mm256_shuffle_epi8(_mm256_castsi128_si256(_mm_loadu_si128((__m128i*)pCtrMask)), M256(endiannes));

      /* upchanged counter bits */
      __m256i unch = _mm256_andnot_si256(mask, ctr);

      ctr = _mm256_inserti128_si256(ctr,  inc128(_mm256_castsi256_si128(ctr)),  1);
      mask= _mm256_inserti128_si256(mask, _mm256_castsi256_si128(mask), 1);
      unch= _mm256_inserti128_si256(unch, _mm256_castsi256_si128(unch), 1);
      ctr = _mm256_and_si256(ctr, mask);

      for(n=0; n<processedLen; n+=(16*MBS_SMS4), pInp+=(16*MBS_SMS4), pOut+=(16*MBS_SMS4)) {
         int itr;
         __m256i K0, K1, K2, K3;
         __m256i P0, P1, P2, P3;

         __m256i T0 = ctr;
         __m256i T1 = inc256_2(T0);
         __m256i T2 = inc256_2(T1);
         __m256i T3 = inc256_2(T2);
         ctr = inc256_2(T3);
         T0 = _mm256_xor_si256(unch, _mm256_and_si256(T0, mask));
         T1 = _mm256_xor_si256(unch, _mm256_and_si256(T1, mask));
         T2 = _mm256_xor_si256(unch, _mm256_and_si256(T2, mask));
         T3 = _mm256_xor_si256(unch, _mm256_and_si256(T3, mask));
         T0 = _mm256_shuffle_epi8(T0, M256(endiannes_swap));
         T1 = _mm256_shuffle_epi8(T1, M256(endiannes_swap));
         T2 = _mm256_shuffle_epi8(T2, M256(endiannes_swap));
         T3 = _mm256_shuffle_epi8(T3, M256(endiannes_swap));
         TRANSPOSE_INP(K0,K1,K2,K3, T0,T1,T2,T3);

         T0 = ctr;
         T1 = inc256(T0);
         T2 = inc256(T1);
         T3 = inc256(T2);
         ctr = inc256(T3);
         T0 = _mm256_xor_si256(unch, _mm256_and_si256(T0, mask));
         T1 = _mm256_xor_si256(unch, _mm256_and_si256(T1, mask));
         T2 = _mm256_xor_si256(unch, _mm256_and_si256(T2, mask));
         T3 = _mm256_xor_si256(unch, _mm256_and_si256(T3, mask));
         T0 = _mm256_shuffle_epi8(T0, M256(endiannes_swap));
         T1 = _mm256_shuffle_epi8(T1, M256(endiannes_swap));
         T2 = _mm256_shuffle_epi8(T2, M256(endiannes_swap));
         T3 = _mm256_shuffle_epi8(T3, M256(endiannes_swap));
         TRANSPOSE_INP(P0,P1,P2,P3, T0,T1,T2,T3);

         for(itr=0; itr<8; itr++, pRKey+=4) {
            /* initial xors */
            T1 = T0 = _mm256_set1_epi32(pRKey[0]);
            T0 = _mm256_xor_si256(T0, K1);
            T0 = _mm256_xor_si256(T0, K2);
            T0 = _mm256_xor_si256(T0, K3);
            T1 = _mm256_xor_si256(T1, P1);
            T1 = _mm256_xor_si256(T1, P2);
            T1 = _mm256_xor_si256(T1, P3);
            /* Sbox */
            T0 = affine(T0, M256(inpMaskLO), M256(inpMaskHI));
            T1 = affine(T1, M256(inpMaskLO), M256(inpMaskHI));
            T0 = AES_ENC_LAST(T0, M128(encKey));
            T1 = AES_ENC_LAST(T1, M128(encKey));
            T0 = _mm256_shuffle_epi8(T0, M256(maskSrows));
            T1 = _mm256_shuffle_epi8(T1, M256(maskSrows));
            T0 = affine(T0, M256(outMaskLO), M256(outMaskHI));
            T1 = affine(T1, M256(outMaskLO), M256(outMaskHI));
            /* Sbox done, now L */
            K0 = _mm256_xor_si256(_mm256_xor_si256(K0, T0), L(T0));
            P0 = _mm256_xor_si256(_mm256_xor_si256(P0, T1), L(T1));

            /* initial xors */
            T1 = T0 = _mm256_set1_epi32(pRKey[1]);
            T0 = _mm256_xor_si256(T0, K2);
            T0 = _mm256_xor_si256(T0, K3);
            T0 = _mm256_xor_si256(T0, K0);
            T1 = _mm256_xor_si256(T1, P2);
            T1 = _mm256_xor_si256(T1, P3);
            T1 = _mm256_xor_si256(T1, P0);
            /* Sbox */
            T0 = affine(T0, M256(inpMaskLO), M256(inpMaskHI));
            T1 = affine(T1, M256(inpMaskLO), M256(inpMaskHI));
            T0 = AES_ENC_LAST(T0, M128(encKey));
            T1 = AES_ENC_LAST(T1, M128(encKey));
            T0 = _mm256_shuffle_epi8(T0, M256(maskSrows));
            T1 = _mm256_shuffle_epi8(T1, M256(maskSrows));
            T0 = affine(T0, M256(outMaskLO), M256(outMaskHI));
            T1 = affine(T1, M256(outMaskLO), M256(outMaskHI));
            /* Sbox done, now L */
            K1 = _mm256_xor_si256(_mm256_xor_si256(K1, T0), L(T0));
            P1 = _mm256_xor_si256(_mm256_xor_si256(P1, T1), L(T1));

            /* initial xors */
            T1 = T0 = _mm256_set1_epi32(pRKey[2]);
            T0 = _mm256_xor_si256(T0, K3);
            T0 = _mm256_xor_si256(T0, K0);
            T0 = _mm256_xor_si256(T0, K1);
            T1 = _mm256_xor_si256(T1, P3);
            T1 = _mm256_xor_si256(T1, P0);
            T1 = _mm256_xor_si256(T1, P1);
            /* Sbox */
            T0 = affine(T0, M256(inpMaskLO), M256(inpMaskHI));
            T1 = affine(T1, M256(inpMaskLO), M256(inpMaskHI));
            T0 = AES_ENC_LAST(T0, M128(encKey));
            T1 = AES_ENC_LAST(T1, M128(encKey));
            T0 = _mm256_shuffle_epi8(T0, M256(maskSrows));
            T1 = _mm256_shuffle_epi8(T1, M256(maskSrows));
            T0 = affine(T0, M256(outMaskLO), M256(outMaskHI));
            T1 = affine(T1, M256(outMaskLO), M256(outMaskHI));
            /* Sbox done, now L */
            K2 = _mm256_xor_si256(_mm256_xor_si256(K2, T0), L(T0));
            P2 = _mm256_xor_si256(_mm256_xor_si256(P2, T1), L(T1));

            /* initial xors */
            T1 = T0 = _mm256_set1_epi32(pRKey[3]);
            T0 = _mm256_xor_si256(T0, K0);
            T0 = _mm256_xor_si256(T0, K1);
            T0 = _mm256_xor_si256(T0, K2);
            T1 = _mm256_xor_si256(T1, P0);
            T1 = _mm256_xor_si256(T1, P1);
            T1 = _mm256_xor_si256(T1, P2);
            /* Sbox */
            T0 = affine(T0, M256(inpMaskLO), M256(inpMaskHI));
            T1 = affine(T1, M256(inpMaskLO), M256(inpMaskHI));
            T0 = AES_ENC_LAST(T0, M128(encKey));
            T1 = AES_ENC_LAST(T1, M128(encKey));
            T0 = _mm256_shuffle_epi8(T0, M256(maskSrows));
            T1 = _mm256_shuffle_epi8(T1, M256(maskSrows));
            T0 = affine(T0, M256(outMaskLO), M256(outMaskHI));
            T1 = affine(T1, M256(outMaskLO), M256(outMaskHI));
            /* Sbox done, now L */
            K3 = _mm256_xor_si256(_mm256_xor_si256(K3, T0), L(T0));
            P3 = _mm256_xor_si256(_mm256_xor_si256(P3, T1), L(T1));
         }

         pRKey -= 32;

         TRANSPOSE_OUT(T0,T1,T2,T3, K0,K1,K2,K3);
         T0 = _mm256_shuffle_epi8(T0, M256(swapBytes));
         T1 = _mm256_shuffle_epi8(T1, M256(swapBytes));
         T2 = _mm256_shuffle_epi8(T2, M256(swapBytes));
         T3 = _mm256_shuffle_epi8(T3, M256(swapBytes));
         _mm256_storeu_si256((__m256i*)(pOut),            _mm256_xor_si256(T0, _mm256_loadu_si256((__m256i*)(pInp))));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*2), _mm256_xor_si256(T1, _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*2))));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*4), _mm256_xor_si256(T2, _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*4))));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*6), _mm256_xor_si256(T3, _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*6))));

         TRANSPOSE_OUT(T0,T1,T2,T3, P0,P1,P2,P3);
         T0 = _mm256_shuffle_epi8(T0, M256(swapBytes));
         T1 = _mm256_shuffle_epi8(T1, M256(swapBytes));
         T2 = _mm256_shuffle_epi8(T2, M256(swapBytes));
         T3 = _mm256_shuffle_epi8(T3, M256(swapBytes));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*8),  _mm256_xor_si256(T0, _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*8))));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*10), _mm256_xor_si256(T1, _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*10))));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*12), _mm256_xor_si256(T2, _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*12))));
         _mm256_storeu_si256((__m256i*)(pOut+MBS_SMS4*14), _mm256_xor_si256(T3, _mm256_loadu_si256((__m256i*)(pInp+MBS_SMS4*14))));
      }

      ctr = _mm256_xor_si256(unch, _mm256_and_si256(ctr, mask));
      ctr = _mm256_shuffle_epi8(ctr, M256(endiannes));
      _mm_storeu_si128((__m128i*)pCtr, _mm256_castsi256_si128(ctr));
   }

   return processedLen + cpSMS4_CTR_aesni_x4(pOut, pInp, len-processedLen, pRKey, pCtrMask, pCtr);
}

#endif /* _IPP_G9, _IPP32E_L9 */

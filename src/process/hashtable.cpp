//
// Created by zhenkai on 2022/4/15.
//

#include "hashtable.h"

#include <string.h>
#include <chrono>

#define OFFSET_OF_HASH(ptr) ((char *)ptr - (char *)Gseg - Gseg->user_app_pos)

/* {{{ MurmurHash2 (Austin Appleby)
 */
static inline uint64_t
hash_func1(const char *data, unsigned int len) {
    unsigned int h, k;

    h = 0 ^ len;

    while (len >= 4) {
        k = data[0];
        k |= data[1] << 8;
        k |= data[2] << 16;
        k |= data[3] << 24;

        k *= 0x5bd1e995;
        k ^= k >> 24;
        k *= 0x5bd1e995;

        h *= 0x5bd1e995;
        h ^= k;

        data += 4;
        len -= 4;
    }

    switch (len) {
        case 3:
            h ^= data[2] << 16;
        case 2:
            h ^= data[1] << 8;
        case 1:
            h ^= data[0];
            h *= 0x5bd1e995;
    }

    h ^= h >> 13;
    h *= 0x5bd1e995;
    h ^= h >> 15;

    return h;
}
/* }}} */

/* {{{ DJBX33A (Daniel J. Bernstein, Times 33 with Addition)
 *
 * This is Daniel J. Bernstein's popular `times 33' hash function as
 * posted by him years ago on comp->lang.c. It basically uses a function
 * like ``hash(i) = hash(i-1) * 33 + str[i]''. This is one of the best
 * known hash functions for strings. Because it is both computed very
 * fast and distributes very well.
 *
 * The magic of number 33, i.e. why it works better than many other
 * constants, prime or not, has never been adequately explained by
 * anyone. So I try an explanation: if one experimentally tests all
 * multipliers between 1 and 256 (as RSE did now) one detects that even
 * numbers are not useable at all. The remaining 128 odd numbers
 * (except for the number 1) work more or less all equally well. They
 * all distribute in an acceptable way and this way fill a hash table
 * with an average percent of approx. 86%.
 *
 * If one compares the Chi^2 values of the variants, the number 33 not
 * even has the best value. But the number 33 and a few other equally
 * good numbers like 17, 31, 63, 127 and 129 have nevertheless a great
 * advantage to the remaining numbers in the large set of possible
 * multipliers: their multiply operation can be replaced by a faster
 * operation based on just one shift plus either a single addition
 * or subtraction operation. And because a hash function has to both
 * distribute good _and_ has to be very fast to compute, those few
 * numbers should be preferred and seems to be the reason why Daniel J.
 * Bernstein also preferred it.
 *
 *
 *                  -- Ralf S. Engelschall <rse@engelschall.com>
 */

static inline uint64_t
hash_func2(const char *key, uint32_t len) {
    register uint64_t hash = 5381;

    /* variant with the hash unrolled eight times */
    for (; len >= 8; len -= 8) {
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
        hash = ((hash << 5) + hash) + *key++;
    }
    switch (len) {
        case 7:
            hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 6:
            hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 5:
            hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 4:
            hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 3:
            hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 2:
            hash = ((hash << 5) + hash) + *key++; /* fallthrough... */
        case 1:
            hash = ((hash << 5) + hash) + *key++;
            break;
        case 0:
            break;
        default:
            break;
    }
    return hash;
}
/* }}} */

/* {{{  COPYRIGHT (C) 1986 Gary S. Brown.  You may use this program, or
 *  code or tables extracted from it, as desired without restriction.
 *
 *  First, the polynomial itself and its table of feedback terms.  The
 *  polynomial is
 *  X^32+X^26+X^23+X^22+X^16+X^12+X^11+X^10+X^8+X^7+X^5+X^4+X^2+X^1+X^0
 *
 *  Note that we take it "backwards" and put the highest-order term in
 *  the lowest-order bit.  The X^32 term is "implied"; the LSB is the
 *  X^31 term, etc.  The X^0 term (usually shown as "+1") results in
 *  the MSB being 1
 *
 *  Note that the usual hardware shift register implementation, which
 *  is what we're using (we're merely optimizing it by doing eight-bit
 *  chunks at a time) shifts bits into the lowest-order term.  In our
 *  implementation, that means shifting towards the right.  Why do we
 *  do it this way?  Because the calculated CRC must be transmitted in
 *  order from highest-order term to lowest-order term.  UARTs transmit
 *  characters in order from LSB to MSB.  By storing the CRC this way
 *  we hand it to the UART in the order low-byte to high-byte; the UART
 *  sends each low-bit to hight-bit; and the result is transmission bit
 *  by bit from highest- to lowest-order term without requiring any bit
 *  shuffling on our part.  Reception works similarly
 *
 *  The feedback terms table consists of 256, 32-bit entries.  Notes
 *
 *      The table can be generated at runtime if desired; code to do so
 *      is shown later.  It might not be obvious, but the feedback
 *      terms simply represent the results of eight shift/xor opera
 *      tions for all combinations of data and CRC register values
 *
 *      The values must be right-shifted by eight bits by the "updcrc
 *      logic; the shift must be unsigned (bring in zeroes).  On some
 *      hardware you could probably optimize the shift in assembler by
 *      using byte-swap instructions
 *      polynomial $edb88320
 *
 *
 * CRC32 code derived from work by Gary S. Brown.
 */

static unsigned int crc32_tab[] = {
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
        0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
        0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
        0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
        0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
        0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
        0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
        0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
        0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
        0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
        0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
        0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

static uint32_t crc32(const char *buf, unsigned int size) {
    const char *p;
    uint32_t crc = 0 ^ 0xFFFFFFFF;

    p = buf;
    while (size--) {
        crc = crc32_tab[(crc ^ *p++) & 0xFF] ^ (crc >> 8);
    }

    return crc ^ 0xFFFFFFFF;
}

Hash *Hasher::getHashAddr() {
    if (!Gseg) {
        return nullptr;
    }
    if (!m_ht) {
        m_ht = (Hash *) ((char *) Gseg + Gseg->user_app_pos);
    }

    return m_ht;
}

void *Hasher::alloc_mem(size_t size) {
    if (!Gseg || Gseg->free < size || size == 0) {
        return NULL;
    }
    size_t pos = Gseg->pos;
    Gseg->free -= size;
    Gseg->pos += size;

    char *retp = (char *)Gseg + pos;
    memset(retp, 0, size);

    return retp;
}

Hval *Hasher::make_hval(const char *data, uint32_t data_len) {
    if (data_len == 0 || !data) {
        return NULL;
    }
    uint32_t real_size = sizeof(Hval) + data_len;
    Hval *v = (Hval *) alloc_mem(real_size);
    if (!v) {
        return NULL;
    }
    memcpy(v->data, data, data_len);
    v->len = data_len;
    v->next = HT_INVALID_IDX;
    v->real_size = real_size;
//    v->atime = time(NULL);

    return v;
}

bool Hasher::init_bucket(Bucket *p, uint64_t h, const char *key, uint32_t key_len,
                         const char *data, uint32_t data_len) {
    if (!p || key_len > MAX_KEY_LEN) {
        return false;
    }

    // bucket
    memset(p, 0, sizeof(Bucket));
    p->crc = crc32(data, data_len);
    p->len = key_len;
    memcpy(p->key, key, key_len);
    p->h = h;

    // value
    Hval *val = make_hval(data, data_len);
    if (!val) {
        return false;
    }
    p->hvalOffset = OFFSET_OF_HASH(val);

    return true;
}

bool Hasher::update_bucket(Hash *ht, Bucket *p, const char *data, uint32_t data_len) {
    if (!p || p->len > MAX_KEY_LEN) {
        return false;
    }
    Hval *v;
    uint32_t voffset = p->hvalOffset;
    if (!voffset) {
        v = make_hval(data, data_len);
        if (!v) {
            return false;
        }
    } else {
        v = HVAL(ht, p);
    }
    uint32_t next_idx = v->next;
    if (v->real_size < data_len + sizeof(Hval)) {
        v = make_hval(data, data_len);
        if (!v) {
            return false;
        }
    } else {
        memcpy(v->data, data, data_len);
        v->len = data_len;
//        v->atime = time(NULL);
    }
    v->next = next_idx;

    p->hvalOffset = OFFSET_OF_HASH(v);
    p->crc = crc32(data, data_len);

    return true;
}

bool Hasher::init(size_t alloc_size, uint32_t table_size) {
    if (Gseg == NULL || alloc_size < sizeof(Segment)) {
        return false;
    }
    Gseg->size = alloc_size;
    Gseg->pos = Gseg->user_app_pos = sizeof(Segment);
    Gseg->free = alloc_size - Gseg->pos;

    Hash *ht = (Hash *)alloc_mem(sizeof(Hash));
    ht->nTableSize = table_size;
    ht->nNumUsed = 0;
    ht->nTableMask = -ht->nTableSize;
    ht->nNextFreeElement = 0;
    uint32_t *ptr = (uint32_t *) alloc_mem(ht->nTableSize * (sizeof(uint32_t) + sizeof(Bucket)));
    if (!ptr) {
        return false;
    }
    memset(ptr, HT_INVALID_IDX, ht->nTableSize * sizeof(uint32_t));
    ht->bucketOffset = sizeof(Hash) + ht->nTableSize * sizeof(uint32_t);

    return true;
}

void Hasher::dump(Hash *ht, bool dump_full) {
    if (dump_full) {
        char *data;
        Hval *val;
        FOREACH_HASH_TABLE_START(ht)
            if (bucket->flag == FLAG_DELETE) {
                continue;
            }
            printf("-------------\n");
            printf("key: %s\n", bucket->key);
            val = HVAL(ht, bucket);
            data = strndup(val->data, val->len);
            printf("value: %s\n", data);
            free(data);
            printf("-------------\n");
        FOREACH_END
    }

    char info[1024] = {0};
    snprintf(info, 1024,
             "Hashtable summary:\n"
             "ht.nNumUsed : %u\n"
             "ht.nNumOfElements: %u\n"
             "ht.nTableSize: %u\n"
             "ht.conflicts %u\n"
             "ht.hits %u\n"
             "ht.miss %u\n"
             "ht.del_cnt %u\n"
             "mem free: %u byte\n",
             ht->nNumUsed,
             ht->nNumOfElements,
             ht->nTableSize,
             ht->conflicts,
             ht->hits,
             ht->miss,
             ht->del_cnt,
             Gseg->free
    );

    printf("%s", info);
}

bool Hasher::hash_add_or_update_bucket(Hash *ht, const char *key, uint32_t len,
                              const char *data, uint32_t size) {
    uint64_t h = hash_func2(key, len);
    Bucket *p;
    Bucket *arData = AR_DATA(ht);
    uint32_t nIndex = h | ht->nTableMask;

    uint32_t idx = ((uint32_t *) arData)[(int32_t) nIndex];
    uint32_t next_idx = HT_INVALID_IDX;

    if (idx == HT_INVALID_IDX) {
ADD_TO_HASH:
        if (ht->nTableSize <= ht->nNumUsed) {
            return false;
        }
        p = arData + ht->nNumUsed;
        bool ret = init_bucket(p, h, key, len, data, size);
        if (!ret) {
            return false;
        }
        ((uint32_t *) arData)[(int32_t) nIndex] = ht->nNumUsed;
        Hval *val = HVAL(ht, p);
        val->next = next_idx;

        ht->nNumUsed++;
        ht->nNumOfElements++;

        return true;
    } else {
        next_idx = idx;
        while (idx != HT_INVALID_IDX) {
            p = arData + idx;
            if (p->h == h && p->key && p->len == len && memcmp(p->key, key, len) == 0) {
UPDATE_TO_HASH:
                if (p->crc != crc32(data, size)) {
                    return update_bucket(ht, p, data, size);
                }
                return true;
            }
            Hval *val = HVAL(ht, p);
            idx = val->next;
        }
        ht->conflicts ++;
        // add
        goto ADD_TO_HASH;

    }
}

Bucket *Hasher::hash_find_bucket(Hash *ht, const char *key, uint32_t len) {
    uint64_t h = hash_func2(key, len);
    Bucket *p;
    uint32_t nIndex = h | ht->nTableMask;

    uint32_t idx = HT_HASH(ht, nIndex);

    int trytimes = 0;
    while (idx != HT_INVALID_IDX) {
        p = HT_HASH_TO_BUCKET(ht, idx);
        Hval *v = HVAL(ht, p);
RETRY:
        if (p->h == h && p->key && p->len == len && memcmp(p->key, key, len) == 0) {
            trytimes++;
            if (v && crc32(v->data, v->len) == p->crc) {
                return p;
            }
            if (trytimes > 3) {
                return NULL;
            }
            sched_yield();
            goto RETRY;
        }
        idx = v->next;
    }

    return NULL;
}

bool Hasher::hash_delete_bucket(Hash *ht, const char *key, uint32_t len) {
    uint64_t h = hash_func2(key, len);
    uint32_t nIndex = h | ht->nTableMask;
    uint32_t idx = HT_HASH(ht, nIndex);

    Bucket *p, *prev = NULL;
    Hval *hval = NULL;
    while (idx != HT_INVALID_IDX) {
        p = HT_HASH_TO_BUCKET(ht, idx);
        if (!p) {
            break;
        }
        hval = HVAL(ht, p);
        if (p->h == h && p->key && p->len == len && memcmp(p->key, key, len) == 0) {
            if (hval && p->flag != FLAG_DELETE) {
                ht->nNumOfElements--;
                p->flag = FLAG_DELETE;
//                hval->atime = time(NULL);
                if (prev == NULL) {
                    HT_HASH(ht, nIndex) = hval->next;
                } else {
                    Hval *prev_hval = HVAL(ht, prev);
                    prev_hval->next = hval->next;
                }
                ht->del_cnt ++;
                return true;
            }
        }
        prev = p;
        idx = hval->next;
    }

    return false;
}

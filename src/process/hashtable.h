//
// Created by zhenkai on 2022/4/15.
//

#ifndef DICTX_HASHTABLE_H
#define DICTX_HASHTABLE_H

#include <memory>
#include <cstring>
#include <iostream>

#define MAX_KEY_LEN                 (64)

#define STORAGE_FACTOR              (1)
#define MEM_ALIGNMENT               8
#define MEM_ALIGNMENT_MASK          ~(MEM_ALIGNMENT - 1)
#define MEM_ALIGNED_SIZE(x)         ((int)((x) + MEM_ALIGNMENT - 1) & (MEM_ALIGNMENT_MASK))
#define MEM_MIN_BLOCK_SIZE          128
#define MEM_TRUE_SIZE(x)            ((x < MEM_MIN_BLOCK_SIZE)? (MEM_MIN_BLOCK_SIZE) : (MEM_ALIGNED_SIZE(x)))

#define HT_INVALID_IDX ((uint32_t) -1)

#define AR_DATA(ht) (Bucket *)((char *)ht + ht->bucketOffset)

#define HVAL(ht, bucket) (Hval *)((char *)ht + bucket->hvalOffset)

#define HT_HASH_EX(data, idx) \
    ((uint32_t*)(data))[(int32_t)(idx)]
#define HT_HASH(ht, idx) \
    HT_HASH_EX(AR_DATA(ht), idx)

#define HT_HASH_TO_BUCKET_EX(data, idx) \
    ((data) + (idx))

#define HT_HASH_TO_BUCKET(ht, idx) \
    HT_HASH_TO_BUCKET_EX(AR_DATA(ht), idx)

#define FOREACH_HASH_TABLE_START(ht) \
                Bucket *bucket = AR_DATA(ht); \
                Bucket *end = bucket + ht->nNumUsed; \
                    for (; bucket != end; bucket++) {

#define FOREACH_END }

typedef enum {
    FLAG_OK = 0,
    FLAG_DELETE = 1,
} FLAG_CODE;

typedef struct {
//    uint64_t atime;
    uint32_t next;      /* hash collision chain */
    uint32_t len;
    uint32_t real_size;
    char data[1];
} Hval;

typedef struct {
    uint64_t h;
    uint32_t hvalOffset; // Hval相对Hash结构体首地址的偏移量
    uint32_t crc;
    uint32_t len;
    uint8_t flag;
    char key[MAX_KEY_LEN];
} Bucket;

/*
 * HashTable Data Layout
 * =====================
 *
 *                 +=============================+
 *                 | HT_HASH(ht, ht->nTableMask) |
 *                 | ...                         |
 *                 | HT_HASH(ht, -1)             |
 *                 +-----------------------------+
 * ht->arData ---> | Bucket[0]                   |
 *                 | ...                         |
 *                 | Bucket[ht->nTableSize-1]    |
 *                 +=============================+
 */

typedef struct {
    uint32_t bucketOffset; // Bucket相对Hash结构体首地址的偏移量
    uint32_t nTableMask;
    uint32_t nNumUsed;
    uint32_t nNumOfElements;
    uint32_t nTableSize;
    uint32_t nInternalPointer;
    uint32_t nNextFreeElement;
    uint32_t conflicts;
    uint32_t hits;
    uint32_t miss;
    uint32_t del_cnt;
} Hash;

typedef struct {
    uint32_t size;
    volatile uint32_t pos;
    uint32_t free;
    uint32_t user_app_pos;
} Segment;

class Hasher {
public:

    explicit Hasher() : Gseg(nullptr), m_ht(nullptr) {}

    explicit Hasher(Segment *seg) : Gseg(seg), m_ht(nullptr) {}

private:
    Segment *Gseg;
    Hash *m_ht;

private:

    void *alloc_mem(uint32_t size);

    Hval *make_hval(const char *data, uint32_t data_len);

    bool init_bucket(Bucket *p, uint64_t h, const char *key, uint32_t key_len,
                     const char *data, uint32_t data_len);

    bool update_bucket(Hash *ht, Bucket *p, const char *data, uint32_t data_len);

public:
    Hash *getHashAddr();

    bool init(uint32_t alloc_size, uint32_t table_size);

    bool hash_delete_bucket(Hash *ht, const char *key, uint32_t len);

    Bucket *hash_find_bucket(Hash *ht, const char *key, uint32_t len);

    bool hash_add_or_update_bucket(Hash *ht, const char *key, uint32_t len,
                                   const char *data, uint32_t size);

    void dump(Hash *ht, bool dump_full = false);
};


template<class Alloc>
class HashTable {
private:
    std::string m_shared_name;
    bool m_readonly;
    std::shared_ptr<Alloc> m_allocator;
    Segment *Gseg;
    std::shared_ptr<Hasher> m_hasher;

public:

    explicit HashTable(const char *shared_name) : m_shared_name(std::move(std::string(shared_name))), m_allocator(
            std::make_shared<Alloc>(m_shared_name)), m_readonly(true), Gseg(nullptr),
                                                  m_hasher(std::move(std::make_shared<Hasher>())) {
    }

    explicit HashTable(const std::string &shared_name) : m_shared_name(shared_name), m_allocator(
            std::make_shared<Alloc>(m_shared_name)), m_readonly(true), Gseg(nullptr),
                                                         m_hasher(std::move(std::make_shared<Hasher>())) {
    }

    ~HashTable() = default;

    std::string Get(const std::string &key);

    std::string Get(const char *key, uint32_t len);

    std::string Get(const char *key);

    bool Exist();

    bool Unlink();

    bool Hinit(uint32_t requested_size);

    bool Hinit(uint32_t key_num, uint32_t value_size);

    bool Set(const std::string &key, const std::string &value);

    bool Set(const char *key, const char *value);

    bool Set(const char *key, uint32_t len,
             const char *data, uint32_t size);

    bool Del(const char *key);

    void Dump();

};

template<class Alloc>
bool HashTable<Alloc>::Exist() {
    return m_allocator->Exist();
}

template<class Alloc>
bool HashTable<Alloc>::Unlink() {
    return m_allocator->Unlink();
}

template<class Alloc>
std::string HashTable<Alloc>::Get(const std::string &key) {
    return Get(key.c_str(), key.size());
}

template<class Alloc>
std::string HashTable<Alloc>::Get(const char *key) {
    return Get(key, strlen(key));
}

template<class Alloc>
std::string HashTable<Alloc>::Get(const char *key, uint32_t len) {
    if (m_readonly && !Gseg) {
        Gseg = (Segment *) m_allocator->OpenR();
        if (Gseg) {
            m_hasher = std::make_shared<Hasher>(Gseg);
        }
    }
    Hash *ht = m_hasher->getHashAddr();
    if (!ht) {
        return {};
    }
    Bucket *bucket = m_hasher->hash_find_bucket(ht, key, len);
    if (!bucket) {
        return {};
    }
    Hval *val = HVAL(ht, bucket);
    if (val && val->len > 0) {
        return std::string(val->data, val->len);
    }

    return {};
}

template<class Alloc>
bool HashTable<Alloc>::Set(const std::string &key, const std::string &value) {
    return Set(key, key.size(), value, value.size());
}

template<class Alloc>
bool HashTable<Alloc>::Set(const char *key, uint32_t len,
                           const char *data, uint32_t size) {
    if (m_readonly) {
        return false;
    }
    Hash *ht = m_hasher->getHashAddr();
    if (!ht) {
        return false;
    }
    return m_hasher->hash_add_or_update_bucket(ht, key, len, data, size);
}

template<class Alloc>
bool HashTable<Alloc>::Set(const char *key, const char *value) {

    return Set(key, strlen(key), value, strlen(value));
}

template<class Alloc>
bool HashTable<Alloc>::Del(const char *key) {
    if (m_readonly) {
        return false;
    }
    Hash *ht = m_hasher->getHashAddr();
    if (!ht) {
        return false;
    }
    return m_hasher->hash_delete_bucket(ht, key, strlen(key));
}

static inline unsigned int align_size(uint32_t size) {
    int bits = 1;
    while ((size = size >> 1)) {
        ++bits;
    }
    return (1 << bits);
}

//template<class Alloc>
//bool HashTable<Alloc>::Hinit(uint32_t k_size, uint32_t v_size) {
//    if (k_size == 0 || v_size == 0) {
//        return false;
//    }
//    m_readonly = false;
//    uint32_t alloc_size = MEM_TRUE_SIZE(k_size + v_size);
//    Gseg = (Segment *) m_allocator->OpenW(alloc_size);
//
//    int table_size = align_size(k_size / sizeof(Bucket));
//    if (!((k_size / sizeof(Bucket)) & ~(table_size << 1))) {
//        table_size <<= 1;
//    }
//
//    return init(alloc_size, table_size);
//}

template<class Alloc>
bool HashTable<Alloc>::Hinit(uint32_t table_size, uint32_t v_size) {
    if (!table_size || !v_size) return false;
    m_readonly = false;

    // Avoid Hash Collisions
    int real_size = align_size(table_size);
    if (!(table_size & ~(real_size << 1))) {
        real_size <<= 1;
    }

    uint32_t requested_size =
            sizeof(Segment) + sizeof(Hash) + real_size * (sizeof(uint32_t) + sizeof(Bucket) + sizeof(Hval)) +
            MEM_ALIGNED_SIZE(v_size);
    uint32_t alloc_size = MEM_TRUE_SIZE(requested_size);
    Gseg = (Segment *) m_allocator->OpenW(alloc_size);

    m_hasher = std::make_shared<Hasher>(Gseg);
    return m_hasher->init(alloc_size, real_size);
}

template<class Alloc>
void HashTable<Alloc>::Dump() {
    if (m_readonly && !Gseg) {
        Gseg = (Segment *) m_allocator->OpenR();
        if (Gseg) {
            m_hasher = std::make_shared<Hasher>(Gseg);
        }
    }
    Hash *ht = m_hasher->getHashAddr();
    if (!ht) {
        return;
    }
    m_hasher->dump(ht);
}


#endif //DICTX_HASHTABLE_H

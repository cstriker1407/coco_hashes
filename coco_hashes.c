/*
*/

#include "coco_hashes.h"

typedef struct __cc_hashnode
{
    unsigned int key_len;
    void *data;
    struct __cc_hashnode *next;

    unsigned char key[0];
}cc_hashnode;

typedef struct
{
    cc_hashnode **table_arr;
    unsigned int table_size;

    unsigned int (*hash_calc)(coco_hashctx ctx, void *key, unsigned int key_len);
    int (*key_cmp)(coco_hashctx ctx, void *key1, unsigned int key1_len, void *key2, unsigned int key2_len);

    void *(*sys_calloc)(unsigned int nmemb, unsigned int size);
    void (*sys_free)(void *ptr);

    void (*sys_lock)(coco_hashctx ctx);
    void (*sys_unlock)(coco_hashctx ctx);
}cc_hashctx;



int coco_hash_init(coco_hashctx *p_ctx,unsigned int table_size,
                   unsigned int (*hash_calc)(coco_hashctx ctx, void *key, unsigned int key_len),
                   int (*key_cmp)(coco_hashctx ctx, void *key1, unsigned int key1_len, void *key2, unsigned int key2_len),
                   void *(*sys_calloc)(unsigned int nmemb, unsigned int size),
                   void (*sys_free)(void *ptr),
                   void (*sys_lock)(coco_hashctx ctx),
                   void (*sys_unlock)(coco_hashctx ctx)
                   )
{
    if( (coco_null == p_ctx) || (coco_null == hash_calc) || (coco_null == key_cmp) ||
        (coco_null == sys_calloc) || (coco_null == sys_free) || (coco_null == sys_lock) || (coco_null == sys_unlock) )
    {
        coco_error("input is invalid");
        return -1;
    }

    *p_ctx = coco_null;
    cc_hashctx *p_hashctx = (cc_hashctx *)sys_calloc( 1, sizeof(cc_hashctx) );
    if(coco_null == p_hashctx)
    {
        coco_error("sys_malloc fails. size:%u", sizeof(cc_hashctx) );
        return -2;
    }

    p_hashctx->table_arr = (cc_hashnode **)sys_calloc( table_size, sizeof(cc_hashnode *) );
    if(coco_null == p_hashctx->table_arr)
    {
        coco_error("sys_malloc fails. size:%u", sizeof(cc_hashnode *) * table_size );
        sys_free(p_hashctx);
        return -3;
    }

    p_hashctx->table_size = table_size;
    p_hashctx->hash_calc = hash_calc;
    p_hashctx->key_cmp = key_cmp;
    p_hashctx->sys_calloc = sys_calloc;
    p_hashctx->sys_free = sys_free;
    p_hashctx->sys_lock = sys_lock;
    p_hashctx->sys_unlock = sys_unlock;
    *p_ctx = p_hashctx;

    coco_debug("init hashmap with table_size:%u success", table_size);
    return 0;
}

int coco_hash_insert(coco_hashctx ctx, void *key, unsigned int key_len, void *data)
{
    if( (coco_null == ctx) || (coco_null == key) || (0 == key_len) || (coco_null == data) )
    {
        coco_error("input is invalid");
        return -1;
    }

    cc_hashctx *p_hashctx = (cc_hashctx *)ctx;
    cc_hashnode *p_node = (cc_hashnode *)( p_hashctx->sys_calloc( 1, sizeof(cc_hashnode) + key_len ) );
    if(coco_null == p_node)
    {
        coco_error("sys_malloc fails. size:%u", sizeof(cc_hashnode) + key_len );
        return -2;
    }
    p_node->key_len = key_len;
    p_node->data = data;
    c_memcpy(p_node->key, key, key_len);

    unsigned int key_hash = p_hashctx->hash_calc(ctx, key, key_len);
    key_hash = key_hash % p_hashctx->table_size;

    p_hashctx->sys_lock(p_hashctx);

    p_node->next = p_hashctx->table_arr[key_hash];
    p_hashctx->table_arr[key_hash] = p_node;

    p_hashctx->sys_unlock(p_hashctx);

    coco_debug("hash inset key_len:%u and calc_hash:%u", key_len, key_hash);
    return 0;
}

int coco_hash_delete(coco_hashctx ctx, void *key, unsigned int key_len)
{
    if( (coco_null == ctx) || (coco_null == key) || (0 == key_len) )
    {
        coco_error("input is invalid");
        return -1;
    }
    cc_hashctx *p_hashctx = (cc_hashctx *)ctx;
    unsigned int key_hash = p_hashctx->hash_calc(ctx, key, key_len);
    key_hash = key_hash % p_hashctx->table_size;

    p_hashctx->sys_lock(p_hashctx);

    cc_hashnode *p_pre_node = p_hashctx->table_arr[key_hash];
    cc_hashnode *p_cur_node = coco_null;
    if(p_pre_node != coco_null)
        p_cur_node = p_pre_node->next;
    while(p_cur_node != coco_null)
    {
        if( (key_len == p_cur_node->key_len) && (c_memcmp(p_cur_node->key, key, key_len) == 0) )
        {
            coco_debug("hash delete key_len:%u and calc_hash:%u success", key_len, key_hash);
            p_pre_node->next = p_cur_node->next;
            p_hashctx->sys_free(p_cur_node);
            p_hashctx->sys_unlock(p_hashctx);
            return 0;
        }
        p_pre_node = p_cur_node;
        p_cur_node = p_cur_node->next;
    }

    p_hashctx->sys_unlock(p_hashctx);
    coco_debug("hash delete key_len:%u and calc_hash:%u fail.not find", key_len, key_hash);
    return -1;
}

int coco_hash_search(coco_hashctx ctx, void *key, unsigned int key_len, void **p_data)
{
    if( (coco_null == ctx) || (coco_null == key) || (0 == key_len) || (coco_null == p_data) )
    {
        coco_error("input is invalid");
        return -1;
    }
    cc_hashctx *p_hashctx = (cc_hashctx *)ctx;
    unsigned int key_hash = p_hashctx->hash_calc(ctx, key, key_len);
    key_hash = key_hash % p_hashctx->table_size;

    p_hashctx->sys_lock(p_hashctx);

    cc_hashnode *p_node = p_hashctx->table_arr[key_hash];
    while(p_node != coco_null)
    {
        if( (key_len == p_node->key_len) && (c_memcmp(p_node->key, key, key_len) == 0) )
        {
            coco_debug("hash search key_len:%u and calc_hash:%u success", key_len, key_hash);
            *p_data = p_node->data;
            p_hashctx->sys_unlock(p_hashctx);
            return 0;
        }
        p_node = p_node->next;
    }

    *p_data = coco_null;
    p_hashctx->sys_unlock(p_hashctx);
    coco_debug("hash search key_len:%u and calc_hash:%u fail.not find", key_len, key_hash);
    return -1;
}

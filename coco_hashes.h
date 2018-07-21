/*
*/

#ifndef COCO_HASHES_H
#define COCO_HASHES_H


#include <string.h>
#define c_memcpy    memcpy
#define c_memcmp    memcmp

#define coco_null   0
#define coco_debug(fmt, ...) printf("[DBG %4d]"fmt"\r\n", __LINE__, ##__VA_ARGS__)
#define coco_error(fmt, ...) printf("[ERR %4d]"fmt"\r\n", __LINE__, ##__VA_ARGS__)


#ifdef __cplusplus
extern "C" {
#endif

typedef void * coco_hashctx;

int coco_hash_init(coco_hashctx *p_ctx,unsigned int table_size,
                   unsigned int (*hash_calc)(coco_hashctx ctx, void *key, unsigned int key_len),
                   int (*key_cmp)(coco_hashctx ctx, void *key1, unsigned int key1_len, void *key2, unsigned int key2_len),
                   void *(*sys_calloc)(unsigned int nmemb, unsigned int size),
                   void (*sys_free)(void *ptr),
                   void (*sys_lock)(coco_hashctx ctx),
                   void (*sys_unlock)(coco_hashctx ctx)
                   );

int coco_hash_insert(coco_hashctx ctx, void *key, unsigned int key_len, void *data);

int coco_hash_delete(coco_hashctx ctx, void *key, unsigned int key_len);

int coco_hash_search(coco_hashctx ctx, void *key, unsigned int key_len, void **p_data);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif // COCO_HASHES_H

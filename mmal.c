/**
 * Implementace My MALloc
 * Demonstracni priklad pro 1. ukol IPS/2019
 * Ales Smrcka
 */

#include "mmal.h"
#include <sys/mman.h> // mmap
#include <stdbool.h> // bool
#include <assert.h> // assert
#include <string.h> // memcpy

#ifdef NDEBUG
/**
 * The structure header encapsulates data of a single memory block.
 *   ---+------+----------------------------+---
 *      |Header|DDD not_free DDDDD...free...|
 *   ---+------+-----------------+----------+---
 *             |-- Header.asize -|
 *             |-- Header.size -------------|
 */
typedef struct header Header;
struct header {

    /**
     * Pointer to the next header. Cyclic list. If there is no other block,
     * points to itself.
     */
    Header *next;

    /// size of the block
    size_t size;

    /**
     * Size of block in bytes allocated for program. asize=0 means the block 
     * is not used by a program.
     */
    size_t asize;
};

/**
 * The arena structure.
 *   /--- arena metadata
 *   |     /---- header of the first block
 *   v     v
 *   +-----+------+-----------------------------+
 *   |Arena|Header|.............................|
 *   +-----+------+-----------------------------+
 *
 *   |--------------- Arena.size ---------------|
 */
typedef struct arena Arena;
struct arena {

    /**
     * Pointer to the next arena. Single-linked list.
     */
    Arena *next;

    /// Arena size.
    size_t size;
};

#define PAGE_SIZE (128*1024)

#endif // NDEBUG

Arena *first_arena = NULL;

static size_t allign_page(size_t size);
static Arena *arena_alloc(size_t req_size);
static void arena_append(Arena *a);
static void hdr_ctor(Header *hdr, size_t size);
static bool hdr_should_split(Header *hdr, size_t size);
static Header *hdr_split(Header *hdr, size_t req_size);
static bool hdr_can_merge(Header *left, Header *right);
static void hdr_merge(Header *left, Header *right);
static Header *best_fit(size_t size);
static Header *hdr_get_prev(Header *hdr);
static Arena *getLastArena();

/**
 * Return size alligned to PAGE_SIZE
 */
static
size_t allign_page(size_t size)
{
    // FIXME
    //(void)size;
    //return size;
    size_t modulo = size % PAGE_SIZE;
    //if(!modulo) return size;
    return size + PAGE_SIZE - modulo; 
    //return (((size / PAGE_SIZE)+1) * PAGE_SIZE);
}

/**
 * Allocate a new arena using mmap.
 * @param req_size requested size in bytes. Should be alligned to PAGE_SIZE.
 * @return pointer to a new arena, if successfull. NULL if error.
 * @pre req_size > sizeof(Arena) + sizeof(Header)
 */

/**
 *   +-----+------------------------------------+
 *   |Arena|....................................|
 *   +-----+------------------------------------+
 *
 *   |--------------- Arena.size ---------------|
 */
static
Arena *arena_alloc(size_t req_size)
{
    // FIXME
    //(void)req_size;
    //return NULL;
    req_size = allign_page(req_size + sizeof(Header));
    if(req_size < sizeof(Arena) + sizeof(Header)) return NULL;

    Arena *ap = (Arena*)mmap(0, req_size + sizeof(Arena), PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1,0);
    if(!ap) return NULL; //mmap failed to allocate memory

    ap->next = NULL;
    ap->size = req_size;
    Header *first_h = (Header*)&ap[1];
    hdr_ctor(first_h, req_size - sizeof(Header));
    first_h->next = first_h;

    return ap;
}

/**
 * Appends a new arena to the end of the arena list.
 * @param a     already allocated arena
 */
static
void arena_append(Arena *a)
{
    // FIXME
    //(void)a;
    Arena * iterator = getLastArena();
    iterator->next = a;
}

/**
 * Header structure constructor (alone, not used block).
 * @param hdr       pointer to block metadata.
 * @param size      size of free block
 * @pre size > 0
 */
/**
 *   +-----+------+------------------------+----+
 *   | ... |Header|........................| ...|
 *   +-----+------+------------------------+----+
 *
 *                |-- Header.size ---------|
 */
static
void hdr_ctor(Header *hdr, size_t size)
{
    // FIXME
    //(void)hdr;
    //(void)size;
    if(size > 0){
        hdr->size = size;
        hdr->asize = 0;
        hdr->next = NULL;
    }
}

/**
 * Checks if the given free block should be split in two separate blocks.
 * @param hdr       header of the free block
 * @param size      requested size of data
 * @return true if the block should be split
 * @pre hdr->asize == 0
 * @pre size > 0
 */
static
bool hdr_should_split(Header *hdr, size_t size)
{
    // FIXME
    //(void)hdr;
    //(void)size;
    assert(size > 0);
    //if(size > 0 && hdr->asize == 0){
        return hdr->size - sizeof(Header) - size > 0;
    //}
    //return false;
}

/**
 * Splits one block in two.
 * @param hdr       pointer to header of the big block
 * @param req_size  requested size of data in the (left) block.
 * @return pointer to the new (right) block header.
 * @pre   (hdr->size >= req_size + 2*sizeof(Header))
 */
/**
 * Before:        |---- hdr->size ---------|
 *
 *    -----+------+------------------------+----
 *         |Header|........................|
 *    -----+------+------------------------+----
 *            \----hdr->next---------------^
 */
/**
 * After:         |- req_size -|
 *
 *    -----+------+------------+------+----+----
 *     ... |Header|............|Header|....|
 *    -----+------+------------+------+----+----
 *             \---next--------^  \--next--^
 */
static
Header *hdr_split(Header *hdr, size_t req_size)
{
    // FIXME
    //the space is not enought for 2, not splitting
    //using the given header
    if(!hdr_should_split(hdr, req_size)) return hdr;
    Header *right = (Header*)((char*)&hdr[1] + sizeof(Header) + req_size);//(Header*)(((char*)&hdr) + req_size + sizeof(Header));
    hdr_ctor(right, hdr->size - sizeof(Header) - req_size);
    right->next = hdr->next;
    hdr->next = right;
    hdr->size = req_size;

    return right;
}

/**
 * Detect if two adjacent blocks could be merged.
 * @param left      left block
 * @param right     right block
 * @return true if two block are free and adjacent in the same arena.
 * @pre left->next == right
 * @pre left != right
 */
static
bool hdr_can_merge(Header *left, Header *right)
{
    // FIXME
    //(void)left;
    //(void)right;
    if(left->next == right && left != right && left < right &&
            left->asize == 0 && right->asize == 0) return true;
    return false;
}

/**
 * Merge two adjacent free blocks.
 * @param left      left block
 * @param right     right block
 * @pre left->next == right
 * @pre left != right
 */
static
void hdr_merge(Header *left, Header *right)
{
    //(void)left;
    //(void)right;
    // FIXME
    if(hdr_can_merge(left, right)){
        left->size += sizeof(Header) + right->size;
        left->next = right->next;
        right->next = NULL;
    }
}

/**
 * Finds the free block that fits best to the requested size.
 * @param size      requested size
 * @return pointer to the header of the block or NULL if no block is available.
 * @pre size > 0
 */
static
Header *best_fit(size_t size)
{
    // FIXME
    //(void)size;
    Header *best = NULL;//(Header*)&first_arena[1];
    Header *first = (Header*)&first_arena[1];
    Arena *it_a = first_arena;
    Header *it_h = first->next;

    while(it_a != NULL){
        do{//while(it_h != first){ //perhaps error here?
            if(!it_h->asize){
                if(best == NULL){
                    if(it_h->size > size) best = it_h;
                }else{
                    if(it_h->size == size) return it_h;
                    if(it_h->size > size && it_h->size < best->size) best = it_h;
                }
            }
            //if(size == it_h->size) return it_h; //correct match
            //if(best->asize || (it_h->size < best->size && it_h->size > size)) best = it_h;
            it_h = it_h->next; //inc iterator
        }while(it_h != first);
        it_a = it_a->next; //inc iterator
        if(it_a){
            first = (Header*)&it_a[1];
            it_h = first->next;
        }//wtf
    }

    //if(best->asize) return NULL; //not free
    //if(best->size < size) return NULL; //not enough free space
    return best;
}

/**
 * Search the header which is the predecessor to the hdr. Note that if 
 * @param hdr       successor of the search header
 * @return pointer to predecessor, hdr if there is just one header.
 * @pre first_arena != NULL
 * @post predecessor->next == hdr
 */
static
Header *hdr_get_prev(Header *hdr)
{
    // FIXME
    //(void)hdr;
    if(first_arena == NULL) return NULL;
    if(hdr->next == hdr) return hdr;
    Header *it_h = (Header*)&first_arena[1];
    
    while(it_h->next != hdr) it_h = it_h->next;

    return it_h;
}

static
Arena *getLastArena(){
    Arena *it_a = first_arena;
    while(it_a->next != NULL) it_a = it_a->next;
    return it_a;
}

/**
 * Allocate memory. Use best-fit search of available block.
 * @param size      requested size for program
 * @return pointer to allocated data or NULL if error or size = 0.
 */
void *mmalloc(size_t size)
{
    // FIXME
    //(void)size;
    //return NULL;
    if(first_arena == NULL) first_arena = arena_alloc(size);
    Header *hdr = best_fit(size);
    if(hdr == NULL){
        Arena *last = getLastArena();
        last->next = arena_alloc(size);
        hdr = best_fit(size);
    }
    Header *right = hdr_split(hdr, size);
    if(hdr_can_merge(right, right->next)) hdr_merge(right, right->next);
    hdr->asize = size;
    return (void*)hdr + sizeof(Header);
}

/**
 * Free memory block.
 * @param ptr       pointer to previously allocated data
 * @pre ptr != NULL
 */
void mfree(void *ptr)
{
    //(void)ptr;
    // FIXME
    Header *hdr = (Header*)(ptr - sizeof(Header));
    hdr->asize = 0;
    if(hdr_can_merge(hdr, hdr->next)) hdr_merge(hdr, hdr->next);
    Header *prev = hdr_get_prev(hdr);
    if(hdr_can_merge(prev, hdr)) hdr_merge(prev, hdr);
}

/**
 * Reallocate previously allocated block.
 * @param ptr       pointer to previously allocated data
 * @param size      a new requested size. Size can be greater, equal, or less
 * then size of previously allocated block.
 * @return pointer to reallocated space.
 */
void *mrealloc(void *ptr, size_t size)
{
    // FIXME
    (void)ptr;
    (void)size;
    return NULL;
}

void *mcalloc(size_t size){
    (void)size;
    return NULL;
}

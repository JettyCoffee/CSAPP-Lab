/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "Coffee",
    /* First member's full name */
    "JettyCoffee",
    /* First member's email address */
    "10235501454@stu.ecnu.edu.cn",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

//注释采用中文
#define ALIGNMENT 8 //ALIGNMENT是8字节对齐
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7) //ALIGN函数将size向上对齐
//SIZE_T_SIZE是size_t的大小
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))
#define WSIZE 4 //字长大小
#define DSIZE 8 //双字大小
#define CLASS_SIZE 20 //大小类的数量

#define CHUNKSIZE (1<<12) //初始堆大小和扩展堆大小
#define MAX(x, y) ((x) > (y) ? (x) : (y))
#define PACK(size, alloc) ((size) | (alloc)) //将大小和分配位打包

#define GET(p) (*(unsigned int *)(p)) //将地址p处的值读出
#define GET_HEAD(class) ((unsigned int *)(GET(heap_listp + (class * WSIZE)))) //获取大小类的头指针
#define PUT(p, val) (*(unsigned int *)(p) = (val)) //将值val写入地址p处

#define GET_SIZE(p) (GET(p) & ~0x7) //当前块中的block_size读出
#define GET_ALLOC(p) (GET(p) & 0x1) //将当前块中的alloc状态读出

#define HDRP(bp) ((char *)(bp) - WSIZE) //返回块的头部地址
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //返回块的脚部地址

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //返回下一个块的地址
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //返回上一个块的地址

#define GET_PREV(bp) (*(char **)(bp)) //找出前序块指针
#define GET_NEXT(bp) (*(char **)(bp + WSIZE)) //找出后继块指针

//函数声明段
static char *heap_listp = 0;
static void *extend_heap(size_t words);
int select_class(size_t size);
void insert_block(void *bp);
void delete_block(void *bp);
static void *coalesce(void *bp);
void place(void *bp, size_t asize);
void *first_fit(size_t asize);
void *best_fit(size_t asize);

//初始化堆
int mm_init(void)
{
    //申请4个字长的空间
    if((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    //申请20个大小类头指针
    for(int i = 0; i < CLASS_SIZE; i++) {
        PUT(heap_listp + (i * WSIZE), 0);
    }

    PUT(heap_listp + CLASS_SIZE * WSIZE, 0); //对齐填充?
    PUT(heap_listp + ((1 + CLASS_SIZE) * WSIZE), PACK(DSIZE, 1)); //序言块头部
    PUT(heap_listp + ((2 + CLASS_SIZE) * WSIZE), PACK(DSIZE, 1)); //序言块脚部
    PUT(heap_listp + ((3 + CLASS_SIZE) * WSIZE), PACK(0, 1)); //结尾块头部

    if(extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

//扩展堆:words是要扩展的字节数
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;
    //根据传入的字节数奇偶性来决定是否需要对齐
    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
    //为bp分配size大小的空间
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    //初始化新块的头部和脚部
    PUT(HDRP(bp), PACK(size, 0)); //free block header
    PUT(FTRP(bp), PACK(size, 0)); //free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //new epilogue header
    //判断前一个块是否是空闲块，如果是则合并
    return coalesce(bp);
}

//根据块大小选择合适的大小类
int select_class(size_t size){
    for(int i=4; i<(CLASS_SIZE + 4 - 1); i++){
        if(size <= (1 << i)){
            return i - 4;
        }
    }
    return CLASS_SIZE - 1;
}

//插入块，并将块插入到对应的大小类的表头
void insert_block(void *bp) {
    size_t size = GET_SIZE(HDRP(bp)); // 获取块大小
    int class = select_class(size);   // 获取块的大小类
    
    if (GET_HEAD(class) == NULL) {
        PUT((unsigned int *)(heap_listp + (class * WSIZE)), (unsigned int)bp);
        PUT((unsigned int *)bp, (unsigned int)NULL);
        PUT((unsigned int *)(bp + WSIZE), (unsigned int)NULL);
    } else {
        // 将块的后继指针指向原来的表头
        PUT((unsigned int *)(bp + WSIZE), (unsigned int)GET_HEAD(class)); 
        // 将原来的表头的前驱指针指向新块
        PUT((unsigned int *)GET_HEAD(class), (unsigned int)bp);
        // 将新块的前驱指针置空
        PUT((unsigned int *)bp, (unsigned int)NULL);
        // 将新块设置为表头
        PUT((unsigned int *)(heap_listp + (class * WSIZE)), (unsigned int)bp);
    }
}


//删除块并清理指针
void delete_block(void *bp) {
    size_t size = GET_SIZE(HDRP(bp)); // 获取块大小
    int class = select_class(size);   // 获取块的大小类

    // PREV 和 NEXT 都为空 -> 头节点设为 NULL
    if (GET_PREV(bp) == NULL && GET_NEXT(bp) == NULL) {
        PUT((unsigned int *)(heap_listp + (class * WSIZE)), (unsigned int)NULL);
    // PREV 为空，NEXT 不为空 (第一个节点) -> 头节点设为 NEXT
    } else if (GET_PREV(bp) == NULL && GET_NEXT(bp) != NULL) {
        PUT((unsigned int *)(heap_listp + (class * WSIZE)), (unsigned int)GET_NEXT(bp));
        PUT((unsigned int *)GET_NEXT(bp), (unsigned int)NULL);
    // PREV 不为空，NEXT 为空 (最后一个节点) -> PREV 的 NEXT 设为 NULL
    } else if (GET_PREV(bp) != NULL && GET_NEXT(bp) == NULL) {
        PUT((unsigned int *)(GET_PREV(bp) + WSIZE), (unsigned int)NULL);
    }
    // PREV 和 NEXT 都不为空 -> PREV 的 NEXT 设为 NEXT，NEXT 的 PREV 设为 PREV
    else {
        PUT((unsigned int *)(GET_PREV(bp) + WSIZE), (unsigned int)GET_NEXT(bp));
        PUT((unsigned int *)GET_NEXT(bp), (unsigned int)GET_PREV(bp));
    }
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}


//分离空闲块
void place(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    delete_block(bp); //删除空闲块
    if((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp); //指向剩余的空闲块
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        insert_block(bp); //插入新的空闲块
    }
    //如果剩余块的大小小于2*DSIZE，则不分离(保证对齐)
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

//合并空闲块
static void *coalesce(void *bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) { // case 1: 前后都分配
        insert_block(bp);
        return bp;
    } else if (prev_alloc && !next_alloc) { // case 2: 前分配，后未分配
        delete_block(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) { // case 3: 前未分配，后分配
        delete_block(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else { // case 4: 前后都未分配
        delete_block(PREV_BLKP(bp));
        delete_block(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    insert_block(bp);
    return bp;
}


void *first_fit(size_t asize) {
    int class = select_class(asize);
    unsigned int *bp;

    // 如果找不到合适的块，那么就搜索下一个更大的大小类
    while (class < CLASS_SIZE) {
        bp = (unsigned int *)GET_HEAD(class);
        // 不为空则寻找
        while (bp) {
            if (GET_SIZE(HDRP(bp)) >= asize) {
                return (void *)bp;
            }
            // 用后继找下一块
            bp = (unsigned int *)GET_NEXT(bp);  // 强制转换为 unsigned int *
        }
        // 找不到则进入下一个大小类
        class++;
    }
    return NULL;
}


void *best_fit(size_t asize){
    void *bp;
    void *best_bp = NULL;
    size_t min_size = 0;
    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if((GET_SIZE(HDRP(bp)) >= asize) && (!GET_ALLOC(HDRP(bp)))){
            if(min_size ==0 || min_size > GET_SIZE(HDRP(bp))){
                min_size = GET_SIZE(HDRP(bp));
                best_bp = bp;
            }
        }
    }
    return best_bp;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size) {
    size_t asize; // 调整后的块大小
    size_t extendsize; // 扩展堆大小
    char *bp;

    if (size == 0)
        return NULL;
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + DSIZE - 1) / DSIZE);

    if ((bp = first_fit(asize)) != NULL) {
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}


/*
 * mm_realloc - 实现重新分配内存块，保留旧块的数据
 */
void *mm_realloc(void *ptr, size_t size)
{
    /* size可能为0,则mm_malloc返回NULL */
    void *newptr;
    size_t copysize;
    
    if((newptr = mm_malloc(size))==NULL)
        return 0;
    copysize = GET_SIZE(HDRP(ptr));
    if(size < copysize)
        copysize = size;
    memcpy(newptr, ptr, copysize);
    mm_free(ptr);
    return newptr;
}
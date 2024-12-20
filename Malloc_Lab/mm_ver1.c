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

//ALIGNMENT是8字节对齐
#define ALIGNMENT 8

//ALIGN函数将size向上对齐
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)

//SIZE_T_SIZE是size_t的大小
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

#define WSIZE 4 //字长大小
#define DSIZE 8 //双字大小
#define CHUNKSIZE (1<<12) //初始堆大小和扩展堆大小

#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc)) //将大小和分配位打包

//将地址p处的值读出
#define GET(p) (*(unsigned int *)(p))
//将值val写入地址p处
#define PUT(p, val) (*(unsigned int *)(p) = (val))

//将地址p处的SIZE读出
#define GET_SIZE(p) (GET(p) & ~0x7)
//将地址p处的分配位读出
#define GET_ALLOC(p) (GET(p) & 0x1)

//bp是块的地址，返回块的头部地址
#define HDRP(bp) ((char *)(bp) - WSIZE)
//bp是块的地址，返回块的脚部地址
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

//bp是块的地址，返回下一个块的地址
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
//bp是块的地址，返回上一个块的地址
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


static char *heap_listp = 0;
static void *coalesce(void *bp);
void divided(void *bp, size_t asize);
void first_fit(size_t asize);
void best_fit(size_t asize);


//coalesce函数合并空闲块
static void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE; //保证对齐
    if((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    PUT(HDRP(bp), PACK(size, 0)); //free block header
    PUT(FTRP(bp), PACK(size, 0)); //free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); //new epilogue header

    return coalesce(bp);
}


int mm_init(void)
{
    if((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)
        return -1;

    PUT(heap_listp, 0); //对齐填充
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1)); //prologue header
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1)); //prologue footer
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1)); //epilogue header

    heap_listp += (2 * WSIZE); //指向prologue footer后的第一个字节

    if(extend_heap(CHUNKSIZE / WSIZE) == NULL)
        return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize; //调整后的块大小
    size_t extendsize; //扩展堆大小
    char *bp;

    if(size == 0)
        return NULL;

    if(size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + DSIZE + DSIZE - 1) / DSIZE);

    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            divided(bp, asize);
            return bp;
        }
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize / WSIZE)) == NULL)
        return NULL;
    divided(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));

    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    coalesce(ptr);
}

void divided(void *bp, size_t asize)
{
    size_t csize = GET_SIZE(HDRP(bp));
    if((csize - asize) >= (2 * DSIZE)) {
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
    }
    else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if(prev_alloc && next_alloc) { //case 1:前后都分配
        return bp;
    }
    else if(prev_alloc && !next_alloc) { //case 2:前分配，后未分配
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }
    else if(!prev_alloc && next_alloc) { //case 3:前未分配，后分配
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    else { //case 4:前后都未分配
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    return bp;
}

void first_fit(size_t asize)
{
    void *bp;
    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            divided(bp, asize);
            return;
        }
    }
    if((bp = extend_heap(MAX(asize, CHUNKSIZE) / WSIZE)) == NULL)
        return;
    divided(bp, asize);
}

void best_fit(size_t asize)
{
    void *bp;
    void *bestp = NULL;
    size_t min = 0x7fffffff;
    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
            if(GET_SIZE(HDRP(bp)) < min) {
                min = GET_SIZE(HDRP(bp));
                bestp = bp;
            }
        }
    }
    if(bestp == NULL) {
        if((bp = extend_heap(MAX(asize, CHUNKSIZE) / WSIZE)) == NULL)
            return;
        divided(bp, asize);
    }
    else {
        divided(bestp, asize);
    }
}

/*
 * mm_realloc - 实现重新分配内存块，保留旧块的数据
 */
void *mm_realloc(void *ptr, size_t size)
{
    if (ptr == NULL) {
        // 如果 ptr 为 NULL，则行为与 malloc 一致
        return mm_malloc(size);
    }

    if (size == 0) {
        // 如果 size 为 0，则行为与 free 一致，并返回 NULL
        mm_free(ptr);
        return NULL;
    }

    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    // 获取旧块的有效负载大小
    copySize = GET_SIZE(HDRP(oldptr)) - DSIZE;

    // 分配新块
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    // 复制数据
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);

    // 释放旧块
    mm_free(oldptr);
    return newptr;
}
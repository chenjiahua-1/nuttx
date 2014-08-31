/****************************************************************************
 * include/nuttx/mm.h
 *
 *   Copyright (C) 2007-2009, 2013-2014 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_MM_H
#define __INCLUDE_NUTTX_MM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <semaphore.h>

/****************************************************************************
 * Pre-Processor Definitions
 ****************************************************************************/
/* Configuration ************************************************************/
/* If the MCU has a small (16-bit) address capability, then we will use
 * a smaller chunk header that contains 16-bit size/offset information.
 * We will also use the smaller header on MCUs with wider addresses if
 * CONFIG_MM_SMALL is selected.  This configuration is common with MCUs
 * that have a large FLASH space, but only a tiny internal SRAM.
 */

#ifdef CONFIG_SMALL_MEMORY
  /* If the MCU has a small addressing capability, then for the smaller
   * chunk header.
   */

#  undef  CONFIG_MM_SMALL
#  define CONFIG_MM_SMALL 1
#endif

/* Decide if there is a user heap.  CONFIG_MM_USER_HEAP=n does not not
 * really that there is no user heap but, rather, that there is no
 * user heap available from within the kernel.  The user heap is
 * Available if:
 *
 *   1. The code is begin build for kernel space and this is a FLAT build
 *      (CONFIG_BUILD_FLAT=y),
 *   2. The code is begin build for kernel space and this is a protected
 *      build (CONFIG_BUILD_PROTECTED=y), OR
 *   3. The code is begin build for user space.
 */

#undef CONFIG_MM_USER_HEAP
#if !defined(CONFIG_BUILD_KERNEL) || !defined(__KERNEL__)
#  define CONFIG_MM_USER_HEAP 1
#endif

/* The kernel heap is never accessible from user code */

#ifndef __KERNEL__
#  undef CONFIG_MM_KERNEL_HEAP
#endif

/* Chunk Header Definitions *************************************************/
/* These definitions define the characteristics of allocator
 *
 * MM_MIN_SHIFT is used to define MM_MIN_CHUNK.
 * MM_MIN_CHUNK - is the smallest physical chunk that can
 *   be allocated.  It must be at least a large as
 *   sizeof(struct mm_freenode_s).  Larger values may
 *   improve performance slightly, but will waste memory
 *   due to quantization losses.
 *
 * MM_MAX_SHIFT is used to define MM_MAX_CHUNK
 * MM_MAX_CHUNK is the largest, contiguous chunk of memory
 *   that can be allocated.  It can range from 16-bytes to
 *   4Gb.  Larger values of MM_MAX_SHIFT can cause larger
 *   data structure sizes and, perhaps, minor performance
 *   losses.
 */

#ifdef CONFIG_MM_SMALL
#  define MM_MIN_SHIFT    4  /* 16 bytes */
#  define MM_MAX_SHIFT   15  /* 32 Kb */
#else
#  define MM_MIN_SHIFT    4  /* 16 bytes */
#  define MM_MAX_SHIFT   22  /*  4 Mb */
#endif

/* All other definitions derive from these two */

#define MM_MIN_CHUNK     (1 << MM_MIN_SHIFT)
#define MM_MAX_CHUNK     (1 << MM_MAX_SHIFT)
#define MM_NNODES        (MM_MAX_SHIFT - MM_MIN_SHIFT + 1)

#define MM_GRAN_MASK     (MM_MIN_CHUNK-1)
#define MM_ALIGN_UP(a)   (((a) + MM_GRAN_MASK) & ~MM_GRAN_MASK)
#define MM_ALIGN_DOWN(a) ((a) & ~MM_GRAN_MASK)

/* An allocated chunk is distinguished from a free chunk by bit 31 (or 15)
 * of the 'preceding' chunk size.  If set, then this is an allocated chunk.
 */

#ifdef CONFIG_MM_SMALL
# define MM_ALLOC_BIT    0x8000
#else
# define MM_ALLOC_BIT    0x80000000
#endif
#define MM_IS_ALLOCATED(n) \
  ((int)((struct mm_allocnode_s*)(n)->preceding) < 0))

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Determines the size of the chunk size/offset type */

#ifdef CONFIG_MM_SMALL
   typedef uint16_t mmsize_t;
#  define MMSIZE_MAX 0xffff
#else
   typedef size_t mmsize_t;
#  define MMSIZE_MAX SIZE_MAX
#endif

/* This describes an allocated chunk.  An allocated chunk is
 * distinguished from a free chunk by bit 15/31 of the 'preceding' chunk
 * size.  If set, then this is an allocated chunk.
 */

struct mm_allocnode_s
{
  mmsize_t size;           /* Size of this chunk */
  mmsize_t preceding;      /* Size of the preceding chunk */
};

/* What is the size of the allocnode? */

#ifdef CONFIG_MM_SMALL
# define SIZEOF_MM_ALLOCNODE   4
#else
# define SIZEOF_MM_ALLOCNODE   8
#endif

#define CHECK_ALLOCNODE_SIZE \
  DEBUGASSERT(sizeof(struct mm_allocnode_s) == SIZEOF_MM_ALLOCNODE)

/* This describes a free chunk */

struct mm_freenode_s
{
  mmsize_t size;                   /* Size of this chunk */
  mmsize_t preceding;              /* Size of the preceding chunk */
  FAR struct mm_freenode_s *flink; /* Supports a doubly linked list */
  FAR struct mm_freenode_s *blink;
};

/* What is the size of the freenode? */

#ifdef CONFIG_MM_SMALL
#  ifdef CONFIG_SMALL_MEMORY
#     define SIZEOF_MM_FREENODE 8
#  else
#     define SIZEOF_MM_FREENODE 12
#  endif
#else
# define SIZEOF_MM_FREENODE     16
#endif

#define CHECK_FREENODE_SIZE \
  DEBUGASSERT(sizeof(struct mm_freenode_s) == SIZEOF_MM_FREENODE)

/* This describes one heap (possibly with multiple regions) */

struct mm_heap_s
{
  /* Mutually exclusive access to this data set is enforced with
   * the following un-named semaphore.
   */

  sem_t mm_semaphore;
  pid_t mm_holder;
  int   mm_counts_held;

  /* This is the size of the heap provided to mm */

  size_t  mm_heapsize;

  /* This is the first and last nodes of the heap */

  FAR struct mm_allocnode_s *mm_heapstart[CONFIG_MM_REGIONS];
  FAR struct mm_allocnode_s *mm_heapend[CONFIG_MM_REGIONS];

#if CONFIG_MM_REGIONS > 1
  int mm_nregions;
#endif

  /* All free nodes are maintained in a doubly linked list.  This
   * array provides some hooks into the list at various points to
   * speed searches for free nodes.
   */

  struct mm_freenode_s mm_nodelist[MM_NNODES];
};

/****************************************************************************
 * Public Data
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

#ifdef CONFIG_MM_USER_HEAP
/* This is the user heap */

EXTERN struct mm_heap_s g_mmheap;
#endif

#ifdef CONFIG_MM_KERNEL_HEAP
/* This is the kernel heap */

EXTERN struct mm_heap_s g_kmmheap;
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Functions contained in mm_initialize.c ***********************************/

void mm_initialize(FAR struct mm_heap_s *heap, FAR void *heap_start,
                   size_t heap_size);
void mm_addregion(FAR struct mm_heap_s *heap, FAR void *heapstart,
                  size_t heapsize);

/* Functions contained in umm_initialize.c **********************************/

#ifdef CONFIG_MM_USER_HEAP
void umm_initialize(FAR void *heap_start, size_t heap_size);
#endif

/* Functions contained in kmm_initialize.c **********************************/

#ifdef CONFIG_MM_KERNEL_HEAP
void kmm_initialize(FAR void *heap_start, size_t heap_size);
#endif

/* Functions contained in umm_addregion.c ***********************************/

#ifdef CONFIG_MM_USER_HEAP
void umm_addregion(FAR void *heapstart, size_t heapsize);
#endif

/* Functions contained in kmm_addregion.c ***********************************/

#ifdef CONFIG_MM_USER_HEAP
void kmm_addregion(FAR void *heapstart, size_t heapsize);
#endif

/* Functions contained in mm_sem.c ******************************************/

void mm_seminitialize(FAR struct mm_heap_s *heap);
void mm_takesemaphore(FAR struct mm_heap_s *heap);
int  mm_trysemaphore(FAR struct mm_heap_s *heap);
void mm_givesemaphore(FAR struct mm_heap_s *heap);

/* Functions contained in umm_sem.c ****************************************/

#ifdef CONFIG_MM_USER_HEAP
int  umm_trysemaphore(void);
void umm_givesemaphore(void);
#endif

/* Functions contained in kmm_sem.c ****************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
int  kmm_trysemaphore(void);
void kmm_givesemaphore(void);
#endif

/* Functions contained in mm_malloc.c ***************************************/

FAR void *mm_malloc(FAR struct mm_heap_s *heap, size_t size);

/* Functions contained in kmm_malloc.c **************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
FAR void *kmm_malloc(size_t size);
#endif

/* Functions contained in mm_free.c *****************************************/

void mm_free(FAR struct mm_heap_s *heap, FAR void *mem);

/* Functions contained in kmm_free.c ****************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
void kmm_free(FAR void *mem);
#endif

/* Functions contained in mm_realloc.c **************************************/

FAR void *mm_realloc(FAR struct mm_heap_s *heap, FAR void *oldmem,
                     size_t size);

/* Functions contained in kmm_realloc.c *************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
FAR void *kmm_realloc(FAR void *oldmem, size_t newsize);
#endif

/* Functions contained in mm_calloc.c ***************************************/

FAR void *mm_calloc(FAR struct mm_heap_s *heap, size_t n, size_t elem_size);

/* Functions contained in kmm_calloc.c **************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
FAR void *kmm_calloc(size_t n, size_t elem_size);
#endif

/* Functions contained in mm_zalloc.c ***************************************/

FAR void *mm_zalloc(FAR struct mm_heap_s *heap, size_t size);

/* Functions contained in kmm_zalloc.c **************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
FAR void *kmm_zalloc(size_t size);
#endif

/* Functions contained in mm_memalign.c *************************************/

FAR void *mm_memalign(FAR struct mm_heap_s *heap, size_t alignment,
                      size_t size);

/* Functions contained in kmm_memalign.c ************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
FAR void *kmm_memalign(size_t alignment, size_t size);
#endif

/* Functions contained in kmm_heapmember.c **********************************/

#if defined(CONFIG_MM_KERNEL_HEAP) && defined(CONFIG_DEBUG)
bool kmm_heapmember(FAR void *mem);
#endif

/* Functions contained in mm_brkaddr.c **************************************/

FAR void *mm_brkaddr(FAR struct mm_heap_s *heap, int region);

/* Functions contained in umm_brkaddr.c *************************************/

#ifdef CONFIG_MM_USER_HEAP
FAR void *umm_brkaddr(int region);
#endif

/* Functions contained in kmm_brkaddr.c *************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
FAR void *umm_brkaddr(int region);
#endif

/* Functions contained in mm_extend.c ***************************************/

void mm_extend(FAR struct mm_heap_s *heap, FAR void *mem, size_t size,
               int region);

/* Functions contained in umm_extend.c **************************************/

#ifdef CONFIG_MM_USER_HEAP
void umm_extend(FAR void *mem, size_t size, int region);
#endif

/* Functions contained in kmm_extend.c **************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
void kmm_extend(FAR void *mem, size_t size, int region);
#endif

/* Functions contained in mm_mallinfo.c *************************************/

struct mallinfo; /* Forward reference */
int mm_mallinfo(FAR struct mm_heap_s *heap, FAR struct mallinfo *info);

/* Functions contained in kmm_mallinfo.c ************************************/

#ifdef CONFIG_MM_KERNEL_HEAP
#ifdef CONFIG_CAN_PASS_STRUCTS
struct mallinfo kmm_mallinfo(void);
#else
int kmm_mallinfo(struct mallinfo *info);
#endif
#endif /* CONFIG_CAN_PASS_STRUCTS */

/* Functions contained in mm_shrinkchunk.c **********************************/

void mm_shrinkchunk(FAR struct mm_heap_s *heap,
                    FAR struct mm_allocnode_s *node, size_t size);

/* Functions contained in mm_addfreechunk.c *********************************/

void mm_addfreechunk(FAR struct mm_heap_s *heap,
                     FAR struct mm_freenode_s *node);

/* Functions contained in mm_size2ndx.c.c ***********************************/

int mm_size2ndx(size_t size);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_NUTTX_MM_H */

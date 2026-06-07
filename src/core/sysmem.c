/**
 * @file sysmem.c
 * @ingroup core
 * @brief Heap memory management for newlib
 *
 * Implements _sbrk() system call for dynamic memory allocation (malloc/free).
 * Uses linker-defined heap region between .bss and stack.
 */

#include <errno.h>
#include <stdint.h>

/**
 * Pointer to the current high watermark of the heap usage
 */
static uint8_t *__sbrk_heap_end = NULL;

/**
 * @brief _sbrk() allocates memory to the newlib heap and is used by malloc
 *        and others from the C library
 *
 * @verbatim
 * ##################################################################################
 * #  .data  #  .bss  #       heap (HEAP_SIZE)            #      MSP stack          #
 * #         #        #                                   #    (STACK_SIZE)         #
 * ##################################################################################
 * ^-- RAM start      ^-- __heap_start__   __heap_end__ --^     __StackTop --^
 *                         (_end)
 * @endverbatim
 *
 * This implementation starts allocating at the '__heap_start__' linker symbol
 * and cannot grow beyond '__heap_end__' which is defined by HEAP_SIZE.
 *
 * @param incr Memory size
 * @return Pointer to allocated memory
 */
void *_sbrk(ptrdiff_t incr)
{
  extern uint8_t __heap_start__; /* Symbol defined in the linker script */
  extern uint8_t __heap_end__;   /* Symbol defined in the linker script */
  const uint8_t *max_heap = (uint8_t *)&__heap_end__;
  uint8_t *prev_heap_end;

  /* Initialize heap end at first call */
  if (NULL == __sbrk_heap_end)
  {
    __sbrk_heap_end = &__heap_start__;
  }

  /* Protect heap from exceeding reserved HEAP_SIZE */
  if (__sbrk_heap_end + incr > max_heap)
  {
    errno = ENOMEM;
    return (void *)-1;
  }

  prev_heap_end = __sbrk_heap_end;
  __sbrk_heap_end += incr;

  return (void *)prev_heap_end;
}
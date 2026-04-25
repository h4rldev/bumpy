#ifndef BUMPY_H
#define BUMPY_H

/*
 * @brief The bumpy header.
 *
 * @details This header contains the heap arena related functions and structs.
 */

#include <stdint.h>

//
//
//

// Type aliases for the fixed sized types used.
typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int32_t i32;

// Convenience macros for converting between types.
#define KiB(num) ((u64)num << 10)
#define MiB(num) ((u64)num << 20)
#define GiB(num) ((u64)num << 30)

/*
 * @brief The arena struct.
 *
 * @details This struct contains the arena's reserved and committed heap, and
 * the current positions of the whole arena and committed heap.
 *
 * @param reserved The amount of reserved heap.
 * @param committed The amount of committed heap.
 * @param position The current position in the arena.
 * @param commit_position The current position in the committed heap.
 */
typedef struct {
  u64 reserved;
  u64 committed;
  u64 position;
  u64 commit_position;
} arena_t;

/*
 * @brief The temp arena struct.
 *
 * @details This struct contains the arena and the start position of the temp
 * arena (which is the current position of the perm arena when the temp arena
 * was initialized).
 */
typedef struct {
  arena_t *arena;
  u64 start_position;
} temp_arena_t;

/*
 * @brief Initializes a new arena with the given reserved_size and
 * committed_size.
 *
 * @details This function initializes a new arena with the given reserve_size
 * and commit_size, by reserving reserve_size bytes and committing
 * commit_size bytes out of reserve_size.
 *
 * @param reserve_size The amount of heap to reserve for the arena.
 * @param commit_size The amount of heap to commit for the arena.
 *
 * @return The initialized arena.
 */
arena_t *arena_new(u64 reserve_size, u64 commit_size);

/*
 * @brief Frees the arena.
 *
 * @details This function frees the arena, by freeing the reserved and committed
 * heap, this function should be ran at the end of an arena's usage, but its not
 * really required since the kernel frees the memory map on process exit.
 *
 * @param arena The arena to free.
 */
void arena_free(arena_t *arena);

/*
 * @brief Allocates a chunk of memory from the arena.
 *
 * @note This function is not meant to be used directly, use `arena_alloc`
 * instead.
 *
 * @details This function allocates a chunk of memory from the arena, by
 * returning a chunk of the arena's committed heap as a castable pointer.
 *
 * @param arena The arena to allocate from.
 * @param size The size of the chunk to allocate.
 *
 * @return The allocated chunk of memory.
 */
void *__arena_alloc(arena_t *arena, u64 size);

/*
 * @brief Allocates a chunk of memory from the arena.
 *
 * @details See `__arena_alloc`
 *
 * @param arena The arena to allocate from.
 * @param type The type of the chunk to allocate.
 * @param size The size of the chunk to allocate.
 *
 * @return The allocated chunk of memory.
 */
#define arena_alloc(arena, type, size) __arena_alloc(arena, sizeof(type) * size)

/*
 * @brief Deallocates a chunk of memory from the arena.
 *
 * @note This function is not meant to be used directly, use `arena_dealloc`
 * instead.
 *
 * @details This function deallocates a chunk of memory from the arena, by just
 * moving the arena position.
 *
 * @param arena The arena to deallocate from.
 * @param size The size of the chunk to deallocate.
 */
void __arena_dealloc(arena_t *arena, u64 size);

/*
 * @brief Deallocates a chunk of memory from the arena.
 *
 * @details See `__arena_dealloc`
 *
 * @param arena The arena to deallocate from.
 * @param type The type of the chunk to deallocate.
 * @param size The size of the chunk to deallocate.
 */
#define arena_dealloc(arena, type, size)                                       \
  __arena_dealloc(arena, sizeof(type) * size)

/*
 * @brief Deallocate to a specific position in the arena.
 *
 * @details This function deallocates to a specific position in the arena, by
 * setting the arena position to the given position.
 *
 * @param arena The arena to deallocate to.
 * @param position The position to deallocate to.
 */
void arena_pop_to(arena_t *arena, u64 position);

/*
 * @brief Clears an arena of all allocations.
 *
 * @details This function clears an arena of all allocations, by setting the
 * position to the initial position.
 *
 * @param arena The arena to clear.
 */
void arena_clear(arena_t *arena);

/*
 * @brief Initializes a new temp arena with the given arena.
 *
 * @details This function initializes a new temp arena with the given arena, by
 * populating a struct with the arena and the position the arena is currently
 * at.
 *
 * @param arena The arena to use for the temp arena.
 *
 * @return The initialized temp arena.
 */
temp_arena_t temp_arena_new(arena_t *arena);

/*
 * @brief Frees the temp arena.
 *
 * @details This function frees the temp arena, by setting the arena position to
 * the initial position specified in the temp arena.
 *
 * @param temp_arena The temp arena to free.
 */
void temp_arena_free(temp_arena_t temp_arena);

#endif // !BUMPY_H

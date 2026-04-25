#include <assert.h>
#include <string.h>

#include <bumpy.h>

#define ALIGN_POW2(num, pow)                                                   \
  (((u64)(num) + ((u64)(pow) - 1)) & (~((u64)(pow) - 1)))

#define MIN(num1, num2) ((num1 < num2) ? num1 : num2)
#define MAX(num1, num2) ((num1 > num2) ? num1 : num2)

#define ARENA_BASE_POS (sizeof(arena_t))
#define ARENA_ALIGNMENT (sizeof(void *))

#if defined(__linux__)
#include <stdbool.h>
#include <sys/mman.h>
#include <unistd.h>

static u32 get_page_size(void) { return (u32)sysconf(_SC_PAGESIZE); }

static void *mem_reserve(u64 size) {
  void *ret = mmap(NULL, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  assert(ret != MAP_FAILED && "Failed to reserve memory, mmap failed.");
  return ret;
}

static bool mem_release(void *ptr, u64 size) {
  return (i32)munmap(ptr, size) == 0;
}

static bool mem_commit(void *ptr, u64 size) {
  return (i32)mprotect(ptr, size, PROT_READ | PROT_WRITE) == 0;
}

static bool mem_decommit(void *ptr, u64 size) {
  i32 ret = mprotect(ptr, size, PROT_NONE);
  assert(ret == 0 && "Failed to decommit memory, mprotect failed.");
  return (i32)madvise(ptr, size, MADV_DONTNEED) == 0;
}
#elif defined(_WIN32)
#include <windows.h>

static u32 get_page_size(void) {
  SYSTEM_INFO sys_info;
  GetSystemInfo(&sys_info);
  return (u32)sys_info.dwPageSize;
}

static void *mem_reserve(u64 size) {
  void *ret = VirtualAlloc(NULL, size, MEM_RESERVE, PAGE_NOACCESS);
  assert(ret != NULL && "Failed to reserve memory, VirtualAlloc failed.");
  return ret;
}

static bool mem_release(void *ptr, u64 size) {
  (void)size;
  return (i32)VirtualFree(ptr, 0, MEM_RELEASE) != 0;
}

static bool mem_commit(void *ptr, u64 size) {
  void *ret = VirtualAlloc(ptr, (SIZE_T)size, MEM_COMMIT, PAGE_READWRITE);
  return ret != NULL;
}

static bool mem_decommit(void *ptr, u64 size) {
  return (i32)VirtualFree(ptr, (SIZE_T)size, MEM_DECOMMIT) != 0;
}
#else
#error "Unsupported OS for now, sorry."
#endif

arena_t *arena_new(u64 reserve_size, u64 commit_size) {
  u32 page_size = get_page_size();

  assert(page_size > 0 && "Failed to get page size.");
  assert(reserve_size > 0 && "Reserve size must be greater than 0.");
  assert(commit_size > 0 && "Commit size must be greater than 0.");
  assert(reserve_size > commit_size &&
         "Reserve size must be greater than commit size.");

  reserve_size = ALIGN_POW2(reserve_size, page_size);
  commit_size = ALIGN_POW2(commit_size, page_size);

  arena_t *arena = mem_reserve(ARENA_BASE_POS + reserve_size);
  assert(mem_commit(arena, commit_size) && "Failed to commit memory.");

  arena->reserved = reserve_size;
  arena->committed = commit_size;
  arena->position = ARENA_BASE_POS;
  arena->commit_position = commit_size;

  return arena;
}

void arena_free(arena_t *arena) {
  assert(mem_release(arena, arena->reserved) && "Failed to release memory.");
}

void *__arena_alloc(arena_t *arena, u64 size) {
  u64 pos_aligned = ALIGN_POW2(arena->position, ARENA_ALIGNMENT);
  u64 new_pos = pos_aligned + size;

  assert(new_pos < arena->reserved && "Arena full.");

  if (new_pos > arena->commit_position) {
    u64 new_commit_pos = new_pos;
    new_commit_pos += arena->committed - 1;
    new_commit_pos -= new_commit_pos % arena->committed;
    new_commit_pos = MIN(new_commit_pos, arena->reserved);

    u8 *mem = (u8 *)arena + arena->commit_position;
    u64 commit_size = new_commit_pos - arena->commit_position;

    assert(mem_commit(mem, commit_size) && "Failed to commit memory.");

    arena->commit_position = new_commit_pos;
  }

  arena->position = new_pos;
  u8 *out = (u8 *)arena + pos_aligned;
  memset(out, 0, size);

  return (void *)out;
}

void __arena_dealloc(arena_t *arena, u64 size) {
  size = MIN(size, arena->position - ARENA_BASE_POS);
  arena->position -= size;
}

void arena_pop_to(arena_t *arena, u64 position) {
  u64 size = position < arena->position ? arena->position - position : 0;
  __arena_dealloc(arena, size);
}

void arena_clear(arena_t *arena) {
  arena_pop_to(arena, ARENA_BASE_POS);

  assert(mem_decommit(arena, arena->committed) && "Failed to decommit memory.");
  assert(mem_commit(arena, arena->committed) && "Failed to recommit memory.");
}

temp_arena_t temp_arena_new(arena_t *arena) {
  return (temp_arena_t){arena, arena->position};
}

void temp_arena_free(temp_arena_t temp_arena) {
  arena_pop_to(temp_arena.arena, temp_arena.start_position);
}

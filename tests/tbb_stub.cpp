// Single-threaded tests: stan-math's rev core registers a TBB thread observer
// so each worker gets its own AD tape. We run one thread and build no TBB, so
// provide the observer and current-arena symbols it needs.
#if defined(__APPLE__)
extern "C" void stanli_observe_stub(void*, bool) {}
extern "C" int stanli_current_slot_stub() { return 0; }
asm(".globl __ZN3tbb8internal26task_scheduler_observer_v37observeEb\n"
    "__ZN3tbb8internal26task_scheduler_observer_v37observeEb = "
    "_stanli_observe_stub\n"
    ".globl "
    "__ZN3tbb10interface78internal15task_arena_base21internal_current_slotEv\n"
    "__ZN3tbb10interface78internal15task_arena_base21internal_current_slotEv = "
    "_stanli_current_slot_stub\n");
#else
namespace tbb {
namespace internal {
class task_scheduler_observer_v3 {
 public:
  void observe(bool);
};
void task_scheduler_observer_v3::observe(bool) {}
}  // namespace internal
inline namespace interface7 {
namespace internal {
class task_arena_base {
 public:
  static int internal_current_slot();
};
int task_arena_base::internal_current_slot() { return 0; }
}  // namespace internal
}  // namespace interface7
}  // namespace tbb
#endif


#define TEN_MILLIONS 10000000
#define ONE_BILLION  1000000000

void fail(const char *reason);
void setup_sched_parameters(pthread_attr_t *attr, int prio, int cpu);
void init_main_thread();
void print_header(char *name);
void print_result(int loop, int samples, int32_t min, int32_t max, int64_t sum);
void sync_process_step(void *first);

#if 0
//atomic
typedef struct {
    volatile int counter;
} atomic_t;

#define atomic_read(v) ((v)->counter)
#define atomic_set(v, i) (((v)->counter) = (i))

static inline void atomic_add(int i, atomic_t *v)
{
    (void)__sync_add_and_fetch(&v->counter, i);
}

static inline void atomic_sub(int i, atomic_t *v)
{
    (void)__sync_sub_and_fetch(&v->counter, i);
}
#endif

static inline long long diff_ts(struct timespec *left, struct timespec *right)
{
    return (long long)(left->tv_sec - right->tv_sec) * ONE_BILLION
        + left->tv_nsec - right->tv_nsec;
}

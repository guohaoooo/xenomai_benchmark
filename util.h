
#define TEN_MILLIONS 10000000
#define ONE_BILLION  1000000000

void fail(const char *reason);
void setup_sched_parameters(pthread_attr_t *attr, int prio, int cpu);
void init_main_thread();

static inline long long diff_ts(struct timespec *left, struct timespec *right)
{
    return (long long)(left->tv_sec - right->tv_sec) * ONE_BILLION
        + left->tv_nsec - right->tv_nsec;
}

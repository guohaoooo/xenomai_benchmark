
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/timerfd.h>


#define ONE_BILLION  1000000000
#define TEN_MILLIONS 10000000
#define SAMPLES_NUM  100000

static inline long long diff_ts(struct timespec *left, struct timespec *right)
{
    return (long long)(left->tv_sec - right->tv_sec) * ONE_BILLION
        + left->tv_nsec - right->tv_nsec;
}

int main(int argc, char *const *argv)
{
    int err;
    int policy = sched_getscheduler(0);
    struct sched_param param, old_param;

#ifdef CONFIG_XENO_COBALT
    err = pthread_setmode_np(0, PTHREAD_WARNSW, NULL);
    if (err)
        error(1, err, "pthread_setmode_np()");
#endif

    if ((policy != SCHED_FIFO) && (policy != SCHED_RR)) {
        err = sched_getparam(0, &old_param);
        if (err)
            error(1, err, "sched_getparam()");
        param = old_param;
        param.sched_priority = 1;
        err = sched_setscheduler(0, SCHED_FIFO, &param);
        if (err)
            error(1, err, "sched_setscheduler()");
    }

    printf("== Real Time Test \n"
           "== Test name: getpid \n"
           "== All results in microseconds\n");

    for (;;) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;
        pid_t pid;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);
            pid = getpid();
            clock_gettime(CLOCK_MONOTONIC, &end);
    
            dt = (int32_t)diff_ts(&end, &start);

            if (dt > max)
                max = dt;
            
            if (dt < min)
                min = dt;

            sum += dt;
        }

        printf("Result|samples:%11d|min:%11.3f|avg:%11.3f|max:%11.3f\n",
                        samples,
                        (double)min / 1000,
                        (double)sum / (samples * 1000),
                        (double)max / 1000);

        sleep(1);
    }

    return 0;
}

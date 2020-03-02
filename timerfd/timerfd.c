
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
    int err, policy;
    struct sched_param param, old_param;

    err = pthread_getschedparam(pthread_self(), &policy, &old_param);
    if (err)
        error(1, err, "pthread_sched_getparam()");

    if ((policy != SCHED_FIFO) && (policy != SCHED_RR)) {
        param = old_param;
        param.sched_priority = 99;
        err = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
        if (err)
            error(1, err, "setscheduler()");
    }


#ifdef CONFIG_XENO_COBALT
    err = pthread_setmode_np(0, PTHREAD_WARNSW, NULL);
    if (err)
        error(1, err, "pthread_setmode_np()");
#endif

    printf("== Real Time Test \n"
           "== Test name: timerfd_create \n"
           "== All results in microseconds\n");

    for (;;) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, tfd, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);
            tfd = timerfd_create(CLOCK_MONOTONIC, 0);
            if (tfd == -1)
                error(1, errno, "timerfd_create()");
            clock_gettime(CLOCK_MONOTONIC, &end);

            if (tfd != -1)
                close(tfd);
    
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

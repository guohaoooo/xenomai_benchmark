#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#ifndef __XENO__
#include <stdint.h>
#endif
#include "../util.h"

#define SAMPLES_NUM  100000
#define SAMPLES_LOOP 100

#ifndef PTHREAD_STACK_MIN
#define PTHREAD_STACK_MIN 16384
#endif

char test_name[32] = "pthread_create_join_ss";

char userstack[PTHREAD_STACK_MIN];

void *emptyfunction(void *arg) {return (arg);}

int main(int argc, char *const *argv)
{
    int err, cpu = 0, i, loop = SAMPLES_LOOP;

    init_main_thread();

    print_header(test_name);

    pthread_t task;
    pthread_attr_t tattr;

    //set task sched attr
    setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO), cpu);

    err = pthread_attr_setstack(&tattr, userstack, PTHREAD_STACK_MIN);
    if (err)
         fail("pthread_attr_setstack()");

    for (i = 0; i < loop; i++) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);

            err = pthread_create(&task, &tattr, emptyfunction, NULL);
            if (err)
                fail("pthread_create()");

            pthread_join(task, NULL);

            clock_gettime(CLOCK_MONOTONIC, &end);

            dt = (int32_t)diff_ts(&end, &start);

            if (dt > max)
                max = dt;

            if (dt < min)
                min = dt;

            sum += dt;
        }

        print_result(i, samples, min, max, sum);
    }

    return 0;
}

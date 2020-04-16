#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#ifndef __XENO__
#include <stdint.h>
#endif
#include "../util.h"

#define SAMPLES_NUM  100000
#define SAMPLES_LOOP 100

char test_name[32] = "sched_yield_inter_thread";

void *function(void *arg)
{
    int i = 0;
    int loop = SAMPLES_LOOP;

    sync_process_step(arg);

    for (i = 0; i < loop; i++) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);

            sched_yield();

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

    return (arg);
}

int main(int argc, char *const *argv)
{
    int err, cpu = 0, first;
    pthread_t task_1;
    pthread_t task_2;
    pthread_attr_t tattr;

    init_main_thread();

    print_header(test_name);

    //set task sched attr
    setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO), cpu);

    err = pthread_create(&task_1, &tattr, function, &first);
    if (err)
        fail("pthread_create task_1()");

    err = pthread_create(&task_2, &tattr, function, NULL);
    if (err)
        fail("pthread_create task_2()");

    pthread_attr_destroy(&tattr);

    pthread_join(task_1, NULL);

    pthread_join(task_2, NULL);

    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#ifndef __XENO__
#include <stdint.h>
#endif
#include "../util.h"

#define SAMPLES_NUM  10000
#define SAMPLES_LOOP 100

char test_name[32] = "in_kernel_mutex";

void *function(void *arg)
{
//    int dog = 0;
    int err, i;
    int loop = SAMPLES_LOOP;
    pthread_mutex_t mutex;
    pthread_mutexattr_t mutexattr;

    err = pthread_mutexattr_init(&mutexattr);
    if(err)
        fail("pthread_mutexattr_init()");

    err = pthread_mutexattr_setprotocol(&mutexattr, PTHREAD_PRIO_PROTECT);
    if(err)
        fail("pthread_mutexattr_setprotocol()");

    err = pthread_mutexattr_setprioceiling(&mutexattr, 99);
    if(err)
        fail("pthread_mutexattr_setprioceiling()");

    err = pthread_mutex_init(&mutex, &mutexattr);
    if(err)
        fail("pthread_mutex_init()");


    for (i = 0; i < loop; i++) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);
            pthread_mutex_lock(&mutex);
            pthread_mutex_unlock(&mutex);
            clock_gettime(CLOCK_MONOTONIC, &end);

            dt = (int32_t)diff_ts(&end, &start);

            if (dt > max)
                max = dt;

            if (dt < min)
                min = dt;

            sum += dt;
        }

        print_result(i, samples, min, max, sum);

#if 0
        dog++;
        if (dog%10 == 0)
            sleep(1);
#endif
    }

    return (arg);
}

int main(int argc, char *const *argv)
{
    int err, cpu = 0;
    pthread_t task;
    pthread_attr_t tattr;

    init_main_thread();

    print_header(test_name);

    //set task sched attr
    setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO), cpu);

    err = pthread_create(&task, &tattr, function, NULL);
    if (err)
        fail("pthread_create()");

    pthread_attr_destroy(&tattr);

    pthread_join(task, NULL);

    return 0;
}

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#ifndef __XENO__
#include <stdint.h>
#endif
#include "../util.h"

#define SAMPLES_NUM  10000
#define SAMPLES_LOOP 100

char test_name[32] = "contested_mutex";

pthread_mutex_t mutex1, mutex2, mutex3, mutex4;

void *function_1(void *arg)
{
//    int dog = 0;
    int err, i;
    int loop = SAMPLES_LOOP;

    err = pthread_mutex_init(&mutex1, NULL);
    if(err)
        fail("pthread_mutex_init(1)");

    err = pthread_mutex_init(&mutex2, NULL);
    if(err)
        fail("pthread_mutex_init(2)");

    err = pthread_mutex_init(&mutex3, NULL);
    if(err)
        fail("pthread_mutex_init(3)");

    err = pthread_mutex_init(&mutex4, NULL);
    if(err)
        fail("pthread_mutex_init(4)");

    err = pthread_mutex_lock(&mutex4);
    if(err)
        fail("pthread_mutex_lock(4)");

    sync_process_step(arg);

    for (i = 0; i < loop; i++) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM, err;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);
            err = pthread_mutex_lock(&mutex1);
            if (err)
                fail("pthread_mutex_lock");

            err = pthread_mutex_lock(&mutex3);
            if (err)
                fail("pthread_mutex_lock");

            err = pthread_mutex_unlock(&mutex1);
            if (err)
                fail("pthread_mutex_unlock");

            err = pthread_mutex_unlock(&mutex4);
            if (err)
                fail("pthread_mutex_unlock");

            err = pthread_mutex_lock(&mutex2);
            if (err)
                fail("pthread_mutex_lock");

            err = pthread_mutex_lock(&mutex4);
            if (err)
                fail("pthread_mutex_lock");

            err = pthread_mutex_unlock(&mutex2);
            if (err)
                fail("pthread_mutex_unlock");

            err = pthread_mutex_unlock(&mutex3);
            if (err)
                fail("pthread_mutex_unlock");

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

void *function_2(void *arg)
{
//    int dog = 0;
    int err, i;
    int loop = SAMPLES_LOOP;

    err = pthread_mutex_lock(&mutex1);
    if(err)
        fail("pthread_mutex_lock(1)");

    sync_process_step(arg);

    for (i = 0; i < loop; i++) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);
            err = pthread_mutex_lock(&mutex3);
            if (err)
                fail("pthread_mutex_lock");

            err = pthread_mutex_lock(&mutex2);
            if (err)
                fail("pthread_mutex_lock");

            err = pthread_mutex_unlock(&mutex3);
            if (err)
                fail("pthread_mutex_unlock");

            err = pthread_mutex_unlock(&mutex1);
            if (err)
                fail("pthread_mutex_unlock");
            err = pthread_mutex_lock(&mutex4);
            if (err)
                fail("pthread_mutex_lock");

            err = pthread_mutex_lock(&mutex1);
            if (err)
                fail("pthread_mutex_lock");

            err = pthread_mutex_unlock(&mutex4);
            if (err)
                fail("pthread_mutex_unlock");

            err = pthread_mutex_unlock(&mutex2);
            if (err)
                fail("pthread_mutex_unlock");

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
    int err, cpu = 0, first;
    pthread_t task1;
    pthread_t task2;
    pthread_attr_t tattr;

    init_main_thread();

    print_header(test_name);

    //set task sched attr
    setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO), cpu);

    err = pthread_create(&task1, &tattr, function_1, &first);
    if (err)
        fail("pthread_create()");

    err = pthread_create(&task2, &tattr, function_2, NULL);
    if (err)
        fail("pthread_create()");

    pthread_attr_destroy(&tattr);

    pthread_join(task1, NULL);
    pthread_join(task2, NULL);

    return 0;
}

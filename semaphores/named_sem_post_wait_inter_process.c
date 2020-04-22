#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifndef __XENO__
#include <stdint.h>
#endif
#include "../util.h"

#define SAMPLES_NUM   10000
#define SAMPLES_LOOP  100
#define SEM_NAME "/named_sem"

char test_name[64] = "named sem_post_wait_inter_process";

void *function_1(void *arg)
{
//    int dog = 0;
    int err, i, loop = SAMPLES_LOOP;
    sem_t *sem;

    sync_process_step(arg);

    sem_unlink(SEM_NAME);
    sem = sem_open(SEM_NAME, O_ACCMODE|O_CREAT, S_IRUSR|S_IWUSR, 0);
    if(sem == SEM_FAILED) {
        fail("sem open failed");
    }

    for (i = 0; i < loop; i++) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);
            err = sem_wait(sem);
            if (err)
                fail("sem_wait()");
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
    int err, i, loop = SAMPLES_LOOP;
    sem_t *sem;

    sync_process_step(arg);

    sem = sem_open(SEM_NAME, O_ACCMODE, 0, 0);
    if (sem == SEM_FAILED) {
        fail("sem open failed");
    }

    for (i = 0; i < loop; i++) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);
            err = sem_post(sem);
            if (err)
                fail("sem_post()");
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

    printf("== Real Time Test \n"
           "== Test name: %s \n"
           "== All results in microseconds\n",
           test_name);


    if (argc == 1) {
        //set task sched attr
        setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO), cpu);

        err = pthread_create(&task1, &tattr, function_1, NULL);
        if (err)
            fail("pthread_create()");

        pthread_join(task1, NULL);
    } else {

        setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO) - 1, cpu);
        err = pthread_create(&task2, &tattr, function_2, &first);
        if (err)
            fail("pthread_create()");

        pthread_join(task2, NULL);
    }

    pthread_attr_destroy(&tattr);

    sem_unlink(SEM_NAME);

    return 0;
}

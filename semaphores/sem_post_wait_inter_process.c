#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../util.h"
#ifndef __XENO__
#include <stdint.h>
#endif

#define SAMPLES_NUM  100000

char test_name[32] = "sem_post_wait_inter_process";

void *function_1(void *arg)
{
    int dog = 0, err, fd;
    sem_t *sem;

    fd = shm_open("/sem", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    ftruncate(fd, sizeof(sem_t));
    sem = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    sem_init(sem, !0, 0);

    for (;;) {

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

        printf("Result| task1: samples:%11d|min:%11.3f|avg:%11.3f|max:%11.3f\n",
                        samples,
                        (double)min / 1000,
                        (double)sum / (samples * 1000),
                        (double)max / 1000);
        dog++;
        if (dog%10 == 0)
            sleep(1);
    }

    return (arg);
}

void *function_2(void *arg)
{
    int dog = 0, err, fd;
    sem_t *sem;

    fd = shm_open("/sem", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);
    ftruncate(fd, sizeof(sem_t));
    sem = mmap(NULL, sizeof(sem_t), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    sem_init(sem, !0, 0);

    for (;;) {

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

        printf("Result| task2: samples:%11d|min:%11.3f|avg:%11.3f|max:%11.3f\n",
                        samples,
                        (double)min / 1000,
                        (double)sum / (samples * 1000),
                        (double)max / 1000);
        dog++;
        if (dog%10 == 0)
            sleep(1);
    }

    return (arg);
}

int main(int argc, char *const *argv)
{
    int err, cpu = 0;
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
        err = pthread_create(&task2, &tattr, function_2, NULL);
        if (err)
            fail("pthread_create()");

        pthread_join(task2, NULL);
    }

    pthread_attr_destroy(&tattr);

    return 0;
}

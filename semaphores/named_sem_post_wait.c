#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include "../util.h"
#ifndef __XENO__
#include <stdint.h>
#endif

#define SAMPLES_NUM  1000000
#define SEM_NAME "/sem"

char test_name[32] = "named_sem_post_wait";

void *function(void *arg)
{
    int dog = 0, err;
    sem_t *sem;

    sem_unlink(SEM_NAME);
    sem = sem_open(SEM_NAME, O_ACCMODE|O_CREAT, S_IRUSR|S_IWUSR, 0);
    if(sem == SEM_FAILED) {
        fail("sem_open()");
    }

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

        printf("Result|samples:%11d|min:%11.3f|avg:%11.3f|max:%11.3f\n",
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
    pthread_t task;
    pthread_attr_t tattr;

    init_main_thread();

    printf("== Real Time Test \n"
           "== Test name: %s \n"
           "== All results in microseconds\n",
           test_name);

    //set task sched attr
    setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO), cpu);

    err = pthread_create(&task, &tattr, function, NULL);
    if (err)
        fail("pthread_create()");

    pthread_attr_destroy(&tattr);

    pthread_join(task, NULL);

    return 0;
}

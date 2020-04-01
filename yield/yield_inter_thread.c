#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#ifndef __XENO__
#include <stdint.h>
#endif
#include "../util.h"

#define SAMPLES_NUM  10000

char test_name[32] = "sched_yield_inter_thread";


void *function(void *arg)
{
    int dog = 0;

    for (;;) {

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


        printf("Result|samples:%11d|min:%11.3f|avg:%11.3f|max:%11.3f\n",
                        samples,
                        (double)min / 1000,
                        (double)sum / (samples * 1000),
                        (double)max / 1000);

        dog++;
        if(dog%10 == 0)
            sleep(1);
    }

    return (arg);
}

int main(int argc, char *const *argv)
{
    int err, cpu = 0;
    pthread_t task_1;
    pthread_t task_2;
    pthread_attr_t tattr;

    init_main_thread();

    printf("== Real Time Test \n"
           "== Test name: %s \n"
           "== All results in microseconds\n",
           test_name);

    //set task sched attr
    setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO), cpu);

    err = pthread_create(&task_1, &tattr, function, NULL);
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

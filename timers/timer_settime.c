#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#ifndef __XENO__
#include <stdint.h>
#endif
#include "../util.h"

#define SAMPLES_NUM  50000
#define SAMPLES_LOOP 100

char test_name[32] = "timer_settime";

void *function(void *arg)
{
//    int dog = 0;
    int err, i, loop = SAMPLES_LOOP;

    for (i = 0; i < loop; i++) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;
        timer_t timerid;
        struct sigevent sev;
        struct itimerspec ts;

        sev.sigev_notify = SIGEV_NONE;
        sev.sigev_value.sival_ptr = &timerid;

        ts.it_interval.tv_sec = 0;
        ts.it_interval.tv_nsec = 0;
        ts.it_value.tv_sec = 60;
        ts.it_value.tv_nsec = 0;

        err = timer_create(CLOCK_MONOTONIC, &sev, &timerid);
        if (err)
            fail("timer_create");

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);

            err = timer_settime(timerid, TIMER_ABSTIME, &ts, NULL);
            if(err)
                fail("timer_settime");

            clock_gettime(CLOCK_MONOTONIC, &end);

            dt = (int32_t)diff_ts(&end, &start);

            if (dt > max)
                max = dt;

            if (dt < min)
                min = dt;

            sum += dt;
        }

        err = timer_delete(timerid);
        if (err)
            fail("timer_create");

        print_result(i, samples, min, max, sum);
#if 0
        dog++;
        if (dog%10 == 0)
        sleep(1);
#endif
    }
    return arg;
}

int main(int argc, char *const *argv)
{
    int err, cpu = 0;
    pthread_t task;
    pthread_attr_t tattr;

    init_main_thread();

    print_header(test_name);

    setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO), cpu);

    err = pthread_create(&task, &tattr, function, NULL);
    if (err)
        fail("pthread_create");

    pthread_attr_destroy(&tattr);
    pthread_join(task, NULL);

    return 0;
}

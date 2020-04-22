#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/syscall.h>
#ifndef __XENO__
#include <stdint.h>
#endif
#include "../util.h"

#define SAMPLES_NUM  10000
#define SAMPLES_LOOP 100

char test_name[32] = "signal_inter_process";

static void emptyhandler(int sig, siginfo_t *si, void *context) {}

void *function(void *arg)
{
    struct sigaction sa __attribute__((unused));
    sigset_t mask;

    sigfillset(&sa.sa_mask);
    sa.sa_sigaction = emptyhandler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGRTMIN+1, &sa, NULL);
    sigfillset(&mask);

//    int dog = 0;
    int i, loop = SAMPLES_LOOP;

//    sync_process_step(arg);

    for (i = 0; i < loop; i++) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);

            sigwaitinfo(&mask, NULL);

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
        if(dog%10 == 0)
            sleep(1);
#endif
    }

    return (arg);
}

void *function_kill(void *arg)
{
//    int dog = 0;
    pid_t pid = *(pid_t *)arg;
    int i, loop = SAMPLES_LOOP;

//    arg = NULL;
//    sync_process_step(arg);

    for (i = 0; i < loop; i++) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);

            kill(pid, SIGRTMIN+1);

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
        if(dog%10 == 0)
            sleep(1);
#endif
    }

    return (arg);

}

int main(int argc, char *const *argv)
{
    int err, cpu = 0, first;
    pthread_attr_t tattr;
    pthread_t task1;
    pthread_t task2;
    pid_t pid;

    init_main_thread();

    print_header(test_name);
    printf("===pid: %d\n", getpid());

    if (argc == 1) {
        setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO), cpu);
        err = pthread_create(&task1, &tattr, function, &first);
        if (err)
            fail("pthread_create()");

        pthread_join(task1, NULL);
    } else {
        pid = (pid_t)atoi(argv[1]);
        printf("argv[1]:%s pid:%d \n",argv[1],pid);
        setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO) - 1, cpu);
        err = pthread_create(&task2, &tattr, function_kill, (void *)&pid);
        if (err)
            fail("pthread_create()");

        pthread_join(task2, NULL);
    }

    pthread_attr_destroy(&tattr);

    return 0;
}

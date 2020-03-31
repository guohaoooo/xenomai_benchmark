#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/syscall.h>
#include "../util.h"

#define SAMPLES_NUM  100000

char test_name[32] = "signal_inter_process";

static void emptyhandler(int sig, siginfo_t *si, void *context) {}

void *function(void *arg)
{
    struct sigaction sa __attribute__((unused));
    sigset_t mask;
    pid_t pid = syscall(__NR_gettid);

    sigfillset(&sa.sa_mask);
    sa.sa_sigaction = emptyhandler;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGUSR1, &sa, NULL);
    sigfillset(&mask);

    int dog = 0;

    for (;;) {

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

        printf("Result|samples:task_1 %11d|min:%11.3f|avg:%11.3f|max:%11.3f\n",
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

void *function_kill(void *arg)
{
    int dog = 0;
    pid_t pid = *(pid_t *)arg;

    for (;;) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);

            kill(pid, SIGUSR1);

            clock_gettime(CLOCK_MONOTONIC, &end);

            dt = (int32_t)diff_ts(&end, &start);

            if (dt > max)
                max = dt;

            if (dt < min)
                min = dt;

            sum += dt;
        }

        printf("Result|samples:task_2 %11d|min:%11.3f|avg:%11.3f|max:%11.3f\n",
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
    pthread_attr_t tattr;
    pthread_t task1;
    pthread_t task2;
    pid_t pid;

    init_main_thread();

    printf("== Real Time Test \n"
           "== Test name: %s pid: %d\n"
           "== All results in microseconds\n",
           test_name, getpid());

    if (argc == 1) {
        setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO), cpu);
        err = pthread_create(&task1, &tattr, function, NULL);
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

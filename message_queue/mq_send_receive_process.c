#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <mqueue.h>
#include "../util.h"
#ifndef __XENO__
#include <stdint.h>
#endif

#define SAMPLES_NUM  10000
#define MQ_NAME "/mq"

char test_name[32] = "mq_send_receive_process";

void *server_function(void *arg)
{
    int dog = 0, err;
    char ret[256] = {0};
    mqd_t mq;
    struct mq_attr mqattr = {0, 1, 256, 0};

    mq_unlink(MQ_NAME);

    mq = mq_open(MQ_NAME, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, &mqattr);
    if(mq == (mqd_t)-1) {
        fail("mq_open");
    }

    for (;;) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);

            err = mq_receive(mq, ret, sizeof(ret), NULL);
            if (errno != EAGAIN && err < 0) {
                fail("mq_receive err");
            }

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

void *client_function(void *arg)
{
    int dog = 0, err;
    char ret[256] = {0};
    mqd_t mq;

    mq = mq_open(MQ_NAME, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, NULL);
    if(mq == (mqd_t)-1) {
        fail("mq_open");
    }

    for (;;) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);

            err = mq_send(mq, ret, 256, 0);
            if (err) {
                fail("mq_send");
            }

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
    pthread_t task1;
    pthread_t task2;
    pthread_attr_t tattr;

    init_main_thread();

    printf("== Real Time Test \n"
           "== Test name: %s \n"
           "== All results in microseconds\n",
           test_name);

    //set task sched attr
    if (argc == 1) {
        setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO), cpu);
        err = pthread_create(&task1, &tattr, server_function, NULL);
        if (err)
            fail("pthread_create()");
    } else {
        setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO) - 1, cpu);
        err = pthread_create(&task2, &tattr, client_function, NULL);
        if (err)
            fail("pthread_create()");
    }
    pthread_attr_destroy(&tattr);

    pthread_join(task1, NULL);
    pthread_join(task2, NULL);

    return 0;
}

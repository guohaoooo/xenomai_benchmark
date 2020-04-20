#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include "../util.h"
#ifndef __XENO__
#include <stdint.h>
#endif

#define SAMPLES_NUM  100000
#define SAMPLES_LOOP 100
#define MQ_NAME "/mq"

char test_name[32] = "unavailable_mq_receive";

void *function(void *arg)
{
//    int dog = 0;
    int err, i, loop = SAMPLES_LOOP;
    char ret[10] = {0};
    mqd_t mq;
    struct mq_attr mqattr = {O_NONBLOCK, 1, 10, 0};

    mq_unlink(MQ_NAME);

    mq = mq_open(MQ_NAME, O_RDWR|O_NONBLOCK|O_CREAT, S_IRUSR|S_IWUSR, &mqattr);
    if(mq == (mqd_t)-1) {
        fail("mq_open");
    }

    for (i = 0; i < loop; i++) {

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

        print_result(i, samples, min, max, sum);
#if 0
        dog++;
        if (dog%10 == 0)
            sleep(1);
#endif
    }

    mq_unlink(MQ_NAME);

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#ifndef __XENO__
#include <stdint.h>
#endif
#include "../util.h"

#define SAMPLES_NUM   10000
#define SAMPLES_LOOP  100
#define MQ_NAME "/mq"

#ifndef MQ_PRIO_MAX
#define MQ_PRIO_MAX 32768
#endif

char test_name[32] = "mq_send_receive_full";

void *function(void *arg)
{
//    int dog = 0;
    int err, fill, i, loop = SAMPLES_LOOP;
    unsigned int priority;
    char ret[256] = {0};
    mqd_t mq;
    struct mq_attr mqattr = {0, 64, 256, 0};

    mq_unlink(MQ_NAME);

    mq = mq_open(MQ_NAME, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR, &mqattr);
    if(mq == (mqd_t)-1) {
        fail("mq_open");
    }

    for (fill = 0; fill < 64; ++fill)
        mq_send(mq, ret, 256, rand() % MQ_PRIO_MAX);

    for (i = 0; i < loop; i++) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);

            err = mq_receive(mq, ret, sizeof(ret), &priority);
            if (errno != EAGAIN && err < 0) {
                fail("mq_receive err");
            }

            err = mq_send(mq, ret, 256, rand() % MQ_PRIO_MAX);
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

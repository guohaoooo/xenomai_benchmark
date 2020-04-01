#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#ifdef __XENO__
#include <rtdm/ipc.h>
#else
#include <stdint.h>
#endif
#include "../util.h"

#define SAMPLES_NUM  10000
#define MSG_LEN 1024
#define IDDP_CLPORT 27
#define IDDP_PORT_LABEL "iddp-server"

char test_name[32] = "messsage_inter_process";

void *server_function(void *arg)
{
    int dog = 0;
    struct sockaddr_ipc saddr, claddr;
    struct rtipc_port_label plabel;
    socklen_t addrlen;
    size_t buffsz;
    char buf[1024];
    int ret, s;

    s = socket(AF_RTIPC, SOCK_DGRAM, IPCPROTO_IDDP);
    if (s < 0)
        fail("socket");


    strcpy(plabel.label, IDDP_PORT_LABEL);
    ret = setsockopt(s, SOL_IDDP, IDDP_LABEL,
                    &plabel, sizeof(plabel));
    if(ret)
        fail("setsockopt");

    saddr.sipc_family = AF_RTIPC;
    saddr.sipc_port = -1;
    ret = bind(s, (struct sockaddr *)&saddr, sizeof(saddr));
    if(ret)
        fail("bind");

    for (;;) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {
            addrlen = sizeof(saddr);

            clock_gettime(CLOCK_MONOTONIC, &start);
            ret = read(s, buf, sizeof(buf));
            if (ret < 0) {
                close(s);
                fail("read");
            }
#if 1
            ret = write(s, buf, sizeof(buf));
            if (ret < 0) {
                close(s);
                fail("write");
            }
#endif
            clock_gettime(CLOCK_MONOTONIC, &end);

            dt = (int32_t)diff_ts(&end, &start);

            if (dt > max)
                max = dt;

            if (dt < min)
                min = dt;

            sum += dt;
        }

        printf("Result|samples:recv-%d  %11d|min:%11.3f|avg:%11.3f|max:%11.3f\n",
                        ret,
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

void *client_function(void *arg)
{
    int dog = 0;
    struct sockaddr_ipc svsaddr, clsaddr;
    struct rtipc_port_label plabel;
    int ret, s, n = 0, len = MSG_LEN;
    char buf[1024] = {0};

    s = socket(AF_RTIPC, SOCK_DGRAM, IPCPROTO_IDDP);
    if (s < 0)
        fail("socket");


    clsaddr.sipc_family = AF_RTIPC;
    clsaddr.sipc_port = IDDP_CLPORT;
    ret = bind(s, (struct sockaddr *)&clsaddr, sizeof(clsaddr));
    if(ret)
        fail("bind");

    strcpy(plabel.label, IDDP_PORT_LABEL);
    ret = setsockopt(s, SOL_IDDP, IDDP_LABEL,
                    &plabel, sizeof(plabel));
    if (ret)
        fail("setsockopt");

    memset(&svsaddr, 0, sizeof(svsaddr));
    svsaddr.sipc_family = AF_RTIPC;
    svsaddr.sipc_port = -1;
    ret = connect(s, (struct sockaddr *)&svsaddr, sizeof(svsaddr));
    if(ret)
        fail("connect");

    for (;;) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;

        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);

            ret = write(s, buf, len);
            if (ret < 0) {
                close(s);
                fail("write");
            }

            clock_gettime(CLOCK_MONOTONIC, &end);

            dt = (int32_t)diff_ts(&end, &start);

            if (dt > max)
                max = dt;

            if (dt < min)
                min = dt;

            sum += dt;

        }

        printf("Result|samples:client-%d %11d|min:%11.3f|avg:%11.3f|max:%11.3f\n",
                        len,
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

    init_main_thread();

    printf("== Real Time Test \n"
           "== Test name: %s pid: %d\n"
           "== All results in microseconds\n",
           test_name, getpid());

    if (argc == 1) {
        setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO), cpu);
        err = pthread_create(&task1, &tattr, server_function, NULL);
        if (err)
            fail("pthread_create()");

        pthread_join(task1, NULL);
    } else {
        setup_sched_parameters(&tattr, sched_get_priority_max(SCHED_FIFO) - 1, cpu);
        err = pthread_create(&task2, &tattr, client_function, NULL);
        if (err)
            fail("pthread_create()");

        pthread_join(task2, NULL);
    }

    pthread_attr_destroy(&tattr);

    return 0;
}

#include <xeno_config.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>


#define ONE_BILLION  1000000000
#define TEN_MILLIONS 10000000
#define SAMPLES_NUM  1000000

char test_name[32] = "unavailable_sem_inter_thread";

static inline long long diff_ts(struct timespec *left, struct timespec *right)
{
    return (long long)(left->tv_sec - right->tv_sec) * ONE_BILLION
        + left->tv_nsec - right->tv_nsec;
}

static const char *reason_str[] = {
    [SIGDEBUG_UNDEFINED] = "received SIGDEBUG for unknown reason",
    [SIGDEBUG_MIGRATE_SIGNAL] = "received signal",
    [SIGDEBUG_MIGRATE_SYSCALL] = "invoked syscall",
    [SIGDEBUG_MIGRATE_FAULT] = "triggered fault",
    [SIGDEBUG_MIGRATE_PRIOINV] = "affected by priority inversion",
    [SIGDEBUG_NOMLOCK] = "process memory not locked",
    [SIGDEBUG_WATCHDOG] = "watchdog triggered (period too short?)",
    [SIGDEBUG_LOCK_BREAK] = "scheduler lock break",
};

static void sigdebug(int sig, siginfo_t *si, void *context)
{
    const char fmt[] = "%s, aborting.\n"
    	"(enabling CONFIG_XENO_OPT_DEBUG_TRACE_RELAX may help)\n";
    unsigned int reason = sigdebug_reason(si);
    int n __attribute__ ((unused));
    static char buffer[256];
    
    if (reason > SIGDEBUG_WATCHDOG)
    	reason = SIGDEBUG_UNDEFINED;
    
    switch(reason) {
    case SIGDEBUG_UNDEFINED:
    case SIGDEBUG_NOMLOCK:
    case SIGDEBUG_WATCHDOG:
    	n = snprintf(buffer, sizeof(buffer), "latency: %s\n",
    		     reason_str[reason]);
    	n = write(STDERR_FILENO, buffer, n);
    	exit(EXIT_FAILURE);
    }
    
    n = snprintf(buffer, sizeof(buffer), fmt, reason_str[reason]);
    n = write(STDERR_FILENO, buffer, n);
    signal(sig, SIG_DFL);
    kill(getpid(), sig);
}

static void setup_sched_parameters(pthread_attr_t *attr, int prio, int cpu)
{
    int ret;
    cpu_set_t cpus;
    struct sched_param p;
    
    ret = pthread_attr_init(attr);
    if (ret)
    	error(1, ret, "pthread_attr_init()");
    
    ret = pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
    if (ret)
    	error(1, ret, "pthread_attr_setinheritsched()");
    
    ret = pthread_attr_setschedpolicy(attr, prio ? SCHED_FIFO : SCHED_OTHER);
    if (ret)
    	error(1, ret, "pthread_attr_setschedpolicy()");
    
    p.sched_priority = prio;
    ret = pthread_attr_setschedparam(attr, &p);
    if (ret)
    	error(1, ret, "pthread_attr_setschedparam()");
    
    CPU_ZERO(&cpus);
    CPU_SET(cpu, &cpus);
    ret = pthread_attr_setaffinity_np(attr, sizeof(cpus), &cpus);
    if (ret)
        error(1, ret, "pthread_attr_setaffinity_np()");
}


void *function(void *arg) 
{
    int dog = 0, err;
    sem_t sem;

    err = sem_init(&sem, 0, 0);
    if(err)
        error(1, err, "sem_init()");

    for (;;) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;
        
        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);
            err = sem_post(&sem);
            if (err)
                error(1, err, "sem_post()");
            err = sem_wait(&sem);
            if (err)
                error(1, err, "sem_wait()");
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

static void init_main_thread()
{
    int err, cpu = 0, policy;
    cpu_set_t cpus;
    sigset_t mask;
    struct sigaction sa __attribute__((unused));
    struct sched_param param, old_param;

    // block signal 
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    // debug signal
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = sigdebug;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGDEBUG, &sa, NULL);

    // set debug mode
    err = pthread_setmode_np(0, PTHREAD_WARNSW, NULL);
    if (err)
        error(1, err, "pthread_setmode_np()");

    // set cpu affinity
    CPU_ZERO(&cpus);
    CPU_SET(cpu, &cpus);
    err = pthread_setaffinity_np(pthread_self(), sizeof(cpus), &cpus);
    if (err)
        error(1, err, "pthread_setaffinity_np");

#if 0
    //set main thread sched policy
    err = pthread_getschedparam(pthread_self(), &policy, &old_param);
    if(err)
        error(1, err, "pthread_getschedparam");

    if ((policy != SCHED_FIFO) && (policy != SCHED_RR)) {
        param = old_param;
        param.sched_priority = 99;
        err = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
        if (err)
            error(1, err, "pthread_setschedparam");
    }
#endif

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
        error(1, err, "pthread_create()");

    pthread_attr_destroy(&tattr);

    pthread_join(task, NULL);

    return 0;
}

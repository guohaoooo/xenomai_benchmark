#include <xeno_config.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <error.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>


#define ONE_BILLION  1000000000
#define TEN_MILLIONS 10000000
#define SAMPLES_NUM  100000

char test_name[32] = "pthread_create_join";

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

static void setup_sched_parameters(pthread_attr_t *attr, int prio, cpu_set_t * pcpus)
{
	struct sched_param p;
	int ret;
	
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

        ret = pthread_attr_setaffinity_np(attr, sizeof(*pcpus), pcpus);
        if (ret)
            error(1, ret, "pthread_attr_setaffinity_np()");

        ret = pthread_attr_setstacksize(attr, PTHREAD_STACK_MIN);
        if (ret)
            error(1, ret, "pthread_attr_setstacksize()");
}

void *emptyfunction(void *arg) {return (arg);}

int main(int argc, char *const *argv)
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
    //set main sched policy
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

    printf("== Real Time Test \n"
           "== Test name: %s \n"
           "== All results in microseconds\n",
           test_name);


    pthread_t task;
    pthread_attr_t tattr;
    //set task sched attr
    setup_sched_parameters(&tattr, 99, &cpus);

    for (;;) {

        int32_t dt, max = -TEN_MILLIONS, min = TEN_MILLIONS;
        int64_t sum;
        int count, samples = SAMPLES_NUM;
        struct timespec start, end;
        
        for (count = sum = 0; count < samples; count++) {

            clock_gettime(CLOCK_MONOTONIC, &start);

            err = pthread_create(&task, &tattr, emptyfunction, NULL);
            if (err)
                error(1, err, "pthread_create()");

            pthread_join(task, NULL);

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

    }

    return 0;
}

#include<stdio.h>
#include<stdlib.h>
#include<signal.h>
#include<errno.h>
#include<pthread.h>
#include<semaphore.h>
#include<fcntl.h>
#include<unistd.h>
#ifndef __XENO__
#include<stdint.h>
#endif

#define SEM_NAME "/sync_task"

void fail(const char *reason)
{
    perror(reason);
    exit(EXIT_FAILURE);
}

#ifdef __XENO__
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
#endif

void print_result(int loop, int samples, int32_t min, int32_t max, int64_t sum)
{

    printf("Result|loop:%5d|samples:%11d|min:%11.3f|avg:%11.3f|max:%11.3f\n",
                loop, samples,
                (double)min / 1000,
                (double)sum / (samples * 1000),
                (double)max / 1000);

}

void print_header(char *name)
{
    printf("== Real Time Test \n"
           "== Test name: %s \n"
           "== All result in microseconds\n",
           name);
}

void sync_process_step(void *first)
{
    int err;
    sem_t *sem;

    if (first) {
        sem_unlink(SEM_NAME);
        sem = sem_open(SEM_NAME, O_ACCMODE|O_CREAT, S_IRUSR|S_IWUSR, 0);
        if(sem == SEM_FAILED)
            fail("sem open failed");

        err = sem_wait(sem);
        if (err)
            fail("sem wait failed");

        sem_unlink(SEM_NAME);

    } else {
        sem = sem_open(SEM_NAME, O_ACCMODE, 0, 0);
        if (sem == SEM_FAILED)
            fail("sem open failed");

        err = sem_post(sem);
        if (err)
            fail("sem post");
    }
}

void setup_sched_parameters(pthread_attr_t *attr, int prio, int cpu)
{
	int ret;
        cpu_set_t cpus;
	struct sched_param p;

	ret = pthread_attr_init(attr);
	if (ret)
		fail("pthread_attr_init()");

	ret = pthread_attr_setinheritsched(attr, PTHREAD_EXPLICIT_SCHED);
	if (ret)
		fail("pthread_attr_setinheritsched()");

	ret = pthread_attr_setschedpolicy(attr, prio ? SCHED_FIFO : SCHED_OTHER);
	if (ret)
		fail("pthread_attr_setschedpolicy()");

	p.sched_priority = prio;
	ret = pthread_attr_setschedparam(attr, &p);
	if (ret)
		fail("pthread_attr_setschedparam()");

        CPU_ZERO(&cpus);
        CPU_SET(cpu, &cpus);
        ret = pthread_attr_setaffinity_np(attr, sizeof(cpus), &cpus);
        if (ret)
            fail("pthread_attr_setaffinity_np()");

}

void init_main_thread() {
    int err, cpu = 0;
    cpu_set_t cpus;
    sigset_t mask;
    struct sigaction sa __attribute__((unused));

    // block signal
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

#ifdef __XENO__
    // debug signal
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = sigdebug;
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGDEBUG, &sa, NULL);

    // set debug mode
    err = pthread_setmode_np(0, PTHREAD_WARNSW, NULL);
    if (err)
        fail("pthread_setmode_np()");
#endif
    // set cpu affinity
    CPU_ZERO(&cpus);
    CPU_SET(cpu, &cpus);
    err = pthread_setaffinity_np(pthread_self(), sizeof(cpus), &cpus);
    if (err)
        fail("pthread_setaffinity_np");

}


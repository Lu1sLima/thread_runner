#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <linux/sched.h>

int N_THREADS = 0;
int BUFFER_SIZE = 0;
int POLICY_TYPE = 0;
int POLICY_PRIORITY = 0;

volatile int running = 1;
char *buffer = NULL;
int idx = 0;
pthread_mutex_t mutex;

pthread_barrier_t barrier;
volatile int threads_created = 0;

int parse_policy_type(const char *policy_str) {
    if (strcmp(policy_str, "SCHED_DEADLINE") == 0) {
        return SCHED_DEADLINE;
    } else if (strcmp(policy_str, "SCHED_FIFO") == 0) {
        return SCHED_FIFO;
    } else if (strcmp(policy_str, "SCHED_RR") == 0) {
        return SCHED_RR;
    } else if (strcmp(policy_str, "SCHED_OTHER") == 0) {
        return SCHED_OTHER;
    } else if (strcmp(policy_str, "SCHED_BATCH") == 0) {
        return SCHED_BATCH;
    } else if (strcmp(policy_str, "SCHED_IDLE") == 0) {
        return SCHED_IDLE;
    } else {
        return -1; // Unknown policy type
    }
}


void *run(void *data)
{
    int i = (int) data;
    char letter = (char) ('A' + i);

    pthread_barrier_wait(&barrier); // Wait for all threads to be created

    while (idx < BUFFER_SIZE) {
        pthread_mutex_lock(&mutex);
        buffer[idx] = letter;
        idx++;
        pthread_mutex_unlock(&mutex);
    }
    printf("Inicia thread: %d, %c\n", i, letter);

    return 0;
}

void print_sched(int policy)
{
    int priority_min, priority_max;

    switch(policy){
        case SCHED_DEADLINE:
            printf("SCHED_DEADLINE");
            break;
        case SCHED_FIFO:
            printf("SCHED_FIFO");
            break;
        case SCHED_RR:
            printf("SCHED_RR");
            break;
        case SCHED_OTHER:
            printf("SCHED_OTHER");
            break;
        case SCHED_BATCH:
            printf("SCHED_BATCH");
            break;
        case SCHED_IDLE:
            printf("SCHED_IDLE");
            break;
        default:
            printf("unknown\n");
    }
    priority_min = sched_get_priority_min(policy);
    priority_max = sched_get_priority_max(policy);
    printf(" PRI_MIN: %d PRI_MAX: %d\n", priority_min, priority_max);
}

int setpriority(pthread_t *thr, int newpolicy, int newpriority)
{
    int policy, ret;
    struct sched_param param;

    if (newpriority > sched_get_priority_max(newpolicy) || newpriority < sched_get_priority_min(newpolicy)){
        printf("Invalid priority: MIN: %d, MAX: %d", sched_get_priority_min(newpolicy), sched_get_priority_max(newpolicy));

        return -1;
    }

    pthread_getschedparam(*thr, &policy, &param);
    printf("current: ");
    print_sched(policy);

    param.sched_priority = newpriority;
    ret = pthread_setschedparam(*thr, newpolicy, &param);
    if (ret != 0)
        perror("perror(): ");

    pthread_getschedparam(*thr, &policy, &param);
    printf("new: ");
    print_sched(policy);

    return 0;
}


int main(int argc, char **argv)
{
    if (argc < 5) {
        printf("usage: ./%s <numero_de_threads> <tamanho_do_buffer_global_em_kilobytes> <politica> <prioridade>\n\n", argv[0]);
        return 0;
    }

    N_THREADS = atoi(argv[1]);
    BUFFER_SIZE = atoi(argv[2]) * 1024;
    POLICY_TYPE = parse_policy_type(argv[3]);
    // POLICY_TYPE = atoi(argv[3]);
    POLICY_PRIORITY = atoi(argv[4]);

    pthread_t thr[N_THREADS];
    buffer = (char*) malloc(sizeof(char) * BUFFER_SIZE);
    memset(buffer, 0, sizeof(char) * BUFFER_SIZE);
    pthread_mutex_init(&mutex, NULL);
    pthread_barrier_init(&barrier, NULL, N_THREADS + 1);

    printf("Inicio: %d %d %d %d\n", N_THREADS, BUFFER_SIZE, POLICY_TYPE, POLICY_PRIORITY);

    for(int i = 0; i < N_THREADS; i++) {
        pthread_create(&thr[i], NULL, run, (void*)i);
        setpriority(&thr[i], POLICY_TYPE, POLICY_PRIORITY);
    }

    pthread_barrier_wait(&barrier); // Wait for all threads to be created

    for(int i = 0; i < N_THREADS; i++) {
        pthread_join(thr[i], NULL);
    }

    // for(int i = 0; i < BUFFER_SIZE; i++) {
    //     printf("%c", buffer[i]);
    // }

    int count[26] = {0}; 
    char* groupedString = (char*) malloc(sizeof(char) * (BUFFER_SIZE + 1));
    memset(groupedString, 0, sizeof(char) * (BUFFER_SIZE + 1));

    char lastLetter = NULL;
    for (int i = 0; i < BUFFER_SIZE; i++) {
        if (buffer[i] != lastLetter) {
            strncat(groupedString, &buffer[i], 1);
            lastLetter = buffer[i];

            char c = buffer[i];
            if (c >= 'A' && c <= 'Z') {
                count[c - 'A']++;
            }
        }
    }

    printf("\n\n");

    printf("%s\n", groupedString);


    for (int i = 0; i < 26; i++) {
        if (count[i] > 0) {
            printf("%c: %d\n", 'A' + i, count[i]);
        }
    }

    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);
    free(buffer);
    free(groupedString);
    printf("\n\nFim\n");

    return 0;
}

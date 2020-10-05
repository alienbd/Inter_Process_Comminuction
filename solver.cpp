#include <iostream>
#include <cstdio>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <cstring>
#include <time.h>
#include <queue>

#include <cstdlib>
#include <ctime>

#define serviceman 3
#define paybooth 2
#define cycle 10

using namespace std;

pthread_mutex_t service_locks[serviceman];

pthread_mutex_t payment_q_lock;
pthread_mutex_t cycle_lock;

sem_t customer_count_lock;
sem_t paybooth_count_lock;

sem_t customer_lock;
sem_t paybooth_lock;
sem_t room_lock;

sem_t full_q;
sem_t buff;

queue<int> payment_q;

int cycle_count = 0;
int customer_count = 0;
int paybooth_count = 0;

int randN()
{
    srand((unsigned)time(0));
    return 1 + rand() % 3;
}
void *take_service(void *arg)
{
    sem_wait(&room_lock);
    sem_wait(&customer_lock);
    //prio_lock_low(&customer_lock);
    //printf("%s\n",(char *)arg);

    sem_wait(&customer_count_lock);
    customer_count++;
    //printf("up %d %s\n",customer_count,(char *)arg);
    if (customer_count == 1)
    {
        sem_wait(&paybooth_lock);
        //printf("Paybooth locked\n");
    }
    sem_post(&customer_count_lock);

    for (int i = 0; i < serviceman; i++)
    {
        pthread_mutex_lock(&service_locks[i]);
        printf("%s started taking service from serviceman %d\n", (char *)arg, i + 1);
        int t = randN();
        sleep(t);
        printf("%s finished taking service from serviceman %d\n", (char *)arg, i + 1);
        pthread_mutex_unlock(&service_locks[i]);
        if (i == 0)
        {
            sem_post(&customer_lock);
            sem_post(&room_lock);
        }
    }
    //printf("here %s\n",(char *)arg);
    //sem_wait(&empty_q);

    pthread_mutex_lock(&payment_q_lock);
    //printf("%s started paying the bill\n",(char*)arg);
    int value = atoi((char *)arg);
    payment_q.push(value);
    pthread_mutex_unlock(&payment_q_lock);

    sem_wait(&customer_count_lock);
    customer_count--;
    //printf("down %d %s\n",customer_count,(char *)arg);
    if (customer_count == 0)
    {
        sem_post(&paybooth_lock);
        //printf("Paybooth UNlocked\n");
    }
    sem_post(&customer_count_lock);

    sem_post(&full_q);

    pthread_exit((void *)0);
}

void *take_bill(void *arg)
{
    int c, t;
    while (cycle_count < cycle)
    {
        pthread_mutex_lock(&cycle_lock);
        cycle_count++;
        pthread_mutex_unlock(&cycle_lock);

        sem_wait(&full_q);

        sem_wait(&buff);

        pthread_mutex_lock(&payment_q_lock);
        c = payment_q.front();
        printf("%d started paying the bill\n", c);
        payment_q.pop();
        t = randN() + 2;
        sleep(t);
        pthread_mutex_unlock(&payment_q_lock);

        sem_post(&buff);

        sem_wait(&paybooth_count_lock);
        paybooth_count++;
        if (paybooth_count == 1)
        {
            sem_wait(&customer_lock);
        }
        sem_post(&paybooth_count_lock);

        printf("%d finished paying the bill\n", c);

        sem_wait(&paybooth_lock);

        t = randN();
        sleep(t);
        printf("%d departed\n", c);

        sem_post(&paybooth_lock);

        sem_wait(&paybooth_count_lock);
        paybooth_count--;
        if (paybooth_count == 0)
        {
            //if(payment_q.empty()){
            sem_post(&customer_lock);
            //prio_unlock_high(&customer_lock);
            //printf("customer unlocked\n");
        }
        sem_post(&paybooth_count_lock);
    }

    pthread_exit((void *)0);
}

void initSem()
{
    int res;

    res = sem_init(&full_q, 0, 0);
    if (res != 0)
        printf("Failed1\n");

    res = sem_init(&buff, 0, paybooth);
    if (res != 0)
        printf("Failed1\n");

    for (int i = 0; i < serviceman; i++)
    {
        res = pthread_mutex_init(&service_locks[i], NULL);
        if (res != 0)
            printf("Failed3\n");
    }

    res = pthread_mutex_init(&cycle_lock, NULL);
    if (res != 0)
        printf("Failed35\n");

    res = pthread_mutex_init(&payment_q_lock, NULL);
    if (res != 0)
        printf("Failed5\n");

    res = sem_init(&customer_lock, 0, 1);
    if (res != 0)
        printf("Failed15\n");

    res = sem_init(&room_lock, 0, 1);
    if (res != 0)
        printf("Failed15\n");

    res = sem_init(&paybooth_lock, 0, 1);
    if (res != 0)
        printf("Failed25\n");

    res = sem_init(&customer_count_lock, 0, 1);
    if (res != 0)
        printf("Failed15\n");

    res = sem_init(&paybooth_count_lock, 0, 1);
    if (res != 0)
        printf("Failed25\n");
}

void destroySem()
{
    int res;
    res = sem_destroy(&full_q);
    if (res != 0)
        printf("Failed6\n");

    res = sem_destroy(&buff);
    if (res != 0)
        printf("Failed1\n");

    for (int i = 0; i < serviceman; i++)
    {
        res = pthread_mutex_destroy(&service_locks[i]);
        if (res != 0)
            printf("Failed8\n");
    }

    res = pthread_mutex_destroy(&payment_q_lock);
    if (res != 0)
        printf("Failed10\n");

    res = pthread_mutex_destroy(&cycle_lock);
    if (res != 0)
        printf("Failed36\n");

    res = sem_destroy(&customer_count_lock);
    if (res != 0)
        printf("Failed15\n");

    res = sem_destroy(&paybooth_count_lock);
    if (res != 0)
        printf("Failed25\n");

    res = sem_destroy(&customer_lock);
    if (res != 0)
        printf("Failed15\n");

    res = sem_destroy(&room_lock);
    if (res != 0)
        printf("Failed15\n");

    res = sem_destroy(&paybooth_lock);
    if (res != 0)
        printf("Failed25\n");
}

int main(int argc, char *argv[])
{
    int res;
    initSem();

    pthread_t payment_thred[paybooth];

    for (int i = 0; i < paybooth; i++)
    {
        res = pthread_create(&payment_thred[i], NULL, take_bill, NULL);

        if (res != 0)
            printf("Thread creation failed (payment)\n");
    }

    pthread_t cycles[cycle];
    for (int i = 0; i < cycle; i++)
    {
        char *id = new char[3];
        strcpy(id, to_string(i + 1).c_str());

        res = pthread_create(&cycles[i], NULL, take_service, (void *)id);

        if (res != 0)
        {
            printf("Thread creation failed (cycles)\n");
        }
    }

    for (int i = 0; i < cycle; i++)
    {
        void *result;
        pthread_join(cycles[i], NULL);
        //printf("%s",(char*)result);
    }

    for (int i = 0; i < paybooth; i++)
    {
        void *result;
        pthread_join(payment_thred[i], NULL);
        //printf("%s",(char*)result);
    }

    destroySem();

    return 0;
}

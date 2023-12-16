#include <iostream>
#include <pthread.h>
#include <random>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

const int bufSize = 20;
int buf[bufSize];

int rear = 0;
int front = 0;

sem_t empty;
sem_t full;

pthread_mutex_t mutex_write;
pthread_mutex_t mutex_output;

unsigned int seed = 101;
std::vector<pthread_t> adder_threads;

int get_random_int(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

struct Adder {
    int adder_num;
    int number1;
    int number2;
};

void write_data(int data) {
    sem_wait(&empty);
    buf[rear] = data;
    rear = (rear + 1) % bufSize;
    sem_post(&full);
}


void *producer(void *param) {
    int producer_thread_num = *((int *) param);
    int data = get_random_int(1, 20);

    sleep(get_random_int(1, 7));

    pthread_mutex_lock(&mutex_write);

    pthread_mutex_lock(&mutex_output);
    printf("Producer %d: Writes a new value = %d to cell [%d]\n", producer_thread_num, data, rear);
    pthread_mutex_unlock(&mutex_output);

    write_data(data);

    pthread_mutex_unlock(&mutex_write);

    return nullptr;
}

void *adder(void *param) {
    Adder adder_obj = *(Adder *) (param);
    int adder_num = adder_obj.adder_num;
    int sum = adder_obj.number1 + adder_obj.number2;
    sleep(get_random_int(3, 6));

    pthread_mutex_lock(&mutex_write);

    pthread_mutex_lock(&mutex_output);
    printf("Adder %d: Calculate sum = %d to cell [%d]\n", adder_num, sum, rear);
    pthread_mutex_unlock(&mutex_output);

    write_data(sum);

    pthread_mutex_unlock(&mutex_write);

    return nullptr;
}

void *consumer(void *param) {
    int adder_cnt = 0;
    for (int i = 0; i < 19; ++i) {
        sem_wait(&full);
        sem_wait(&full);

        int number1 = buf[front];
        front = (front + 1) % bufSize;

        pthread_mutex_lock(&mutex_output);
        printf("Reader reads value = %d from cell [%d]\n", number1, front - 1 < 0 ? bufSize - 1 : front - 1);
        pthread_mutex_unlock(&mutex_output);

        int number2 = buf[front];
        front = (front + 1) % bufSize;

        pthread_mutex_lock(&mutex_output);
        printf("Reader reads value = %d from cell [%d]\n", number2, front - 1 < 0 ? bufSize - 1 : front - 1);
        pthread_mutex_unlock(&mutex_output);

        Adder *adder_obj = new Adder{adder_cnt, number1, number2};
        pthread_create(&adder_threads.emplace_back(), nullptr, adder, (void *) (adder_obj));
        adder_cnt++;

        sem_post(&empty);
        sem_post(&empty);
    }
    for (auto &x: adder_threads)
        pthread_join(x, nullptr);

    printf("\nSum = %d", buf[rear]);

    return nullptr;
}


int main() {
    srand(seed);
    pthread_mutex_init(&mutex_write, nullptr);

    pthread_mutex_init(&mutex_output, nullptr);

    sem_init(&empty, 0, bufSize);
    sem_init(&full, 0, 0);

    pthread_t reader;
    pthread_create(&reader, nullptr, consumer, nullptr);

    pthread_t threadP[20];
    int producers[20];
    for (int i = 0; i < 20; i++) {
        producers[i] = i + 1;
        pthread_create(&threadP[i], nullptr, producer, (void *) (producers + i));
    }

    for (auto x: threadP)
        pthread_join(x, nullptr);

    pthread_join(reader, nullptr);

    std::cout << '\n'
              << rear - 1 << '\n';
    return 0;
}
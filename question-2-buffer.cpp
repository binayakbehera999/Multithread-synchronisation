#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <iostream>
#define CAP 5
#define CROSSTIME 1
#define MAXWAIT 2

using namespace std;

pthread_mutex_t bridgeMutex;
pthread_cond_t empty;

int peopleOnBridge = 0;
int bridgeDirection = 1;

// CASE 1
int enterBridge1(int direction){
    pthread_mutex_lock(&bridgeMutex);
    if(peopleOnBridge > 0 && bridgeDirection != direction){
        pthread_mutex_unlock(&bridgeMutex);
        return 0;
    }
    else {
        while (peopleOnBridge > 0 && (peopleOnBridge >= CAP)){
            pthread_cond_wait(&empty, &bridgeMutex);
            }
        if (peopleOnBridge == 0)
            bridgeDirection = direction;
        peopleOnBridge++;
    }
    pthread_mutex_unlock(&bridgeMutex);
    return 1;
}

void exitBridge1(){
    pthread_mutex_lock(&bridgeMutex);
    peopleOnBridge--;
    pthread_cond_signal(&empty);
    pthread_mutex_unlock(&bridgeMutex);
}

void* northWorker1(void *arg){
    sleep(rand() % MAXWAIT);
    if(enterBridge1(1)){
        cout << "Crossing bridge from north...Number of people on bridge " << peopleOnBridge << "\n";
        sleep(CROSSTIME);
        exitBridge1();
    }
    else{
        cout << "Cant cross the bridge from north... \n";
    }
}

void* southWorker1(void *arg){
    if(enterBridge1(2)){
        cout << "Crossing bridge from south...Number of people on bridge " << peopleOnBridge << "\n";
        sleep(CROSSTIME);
        exitBridge1();
    }
    else{
        cout << "Cant cross the bridge from south... \n";
    }
}

int case1(int numOfNorthPerson, int numOfSouthPerson){
    pthread_t northThread[numOfNorthPerson], southThread[numOfSouthPerson];

    pthread_mutex_init(&bridgeMutex, NULL);
    pthread_cond_init(&empty, NULL);

    for (int i = 0; i < max(numOfNorthPerson, numOfSouthPerson); i++){
        if(i < numOfSouthPerson)
            if (pthread_create(&southThread[i], NULL, southWorker1, NULL))
            {
                printf("thread creation failed\n");
                return EXIT_FAILURE;
            }
        if(i < numOfNorthPerson)
            if (pthread_create(&northThread[i], NULL, northWorker1, NULL))
            {
                printf("thread creation failed\n");
                return EXIT_FAILURE;
            }
    }

    

    for (int i = 0; i < max(numOfNorthPerson, numOfSouthPerson); i++){
        if(i < numOfNorthPerson)
            if (northThread[i])
                pthread_join(northThread[i], NULL);
        if(i < numOfSouthPerson)
            if (southThread[i])
                pthread_join(southThread[i], NULL);
    }
}

// CASE 2
void enterBridge2(int direction){
    pthread_mutex_lock(&bridgeMutex);
    while (peopleOnBridge > 0 && (peopleOnBridge >= CAP || direction != bridgeDirection)){
        pthread_cond_wait(&empty, &bridgeMutex);
        }
    if (peopleOnBridge == 0)
        bridgeDirection = direction;
    peopleOnBridge++;
    pthread_mutex_unlock(&bridgeMutex);
}

void exitBridge2(){
    pthread_mutex_lock(&bridgeMutex);
    peopleOnBridge--;
    pthread_cond_signal(&empty);
    pthread_mutex_unlock(&bridgeMutex);
}

void* northWorker2(void *arg){
    enterBridge2(1);
    cout << "Crossing bridge...Number of people on bridge " << peopleOnBridge << "\n";
    sleep(CROSSTIME);
    exitBridge2();
    cout << "Exited bridge....Travelled from north to south side...\n";
}

void* southWorker2(void *arg){
    enterBridge2(2);
    cout << "Crossing bridge...Number of people on bridge " << peopleOnBridge << "\n";
    sleep(CROSSTIME);
    exitBridge2();
    cout << "Exited bridge....Travelled from south to north side...\n";
}

int case2(int numOfNorthPerson, int numOfSouthPerson){
    pthread_t northThread[numOfNorthPerson], southThread[numOfSouthPerson];

    pthread_mutex_init(&bridgeMutex, NULL);
    pthread_cond_init(&empty, NULL);

    for (int i = 0; i < max(numOfNorthPerson, numOfSouthPerson); i++){
        if(i < numOfNorthPerson)
            if (pthread_create(&northThread[i], NULL, northWorker2, NULL))
            {
                printf("thread creation failed\n");
                return EXIT_FAILURE;
            }
        if(i < numOfSouthPerson)
            if (pthread_create(&southThread[i], NULL, southWorker2, NULL))
            {
                printf("thread creation failed\n");
                return EXIT_FAILURE;
            }
    }

    for (int i = 0; i < max(numOfNorthPerson, numOfSouthPerson); i++){
        if(i < numOfNorthPerson)
            if (northThread[i])
            pthread_join(northThread[i], NULL);
        if(i < numOfSouthPerson)
            if (southThread[i])
            pthread_join(southThread[i], NULL);
    }
}

int main(){
    int numOfNorthPerson, numOfSouthPerson;
    int t;

    cout << "Enter Number of North Person\n";
    cin >> numOfNorthPerson;
    cout << "Enter Number of South Person\n";
    cin >> numOfSouthPerson;
    cout << "Enter Case\n";
    cin >> t;

    switch (t)
    {
        case 1:
            case1(numOfNorthPerson, numOfSouthPerson);
            break;
        case 2:
            case2(numOfNorthPerson, numOfSouthPerson);
            break;
    }
    return 0;
}
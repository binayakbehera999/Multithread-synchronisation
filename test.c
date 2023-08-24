#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#define EAST 1
#define WEST 2

typedef struct bridge
{
    int cars;
    int direction;
    pthread_mutex_t mutex;
    pthread_cond_t empty;
} bridge_t;

bridge_t shared_bridge = {
    .cars = 0,
    .direction = WEST,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .empty = PTHREAD_COND_INITIALIZER,
};

void arrive(bridge_t *bridge, int direction)
{
    pthread_mutex_lock(&bridge->mutex);
    while (bridge->cars > 0 && (bridge->cars > 2 || bridge->direction != direction))
        pthread_cond_wait(&bridge->empty, &bridge->mutex);
    if (bridge->cars == 0)
        bridge->direction = direction;
    bridge->cars++;
    pthread_mutex_unlock(&bridge->mutex);
}

void cross(bridge_t *bridge)
{
    if (bridge->direction == 1)
        printf("Crossing East ... number of Cars on bridge = %d \n", bridge->cars);
    else
        printf("Crossing West ... number of Cars on bridge = %d \n", bridge->cars);
    sleep(1);
}

void leave(bridge_t *bridge)
{
    pthread_mutex_lock(&bridge->mutex);
    bridge->cars--;
    pthread_cond_signal(&bridge->empty);
    pthread_mutex_unlock(&bridge->mutex);
}

void drive(bridge_t *bridge, int direction)
{
    arrive(bridge, direction);
    cross(bridge);
    leave(bridge);
}

void *east(void *data)
{
    drive((bridge_t *)data, EAST);
    return NULL;
}

void *west(void *data)
{
    drive((bridge_t *)data, WEST);
    return NULL;
}

int run(int nw, int ne)
{
    int i, n = nw + ne;
    pthread_t thread[n];
    for (i = 0; i < n; i++)
        if (pthread_create(&thread[i], NULL, i < nw ? east : west, &shared_bridge))
        {
            printf("thread creation failed\n");
            return EXIT_FAILURE;
        }
    for (i = 0; i < n; i++)
        if (thread[i])
            pthread_join(thread[i], NULL);
    return EXIT_SUCCESS;
}

// after compiling use ./bridge -e X -w Y
// X: number of cars going to east
// Y: number of cars going to west
// use ./bridge -h for help
int main(int argc, char **argv)
{
    int c, nw = 1, ne = 1;
    while ((c = getopt(argc, argv, "e:w:h")) >= 0)
    {
        switch (c)
        {
        case 'e':
            if ((ne = atoi(optarg)) <= 0)
            {
                fprintf(stderr, "Number of cars going east must be greater than 0\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'w':
            if ((nw = atoi(optarg)) <= 0)
            {
                fprintf(stderr, "Number of cars going west must be greater than 0\n");
                exit(EXIT_FAILURE);
            }
            break;
        case 'h':
            printf("Usage: bridge [-e east] [-w west] [-h]\n");
            exit(EXIT_SUCCESS);
        }
    }
    return run(nw, ne);
}
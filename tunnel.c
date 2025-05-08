#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

#define N_X 5        // Nombre de bus à partir de X
#define N_Y 4        // Nombre de bus à partir de Y
#define N_TRIPS 10   // Allers-retours par bus

// Directions
#define DIR_X 0
#define DIR_Y 1
#define DIR_NONE -1

// Sémaphores et variables partagées
sem_t mutex;       // Protection des compteurs
sem_t queue_X;     // File d'attente pour X
sem_t queue_Y;     // File d'attente pour Y

int turn = DIR_NONE;  // Sens actuellement autorisé
int waiting_X = 0;    // Bus en attente côté X
int waiting_Y = 0;    // Bus en attente côté Y
int in_tunnel = 0;    // Nombre de bus dans le tunnel

// Utilitaires
int opposite(int dir) {
    return (dir == DIR_X) ? DIR_Y : DIR_X;
}

int waiting_same(int dir) {
    return (dir == DIR_X) ? waiting_X : waiting_Y;
}

int waiting_opposite(int dir) {
    return (dir == DIR_X) ? waiting_Y : waiting_X;
}

// Entrée dans le tunnel
void enter_tunnel(int direction) {
    sem_wait(&mutex);
    if (turn == DIR_NONE) {
        turn = direction;  // Premier bus fixe le sens
    }
    if (direction == turn && waiting_opposite(direction) == 0) {
        in_tunnel++;
        sem_post(&mutex);
    } else {
        if (direction == DIR_X) {
            waiting_X++;
            sem_post(&mutex);
            sem_wait(&queue_X);
            sem_wait(&mutex);
            waiting_X--;
        } else {
            waiting_Y++;
            sem_post(&mutex);
            sem_wait(&queue_Y);
            sem_wait(&mutex);
            waiting_Y--;
        }
        in_tunnel++;
        sem_post(&mutex);
    }
}

// Sortie du tunnel
void exit_tunnel() {
    sem_wait(&mutex);
    in_tunnel--;
    if (in_tunnel == 0) {
        if (waiting_opposite(turn) > 0) {
            turn = opposite(turn);  // Change de sens
            int count = waiting_same(turn);
            for (int i = 0; i < count; ++i) {
                if (turn == DIR_X) sem_post(&queue_X);
                else               sem_post(&queue_Y);
            }
        } else if (waiting_same(turn) > 0) {
            int count = waiting_same(turn);
            for (int i = 0; i < count; ++i) {
                if (turn == DIR_X) sem_post(&queue_X);
                else               sem_post(&queue_Y);
            }
        } else {
            turn = DIR_NONE;  // Tunnel libre
        }
    }
    sem_post(&mutex);
}

// Thread de bus
void* bus_thread(void* arg) {
    int bus_id = ((int*)arg)[0];
    int home = ((int*)arg)[1];
    int other = opposite(home);
    free(arg);
    int i = 1;
    while (i <= N_TRIPS) {
        // Aller
        printf("Bus %d de %s : %s->%s (Trajet %d aller)\n",
               bus_id,
               (home == DIR_X) ? "X" : "Y",
               (home == DIR_X) ? "X" : "Y",
               (home == DIR_X) ? "Y" : "X",
               i);
        enter_tunnel(home);
        usleep((rand() % 500001) + 1000000);  // 1 à 1.5s
        exit_tunnel();
        // Retour
        printf("Bus %d de %s : %s->%s (Trajet %d retour)\n",
               bus_id,
               (home == DIR_X) ? "X" : "Y",
               (home == DIR_X) ? "Y" : "X",
               (home == DIR_X) ? "X" : "Y",
               i);
        enter_tunnel(other);
        usleep((rand() % 500001) + 1000000);
        exit_tunnel();
        i++;
    }
    return NULL;
}

int main() {
    srand(time(NULL));
    pthread_t threads[N_X + N_Y];

    // Init sémaphores
    sem_init(&mutex, 0, 1);
    sem_init(&queue_X, 0, 0);
    sem_init(&queue_Y, 0, 0);

    // Création threads avec while
    int idx = 0;
    int count = 0;
    while (count < N_X) {
        int* arg = malloc(2 * sizeof(int));
        arg[0] = count + 1;
        arg[1] = DIR_X;
        pthread_create(&threads[idx++], NULL, bus_thread, arg);
        count++;
    }
    count = 0;
    while (count < N_Y) {
        int* arg = malloc(2 * sizeof(int));
        arg[0] = N_X + count + 1;
        arg[1] = DIR_Y;
        pthread_create(&threads[idx++], NULL, bus_thread, arg);
        count++;
    }

    // Attente fin threads
    int j = 0;
    while (j < idx) {
        pthread_join(threads[j++], NULL);
    }

    // Destruction sémaphores
    sem_destroy(&mutex);
    sem_destroy(&queue_X);
    sem_destroy(&queue_Y);
    return 0;
}

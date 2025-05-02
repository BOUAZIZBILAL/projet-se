#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define NB_BUS_X 5
#define NB_BUS_Y 4
#define NB_TRIPS 10

sem_t mutex;
sem_t tunnel;
int waiting_x = 0;
int waiting_y = 0;
int in_tunnel_x = 0;
int in_tunnel_y = 0;
char turn = 'X'; // Pour alterner les priorités

void enter_tunnel(char ville_depart, int bus_id) {
    int isX = (ville_depart == 'X');
    
    sem_wait(&mutex);
    if (isX) waiting_x++;
    else waiting_y++;
    
   
    while ((isX && in_tunnel_y > 0) || (!isX && in_tunnel_x > 0) || 
           ((turn != ville_depart) && ((isX && waiting_y > 0) || (!isX && waiting_x > 0)))) {
        sem_post(&mutex);
        sem_wait(&tunnel);
        sem_wait(&mutex);
    }

    if (isX) {
        waiting_x--;
        in_tunnel_x++;
    } else {
        waiting_y--;
        in_tunnel_y++;
    }

    sem_post(&mutex);
}

void exit_tunnel(char ville_depart, int bus_id) {
    int isX = (ville_depart == 'X');

    sem_wait(&mutex);
    if (isX) {
        in_tunnel_x--;
        if (in_tunnel_x == 0) {
    
            if (waiting_y > 0) turn = 'Y';
            sem_post(&tunnel);
        }
    } else {
        in_tunnel_y--;
        if (in_tunnel_y == 0) {
            if (waiting_x > 0) turn = 'X';
            sem_post(&tunnel);
        }
    }
    sem_post(&mutex);
}

void* bus(void* arg) {
    int bus_id = *((int*)arg);
    char ville = (bus_id < NB_BUS_X) ? 'X' : 'Y'; 
    
    for (int i = 1; i <= NB_TRIPS; i++) {
       
        enter_tunnel(ville, bus_id);
        printf("Bus %d de %c : %c -> %c (Trajet %d)\n", 
               bus_id, ville, ville, (ville == 'X') ? 'Y' : 'X', i);
        usleep(1000000 + rand() % 500000);
        exit_tunnel(ville, bus_id);

     
        char retour = (ville == 'X') ? 'Y' : 'X';
        enter_tunnel(retour, bus_id);
        printf("Bus %d de %c : %c -> %c (Trajet %d)\n", 
               bus_id, ville, retour, ville, i);
        usleep(1000000 + rand() % 500000);
        exit_tunnel(retour, bus_id);
    }
    pthread_exit(NULL);
}

int main() {
    pthread_t buses[NB_BUS_X + NB_BUS_Y];
    int ids[NB_BUS_X + NB_BUS_Y];       
    
    sem_init(&mutex, 0, 1);
    sem_init(&tunnel, 0, 1);
    srand(time(NULL));

    for (int i = 0; i < NB_BUS_X + NB_BUS_Y; i++) {
        ids[i] = i;
        pthread_create(&buses[i], NULL, bus, &ids[i]);
    }

    for (int i = 0; i < NB_BUS_X + NB_BUS_Y; i++) {
        pthread_join(buses[i], NULL);
    }

    sem_destroy(&mutex);
    sem_destroy(&tunnel);
    
    return 0;
}

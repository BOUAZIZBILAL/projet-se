#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

// Nombre de bus et trajets
#define NB_BUS_X    5
#define NB_BUS_Y    4
#define NB_TRAJETS  10

// Mutex pour protéger les variables partagées
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Variables de contrôle du tunnel
// sens_en_cours: -1 = libre, 0 = X->Y, 1 = Y->X
static int sens_en_cours   = -1;
static int nb_dans_tunnel  = 0;
static int attente_X       = 0;
static int attente_Y       = 0;

// Structure représentant un bus
typedef struct {
    int id;
    int ville; // 0 = X, 1 = Y
} Bus;

// Fonction d’entrée dans le tunnel
void entrer_tunnel(int ville) {
    pthread_mutex_lock(&mutex);

    // Incrémenter le compteur d'attente de ce sens
    if (ville == 0) attente_X++;
    else             attente_Y++;

    // Tant que le tunnel est occupé dans l'autre sens, on attend
    while (sens_en_cours != -1 && sens_en_cours != ville) {
        pthread_mutex_unlock(&mutex);
        usleep(10 * 1000); // 10 ms d'attente active
        pthread_mutex_lock(&mutex);
    }

    // On peut entrer : décrémenter l'attente, augmenter nb_dans_tunnel
    if (ville == 0) attente_X--;
    else             attente_Y--;

    nb_dans_tunnel++;
    sens_en_cours = ville;

    pthread_mutex_unlock(&mutex);
}

// Fonction de sortie du tunnel
void sortir_tunnel(int ville) {
    pthread_mutex_lock(&mutex);

    nb_dans_tunnel--;
    // Si plus personne dans le tunnel, on peut éventuellement changer de sens
    if (nb_dans_tunnel == 0) {
        // Si des bus attendent dans l'autre sens, on bascule
        if ((ville == 0 && attente_Y > 0) ||
            (ville == 1 && attente_X > 0)) {
            sens_en_cours = 1 - ville;
        } else {
            sens_en_cours = -1;
        }
    }

    pthread_mutex_unlock(&mutex);
}

// Routine exécutée par chaque thread bus
void* bus_routine(void* arg) {
    Bus* bus = (Bus*)arg;
    int depart = bus->ville;
    int arrivee = 1 - depart;

    for (int i = 1; i <= NB_TRAJETS; i++) {
        // Trajet Aller
        entrer_tunnel(depart);
        printf("Bus %d de Ville %c : %c -> %c (Trajet %d)\n",
               bus->id,
               depart == 0 ? 'X' : 'Y',
               depart == 0 ? 'X' : 'Y',
               arrivee  == 0 ? 'X' : 'Y',
               i);
        usleep((rand() % 500 + 1000) * 1000);

        sortir_tunnel(depart);

        // Trajet Retour
        entrer_tunnel(arrivee);
        printf("Bus %d de Ville %c : %c -> %c (Trajet %d)\n",
               bus->id,
               depart == 0 ? 'X' : 'Y',
               arrivee  == 0 ? 'X' : 'Y',
               depart   == 0 ? 'X' : 'Y',
               i);
        usleep((rand() % 500 + 1000) * 1000);

        sortir_tunnel(arrivee);
    }

    free(bus);
    return NULL;
}

int main(void) {
    srand(time(NULL));
    pthread_t threads[NB_BUS_X + NB_BUS_Y];

    // Création des bus de X
    for (int i = 0; i < NB_BUS_X; i++) {
        Bus* b = malloc(sizeof(Bus));
        b->id    = i + 1;
        b->ville = 0;
        pthread_create(&threads[i], NULL, bus_routine, b);
    }

    // Création des bus de Y
    for (int i = 0; i < NB_BUS_Y; i++) {
        Bus* b = malloc(sizeof(Bus));
        b->id    = i + 1;
        b->ville = 1;
        pthread_create(&threads[NB_BUS_X + i], NULL, bus_routine, b);
    }

    // Attente de la fin de tous les threads
    for (int i = 0; i < NB_BUS_X + NB_BUS_Y; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>

#define NB_BUS_X 5
#define NB_BUS_Y 4
#define NB_TRAJETS 10

// Sémaphores bach sncronisé
sem_t mutex;           // Protège l'accès aux variables partagées
sem_t sem_x_to_y;      // Autorise les trajets X -> Y
sem_t sem_y_to_x;      // Autorise les trajets Y -> X

// Variables partagées
int buses_in_tunnel = 0;      // Nombre de bus dans le tunnel
int waiting_x_to_y = 0;       // Nombre de bus attendant pour X -> Y
int waiting_y_to_x = 0;       // Nombre de bus attendant pour Y -> X
int direction = 0;            // Direction actuelle dans le tunnel (0: vide, 1: X->Y, -1: Y->X)

//  les paramètres type
typedef struct {
    int id;
    char origine;
} BusParams;

// Fonction qui simule un trajet dans le tunnel
void simuler_trajet() {
    // Simulation du trajet (durée aléatoire entre 1 et 1.5 secondes)
    int sleep_time = 1000 + rand() % 501;
    usleep(sleep_time * 1000);  // usleep prend des microsecondes
}

// Fonction pour entrer dans le tunnel (direction X -> Y)
void entrer_tunnel_x_to_y(int bus_id) {
    sem_wait(&mutex);
    
    // Si le tunnel est utilisé dans l'autre sens ou si des bus de Y attendent et équité nécessaire
    if (direction == -1 || (direction == 0 && waiting_y_to_x > 0 && buses_in_tunnel == 0)) {
        waiting_x_to_y++;
        sem_post(&mutex);
        sem_wait(&sem_x_to_y);  // Attendre l'autorisation
        waiting_x_to_y--;
    }
    
    direction = 1;  // Direction X -> Y
    buses_in_tunnel++;
    
    printf("Bus %d de X : X -> Y (Trajet en cours)\n", bus_id);
    
    sem_post(&mutex);
}

// Fonction pour entrer dans le tunnel (direction Y -> X)
void entrer_tunnel_y_to_x(int bus_id) {
    sem_wait(&mutex);
    
    // Si le tunnel est utilisé dans l'autre sens ou si des bus de X attendent et équité nécessaire
    if (direction == 1 || (direction == 0 && waiting_x_to_y > 0 && buses_in_tunnel == 0)) {
        waiting_y_to_x++;
        sem_post(&mutex);
        sem_wait(&sem_y_to_x);  // Attendre l'autorisation
        waiting_y_to_x--;
    }
    
    direction = -1;  // Direction Y -> X
    buses_in_tunnel++;
    
    printf("Bus %d de Y : Y -> X (Trajet en cours)\n", bus_id);
    
    sem_post(&mutex);
}

// Fonction pour sortir du tunnel (direction X -> Y)
void sortir_tunnel_x_to_y(int bus_id) {
    sem_wait(&mutex);
    
    buses_in_tunnel--;
    printf("Bus %d de X : arrivé à Y\n", bus_id);
    
    // Si c'est le dernier bus à sortir du tunnel
    if (buses_in_tunnel == 0) {
        direction = 0;  // Tunnel vide
        
        // Réveiller les bus en attente selon la politique d'équité
        if (waiting_y_to_x > 0) {
            // Autoriser tous les bus en attente dans la direction Y -> X
            for (int i = 0; i < waiting_y_to_x; i++) {
                sem_post(&sem_y_to_x);
            }
        } else if (waiting_x_to_y > 0) {
            // Autoriser tous les bus en attente dans la direction X -> Y
            for (int i = 0; i < waiting_x_to_y; i++) {
                sem_post(&sem_x_to_y);
            }
        }
    }
    
    sem_post(&mutex);
}

// Fonction pour sortir du tunnel (direction Y -> X)
void sortir_tunnel_y_to_x(int bus_id) {
    sem_wait(&mutex);
    
    buses_in_tunnel--;
    printf("Bus %d de Y : arrivé à X\n", bus_id);
    
    // Si c'est le dernier bus à sortir du tunnel
    if (buses_in_tunnel == 0) {
        direction = 0;  // Tunnel vide
        
        // Réveiller les bus en attente selon la politique d'équité
        if (waiting_x_to_y > 0) {
            // Autoriser tous les bus en attente dans la direction X -> Y
            for (int i = 0; i < waiting_x_to_y; i++) {
                sem_post(&sem_x_to_y);
            }
        } else if (waiting_y_to_x > 0) {
            // Autoriser tous les bus en attente dans la direction Y -> X
            for (int i = 0; i < waiting_y_to_x; i++) {
                sem_post(&sem_y_to_x);
            }
        }
    }
    
    sem_post(&mutex);
}
// Fonction exécutée par chaque thread de bus partant de X
void* bus_x(void* arg) {
    BusParams* params = (BusParams*)arg;
    int bus_id = params->id;
    
    for (int i = 1; i <= NB_TRAJETS; i++) {
        // Trajet X -> Y
        entrer_tunnel_x_to_y(bus_id);
        simuler_trajet();
        sortir_tunnel_x_to_y(bus_id);
        
        // Petite pause avant le retour (simulation du temps de chargement/déchargement)
        usleep(500000);  // 0.5 seconde
        
        // Trajet Y -> X
        entrer_tunnel_y_to_x(bus_id);
        simuler_trajet();
        sortir_tunnel_y_to_x(bus_id);
        
        // Petite pause avant le prochain aller-retour
        usleep(500000);  // 0.5 seconde
    }
    
    free(params);
    pthread_exit(NULL);
}

// Fonction exécutée par chaque thread de bus partant de Y
void* bus_y(void* arg) {
    BusParams* params = (BusParams*)arg;
    int bus_id = params->id;
    
    for (int i = 1; i <= NB_TRAJETS; i++) {
        // Trajet Y -> X
        entrer_tunnel_y_to_x(bus_id);
        simuler_trajet();
        sortir_tunnel_y_to_x(bus_id);
        
        // Petite pause avant le retour (simulation du temps de chargement/déchargement)
        usleep(500000);  // 0.5 seconde
        
        // Trajet X -> Y
        entrer_tunnel_x_to_y(bus_id);
        simuler_trajet();
        sortir_tunnel_x_to_y(bus_id);
        
        // Petite pause avant le prochain aller-retour
        usleep(500000);  // 0.5 seconde
    }
    
    free(params);
    pthread_exit(NULL);
}

int main() {
    // Initialisation du générateur de nombres aléatoires
    srand(time(NULL));
    
    // Initialisation des sémaphores
    sem_init(&mutex, 0, 1);
    sem_init(&sem_x_to_y, 0, 0);
    sem_init(&sem_y_to_x, 0, 0);
    
    // Création des threads pour les bus
    pthread_t bus_threads[NB_BUS_X + NB_BUS_Y];
    int thread_count = 0;
    
    // Création des bus de la ville X
    for (int i = 0; i < NB_BUS_X; i++) {
        BusParams* params = malloc(sizeof(BusParams));
        params->id = i + 1;
        params->origine = 'X';
        
        if (pthread_create(&bus_threads[thread_count++], NULL, bus_x, params) != 0) {
            perror("Erreur lors de la création d'un thread bus X");
            exit(EXIT_FAILURE);
        }
    }
    
    // Création des bus de la ville Y
    for (int i = 0; i < NB_BUS_Y; i++) {
        BusParams* params = malloc(sizeof(BusParams));
        params->id = i + 1;
        params->origine = 'Y';
        
        if (pthread_create(&bus_threads[thread_count++], NULL, bus_y, params) != 0) {
            perror("Erreur lors de la création d'un thread bus Y");
            exit(EXIT_FAILURE);
        }
    }
    
    // Attente de la fin de tous les threads
    for (int i = 0; i < thread_count; i++) {
        pthread_join(bus_threads[i], NULL);
    }
    
    // Destruction des sémaphores
    sem_destroy(&mutex);
    sem_destroy(&sem_x_to_y);
    sem_destroy(&sem_y_to_x);
    
    printf("Simulation terminée - Tous les bus ont effectué leurs trajets\n");
    
    return 0;
}

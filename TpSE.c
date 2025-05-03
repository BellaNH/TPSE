#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>
#include <time.h>

#define NB_BUS_X 5
#define NB_BUS_Y 4
#define NB_TRAJETS 10

sem_t mutex, sens_x_to_y, sens_y_to_x;
int dans_tunnel = 0;
int attente_x_to_y = 0;
int attente_y_to_x = 0;
int direction = 0; // 0 = libre, 1 = X->Y, 2 = Y->X

void sleep_random() {
    usleep((rand() % 501 + 1000) * 1000);
}

void entrer_tunnel(int sens) {
    sem_wait(&mutex);
    if (sens == 1) attente_x_to_y++;
    else attente_y_to_x++;

    while (direction != 0 && direction != sens) {
        sem_post(&mutex);
        if (sens == 1) sem_wait(&sens_x_to_y);
        else sem_wait(&sens_y_to_x);
        sem_wait(&mutex);
    }

    dans_tunnel++;
    direction = sens;
    if (sens == 1) attente_x_to_y--;
    else attente_y_to_x--;
    sem_post(&mutex);
}

void sortir_tunnel() {
    sem_wait(&mutex);
    dans_tunnel--;
    if (dans_tunnel == 0) {
        if ((direction == 1 && attente_y_to_x > 0) || (direction == 2 && attente_x_to_y > 0)) {
            int nb = (direction == 1) ? attente_y_to_x : attente_x_to_y;
            direction = 0;
            for (int i = 0; i < nb; i++) {
                if (direction == 1) sem_post(&sens_y_to_x);
                else sem_post(&sens_x_to_y);
            }
        } else if ((direction == 1 && attente_x_to_y > 0) || (direction == 2 && attente_y_to_x > 0)) {
            int nb = (direction == 1) ? attente_x_to_y : attente_y_to_x;
            for (int i = 0; i < nb; i++) {
                if (direction == 1) sem_post(&sens_x_to_y);
                else sem_post(&sens_y_to_x);
            }
        } else {
            direction = 0;
        }
    }
    sem_post(&mutex);
}

void* bus_thread(void* arg) {
    int id = *(int*)arg;
    int ville = id < NB_BUS_X ? 0 : 1;
    int sens, trajet = 1;
    for (int i = 0; i < NB_TRAJETS; i++) {
        sens = ville == 0 ? 1 : 2;
        entrer_tunnel(sens);
        printf("Bus %d de %s : %s -> %s (Trajet %d)\n", id, ville == 0 ? "X" : "Y", ville == 0 ? "X" : "Y", ville == 0 ? "Y" : "X", trajet);
        sleep_random();
        sortir_tunnel();

        sens = ville == 0 ? 2 : 1;
        entrer_tunnel(sens);
        printf("Bus %d de %s : %s -> %s (Trajet %d)\n", id, ville == 0 ? "X" : "Y", ville == 0 ? "Y" : "X", ville == 0 ? "X" : "Y", trajet);
        sleep_random();
        sortir_tunnel();
        trajet++;
    }
    free(arg);
    return NULL;
}

int main() {
    srand(time(NULL));
    pthread_t threads[NB_BUS_X + NB_BUS_Y];
    sem_init(&mutex, 0, 1);
    sem_init(&sens_x_to_y, 0, 0);
    sem_init(&sens_y_to_x, 0, 0);

    for (int i = 0; i < NB_BUS_X + NB_BUS_Y; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads[i], NULL, bus_thread, id);
    }

    for (int i = 0; i < NB_BUS_X + NB_BUS_Y; i++) {
        pthread_join(threads[i], NULL);
    }

    sem_destroy(&mutex);
    sem_destroy(&sens_x_to_y);
    sem_destroy(&sens_y_to_x);
    return 0;
}

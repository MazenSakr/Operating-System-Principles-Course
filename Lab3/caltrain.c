#include <pthread.h>
#include <stdio.h>
#include "caltrain.h"



void station_init(struct station *station) {
    station->seats_available = 0;
    station->passengers_waiting = 0;
    station->waiting_boarding = 0;
    pthread_mutex_init(&station->mutex, NULL);
    pthread_cond_init(&station->train_arrived, NULL);
    pthread_cond_init(&station->passenger_boarded, NULL);
}


void station_load_train(struct station *station, int count) {

    if (station->passengers_waiting == 0 || count == 0) return;

    pthread_mutex_lock(&station->mutex);

    station->seats_available = count;
    pthread_cond_broadcast(&station->train_arrived);

    // Wait until all passengers are on board or train is full.
    while ((station->passengers_waiting  && station->seats_available) || station->waiting_boarding) {
        pthread_cond_wait(&station->passenger_boarded, &station->mutex);
    }
    pthread_mutex_unlock(&station->mutex);
}


void station_wait_for_train(struct station *station) {
    pthread_mutex_lock(&station->mutex);

    // Signal that a passenger is waiting
    station->passengers_waiting++;

    // Wait until a train is in the station and there are enough free seats
    while (station->seats_available == 0) {
        pthread_cond_wait(&station->train_arrived, &station->mutex);
    }
    
    // Signal that a passenger has boarded
    station->seats_available--;
    station->waiting_boarding++;
    pthread_mutex_unlock(&station->mutex);
}


void station_on_board(struct station *station) {
    pthread_mutex_lock(&station->mutex);
    station->waiting_boarding--;
    station->passengers_waiting--;
    // If all passengers have boarded signal the train to leave
    if (station->waiting_boarding == 0) {
        pthread_cond_signal(&station->passenger_boarded);
    }
    pthread_mutex_unlock(&station->mutex);
}

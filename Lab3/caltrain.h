#include <pthread.h>

struct station{
    int seats_available; // Number of seats available on the train
    int passengers_waiting; // Number of passengers waiting at the station
    int waiting_boarding; // Number of passengers on board
    pthread_mutex_t mutex; // Mutex to protect shared data
    pthread_cond_t train_arrived; // Condition variable to signal when a train arrives
    pthread_cond_t passenger_boarded; // Condition variable to signal when a passenger boards
};


void station_init(struct station *station);

void station_load_train(struct station *station, int count);

void station_wait_for_train(struct station *station);

void station_on_board(struct station *station);
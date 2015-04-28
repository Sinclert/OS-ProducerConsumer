/*
 * Authors:
 *
 * Sinclert Perez (NIA: 100317201)
 * Adrian Pappalardo (NIA: 100317950)
 */

#include <stdio.h>
#include <stdlib.h>
#include "db_factory.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "factory.h"


// Object that will be inserted into the belt (belt)
struct object{
    int id;
    char name [255];
};

// Common belt for transporters and receivers
struct object belt[MAX_BELT];


/* GLOBAL VARIABLES */
int error_number = -1;
int correct_number = 0;
char names [16][10] = {"Element-0", "Element-1", "Element-2", "Element-3", "Element-4", "Element-5", "Element-6", "Element-7",
                      "Element-8", "Element-9", "Element-10", "Element-11", "Element-12", "Element-13", "Element-14", "Element-15"};

// Total number of elements to be transported and already transported at a time
int total_number = 0;

// Number of elements on the belt
int belt_elements = 0;

// Control variables to know where to exit a thread
int transported_elements = 0;
int received_elements = 0;

// Current position of receivers
int received_position = 0;

// Synchronization variables
pthread_cond_t space = PTHREAD_COND_INITIALIZER;
pthread_cond_t item = PTHREAD_COND_INITIALIZER;
pthread_cond_t write = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


// Number of threads for transport
int number_transporters = 0;
// Number of threads for insert
int number_inserters = 0;
int finished_inserters = 0;
// Number of threads for receive
int number_receivers = 0;

pthread_t * inserters;
pthread_t * transporters;
pthread_t * receivers;


int main(int argc, char ** argv){

    if (argc == 2){
		    init_factory(argv[1]);
		    close_factory();
    }

    else {
		    perror("Invalid syntax: ./factory input_file ");
		    exit(-1);
    }

    exit(0);
}



/* Function that parses the input file and initializes all structures and resources */
int init_factory(char *file){

    int i, error = 0;

    int * number_elements;
    int * number_modified_elements;
    int * number_modified_stock;


    if (file != NULL){

        FILE * filefd = fopen(file, "r");
        error = fscanf(filefd, "%d", &number_inserters);
        printf("Number inserters %d\n", number_inserters);

        if (error != 1){
            perror("Error reading from file\n");
        }

        number_transporters = 1;
        error = fscanf(filefd, "%d", &number_receivers);
        printf("Number receivers %d\n", number_receivers);

        if (error != 1){
            perror("Error reading from file\n");
        }


        /* ALLOCATION OF MEMORY */

        // Number of elements that are going to be inserted by each thread
        number_elements = (int *) malloc(sizeof(int)*number_inserters);
        // Number of elements which stock is going to be updated
        number_modified_elements = (int *) malloc(sizeof(int)*number_inserters);
        // Number of elements that are going to be added to the stock
        number_modified_stock = (int *) malloc(sizeof(int)*number_inserters);
        // Pthread_t of each inserter
        inserters = (pthread_t *) malloc(sizeof(pthread_t)*number_inserters);
        // Pthread_t of each transporter
        transporters = (pthread_t *) malloc(sizeof(pthread_t)*number_transporters);
        // Pthread_t of each receiver
        receivers = (pthread_t *) malloc(sizeof(pthread_t)*number_receivers);

        // Reading elements depending on number of inserters
        for (i = 0 ; i < number_inserters ; i++){
            error = fscanf(filefd, "%d %d %d", &number_elements[i], &number_modified_elements[i], &number_modified_stock[i]);

            if (error != 3 || number_modified_elements[i] > number_elements[i]){
                perror("Error reading from file\n");
                exit(-1);
            }
        }

        // Total number of elements that will be in the database
        for (i = 0 ; i < number_inserters ; i++){
            total_number += number_elements[i] + (number_modified_elements[i]*number_modified_stock[i]);
        }


        // Initialization of the database
        error = db_factory_init();	
        if (error != 0){
	           perror("Error when initializing the database\n");
        }

        printf("Total number of elements %d\n", total_number);

        
        /* CREATION OF THE THREADS */
        int insertion_args [number_inserters][3];

        // Inserter threads creation
        for (i = 0 ; i < number_inserters ; i++){

            // Each row of the "insertion_args" matrix stores the elements to create, modify and the stock to add of each thread
            insertion_args[i][0] = number_elements[i];
            insertion_args[i][1] = number_modified_elements[i];
            insertion_args[i][2] = number_modified_stock[i];

            pthread_create (&inserters[i], NULL, (void*) inserter, &insertion_args[i]);
        }

        // Transporter thread creation
        pthread_create (transporters, NULL, (void*) transporter, NULL);

        // Receiver threads creation
        for (i = 0 ; i < number_receivers ; i++){
            pthread_create (&receivers[i], NULL, (void*) receiver, NULL);
        }


        /* DESTRUCTION OF THE THREADS */

        // Closing inserters thread
        for (i = 0 ; i < number_inserters ; i++){
            pthread_join (inserters[i], NULL);
        }
        
        // Closing transporter thread
        pthread_join (transporters[0], NULL);

        // Closing receivers thread
        for (i = 0 ; i < number_receivers ; i++){
            pthread_join (receivers[i], NULL);
        }

        return 0;
    }

    else {
        return -1;
    }
}



/* Function that closes and free the resources used by the factory */
int close_factory(){

    int error = 0;

    free (inserters);
    free (transporters);
    free (receivers);

    // Close the database
    error = db_factory_destroy();
    if (error != 0){
        perror("Error when closing the database\n");
    }

    return 0;
}



/* Function that inserts a new element to the database */
/* Function that updates the stock of the element of the database */
/* Function executed by the inserter thread which is in charge of inserting and updating */
void * inserter(void * data){

    int i, ID, stock;
    int error = 0;
    int * dataArr = (int*) data;

    // Creation of the elements
    for (i = 0 ; i < dataArr[0] ; i++){


        /* LOCK */
        pthread_mutex_lock(&mutex);

        error = db_factory_create_element(names[i], 1, &ID);

        if (error != 0){
            perror("Error when creating the elements");
            pthread_exit(&error_number);
        }

        /* UNLOCK */
        pthread_mutex_unlock(&mutex);


        // Update the elements stock
        if (i < dataArr[1]){
            error = db_factory_get_stock(ID, &stock);

            if (error != 0){
                perror("Error when getting the stock of the elements");
                pthread_exit(&error_number);
            }

            // To calculate the new stock, the current one and the input one are added
            error = db_factory_update_stock(ID, (stock + dataArr[2]));

            if (error != 0){
                perror("Error when updating the stock of the elements");
                pthread_exit(&error_number);
            }
        }

        pthread_cond_signal(&write);
    }

    finished_inserters++;
    printf("Exitting inserter thread\n");
    pthread_exit(&correct_number);
}



/* Function executed by the thread transporter */
void * transporter(void){

  int i, status, stock;
  int ID = 0;
  int error = 0;
  int position = 0;
  char * name = malloc(255);

  while (finished_inserters < number_inserters){
      pthread_cond_wait(&write, &mutex);
  }

  while (transported_elements < total_number){
      error = db_factory_get_ready_state(ID, &status);

      if (error != 0){
          perror("Error when checking the state of the elements");
          pthread_exit(&error_number);
      }

      // If the element is ready to be transported
      if (status == 1){
          error = db_factory_get_stock(ID, &stock);

          if (error != 0){
              perror("Error when getting the stock of the elements");
              pthread_exit(&error_number);
          }

          // If it has enough stock
          while (stock > 0){


              // It waits until a signal from receiver is reached
              while (belt_elements == 1){
                  pthread_cond_wait(&space, &mutex);               
              }


              error = db_factory_get_element_name(ID, name);

              if (error != 0){
                  perror("Error when getting the name of the elements");
                  pthread_exit(&error_number);
              }

              // The id and the name of the current position object are stored
              belt[position].id = ID;
                  
              for (i = 0 ; i < 255 ; i++){
                  belt[position].name[i] = name[i];
              }

              error = db_factory_update_stock(ID, (stock-1));

              if (error != 0){
                  perror("Error when updating the stock of the elements");
                  pthread_exit(&error_number);
              }

              error = db_factory_get_stock(ID, &stock);

              if (error != 0){
                  perror("Error when getting the stock of the elements");
                  pthread_exit(&error_number);
              }

              // The variables need to be updated
              belt_elements++;
              transported_elements++;
              
              // To be printed when inserted in the belt
              printf("Introducing element %d, %s in position [%d] with %d number of elements\n", ID, name, position, belt_elements);

              position = (position+1) % MAX_BELT;

              if (transported_elements == total_number){
                  pthread_cond_broadcast(&item);
                  printf("HOLA\n");
                  printf("Exitting thread transporter\n");
                  pthread_exit(&correct_number);
              }
              // Signal sended to the receiver thread
              if (belt_elements == 1){
                  pthread_cond_signal(&item);
              }
          }

          // The ID is updated
          ID++;
      }
  }

  free (name);
  printf("Exitting thread transporter\n");
  pthread_exit(&correct_number);
}



/* Function execute by the receiver thread */
void * receiver(){

  char * name = malloc(255);
  int error = 0;

  while (received_elements < total_number){


      /* LOCK */
      pthread_mutex_lock(&mutex);
      
      // It waits until a signal from receiver is reached
      while (belt_elements == 0){
          pthread_cond_wait(&item, &mutex);

      }

      error = db_factory_get_element_name(belt[received_position].id, name);

      if (error != 0){
          perror("Error when getting the name of the elements");
          pthread_exit(&error_number);
      }

      // The variables need to be updated
      belt_elements--;
      received_elements++;

      // To be printed when an element is received
      printf("Element %d, %s has been received from position [%d] with %d number of elements\n", belt[received_position].id, name, received_position, belt_elements);

      received_position = (received_position+1) % MAX_BELT;


      // Signal sended to the transporter thread
      if (belt_elements < MAX_BELT){
          pthread_cond_signal(&space);
      }

      /* UNLOCK */
      pthread_mutex_unlock(&mutex);
  }

  free (name);
  printf("Exitting thread receiver\n");
  pthread_exit(&correct_number);
}
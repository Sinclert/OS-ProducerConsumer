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
int name = 0;
char names [16][11] = {"First", "Second", "Third", "Fourth", "Fifth", "Sixth", "Seventh", "Eighth",
                      "Ninth", "Tenth", "Eleventh", "Twelfth", "Thirteenth", "Fourteenth", "Fifteenth", "Sixteenth"};

// Total number of elements to be transported and already transported at a time
int total_number = 0;

// Number of elements on the belt
int belt_elements = 0;

// Control variables to know where to exit a thread
int created_elements = 0;
int transported_elements = 0;
int received_elements = 0;

// Current position of receivers
int receiver_position = 0;

// Synchronization variables
pthread_cond_t space = PTHREAD_COND_INITIALIZER;
pthread_cond_t item = PTHREAD_COND_INITIALIZER;
pthread_cond_t write = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;


// Number of threads for transport
int number_transporters = 0;
// Number of threads for insert
int number_inserters = 0;
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
        if (filefd == NULL){
            perror("Error opening the file\n");
            exit(-1);
        }

        error = fscanf(filefd, "%d", &number_inserters);
        if (error != 1){
            perror("Error reading from file\n");
            exit(-1);
        }

        printf("Number inserters %d\n", number_inserters);

        number_transporters = 1;

        error = fscanf(filefd, "%d", &number_receivers);
        if (error != 1){
            perror("Error reading from file\n");
            exit(-1);
        }

        printf("Number receivers %d\n", number_receivers);


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


        // Variable to check that the number of elements to insert is at most 16
        int diff_elements = 0;

        // Reading elements depending on number of inserters
        for (i = 0 ; i < number_inserters ; i++){
            error = fscanf(filefd, "%d %d %d", &number_elements[i], &number_modified_elements[i], &number_modified_stock[i]);

            if (error != 3 || number_modified_elements[i] > number_elements[i]){
                perror("Error reading from file\n");
                exit(-1);
            }
            
            // Total number of elements that will be in the database
            total_number += number_elements[i] + (number_modified_elements[i] * number_modified_stock[i]);
            diff_elements += number_elements[i];

            if (diff_elements > 16){
                printf("The maximum number of elements to introduce in the database is 16\n");
                exit(-1);
            }
        }
        
        printf("Total number of elements %d\n", total_number);

        // Initialization of the database
        error = db_factory_init();
        if (error != 0){
            perror("Error when initializing the database\n");
            exit(-1);
        }


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
        exit(-1);
    }
}



/* Function that closes and free the resources used by the factory */
int close_factory(){

    int error = 0;

    // Thread pointers released
    free (inserters);
    free (transporters);
    free (receivers);

    // Control variables destruction
    pthread_cond_destroy(&space);
    pthread_cond_destroy(&item);
    pthread_cond_destroy(&write);
    pthread_mutex_destroy(&mutex);

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

        error = db_factory_create_element(names[name], 1, &ID);

        if (error != 0){
            perror("Error when creating the elements");
            pthread_exit((void *) -1);
        }

        // This variable help us to know when the transporter must wait for the receivers
        created_elements++;
        name++;

        /* UNLOCK */
        pthread_mutex_unlock(&mutex);


        // Update the elements stock
        if (i < dataArr[1]){
            error = db_factory_get_stock(ID, &stock);

            if (error != 0){
                perror("Error when getting the stock of the elements");
                pthread_exit((void *) -1);
            }

            // To calculate the new stock, the current one and the input one are added
            error = db_factory_update_stock(ID, (stock + dataArr[2]));

            if (error != 0){
                perror("Error when updating the stock of the elements");
                pthread_exit((void *) -1);
            }

            created_elements = created_elements + dataArr[2];
        }

        pthread_cond_signal(&write);
    }

    printf("Exitting inserter thread\n");
    pthread_exit((void *) 0);
}



/* Function executed by the thread transporter */
void * transporter(void){

  int status, stock;
  int ID = 0;
  int error = 0;
  int position = 0;
  char * name = malloc(255);

  while (transported_elements < total_number){


      /* LOCK */
      pthread_mutex_lock(&mutex);

      // It waits until a signal from inserter is reached
      while (created_elements == transported_elements){
          pthread_cond_wait(&write, &mutex);
      }

      // It waits until a signal from receiver is reached
      while (belt_elements == MAX_BELT){
          pthread_cond_wait(&space, &mutex);               
      }

      /* UNLOCK */
      pthread_mutex_unlock(&mutex);


      error = db_factory_get_ready_state(ID, &status);

      if (error != 0){
          perror("Error when checking the state of the elements");
          pthread_exit((void *) -1);
      }

      // If the element is ready to be transported
      if (status == 1){
          error = db_factory_get_stock(ID, &stock);

          if (error != 0){
              perror("Error when getting the stock of the elements");
              pthread_exit((void *) -1);
          }

          // If it has enough stock and space
          while (stock > 0 && belt_elements < MAX_BELT){

              error = db_factory_get_element_name(ID, name);

              if (error != 0){
                  perror("Error when getting the name of the elements");
                  pthread_exit((void *) -1);
              }

              // The ID and the name of the current position object are stored
              belt[position].id = ID;
              strcpy(belt[position].name, name);

              error = db_factory_update_stock(ID, (stock-1));

              if (error != 0){
                  perror("Error when updating the stock of the elements");
                  pthread_exit((void *) -1);
              }

              error = db_factory_get_stock(ID, &stock);

              if (error != 0){
                  perror("Error when getting the stock of the elements");
                  pthread_exit((void *) -1);
              }


              /* LOCK */
              pthread_mutex_lock(&mutex);

              // The variables need to be updated
              belt_elements++;
              transported_elements++;
              
              // To be printed when inserted in the belt
              printf("Introducing element %d, %s in position [%d] with %d number of elements\n", ID, name, position, belt_elements);

              position = (position+1) % MAX_BELT;

              // Signal sended to the receiver thread once an element is transported
              if (belt_elements > 0){
                  pthread_cond_signal(&item);
              }

              /* UNLOCK */
              pthread_mutex_unlock(&mutex);
          }

          // The ID is updated if we went out of the loop because there is no more stock
          if (belt_elements != MAX_BELT){
              ID = (ID+1) % MAX_DATABASE;
          }
      }

      // If an uninitialized ID is found, the ID is restarted
      else {
          ID = 0;
      }
  }

  // Signal sended to the receiver thread in the last iteration
  pthread_cond_broadcast(&item);

  free (name);
  printf("Exitting thread transporter\n");
  pthread_exit((void *) 0);
}



/* Function execute by the receiver thread */
void * receiver(){

  char * name = malloc(255);
  int error = 0;
  
  while (received_elements < total_number){


      /* LOCK */
      pthread_mutex_lock(&mutex);

      // It waits until a signal from receiver is reached
      while (belt_elements == 0 && received_elements < total_number){
          pthread_cond_wait(&item, &mutex);
      }


      if (belt_elements > 0){

          error = db_factory_get_element_name(belt[receiver_position].id, name);

          if (error != 0){
              perror("Error when getting the name of the elements");
              pthread_exit((void *) -1);
          }

          // The variables need to be updated
          belt_elements--;
          received_elements++;

          // To be printed when an element is received
          printf("Element %d, %s has been received from position [%d] with %d number of elements\n", belt[receiver_position].id, name, receiver_position, belt_elements);

          receiver_position = (receiver_position+1) % MAX_BELT;


          // Signal sended to the transporter thread
          if (belt_elements == MAX_BELT-1){
              pthread_cond_signal(&space);
          }  
      }

      /* UNLOCK */
      pthread_mutex_unlock(&mutex);
  }

  free (name);
  printf("Exitting thread receiver\n");
  pthread_exit((void *) 0);
}

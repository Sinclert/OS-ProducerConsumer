#include <stdio.h>
#include <stdlib.h>
#include "db_factory.h"
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include "factory.h"

// You can add here more global variables


//Defined variables
//Object that will be inserted into the belt (belt)

struct object{
  int id;
  char name [255];
};



struct object belt[MAX_BELT]; /* common belt for transporters and receivers */



//Number of threads for transport
int number_transporters=0;
//Number of threads for insert
int number_inserters=0;
//Number of threads for receive
int number_receivers=0;

pthread_t * transporters;
pthread_t * inserters;
pthread_t * receivers;


//Total number of elements to be transported
int total_number=0;





int main(int argc, char ** argv){

  if(argc == 2){
		init_factory(argv[1]);
		close_factory();
	}else{
		perror("Invalid syntax: ./factory input_file ");
		return 0;
	}
	
  exit(0);


}

/*Function that parses the input file and initializes all structures and resources*/
int init_factory(char *file){
  int i=0;

  int error=0;

  int * number_elements;
  int * number_modified_elements;
  int * number_modified_stock;



  if(file != NULL){
    FILE * filefd = fopen(file, "r");


    error=fscanf(filefd, "%d", &number_inserters);
    printf("Number inserters %d\n", number_inserters);
    if(error != 1){
      perror("Error reading from file\n");
    }
    number_transporters=1;

    error=fscanf(filefd, "%d", &number_receivers);
    printf("Number receivers %d\n", number_receivers);
    if(error != 1){
      perror("Error reading from file\n");
    }

    //Allocation of memory
    //Number of elements that are going to be inserted by each thread
    number_elements= (int *) malloc(sizeof(int)*number_inserters);
    //Number of elements which stock is going to be updated
    number_modified_elements= (int *) malloc(sizeof(int)*number_inserters);
    //Number of eements that are going to be added to the stock
    number_modified_stock= (int *) malloc(sizeof(int)*number_inserters);
    //Pthread_t of each inserter
    inserters =(pthread_t *) malloc(sizeof(pthread_t)*number_inserters);
    //Pthread_t of each transporter
    transporters =(pthread_t *) malloc(sizeof(pthread_t)*number_transporters);
    //Pthread_t of each receiver
    receivers =(pthread_t *) malloc(sizeof(pthread_t)*number_receivers);

    //Reading elements depending on number of inserters
    for(i=0; i< number_inserters; i++){
      error= fscanf(filefd, "%d %d %d", &number_elements[i], &number_modified_elements[i], &number_modified_stock[i]);

      if(error != 3 || number_modified_elements[i] > number_elements[i]){
        perror("Error reading from file\n");
        exit(-1);
      }
    }
    //Total number of elements that will be in the database
    for(i=0; i< number_inserters; i++ ){
      total_number+=number_elements[i]+(number_modified_elements[i]*number_modified_stock[i]);
    }


    //Init the database
    
    error=db_factory_init();	
    if(error!=0){
	perror("Error when initializing the database\n");
    }

    printf("Total number of elements %d\n", total_number);

    //You can add more code here



    return 0;

  }else{
    return -1;
  }

}
/*Function that closes and free the resources used by the factory*/

int close_factory(){
  //You can add your code here
  return 0;
}



/*Function that inserts a new element to the database*/


/*Function that updates the stock of the element of the database*/


/*Function executed by the inserter thread.
 * It is in charge of inserting and updating elements
 */
void * inserter(void * data){
  //To be completed for concurrency

  printf("Exitting inserter thread\n");
  return 0;

}



/*Function executed by the thread transporter*/
void * transporter(void){
  //To be changed for concurrency
  char * name= "Example";
  int id=0;
  int number_elements=0;
  int position=0;

  //To be printed when inserted in the belt (belt)
  printf("Introducing element %d, %s in position [%d] with %d number of elements\n",id, name, position, number_elements);

  printf("Exitting thread transporter\n");

  return 0;

}


/*Function execute by the receiver thread*/
void * receiver(){
  //To be changed for concurrency
  char * name= "Example";
  int id=0;
  int number_elements=0;
  int position=0;

  //To be printed when an element is received
  printf("Element %d, %s has been received from position [%d] with %d number of elements\n",id, name, position, number_elements );

  printf("Exitting thread receiver\n");
  return 0;

}




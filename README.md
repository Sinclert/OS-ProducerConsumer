# OS-ProducerConsumer

### What is it?
The <a href="https://en.wikipedia.org/wiki/Producer–consumer_problem">Producer-Consumer problem</a> is a classic example of a multi-process synchronization problem. It consists of one or more threads producing or inserting elements into a buffer, and one or more threads consuming or taking out them from it. There could be <a href="https://en.wikipedia.org/wiki/Race_condition">race conditions</a> in case there is not a synchronization control mechanism.

### How was my implementation?
Our implementations was supposing there is a factory, having a warehouse with a number of different elements in stock (up to 16 elements). In this factory there are 3 types of workers, being each of them represented by a type of thread:<br>
<br>
<b>A) Inserter:</b>  We can create as many inserter threads as we want. Their task is to create the database elements that are going to be put in the belt by the transporter, and update the stock of those which have a stock greater than one. It is important to remember that the stocks update must be protected inside a critical section.
<br>
<b>B) Transporter:</b> We can only create one of this threads. Its task is to move the elements indicated by any Inserter, printing the information and decreasing one unit from the moved element stock. It is important to keep in mind that there could be moments when there are not free spaces in the transporter belt.
<br>
<b>C) Receiver:</b> We can create as many receivers as we want. Their task is to take out elements from the transporter belt, taking into account that there could be moments where there are no elements in the belt, waiting until some element is placed.<br>
<br>
In order to solve the synchronization problem, our approach was to use <a href="https://en.wikipedia.org/wiki/Mutual_exclusion">mutual exclusion</a> variables to control the access to the transporter belt (buffer).

### What is in the repository?

#### 1. include folder:
Folder containing two headers, one of them with the definitions of the factory.c methods, and the other with several constant definitions such as the number of positions in the belt, or the number of different elements that can be in the database.

#### 2. lib folder:
Folder containing two static libraries, that are going to be use when compiling the factory.c file. There is a x32 version and a x64 version.

#### 3. Input.txt
Example of an input file that needs to be introduced as parameter to the compiled file, in order to indicate the number of inserters, the number of receivers, and how many elements are created by each inserter.
The first number indicates te number of inserters.
The second number indicates de number of receivers.
Then, there should be as many lines as inserter threads, indicating in each of them:
  1. The number of elements created, each one with a different ID.
  2. The number of these type of elements that needs to change its stock.
  3. The stock that is going to be assigned to the specified elements.

#### 4. Makefile
File to compile the other repository files. It can be also used to remove the compiled ones.

```shell
$ make
gcc factory.c -Wall -g ./include/ -L./lib/ -ldb_factory -lpthread -o factory.exe
Example compiled successfully!
```

```shell
$ make clean
rm -f *.o *exe
Deleted files!
```

#### 5. factory.c
Main file of the entire repository, it contains the main methods of the factory (create / close), and the implementations of every kind of thread.

### Requirements:
This implementation is designed to work in a LINUX operating system. The reasons of this requirement are two: First of all a GCC (GNU Compiler-Compiler) is needed to execute the Makefile and compile the files; and secondly, because the <a href="https://en.wikipedia.org/wiki/System_call"system calls></a> could have a different name in other operating systems.


### Special considerations:

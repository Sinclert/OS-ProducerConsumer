# OS-ProducerConsumer

### What is it?
The <a href="https://en.wikipedia.org/wiki/Producer–consumer_problem">Producer-Consumer problem</a> is a classic example of a multi-process synchronization problem. It consists of one or more threads producing or inserting elements into a buffer, and one or more threads consuming or taking out them from it. There could be <a href="https://en.wikipedia.org/wiki/Race_condition">race conditions</a> in case there is not a proper synchronization control mechanism.

### How was my implementation?
My implementation was planified supposing there is a factory, having a warehouse containing different products (up to 16 different products) in stock. The number of elements in stock of a specific product is by default one, but can be updated. Additionally, there are 3 types of workers in the factory, being each of them represented by a type of thread:<br>
<br>
<b>A) Inserter:</b>  We can create as many inserters threads as we want. Their task is to initialize the database products that are going to be place in the belt by the transporter (buffer), updating the stock of those which have been indicated. Those stock updates must be protected inside a critical section.
<br>
<b>B) Transporter:</b> There will be only one of this threads. Its task is to move the products of the database (warehouse) to the belt, printing the information and decreasing one unit from the moved element stock. It is important to keep in mind that there could be moments when there are not free spaces in the transporter belt.
<br>
<b>C) Receiver:</b> We can create as many receivers as we want. Their task is to take out elements from the transporter belt, taking into account that there could be moments where there are no elements in the belt, and some wait will be required.<br>
<br>
In order to solve the synchronization problem, our approach was to use <a href="https://en.wikipedia.org/wiki/Mutual_exclusion">mutual exclusion</a> variables to control the access to the transporter belt (buffer), however, the belt is not a common array, but a circular array of 8 positions.

### What is in the repository?

#### 1. include:
Folder containing two headers, one of them with the definitions of the factory.c methods, and the other with several constant definitions such as the number of positions in the belt, or the number of different elements that can be in the database.

#### 2. lib:
Folder containing two static libraries, that are going to be use when compiling the factory.c file. There is a x32 version and a x64 version.

#### 3. Input.txt
Example of an input file that needs to be introduced as parameter to the compiled file, in order to indicate the number of inserters, the number of receivers, and the stocks of the created products.
<br>
First number: number of inserters.
<br>
Second number: number of receivers.
<br>
Then, there must be as many lines as inserter threads, indicating in each of them:
  1. The number of products created (each one will be created with one unit in stock).<br>
  2. The number of these products that needs to change its stock.<br>
  3. The stock that is going to be assigned to those specific products.

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
This implementation is designed to work in a LINUX operating system. The reasons of this requirement are two: First of all a GCC (GNU Compiler-Compiler) is needed to execute the Makefile and compile the files; and secondly, because the <a href="https://en.wikipedia.org/wiki/System_call">system calls</a> used in during the implementation, could have different names in other operating systems.


### Special considerations:

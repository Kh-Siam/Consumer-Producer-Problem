#include <iostream>
#include <iomanip>
#include <string>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <random>
#include <cstdlib>
#include <errno.h>
#include <cstring>
#include <unistd.h>
#include <time.h>
#include <math.h>

using namespace std;

struct
Commodity
{
    int currentData;
    int commodityNumber;
    double commodityPrice;
};

int bufferSize;
int SIZE;
int currentCommodity;
double price = 0;

string commodityName[] = { "ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD", "LEAD", "MENTHAOIL", "NATURALGAS", "NICKEL", "SILVER", "ZINC" };

string name;
double mean;
double deviation;
int sleepInterval;

int shmid;
int semid;
struct sembuf sem_buf[3];

struct timespec TIME;

void 
log ( string message ) 
{
    clock_gettime(  CLOCK_REALTIME, &TIME );
    tm* t = localtime(&TIME.tv_sec);
    double ns = TIME.tv_nsec / 1000000000.0;
    cerr << "[" << put_time( t, "%d/%m/%Y %H:%M:%S" ) << to_string( roundf( ns * 1000 ) / 1000 ).substr( 1, 4 ) << "]";
    cerr << " " << commodityName[currentCommodity] << ": " << message << endl;
}

void
produce () 
{
    srand( TIME.tv_nsec );
    default_random_engine generator( rand() );
    normal_distribution<double> distribution( mean, deviation );
    double temp = distribution( generator );
    price = temp < 0? temp * -1 : temp;
}

void
append ( Commodity commodity[] )
{
    char* addr = (char*) shmat( shmid, (void*) 0, 0 );
    memcpy( commodity, addr, SIZE*sizeof( struct Commodity ) );
    commodity[commodity[bufferSize].currentData].commodityNumber = currentCommodity;
    commodity[commodity[bufferSize].currentData].commodityPrice = price;
   
    commodity[bufferSize].currentData = (commodity[bufferSize].currentData + 1) % bufferSize;

    memcpy( addr, commodity, SIZE*sizeof( struct Commodity ) );
    shmdt( addr );
}


void
execute ( Commodity commodity[] )
{
    while ( true ) {
        produce();

        log( "generating a new value " + to_string( price ) );

        sem_buf[2].sem_op = -1;
        sem_buf[2].sem_flg = SEM_UNDO;
        semop( semid, &sem_buf[2], 1 );

        log( "trying to get mutex on shared buffer" );

        sem_buf[0].sem_op = -1;
        sem_buf[0].sem_flg = SEM_UNDO;
        semop( semid, &sem_buf[0], 1 );

        log( "placing " + to_string( price ) + " on shared buffer" );

        append( commodity );

        sem_buf[0].sem_op = 1;
        semop( semid, &sem_buf[0], 1 );

        sem_buf[1].sem_op = 1;
        semop( semid, &sem_buf[1], 1 );

        log( "sleeping for " + to_string( sleepInterval ) + " ms" );

        usleep( sleepInterval * 1000);
    }    
}

int
main ( int argc, char** argv ) 
{
    if ( argc < 6 )
    {
        cerr << "Wrong arguments for producer." << endl;
        cerr << "Terminating Process..." << endl;
        exit( EXIT_FAILURE );
    }
    name = argv[1]; mean = atof( argv[2] ); deviation = atof( argv[3] ); sleepInterval = atoi( argv[4] ); bufferSize = atoi( argv[5] );
    SIZE = bufferSize + 1;

    Commodity commodity[SIZE];

    currentCommodity = -1;
    for ( int i = 0; i <= 10; i++ ) 
    {
        if ( !name.compare( commodityName[i] ) )
         {
            currentCommodity = i;
            break;
        }
    }

    if ( currentCommodity == -1 )
    {
        cerr << "Commidity entered is not available." << endl;
        cerr << "Terminating Process..." << endl;
        exit( EXIT_FAILURE );
    }

    key_t key = ftok( "commodity", bufferSize );
    shmid = shmget( key, SIZE*sizeof( struct Commodity ), 0666 );

    semid = semget( key, 3, 0666 );

    sem_buf[0].sem_num = 0;
    sem_buf[1].sem_num = 1;
    sem_buf[2].sem_num = 2;
    sem_buf[0].sem_op = 0;
    sem_buf[1].sem_op = 0;
    sem_buf[2].sem_op = 0;
    sem_buf[0].sem_flg = 0;
    sem_buf[1].sem_flg = 0;
    sem_buf[2].sem_flg = 0;

    execute( commodity );

    return 0;
}
#include <iostream>
#include <iomanip>
#include <string>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <fstream>
#include <cstring>
#include <unistd.h>

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
int pointer;

string commodityName[] = { "ALUMINIUM", "COPPER", "COTTON", "CRUDEOIL", "GOLD", "LEAD", "MENTHAOIL", "NATURALGAS", "NICKEL", "SILVER", "ZINC" };
double price[11];
double previousPrice[11][4];
double average[11];
double previousAverage[11];
int num[11];

int shmid;
int semid;
struct sembuf sem_buf[3];

int 
color ( int i )
{
    if ( price[i] == previousPrice[i][num[i] % 4] ) 
        return 34;
    else if ( price[i] < previousPrice[i][num[i] % 4] )
        return 31;
    else
        return 32;
}

string
arrow ( int i ) 
{
    if ( price[i] == previousPrice[i][num[i] % 4] ) 
        return " ";
    else if ( price[i] < previousPrice[i][num[i] % 4] )
        return "↓";
    else
        return "↑";
}

int 
colorAverage ( int i )
{
    if ( average[i] == previousAverage[i] ) 
        return 34;
    else if ( average[i] < previousAverage[i] )
        return 31;
    else
        return 32;
}

string
arrowAverage ( int i ) 
{
    if ( average[i] == previousAverage[i] ) 
        return " ";
    else if ( average[i] < previousAverage[i] )
        return "↓";
    else
        return "↑";
}

void 
print () 
{
    printf("\e[1;1H\e[2J");
    string line = "+-------------------------------------+";

    cout << line << endl;
    cout << "| Currency      |  Price   | AvgPrice |" << endl;
    cout << line << endl;

    for ( int i = 0; i <= 10; i++ )
    {
        cout << "| " << setw( 14 ) << left << commodityName[i];
        cout << "| " <<  "\033[;" << color( i ) << "m" << fixed << setprecision( 2 ) << setw( 7 ) << right << price[i] << arrow( i ) << "\033[0m" << " ";
        cout << "| " <<  "\033[;" << colorAverage( i ) << "m" << fixed << setprecision( 2 ) << setw( 7 ) << right << average[i] << arrowAverage( i ) << "\033[0m" << " ";
        cout << "|" << endl;
    }

    cout << line << endl;
}

void 
update ( int i )
{
    printf( "\033[%d;19H", 4 + i );
    cout << "\033[;" << color( i ) << "m" << fixed << setprecision( 2 ) << setw( 7 ) << right << price[i] << arrow( i ) << "\033[0m";
    printf( "\033[%d;30H", 4 + i );
    cout << "\033[;" << colorAverage( i ) << "m" << fixed << setprecision( 2 ) << setw( 7 ) << right << average[i] << arrowAverage( i ) << "\033[0m";
    printf( "\033[16;1H" );
}

void
initialize ( Commodity commodity[] )
{
    for ( int i = 0; i <= 10; i++ ) 
    {
        price[i] = 0;
        for ( int j = 0; j < 4; j++ )
            previousPrice[i][j] = 0;
        average[i];
        previousAverage[i] = 0;
        num[i] = 0;
    }

    pointer = 0;

    for ( int i = 0; i < SIZE; i++ )
    {
        commodity[i].currentData = 0;
        commodity[i].commodityNumber = -1;
        commodity[i].commodityPrice = 0;
    }
    char* addr = (char*) shmat( shmid, (void*) 0, 0 );
    memcpy( addr, commodity, SIZE*sizeof( struct Commodity ) );
    shmdt( addr );
}

void 
consume ()
{
    if ( num[currentCommodity] > 0 )
    {
        if ( price[currentCommodity] != previousPrice[currentCommodity][num[currentCommodity] % 4] )
        {

            double sum = 0;
            if ( num[currentCommodity] < 5 )
            {
                for ( int j = 0; j < 4; j++ ) 
                    sum += previousPrice[currentCommodity][j];
                sum += price[currentCommodity];
                average[currentCommodity] = sum / num[currentCommodity];
            }
            else 
            {
                for ( int j = 0; j < 4; j++ ) 
                    sum += previousPrice[currentCommodity][j];
                sum += price[currentCommodity];
                average[currentCommodity] = sum / 5;
            }

            update( currentCommodity );
            
        }
    }
}

void
take ( Commodity commodity[] )
{
    char* addr = (char*) shmat( shmid, (void*) 0, 0 );
    memcpy( commodity, addr, SIZE*sizeof( struct Commodity ) );

    num[commodity[pointer].commodityNumber] ++;
    previousPrice[commodity[pointer].commodityNumber][num[commodity[pointer].commodityNumber] % 4] = price[commodity[pointer].commodityNumber];
    previousAverage[commodity[pointer].commodityNumber] = average[commodity[pointer].commodityNumber];

    price[commodity[pointer].commodityNumber] = commodity[pointer].commodityPrice;
    currentCommodity = commodity[pointer].commodityNumber;

    pointer = ( pointer + 1 ) % bufferSize;

    addr = (char*) shmat( shmid, (void*) 0, 0 );
    memcpy( addr, commodity, SIZE*sizeof( struct Commodity ) );
    shmdt( addr );
}

void 
execute ( Commodity commodity[] )
{
    while ( true ) 
    {
        sem_buf[1].sem_op = -1;
        sem_buf[1].sem_flg = SEM_UNDO;
        semop( semid, &sem_buf[1], 1 );

        sem_buf[0].sem_op = -1;
        sem_buf[0].sem_flg = SEM_UNDO;
        semop( semid, &sem_buf[0], 1 );

        take( commodity );
        cout << "Bounded-Buffer Size (number of entries);" << bufferSize << endl;

        sem_buf[0].sem_op = 1;
        semop( semid, &sem_buf[0], 1 );

        sem_buf[2].sem_op = 1;
        semop( semid, &sem_buf[2], 1 );

        consume();
    }
}

int 
main ( int argc, char** argv ) 
{
    if ( argc < 2 )
    {
        cerr << "Wrong arguments for consumer." << endl;
        cerr << "Terminating Process..." << endl;
        exit( EXIT_FAILURE );
    }
    bufferSize = atoi( argv[1] );
    SIZE = bufferSize + 1;

    Commodity commodity[SIZE];

    key_t key = ftok( "commodity", bufferSize );
    shmid = shmget( key, SIZE*sizeof( struct Commodity ), 0666|IPC_CREAT );
    initialize( commodity );
    print();

    semid = semget( key, 3, 0666|IPC_CREAT );

    sem_buf[0].sem_num = 0;
    sem_buf[1].sem_num = 1;
    sem_buf[2].sem_num = 2;
    sem_buf[0].sem_op = 0;
    sem_buf[1].sem_op = 0;
    sem_buf[2].sem_op = 0;
    sem_buf[0].sem_flg = 0;
    sem_buf[1].sem_flg = 0;
    sem_buf[2].sem_flg = 0;

    semctl( semid, 0, SETVAL, 1 );

    semctl( semid, 1, SETVAL, 0 );

    semctl( semid, 2, SETVAL, bufferSize );

    execute( commodity );

    return 0; 
}
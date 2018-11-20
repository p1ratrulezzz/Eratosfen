#include <iostream>
#include <stdio.h>
#include <cstring>
#include <algorithm>
#include <ctime>
#include "lib/sqlite/sqlite3.h"

#define LENGTH 10000000
#define uint unsigned int
#define PRIME_CACHEFILE "primecache.db"

using namespace std;

bool is_prime(uint number) {
    static sqlite3* hDB;
    static bool init = false;
    static sqlite3_stmt* hstmt_prime_check;
    static sqlite3_stmt* hstmt_prime_write;

    char* err = 0;
    int testers[] = {2, 4, 6, 8};

    // Init database cache
    if (!init) {
        init = true;
        if (sqlite3_open(PRIME_CACHEFILE, &hDB) != SQLITE_OK) {
            throw "Can\'t read file PRIME_CACHEFILE";
        }

        const char* SQL = "CREATE TABLE IF NOT EXISTS `primes` ( `number` INTEGER NOT NULL UNIQUE, `prime` TEXT, PRIMARY KEY(`number`) )";
        sqlite3_free(err);
        sqlite3_exec(hDB, SQL, 0, 0, &err);

        sqlite3_prepare_v2(hDB, "SELECT prime FROM primes WHERE number=:number", -1, &hstmt_prime_check, NULL);
        sqlite3_prepare_v2(hDB, "INSERT INTO primes(number, prime) VALUES(:number, :prime)", -1, &hstmt_prime_write, NULL);
    }


    // Check using cache
    sqlite3_reset(hstmt_prime_check);
    sqlite3_bind_int(hstmt_prime_check, 1, number);
    int status = sqlite3_step(hstmt_prime_check);
    if (status  == SQLITE_ROW) {
        const unsigned char* result = sqlite3_column_text(hstmt_prime_check, 0);
        // @fixme: Crash?
        //sqlite3_clear_bindings(hstmt_prime_check);

        return (result[0] == '1');
    }
    /*else {
        printf("ERROR inserting data: %s\n", sqlite3_errmsg(hDB));
    }*/

    bool prime = true;
    if (number != 2) {
        for (int i=0; i<4; i++) {
            if (number % testers[i] == 0) {
                prime = false;
                break;
            }
        }

        if (prime) {
            // Probably prime... let's check.
            for (uint i=9; i<number; i++) {
                if (number % i == 0) {
                    prime = false;
                    break;
                }
            }
        }
    }

    // Write to cache
    sqlite3_reset(hstmt_prime_write);
    sqlite3_bind_int(hstmt_prime_write, 1, number);
    sqlite3_bind_text(hstmt_prime_write, 2, prime ? "1" : "0", -1, SQLITE_STATIC);
    status = sqlite3_step(hstmt_prime_write);
    if (status != SQLITE_DONE && status != SQLITE_ROW) {
        printf("ERROR inserting data: %s\n", sqlite3_errmsg(hDB));
        throw "Can\'t write a new value into db";
    }

    return prime;
}

int main()
{
    clock_t begin = clock();
    // Переменная на будущее, чтобы можно было легко заменить все на динамические массивы
    uint n = LENGTH;

    // Создаем статический массив из n элементов.
    unsigned char* durshlag = new unsigned char[n];

    // Заполняем массив значениями '1' типа char: пишем в память начиная с указателя начала массива
    // Пишем ровно n байт.
    // Встроенная функция memset() сделает это куда быстрее, чем кастомный цикл for()
    // memset(durshlag, '1', n);
    // fill_n(durshlag, n, '1');

    for (uint i=0; i<n; i++) {
        durshlag[i] = '1';
    }

    for (uint i=2; i*i < n; i++) {
        uint sqi = i*i;
        if (durshlag[i] == '1') {
            for (uint j=sqi; j < n; j+=i) {
                durshlag[j] = '0';
            }
        }
    }



    clock_t end = clock();
    double elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
    begin = end;

    int primes_found = 0, primes_checked = 0;
    for (uint i=2; i<n; i++) {
        if (durshlag[i] == '1') {
            // Check if it is really prime
            primes_found++;
            if (is_prime(i)) {
                primes_checked++;
            }
            else {
                durshlag[i] = '2';
            }
        }
    }

    // Measure time for process of checking numbers
    end = clock();
    double elapsed_secs_check = double(end - begin) / CLOCKS_PER_SEC;

    uint simpleCount = 0;
    cout<<"Simple numbers from 2 to "<<n<<" are:\n";
    for (uint i=2; i<n; i++) {
        if (durshlag[i] != '0') {
            simpleCount++;
            // cout<<i<<"\n";
        }

        if (durshlag[i] == '2') {
                cout<<i<<" is not prime"<<endl;
        }
    }

    cout
        <<"Total: "<<simpleCount<<endl
        <<"Total time: "<<elapsed_secs*1000<<"ms"<<endl<<endl
        <<"Primes found: "<<primes_found<<"\nPrimes checked: "<<primes_checked<<endl
        <<"Total time: "<<elapsed_secs_check*1000<<"ms"<<endl<<endl
        <<"Press any key to exit"<<endl;

    getchar();

    // Release memory for all range taken by durshlag.
    delete[] durshlag;

    return 0;
}

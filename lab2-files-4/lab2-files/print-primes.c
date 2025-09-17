/*
 print-primes.c
 By David Broman.
 Last modified: 2015-09-15
 This file is in the public domain.
*/


#include <stdio.h>
#include <stdlib.h>

#define COLUMNS 6
int a = 0;
int j = 2;
void print_primes(int n){

  while(j <= n)
  is_prime(j);
  
}
int print_number(int n) {

  printf("%10d ", n);
  if (a % 6 == 0) {
    printf("\n");
  }
}
int is_prime(int n){
  int i;
  j++;
  for (i = 2; i <= n; i++) {
      if (n % i == 0 && n != i)  {
        return 0;
      }
  }
  a++;
  print_number(n);
}
// 'argc' contains the number of program arguments, and
// 'argv' is an array of char pointers, where each
// char pointer points to a null-terminated string.
int main(int argc, char *argv[]){
    if(argc == 2)
    {
        print_primes(atoi(argv[1]));
    }
  else
    printf("Please state an integer number.\n");
  return 0;
}

 

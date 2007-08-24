/* =========FOR INTERNAL USE ONLY. NO DISTRIBUTION PLEASE ========== */

/*********************************************************************
 Copyright 1999-2004, University of Cincinnati.  All rights reserved.
 By using this software the USER indicates that he or she has read,
 understood and will comply with the following:

 --- University of Cincinnati hereby grants USER nonexclusive permission
 to use, copy and/or modify this software for internal, noncommercial,
 research purposes only. Any distribution, including commercial sale
 or license, of this software, copies of the software, its associated
 documentation and/or modifications of either is strictly prohibited
 without the prior consent of University of Cincinnati.  Title to copyright
 to this software and its associated documentation shall at all times
 remain with University of Cincinnati.  Appropriate copyright notice shall
 be placed on all software copies, and a complete copy of this notice
 shall be included in all copies of the associated documentation.
 No right is  granted to use in advertising, publicity or otherwise
 any trademark, service mark, or the name of University of Cincinnati.


 --- This software and any associated documentation is provided "as is"

 UNIVERSITY OF CINCINNATI MAKES NO REPRESENTATIONS OR WARRANTIES, EXPRESS
 OR IMPLIED, INCLUDING THOSE OF MERCHANTABILITY OR FITNESS FOR A
 PARTICULAR PURPOSE, OR THAT  USE OF THE SOFTWARE, MODIFICATIONS, OR
 ASSOCIATED DOCUMENTATION WILL NOT INFRINGE ANY PATENTS, COPYRIGHTS,
 TRADEMARKS OR OTHER INTELLECTUAL PROPERTY RIGHTS OF A THIRD PARTY.

 University of Cincinnati shall not be liable under any circumstances for
 any direct, indirect, special, incidental, or consequential damages
 with respect to any claim by USER or any third party on account of
 or arising from the use, or inability to use, this software or its
 associated documentation, even if University of Cincinnati has been advised
 of the possibility of those damages.
*********************************************************************/

#include <stdio.h>
#include <sys/time.h>
#include "sbsat.h"

/*
 * van der Warden Number problem:
 * 
 * Consider the small example: find an arrangement of positive 
 * integers from 1 to n, into one and only one of our k buckets, 
 * such that there is *no* arithmetic progression of length p.
 * 
 * vdW(4,5)=100 (so n=100, k=4, p=5)
 * 
 * Model this with 4*100 boolean variables, 
 * vars 1 - 100 represent the integers existence in bucket 1
 * vars 101 - 200 represent the integers existence in bucket 2
 * ..
 * vars 301 - 400 represent the integers existence in bucket 4
 * 
 * * a variable is true => the integer is in the bucket
 * 
 * We need three types of cnf clauses (or just two when using 
 * our new input format):
 * 1. every integer is in one bucket
 * 
 * so clauses
 * 1 101 201 301 0
 * 2 102 202 302 0
 * ...
 * 100 200 300 400 0
 * 
 * 2. but in only one bucket (so we have to pairwise prevent 
 * this from happening in two (or more) buckets
 * 
 * // for bucket one
 * -1 -101 0
 * -1 -201 0
 * -1 -301 0
 * -101 -201 0
 * -101 -301 0
 * -201 -301 0
 * 
 * // for bucket two
 * -2 -102 0
 * -2 -202 0
 * -2 -302 0
 * -102 -202 0
 * -102 -302 0
 * -202 -302 0
 * 
 * etc
 * 
 * 3. prevent any arithmetic progression of length 5
 * 
 * for each bucket
 * for each possible progression of length 5
 * 
 * -1 -2 -3 -4 -5 0
 * -1 -3 -5 -7 -9 0
 * -1 -4 -7 -10 -13 0
 * ... 
 * -1 -21 -41 -61 -81 0
 * -1 -22 -43 -64 -85 0
 * -1 -23 -45 -67 -89 0
 * -1 -24 -47 -70 -93 0
 * -1 -25 -49 -73 -97 0
 * 
 * note: we don't need -1 -26 -51 -76 -101 0 since there is no 
 * integer 101 for placement
 * 
 * We need to do this for every progression of length 5, for every bucket
 * 
 * Our new format will eliminate the first two types of cnf 
 * clauses and replace it with a nice compact representation. 
 * 
 * If you can think of a way to condense #3 type clauses into a 
 * nice state machine, I would love to hear it.
 */
#define CNF  0
#define XCNF 1
#define ITE  2
#define CNF2 3
#define MINXCNF 4
#define ITE2  5
#define XITE2  6
#define XCNF2 7
#define CNF2MK 8
#define CNF2POSONLY 9
#define CNF2INFS 10 
#define CNF2BYSTEP 11 
#define CNF2PROGDETECT_BLK 12 
#define CNF2PROGDETECT_V 13 
#define CNF2PROGDETECT_MACROS 14
#define CNF2PROGDETECT_INFO 15

char name[128];
int formula_type = XCNF;

void print_par(FILE *fout, char *slit, char *delim, int n) ;

char *var(int n, int x, int k) { 
   int i = k*n+x;
   if (formula_type != ITE)
      sprintf(name, "%d", i);
   else
      sprintf(name, "b%dv%d", k, x);
   return name;
}

void vanDerWaerden(char *vdw_type, int n, int k, int p) {
   if (!strcmp(vdw_type, "cnf")) formula_type = CNF; else 
   if (!strcmp(vdw_type, "xcnf")) formula_type = XCNF; else 
   if (!strcmp(vdw_type, "xcnf2")) formula_type = XCNF2; else 
   if (!strcmp(vdw_type, "cnf2")) formula_type = CNF2; else 
   if (!strcmp(vdw_type, "cnf2mk")) formula_type = CNF2MK; else 
   if (!strcmp(vdw_type, "cnf2posonly")) formula_type = CNF2POSONLY; else 
   if (!strcmp(vdw_type, "cnf2infs")) formula_type = CNF2INFS; else 
   if (!strcmp(vdw_type, "cnf2bystep")) formula_type = CNF2BYSTEP; else 
   if (!strcmp(vdw_type, "cnf2pdblk")) formula_type = CNF2PROGDETECT_BLK; else
   if (!strcmp(vdw_type, "cnf2pdv")) formula_type = CNF2PROGDETECT_V; else
   if (!strcmp(vdw_type, "cnf2pdmacros")) formula_type = CNF2PROGDETECT_MACROS; else
   if (!strcmp(vdw_type, "cnf2pdinfo")) formula_type = CNF2PROGDETECT_INFO; else
   if (!strcmp(vdw_type, "ite")) formula_type = ITE; else 
   if (!strcmp(vdw_type, "ite2")) formula_type = ITE2; else 
   if (!strcmp(vdw_type, "xite2")) formula_type = XITE2; else 
   if (!strcmp(vdw_type, "minxcnf")) formula_type = MINXCNF; else {
      fprintf(stderr, "Unknown vdw formula type\n");
      exit(1);
   }

   // clauses max step = (n-1)/(p-1)  [n is 1 based and k is 1 based]
   // clauses with step 1 = n-p+1
   // clauses with step 2 = n-p*2+2
   // clauses with step 3 = n-p*3+3
   // clauses with step 4 = n-p*4+4
   // prog_sum = (1+max_step)*max_step/2
   // number of regressions = max_step * n - max_step * p * prog_sum + prog_sum
   // and make it for each bucket = k*number_of_regressions
   int max_step = (n-1)/(p-1); // max sure this is integer
   int prog_sum = ((1+max_step)*max_step)/2;
   int clauses = n+k*(max_step*n-p*prog_sum+prog_sum);
   int clause_count = 0;
   //int sym = 0;
   int sym_clauses = 0;
   switch (formula_type) {
    case CNF: 
      clauses += n*k*(k-1)/2;
      fprintf(stdout, "p cnf %d %d\n", n*k, clauses);
      break;
    case XCNF: 
      fprintf(stdout, "p cnf %d %d\n", n*k, clauses);
      break;
    case ITE: 
      fprintf(stdout, "p bdd %d %d\n", n*k, 1+clauses);
      //fprintf(stdout, "initial_branch(");
      //for(int x=1; x<=n; x++)
      //   fprintf(stdout, "%s ", var(n, x, 0));
      //fprintf(stdout, ")\n");
      break;
    case XCNF2:
      if (k != 2) {
         fprintf(stderr, "Can't use XCNF2 for any other k but 2\n");
         exit(1);
      }
      clauses -= n;
      clauses /= 2; 
      fprintf(stdout, "p cnf %d %d\n", n, clauses);
      break;
    case CNF2BYSTEP:
      break;
    case CNF2INFS:
      break;
    case CNF2PROGDETECT_BLK:
      break;
    case CNF2PROGDETECT_V:
      break;
    case CNF2PROGDETECT_MACROS:
      break;
    case CNF2PROGDETECT_INFO:
      break;    
    case CNF2POSONLY:
      if (k != 2) {
         fprintf(stderr, "Can't use CNF2 for any other k but 2\n");
         exit(1);
      }
      clauses -= n;
      clauses /= 2;
      // fprintf(stdout, "p cnf %d %d\n", n, clauses+sym_clauses);
      break;
    case CNF2MK:
      /*
      sym = n/(p-1); //mk version
      if (sym*(p-1) != n) { fprintf(stderr, "Can't do it properly\n"); exit(1); }
      sym_clauses = (sym)*(p-2)*2;
      sym_clauses += - 2*(p-2) - ((sym-1)/11)*2*(p-2);
      */
    case CNF2:
      if (k != 2) {
         fprintf(stderr, "Can't use CNF2 for any other k but 2\n");
         exit(1);
      }
      clauses -= n;
      fprintf(stdout, "p cnf %d %d\n", n, clauses+sym_clauses);
      break;
    case ITE2: 
      if (k != 2) {
         fprintf(stderr, "Can't use ITE2 for any other k but 2\n");
         exit(1);
      }
      clauses -= n;
      fprintf(stdout, "p bdd %d %d\n", n, clauses+sym_clauses);
      break;
    case XITE2: 
      if (k != 2) {
         fprintf(stderr, "Can't use XITE2 for any other k but 2\n");
         exit(1);
      }
      clauses -= n;
      clauses /= 2;
      fprintf(stdout, "p bdd %d %d\n", n, clauses+sym_clauses);
      break;
    case MINXCNF:
      clauses -= n;
      clauses /= k;
      clauses++;
      fprintf(stdout, "p cnf %d %d\n", n, clauses);
      break;
    default: 
      exit(1);
   }

   if (formula_type == CNF || formula_type == XCNF)
      fprintf(stdout, "c max_step %d\n", max_step);

   // Problem:
   // n integer, k buckets, n*k variables

   // 1. 
   if (formula_type == CNF || formula_type == XCNF)
      fprintf(stdout, "c every integer only in one bucket\n");
   for(int x = 1; x <= n; x++) {
      int bucket;
      switch (formula_type) {
       case CNF: {
          // at least one bucket
          fprintf(stdout, "%s ", var(n, x, 0));
          for(bucket = 1; bucket < k; bucket++)
             fprintf(stdout, "%s ", var(n, x, bucket));
          fprintf(stdout, "0\n");
          clause_count++;
          // at most one bucket
          for(bucket = 0; bucket < k; bucket++)
             for (int bucket2=bucket+1; bucket2 < k; bucket2++) {
                fprintf(stdout, "-%s ", var(n, x, bucket));
                fprintf(stdout, "-%s 0\n", var(n, x, bucket2));
                clause_count++;
             }
       } break;
       case XCNF: {
         fprintf(stdout, "#1 [ %s ", var(n, x, 0));
         for(bucket = 1; bucket < k; bucket++)
            fprintf(stdout, "%s ", var(n, x, bucket));
         fprintf(stdout, "] 1\n");
         clause_count++;
       } break;
       case ITE: {
          // at least one bucket
          fprintf(stdout, "* AND%d( OR%d(%s ", 1+k*(k-1)/2, k, var(n, x, 0));
          for(bucket = 1; bucket < k; bucket++)
             fprintf(stdout, "%s ", var(n, x, bucket));
          clause_count++;
          fprintf(stdout, ") ");
          // at most one bucket
          for(bucket = 0; bucket < k; bucket++) {
             for (int bucket2=bucket+1; bucket2 < k; bucket2++) {
                fprintf(stdout, "OR(-%s  ", var(n, x, bucket));
                fprintf(stdout, "-%s) ", var(n, x, bucket2));
                clause_count++;
             }
          }
          fprintf(stdout, ")\n");
       } break;
       case MINXCNF: {
          /* nothing */
      } break;
      }
  }

   if (formula_type == CNF || formula_type == XCNF) {
      fprintf(stdout, "c prevent any arithmetic progression of length %d for every bucket %d\n", p, k);
      for(int bucket=0; bucket<k; bucket++) {
         for(int num = 1; num <= n; num++) {
            for(int step = 1; 1; step++) {
               if (step * (p-1) + num > n) break;
               int base = num;
               for(int z = 0; z < p; z++) {
                  fprintf(stdout, "-%s ", var(n, base, bucket));
                  base+=step;
               }
               fprintf(stdout, "0\n");
               clause_count++;
            }
         }
      }
      if (clause_count != clauses) {
         fprintf(stderr, "======================== Problem\n");
      }
   } else if (formula_type == ITE) {
      fprintf(stdout, "; prevent any arithmetic progression of length %d for every bucket %d\n", p, k);
      for(int bucket=0; bucket<k; bucket++) {
         for(int num = 1; num <= n; num++) {
//#define MK_TEST
#ifdef MK_TEST
            if ((n-num)/(p-1) == 0) continue;
            fprintf(stdout, "* AND%d(", (n-num)/(p-1));
#endif
            for(int step = 1; 1; step++) {
               if (step * (p-1) + num > n) break;
               int base = num;
#ifdef MK_TEST
               fprintf(stdout, " OR%d( ", p);
#else
               fprintf(stdout, "* OR%d( ", p);
#endif
               for(int z = 0; z < p; z++) {
                  fprintf(stdout, "-%s ", var(n, base, bucket));
                  base+=step;
               }
#ifdef MK_TEST
               fprintf(stdout, ")");
#else
               fprintf(stdout, ")\n");
#endif
               clause_count++;
            }
#ifdef MK_TEST
            fprintf(stdout, ")\n");
#endif
         }
      }
   } else if (formula_type == ITE2) {
      fprintf(stdout, "; prevent any arithmetic progression of length %d for every bucket %d\n", p, k);
      for(int num = 1; num <= n; num++) {
         for(int step = 1; 1; step++) {
            if (step * (p-1) + num > n) break;
            int base = num;
            fprintf(stdout, "* OR%d( ", p);
            for(int z = 0; z < p; z++) {
               fprintf(stdout, "-%s ", var(n, base, 0));
               base+=step;
            }
            fprintf(stdout, ")\n");
            clause_count++;
            base = num;
            fprintf(stdout, "* OR%d( ", p);
            for(int z = 0; z < p; z++) {
               fprintf(stdout, "%s ", var(n, base, 0));
               base+=step;
            }
            fprintf(stdout, ")\n");
            clause_count++;
         }
      }
      if (clause_count != clauses) {
         fprintf(stderr, "======================== Problem\n");
      }
   } else if (formula_type == CNF2INFS) {
      int y_count[n+1];

      char filename[100];
//ib.h
      sprintf(filename, "ib.h");
      FILE *fout = fopen(filename, "w");
      
      fprintf(fout, "void ib(");
      for(int x = 0; x < n/64; x++)
		  fprintf(fout, "int64_t B%d, ", x);
		fprintf(fout, "int64_t C0");
      for(int x = 1; x < n/64; x++)
		  fprintf(fout, ", int64_t C%d", x);
      fprintf(fout, ", int8_t *out);\n");
      fclose(fout);
//ib.c
      sprintf(filename, "ib.c");
      fout = fopen(filename, "w");
      
      fprintf(fout, "void ib(");
      for(int x = 0; x < n/64; x++)
		  fprintf(fout, "int64_t B%d, ", x);
		fprintf(fout, "int64_t C0");
      for(int x = 1; x < n/64; x++)
		  fprintf(fout, ", int64_t C%d", x);
      fprintf(fout, ") {\n");
      
      fprintf(fout, "  int8_t v0");
      for(int x = 1; x < n; x++)
		  fprintf(fout, ", v%d", x);
      fprintf(fout, ";\n");

      fprintf(fout, "  int8_t y_1");
      for(int x = 2; x <= n; x++)
		  fprintf(fout, ", y_%d", x);
      fprintf(fout, ";\n");
		
		for(int i=1;i<=n;i++) y_count[i]=0;
      for(int step = 1; step<=(n-1)/(p-1); step++) {
         for(int inf = 1; inf <= n; inf++) {
            // at most inf - step*(p-1), inf + step*(p-1)
            int count=0;
            for(int num = inf - step*(p-1); num <= inf+step*(p-1); num+=step) {
               if (num<1 || num==inf || num>n) continue;
               count++;
            }
            if (count < (p-1)) continue;
            for(int num = inf - step*(p-1); num <= inf+step*(p-1); num+=step) {
               if (num<1 || num==inf || num>n) continue;
            }
            y_count[inf]++;
			}
		}
		
		for(int inf = 1; inf <= n; inf++) {
         if (y_count[inf]) {
            fprintf(fout, "  int8_t ");
            for(int i=1;i<=y_count[inf];i++) {
               if (i>1) fprintf(fout, ", ");
               fprintf(fout, "y_%d_%d", inf, i);
            }
            fprintf(fout, ";\n");
         }
      }
		
//fanout
		
      for(int x = 0; x < n; x++) 
		  fprintf(fout, "  v%d = B%d&((int64_t) 1<<%d);\n", x, x/64, x%64);

      for(int i=1;i<=n;i++) y_count[i]=0;
      for(int step = 1; step<=(n-1)/(p-1); step++) {
         for(int inf = 1; inf <= n; inf++) {
            // at most inf - step*(p-1), inf + step*(p-1)
            int count=0;
            for(int num = inf - step*(p-1); num <= inf+step*(p-1); num+=step) {
               if (num<1 || num==inf || num>n) continue;
               count++;
            }
            if (count < (p-1)) continue;
            fprintf(fout, "  prog_detect_%dL%d(", count, p-1);
            for(int num = inf - step*(p-1); num <= inf+step*(p-1); num+=step) {
               if (num<1 || num==inf || num>n) continue;
               fprintf(fout, "v%s, ", var(n, num-1, 0));
            }
            clause_count++;
            y_count[inf]++;
            fprintf(fout, "&y_%d_%d);\n", inf, y_count[inf]);
         }
      }
      for(int inf = 1; inf <= n; inf++) {
         if (y_count[inf]) {
            char y[10];
            sprintf(y, "y_%d_%%d", inf);
            fprintf(fout, "  if");
            print_par(fout, y, "|", y_count[inf]);
            fprintf(fout, " y_%d=1; else y_%d=0;\n", inf, inf);
         }
      }
	
      for(int x = 1; x <= n;) {
			fprintf(fout, "  C%d = ((int64_t) y_%d&1)", x/64, x);
			for(int y = 1; y < 64; y++) {
				x++;
				if(x > n) break;
				fprintf(fout, " | (((int64_t) y_%d&1)<<%d)", x, y);
			}
			fprintf(fout, "\n");
		}
		fprintf(fout, "}\n");
      fclose(fout);
   } else if (formula_type == CNF2POSONLY) {
      for(int num = 1; num <= n; num++) {
         for(int step = 1; 1; step++) {
            if (step * (p-1) + num > n) break;
            int base = num;
            fprintf(stdout, "my_operator(");
            for(int z = 0; z < p; z++) {
               fprintf(stdout, "v%s, ", var(n, base, 0));
               base+=step;
            }
            clause_count++;
            fprintf(stdout, "&y%d);\n", clause_count);
         }
      }
   } else if (formula_type == CNF2BYSTEP) {
      char filename[100];
//cd.h
      sprintf(filename, "cd.h");
      FILE *fout = fopen(filename, "w");
      
      fprintf(fout, "void cd(");
      for(int x = 0; x <= n/64; x++)
		  fprintf(fout, "int64_t B%d, ", x);
      fprintf(fout, "int8_t *out);\n");
      fclose(fout);
//cd.c
      sprintf(filename, "cd.c");
      fout = fopen(filename, "w");
      
      fprintf(fout, "void cd(");
      for(int x = 0; x <= n/64; x++)
		  fprintf(fout, "int64_t B%d, ", x);
      fprintf(fout, "int8_t *out) {\n");
      
      fprintf(fout, "  int8_t v0");
      for(int x = 1; x < n; x++)
		  fprintf(fout, ", v%d", x);
      fprintf(fout, ";\n");
		
      fprintf(fout, "  int8_t y0");
      int y = 1;
      for(int step = 1; step<=(n-1)/(p-1); step++)
		  for(int start = 1; start <= step; start++) {
			  fprintf(fout, ", y%d", y++);
		  }
      fprintf(fout, ";\n");
      
//fanout
		
      for(int x = 0; x < n; x++) 
		  fprintf(fout, "  v%d = B%d&((int64_t) 1<<%d);\n", x, x/64, x%64);
		
      for(int step = 1; step<=(n-1)/(p-1); step++) {
         for(int start = 1; start <= step; start++) {
            if (start+(p-1)*step > n) break;
            int lit_count=0;
            for(int num = start; num <= n; num+=step) {
               lit_count++;
            }
            clause_count++;
            fprintf(fout, "  prog_detect_%dL%d(",lit_count,p);
            for(int num = start; num <= n; num+=step) {
               fprintf(fout, "v%s, ", var(n, num-1, 0));
            }
				fprintf(fout, "&y%d);\n", clause_count);
         }
      }
      fprintf(fout, "  if \n");
      print_par(fout, "y%d", "|", clause_count);
      fprintf(fout, "\n");
      fprintf(fout, "    (*out) = 1;\n");
      fprintf(fout, "  else (*out) = 0;\n");
      fprintf(fout, "}\n");
      fclose(fout);
   } else if (formula_type == CNF2PROGDETECT_BLK) {
      int **saveit = (int **)calloc(1000, sizeof(int *));
      for(int x = 0; x < 1000; x++)
		  saveit[x] = (int *)calloc(100, sizeof(int));
      char filename[100];
      sprintf(filename, "blk.v");
      FILE *fout = fopen(filename, "w");      
      
      for(int step = 1; step<=(n-1)/(p-1); step++) {
         for(int start = 1; start <= step; start++) {
            if (start+(p-1)*step > n) break;
            int lit_count=0;
            for(int num = start; num <= n; num+=step) {
               lit_count++;
            }
				if(lit_count >=  1000 || p >= 100) {
					fprintf(stderr, "Sean was lazy - increase bounds in vanDerWaerden.cc@CNF2PROGDETECT_BLK -- saveit[][]\n");
					exit(0);
				}
				
				if(saveit[lit_count][p] == 0) {
					saveit[lit_count][p] = 1;
					fprintf(fout, "module PROG_DETECT_%dL%d(",lit_count,p);
					for(int x = 0; x < lit_count; x++)
					  fprintf(fout, "v%d, ", x);
					fprintf(fout, "OUT, CLK)\n");
					for(int x = 0; x < lit_count; x++)
					  fprintf(fout, "    input [0:0] v%d;\n", x);
					fprintf(fout, "    output [0:0] OUT;\n    input CLK;\n endmodule\n\n");
//p-1
					fprintf(fout, "module PROG_DETECT_%dL%d(",lit_count,p-1);
					for(int x = 0; x < lit_count; x++)
					  fprintf(fout, "v%d, ", x);
					fprintf(fout, "OUT, CLK)\n");
					for(int x = 0; x < lit_count; x++)
					  fprintf(fout, "    input [0:0] v%d;\n", x);
					fprintf(fout, "    output [0:0] OUT;\n    input CLK;\n endmodule\n\n");
				}
			}
      }
      fclose(fout);
      for(int x = 0; x < 1000; x++)
		  free(saveit[x]);
      free(saveit);
   } else if (formula_type == CNF2PROGDETECT_V) {
      int **saveit = (int **)calloc(1000, sizeof(int *));
      for(int x = 0; x < 1000; x++)
		  saveit[x] = (int *)calloc(100, sizeof(int));
      char filename[100];
      
      for(int step = 1; step<=(n-1)/(p-1); step++) {
         for(int start = 1; start <= step; start++) {
            if (start+(p-1)*step > n) break;
            int lit_count=0;
            for(int num = start; num <= n; num+=step) {
               lit_count++;
            }
				if(lit_count >=  1000 || p >= 100) {
					fprintf(stderr, "Sean was lazy - increase bounds in vanDerWaerden.cc@CNF2PROGDETECT_V -- saveit[][]\n");
					exit(0);
				}
				
				if(saveit[lit_count][p] == 0) {
					saveit[lit_count][p] = 1;
					sprintf(filename, "PROG_DETECT_%dL%d.v", lit_count, p);
					FILE *fout = fopen(filename, "w");
					fprintf(fout, "module PROG_DETECT_%dL%d(",lit_count,p);
					for(int x = 0; x < lit_count; x++)
					  fprintf(fout, "v%d, ", x);
					fprintf(fout, "OUT, CLK);\n");
					for(int x = 0; x < lit_count; x++)
					  fprintf(fout, "    input [0:0] v%d;\n", x);
					fprintf(fout, "    output [0:0] OUT;\n    input CLK;\n    reg[0:0] OUT;\n");
					fprintf(fout, "    always @ (posedge CLK) begin\n");
					fprintf(fout, "         OUT <= ");
					
					for(int x = 0; x < lit_count - (p-1); x++) {
						if(x != 0) fprintf(fout, " | ");
						fprintf(fout, "(v%d", x);
						for(int y = 1; y < p; y++)
						  fprintf(fout, " & v%d", x+y);
						fprintf(fout, ")");
					}
					
					fprintf(fout, ";\n        end\n");
					fprintf(fout, "endmodule\n");
					fclose(fout);
//p-1
					sprintf(filename, "PROG_DETECT_%dL%d.v", lit_count, p-1);
					fout = fopen(filename, "w");
					fprintf(fout, "module PROG_DETECT_%dL%d(",lit_count,p-1);
					for(int x = 0; x < lit_count; x++)
					  fprintf(fout, "v%d, ", x);
					fprintf(fout, "OUT, CLK);\n");
					for(int x = 0; x < lit_count; x++)
					  fprintf(fout, "    input [0:0] v%d;\n", x);
					fprintf(fout, "    output [0:0] OUT;\n    input CLK;\n    reg[0:0] OUT;\n");
					fprintf(fout, "    always @ (posedge CLK) begin\n");
					fprintf(fout, "         OUT <= ");
					
					for(int x = 0; x < lit_count - (p-2); x++) {
						if(x != 0) fprintf(fout, " | ");
						fprintf(fout, "(v%d", x);
						for(int y = 1; y < p-1; y++)
						  fprintf(fout, " & v%d", x+y);
						fprintf(fout, ")");
					}
					
					fprintf(fout, ";\n        end\n");
					fprintf(fout, "endmodule\n");
					fclose(fout);
				}
			}
      }
      for(int x = 0; x < 1000; x++)
		  free(saveit[x]);
      free(saveit);
   } else if (formula_type == CNF2PROGDETECT_MACROS) {
      int **saveit = (int **)calloc(1000, sizeof(int *));
      for(int x = 0; x < 1000; x++)
		  saveit[x] = (int *)calloc(100, sizeof(int));
      
      for(int step = 1; step<=(n-1)/(p-1); step++) {
         for(int start = 1; start <= step; start++) {
            if (start+(p-1)*step > n) break;
            int lit_count=0;
            for(int num = start; num <= n; num+=step) {
               lit_count++;
            }
				if(lit_count >=  1000 || p >= 100) {
					fprintf(stderr, "Sean was lazy - increase bounds in vanDerWaerden.cc@CNF2PROGDETECT_MACROS -- saveit[][]\n");
					exit(0);
				}
				
				if(saveit[lit_count][p] == 0) {
					saveit[lit_count][p] = 1;
					fprintf(stdout, " my_macro/PROG_DETECT_%dL%d.v",lit_count,p);
					//p-1
					fprintf(stdout, " my_macro/PROG_DETECT_%dL%d.v",lit_count,p-1);
				}
			}
      }
      for(int x = 0; x < 1000; x++)
		  free(saveit[x]);
      free(saveit);
   } else if (formula_type == CNF2PROGDETECT_INFO) {
      int **saveit = (int **)calloc(1000, sizeof(int *));
      for(int x = 0; x < 1000; x++)
		  saveit[x] = (int *)calloc(100, sizeof(int));
      char filename[100];
      sprintf(filename, "info");
      FILE *fout = fopen(filename, "w");

      
      for(int step = 1; step<=(n-1)/(p-1); step++) {
         for(int start = 1; start <= step; start++) {
            if (start+(p-1)*step > n) break;
            int lit_count=0;
            for(int num = start; num <= n; num+=step) {
               lit_count++;
            }
    if(lit_count >=  1000 || p >= 100) {
       fprintf(stderr, "Sean was lazy - increase bounds in vanDerWaerden.cc@CNF2PROGDETECT_MACROS -- saveit[][]\n");
       exit(0);
    }
    
    if(saveit[lit_count][p] == 0) {
       saveit[lit_count][p] = 1;
       fprintf(fout, "BEGIN_DEF \"prog_detect_%dL%d\"\n", lit_count, p);
       fprintf(fout, "    MACRO = \"PROG_DETECT_%dL%d\";\n", lit_count, p);
       fprintf(fout, "    STATEFUL = NO;\n");
       fprintf(fout, "    EXTERNAL = NO;\n");
       fprintf(fout, "    PIPELINED = YES;\n");
       fprintf(fout, "    LATENCY = 1;\n\n");
       fprintf(fout, "    INPUTS = %d:\n", lit_count);
       for(int x = 0; x < lit_count; x++)
			fprintf(fout, "      I%d = INT 8 BITS (v%d[0:0])\n", x, x);
       
       fprintf(fout, ";\n    OUTPUTS = 1:\n      O0 = INT 8 BITS (OUT[0:0]);\n\n");
       fprintf(fout, "    IN_SIGNAL : 1 BITS \"CLK\" = \"CLOCK\";\n\n");
       fprintf(fout, "    DEBUG_HEADER = #\n");
       fprintf(fout, "        void prog_detect_%dL%d__dbg (", lit_count, p);
       for(int x = 0; x < lit_count; x++)
			fprintf(fout, "int8_t v%d, ", x);
       fprintf(fout, "int8_t *res);\n");
       fprintf(fout, "    #;\n");
       fprintf(fout, "    DEBUG_FUNC = #\n");
       fprintf(fout, "        void prog_detect_%dL%d__dbg (", lit_count, p);
       for(int x = 0; x < lit_count; x++)
			fprintf(fout, "int8_t v%d, ", x);
       fprintf(fout, "int8_t *res) {\n");
       fprintf(fout, "            *res = ");
       for(int x = 0; x < lit_count - (p-1); x++) {
			 if(x != 0) fprintf(fout, " | ");
			 fprintf(fout, "((v%d&1)", x);
			 for(int y = 1; y < p; y++)
				fprintf(fout, " & (v%d&1)", x+y);
			 fprintf(fout, ")");
       }

       fprintf(fout, ";\n            }\n    #;\n");
       fprintf(fout, "END_DEF\n\n");
//p-1
       fprintf(fout, "BEGIN_DEF \"prog_detect_%dL%d\"\n", lit_count, p-1);
       fprintf(fout, "    MACRO = \"PROG_DETECT_%dL%d\";\n", lit_count, p-1);
       fprintf(fout, "    STATEFUL = NO;\n");
       fprintf(fout, "    EXTERNAL = NO;\n");
       fprintf(fout, "    PIPELINED = YES;\n");
       fprintf(fout, "    LATENCY = 1;\n\n");
       fprintf(fout, "    INPUTS = %d:\n", lit_count);
       for(int x = 0; x < lit_count; x++)
			fprintf(fout, "      I%d = INT 8 BITS (v%d[0:0])\n", x, x);
       
       fprintf(fout, ";\n    OUTPUTS = 1:\n      O0 = INT 8 BITS (OUT[0:0]);\n\n");
       fprintf(fout, "    IN_SIGNAL : 1 BITS \"CLK\" = \"CLOCK\";\n\n");
       fprintf(fout, "    DEBUG_HEADER = #\n");
       fprintf(fout, "        void prog_detect_%dL%d__dbg (", lit_count, p-1);
       for(int x = 0; x < lit_count; x++)
			fprintf(fout, "int8_t v%d, ", x);
       fprintf(fout, "int8_t *res);\n");
       fprintf(fout, "    #;\n");
       fprintf(fout, "    DEBUG_FUNC = #\n");
       fprintf(fout, "        void prog_detect_%dL%d__dbg (", lit_count, p-1);
       for(int x = 0; x < lit_count; x++)
			fprintf(fout, "int8_t v%d, ", x);
       fprintf(fout, "int8_t *res) {\n");
       fprintf(fout, "            *res = ");
       for(int x = 0; x < lit_count - (p-2); x++) {
			 if(x != 0) fprintf(fout, " | ");
			 fprintf(fout, "((v%d&1)", x);
			 for(int y = 1; y < p-1; y++)
				fprintf(fout, " & (v%d&1)", x+y);
			 fprintf(fout, ")");
       }

       fprintf(fout, ";\n            }\n    #;\n");
       fprintf(fout, "END_DEF\n\n");
    }
 }
      }
      fclose(fout);
      for(int x = 0; x < 1000; x++)
		  free(saveit[x]);
      free(saveit);
   } else if (formula_type == CNF2 || formula_type == CNF2MK) {
      fprintf(stdout, "c prevent any arithmetic progression of length %d for every bucket %d\n", p, k);
      for(int num = 1; num <= n; num++) {
         for(int step = 1; 1; step++) {
            if (step * (p-1) + num > n) break;
            int base = num;
            for(int z = 0; z < p; z++) {
               fprintf(stdout, "-%s ", var(n, base, 0));
               base+=step;
            }
            fprintf(stdout, "0\n");
            clause_count++;
            base = num;
            for(int z = 0; z < p; z++) {
               fprintf(stdout, "%s ", var(n, base, 0));
               base+=step;
            }
            fprintf(stdout, "0\n");
            clause_count++;
         }
      }
      if (clause_count != clauses) {
         fprintf(stderr, "======================== Problem\n");
      }
      if (formula_type == CNF2MK) {
         // add  min sat
         for(int i=1;i<=n;i++) {
            if ((i-1)%6 == 0) {
               if (i!=1) fprintf(stdout, " ] 3\n");
               fprintf(stdout, "#1 [ ");
            }
            fprintf(stdout, "%s ", var(n, i, 0));
         }
         // add symetry
         /*
         for(int i=1;i<=sym;i++) {
            if ((i-1)%11 == 0) continue;
            for(int j=1;j<(p-1);j++) {
               int eq_var = j*sym+i;
               fprintf(stdout, "-%s ", var(n, i, 0));
               fprintf(stdout, "%s 0\n", var(n, eq_var, 0)); 
               fprintf(stdout, "%s ", var(n, i, 0));
               fprintf(stdout, "-%s 0\n", var(n, eq_var, 0)); 
            }
         }
         */
      }
   } else if (formula_type == XCNF2) {
      fprintf(stdout, "c prevent any arithmetic progression of length %d for every bucket %d\n", p, k);
      for(int num = 1; num <= n; num++) {
         for(int step = 1; 1; step++) {
            if (step * (p-1) + num > n) break;
            fprintf(stdout, "#1 [ ");
            int base = num;
            for(int z = 0; z < p; z++) {
               fprintf(stdout, "%s ", var(n, base, 0));
               base+=step;
            }
            fprintf(stdout, " ] %d\n", p-1);
            clause_count++;
         }
      }
      if (clause_count != clauses) {
         fprintf(stderr, "======================== Problem\n");
      }
   } else if (formula_type == XITE2) {
      fprintf(stdout, "; prevent any arithmetic progression of length %d for every bucket %d\n", p, k);
      for(int num = 1; num <= n; num++) {
         for(int step = 1; 1; step++) {
            if (step * (p-1) + num > n) break;
            fprintf(stdout, "* minmax(%d 1 %d ", p, p-1);
            int base = num;
            for(int z = 0; z < p; z++) {
               fprintf(stdout, "%s ", var(n, base, 0));
               base+=step;
            }
            fprintf(stdout, " )\n");
            clause_count++;
         }
      }
      /*
      fprintf(stdout, "* minmax(%d %d %d ", n, p, n/2);
      for(int num = 1; num <= n; num++) {
         fprintf(stdout, "%s ", var(n, num, 0));
      }
      fprintf(stdout, " )\n");
      */
      if (clause_count != clauses) {
         fprintf(stderr, "======================== Problem\n");
      }
   } else if (formula_type == MINXCNF) {
      fprintf(stdout, "c prevent any arithmetic progression of length %d for every bucket %d\n", p, k);
      fprintf(stdout, "#1 [ ");
      for(int y = 1; y <= n; y++)
         fprintf(stdout, "%s ", var(n, y, 0));
      fprintf(stdout, "] %d\n", n/2);
      clause_count++;
      for(int num = 1; num <= n; num++) {
         for(int step = 1; 1; step++) {
            if (step * (p-1) + num > n) break;
            int base = num;
            for(int z = 0; z < p; z++) {
               fprintf(stdout, "-%s ", var(n, base, 0));
               base+=step;
            }
            fprintf(stdout, "0\n");
            clause_count++;
         }
      }
      if (clause_count != clauses) {
         fprintf(stderr, "======================== Problem (%d != %d)\n",
               clause_count, clauses);
      }
   }
}

void print_par_rec(FILE *fout, char *slit, char *delim, int *i, int depth, int n) ;

// void print_par("y%d", "|", clause_count);
void print_par(FILE *fout, char *slit, char *delim, int n) 
{
   int i=1;
   print_par_rec(fout, slit, delim, &i, n, n);
}


void print_par_rec(FILE *fout, char *slit, char *delim, int *i, int depth, int n) 
{
   if (depth<2) {
      if (*i<n) {
         fprintf(fout, "(");
         fprintf(fout, slit, *i);
         (*i)++;
         fprintf(fout, "%s", delim);
         fprintf(fout, slit, *i);
         (*i)++;
         fprintf(fout, ")");
      } else if (*i<=n) {
         fprintf(fout, slit, *i);
         (*i)++;
      }
      return;
   }
   fprintf(fout, "(");
   print_par_rec(fout, slit, delim, i, depth/2, n);
   if ((*i)<=n) {
      fprintf(fout, "%s", delim);
      print_par_rec(fout, slit, delim, i, depth/2, n);
   }
   fprintf(fout, ")");
}

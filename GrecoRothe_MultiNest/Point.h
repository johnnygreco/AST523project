/***************************************************

File: Point.h 

Description:
Header file for the Point class. Point objects store 
all information about live and dead points, such as
parameter values and prior distributions, logL, 
contribution to the evidence, etc. In addition, 
the hypercube to physical prior transformations 
take place in a method of this class. 

Programmers: Johnny Greco & Johannes Rothe
Contacts: greco@princeton.edu, jrothe@princeton.edu

****************************************************/
#ifndef POINT_H
#define POINT_H
#include <iostream>
#include <list>
#include <fstream>
#include <string>
#include <float.h>
#include <time.h>
#include <vector>
#include <cmath>
#include <iostream>
#include <gsl/gsl_eigen.h>
#include <gsl/gsl_blas.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_linalg.h>
using namespace std;

#define THRESH 0.0001
#define UNIFORM ((rand() + 0.5)/(RAND_MAX+1.0))
#define PLUS(x,y) (x>y ? x+log(1+exp(y-x)) : y+log(1+exp(x-y))) // returns log(exp(x)+exp(y))

class Point
{
    private:
        int D; 
        double logL;
        double logWt;
        struct Parameter
        {
            std::string prior;
            double u;
            double theta;
            double min;
            double max;
        };
        vector<Parameter> myparams;

    public:

        Point(int);
        void set_params(vector<string>,vector<double>,vector<double>);
        void set_u(int i, double x){myparams[i].u = x;}
        void set_u(gsl_vector * coor){for(int i = 0; i < D; i++){myparams[i].u = gsl_vector_get(coor,i);}}
        void set_theta(int i, double x){myparams[i].theta = x;}
        double get_theta(int i){return myparams[i].theta;}
        double get_u(int i){return myparams[i].u;}
        void set_logL(double L){logL = L;}
        double get_logL(){return logL;}
        void set_logWt(double W){logWt = W;}
        double get_logWt(){return logWt;}
        int get_D(){return D;}
        void hypercube_prior();
        void transform_prior();
};
#endif

#ifndef SAMPLERS_H
#define SAMPLERS_H

#include "Data.h"
#include "Ellipsoid.h"

// **** Samplers Utilities 
double dist(int, Point *, gsl_vector *);
int KMeans(vector<Point *>&, int, int, int *);
Ellipsoid FindEnclosingEllipsoid(vector<Point *>&, int);
void SelectFromGrouping(vector<Point *>&, int, int *, int, vector<Point *>&);
bool u_in_hypercube(gsl_vector *, int);
// ****

class Samplers
{
    private:
        int D;
	int N;
        double Vtot;
        double e;
	double logZ; 
	double H;

        vector<Ellipsoid *> clustering;
	list<Point *> discard_pts;
    public:
        Samplers(int Dim, int Npts, Data data_obj, vector<string> prior_types, vector<double> min_vals, vector<double> max_vals, double eff);
	~Samplers();
        gsl_vector * DrawSample();
	double ResetWorstPoint(int nest, Data data_obj);
	void printout();
	void EllipsoidalPartitioning(vector<Point *>& pts, double Xi);
        int get_NumEll() {return clustering.size();}
        void mcmc(Point*, Data, double);
        void CalcVtot(); 
        double ClusteringQuality(double Xtot) {return Vtot/Xtot;} 
        void ClearCluster();
	void EraseFirst() {clustering.erase(clustering.begin());}

	int getN() {return N;}
};
#endif
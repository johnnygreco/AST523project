#include "Samplers.h"
#include "Point.h"

Samplers::Samplers(string const& prior_types, double* min_vals, int nmin, double* max_vals, int nmax, double eff, int Npts) {
  N = Npts;
  D = nmin; 
  e = eff; 
  Vtot = 0.0;
  logZ = -DBL_MAX;
  H = 0.0;

  cout << "creating " << N << " active points" << endl;
  
  // **** create N active points and set params
  //temporary vector, but pointers will be given to the first ellipsoid
  vector <Point *> pts(N);
  for(int j=0; j<N; j++)
    {
      pts[j] = new Point(D); 
      pts[j]->set_params(prior_types, min_vals, max_vals);
      pts[j]->hypercube_prior();
      pts[j]->transform_prior();
      //data_obj.logL(pts[j]); //this need to be done not within sampler
    }
  // ******************************************
  Ellipsoid  firstEll = FindEnclosingEllipsoid(pts,D); //I think this does not need L right now
  clustering.push_back(new Ellipsoid(D, firstEll.getCenter(), firstEll.getCovMat(), firstEll.getEnlFac(), pts)); //new ellipsolid also doesn't need to know L
  // firstEll and vector points go out of scope here, but values survive copied into clustering[0
  CalcVtot();
}
 
Samplers::~Samplers() {
  
  for(list<Point *>::iterator s=discard_pts.begin();s!=discard_pts.end();s++){delete *s;} 
  int size = clustering.size();
  for(int i=0;i<size;i++){delete clustering[i];}
}

gsl_vector * Samplers::DrawSample()
{
  // chooses an ellipsoid from clustering (volume-weighted for uniformity)
  // then causes it to sample a point into its newcoor member variable
  // and returns a pointer to it

    int RandEll;
    int NumEll = clustering.size();
    int n_e;
    gsl_vector * tmp_coor = gsl_vector_alloc(D);

    do 
    {
        do RandEll= rand() % clustering.size();
        while (clustering[RandEll]->getVol()/Vtot < UNIFORM);

        do clustering[RandEll]->SampleEllipsoid();
        while (!u_in_hypercube(clustering[RandEll]->get_newcoor(), D));

        gsl_vector_memcpy(tmp_coor, clustering[RandEll]->get_newcoor());

        n_e = 0;
        for(int i = 0; i<NumEll; i++)
        {
           if(clustering[i]->IsMember(tmp_coor)) {n_e++;}
        }
    }
    while(1.0/n_e < UNIFORM);
    
    gsl_vector_free(tmp_coor);

    return clustering[RandEll]->get_newcoor();
}

void Samplers::CalcVtot()
{
    Vtot=0.0; 

    for(int i=0; i<clustering.size(); i++)
    {
       Vtot += clustering[i]->getVol();
    }
}

void Samplers::DisgardWorstPoint(double * logL,int nL, int nest) {

  double logwidth = log(1.0 - exp((double)-1.0/N)) - (double) nest/N; 
  double  logZnew, logZ_err;

  //double logLmin = clustering[0]->ell_pts_[0]->get_logL();
  //logL is returned by data_obj as an array that match the sequence of the clustering iteration.
  double logLmin = logL[0];
  double logLmax = -999.9;
  double logL_tmp = 0.0;

  //int ellworst = 0; 
  //int ptworst = 0;
  Point * worst;
  int counter = 0; 
  // find lowest and highest logL
  for(int i=0; i<clustering.size(); i++) {
    for(int j=0; j<clustering[i]->ell_pts_.size(); j++) {
      logL_tmp = logL[counter];
      if(logL_tmp < logLmin) {ellworst = i; ptworst = j; logLmin = logL_tmp;} 
      if(logL_tmp > logLmax) {logLmax = logL_tmp;}
      counter++;
    }
  }
  // find contribution to evidence
  worst = clustering[ellworst]->ell_pts_[ptworst];
  //worst->set_logWt(logwidth + worst->get_logL()); 
  worst->set_logWt(logwidth + logLmin); 
  
  logZnew = PLUS(logZ, worst->get_logWt()); 
  H = exp(worst->get_logWt() - logZnew)*(logLmin) + exp(logZ - logZnew)*(H + logZ) - logZnew;
  logZ = logZnew; // update global evidence
  
  // save discarded point for posterior sampling
  discard_pts.push_back( new Point(*worst) ); 

}

void Samplers::ResetWorstPoint(Data data_obj){  
  // **************** ellipsoidal sampling
  // CH:I need to think about how to change this
  Point * worst;
  worst = clustering[ellworst]->ell_pts_[ptworst];
  do
    {
      worst->set_u(DrawSample());
      worst->transform_prior();
      data_obj.logL(worst);
    }
  while(logLmin > worst->get_logL());
  return logLmax;
}

void Samplers::getAlltheta(double *Alltheta, int nx, int ny){
  int counter=0;
  Point * pt;
  for(int i=0; i<clustering.size(); i++) {
    for(int j=0; j<clustering[i]->ell_pts_.size(); j++) {
      pt = clustering[ellworst]->ell_pts_[ptworst];
      pt->get_theta(Alltheta,nt);
      counter+=nx;
    }
  }
}

void Samplers::Recluster(double X_i){
    ClearCluster();
	  vector <Point *> empty;
	  EllipsoidalPartitioning(empty, X_i);
	  EraseFirst();
	  CalcVtot();
}

void Samplers::printout() {
  double logZ_err = sqrt(H/N);
  cout << "information: H =  " << H/log(2.0) << " bits " << endl;
  cout << "global evidence: logZ = " << logZ << " +/- " << logZ_err << endl;
  cout << "****************" << endl;
  ofstream outfile;
  outfile.open("posterior_pdfs.dat");
  list<Point *>::iterator s;
  for(s=discard_pts.begin(); s!=discard_pts.end(); s++)
    outfile << (*s)->get_theta(0) << " " << (*s)->get_theta(1) << " " << (*s)->get_logL() << " " << exp((*s)->get_logL() - logZ) << endl;
  // ************* 
  outfile.close();
}

void Samplers::ClearCluster() {

  for(int i=1; i<clustering.size(); i++) {
    (*clustering[0]).fetchPoints(*clustering[i]);
  }

  while(clustering.size()>1) {
    clustering.pop_back();
  }
  
}



void Samplers::EllipsoidalPartitioning(vector<Point *>& pts, double Xtot) 
{
  // this is the variant currently in development, because segfaults could be
  // avoided. will check if this implementation is "good" in some sense...
  // vector of ellipsoid pointers is returned, these are deleted after use in
  // the function calling EllipsoidalPartitioningVec performs Algorithm I from
  // Feroz, Hobson and Bridges (2009) on N points in D-dimensional
  // [0,1]-hypercube given in coors. Returns resulting number of ellipsoids in
  // Nell, uses new to create array of ellipsoids, first address is returned in
  // clustering

  /////////////////
  // ALGORITHM I //
  /////////////////

  // create mainEllipsoid which may be split further
  // ?? in recursion depth, this first ellipsoid can be passed by parent??


  // if function was called from main.cc (and not itself), pts will be empty, data instead will be in (irrelevant) first ellipsoid
  if(pts.size()==0) {
    pts = clustering[0]->ell_pts_;
  }

  int N = pts.size();

  Ellipsoid mainEll = FindEnclosingEllipsoid(pts,D);

  //enlarge if neccessary
  if(Xtot/e>mainEll.getVol()) {
    mainEll.setEnlFac(mainEll.getEnlFac()*pow(Xtot/mainEll.getVol()/e,1.0/D));
  }
  // initialize splitting using kmeans
  int i;
  int grouping[N];
  int k=2;
  KMeans(pts,D,k,&grouping[0]);
  vector<Point *> pts_group_0;
  vector<Point *> pts_group_1;

  SelectFromGrouping(pts, D, grouping, 0, pts_group_0);
  SelectFromGrouping(pts, D, grouping, 1, pts_group_1);

  bool changed = true;
  double X1;
  double X2;
  double h1;
  double h2;
  double tmp1,tmp2;
  double vol1,vol2;

  while(changed) {
    // return main ellipsoid immediately if partitioning would create singular (flat) ellipsoid
    if(pts_group_0.size()<D+1 or pts_group_1.size()<D+1) {
      clustering.push_back (new Ellipsoid(D, mainEll.getCenter(), mainEll.getCovMat(), mainEll.getEnlFac(), pts) );
      return;
    }
    // find new sub-Ellipsoids
    //"locality" of these variables removes object overwriting trouble
    Ellipsoid subEll1 = FindEnclosingEllipsoid(pts_group_0,D);
    vol1 = subEll1.getVol();
    X1 = ((double)pts_group_0.size()/N)*Xtot;
    Ellipsoid subEll2 = FindEnclosingEllipsoid(pts_group_1,D);
    vol2 = subEll2.getVol();
    X2 = ((double)pts_group_1.size()/N)*Xtot;

    //enlarge if neccessary
    if(X1/e>vol1) {
      subEll1.setEnlFac(subEll1.getEnlFac()*pow(X1/vol1/e,(double)(1.0/D)));
      vol1 = subEll1.getVol();
    }
    if(X2/e>vol2) {
    subEll2.setEnlFac(subEll2.getEnlFac()*pow(X2/vol2/e,(double)(1.0/D)));
    vol2 = subEll2.getVol();
    }

    // optimize splitting using mahalanobis-h-metric
    changed = false;    
    for(i=0;i<N;i++) 
    {
      tmp1 = subEll1.mdist(pts[i]);
      tmp2 = subEll2.mdist(pts[i]);
   
      h1 = vol1 / X1 * tmp1;
      h2 = vol2 / X2 * tmp2;

      if (h2<h1 and grouping[i]==0) 
      {
          changed = true;
          grouping[i] = 1;
      }
      if (h1<h2 and grouping[i]==1) 
      {
          changed = true;
          grouping[i] = 0;
      }
     }

     pts_group_0.clear();
     pts_group_1.clear();
     SelectFromGrouping(pts, D, grouping, 0, pts_group_0);
     SelectFromGrouping(pts, D, grouping, 1, pts_group_1);
  }
  // judgement if splitting should be continued
  if( vol1+vol2<mainEll.getVol() or mainEll.getVol()>2*Xtot) {

    // recursively start the splittings of subEll1 and subEll2
    EllipsoidalPartitioning(pts_group_0, X1);
    // second one
    EllipsoidalPartitioning(pts_group_1, X2);

  }
  else{
    // allocate memory for mainEll and push it to the vector
    clustering.push_back (new Ellipsoid(D, mainEll.getCenter(), mainEll.getCovMat(), mainEll.getEnlFac(), pts));
  }
  return;
}

void Samplers::mcmc(Point* pt, Data data_obj, double logLmin)
{
    vector<double> new_coords(D);
    double step = 0.1;
    int accept = 0;
    int reject = 0;
    Point *trial;
    trial = new Point(D);
    *trial = *pt;

    for(int j = 20; j > 0; j--)
    {
        for(int i = 0; i<D; i++)
        {
            new_coords[i] = pt->get_u(i) + step*(2.0*UNIFORM - 1.0);
            new_coords[i] -= floor(new_coords[i]);
            trial->set_u(i, new_coords[i]);
        }

        trial->transform_prior();
        data_obj.logL(trial);

        if(trial->get_logL() > logLmin){*pt = *trial; accept++;}
        else reject++;

        if(accept > reject) step *= exp(1.0 / accept);
        else if(accept < reject) step /= exp(1.0 / reject);
    }

    delete trial;
}
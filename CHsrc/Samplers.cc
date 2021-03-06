#include "Samplers.h"

Samplers::Samplers(double* min_vals, int nmin, double* max_vals, int nmax, double eff, int Npts, string const& prior_types) {
  N_ = Npts;
  D_ = nmin; 
  e_ = eff; 
  Vtot = 0.0;
  logZ = -DBL_MAX;
 //logZ = 0;
  H = 0.0;
  newcoor_ = gsl_vector_alloc(D_);

  srand(time(NULL));  
  unsigned long long int seed = rand();
  myrand.v=4101842887655102017LL;myrand.w=1;
  myrand.v = seed^myrand.v; myrand.int64();
  myrand.w = myrand.v; myrand.int64();                                                 
//srand(time(NULL));  // seed random number generator
  //Ran myrand(rand());        
  printf("creating %d active points\n",N_);
  //for (int i=0; i<1000; i++){
  //printf("%d\n",myrand.int64()%10);
  //} 
  // **** create N active points and set params
  //temporary vector, but pointers will be given to the first ellipsoid
  vector <Point *> pts(N_);
  
    for(int j=0; j<N_; j++)
    {
      pts[j] = new Point(D_); 
      pts[j]->set_params(prior_types, min_vals, max_vals);
      for (int i=0; i<D_;i++){
      //pts[j]->set_u_single(i,randomsave[i+j*D_]); 
      pts[j]->set_u_single(i,myrand.doub());
      //printf("u=%f",pts[j]->get_u(i)); 
      }
      //pts[j]->hypercube_prior();
      pts[j]->transform_prior();
    }
  //delete [] randomsave;
  // ******************************************
  Ellipsoid  firstEll = FindEnclosingEllipsoid(pts,D_); //I think this does not need L right now
  clustering.push_back(new Ellipsoid(D_, firstEll.getCenter(), firstEll.getCovMat(), firstEll.getEnlFac(), pts)); //new ellipsolid also doesn't need to know L
  // firstEll and vector points go out of scope here, but values survive copied into clustering[0
  CalcVtot();
 for(int j=0; j<N_; j++){
 	delete pts[j];
 }
 pts.clear();
}
 
Samplers::~Samplers() {
  gsl_vector_free(newcoor_); 
  for(list<Point *>::iterator s=discard_pts.begin();s!=discard_pts.end();s++){delete *s;}
  discard_pts.clear(); 
  int size = clustering.size();
  for(int i=0;i<size;i++){delete clustering[i];}
  clustering.clear();
}

void Samplers::DrawSample()
{
  // chooses an ellipsoid from clustering (volume-weighted for uniformity)
  // then causes it to sample a point into its newcoor member variable
  // and stores it in the clustering variable newcoor_

    int RandEll;
    int NumEll = clustering.size();
    int n_e;

    do 
    {
      // pick an ellipsoid (weighted by volume)
      do RandEll= myrand.int64() % (clustering.size());
      while (clustering[RandEll]->getVol()/Vtot < myrand.doub());
      
      // choose a point from the ellipsoid lying within the hypercube
      do clustering[RandEll]->SampleEllipsoid();
      while (!u_in_hypercube(clustering[RandEll]->get_newcoor(), D_));

      // count ellipsoids intersecting over the chosen coordinate
      n_e = 0;
      for(int i = 0; i<NumEll; i++)
        {
	  if(clustering[i]->IsMember(clustering[RandEll]->get_newcoor())) {n_e++;}
        }
    }
    // reject to not oversample overlapping volume
    while(1.0/n_e < myrand.doub());
    
    ellnew_ = RandEll;
    gsl_vector_memcpy(newcoor_,clustering[RandEll]->get_newcoor());
}

void Samplers::CalcVtot()
{
    Vtot=0.0; 

    for(unsigned int i=0; i<clustering.size(); i++)
    {
       Vtot += clustering[i]->getVol();
    }
}

void Samplers::SetAllPoint(double * logL,int nL) {
  int counter=0;
  double logL_tmp;
  for(unsigned int i=0; i<clustering.size(); i++) {
    for(unsigned int j=0; j<clustering[i]->ell_pts_.size(); j++) {
      logL_tmp = logL[counter];
      clustering[i]->ell_pts_[j]->set_logL(logL_tmp);
      counter++;
      //double * theta = new double [D_];
      //clustering[i]->ell_pts_[j]->get_theta(theta,D_);
      //printf("theta[0]= %f, %f, %f\n",theta[0],theta[1],logL_tmp);
    }
  }
}
void Samplers::DisgardWorstPoint(int nest) {

  //double logwidth = log(1.0 - exp(-1.0/(double) N_)) - (double) nest/(double) N_; 
  double logwidth = log(0.5*(exp(-((double)(nest-1)/(double)N_)) - exp(-((double)(nest+1)/(double)N_)))); 
  double  logZnew;

  //double logLmin = clustering[0]->ell_pts_[0]->get_logL();
  //logL is returned by data_obj as an array that match the sequence of the clustering iteration.
  logLmin_ = clustering[0]->ell_pts_[0]->get_logL();
  logLmax_ = -999.9;
  double logL_tmp = 0.0;

  //int ellworst = 0; 
  //int ptworst = 0;
  Point * worst;
  // find lowest and highest logL
  ellworst_= 0; ptworst_=0;
  for(unsigned int i=0; i<clustering.size(); i++) {
    for(unsigned int j=0; j<clustering[i]->ell_pts_.size(); j++) {
      logL_tmp=clustering[i]->ell_pts_[j]->get_logL();
      if(logL_tmp < logLmin_) {ellworst_ = i; ptworst_ = j; logLmin_ = logL_tmp;} 
      if(logL_tmp > logLmax_) {logLmax_ = logL_tmp;}
    }
  }
  //printf("logLmin_=%f, logLmax_=%f,ellworst=%d, ptworst=%d\n",logLmin_,logLmax_,ellworst_,ptworst_);
  //printf("clustering.size=%d,%d\n",clustering.size(),clustering[0]->ell_pts_.size());
  // find contribution to evidence
  worst = clustering[ellworst_]->ell_pts_[ptworst_];
  //worst->set_logWt(logwidth + worst->get_logL()); 
  //printf("logwidth=%f,ellworst=%d,clustsize=%d,ptworst=%d\n",logwidth,ellworst_,ptworst_);
  worst->set_logWt(logwidth + logLmin_); 
  //printf("worst,get_logWt\n", worst->get_logWt()); 
  logZnew = PLUS(logZ, worst->get_logWt()); 
  H = exp(worst->get_logWt() - logZnew)*(logLmin_) + exp(logZ - logZnew)*(H + logZ) - logZnew;
  logZ = logZnew; // update global evidence
  //printf("logZ=%f\n",logZ);
  
  // save discarded point for posterior sampling

  discard_pts.push_back( new Point(*worst) );
  //double * theta = new double [D_];
  //worst->get_theta(theta,D_);
  //for (int k=0;k<D_; k++){ 
  //  printf("%f ",theta[k]);
  //}	
  //printf("%f %f %f\n",worst->get_logL(),logwidth,logZ);
  //delete [] theta;

  clustering[ellworst_]->ell_pts_.erase(clustering[ellworst_]->ell_pts_.begin() + ptworst_);
  if(clustering[ellworst_]->ell_pts_.size() ==0){
    clustering.erase(clustering.begin()+ellworst_);
  } else {
    if(clustering[ellworst_]->ell_pts_.size()>D_){
    Ellipsoid newEll = FindEnclosingEllipsoid(clustering[ellworst_]->ell_pts_,D_);
    double Xell = exp((double) -nest/N_)*newEll.ell_pts_.size()/N_;
    if(Xell/e_>newEll.getVol()) {
      newEll.setEnlFac(newEll.getEnlFac()*pow(Xell/newEll.getVol()/e_,2.0/D_));
  }
  
    //printf("create newEll\n");
    clustering.erase(clustering.begin()+ellworst_);
    //printf("erase\n");
    clustering.push_back(new Ellipsoid(D_, newEll.getCenter(), newEll.getCovMat(), newEll.getEnlFac(), newEll.ell_pts_));
    //printf("push back\n");
    }
    }
}

void Samplers::ResetWorstPoint(double *theta, int nt){  
  // **************** ellipsoidal sampling
  Point * worst;
  
  worst = new Point(*(clustering[0]->ell_pts_[0])); //copy the format of a random point
  DrawSample();
  //printf("%d %f %f\n",ellnew_,gsl_vector_get(newcoor_,0),gsl_vector_get(newcoor_,1));
  //worst = clustering[ellworst_]->ell_pts_[ptworst_];
  worst->set_u(newcoor_);
  worst->transform_prior();
  worst->get_theta(theta,nt);
  //printf("reset,%d %f %f %f %f\n",ellnew_,gsl_vector_get(newcoor_,0),gsl_vector_get(newcoor_,1),theta[0],theta[1]);
  
  delete worst;
  //data_obj.logL(worst);
  return ;
}

void Samplers::ResetWorstPointLogL(double logL){
  //printf("ellworst_=%d, ptworst_=%d,logL=%f\n",ellworst_,ptworst_,logL);
  //printf("ellnew_=%d\n",ellnew_);
  clustering[ellnew_]->ell_pts_.push_back(new Point(*(clustering[0]->ell_pts_[0])));
  int n = clustering[ellnew_]->ell_pts_.size();
  //printf("n=%d\n",n);
  clustering[ellnew_]->ell_pts_[n-1]->set_u(newcoor_);
  //printf("newcoor_=\n");
  clustering[ellnew_]->ell_pts_[n-1]->transform_prior();
  //printf("transform_=\n");
  clustering[ellnew_]->ell_pts_[n-1]->set_logL(logL);
}

void Samplers::getAlltheta(double *Alltheta, int nx, int ny){
  int counter=0;
  Point * pt;
  //printf("D=%d,nx=%d,ny=%d\n", D);
  for(unsigned int i=0; i<clustering.size(); i++) {
    for(unsigned int j=0; j<clustering[i]->ell_pts_.size(); j++) {
      pt = clustering[i]->ell_pts_[j];
      //printf("i=%d,j=%d\n", i,j);
      pt->get_theta(&Alltheta[counter],D_);
      //printf("Alltheta[counter]=%f,%f",Alltheta[counter],Alltheta[counter+1]);
      counter+=nx;
      //printf("counter=%d\n", counter);
    }
  }
}

void Samplers::FullRecluster(double X_i)
{

  //deletes all ellipsoids and performs a top-down recursive reclustering
  
   //use empty vector to signal top-level-call to EllipsoidalPartitioning
   //New ways of doing reclustering:
   //1)copy the clusters into a new one instead of clear it.

   vector <Ellipsoid *>cpcluster;
   for(int i=0; i<clustering.size(); i++) {
      cpcluster.push_back (new Ellipsoid(clustering[i]));
   }
   //printf("vtot=%f,X_i=%f\n",Vtot,X_i);
   //printf("copy successful to cpcluster\n");
   //delete all ellipsoids, keep first to store all points
   double oldVtot;
   CalcVtot();
   oldVtot = Vtot;

   ClearCluster();
   vector <Point *> empty;

   //FILE * debugout;
   //debugout = fopen("deb.out","a");
   //fprintf(debugout,"\n");
   
   //printf("EllipsoidalPartitioning Start\n");
   EllipsoidalPartitioning(empty, X_i);
   //printf("EllipsoidalPartitioning Done\n"); 
   EraseFirst();
   //InflateEllipsoids(X_i);
   CalcVtot();
   //check the reclustering quality
   //double quality = ClusteringQuality(X_i);
   
   //printf("quality=%f,vtot=%f,X_i=%f\n",quality,Vtot,X_i);
   // if reclustering is good, keep the reclustering, else copy back 
   // the old clustering
   //if (quality>1.2){
   if( Vtot > oldVtot ){
      //printf("do not use the reclustering result\n");
      //fprintf(debugout,"did not use the reclustering result\n");
      while(clustering.size()>0) {
        clustering.pop_back();
      }
      for(int i=0; i<cpcluster.size(); i++) {
        clustering.push_back(new Ellipsoid(cpcluster[i]));
      }
      //printf("cp back successful\n");
      //printf("quality=%f,vtot=%f,X_i=%f\n",quality,Vtot,X_i);
   } 
   //fclose(debugout);


}

int Samplers::Recluster(double X_i, double qualthresh, bool verbose){
  // single ellipsoid reclustering
  // returns 1 iff reclustering happened
  // instead of just performing the case distinction between top-level and recursion-call, this provides the details of the individual-ellipsoid-reclustering

  int reclustered = 0;
  // remember number of previous ellipsoids to avoid rechecking newly created ones
  int Nellprev = clustering.size();
  // check individual ellipsoids for reclustering
  for(int i=0;i<Nellprev;i++) {
    if(clustering[i]->ell_pts_.size() > D_+1) {
      // check clustering criterion
      double Xell = X_i*clustering[i]->ell_pts_.size()/N_;
      if(verbose) {
	printf("%f %f %i %i %f %f\n",Xell,X_i,clustering[i]->ell_pts_.size(),N_,clustering[i]->getVol(),clustering[i]->getVol()/Xell);
	clustering[i]->printout();
      }
      if(clustering[i]->getVol()/Xell > qualthresh) {
	// if necessary, call partitioning on its list of points.
	reclustered = 1;
	EllipsoidalPartitioning(clustering[i]->ell_pts_,Xell);
	// new ellipsoids have been appended to clustering with hard copies of this one's points, so it can be removed
	delete clustering[i];
	clustering.erase(clustering.begin()+i);
	// one ellipsoid fewer to worry about
	Nellprev--;
	i--;
      }
    }
    // good ellipsoids are left alone
  }
  
  CalcVtot();

  return reclustered;
}

int Samplers::countTotal(){
  return discard_pts.size()+N_;
}


void Samplers::OutputClusters(){
  //double *theta = new double [D_]; 
 Point * pt;
 for(int i=0; i<clustering.size(); i++) {
    for (int j=0;j<clustering[i]->ell_pts_.size(); j++){
      pt = clustering[i]->ell_pts_[j];
      //pt->get_theta(theta,D_);
      for (int k=0;k<D_; k++){
	//printf("%f ",theta[k]);
	printf("%f ",pt->get_u(k));
      }
      printf("\n");
    }
 }
 //delete [] theta;

 for(int i=0; i<clustering.size(); i++) {
  clustering[i]->printout();
 }
}

void Samplers::getPosterior(double * posterior, int nx, int ny, double *prob, int np, double *logWts, int np2) {
  list<Point *>::iterator s;
  Point * pt;
  int counter = 0, i=0, k=0;
  for(s=discard_pts.begin(); s!=discard_pts.end(); s++){
      (*s)->get_theta(&posterior[counter],D_);
      prob[k] = (*s)->get_logL();
      logWts[k] = (*s)->get_logWt();
      //printf("prob[i]=%f\n",prob[i]);
    counter+=D_;
    k+=1;
    
  }
   
  for(i=0; i<clustering.size(); i++) {
    for(int j=0; j<clustering[i]->ell_pts_.size(); j++) {
      pt = clustering[i]->ell_pts_[j];
      //printf("i=%d,j=%d\n", i,j);
      pt->get_theta(&posterior[counter],D_);
      prob[k] = pt->get_logL();
      //printf("Alltheta[counter]=%f,%f",Alltheta[counter],Alltheta[counter+1]);
      k++;
      counter+=D_;
      //printf("counter=%d\n", counter);
    }
  }
}
void Samplers::getlogZ(double *logzinfo, int nz){
  double logZ_err = sqrt(H/N_);
  logzinfo[0] = H/log(2.0);
  logzinfo[1] = logZ;
  logzinfo[2] = logZ_err;
}

void Samplers::ClearCluster() {

  for(int i=1; i<clustering.size(); i++) {
    (*clustering[0]).fetchPoints(*clustering[i]);
  }
  while(clustering.size()>1) {
    clustering.pop_back();
  }
  
}

void Samplers::EraseFirst() {
  delete clustering[0];
  clustering.erase(clustering.begin());
}

void Samplers::EllipsoidalPartitioning(vector<Point *>& pts, double Xtot)
{
  // vector of ellipsoid pointers is returned, these are deleted after use in
  // the function calling EllipsoidalPartitioningVec performs Algorithm I from
  // Feroz, Hobson and Bridges (2009) on N points in D-dimensional
  // [0,1]-hypercube given in coors. Returns resulting number of ellipsoids in
  // Nell, uses new to create array of ellipsoids, first address is returned in
  // clustering
  
  /////////////////
  // ALGORITHM I //
  /////////////////
  
  bool outputdetailed = false;

  // create mainEllipsoid which may be split further
  // ?? in recursion depth, this first ellipsoid can be passed by parent?? 
  //printf("Level down\n");
  
  // if function was called from main.cc (and not itself), pts will be empty, data instead will be in (irrelevant) first ellipsoid
  if(pts.size()==0) {
    pts = clustering[0]->ell_pts_;
  }

  int N = pts.size();

  if(outputdetailed)
    {
      FILE * debugout;
      debugout = fopen("test/partitioning_detailed/partitioning_details.txt","a");
      fprintf(debugout,"#New Instance\n");
      for(int i=0;i<N;i++)
	{
	  for(int j=0;j<D_;j++)
	    {
	      fprintf(debugout,"%8.4f\t",pts[i]->get_u(j));
	    }
	  //fprintf(debugout,"%i\n",grouping[i]);
	}
      fclose(debugout);
      //fprintf(debugout,"%f\t%f\t",Xtot,mainEll.getVol());
    }
  
  Ellipsoid mainEll = FindEnclosingEllipsoid(pts,D_);
  
  if(Xtot/e_>mainEll.getVol()) {
    mainEll.setEnlFac(mainEll.getEnlFac()*pow(Xtot/mainEll.getVol()/e_,2.0/D_));
  }
  
  //fprintf(debugout,"%f\n",mainEll.getVol());

  int i;
  int grouping[N];
  int k=2;
  int convergescale = 30;

  // initialize splitting with K-means algorithm
  KMeans(pts,D_,k,&grouping[0]);

  if(outputdetailed)
    {
      FILE * debugout;
      debugout = fopen("test/partitioning_detailed/partitioning_details.txt","a");
      for(i=0;i<N;i++)
	{
	  for(int j=0;j<D_;j++)
	    {
	      fprintf(debugout,"%8.4f\t",pts[i]->get_u(j));
	    }
	  fprintf(debugout,"%i\n",grouping[i]);
	}
      fclose(debugout);
    }

  vector<Point *> pts_group_0;
  vector<Point *> pts_group_1;
  vector<double> volumesave(convergescale);
  SelectFromGrouping(pts, D_, grouping, 0, pts_group_0);
  SelectFromGrouping(pts, D_, grouping, 1, pts_group_1);

  bool changed = true;
  bool uselast = false;
  int lastgrouping[N];
  double X1;
  double X2;
  double h1;
  double h2;
  double tmp1,tmp2;
  double vol1,vol2;
  //printf("TradePoints Start\n");
  int count=0;

  // check initial splitting for vol0 ellipsoid and initialize ellipsoids
  if(pts_group_0.size()<D_+1 or pts_group_1.size()<D_+1) {
    clustering.push_back (new Ellipsoid(D_, mainEll.getCenter(), mainEll.getCovMat(), mainEll.getEnlFac(), pts) );
    if(outputdetailed)
      {
	FILE * debugout;
	debugout = fopen("test/partitioning_detailed/partitioning_details.txt","a");
	fprintf(debugout,"#Returning2 (splinter)\n");
	fclose(debugout);
      }
    //printf("Returning (splinter)\n");
    return;
  }

  Ellipsoid subEll1 = FindEnclosingEllipsoid(pts_group_0,D_);
  X1 = ((double)pts_group_0.size()/N)*Xtot;
  if(X1/e_>subEll1.getVol()) {
    subEll1.setEnlFac(subEll1.getEnlFac()*pow(X1/e_/subEll1.getVol(),2.0/D_));
  }
  vol1 = subEll1.getVol();

 
  Ellipsoid subEll2 = FindEnclosingEllipsoid(pts_group_1,D_);
  X2 = ((double)pts_group_1.size()/N)*Xtot;
  if(X2/e_>subEll1.getVol()) {
    subEll1.setEnlFac(subEll1.getEnlFac()*pow(X2/e_/subEll1.getVol(),2.0/D_));
  }
  vol2 = subEll2.getVol();

  //subEll1.printout();
  //subEll2.printout();

  if(outputdetailed)
    {      
      FILE * debugout;
      debugout = fopen("test/partitioning_detailed/partitioning_details.txt","a");
      fprintf(debugout,"#TradingPoints\n");
      fclose(debugout);
    }
  
  for (i=0;i<N;i++){
    lastgrouping[i] = grouping[i];
  }

  // start trading points
  while(changed){

    changed = false;
    for(i=0;i<N;i++) {
      tmp1 = subEll1.mdist(pts[i]);
      tmp2 = subEll2.mdist(pts[i]);
      
      h1 = vol1 / X1 * tmp1;
      h2 = vol2 / X2 * tmp2;
      
      if (h2<h1 and grouping[i]==0) {
	changed = true;
	grouping[i] = 1;
	//printf("TradePoint > %i %f %f\n",i,h1,h2);
      }
      if (h1<h2 and grouping[i]==1) {
	changed = true;
	grouping[i] = 0;
	//printf("TradePoint < %i %f %f\n",i,h1,h2);
      }

    }
    pts_group_0.clear();
    pts_group_1.clear();
    SelectFromGrouping(pts, D_, grouping, 0, pts_group_0);
    SelectFromGrouping(pts, D_, grouping, 1, pts_group_1);

    // check new splitting for vol0 ellipsoid and create new ellipsoids
    if(pts_group_0.size()<D_+1 or pts_group_1.size()<D_+1) {
      clustering.push_back (new Ellipsoid(D_, mainEll.getCenter(), mainEll.getCovMat(), mainEll.getEnlFac(), pts) );

      if(outputdetailed)
	{
	  FILE * debugout;      
	  debugout = fopen("test/partitioning_detailed/partitioning_details.txt","a");
	  fprintf(debugout,"#Returning (splinter)\n");
	  fclose(debugout);
	}
      return;
    }
    Ellipsoid subEll1 = FindEnclosingEllipsoid(pts_group_0,D_);
    X1 = ((double)pts_group_0.size()/N)*Xtot;
    vol1 = subEll1.getVol();
    
    Ellipsoid subEll2 = FindEnclosingEllipsoid(pts_group_1,D_);
    X2 = ((double)pts_group_1.size()/N)*Xtot;
    vol2 = subEll2.getVol();
    
    // check whether volume improvement has converged
    double oldsum=0.,newsum=0.;
    if(count<convergescale){
      volumesave[count]=vol1+vol2;
    } else{
      volumesave.erase(volumesave.begin());
      volumesave.push_back(vol1+vol2);
      for (int ni=0; ni<convergescale; ni++){
        if(ni<convergescale/2.){
          oldsum+=volumesave[ni];
        }
	else {
	  newsum+=volumesave[ni];
	}
      }
      if(oldsum-newsum<1.e-7){
	//printf("PointTrade Converged\n");
        //printf("+++ %d %f %f %f %f %f\n",count,vol1,vol2,vol1+vol2,oldsum,newsum);
	uselast = true;
      }
    }

    count+=1;
    //printf("+++ %d %f %f %f %f %f\n",count,vol1,vol2,vol1+vol2,oldsum,newsum);
    
    // if volume improvement converged, go on until volume worsens, then take previous
    if(uselast){
      if((vol1+vol2)>=volumesave[convergescale-2]){  
	pts_group_0.clear();
	pts_group_1.clear();
	SelectFromGrouping(pts, D_, lastgrouping, 0, pts_group_0);
	SelectFromGrouping(pts, D_, lastgrouping, 1, pts_group_1);
	break;
      }
    }   

    // inflate ellipsoids for next round of trading
    if(X1/e_>subEll1.getVol()) {
      subEll1.setEnlFac(subEll1.getEnlFac()*pow(X1/e_/subEll1.getVol(),2.0/D_));
    }
    if(X2/e_>subEll1.getVol()) {
      subEll1.setEnlFac(subEll1.getEnlFac()*pow(X2/e_/subEll1.getVol(),2.0/D_));
    }

    // update saved previous grouping
    for(i=0;i<N;i++) {lastgrouping[i]=grouping[i];}
    
    //printf("...\n");
  } 
  //printf("TradePoints Done\n");

  if(outputdetailed)
    {
      FILE * debugout;
      debugout = fopen("test/partitioning_detailed/partitioning_details.txt","a");
      fprintf(debugout,"#CompletedTrade\n");
      for(i=0;i<N;i++)
	{
	  for(int j=0;j<D_;j++)
	    {
	      fprintf(debugout,"%8.4f\t",pts[i]->get_u(j));
	    }
	  fprintf(debugout,"%i\n",lastgrouping[i]);
	}
      fclose(debugout);
    }

  //if( vol1+vol2<mainEll.getVol() or mainEll.getVol()>2.0*Xtot/e_) {
  //if( (vol1+vol2)/e_<mainEll.getVol() or mainEll.getVol()>2.0*Xtot/e_) {
  
  //inflate post
  //if( (vol1+vol2)<mainEll.getVol() or mainEll.getVol()>2.0*Xtot) {
  
  // inflated at birth
  //if( (vol1+vol2)<mainEll.getVol() or mainEll.getVol()>2.0*Xtot/e_) {
  // treat as inflated "at birth"
  
  // decide whether to accept proposed new splitting
  if( (max(vol1,X1/e_)+max(vol2,X2/e_))<mainEll.getVol() or mainEll.getVol()>2.0*Xtot/e_) {
    
    // recursively start the splittings of subEll1 and subEll2
    EllipsoidalPartitioning(pts_group_0, X1);
    // second one
    EllipsoidalPartitioning(pts_group_1, X2);
  
  }
  else{
    // allocate memory for mainEll and push it to the vector
    clustering.push_back (new Ellipsoid(D_, mainEll.getCenter(), mainEll.getCovMat(), mainEll.getEnlFac(), pts));
  } 
 
  //printf("Returning (split worse)\n");
  return;
}

void Samplers::InflateEllipsoids(double Xtot) {
  // after EllipsoidalPartitioning, match sampling efficiency by enlarging ellipsoids to volume X/e where necessary
  for (int i=0;i<clustering.size();i++) {
    //    printf("I checked ellipsoid %i\n",i);
    //    printf("%i\t%i\t%f\t%f\t%f\n",clustering[i]->ell_pts_.size(),N_,Xtot,e_,clustering[i]->getVol());
    if((double)clustering[i]->ell_pts_.size()/N_*Xtot/e_ > clustering[i]->getVol()) {
      clustering[i]->setEnlFac(clustering[i]->getEnlFac()*pow((double)clustering[i]->ell_pts_.size()/N_*Xtot/clustering[i]->getVol()/e_,2.0/D_));
      //      printf("I rescaled ellipsoid %i\n",i);
    } 

  }
}

void Samplers::EllipsoidalRescaling(double Xi) {
    // cycle through all ellipsoids, shrink by exponential decay of prior volume and rescale to make bounding again
  int Npoints;
  double Vscale;
  //printf("Vtotal=%f",Vtot);
  for(unsigned int i=0; i<clustering.size(); i++) {
    Npoints = clustering[i]->ell_pts_.size();
    if(Npoints>0){
    // rescale to current partial prior volume
    Vscale = Xi*Npoints/N_;
    if(Vscale>clustering[i]->getVol()){
      //printf("did rescaling,Vscale=%f,Vold=%f\n",Vscale,clustering[i]->getVol());
    clustering[i]->setEnlFac(clustering[i]->getEnlFac()*pow(Xi/e_*(double)Npoints/(double)N_/clustering[i]->getVol(),(double)1./(double)D_));
    // rescale to catch all points
    clustering[i]->RescaleToCatch();
    }
    }
  }
  //CalcVtot();
  //printf("Vtotal=%f",Vtot);
} 
    

void Samplers::MergeEllipsoids(double Xi)
{
  // DESCRIPTION
  // make list to mark merge checks
  // loop over ellipsoids
     // list ten closest other ellipsoids
     // loop over 10-closest-list
        // if total volume decreases, merge them and go back to list-step   
     // if loop completed and no merge just happened, go to next ellipsoid

  // IMPLEMENTATION
  // make list to mark merge checks
  //int checked[clustering.size()][clustering.size()];
  //for (int i=0;i<clustering.size();i++) {
  //  for (int j=0;j<clustering.size();j++) {
  //    checked[i][j] = 0;
  //  }
  //}
  
  // number of closest ellipsoids to merge-check
  int maxclosest = 2*D_;
  double allowed_growth = 1.1;
  
  printf("# new clustering\n");
  for(int i=0; i<clustering.size(); i++) {
    clustering[i]->printout();
  }

  // loop over ellipsoids
  int ell_check = 0;
  while (ell_check < clustering.size())
    {
      //printf("# checking ell %i\n",ell_check);
      
      // indices and distances of closest ellipsoids
      std::vector< std::pair<int,double> > closest;
      // list ten closest other ellipsoids
      double maxdist = 2*sqrt(D_); // high value      
      for (int ell_other=0;ell_other<clustering.size();ell_other++) {
	if (ell_other==ell_check) continue;
	  
	double d = distvec(D_,clustering[ell_check]->getCenter(),clustering[ell_other]->getCenter());
	if (d<maxdist)
	  {
	    closest.push_back(std::pair<int,double> (ell_other,d));
	    std::sort(closest.begin(),closest.end(),sort_ind_comp);
	    if(closest.size()>maxclosest)
	      {
		closest.pop_back();
		maxdist = closest[maxclosest-1].second;
	      }
	  }	
      }
      // loop over 10-closest-list
      bool merge = false;
      for (int i_other=0;i_other<closest.size();i_other++) {
	int ell_other = closest[i_other].first;
	//printf("# neighbor %i: ell %i\tat %f\t",i_other,ell_other,closest[i_other].second);
	printf("# checking ells:\n");
	printf("%f\t%f\t%f\n%f\t%f\t%f\n",gsl_vector_get(clustering[ell_check]->getCenter(),0),gsl_vector_get(clustering[ell_check]->getCenter(),1),gsl_vector_get(clustering[ell_check]->getCenter(),2),gsl_vector_get(clustering[ell_other]->getCenter(),0),gsl_vector_get(clustering[ell_other]->getCenter(),1),gsl_vector_get(clustering[ell_other]->getCenter(),2));
	// create merged ellipsoid
	vector<Point *> merge_pts;
	for (int j=0;j<clustering[ell_check]->ell_pts_.size();j++) {
	  merge_pts.push_back(clustering[ell_check]->ell_pts_[j]);
	}
	for (int j=0;j<clustering[ell_other]->ell_pts_.size();j++) {
	  merge_pts.push_back(clustering[ell_other]->ell_pts_[j]);
	}
	Ellipsoid mergedEll = FindEnclosingEllipsoid(merge_pts,D_);
	double Xm = ((double)mergedEll.ell_pts_.size()/N_)*Xi;
	if(Xm/e_>mergedEll.getVol()) {
	  mergedEll.setEnlFac(mergedEll.getEnlFac()*pow(Xm/e_/mergedEll.getVol(),2.0/D_));
	}
	//printf("# points: %i\t%i\t%i",clustering[ell_check]->ell_pts_.size(),clustering[ell_other]->ell_pts_.size(),mergedEll.ell_pts_.size());
	
	double vol1 = clustering[ell_check]->getVol();
	double vol2 = clustering[ell_other]->getVol();
	double volm = mergedEll.getVol();
	// if total volume decreases, merge them and go back to list-step   
	//printf("comparing %f+%f=%f,%f\n",vol1,vol2,vol1+vol2,volm);
	
	//clustering[ell_check]->printout();
	//clustering[ell_other]->printout();
	//mergedEll.printout();

      	if ( volm < allowed_growth*(vol1+vol2) )
	  {
	    printf("# new clustering\n");
	    merge = true;
	    clustering[ell_check] = new Ellipsoid(D_, mergedEll.getCenter(), mergedEll.getCovMat(), mergedEll.getEnlFac(), merge_pts);
	    //clustering[ell_check] = new Ellipsoid(mergedEll);
	    clustering.erase(clustering.begin()+ell_other);
	    for(int i=0; i<clustering.size(); i++) {
	      clustering[i]->printout();
	    }
	    break;
	  }
      }      
      // if loop completed and no merge just happened, go to next ellipsoid
      if (!merge) {ell_check++;}
    }

  return;
}

bool sort_ind_comp (const std::pair<int,double>& l, const std::pair<int,double>& r) { return l.second < r.second; }

double Samplers::dist(int D, Point *pt, gsl_vector * pos2)
{
  // returns 2-norm of difference between vector and point coordinate
  double val=0;
  for (int i=0;i<D;i++){
    val += pow(pt->get_u(i)-gsl_vector_get(pos2,i),2);
  }
  return sqrt(val);
}

double Samplers::distvec(int D, gsl_vector * pos1, gsl_vector * pos2)
{
  // returns 2-norm of difference between two vectors
  double val=0;
  for (int i=0;i<D;i++){
    val += pow(gsl_vector_get(pos1,i)-gsl_vector_get(pos2,i),2);
  }
  return sqrt(val);
}

int Samplers::KMeans(vector<Point *>& pts, int D, int k, int * grouping)
{
 // Implementation of the K-means algorithm following MacKay, D. C. J., 2003,
 // Information Theory, Inference and Learning Algorithms, Cambridge University
 // Press, Cambridge, p. 640 points gives the coordinates of N points in the
 // D-dimensional [0,1]-hypercube, to be split into k clusters. Association of
 // each point to a cluster is returned in int-array grouping
  int i,j;
  int N = pts.size();

  gsl_vector * centers[k];
  //  double centers[k][D];
  //  double ** centers = new double * [k];
  //  for(i=0;i<k;i++){
  //    centers[i] = new double [D];
  //  }

  int nomemb[k];
  bool changed = true;
  int current = 0; 
  //srand(time(NULL));  
  //Ran myrand(rand());        
  // assign points randomly to groups
  for(i=0;i<N;i++){
    //grouping[i] = rand()%k;
    //grouping[i] = myrand.int64()%(k-1);
    grouping[i] = myrand.int64()%(k);
  }
  while(changed){
  // calculate group centers
    for(i=0;i<k;i++){
      nomemb[i] = 0;
      centers[i] = gsl_vector_calloc(D);
    }
    for(i=0;i<N;i++){
      nomemb[grouping[i]]++;
      for(j=0;j<D;j++){
    gsl_vector_set(centers[grouping[i]],j, gsl_vector_get(centers[grouping[i]],j)+pts[i]->get_u(j));
      }
    }
    for(i=0;i<k;i++){
      for(j=0;j<D;j++){
    gsl_vector_set(centers[i],j,gsl_vector_get(centers[i],j)/nomemb[i]);
      }
    }
  // assign points to closest group center
    changed = false;
    for(i=0;i<N;i++){
      current = grouping[i];
      double mindist = dist(D, pts[i], centers[current]);
      int mink = current;
      for(j=0;j<k;j++){
    if( dist(D, pts[i], centers[j])<mindist ){
      mindist = dist(D, pts[i], centers[j]);
      mink = j;
    }
      }
      if( mink != current ){
    changed = true;
    grouping[i] = mink;
      }
    }
    for (i=0;i<k;i++){
    gsl_vector_free(centers[i]);
   }   
  }
  return 0;
}

Ellipsoid Samplers::FindEnclosingEllipsoid(vector<Point *>& pts, int D)
{
  // enclosing ellipsoid data for a given point cloud (N D-dimensional
  // coordinates), enlargement factor f is chosen so that all points are below
  // the ellipsoid surface

  int i,j;
  double tmp;
  int N = pts.size();
  gsl_vector * tmpvec = gsl_vector_alloc(D);
  gsl_vector * tmpvec2 = gsl_vector_alloc(D);

  gsl_vector * center = gsl_vector_alloc(D);
  gsl_matrix * C = gsl_matrix_alloc(D,D);
  double f;
  //printf("before find center\n");
  //find center
  for(i=0;i<D;i++){
    tmp = 0.0;
    for(j=0;j<N;j++){
      tmp += pts[j]->get_u(i);
    }
    gsl_vector_set(center,i,tmp/N);
  }

  //printf("after find center\n");
  //find covariance matrix
  // set C to zero
  gsl_blas_dgemm(CblasNoTrans, CblasNoTrans, 0.0, C, C, 0.0, C);
  //add covariance
  for(j=0;j<N;j++){
    for(i=0;i<D;i++){
      // subtract center from each coor to tmpvec
      gsl_vector_set(tmpvec,i,pts[j]->get_u(i)-gsl_vector_get(center,i));
    }
    // symmetric rank-1 update
    gsl_blas_dsyr(CblasUpper, 1.0/N , tmpvec, C);
  }
  // copy upper half into lower half
  for(i=0;i<D;i++){
    for(j=i+1;j<D;j++){
      gsl_matrix_set(C,j,i,gsl_matrix_get(C,i,j));
    }
  }

  //invert covariance matrix
  int signum = +1;
  gsl_matrix * tmpmat = gsl_matrix_alloc(D,D);
  gsl_matrix * Cinv = gsl_matrix_alloc(D,D);
  gsl_permutation *p = gsl_permutation_calloc(D);
  //  gsl_vector * tmpvec = gsl_vector_alloc(D);

  gsl_matrix_memcpy(tmpmat,C);
  gsl_linalg_LU_decomp (tmpmat, p, &signum);
  gsl_linalg_LU_invert (tmpmat, p, Cinv);

  //find enlargement factor
  f = 0;
  for(j=0;j<N;j++){
    for(i=0;i<D;i++){
      // subtract center from each coor to tmpvec
      gsl_vector_set(tmpvec2,i,pts[j]->get_u(i)-gsl_vector_get(center,i));
    }
  gsl_blas_dsymv (CblasUpper, 1.0, Cinv, tmpvec2, 0.0, tmpvec);
  gsl_blas_ddot (tmpvec2, tmpvec, &tmp);
  if (tmp > f) f = tmp;
  }

  gsl_vector_free(tmpvec);
  gsl_vector_free(tmpvec2);
  gsl_matrix_free(tmpmat);
  gsl_matrix_free(Cinv);
  gsl_permutation_free(p);

  //printf("before the encl\n");
  Ellipsoid encl(D, center, C, f, pts);
  //printf("after the encl\n");

  gsl_vector_free(center);
  gsl_matrix_free(C);

  return encl;
}

void Samplers::SelectFromGrouping(vector<Point *>& pts, int D, int * grouping, int index, vector<Point *>& pts_subset) 
{
  // copies the coor-vectors for which the grouping entry equals index into group, 
  // length of array group must be pre-arranged and all entries calloced
  unsigned int i;
  for(i=0;i<pts.size();i++) {
    if(grouping[i]==index) {
      pts_subset.push_back(pts[i]);
    }
  }
}

bool Samplers::u_in_hypercube(gsl_vector * newcoor, int D)
{
    double x_i;

    for(int i = 0; i<D; i++)
    {
        x_i = gsl_vector_get(newcoor, i);
        if(x_i < 0 || x_i > 1) {return false;}
    }
    return true;
}



//void Samplers::mcmc(Point* pt, Data data_obj, double logLmin)
//{
//    vector<double> new_coords(D);
//    double step = 0.1;
//    int accept = 0;
//    int reject = 0;
//    Point *trial;
//    trial = new Point(D);
//    *trial = *pt;
//
//    for(int j = 20; j > 0; j--)
//    {
//        for(int i = 0; i<D; i++)
//        {
//            new_coords[i] = pt->get_u(i) + step*(2.0*UNIFORM - 1.0);
//            new_coords[i] -= floor(new_coords[i]);
//            trial->set_u_single(i, new_coords[i]);
//        }
//
//        trial->transform_prior();
//        data_obj.log(trial);
//
//        if(trial->get_logL() > logLmin){*pt = *trial; accept++;}
//        else reject++;
//
//        if(accept > reject) step *= exp(1.0 / accept);
//        else if(accept < reject) step /= exp(1.0 / reject);
//    }
//
//    delete trial;
//}

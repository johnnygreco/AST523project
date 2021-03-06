/***************************************************

File: main.cc

Description:
This is the primary driver of the program. It reads in 
a runtime parameter file and executes the nested sampling 
algorithm, where the sampling is done with the multiple 
ellipsoidal sampling method of Feroz, Hobson, & Bridges (2009).

How to run the code:
The program takes the name of a runtime parameter file as 
a command-line argument. The file should be located in 
RunTimeFiles. For example, to run the lighthouse, 
egg-box, or Gaussian shells problems, enter the commands
below.

./multinest lighthouse.txt
./multinest eggbox.txt
./multinest gauss_shells.txt

All relevant parameters such as the number of active 
points are set in the runtime files. 

To do: 
The main function is far too complicated. We plan to 
make some changes to the data structure, which will allow 
for a much more compact main function. As it stands, 
this code is not ready for more general use. 

Programmers: Johnny Greco & Johannes Rothe
Contacts: greco@princeton.edu, jrothe@princeton.edu

****************************************************/
#include "Point.h"
#include "Data.h"
#include "Samplers.h"

int main(int argc, char *argv[])
{

    if (argc != 2)   // check for command-line arguments
    {
        cout << "usage: " << argv[0] << " problem"<< endl;
        exit (1);   
    }

    cout << "getting runtime parameters" << endl;

    // **** get runtime parameters
    string runtime_filename = argv[1];
    runtime_filename = "RunTimeFiles/"+runtime_filename;
    int N, D, num_cols;
    double e, RepartitionFactor;
    ifstream runtime_file(runtime_filename.c_str());
    string datafile_name; 
    runtime_file.ignore(256, ':');
    runtime_file >> datafile_name;
    runtime_file.ignore(256, ':');
    runtime_file >> num_cols;
    runtime_file.ignore(256, ':');
    runtime_file >> D;
    runtime_file.ignore(256, ':');
    runtime_file >> N;
    runtime_file.ignore(256, ':');
    runtime_file >> e;
    runtime_file.ignore(256, ':');
    runtime_file >> RepartitionFactor;
    vector<string> prior_types(D);
    vector<double> min_vals(D), max_vals(D);
    for(int i = 0; i<D; i++)
    {
        runtime_file.ignore(256, ':');
        runtime_file >> prior_types[i];  
        runtime_file >> min_vals[i];  
        runtime_file >> max_vals[i];  
    }
    runtime_file.close();
    // **************************
    
    Data data_obj(D, num_cols, datafile_name); // create data object
    if(num_cols != 0) {data_obj.get_data();} // **** get data if necessary
    Samplers sampler(D, e); // create sampler object
    srand(time(NULL));  // seed random number generator
    
    cout << "creating " << N << " active points in " << D << " dimensions" << endl;

    // **** create N active points and set params
    vector <Point *> pts(N); 
    for(int j=0; j<N; j++)
    {
        pts[j] = new Point(D); 
        pts[j]->set_params(prior_types, min_vals, max_vals);
        pts[j]->hypercube_prior();
        pts[j]->transform_prior();
        data_obj.logL(pts[j]);
    }
    // ******************************************

    cout << "running MultiNest algorithm... this may take a few minutes" << endl;

    // ****************************************** start nested sampling algorithm 
    double logZ = -DBL_MAX;
    double logwidth, logZnew, logZ_err, Xtot; 
    double logLmin, H=0.0;
    double logLmax = -999.9;
    double logL_tmp = 0.0;
    int NumRecluster = 0;
    int j, nest, worst, copy;
    nest = 0;
    list<Point *> discard_pts; // list of Point objects to sample posterior 

    do
    {
        // calculate log(delta_X) using the trapezium rule
        logwidth = log(0.5) + log(exp(-(nest-1)/N) - exp(-(nest+1)/N)); 

        Xtot = exp(-nest/N); // total remaining prior mass
        worst = 0; 
        for(j=1; j<N; j++) // for lowest and highest logL
        {
            logL_tmp = pts[j]->get_logL();
            if(logL_tmp < pts[worst]->get_logL()) worst = j; 
            if(logL_tmp > logLmax) logLmax = logL_tmp;
        }
        logLmin = pts[worst]->get_logL();
        pts[worst]->set_logWt(logwidth + pts[worst]->get_logL()); // find contribution to evidence

        logZnew = PLUS(logZ, pts[worst]->get_logWt()); 
        H = exp(pts[worst]->get_logWt() - logZnew)*(pts[worst]->get_logL()) 
            + exp(logZ - logZnew)*(H + logZ) - logZnew;
        logZ = logZnew; // update global evidence
         
        discard_pts.push_back( new Point(*pts[worst]) ); // save discarded point for posterior sampling
        logLmin = pts[worst]->get_logL();

        
        // **************** ellipsoidal partitioning 
        if(nest==0 || sampler.ClusteringQuality(Xtot) > RepartitionFactor)  // recluster? 
        {
            sampler.ClearCluster();
            sampler.EllipsoidalPartitioning(pts, Xtot);
            sampler.CalcVtot();
            NumRecluster++;
        }
    	// **************** ellipsoidal sampling 
    	do
        {
            pts[worst]->set_u(sampler.get_newcoor());
            pts[worst]->transform_prior();
            data_obj.logL(pts[worst]);
        }
        while(logLmin > pts[worst]->get_logL());
    	// *********************************************
        
        nest++;
    }
    while(exp(log(Xtot) + logLmax - logZ) > THRESH); // stopping criterion
    // ****************************************** end nested sampling algorithm 

    // add final contribution to logZ from the remaining live points
    double X_last = exp(-(nest)/N);
    for(int i = 0; i<N; i++) {logZ += pts[i]->get_logL()*(X_last/N);}

    // ************* output results
    logZ_err = sqrt(H/N);
    cout << "job complete!" << endl;
    cout << "**** results ****" << endl;
    if(datafile_name == "lighthouse.dat"){data_obj.get_results(discard_pts, logZ);}
    cout << "number iterations = " << nest << endl;
    cout << "number reclusters = " << NumRecluster <<endl;
    cout << "information: H =  " << H/log(2.0) << " bits " << endl;
    cout << "global evidence: logZ = " << logZ << " +/- " << logZ_err << endl;
    cout << "****************" << endl;
    ofstream outfile;
    outfile.open("posterior_pdfs.dat");
    list<Point *>::iterator s;
    for(s=discard_pts.begin(); s!=discard_pts.end(); s++)
      outfile << (*s)->get_theta(0) << " " << (*s)->get_theta(1) << " " << (*s)->get_logL() << " " << exp((*s)->get_logL() - logZ) << endl;
    outfile.close();
    // ************* 

    for(int j=0; j<N; j++){delete pts[j];} 
    for(list<Point *>::iterator s=discard_pts.begin();s!=discard_pts.end();s++){delete *s;} 

    return 0;
}

#include "Data.h"

Data::Data(int Dim, int n_col, string filename) : data(n_col)
{
    D = Dim;
    num_col = n_col;
    data_filename = filename;
}

void Data::get_data()
{
    double dat;
    ifstream datafile(data_filename.c_str());


    while(!datafile.eof())
    {
        for(int i=0; i<num_col; i++)
        {
            datafile >> dat;
            data[i].push_back(dat);
        }
    }
    datafile.close();
}

void Data::get_results(list<Point *>& samples, double logZ)
{
    vector<double> x(D, 0.0);
    vector<double> xx(D, 0.0);

    double w;
    list<Point *>::iterator s;
    for(s=samples.begin(); s!=samples.end(); s++)
    {
      w = exp((*s)->get_logWt() - logZ);
        for(int i = 0; i<D; i++)
        {
            x[i] += w*((*s)->get_theta(i));
            xx[i] += w*((*s)->get_theta(i))*((*s)->get_theta(i));
        }
    }

    for(int i = 0; i<D; i++)
    {
        cout << "<x" << i+1 << "> = " << x[i] << " +/- " << sqrt(xx[i] - x[i]*x[i]) << endl;
    }
}

void Data::logL(Point* pt)
{
    if(data_filename == "lighthouse.dat")
    {
        double x, y;
        double logL = 0;
        x = pt->get_theta(0);
        y = pt->get_theta(1);
        for(datum=data[0].begin(); datum!=data[0].end(); datum++)
        {
            logL += log((y/M_PI) / ((*datum-x)*(*datum-x) + y*y));
        }
        pt->set_logL(logL);
    }
    else if (data_filename == "eggbox")
    {
        double x = 1.0;
        for(int i = 0; i < D; i++) {x *= cos(pt->get_theta(i)/2.0);}
        pt->set_logL(pow(2.0 + x, 5.0));

    }
    else if (data_filename == "gauss_shell")
    {
        vector<double> c1(D), c2(D);
        double r = 2.0;
        double ww = 0.1*0.1;
        double logL, arg1, arg2;
        double dist1_sq = 0.0, dist2_sq = 0.0;
        c1[0] = -3.0;
        c2[0] = 3.0;
        double Norm = 1.0/sqrt(2*M_PI*ww);
        for(int i = 1; i<D; i++){c1[i] = 0.0;}

        for(int i = 0; i<D; i++)
        {
            dist1_sq += pow(pt->get_theta(i) - c1[i], 2);
            dist2_sq += pow(pt->get_theta(i) - c2[i], 2);
        }

        arg1 = -pow(sqrt(dist1_sq) - r, 2)/(2*ww);
        arg2 = -pow(sqrt(dist2_sq) - r, 2)/(2*ww);

        if(arg1 > arg2) {logL = log(Norm) + arg1 + log(1.0 + exp(arg2 - arg1));}
        else {logL = log(Norm) + arg2 + log(1.0 + exp(arg1 - arg2));}

        pt->set_logL(logL);
    }
    else{cout << "cannot find likelihood function!" << endl;}
}

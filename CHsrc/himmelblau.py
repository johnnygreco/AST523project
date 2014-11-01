#!/usr/bin/env python
import numpy as np
import scipy as sp
import cfg_parse as cfgp
from dataio import readcolumn
class himmelblaumodel():
    def __init__(self,cfgfile):
        #self.infile_ = cfgp.File_parse(cfgfile,'infile')
        self.outfile_ = cfgp.File_parse(cfgfile,'outfile')
        self.inpath_ = cfgp.File_parse(cfgfile,'inpath')
        #self.data_ = []; readcolumn(self.data_,1,self.infile_); self.data_ = np.array(self.data_)
        self.D=2
        self.Np_=2000
        self.var0_=[0.,0.]
        self.varerr_=[5.,5.]
        self.repartition = 1.2
        self.NL_=0
        self.thresh = 0.05 #1.0*10**-2.5
        return
    
    def Getinitial(self):
        minvals = np.array(self.var0_)-np.array(self.varerr_)
        maxvals = np.array(self.var0_)+np.array(self.varerr_)
        guessvals = np.array(self.var0_)
        eff = 1.0
        return [minvals,maxvals,eff,self.Np_]


    def Get_L(self,model_params, logL, nl):
        self.NL_+=1
        for i in xrange(nl):
            x = model_params[i*self.D]
            y = model_params[i*self.D+1]
            L=-(x**2+y-11)**2-(x+y**2-7)**2
            logL[i] = L 
        return
    def Output(self,posterior, prob):
        fout = open(self.outfile_,mode='w')
        #print "output to %s" % self.outfile_
        #logLtemp = 0. 
        for i in xrange(posterior.shape[0]/self.D):
            for j in xrange(int(self.D)):
                fout.write('%f ' % posterior[i*self.D+j])
            logLtemp=np.array([0])
            self.Get_L(posterior[i*self.D:(i+1)*self.D],logLtemp,1)
            fout.write('%f %f\n' % (prob[i],logLtemp))
        fout.close()

#!/usr/bin/env python
import os
#os.chdir("oblateTransit")
#os.system("make clean")
#os.system("make all")
#os.chdir("..")
import sys
sys.path.append("oblateTransit")
#from transitmodel import transitmodel as Model #module that call occultquad, and also hold the data
#from lighthouse import lighthousemodel as Model
#from eggbox import eggboxmodel as Model
#from toygauss import toygaussmodel as Model
#from twingauss import twingaussmodel as Model
#from torus2d import torusmodel as Model
#from himmelblau import himmelblaumodel as Model
#from rosenbrock import rosenbrockmodel as Model
#from rastrigin import rastriginmodel as Model
#from poly import polymodel as Model
from gaussianshell import gaussianshellmodel as Model
#from objectdetection_single import obj_singlemodel as Model
#from occultquad import occultquad
from Point import Point
from Ellipsoid import Ellipsoid
from Samplers import Samplers as MNest
import numpy as np
def main():
    model = Model('example.cfg')
    minvals,maxvals,e,Np = model.Getinitial()
    sampler = MNest(minvals,maxvals,e,Np, "uniform")
    sampler.OutputClusters()
    print str(0)+"\t"+str(1)+"\t"+str(sampler.getVtot())+"\n"
    #what I intended to use, for sampler does not need to know the names
    #sampler = MNest(priortypes,minvals,maxvals,guessvals) 
    #print "# running MultiNest algorithm... this may take a few minutes\n"
    nest = 0
    Flag = True
    NumRecluster = 0
    NumLeval = Np
    Alltheta = np.zeros([model.D,int(Np)])
    theta = np.zeros(model.D)
    logL = np.zeros(Np)*1.0
    logLmax = 0.
    logLmin = 0.
    THRESH  = model.thresh #1.0*10**-2.5
    FullReclusterPeriod = int(np.sqrt(Np))
    SinceLastReclustering = 0
    sampler.getAlltheta(Alltheta)
    model.Get_L(Alltheta.ravel(),logL,Np)
    sampler.SetAllPoint(logL) 
    #print Alltheta[0,0],Alltheta[1,0]
    #Flag = False
    Debug = False
    count=0
    logzinfo = np.zeros(3)
    while Flag:

        X_i = np.exp(-1.0*nest/(1.0*Np))
        #discard and resample, get logLmax for convergence check as byproduct
        if(Debug):
            print '# ----------------------'
            ntotal = sampler.countTotal()
            posterior = np.zeros([model.D,ntotal])
            prob = np.zeros(ntotal)
            logWts = np.zeros(ntotal)
            sampler.getPosterior(posterior,prob,logWts)
            print "#",posterior
            print "#",prob
        
        sampler.DisgardWorstPoint(int(nest)) 
        if(Debug): 
            print '# ----------------------'
            ntotal = sampler.countTotal()
            posterior = np.zeros([model.D,ntotal])
            prob = np.zeros(ntotal)
            logWts = np.zeros(ntotal)
            sampler.getPosterior(posterior,prob,logWts)
            print "#",posterior
            print "#",prob

        logLmax = sampler.getlogLmax() 
        logLmin = sampler.getlogLmin()
        #print logLmax,logLmin
        FlagSample = True
        templogL = np.array([0.])
        #print 'before reset'
        while FlagSample:
            #print "ResetWorstPoint called with",theta
            sampler.ResetWorstPoint(theta) 
            model.Get_L(theta,templogL,1)
            NumLeval += 1
            #print theta, templogL,logLmin
            FlagSample=(logLmin > templogL[0])
            #FlagSample=False
        
        temp = templogL[0]
        #print templogL[0]
        sampler.ResetWorstPointLogL(temp) 
        #print 'after reset'
        if (Debug): 
            print '# ----------------------'
            ntotal = sampler.countTotal()
            posterior = np.zeros([model.D,ntotal])
            prob = np.zeros(ntotal)
            logWts = np.zeros(ntotal)
            sampler.getPosterior(posterior,prob,logWts)
            print "#",posterior
            print "#",prob

        sampler.CalcVtot()
        #if nest % 1000 == 0:
        #    sampler.OutputClusters()
        #    print str(nest)+"\t"+str(X_i)+"\t"+str(sampler.getVtot())+"\n"
            
        if sampler.ClusteringQuality(X_i) > model.repartition and SinceLastReclustering > FullReclusterPeriod:
            SinceLastReclustering = 0
            NumRecluster += 1
            #print "reclustering",nest,"..."
            sampler.FullRecluster(X_i)
            #sampler.MergeEllipsoids(X_i)
            #print "done."
            sampler.CalcVtot()
            sampler.OutputClusters()
            print str(nest)+"\t"+str(X_i)+"\t"+str(sampler.getVtot())+"\n"
            
        else:
            SinceLastReclustering += 1
        
        nest+=1 
        zold = logzinfo[1]
        sampler.getlogZ(logzinfo)

        #Flag = THRESH < abs(zold-logzinfo[1])

        #if(NumRecluster> 20):
        #    break

        #Flag = THRESH < abs(X_i*logLmax)
        Flag = THRESH < abs(X_i*np.exp(logLmax-logzinfo[1]))

    #print 'before output'
    #output
    ntotal = sampler.countTotal()
    print "# number of iterations = ",ntotal
    print "# sampling efficiency = ",1.0*ntotal/NumLeval 
    posterior = np.zeros([model.D,ntotal])
    prob = np.zeros(ntotal)
    logWts = np.ones(ntotal)*-999.9
    sampler.getPosterior(posterior,prob,logWts)
    print "# dimensions of posterior sampling = ",posterior.shape
    print "# number of likelihood evaluations = ",NumLeval
    #print "number of modellikelihood evaluations = ",model.NL_
    print "# number of reclusterings = ",NumRecluster
    logzinfo = np.zeros(3)
    sampler.getlogZ(logzinfo)
    model.Output(posterior.ravel(),prob,np.exp(logWts-logzinfo[1]))
    print "# information: H=%f bits" % logzinfo[0]
    print "# global evidence: logZ = %f +/- %f" % (logzinfo[1],logzinfo[2])
    #os.system("tail -n %d multivar.tab >> temp9.txt" % model.Np_)
    return

def simu():
    for i in xrange(100):
        main()
    return

def testL():
    model = Model('example.cfg')
    minvals,maxvals,e,Np = model.Getinitial()
    x = 4.0*np.random.randn(10000)-(minvals[0]+maxvals[0])/2.
    #x = 16.0*np.random.randn(10000)+(minvals[0]+maxvals[0])/2.
    #y = 16.0*np.random.randn(10000)+(minvals[0]+maxvals[0])/2.
    #x = 6.0*np.random.randn(10000)+(minvals[0]+maxvals[0])/2.
    #y = 6.0*np.random.randn(10000)+(minvals[0]+maxvals[0])/2.
    y = np.random.randn(10000)*4.0
    #lighthouse
    if(1):
        indexa = y>0.5
        indexb = y<1.5
        indexc = x>-2
        indexd = x<2
    #eggbox
    if(0):
        indexa = y>0
        indexb = y<31.415
        indexc = x>0
        indexd = x<31.415
    if(0):
        indexa = y>-6
        indexb = y<6
        indexc = x>-6
        indexd = x<6

    y = y[indexa*indexb*indexc*indexd]
    x = x [indexa*indexb*indexc*indexd]
    logL = np.zeros(len(y))
    theta = np.vstack((x,y)).ravel(order='F')
    #print theta.shape
    #.ravel()
    #print theta[0],theta[1],x[0],y[0]
    #return
    #print theta.shape
    #print x
    #return
    model.Get_L(theta,logL,len(y))
    for i in xrange(len(y)):
        print x[i],y[i],logL[i]


if __name__=='__main__':
    #simu()
    main()
    #testL()
    

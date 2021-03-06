multinest:multinest.o findenclosing.o sampleellipsoid.o cluster.o ellipsoid.o
	g++ -g -o multinest multinest.o findenclosing.o sampleellipsoid.o cluster.o ellipsoid.o -lgsl -lgslcblas -lm
multinest.o:multinest.cc
	g++ -g -c multinest.cc
sampleellipsoid:sampleellipsoid.o
	g++ -g -o sampleellipsoid sampleellipsoid.o -lgsl -lgslcblas -lm
sampleellipsoid.o:sampleellipsoid.cc
	g++  -g -c sampleellipsoid.cc
findenclosing:findenclosing.o
	g++ -g -o findenclosing findenclosing.o -lgsl -lgslcblas -lm
findenclosing.o:findenclosing.cc
	g++ -g -c findenclosing.cc
cluster:cluster.o
	g++ -g -o cluster cluster.o -lgsl -lgslcblas -lm
cluster.o:cluster.cc
	g++ -g -c cluster.cc
ellipsoid:ellipsoid.o
	g++ -g -o ellipsoid ellipsoid.o -lgsl -lgslcblas -lm
ellipsoid.o:ellipsoid.cc
	g++ -g -c ellipsoid.cc

clean:
	rm multinest.o multinest
	rm sampleellipsoid.o sampleellipsoid
	rm findenclosing.o findenclosing
	rm cluster.o cluster
	rm ellipsoid.o ellipsoid

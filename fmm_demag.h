/** \file fmm_demag.h
this header is the interface to scalfmm. Its purpose is to prepare an octree for the application of the fast multipole algorithm
*/

#include "tiny.h"

#ifndef IF_VERBOSE  // same goes for SYSTEM_ERROR
#error "fem.h" must be #included before "fmm_demag.h"
#endif

#ifndef FMM_DEMAG_H
#define FMM_DEMAG_H

/** double redefinition for the parametrization of some sczlfmm templates */
#define FReal double // pour FReal  *ct*


/**
\namespace fmm to grab altogether the templates and functions using scalfmm for the computation of the demag field 
*/
namespace fmm{
template <class CellClass, class ContainerClass, class LeafClass, class OctreeClass,
          class KernelClass, class FmmClass, typename... Args>

/**
initialization function for the building of an octree and a kernel passed to scalfmm to compute the demag field
*/
int init(Fem &fem, OctreeClass* &tree, KernelClass* &kernels, Args... kernelPreArgs)
{
//boost::timer time; // pas utilisé. *ct*

    FTic counter;
    const int NbLevels = 8; 
    const int SizeSubLevels = 6;
    const unsigned int NbThreads  =  omp_get_max_threads();

    omp_set_num_threads(NbThreads);
    IF_VERBOSE(fem) std::cout << "\n>> Using " << omp_get_max_threads() << " threads.\n" << std::endl;

    const double boxWidth=2.01;

//const FPoint centerOfBox(0., 0., 0.);
    const FPoint<double> centerOfBox(0., 0., 0.);// manque le typename du template FPoint *ct*

    // -----------------------------------------------------
    tree=new OctreeClass(NbLevels, SizeSubLevels, boxWidth, centerOfBox);
    if (!tree) SYSTEM_ERROR;
    // -----------------------------------------------------

// systeme pour nfft dans [-1, 1]
fem.fmm_normalizer = 2./fem.diam*0.999999;
double norm=fem.fmm_normalizer;

const int NOD = fem.NOD;
const int FAC = fem.FAC;
const int TET = fem.TET;

    IF_VERBOSE(fem){
    std::cout << "Creating & Inserting particles ..." << std::endl;
    std::cout << "\tHeight : " << NbLevels << " \t sub-height : " << SizeSubLevels << std::endl;
    }
    counter.tic();

int idxPart=0;
for (int i=0; i<NOD; i++, idxPart++){       // cibles (noeuds)
    double xTarget = (fem.node[i].x-fem.cx) * norm;
    double yTarget = (fem.node[i].y-fem.cy) * norm;
    double zTarget = (fem.node[i].z-fem.cz) * norm;
    FPoint<double> particlePosition(xTarget, yTarget, zTarget);// manque le typename du template FPoint ct
    tree->insert(particlePosition, FParticleType::FParticleTypeTarget, idxPart, 0.0);//ct
    }

for (int t=0; t<TET; t++){       // sources de volume
    Tet &tet = fem.tet[t];
    const int N = Tet::N;
    const int NPI = Tet::NPI;
    double nod[3][N], gauss[3][NPI];
    for (int i=0; i<N; i++){
        int i_= tet.ind[i];
        nod[0][i] = fem.node[i_].x;
        nod[1][i] = fem.node[i_].y;
	nod[2][i] = fem.node[i_].z;
	}
    tiny::mult<double, 3, N, NPI> (nod, tet.a, gauss);

    for (int j=0; j<NPI; j++, idxPart++){
        double xSource = (gauss[0][j]-fem.cx) * norm;
        double ySource = (gauss[1][j]-fem.cy) * norm;
	double zSource = (gauss[2][j]-fem.cz) * norm;
        FPoint<double> particlePosition(xSource, ySource, zSource);// manque le typename du template FPoint ct
        tree->insert(particlePosition, FParticleType::FParticleTypeSource, idxPart, 0.0);//ct
	}
    }

for (int f=0; f<FAC; f++){        // sources de surface
    Fac &fac = fem.fac[f];
    const int N = Fac::N;
    const int NPI = Fac::NPI;
    double nod[3][N], gauss[3][NPI];
    for (int i=0; i<N; i++){
        int i_= fac.ind[i];
        nod[0][i] = fem.node[i_].x;
        nod[1][i] = fem.node[i_].y;
	nod[2][i] = fem.node[i_].z;
	}
    tiny::mult<double, 3, N, NPI> (nod, fac.a, gauss);

    for (int j=0; j<NPI; j++, idxPart++){
        double xSource = (gauss[0][j]-fem.cx) * norm;
        double ySource = (gauss[1][j]-fem.cy) * norm;
	double zSource = (gauss[2][j]-fem.cz) * norm;
        FPoint<double> particlePosition(xSource, ySource, zSource);// manque le typename du template FPoint ct
        tree->insert(particlePosition, FParticleType::FParticleTypeSource, idxPart, 0.0);//ct
	}
    }
    counter.tac();
    IF_VERBOSE(fem){
    std::cout << "Done  " << "(@Creating and Inserting Particles = " << counter.elapsed() << "s)." << std::endl;

    std::cout << "Create kernel ..." << std::endl;
    }
    counter.tic();

    kernels=new KernelClass( kernelPreArgs... , NbLevels, boxWidth, centerOfBox);
    if (!kernels) SYSTEM_ERROR;

    counter.tac();
    IF_VERBOSE(fem) std::cout << "Done  " << " in " << counter.elapsed() << "s)." << std::endl;

return 0;
}

/**
computes potential using fem struct
*/
template <int Hv> double potential(Fem &fem, Fac &fac, int i);


/**
computes the demag field
*/
template <int Hv, class CellClass, class ContainerClass, class LeafClass, class OctreeClass,
          class KernelClass, class FmmClass, typename... Args>
void demag(Fem &fem, OctreeClass *tree, KernelClass *kernels, Args... kernelPreArgs)
{
        // changed return type to void, because nothing is returned (bcarvello, 2017)

FTic counter;
FmmClass algo(tree, kernels);

const bool analytic_corr = true;

IF_VERBOSE(fem) cout << "\t magnetostatics ..................... ";

const int NOD = fem.NOD;
const int FAC = fem.FAC;
const int TET = fem.TET;
const int SRC = fem.SRC;

FReal *srcDen=(FReal*) new FReal[SRC]; if (!srcDen) SYSTEM_ERROR;
memset(srcDen, 0, SRC*sizeof(FReal));

FReal *corr=(FReal*) new FReal[NOD]; if (!corr) SYSTEM_ERROR;
memset(corr, 0, NOD*sizeof(FReal));

int nsrc = 0;

/*********************** TETRAS *********************/
for (int t=0; t<TET; t++){
    Tet &tet = fem.tet[t];
    const int N   = Tet::N;
    const int NPI = Tet::NPI;
    pair <string,int> p = make_pair("Js",tet.reg);
    double Ms = nu0 * fem.param[p];

   /*---------------- INTERPOLATION ---------------*/
    double u_nod[3][N];
    double dudx[3][NPI], dudy[3][NPI], dudz[3][NPI];

    for (int i=0; i<N; i++) {
        int i_= tet.ind[i];
        Node &node = fem.node[i_];
        for (int d=0; d<3; d++)
            u_nod[d][i] = (Hv? node.v[d]: node.u[d]);
        }

	tiny::mult<double, 3, N, NPI> (u_nod, tet.dadx, dudx);
	tiny::mult<double, 3, N, NPI> (u_nod, tet.dady, dudy);
	tiny::mult<double, 3, N, NPI> (u_nod, tet.dadz, dudz);
   /*-----------------------------------------------*/

    for (int j=0; j<NPI; j++, nsrc++){
        double div_u = dudx[0][j] + dudy[1][j] + dudz[2][j];
        double q = -Ms * div_u * tet.weight[j];

        srcDen[nsrc] = q;
        }
 }


/************************ FACES **************************/
for (int f=0; f<FAC; f++){
    Fac &fac = fem.fac[f];
    const int N    = Fac::N;
    const int NPI  = Fac::NPI;
    double Ms = fac.Ms;
    double nx,ny,nz;
    nx=fac.nx; ny=fac.ny; nz=fac.nz;

    /** calc u gauss **/  
    double u_nod[3][N], u[3][NPI];
        for (int i=0; i<N; i++){
        int i_= fac.ind[i];
        Node &node = fem.node[i_];
        for (int d=0; d<3; d++)
            u_nod[d][i] = (Hv? node.v[d]: node.u[d]);
        }

    tiny::mult<double, 3, N, NPI> (u_nod, fac.a, u);

    /** calc sigma, fill distrib.alpha **/
    for (int j=0; j<NPI; j++, nsrc++){
        double un = u[0][j]*nx + u[1][j]*ny + u[2][j]*nz;
        double s = Ms * un * fac.weight[j];
        srcDen[nsrc] =  s; 
        }

    if (analytic_corr) {
      /** calc coord gauss **/
      double nod[3][N], gauss[3][NPI];
      for (int i=0; i<N; i++) {
	      int i_= fac.ind[i];
	      //Node &node = fem.node[i_]; // inutilisé *ct*
	      nod[0][i] = fem.node[i_].x;
	      nod[1][i] = fem.node[i_].y;
	      nod[2][i] = fem.node[i_].z;
          }
      tiny::mult<double, 3, N, NPI> (nod, fac.a, gauss);

      /** calc corr node by node **/
      for (int i=0; i<N; i++) {
	      int i_= fac.ind[i];
	      Node &node = fem.node[i_];
	      double x,y,z;
	      x=node.x;  y=node.y;  z=node.z;
	      for (int j=0; j<NPI; j++) {
	          double xg,yg,zg;
	          xg=gauss[0][j];  yg=gauss[1][j];  zg=gauss[2][j];
	          double rij = sqrt( sq(x-xg) + sq(y-yg) + sq(z-zg) );
	          double sj = Ms* ( u[0][j]*nx + u[1][j]*ny + u[2][j]*nz );
	          corr[i_]-= sj/rij * fac.weight[j];
	          }
	      corr[i_]+= potential<Hv>(fem, fac, i);
          }
      }
   }

fflush(NULL);

    { // reset potentials and forces - physicalValues[idxPart] = Q

    tree->forEachLeaf([&](LeafClass* leaf){
	const int nbParticlesInLeaf = leaf->getSrc()->getNbParticles();
	//const FVector<int>& indexes = leaf->getSrc()->getIndexes(); // *ct*
	const FVector<long long>& indexes = leaf->getSrc()->getIndexes(); // pas int mais long long  *ct*
	FReal* const physicalValues = leaf->getSrc()->getPhysicalValues();
	memset(physicalValues, 0, nbParticlesInLeaf*sizeof(FReal));

	for(int idxPart = 0 ; idxPart < nbParticlesInLeaf ; ++idxPart){
	    const int indexPartOrig = indexes[idxPart];
            const int nsrc = indexPartOrig-NOD;
 	    assert((nsrc>=0) && (nsrc<SRC));
	    physicalValues[idxPart]=srcDen[nsrc];
	    }
        });

    tree->forEachLeaf([&](LeafClass* leaf){
	FReal*const potentials = leaf->getTargets()->getPotentials();
	const int nbParticlesInLeaf = leaf->getTargets()->getNbParticles();
	memset(potentials, 0, nbParticlesInLeaf*sizeof(FReal));
        });


    tree->forEachCell([&](CellClass* cell){
	cell->resetToInitialState();
        });

    }// end reset

counter.tic();
algo.execute();
 counter.tac();
IF_VERBOSE(fem) std::cout << "Done  " << "(@Algorithm = " << counter.elapsed() << "s)." << std::endl;

double norm = fem.fmm_normalizer;

    tree->forEachLeaf([&](LeafClass* leaf){
        const FReal*const potentials = leaf->getTargets()->getPotentials();
        const int nbParticlesInLeaf  = leaf->getTargets()->getNbParticles();
        //const FVector<int>& indexes  = leaf->getTargets()->getIndexes(); // *ct*
	const FVector<long long>& indexes  = leaf->getTargets()->getIndexes(); // int -> long long *ct*

        for(int idxPart = 0 ; idxPart < nbParticlesInLeaf ; ++idxPart){
	    const int indexPartOrig = indexes[idxPart];

	    if (Hv) 
       		fem.node[indexPartOrig].phiv = (potentials[idxPart]*norm + corr[indexPartOrig])/(4*M_PI);
	    else 
       		fem.node[indexPartOrig].phi  = (potentials[idxPart]*norm + corr[indexPartOrig])/(4*M_PI);
        }
    });

delete [] srcDen;
delete [] corr;
}

template <int Hv>
double potential(Fem &fem, Fac &fac, int i) // pourquoi c'est un template??? Hv n'est pas utilisé 
{
  double nx,ny,nz,Ms;
  nx=fac.nx;  ny=fac.ny;  nz=fac.nz; Ms=fac.Ms;

 int ii  = (i+1)%3;
 int iii = (i+2)%3;

 int i_,ii_,iii_;
 i_=fac.ind[i];  ii_=fac.ind[ii];  iii_=fac.ind[iii];

 double x1,x2,x3, y1,y2,y3, z1,z2,z3;
 Node &node1 = fem.node[i_];
 Node &node2 = fem.node[ii_];
 Node &node3 = fem.node[iii_];
 x1=node1.x;  x2=node2.x;  x3=node3.x;
 y1=node1.y;  y2=node2.y;  y3=node3.y;
 z1=node1.z;  z2=node2.z;  z3=node3.z;

 double b,t,h;
 b = sqrt( sq(x2-x1) + sq(y2-y1) + sq(z2-z1) );
 t = (x2-x1)*(x3-x1) + (y2-y1)*(y3-y1) + (z2-z1)*(z3-z1);
 h = 2.*fac.surf;
 t/=b;  h/=b;
 double a = t/h;  double c = (t-b)/h;

double s1, s2, s3;
if (Hv) {
	s1 = node1.v[0]*nx + node1.v[1]*ny + node1.v[2]*nz;
	s2 = node2.v[0]*nx + node2.v[1]*ny + node2.v[2]*nz;
	s3 = node3.v[0]*nx + node3.v[1]*ny + node3.v[2]*nz;
   }
else {
	s1 = node1.u[0]*nx + node1.u[1]*ny + node1.u[2]*nz;
	s2 = node2.u[0]*nx + node2.u[1]*ny + node2.u[2]*nz;
	s3 = node3.u[0]*nx + node3.u[1]*ny + node3.u[2]*nz;
   }

 double l = s1;
 double j = (s2-s1)/b;
 double k = t/b/h*(s1-s2) + (s3-s1)/h;

 double cc1 = c*c+1;
 double r = sqrt(h*h + (c*h+b)*(c*h+b));
 double ll = log( (cc1*h + c*b + sqrt(cc1)*r) / (b*(c+sqrt(cc1))) );

 double pot1, pot2, pot3, pot;
 pot1 = b*b/pow(cc1,1.5)*ll + c*b*r/cc1 + h*r - c*b*b/cc1 - sqrt(a*a+1)*h*h;
 pot1*= j/2.;

 pot2 = -c*b*b/pow(cc1,1.5)*ll + b*r/cc1 - h*h/2. + h*h*log(c*h+b+r) - b*b/cc1;
 pot2*= k/2.;

 pot3 = h*log(c*h+b+r) - h + b/sqrt(cc1)*ll;
 pot3*= l;

 pot = pot1 + pot2 + pot3 + h*(k*h/2.+l)*(1-log(h*(a+sqrt(a*a+1)))) - k*h*h/4.;

 return Ms*pot;
}
}
#endif

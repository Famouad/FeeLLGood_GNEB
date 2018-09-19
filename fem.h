/** \file fem.h
\brief principal header, contains the struct fem <br>
This file is called by almost all source files in feellgood project <br>
It does also contains the definition of many constants for the solver, and for scalfmm 
*/

#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <utility>
#include <cmath>
#include <cstdlib>
#include <cassert>
#include <list>

#include <time.h>
#include <sys/times.h>
#include <unistd.h>

/* only pull GSL if needed */
#ifdef STAT
	#include <gsl/gsl_rng.h>
	#include <gsl/gsl_histogram.h>
	const size_t NCLASSES=100;/**< number of BIN for statistics */
	const double HMAX=1e6;/**< maximum value */
#endif //endif STAT

#include "config.h"
#include "feellgoodSettings.h"


#include "ANN.h" // ct "ANN/ANN.h"		// ANN declarations

#include "tiny.h"
#include "pt3D.h"
#include "tetra.h"
#include "facette.h"

#include "node.h"

const bool U = true;/**< used as a template parameter */
const bool V = false;/**< used as a template parameter */

const int D = 3;         /**< dimension, required by ANN for kdTree */
const double invD = 1./(double)D;/**< convenient const value */




/** \struct Stat
used to build some statistics, with GSL library
*/
struct Stat{
#ifdef STAT
    gsl_histogram* h;/**< pointer to GSL histogram */
#else
    void* h;  /**< placeholder, we don't include GSL */
#endif
    double M; /**< M defined by \f$ M = \alpha/ \tau_R |log(dt/ \tau_R)| \f$ */
    double R; /**< R defined by \f$ R = dt/\tau_R*|log(dt/\tau_R)| \f$ */
    double r; /**< r defined by \f$ r = 0.5*dt/\tau_R*|log(dt/\tau_R)| \f$ */
    };

/** \struct Fem
massive container to grab altogether all parameters of a simulation, including mesh geometry, containers for the mesh
*/
struct Fem{
	//int REG; /**< number of regions in the msh file */
	int NOD;/**< number of nodes in the corresponding container */
	int FAC;/**< number of faces in the corresponding container */
	int TET;/**< number of tetrahedron in the corresponding container */
	int SRC;/**< number of sources for scalfmm */
	int SEQ;/**< number of sequences, usefull to define a vector applied field eventually varying in time */
	Pt::pt3D c;/**< center position */	
	Pt::pt3D l;/**< lengths along x,y,z axis */	
	
	double diam;/**< max of l coordinates, to define a bounding box */
	double surf;/**< total surface */
	double vol;/**< total volume of the mesh */
	
	double fmm_normalizer;/**< normalizing geometrical constant for the computation of the demag field */
	
	double locmax;/**< no idea */
	double t;/**< physical current time of the simulation */
	
	double vmax;/**< maximum speed of magnetization */

	double E0[4];/**< table to store initial energy values <br> 
index convention : 0-exchange 1-anisotropy 2-demagnetizing 3-applied */

	double E[4]; /**< table to store energy values at time t <br>
index convention : 0-exchange 1-anisotropy 2-demagnetizing 3-applied */

	double DW_vz0;/**< initial speed of the domain wall (to check) */
	double DW_vz;/**< speed of the domain wall along z */
	double DW_dir;/**< direction of the domain wall (to check) */
	double DW_z; /**< domain wall displacement along Oz */
	
	double Etot0;/**< initial total energy (to check) */
	double Etot;/**< total energy */
	double evol;/**< increment dE for dt */
	double phy;/**< no idea */
	std::vector <Node> node; /**< node container */
	std::vector <Facette::Fac>  fac; /**< face container */
	std::vector <Tetra::Tet>  tet; /**< tetrahedron container */
    double Bext;/**< amplitude of the applied field */
    triple Hext;/**< external applied field direction (should be normalized) */
    

#ifdef STAT
	Stat stat;/**< to build some histograms  */
	//void savestat(Fem &fem, int nt);/**< file writing function for the statistics (if any) */
#endif

    ANNkd_tree* kdtree;/**< a kdtree to find efficiently the closest set of nodes to a physical point in the mesh  */
    ANNpointArray pts;/**< container for the building of the kdtree (handled by ANN library) */
    




/**
print some informations of fem container
*/
void affichage(void);

/**
reset the nodes struct to restart another step time simulation
Undo the action of one or many "vsolve" runs in case of failure.
Demagnetizing field and energies don't need to be reset, because they won't be updated if failure is detected.
I don't know how to cleanly reset "fem.DW_vz". BC
*/
inline void reset(void) { std::for_each(node.begin(),node.end(),[](Node &n) {n.reset();}); }

/**
time evolution : one step in time
*/
inline void evolution(void)
{
std::for_each(node.begin(), node.end(), [](Node &n){ n.evolution();} );
for (int e=0; e<4; e++) { E0[e] = E[e]; }
Etot0 = Etot;
}

/**
computes the hat functions for all containers
*/
void chapeaux(void); 

/**
computes an analytical initial magnetization distribution as a starting point for the simulation
*/
inline void init_distrib(void)
	{ std::for_each( node.begin(),node.end(), [](Node &n) { n.u.x(1./sqrt(2.)); n.u.y(0.); n.u.z(1./sqrt(2.)); n.phi  = 0.;} ); }

/**
read a solution from a file (tsv formated) and initialize fem struct to restart computation from that distribution
*/
void restoresol(double scaling /**< [in] scaling factor for physical coordinates */,
	std::string *filename /**< [in] */ );


/** \struct Regions
contains two maps of the volume regions and surfaces numbers
*/

//struct Regions{
//    std::map <int,int> surfaces;/**< map for the surfaces region generated by gmsh */
//    std::map <int,int> volumes;/**< map for the volumes region generated by gmsh */
//    };

/** reading file function */
void lecture(Settings &mySets, double scale);//, Regions *regions);

/** initialize pts,kdtree,l,c,diam in fem struct */
void femutil_node(void);


/** decomposition of the tetrahedrons in surface elements; calculation of the normal vectors to the face */
void femutil_facMs(Settings &settings /**< [in] */);


/** utilitary function to call all femutil_xxxxx functions <br>
calculation of the volumes and reorientation of the tetrahedrons if needed in fem struct with the following convention :

                        v
                      .
                    ,/
                   /
                2(ic)                                 2
              ,/|`\                                 ,/|`\
            ,/  |  `\                             ,/  |  `\
          ,/    '.   `\                         ,6    '.   `5
        ,/       |     `\                     ,/       8     `\
      ,/         |       `\                 ,/         |       `\
     0(ia)-------'.--------1(ib) --> u     0--------4--'.--------1
      `\.         |      ,/                 `\.         |      ,/
         `\.      |    ,/                      `\.      |    ,9
            `\.   '. ,/                           `7.   '. ,/
               `\. |/                                `\. |/
                  `3(id)                                `3
                     `\.
                        ` w

 and calculation of all elementary surfaces and total surface in the face container in fem struct */
void femutil(Settings &settings);

/** find direction of motion of DW */
void direction(void);

/** computes energies stored in E table */
void energy(Settings &settings);

/** recentering algorithm for the study of the motion of an object, for example a domain wall. Mesh must be adequate. */
bool recentrage(double thres/**< [in] translation parameter */,double mz /**<[in] average magnetization along z */);

/** saving function for a solution */
void saver(Settings &settings, std::ofstream &fout, int nt);

/** text file (vtk) writing function for a solution */
void savecfg_vtk(std::string baseName, int nt, std::string *filename);

/** text file (tsv) writing function for a solution */
void savesol(std::string baseName,double s, int nt, std::string *filename);

/** save the field values */
void saveH(std::string baseName,double scale, int nt);


/** 
template to compute average of either u or v on the whole set of tetetrahedron
*/
template <int UorV>
double moy(int d)
{
double sum = 0.;
for (int i_t=0; i_t<TET; i_t++){
    Tetra::Tet &te = tet[i_t];
    double val_nod[Tetra::N], val[Tetra::NPI];
    for (int ie=0; ie<Tetra::N; ie++) 
	{
        int i = te.ind[ie];
        Node &n = node[i];
	if(UorV)        
		val_nod[ie] = n.u(d);
	else val_nod[ie] = n.v(d); 
        }
   tiny::transposed_mult<double, Tetra::N, Tetra::NPI> (val_nod, te.a, val);
   sum += tiny::sp<double, Tetra::NPI> (val, te.weight);
   }

return sum/vol;
}


}; // end struct fem definition



double cputime();/**< convenient function to compute the duration of a simulation */










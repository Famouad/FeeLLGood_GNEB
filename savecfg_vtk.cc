#include "fem.h"

using namespace std;

void Fem::savecfg_vtk(string baseName, int nt, string *filename)  // filename may be NULL
{
string str;

if (filename) { str = *filename; }
else{
    str = baseName + "_" + to_string(SEQ) + "_B" + to_string(Bext) + "_iter" + to_string(nt) + ".vtk";
 //<< boost::format("_%d_B%6f_iter%d.vtk") % fem.SEQ % fem.Bext % nt;
    }

if(VERBOSE) { cout <<"\n -------------------" << endl << " " << str << endl; }

ofstream fout(str, ios::out);
if (!fout){
    if(VERBOSE) cerr << "pb ouverture fichier : " << str << "en ecriture" << endl;
    SYSTEM_ERROR;}


fout << "# vtk DataFile Version 2.0" << endl;
fout << "time : " << t << endl; // boost::format(" time : %+20.10e") % fem.t;
fout << "ASCII" << endl;
fout << "DATASET UNSTRUCTURED_GRID" << endl;
fout << "POINTS "<< NOD << " float" << endl;

for (int i=0; i<NOD; i++){
fout << node[i].p.x() << "\t" << node[i].p.y() << "\t" << node[i].p.z() + DW_z << endl;
//fout << boost::format("%+20.10e %+20.10e %+20.10e") % x % y % z << endl;
    }

fout << "CELLS " << setw(8) << TET << "\t" << setw(8) << (5*TET) << endl;

for (int t=0; t<TET; t++){
    fout << setw(8) << Tetra::N;
    for (int i=0; i<Tetra::N; i++) { fout << setw(8) << tet[t].ind[i]; }
    fout << endl;
    }

fout << "CELL_TYPES " << setw(8) << TET << endl;

for (int t=0; t<TET; t++) { fout << setw(8) << 10 << endl; }

fout <<"POINT_DATA " << setw(8) << NOD << endl;
fout <<"SCALARS phi float 1" << endl;
fout << "LOOKUP_TABLE my_table" << endl;

for (int i=0; i<NOD; i++) { fout << node[i].phi << endl; }

fout << "VECTORS u float" << endl;

for (int i=0; i<NOD; i++){
    //fout << boost::format("%+20.10e %+20.10e %+20.10e") % u1 % u2 % u3 << endl;
fout << node[i].u << endl;
    }
}    

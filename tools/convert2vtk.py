#!/usr/bin/env python

import vtk
#from vtk.numpy_interface import dataset_adapter as dsa
import sys
import os
import numpy as np

class mesh(object):
    def __init__ (self,fileName):
        self.Nodes = []
        self.Tet = []
        mshFile = open(fileName,'r')
        count = 0
        current_line =''
        while current_line.strip() != '$MeshFormat':
            current_line = mshFile.readline()

        mshFormat = mshFile.readline()
        if mshFormat.strip() != "2.2\t0\t8\n":
            print("mesh format = 2.2")
        else:
            print("cannot read " + fileName + "file, wrong format, 2.2 required.")
            sys.exit(1)

        while current_line.strip() != '$Nodes':
            current_line = mshFile.readline()
        current_line = mshFile.readline()
        
        while current_line.strip() != '$EndNodes':
            current_line = mshFile.readline()
            ls = current_line.strip().split("\t")
            if len(ls) == 4:
                self.Nodes.append( [float(ls[1]),float(ls[2]),float(ls[3]) ])
        current_line = mshFile.readline()
        current_line = mshFile.readline()
        
        while current_line.strip() != '$EndElements':
            current_line = mshFile.readline()
            ls = current_line.strip().split("\t")
            if len(ls) == 9: # we only consider tetrahedrons, assuming there is no other objects than triangles and tetrahedrons in the Elements list
                self.Tet.append( [int(ls[5])-1,int(ls[6])-1,int(ls[7])-1,int(ls[8])-1 ])
        mshFile.close()
        print("mesh read from file " + fileName)

    def infos(self):
        print("nb Nodes: ",len(self.Nodes),"\tnb Tetrahedrons: ",len(self.Tet))

class data(object):
    def __init__(self,fileName):
        self.Mag = vtk.vtkFloatArray()
        self.Mag.SetNumberOfComponents(3)
        magFile = open(fileName,'r')
        lines = magFile.readlines()        
        # first .sol line is always "#t = xxx", so it is skipped starting from i=1
        for i in range(1,len(lines)):
            ls = lines[i].strip().split('\t')
            self.Mag.InsertNextTuple([float(ls[4]),float(ls[5]),float(ls[6])])
        magFile.close()

def get_params():
    import argparse
    description = 'convert .sol to .vtk using gmsh mesh.'
    epilogue = '''
 convert feellgood output .sol files to .vtk file using mesh in gmsh version 2.2 format 
   '''
    parser = argparse.ArgumentParser(description=description, epilog=epilogue,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('mshFileName', help='myMeshFile.msh')
    parser.add_argument('solFileName', help='myNumSolution.sol')
    args = parser.parse_args()
    return [args.mshFileName,args.solFileName]

def main():
    print("This is convert2vtk python script, using vtk version " + vtk.vtkVersion.GetVTKVersion())
    FileNames = get_params()
    path, ext = os.path.splitext(FileNames[0])
    ext = ext.lower()
    if ext == '.msh':
        print("input mesh file is ", FileNames[0])
    
    print("input sol file name =", FileNames[1])
    
    msh = mesh(FileNames[0])
    msh.infos()

    my_data = data(FileNames[1])
    
    points = vtk.vtkPoints()
    points.SetNumberOfPoints(len(msh.Nodes))

    for i in range(0,len(msh.Nodes)):
        points.InsertPoint(i,msh.Nodes[i])
    
    ugrid = vtk.vtkUnstructuredGrid()
    ugrid.SetPoints(points)

# here we insert all the tetraherons, in some append mode, so no need to call ugrid.Allocate
    for i in range(0,len(msh.Tet)):
        ugrid.InsertNextCell(vtk.VTK_TETRA,4,msh.Tet[i])

    #the vectors are fixed on each nodes, so we have to associate them to ugrid as Points, not to the cells = tetrahedrons
    ugrid.GetPointData().SetVectors(my_data.Mag) 

    writer = vtk.vtkXMLUnstructuredGridWriter()
    newFileName = FileNames[1].split(".")[0]
    newFileName += ".vtu"
    writer.SetFileName(newFileName)
    writer.SetInputData(ugrid)
    
    #writer.SetDataModeToAscii() #only if XML
    writer.Write()
    print("VTK file " + newFileName + " written.")

if __name__ == "__main__":
    main()

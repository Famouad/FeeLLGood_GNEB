#include <fstream>
#include <algorithm>
#include <gmsh.h>

#include "feellgoodSettings.h"
#include "mesh.h"
#include "tags.h"

namespace Mesh
    {
bool mesh::checkMeshFile(Settings const &mySets)
    {
    using namespace tags::msh;
    bool existRegions(true);
    
    gmsh::initialize();
    gmsh::option::setNumber("General.Terminal",0); // to silent gmsh
    gmsh::open(mySets.getPbName());
    
    int msh_dim = gmsh::model::getDimension();
    
    bool msh_available = (msh_dim == DIM_OBJ_3D);
    
    std::vector<int> elemTypes;
    gmsh::model::mesh::getElementTypes(elemTypes, DIM_OBJ_3D, -1);
    msh_available = msh_available && (elemTypes.size() == 1); // only one type of 3D element allowed
    msh_available = msh_available && (elemTypes[0] == TYP_ELEM_TETRAEDRON); // only Tetrahedrons

    gmsh::model::mesh::getElementTypes(elemTypes, DIM_OBJ_2D, -1);
    msh_available = msh_available && (elemTypes.size() == 1); // only one type of 2D element allowed
    msh_available = msh_available && (elemTypes[0] == TYP_ELEM_TRIANGLE); // only Triangles
    
    std::vector<std::pair<int, int> > physGroups2D;
    gmsh::model::getPhysicalGroups(physGroups2D,DIM_OBJ_2D);
    std::vector<std::string> names2D;
    std::for_each(physGroups2D.begin(),physGroups2D.end(),[&names2D]( std::pair<int, int> &pGroup)
        {
        std::string physGroupName;
        gmsh::model::getPhysicalName(pGroup.first, pGroup.second, physGroupName);
        names2D.push_back(physGroupName);
        });
    
    std::for_each(mySets.paramFacette.begin(),mySets.paramFacette.end(),[&existRegions,&names2D](auto p)
                  {
                  if (p.regName != "__default__")
                      {
                      existRegions = existRegions && std::any_of(names2D.begin(),names2D.end(),
                          [&p] (std::string &elem) { return p.regName == elem; } );
                      } 
                  } );
    
    std::vector<std::pair<int, int> > physGroups3D;
    gmsh::model::getPhysicalGroups(physGroups3D,DIM_OBJ_3D);
    std::vector<std::string> names3D;
    std::for_each(physGroups3D.begin(),physGroups3D.end(),[&names3D]( std::pair<int, int> &pGroup)
        {
        std::string physGroupName;
        gmsh::model::getPhysicalName(pGroup.first, pGroup.second, physGroupName);
        names3D.push_back(physGroupName);
        });
        
    std::for_each(mySets.paramTetra.begin(),mySets.paramTetra.end(),[&existRegions,&names3D](auto p)
                  {
                  if (p.regName != "__default__")
                      {
                      existRegions = existRegions && std::any_of(names3D.begin(),names3D.end(),
                          [&p] (std::string &elem) { return p.regName == elem; } );
                      }
                  } );
    gmsh::clear();
    gmsh::finalize();
    return (existRegions && msh_available);
    }

void mesh::readMesh(Settings const &mySets)
    {
    if (mySets.verbose)
        {
        std::cout << "Reading mesh file " << mySets.getPbName() << ":\n";
        }
    std::string symb;
    std::ifstream msh(mySets.getPbName());

    on_fail_msg_error(msh, "cannot open file " + mySets.getPbName() );

    msh >> symb;
    if (symb == tags::msh::format)
        {
        std::string mshFormat = "";
        msh >> mshFormat;
        if (mshFormat == tags::msh::version)
            {
            int _tags, reg, TYP,nbRegNames;

            if (tags::lookFor(false, msh, tags::msh::begin_physical_names))
                {
                msh >> nbRegNames;

                while ((msh >> symb) && (symb != tags::msh::end_physical_names) && (!msh.fail()))
                    {
                    std::string name;
                    msh >> _tags >> name;

                    switch (stoi(symb))
                        {
                        case tags::msh::DIM_OBJ_2D:
                            {
                            surfRegNames[_tags] = name.substr(1, name.length() - 2);
                            break;
                            }
                        case tags::msh::DIM_OBJ_3D:
                            {
                            volRegNames[_tags] = name.substr(1, name.length() - 2);
                            break;
                            }
                        default:
                            std::cerr << "unknown type in mesh " << tags::msh::begin_physical_names << std::endl;
                            break;
                        }
                    }
                if (mySets.verbose)
                    {
                    std::cout << "  found " << nbRegNames << " regions:\n";
                    std::map<int, std::string>::iterator it;
                    for (it = surfRegNames.begin(); it != surfRegNames.end(); ++it)
                        {
                        std::cout << "    " << it->first << ": " << it->second << '\n';
                        }
                    for (it = volRegNames.begin(); it != volRegNames.end(); ++it)
                        {
                        std::cout << "    " << it->first << ": " << it->second << '\n';
                        }
                    }
                }
            else
                {
                std::cerr << tags::msh::begin_physical_names <<" undefined." << std::endl;
                msh.clear();
                msh.seekg(std::ios::beg);
                }

            double scale = mySets.getScale();
            int nbNod;

            if (tags::lookFor(true, msh, tags::msh::begin_nodes))
                {
                msh >> nbNod;
                if (nbNod > 0) node.resize(nbNod);

                for (int i = 0; i < nbNod; i++)
                    {
                    double x,y,z;
                    msh >> symb >> x >> y >> z;
                    x *= scale;
                    y *= scale;
                    z *= scale;
                    node[i].p = Eigen::Vector3d(x,y,z);
                    }
                }

            on_fail_msg_error(msh, "error while reading nodes");

            for (auto it = surfRegNames.begin(); it != surfRegNames.end(); ++it)
                {
                s.push_back(Mesh::Surf(node, it->second));
                }

            int nbElem;

            if (tags::lookFor(true, msh, tags::msh::begin_elements))
                {
                msh >> nbElem;
                if (mySets.verbose)
                    {
                    std::cout << "  element count: " << nbElem << '\n';
                    }
                while ((msh >> symb) && (symb != tags::msh::end_elements) && (!msh.fail()))
                    {
                    msh >> TYP >> _tags >> reg;
                    for (int i = 1; i < _tags; i++)
                        msh >> symb;
                    switch (TYP)
                        {
                        case tags::msh::TYP_ELEM_TRIANGLE:
                            {
                            int i0, i1, i2;
                            msh >> i0 >> i1 >> i2;
                            if (auto search = surfRegNames.find(reg); search != surfRegNames.end())
                                {  // found named surface
                                for (auto it = s.begin(); it != s.end(); ++it)
                                    {
                                    if (it->getName() == search->second)
                                        {
                                        it->push_back(Mesh::Triangle(node, i0, i1, i2));
                                        }
                                    }

                                int idx = mySets.findFacetteRegionIdx(search->second);
                                if (idx > -1)
                                    fac.push_back(
                                            Facette::Fac(node, nbNod, idx, {i0, i1,
                                                         i2}));  // we only want to store in fac
                                                                // vector the facettes for micromag
                                                                // problem, nothing for bc for stt
                                }
                            else
                                {
                                std::cout << "mesh reading error : unnamed surface region"
                                          << std::endl;
                                }  // unnamed surface

                            break;
                            }
                        case tags::msh::TYP_ELEM_TETRAEDRON:
                            {
                            int i0,i1,i2,i3;
                            msh >> i0 >> i1 >> i2 >> i3;
                            if (auto search = volRegNames.find(reg); search != volRegNames.end())
                                {  // found named volume
                                int idx = mySets.findTetraRegionIdx(search->second);
                                if (idx > -1) tet.push_back(Tetra::Tet(node, idx, {i0,i1,i2,i3}));
                                }
                            else
                                {
                                std::cout << "mesh reading error : unnamed volume region"
                                          << std::endl;
                                }
                            break;
                            }
                        default: std::cerr << "unknown object type in mesh\n"; break;
                        }
                    }
                }
            on_fail_msg_error(msh, "error while reading elements");
            }
        else
            {
            std::cout << "mesh file format " << mshFormat << " not supported." << std::endl;
            SYSTEM_ERROR;
            }
        }

    if (!msh.fail())
        {
        if (mySets.verbose) std::cout << "  closing file\n";
        msh.close();
        }
    else
        {
        std::cout << "error before closing mesh." << std::endl;
        SYSTEM_ERROR;
        }

    for (unsigned int i = 0; i < tet.size(); i++)
        {
        tet[i].idx = i;
        }
    }

double mesh::readSol(bool VERBOSE, const std::string fileName)
    {
    double t(0);
    std::ifstream fin(fileName, std::ifstream::in);

    on_fail_msg_error(fin, "cannot open file " + fileName);

    std::string str;
    bool flag_t = false;
    unsigned long idx;

    while (fin.peek() == '#' || fin.peek() == '\n')
        {
        getline(fin, str);
        idx = str.find(tags::sol::time);
        if (idx != std::string::npos)
            {  // found tag "## time:"
            t = stod(str.substr(idx + tags::sol::time.length() ));
            flag_t = true;
            }
        }

    if (!flag_t)
        {
        std::cerr << "error: no ## time: tag in input .sol file " << fileName << std::endl;
        SYSTEM_ERROR;
        }

    if (VERBOSE)
        {
        std::cout << ".sol file: " << fileName << " @ time t = " << t << std::endl;
        }

    for (unsigned int i = 0; i < node.size(); i++)
        {
        double mx,my,mz;
        unsigned int i_;
        int node_idx = node_index[i];
        fin >> i_ >> mx >> my >> mz >> node[node_idx].d[Nodes::NEXT].phi;
        node[node_idx].d[Nodes::NEXT].u = Eigen::Vector3d(mx,my,mz);
        if (i != i_)
            {
            std::cerr << "error: mesh node index mismatch between mesh and input .sol file"
                      << std::endl;
            fin.close();
            SYSTEM_ERROR;
            }
        }
    fin.close();

    return t;
    }

    }  // namespace Mesh

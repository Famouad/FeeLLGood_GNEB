#ifndef element_h
#define element_h

#include <execution>
#include "node.h"

/** \class element
template class, mother class for tetraedrons and facettes
*/

template <int N,int NPI>
class element
    {
    /** constructor */
    public:
    element(const std::vector<Nodes::Node> &_p_node /**< vector of nodes */) : refNode(_p_node)
        {
        ind.resize(N);
        }

    /** indices to the nodes */
    std::vector<int> ind;
    
    /** matrix for integrales */
    double Kp[2*N][2*N];

    /** vector for integrales */
    double Lp[2*N];

    /** index setter */
    inline void set_ind(std::initializer_list<int> & _i)
        {
        ind.assign(_i.begin(),_i.end());
        }

    /** getter for N */
    inline constexpr int getN(void) const { return N; }

    /** getter for NPI */
    inline constexpr int getNPI(void) const { return NPI; }

    /** getter for node */
    inline const Nodes::Node &getNode(const int i)
        { return refNode[ind[i]]; }
    
    /** set all indices to zero */
    inline void indicesToZero(void)
        { std::fill(ind.begin(),ind.end(),0); }

    /** print the node indices of the element */
    void print_indices(void) const
        {
        std::cout << '(';
        for(unsigned int i = 0; i < N-1; i++)
            { std::cout << ind[i] << ", "; }
        std::cout << ind[N-1] << ")\n";
        }

    /** assemble the big sparse matrix K from tetra or facette inner matrix Kp */
    void assemblage_mat(const int NOD, write_matrix &K) const
        {
        for (int i = 0; i < N; i++)
            {
            int i_ = ind[i];

            for (int j = 0; j < N; j++)
                {
                int j_ = ind[j];
                K(NOD + i_, j_) += Kp[i][j];
                K(NOD + i_, NOD + j_) += Kp[i][N + j];
                K(i_, j_) += Kp[N + i][j];
                K(i_, NOD + j_) += Kp[N + i][N + j];
                }
            }
        }

    /** assemble the big vector L from tetra or facette inner vector Lp */
    void assemblage_vect(const int NOD, std::vector<double> &L) const
        {
        for (int i = 0; i < N; i++)
            {
            const int i_ = ind[i];
            L[NOD + i_] += Lp[i];
            L[i_] += Lp[N + i];
            }
        }

    protected:
        /** vector of nodes */
        const std::vector<Nodes::Node> &refNode;

    /** zeroBasing : index convention Matlab/msh (one based) -> C++ (zero based) */
        inline void zeroBasing(void)
            {// par_unseq to benefit of parallelization and vectorization (SSE|AVX)
            std::for_each(std::execution::par_unseq, ind.begin(), ind.end(),
                          [](int &idx) { --idx; });
            }

    
    private:
        /** a method to orientate the element must be provided in derived class */
        virtual void orientate() = 0;
    };
    
#endif

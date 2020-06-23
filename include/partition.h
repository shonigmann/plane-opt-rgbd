#ifndef PARTITION_H
#define PARTITION_H

#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <eigen3/Eigen/Eigen>
//#include "../common/covariance.h"
#include "covariance.h"
#include "MxHeap.h"
#include "qemquadrics.h"

using namespace std;
using namespace Eigen;

class Partition
{
public:
    struct Edge : public MxHeapable
    {
        int v1, v2;
        Edge(int a, int b) : v1(a), v2(b) {}
    };

    struct SwapFace
    {
        int face_id, from, to;
        SwapFace(int v, int f, int t)
        {
            face_id = v;
            from = f;
            to = t;
        }
    };

    struct Vertex
    {
        bool is_valid;  // false if it is removed (all its adjacent faces are removed)
        int cluster_id;
        Vector3d pt;
        unordered_set<int> nbr_vertices, nbr_faces;
        vector<Edge*> nbr_edges;
        QEMQuadrics Q;
        Vertex() : is_valid(false), cluster_id(-1) {}
    };

    struct Face
    {
        int cluster_id;
        bool is_visited;  // used in Breadth-first search to get connected components in clusters
        bool is_valid;    // false if this face is removed.
        int indices[3];
        double area;
        CovObj cov;
        unordered_set<int> nbr_faces;
        Face() : cluster_id(-1), area(0), is_visited(false), is_valid(true) {}
    };

    struct Cluster
    {
        int original_id;
        int num_faces;
        int num_vertices;
        double energy;             // to save some computation time of calling CovObj::energy() too frequently
        double area;               // used to calculate, store, and sort by mesh cluster area
        bool is_visited;           // used in Breath-first search to remove small floating clusters
        unordered_set<int> faces;  // faces each cluster contains
        unordered_set<int> nbr_clusters;
        vector<SwapFace> faces_to_swap;
        Vector3f color;
        CovObj cov;
        Cluster() : energy(0), area(0.0) {}
    };

public:
    Partition();
    ~Partition();
    bool readPLY(const std::string& filename);
    bool writePLY(const std::string& filename);
    bool writePLY(const std::string& filename, double min_area);
    bool writeTopPLYs(const std::string& basefilename, double min_area, Vector3d gravity_direction=Vector3d(0,1,0));

    bool runPartitionPipeline();
    void writeClusterFile(const std::string& filename);
    bool readClusterFile(const std::string& filename);
    void setTargetClusterNum(int num) { target_cluster_num_ = num; }
    int getCurrentClusterNum() { return curr_cluster_num_; }
    void printModelInfo() { cout << "#Vertices: " << vertices_.size() << ", #Faces: " << faces_.size() << endl; }
    void runPostProcessing();
    void runSimplification();
    void doubleCheckClusters();
    void updateClusterInfo();
    void updateClusters();

private:
    /* Merging */
    bool runMerging();
    void initMerging();
    void initMeshConnectivity();
    void computeEdgeEnergy(Edge* edge);
    bool removeEdgeFromList(Edge* edge, vector<Edge*>& edgelist);
    bool isClusterValid(int cidx) { return !clusters_[cidx].faces.empty(); }
    bool mergeOnce();
    void applyFaceEdgeContraction(Edge* edge);
    void mergeClusters(int c1, int c2);
    int findClusterNeighbors(int cidx);
    int findClusterNeighbors(int cidx, unordered_set<int>& cluster_faces, unordered_set<int>& neighbor_clusters);
    double getTotalEnergy();
    void createClusterColors();
    void updateCurrentClusterNum();
    void releaseHeap();

    /* Swap */
    void runSwapping();
    int swapOnce();
    double computeSwapDeltaEnergy(int fidx, int from, int to);
    void processIslandClusters();
    int splitCluster(int cidx, vector<unordered_set<int>>& connected_components);
    int traverseFaceBFS(int start_fidx, int start_cidx, unordered_set<int>& component);
    void mergeIslandComponentsInCluster(int original_cidx, vector<unordered_set<int>>& connected_components);

    /* Post processing */
    double computeMaxDisBetweenTwoPlanes(int c1, int c2, bool flag_use_projection = false);
    double computeAvgDisBtwTwoPlanes(int c1, int c2);
    void removeSmallClusters();
    void updateNewMeshIndices();
    void mergeAdjacentPlanes();
    void mergeIslandClusters();

    /* Simplification */
    void initSimplification();
    void findInnerAndBorderEdges();
    void initInnerEdgeQuadrics();
    void initBorderEdges();
    void simplifyInnerEdges();
    void simplifyBorderEdges();
    bool checkEdgeContraction(Edge* edge);
    int getCommonNeighborNum(int v1, int v2);
    bool checkFlippedFaces(Edge* edge, int endpoint, const Vector3d& contracted_vtx);
    void applyVtxEdgeContraction(Edge* edge, int cluster_idx);

    /* Geometric Functions */
    double computeFaceArea(int f);
    static bool compareByArea(const Cluster& a, const Cluster& b){
      return a.area > b.area;
    }
    static bool compareByNumFaces(const Cluster& a, const Cluster& b){
      return a.num_faces > b.num_faces;
    }
    bool faceInTopNClusters(int face_num, int n_clusters);    
    void computeAllFaceAreas();
    void orderClustersByArea();
    void orderClustersByFaceCount();
    void sortClusters(bool byArea); //TODO: remove unused functions
    Vector3d computeMeshCentroid(double min_cluster_area);
    Vector3d computeClusterCentroid(int c){
      return clusters_[c].cov.center_;
    }
    void changeClusterNormalDirection(int cidx, Vector3f grav_dir);

    /* Small functions */
    template <typename T> int sgn(T val) {
        return (T(0) < val) - (val < T(0));
    }

    //! Check if a face contains two vertices
    inline bool checkFaceContainsVertices(int fidx, int v1, int v2)
    {
        return checkFaceContainsVertices(fidx, v1) && checkFaceContainsVertices(fidx, v2);
    }
    //! Check if a face contains one vertex
    inline bool checkFaceContainsVertices(int fidx, int v1)
    {
        return faces_[fidx].indices[0] == v1 || faces_[fidx].indices[1] == v1 || faces_[fidx].indices[2] == v1;
    }
    //! Convert an long long edge type to two endpoints
    inline void getEdge(const long long& key, int& v1, int& v2)
    {
        v2 = int(key & 0xffffffffLL);  // v2 is lower 32 bits of the 64-bit edge integer
        v1 = int(key >> 32);           // v1 is higher 32 bits
    }

private:

    // store the final maps for each cluster to make PLYs from each cluster as needed
    map<int, vector<int>> cluster_face_num;
    map<int, unordered_set<int>> cluster_vert_num;
    map<int, unordered_map<int, int>> cluster_vert_old2new;
    map<int, unordered_map<int, int>> cluster_vert_new2old;
    Vector3d mesh_centroid_;


    int vertex_num_, face_num_;
    int init_cluster_num_, curr_cluster_num_, target_cluster_num_;
    bool flag_read_cluster_file_;
    vector<Vertex> vertices_;
    vector<Face> faces_;
    vector<Cluster> clusters_;
    vector<Cluster> ordered_clusters_;
    vector<vector<Edge*>> global_edges_;
    MxHeap heap_;
    double total_energy_;
    unordered_set<int> clusters_in_swap_, last_clusters_in_swap_;
    unordered_map<long long, vector<int>> edge_to_face_;         // edge (represented by two int32 endpoints) -> face id
    unordered_map<int, vector<long long>> cluster_inner_edges_;  // edges inside each cluster
    unordered_set<long long> border_edges_;                      // mesh border and cluster border edges
    unordered_map<int, int> vidx_old2new_;  // original vertex indices -> new mesh indices (after removing some faces)
    unordered_map<int, int> fidx_old2new_;  // original vertex indices -> new mesh indices (after removing some faces)
    int new_vertex_num_, new_face_num_;
    bool flag_new_mesh_;  // true if removing some faces/vertices/clusters; false by default

    // These are used to balance the importance of point and triangle quadrics, respectively.
    // However, equal values work well in experiments.
    const double kFaceCoefficient = 1.0, kPointCoefficient = 1.0;
    int curr_edge_num_;
};

#endif  // !PARTITION_H

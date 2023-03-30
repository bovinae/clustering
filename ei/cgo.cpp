// #include <vector>
// #include <algorithm>
// #include <iterator>
// #include <cmath>
// #include <Eigen/Core>
// #include "SpectralClustering.h"

// int** SpectralClusteringCWrapper(int** similarities, int numDims);

// int main() {
//     int items[] = {1,2,3,4,5,6,7,8,9,10};

//     // generate similarity matrix
//     unsigned int size = sizeof(items) / sizeof(int);

//     // the number of eigenvectors to consider. This should be near (but greater) than the number of clusters you expect. Fewer dimensions will speed up the clustering
//     int numDims = size;

//     int** similarities = (int**)malloc(numDims * sizeof(int*));
//     for (size_t i = 0; i < numDims; i++) {
//         similarities[i] = (int *)malloc(numDims * sizeof(int));
//     }
//     for (unsigned int i=0; i < numDims; i++) {
//         for (unsigned int j=0; j < numDims; j++) {
//             // generate similarity
//             int d = items[i] - items[j];
//             int similarity = exp(-d*d / 100);
//             similarities[i][j] = similarity;
//             similarities[j][i] = similarity;
//         }
//     }

//     int** clusters = SpectralClusteringCWrapper(similarities, numDims);

//     // output clustered items
//     // items are ordered according to distance from cluster centre
//     // for (unsigned int i=0; i < clusters.size(); i++) {
//     //     std::cout << "Cluster " << i << ": " << "Item ";
//     //     std::copy(clusters[i].begin(), clusters[i].end(), std::ostream_iterator<int>(std::cout, ", "));
//     //     std::cout << std::endl;
//     // }
//     std::cout << clusters[0][0] << std::endl;
// }

// int** SpectralClusteringCWrapper(int** similarities, int numDims) {
//     Eigen::MatrixXd m = Eigen::MatrixXd::Zero(numDims,numDims);
//     for (unsigned int i=0; i < numDims; i++) {
//         for (unsigned int j=0; j < numDims; j++) {
//             m(i,j) = similarities[i][j];
//             m(j,i) = similarities[j][i];
//         }
//     }
//     for (size_t i = 0; i < numDims; i++) {
//         free(similarities[i]);
//     }
//     free(similarities);

//     // do eigenvalue decomposition
//     SpectralClustering* c = new SpectralClustering(m, numDims);

//     // whether to use auto-tuning spectral clustering or kmeans spectral clustering
//     bool autotune = true;

//     std::vector<std::vector<int> > clusters;
//     if (autotune) {
//         // auto-tuning clustering
//         clusters = c->clusterRotate();
//     } else {
//         // how many clusters you want
//         int numClusters = 5;
//         clusters = c->clusterKmeans(numClusters);
//     }

//     int** result = (int**)malloc(clusters.size() * sizeof(int*));
//     for (size_t i = 0; i < clusters.size(); i++) {
//         result[i] = (int *)malloc(clusters[i].size() * sizeof(int));
//     }
//     for (size_t i = 0; i < clusters.size(); i++) {
//         for (size_t j = 0; j < clusters[i].size(); j++) {
//             result[i][j] = clusters[i][j];
//         }
//     }

//     return result;
// }

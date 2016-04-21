/* ---------------------------------------------------------------------
 * Numenta Platform for Intelligent Computing (NuPIC)
 * Copyright (C) 2014-2015, Numenta, Inc.  Unless you have an agreement
 * with Numenta, Inc., for a separate license for this software code, the
 * following terms and conditions apply:
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Affero Public License for more details.
 *
 * You should have received a copy of the GNU Affero Public License
 * along with this program.  If not, see http://www.gnu.org/licenses.
 *
 * http://numenta.org/licenses/
 * ----------------------------------------------------------------------
 */

/* ---------------------------------------------------------------------
 * This file runs the MNIST dataset using a simple model composed of a
 * set of dendrites. Each dendrite randomly samples pixels from one image.
 *
 * Key parameters:
 *     Binarization threshold
 *     Number of synapses per dendrite
 *     Threshold
 *     min threshold
 * ----------------------------------------------------------------------
 */

#include <assert.h>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <utility>

#include <nupic/math/Math.hpp>
#include <nupic/math/SparseMatrix.hpp>
#include <nupic/math/SparseMatrix01.hpp>
#include <nupic/types/Types.hpp>
#include <nupic/utils/Random.hpp>

#include "dendrite_classifier1.hpp"
#include "dendrite_classifier2.hpp"
#include "knn_classifier.hpp"

using namespace std;
using namespace nupic;

///////////////////////////////////////////////////////////
//
// External definitions, should really be in header files

// Read test/training images from top-level directory and return number read
extern int readImages(int *numImages, const char *path,
                std::vector< SparseMatrix01<UInt, Int> * > &images,
                Real samplingFactor = 1.0);

extern void classifyDataset(
           int threshold,
           std::vector< SparseMatrix01<UInt, Int> * > &dataSet,
           std::vector< SparseMatrix01<UInt, Int> * > &dendrites);

extern void trainDendrites(int k, int nSynapses,
           std::vector< SparseMatrix01<UInt, Int> * > &trainingSet,
           std::vector< SparseMatrix01<UInt, Int> * > &dendrites,
           Random &r);

extern void trainDendrites2(int k, int nSynapses,
           std::vector< SparseMatrix01<UInt, Int> * > &trainingSet,
           std::vector< SparseMatrix01<UInt, Int> * > &dendrites,
           Random &r);

extern void createNoisyDataset(
        std::vector< SparseMatrix01<UInt, Int> * > &dataset,
        std::vector< SparseMatrix01<UInt, Int> * > &noisyDataset,
        float noisePct, int noiseType, Random &r );


// Run the whole MNIST example using brute force KNN
void runMNISTKNN(std::vector< SparseMatrix01<UInt, Int> * > &trainingSet,
              std::vector< SparseMatrix01<UInt, Int> * > &testSet,
              bool runTrainingSet = false)
{
  KNNClassifier knnModel;

  //////////////////////////////////////////////////////
  //
  // Create trained model for each category, by randomly sampling from
  // training images.
  cout << "Training KNN model.\n";
  knnModel.trainDataset(trainingSet);

  if (runTrainingSet)
  {
    cout << "Testing KNN model on training set\n";
    knnModel.classifyDataset(1, trainingSet);
  }

  cout << "\nTesting KNN model on test set\n";
  knnModel.classifyDataset(1, testSet);


  //////////////////////////////////////////////////////
  //
  // Run noise tests

  // Random number generator
  Random rng(42);

  for (Real noise=0.05; noise <= 0.4; noise += 0.05)
  {
    std::vector< SparseMatrix01<UInt, Int> * > noiseTestSet;
    createNoisyDataset(testSet, noiseTestSet, noise, 2, rng);
    cout << "Test set using KNN and " << noise << " noise:\n";
    knnModel.classifyDataset(1, noiseTestSet);
  }


}


// Run the whole MNIST example.
void runMNIST(std::vector< SparseMatrix01<UInt, Int> * > &trainingSet,
              std::vector< SparseMatrix01<UInt, Int> * > &testSet,
              int nSynapses, int trainingThreshold,
              bool useDefaultWeights,
              bool runTrainingSet = false)
{
  DendriteClassifier1 model1(6000);
//  DendriteClassifier model2(42, 10, 784, 500);

  //////////////////////////////////////////////////////
  //
  // Create trained model for each category, by randomly sampling from
  // training images.
  cout << "Training dendrite model1 with " << nSynapses
       << " synapses per dendrite and " << model1.nPrototypesPerClass_
       << " dendrite models per class.\n";
  model1.trainDataset(nSynapses, trainingSet);

//  cout << "Training dendrite model2 with " << nSynapses
//       << " synapses per dendrite and training threshold "
//       << trainingThreshold << ".\n";
//  model2.trainDataset(nSynapses, trainingThreshold, trainingSet,
//                      useDefaultWeights);

  //////////////////////////////////////////////////////
  //
  // Classify the data sets and compute accuracy
  cout << "Running classification with a bunch of different thresholds.\n";
  for (int threshold = trainingThreshold;
       threshold <= trainingThreshold; threshold+= 2)
  {
    cout << "\nUsing threshold = " << threshold << "\n";
  if (runTrainingSet)
  {
      cout << "Training set using model1:\n";
      model1.classifyDataset(threshold, trainingSet);

//      cout << "Training set using model2:\n";
//      model2.classifyDataset(threshold, trainingSet);
  }

    cout << "Test set using model1:\n";
    model1.classifyDataset(threshold, testSet);
//    cout << "Test set using model2:\n";
//    model2.classifyDataset(threshold, testSet);
  }

  //////////////////////////////////////////////////////
  //
  // Run noise tests

  // Random number generator
  Random rng(42);

  for (Real noise=0.05; noise <= 0.020; noise += 0.05)
  {
    std::vector< SparseMatrix01<UInt, Int> * > noiseTestSet;
    createNoisyDataset(testSet, noiseTestSet, noise, 2, rng);
    cout << "Test set using model1 and " << noise << " noise:\n";
    model1.classifyDataset(trainingThreshold, noiseTestSet);
//    cout << "Test set using model2 and " << noise << " noise:\n";
//    model2.classifyDataset(trainingThreshold, noiseTestSet);
  }

}


// Run the trials!  Currently need to hard code the specific trial you are
// about to run.
int main(int argc, char * argv[])
{
  // Experiment parameters
  Real samplingFactor = 1.0;

  //////////////////////////////////////////////////////
  //
  // Read in the given number of training and test images
  int trainingImages[] = {
    5923, 6742, 5958, 6131, 5842, 5421, 5918, 6265, 5851, 5949
  };
  int testImages[] = { 980, 1135, 1032, 1010, 982, 892, 958, 1028, 974, 1009 };

  std::vector< SparseMatrix01<UInt, Int> * > trainingSet;
  std::vector< SparseMatrix01<UInt, Int> * > testSet;

  for (int i= 0; i<10; i++)
  {
    trainingSet.push_back( new SparseMatrix01<UInt, Int>(28*28, 1));
    testSet.push_back( new SparseMatrix01<UInt, Int>(28*28, 1));
  }

  int numImages = readImages(trainingImages,
             "../image_test/mnist_extraction_source/training/%d", trainingSet,
             samplingFactor);
  cout << "Read in " << numImages << " total images\n";

  int numTestImages = readImages(testImages,
             "../image_test/mnist_extraction_source/testing/%d", testSet);
  cout << "Read in " << numTestImages << " total test images\n";


//  runMNISTKNN(trainingSet, testSet, false);
  runMNIST(trainingSet, testSet, 70, 50, false, false);

}

Music-Genre-Classifier
======================

Classifies musical data pulled from MIDI files into genres using only harmonic structure.

File Structure
-------
* classifier: contains code in java for classification, as well as training and test files
* melisma: contains third party software used to generate the data
* results: contains the results of the execution of Melisma


Part Ia: Data Acquisition
-------
The melisma folder is divided into four subfolders and a script file:
 * harmony
 * key
 * meter
 * mftext
 * makeData.sh
Each one is a subprogram of the Melisma program used as a third party software to get the data for this project. 

MIDI files have been stored in melisma/mftext/midi

To execute this software, I wrote a script “makeData.sh” that creates the harmonic analysis of each midi file and stores it in text files in the same folder containing the midi files. 

The script can be run by typing: “./makeData.sh” in the melisma folder. 

Part Ib: Formatting the Data
-------
The results folder is divided into:
 * penalty1: contains results from the Melisma program using penalty=1
 * penalty6: contains results from the Melisma program using penalty=6
 * penalty12: contains results from the Melisma program using penalty=12
 * appendFilesRock.sh: 
 * appendFilesClassical.sh

The two script files group the multiple text files with the results of the analysis into two text files, one for rock music the other for classical music, containing all the samples for each genre. 
The two resulting files are: “rock.txt” and “classical.txt”. These contain the samples to be used for classification. 

The scripts are set to run on the files in the folder penalty1 as default. It can be changed just by rewriting the number in the code. 

Part II: Classification
-------
The classification folder contains:
 * penalty1: directory with test file, training file and another file with all the samples. These samples were generated using a penalty=1 in the Melisma program
 * penalty6: directory with test file, training file and another file with all the samples. These samples were generated using a penalty=6 in the Melisma program
 * penalty12: directory with test file, training file and another file with all the samples. These samples were generated using a penalty=12 in the Melisma program
 * Classifier.java: the main program for the classifier. Takes two arguments. The first is the training file and the second is the test file. It generates a document with the results and also prints them out to the console. 
 * NearestNeighbor.java: class that executes the nearest neighbor classification
 * Data.java: class that reads from data and formats it appropriately

To run the classification program:
1) Compile the code: 
    javac *.java

2) To run, for example, on data acquired by using a penalty=1
    java Classifier ./penalty1/trainfile.txt ./penalty1/testfile.txt

3) To change the K of the nearest neighbor classifier, change variable NEAREST_NEIGHBOR in NearestNeighbor.java. 


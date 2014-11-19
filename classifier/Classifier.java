/*
 * Classifier.java 		Author: Irene Alvarado 
 * Program that takes a trainfile and
 * testfile and classifies testfile according to the Nearest Neighbor
 * classifier
 */
import java.io.*;

public class Classifier
{
	public static void main(String[] args) throws IOException
	{
		/*
		 * Preparing the data
		 */
		
		File inputFile1 = new File(args[0]); // Training file
		
		Data data = new Data(inputFile1); //Create a Data object to read from the file
		data.findDimension();
		String[][] keys = data.returnData(); // Return a string array with the data
		
		
		File inputFile2 = new File(args[1]); //Testing file
		
		Data testData = new Data(inputFile2); //Create a Data object to read from the file
		testData.findDimension();
		String[][] test = testData.returnData(); // Return a string array with the data
		
		
		/*
		 * K-Nearest Neighbor Classifier
		 */
		
		NearestNeighbor classifier = new NearestNeighbor(keys, test);
		
		classifier.measureVariation(); //Measure the harmonic variation
		classifier.theNearestNeighbor();	//Find the nearest neighbor for the test file
		classifier.measureAccuracy(); //Measure the accuracy of the classification
		
		String[] results = classifier.returnResults();

		File resultsFile = new File("results.txt");
      FileOutputStream fos = new FileOutputStream(resultsFile);
      DataOutputStream dos=new DataOutputStream(fos);
	
      for (int k = 0; k < results.length ; k++) 
		{ 
      	dos.writeChars(results[k]) ;
      	dos.writeChars("\t") ;
      	
			for (int t = 1 ; t < test[0].length ; t++) 
			{ 
				if(!test[k][t].equals(" "))
				{
					dos.writeChars(test[k][t]) ; 
					dos.writeChars(" ") ; 
				}
			}
			dos.writeChars("\n") ; 
		}	
	}
}

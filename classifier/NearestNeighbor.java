/*
 * Nearest Neighbor.java 		Author: Irene Alvarado 
 * Performs classification according to nearest neighbor by using
 * two arrays containing the data, one for the training data and
 * another for the test data. 
 */
public class NearestNeighbor
{
	String[][] keys; // contains the training data
	String[][] test; // contains the test data
	
	int numTrain; //total number of samples to training
	int numTest; //total number of samples to test
	int columns;
	
	double[] variationTrain; //Stores the measure of variation for the training data
	double[] variationTest; //Stores the measure of variation for the test data
	
	String[] results; //Stores the results of the classification

	// Represents all the information needed to perform a nearest neighbor
	// classification
	public NearestNeighbor(String[][] keyArray, String[][] testArray)
	{
		keys = keyArray;
		test = testArray;
		
		columns = keys[0].length; //total number of columns the the arrays.
		
		numTrain = keys.length;
		variationTrain = new double[numTrain];
		
		numTest = test.length;
		variationTest = new double[numTest];
		
		results = new String[numTest];
	}

	// Measure the variation in each test and training sample
	// Variation is measured by the rate of change of the keys of each song
	public void measureVariation()
	{
		for (int j = 0; j < numTrain; j++) // Measure variation in training data
		{
			int count = 1;
			String current = keys[j][1]; //Ignore index 0, which is the tag
			int i = 2;
			while ((i != columns) && (!keys[j][i].equals(" "))) //while there are still keys
			{
				if (!current.equals(keys[j][i])) //if adjacent keys are not equal
				{
					current = keys[j][i];
					count++; //increment the count
				}
				i++;
			}
			double variation = ((double) count) / (i - 1) ;
			
			variationTrain[j] = variation; //Store the harmonic variation of the song. 
		}		
		
		
		for (int j = 0; j < numTest; j++) // Measure variation in test data
		{
			int count = 1;
			String current = test[j][1]; //Ignore index 0, which is the tag
			int i = 2;
			while ((i != columns) && (!test[j][i].equals(" "))) //while there are still keys
			{
				if (!current.equals(test[j][i])) //if adjacent keys are not equal
				{
					current = test[j][i];
					count++;
				}
				i++;
			}
			double variation = ((double) count) / (i - 1) ;
			
			variationTest[j] = variation;
		}
	}

	
	
	
	int NEAREST_NEIGHBOR = 21; //Number of samples to take into account for nearest neighbor

	// Classifies the testing data according to nearest neighbor classifier
	public void theNearestNeighbor()
	{	
		for (int j = 0; j < numTest; j++) // For each test data
		{
			double[] tempTrain = new double[numTrain] ;
			for (int t = 0 ; t < numTrain ; t++)
			{
				tempTrain[t] = variationTrain[t] ;
			}
			
			
			int classicalCount = 0; //Start with 0 matches for classical
			int rockCount = 0; //Start with 0 matches for rock
			
			String tag = " ";
			
			for (int k = 0; k < NEAREST_NEIGHBOR; k++) //For the number of samples that want 
				//to be taken for nearest neighbor
			{
				double min = 10000;
				int indexMin = 0;
				
				for (int i = 0; i < numTrain; i++) //For each testing sample
				{
					if (Math.abs(tempTrain[i] - variationTest[j]) < min) //if the sample is the 
						//smallest so far
					{
						min = Math.abs(tempTrain[i] - variationTest[j]);
						indexMin = i;
					}
				}
				tempTrain[indexMin] = 10000; //set the smallest so far to a big number
				
				tag = keys[indexMin][0];
				if (tag.equals("rock"))
				{
					rockCount++;
				}
				if (tag.equals("classical"))
				{
					classicalCount++;
				}
			}
			
			if (rockCount > classicalCount) //If more samples suggest rock music
			{
				tag = "rock";
			}
			else // More samples suggest classical music
			{
				tag = "classical";
			}
			
			results[j] = tag; //Store the result of the classification
		}
	}
	
	// Calculates the accuracy of the nearest neighbor classification
	public void measureAccuracy()
	{
		double match = 0;
		for (int i = 0; i < numTest; i++) //For each test sample
		{
			System.out.println("Test: " + test[i][0] + "\t\t Result of classification: " + results[i]) ;
			if (test[i][0].equals(results[i])) //See if it is a correct match
			{
				match++;
			}
		}
		double accuracy = (match / numTest) * 100;
		System.out.println("Accuracy is: " + accuracy);
	}
	
	//return the results of the classification
	public String[] returnResults()
	{
		return results ;
	}
}

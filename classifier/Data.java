/*
 * Data.java 		Author: Irene Alvarado 
 * A class that reads data from an input file and creates an 
 * array of strings 
 */


import java.io.File;
import java.io.FileNotFoundException;
import java.util.* ;

public class Data
{
	File file ; //input file
	String[][] keys ; //array that will store the keys of each training or test sample
	int numlines ;
	int max ;
	
	public Data(File inputFile)
	{
		file = inputFile ;
	}
	
	int COLUMNS = 800 ;
	
	// Counts the number of samples in the file
	public void findDimension() throws FileNotFoundException
	{
		Scanner input = new Scanner(file) ;
		
		numlines = 0 ;

		while (input.hasNextLine()) 
		{

			if (input.hasNextLine()) ;
			{			
				numlines++ ;
	
				input.nextLine() ;
	
			}
		}		
		keys = new String[numlines][COLUMNS] ; //creates a string array to hold the sample keys
	}
	
	//Returns the string array with the sample keys (one in each cell)
	public String[][] returnData() throws FileNotFoundException
	{
		Scanner input = new Scanner(file) ;
		int i = 0 ;
		while (input.hasNextLine()) 
		{	
			if (input.hasNextLine()) ;
			{			
				String line = input.nextLine() ; 
		
				int j = 0 ;
		
				StringTokenizer st = new StringTokenizer(line); //Tokenizer will break up
					// the keys in each line 
				while (st.hasMoreTokens() && (j < COLUMNS)) 
				{
					keys[i][j] = st.nextToken(); //Store each key in a different cell
					j++;
				}	    
				while (j < COLUMNS)
				{
					keys[i][j] = " " ;
					j++;
				}
			}
				i++;
			}
		
		return keys ; //return the array
	}
	
}

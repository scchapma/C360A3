#include<stdio.h>
#include<stdlib.h>

#define BYTES_PER_SECTOR 512

void getOSName(FILE *fp, char *osname)
{
	fseek(fp,3L,SEEK_SET);
	fread(osname,1,8,fp);

}

int getSize(FILE *fp)
{
	int *tmp1 = malloc(sizeof(int));
	int *tmp2 = malloc(sizeof(int));
	int retVal;
	fseek(fp,19L,SEEK_SET);
	fread(tmp1,1,1,fp);
	fread(tmp2,1,1,fp);
	retVal = *tmp1+((*tmp2)<<8);
	free(tmp1);
	free(tmp2);
	return retVal;
}

void getLabel(FILE *fp, char *label)
{
	fseek(fp,43L,SEEK_SET);
	fread(label,11,8,fp);
}

int getFreeSpace(FILE* fp, int size)
{
	int n = 2;  // logical number of the first sector in Data Area
	int base = 512; // the first byte of the FAT table 

	int tmp1 = 0;
	int tmp2 = 0;

	int counter = 0;
	int result = 0;

	int free_space = 0;

	int numSectors = size;
	
	// The logical number for all the sectors in Data Area is from 2 to 2848
	// numSectors = 2880 (or could be derived from getTotalSize() in mmap.c)
       for (n = 2; n <= (numSectors-1-33+2); n++) 
	{
		// given logical no. of sector in data area
		// where is the corresponding entry in FAT table ?
		// For the algorithm, refer to the last page of FAT Description: 
		// http://webhome.csc.uvic.ca/~wkui/Courses/CSC360/FAT12Description.pdf
		
		// if the logical sector number is even
		if (n % 2 == 0)
		{
			fseek(fp, base + 3*n/2, SEEK_SET);
			fread(&tmp1, 1, 1, fp);  // get all 8 bits 
			fread(&tmp2,1 ,1, fp);
			tmp2 = tmp2 & 0x0F;   // use mask to get the low 4 bits 

			// Then apply "Little Endian": (4 bits)**** + (8 bits)********
			result = (tmp2 << 8) + tmp1;  
		}

		// if the logical sector number is odd
		else
		{
			fseek(fp, base + 3*n/2, SEEK_SET);
			fread(&tmp1, 1, 1, fp);  // get all 8 bits 
			fread(&tmp2,1 ,1, fp);
			tmp1 = tmp1 & 0xF0;   // use mask to get the low 4 bits 

			// Then apply "Little Endian": (4 bits)**** + (8 bits)********
			result = (tmp2 << 4) + (tmp1 >> 4); 
		}
		
		if (result == 0x00)  // if the value is 0x00, that sector is free/unused
		{
			counter ++;
		}

	}
	//printf("Counter: %d\n", counter);
	free_space = counter * BYTES_PER_SECTOR;
	printf("Free size of the disk: %d bytes.\n", free_space);

	return free_space;
}

// loop through the root directory
// Each entry has 32 bytes in root directory
//int countRootDirFiles(FILE* fp)
void getNumberFiles(FILE *fp, int* number_files)
{
	int base = 9728;  // the first byte of the root directory

	int cur = base;   // point to the first byte of the current entry
	int offset = 32;  // Each entry has 32 bytes in root directory

	int attribute_offset = 11;

	*number_files = 0;

	fseek(fp, base, SEEK_SET);
	//char tmp;
	//char tmp2;
	int tmp;
	char tmp2;
	fread(&tmp,1,1,fp);

	/* Read "notes on directory entries" for the correct conditions of how we identify a file */
	/* Why 0x00 here? 0x00 means this entry and all remaining entries are free */
	while(tmp != 0x00)  
	{
		// Search for files
		// 0xE5 indicates that the directory entry is free (i.e., currently unused) 
		if (tmp != 0xE5)
		{
			/* Locate the byte for the current entry's attribute */
			fseek(fp, cur + attribute_offset, SEEK_SET);
			fread(&tmp2,1,1,fp);
			//printf("Attribute: %d\n", tmp2);
			
			/* What is the attribute of the entry ? */
			/* if not 0x0F(not part of a long file name), not suddirectory, not volume label, then it is a file. */
			/* mask for subdirectory is 0x10, mask for label is 0x08 */
			if((tmp2 != 0x0F) && !(tmp2 & 0x10) && !(tmp2 & 0x08))
			{
				//*number_files = *number_files + 1;
				(*number_files)++;
			}	

		}
		
		// Go to next entry in Root Directory
		cur = cur + offset;
		fseek(fp, cur, SEEK_SET);
		fread(&tmp,1,1,fp);
	}
	//*number_files = counter;
	//printf("Number of files: %d\n", *number_files);
}

void getNumberFATCopies(FILE *fp, int* number_FAT_copies)
{
	fseek(fp,16L,SEEK_SET);
	fread(number_FAT_copies,1,1,fp);
}

void getSectorsPerFAT(FILE *fp, int* sectors_per_FAT)
{
	int *tmp1 = malloc(sizeof(int));
	int *tmp2 = malloc(sizeof(int));
	
	fseek(fp,22L,SEEK_SET);
	fread(tmp1,1,1,fp);
	fread(tmp2,1,1,fp);
	*sectors_per_FAT = *tmp1+((*tmp2)<<8);
	free(tmp1);
	free(tmp2);	
}

int main()
{
	FILE *fp;
	char *osname = malloc(sizeof(char)*8);
	char *label = malloc(sizeof(char)*11*8);
	int *number_files = malloc(sizeof(int));
	int *number_FAT_copies = malloc(sizeof(int));
	int *sectors_per_FAT = malloc(sizeof(int));

	int size;
	
	if ((fp=fopen("disk2.IMA","r")))
	{
		//printf("Successfully open the image file.\n");
		
		getOSName(fp,osname);
		printf("OS Name: %s\n", osname);

		getLabel(fp,label);
		printf("Label of the disk: %s\n", label);

		size = getSize(fp);	
		//printf("Total Sectors: %d\n", size);
		printf("Total Size of the disk: %d bytes.\n", size*512);
		getFreeSpace(fp, size);

		printf("\n========================\n");	

		getNumberFiles(fp, number_files);
		printf("The number of files in the root directory (not including subdirectories): %d\n", *number_files);
		
		printf("\n========================\n");	

		getNumberFATCopies(fp, number_FAT_copies);
		printf("Number of FAT copies: %d\n", *number_FAT_copies);

		getSectorsPerFAT(fp, sectors_per_FAT);
		printf("Sectors per FAT: %d\n", *sectors_per_FAT);			

	}
	else
	{
		printf("Fail to open the image file.\n");
	}

	free(osname);
	free(label);	
	fclose(fp);
	
	return 0;
}

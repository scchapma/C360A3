#include<stdio.h>
#include<stdlib.h>

#define BYTES_PER_SECTOR 512

void *emalloc(size_t n){
    void *p;
    p = malloc(n);
    if (p == NULL) {
        fprintf(stderr, "malloc of %lu bytes failed", n);
        exit(1);
    }
    return p;
}

void getOSName(FILE *fp, unsigned char *osname)
{
	fseek(fp,3L,SEEK_SET);
	fread(osname,1,8,fp);
}

void getSize(FILE *fp, unsigned int *fileSize)
{
	unsigned char *tmp1 = (unsigned char *) emalloc(sizeof(unsigned char));
    unsigned char *tmp2 = (unsigned char *) emalloc(sizeof(unsigned char));
	fseek(fp,19L,SEEK_SET);
	fread(tmp1,1,1,fp);
	fread(tmp2,1,1,fp);
	*fileSize = *tmp1+((*tmp2)<<8);
	
	free(tmp1);
	free(tmp2);
}

void getLabel(FILE *fp, unsigned char *label)
{
	fseek(fp,43L,SEEK_SET);
	fread(label,11,8,fp);
}

unsigned int getFreeSpace(FILE* fp, unsigned int *fileSize)
{
	unsigned int n = 2;  // logical number of the first sector in Data Area
	unsigned int base = 512; // the first byte of the FAT table 

	unsigned char tmp1 = 0;
	unsigned char tmp2 = 0;

	unsigned int counter = 0;
	unsigned int result = 0;
	unsigned int free_space = 0;

	unsigned int numSectors = *fileSize;

	//TODO:  calculate numSector-1-33+2 outside for loop
	unsigned int dataSectors = (numSectors-1-33+2);
	
	// The logical number for all the sectors in Data Area is from 2 to 2848
	// numSectors = 2880 (or could be derived from getTotalSize() in mmap.c)
    //for (n = 2; n <= (numSectors-1-33+2); n++)
    for (n = 2; n <= dataSectors; n++) 
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
	free_space = counter * BYTES_PER_SECTOR;
	return free_space;
}

// loop through the root directory
// Each entry has 32 bytes in root directory
//int countRootDirFiles(FILE* fp)
void getNumberFiles(FILE *fp, unsigned int* number_files, unsigned char* fileName)
{
	unsigned int base = 9728;  // the first byte of the root directory
	unsigned int cur = base;   // point to the first byte of the current entry
	unsigned int offset = 32;  // Each entry has 32 bytes in root directory
	unsigned int attribute_offset = 11;

	*number_files = 0;

	//int *tmp1 = malloc(sizeof(int));
	unsigned char *tmp1 = (unsigned char*) emalloc(sizeof(unsigned char));
	unsigned char *tmp2 = (unsigned char*) emalloc(sizeof(unsigned char));

	fseek(fp, base, SEEK_SET);	
	fread(tmp1,1,1,fp);

	/* Read "notes on directory entries" for the correct conditions of how we identify a file */
	/* Why 0x00 here? 0x00 means this entry and all remaining entries are free */
	while(*tmp1 != 0x00)  
	{
		// Search for files
		// 0xE5 indicates that the directory entry is free (i.e., currently unused) 
		if (*tmp1 != 0xE5)
		{
			/* Locate the byte for the current entry's attribute */
			fseek(fp, cur + attribute_offset, SEEK_SET);
			fread(tmp2,1,1,fp);
			
			/* What is the attribute of the entry ? */
			/* if not 0x0F(not part of a long file name), not suddirectory, not volume label, then it is a file. */
			/* mask for subdirectory is 0x10, mask for label is 0x08 */			
			if((*tmp2 != 0x0F) && !(*tmp2 & 0x10) && !(*tmp2 & 0x08))
			{
				(*number_files)++;
			}	
			/* If item is label, set fileName to label */
			if((*tmp2 != 0x0F) && (*tmp2 & 0x08))
			{
				fseek(fp, cur, SEEK_SET);	
				fread(fileName, 1, 8, fp);
			}
		}
		
		// Go to next entry in Root Directory
		cur = cur + offset;
		fseek(fp, cur, SEEK_SET);
		fread(tmp1,1,1,fp);
	}

	free(tmp1);
	free(tmp2);
}

void getNumberFATCopies(FILE *fp, unsigned int* number_FAT_copies)
{
	fseek(fp,16L,SEEK_SET);
	fread(number_FAT_copies,1,1,fp);
}

void getSectorsPerFAT(FILE *fp, unsigned int* sectors_per_FAT)
{
	unsigned char *tmp1 = (unsigned char*) emalloc(sizeof(unsigned char));
	unsigned char *tmp2 = (unsigned char*) emalloc(sizeof(unsigned char));
	
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
	unsigned char *osname = (unsigned char*) emalloc(sizeof(unsigned char)*8);
	unsigned char *label = (unsigned char*) emalloc(sizeof(unsigned char)*11*8);
	unsigned int *fileSize = (unsigned int*) emalloc(sizeof( unsigned int));
	unsigned char *fileName = (unsigned char*) emalloc(sizeof(unsigned char)*8*8);
	unsigned int *number_files = (unsigned int*) emalloc(sizeof(unsigned int));
	unsigned int *number_FAT_copies = (unsigned int*) emalloc(sizeof(unsigned int));
	unsigned int *sectors_per_FAT = (unsigned int*) emalloc(sizeof(unsigned int));
	unsigned int free_space;
	
	if ((fp=fopen("disk2.IMA","r")))
	{
		//printf("Successfully open the image file.\n");
		
		getOSName(fp,osname);
		getLabel(fp,label);
		getSize(fp, fileSize);		
		free_space = getFreeSpace(fp, fileSize);
		getNumberFiles(fp, number_files, fileName);
		getNumberFATCopies(fp, number_FAT_copies);
		getSectorsPerFAT(fp, sectors_per_FAT);

		printf("OS Name: %s\n", osname);		
		printf("Label of the disk: %s\n", fileName);
		printf("Total Size of the disk: %d bytes.\n", (*fileSize)*512);
		printf("Free size of the disk: %d bytes.\n", free_space);
		printf("\n========================\n");			
		printf("The number of files in the root directory (not including subdirectories): %d\n", *number_files);	
		printf("\n========================\n");			
		printf("Number of FAT copies: %d\n", *number_FAT_copies);		
		printf("Sectors per FAT: %d\n", *sectors_per_FAT);			

	}
	else
	{
		printf("Fail to open the image file.\n");
	}

	free(osname);
	free(label);
	free(fileSize);
	free(fileName);
	free(number_files);
	free(number_FAT_copies);
	free(sectors_per_FAT);	
	fclose(fp);
	
	return 0;
}

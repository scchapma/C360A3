#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>

#define BYTES_PER_SECTOR 512
#define SIZE 256

void *emalloc(size_t n){
    void *p;
    p = malloc(n);
    if (p == NULL) {
        fprintf(stderr, "malloc of %lu bytes failed", n);
        exit(1);
    }
    return p;
}

void splitString (char **input, char **file_name, char **file_extension)
{
	char *inputString[2];

	inputString[0] = strtok(*input, ".");
	inputString[1] = strtok(NULL, " ");

	printf("file, extension: %s, %s\n", inputString[0], inputString[1]); 

	*file_name = inputString[0];
	*file_extension = inputString[1];

	/*
	char inputFileName[15];
	strncpy(inputFileName, *input, 12);


	char *inputString[2];

	inputString[0] = strtok(inputFileName, ".");
	inputString[1] = strtok(NULL, " ");

	printf("file, extension: %s, %s\n", inputString[0], inputString[1]); 

	*file_name = inputString[0];
	*file_extension = inputString[1];
	*/

}


void findFile (FILE *fp, char *file_name, char *file_extension, unsigned int *first_sector)
{
	printf("Enter findFile.\n");
	unsigned int base = 9728;  // the first byte of the root directory
	unsigned int cur = base;   // point to the first byte of the current entry
	unsigned int offset = 32;  // Each entry has 32 bytes in root directory
	unsigned int extension_offset = 8;
	unsigned int cluster_offset = 26;

	//int *tmp1 = malloc(sizeof(int));
	unsigned  tmp1; 
	unsigned char tmp2;
	unsigned char tmp3;

	char *currentFileName = malloc(sizeof(char)*8);
	char *currentFileExtension = malloc(sizeof(char)*3);

	fseek(fp, base, SEEK_SET);	
	fread(&tmp1,1,1,fp);

	/* Read "notes on directory entries" for the correct conditions of how we identify a file */
	/* Why 0x00 here? 0x00 means this entry and all remaining entries are free */
	while(tmp1 != 0x00)  
	{
		// Search for files
		// 0xE5 indicates that the directory entry is free (i.e., currently unused) 
		// Assume that only files have names and extensions
		if (tmp1 != 0xE5)
		{
			fseek(fp, cur, SEEK_SET);
			fread(currentFileName,1,8,fp);
			currentFileName = strtok(currentFileName, " ");
			
			fseek(fp, cur + extension_offset, SEEK_SET);
			fread(currentFileExtension,1,3,fp);
			
			//compare current file with target file
			if (!strcmp(file_name, currentFileName) && !strcmp(file_extension, currentFileExtension))
			{
				//get address of first sector
				fseek(fp, cur + cluster_offset, SEEK_SET);
				fread(&tmp2, 1, 1, fp);  // get all 8 bits 
				fread(&tmp3,1 ,1, fp); 
				printf("tmp2: %02x\n", tmp2);
				printf("tmp3: %02x\n", tmp3);					
				*first_sector = (tmp3 << 8) + tmp2; 
				printf("first sector initialized: %d\n", *first_sector);

				break;
			}
		}
		
		// Go to next entry in Root Directory
		cur = cur + offset;
		fseek(fp, cur, SEEK_SET);
		fread(&tmp1,1,1,fp);
	}

	//free(tmp1);
	free(currentFileName);
	free(currentFileExtension);
}

int nextSector(FILE *fp, unsigned int *fat_sector)
{
	unsigned int n = *fat_sector;  // logical number of the first sector in Data Area
	unsigned int base = 512; // the first byte of the FAT table 

	unsigned char tmp1;
	unsigned char tmp2;
	unsigned int result;

	// if the logical sector number is even
	if (n % 2 == 0)
	{
		fseek(fp, base + 3*n/2, SEEK_SET);
		fread(&tmp1, 1, 1, fp);  // get all 8 bits 
		fread(&tmp2, 1 ,1, fp);
		tmp2 = tmp2 & 0x0F;   // use mask to get the low 4 bits 

		// Then apply "Little Endian": (4 bits)**** + (8 bits)********
		result = (tmp2 << 8) + tmp1;  
	}

	// if the logical sector number is odd
	else
	{
		fseek(fp, base + 3*n/2, SEEK_SET);
		fread(&tmp1, 1, 1, fp);  // get all 8 bits 
		fread(&tmp2, 1 ,1, fp);
		tmp1 = tmp1 & 0xF0;   // use mask to get the low 4 bits 

		// Then apply "Little Endian": (4 bits)**** + (8 bits)********
		result = (tmp2 << 4) + (tmp1 >> 4); 
	}

	*fat_sector = result;
	
	if (result >= 0xFF0 && result <= 0xFF6)
	{
		printf("Reserved cluster - exiting.\n");
		return 1;
	}
	else if (result == 0xFF7)
	{
		printf("Bad cluster - exiting.\n");
		return 1;
	}
	else if (result >= 0xFF8 && result <= 0xFFF)
	{
		printf("Last cluster in file - exiting.\n");
		return 1;
	}
	else if (result == 0x00) 
	{
		printf("Unused cluster encountered - exiting.\n");
		return 1;
	}
	else
	{
		return 0;
	}	
}

void writeFile(FILE *fp, char *diskname, char *filename, unsigned int *first_sector)
{
	printf("Enter writeFile.\n");
	printf("first sector: %d\n", *first_sector);
	FILE *fp2 = NULL;
	char buffer[512];

	char *tmp1 = malloc(sizeof(char));
	unsigned int fat_sector = *first_sector;
	
	unsigned int j;
	unsigned int physical_sector;
	unsigned int cur;		
	
	if ((fp2 = fopen(filename, "w")))
	{	
		do
		{
			physical_sector = 33 + fat_sector - 2; 
			cur = physical_sector * BYTES_PER_SECTOR;
			fseek(fp,cur,SEEK_SET);
			
			for (j = 0; j < 512; j++){
				fread(tmp1,1,1,fp);
				buffer[j] = *tmp1;
			}
			
			fseek(fp,cur,SEEK_SET);
			fwrite(buffer, 1, 512, fp2);

			printf("fat_sector: %d\n", fat_sector);

		}while (!nextSector(fp, &fat_sector));	
		
		fclose(fp2);
	}
	else
	{
		printf("Fail to open the target file.\n");
	}

	free(tmp1);
}

int main(int argc, char *argv[])
{
	FILE *fp;
	char *file_name; 
	char *file_extension; 
	
	unsigned int *first_sector = (unsigned int *) emalloc(sizeof(unsigned char)*2);

	if ( argc != 3 ) {
		fprintf(stderr, "usage: <diskname> <file name>\n");
		exit(1);
	}

	char *input = argv[2];

	if ((fp=fopen(argv[1],"r")))
	{
		splitString (&input, &file_name, &file_extension);
		findFile (fp, file_name, file_extension, first_sector);
		writeFile(fp, argv[1], argv[2], first_sector);
	}
	else
	{
		printf("Fail to open the image file.\n");
	}
    			
	free(first_sector);

	return 0;
}



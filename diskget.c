#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>

#define BYTES_PER_SECTOR 512
#define SIZE 256

void splitString (char **input, char **file_name, char **file_extension)
{
	char **ap, *argv[2];

	for (ap = argv; (*ap = strsep(input, ".")) != NULL;)
       if (**ap != '\0')
           if (++ap >= &argv[2])
               break;
	
	*file_name = argv[0];
	*file_extension = argv[1];
	//TODO:  
	//Don't get rid of this print statement!
	//Related to problems with adding extensions to file?
	printf("file name and extension: %s.%s\n", argv[0], argv[1]);
}

void findFile (FILE *fp, char *file_name, char *file_extension, int *first_sector)
{
	int base = 9728;  // the first byte of the root directory
	int cur = base;   // point to the first byte of the current entry
	int offset = 32;  // Each entry has 32 bytes in root directory
	int extension_offset = 8;
	int cluster_offset = 26;

	int *tmp1 = malloc(sizeof(int));
	int tmp2, tmp3;

	char *currentFileName = malloc(sizeof(char)*8);
	char *currentFileExtension = malloc(sizeof(char)*3);

	fseek(fp, base, SEEK_SET);	
	fread(tmp1,1,1,fp);

	/* Read "notes on directory entries" for the correct conditions of how we identify a file */
	/* Why 0x00 here? 0x00 means this entry and all remaining entries are free */
	while(*tmp1 != 0x00)  
	{
		// Search for files
		// 0xE5 indicates that the directory entry is free (i.e., currently unused) 
		// Assume that only files have names and extensions
		if (*tmp1 != 0xE5)
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
				*first_sector = (tmp3 << 8) + tmp2; 
				break;
			}
		}
		
		// Go to next entry in Root Directory
		cur = cur + offset;
		fseek(fp, cur, SEEK_SET);
		fread(tmp1,1,1,fp);
	}

	free(tmp1);
	free(currentFileName);
	free(currentFileExtension);
}

int nextSector(FILE *fp, int *fat_sector)
{
	int n = *fat_sector;  // logical number of the first sector in Data Area
	int base = 512; // the first byte of the FAT table 

	int tmp1 = 0;
	int tmp2 = 0;

	int result = 0;

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

void writeFile(FILE *fp, char *diskname, char *filename, int *first_sector)
{
	FILE *fp2 = NULL;
	char buffer[512];

	char *tmp1 = malloc(sizeof(char));
	int fat_sector = *first_sector;
	
	int j = 0;
	int physical_sector = 0;
	int cur = 0;		
	
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
	
	int *first_sector = malloc(sizeof(char)*2);

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



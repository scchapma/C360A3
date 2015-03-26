#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>

#define BYTES_PER_SECTOR 512
#define SIZE 256

//void splitString (char **input, char **file_name, char **file_extension)
void splitString (char **input, char **file_name, char **file_extension)
{
	/*
	file_name = strsep(&input, ".");
	file_extension = strsep(&input, " ");
	*/
	
	char **ap, *argv[2];

	for (ap = argv; (*ap = strsep(input, ".")) != NULL;)
       if (**ap != '\0')
           if (++ap >= &argv[2])
               break;
	
	*file_name = argv[0];
	*file_extension = argv[1];
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
				*first_sector += 31;
				printf("First sector: %d\n", *first_sector);
				break;
			}
			else
			{
				printf("Strings not equal: %s, %s, %s, %s\n", file_name, currentFileName, file_extension, currentFileExtension);
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

void writeFile(FILE *fp, char *diskname, char *filename)
{
	FILE *fp2 = NULL;

	char buffer[512];

	char *tmp1 = malloc(sizeof(char));

	int base = 16896;
	int i = 0;
	int j = 0;
	
	if ((fp2 = fopen(filename, "w")))
	{	
		int cur = base;

		for (i = 0; i<98; i++){
			fseek(fp,cur,SEEK_SET);
			for (j = 0; j < 512; j++){
				fread(tmp1,1,1,fp);
				buffer[j] = *tmp1;
			}
			fseek(fp,cur,SEEK_SET);
			fwrite(buffer, 1, 512, fp2);
			cur += 512;
		}
		fclose(fp2);
	}
	else
	{
		printf("Fail to open the target file.\n");
	}

	free(tmp1);
}

void getFirstSector()
{
	;
}

int main(int argc, char *argv[])
{
	FILE *fp;
	char *file_name = malloc(sizeof(char)*8);
	char *file_extension = malloc(sizeof(char)*3);
	int *first_sector = malloc(sizeof(char)*2);

	if ( argc != 3 ) {
		fprintf(stderr, "usage: <diskname> <file name>\n");
		exit(1);
	}

	char *input = argv[2];

	if ((fp=fopen(argv[1],"r")))
	{
		//parseInput(fp, argv[2]);
		splitString (&input, &file_name, &file_extension);
		printf("file_name: %s\n", file_name);
		printf("file_extension: %s\n", file_extension);
		findFile (fp, file_name, file_extension, first_sector);
		writeFile(fp, argv[1], argv[2]);
	}
	else
	{
		printf("Fail to open the image file.\n");
	}
    			
	return 0;
}



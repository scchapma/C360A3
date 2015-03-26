#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>

#define BYTES_PER_SECTOR 512
#define SIZE 256

void splitString (char *input, char *file_name, char *file_extension)
{
	file_name = strsep(&input, ".");
	file_extension = strsep(&input, " ");
	printf("file name and extension: %s.%s\n", file_name, file_extension);
	
	/*
	char **ap, *argv[2], *inputstring;

	for (ap = argv; (*ap = strsep(&inputstring, ".")) != NULL;)
       if (**ap != '\0')
           if (++ap >= &argv[2])
               break;
	*/
}

void findFile (char *file_name, char *file_extension)
{
	//set pointer to first item in root directory



	//iterate through root directory, searching for match
	//if match, write file to directory
	//else, print message and return
}

void getNumberFiles(FILE *fp, int* number_files, char* fileName)
{
	int base = 9728;  // the first byte of the root directory
	int cur = base;   // point to the first byte of the current entry
	int offset = 32;  // Each entry has 32 bytes in root directory
	int attribute_offset = 11;

	*number_files = 0;

	int *tmp1 = malloc(sizeof(int));
	char *tmp2 = malloc(sizeof(char));

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
			//printf("Attribute: %d\n", tmp2);
			
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

void parseInput(char *input)
{
	//printf("Input string: %s\n", input);
	//split input into file name and file extension
	char *file_name = malloc(sizeof(char)*8);
	char *file_extension = malloc(sizeof(char)*3);
	splitString (input, file_name, file_extension);
	
	//traverse root directory looking for match
	findFile (file_name, file_extension);
	
	free (file_name);
	free (file_extension);
}

void writeFile(FILE *fp, char *diskname, char *filename)
{
	//FILE *fp = NULL;
	FILE *fp2 = NULL;

	char buffer[512];

	char *tmp1 = malloc(sizeof(char));

	int base = 16896;
	int i = 0;
	int j = 0;
	
	//if ((fp=fopen("disk2.IMA","r")))
	//if ((fp=fopen(diskname,"r")))
	//{
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

	if ( argc != 3 ) {
		fprintf(stderr, "usage: <diskname> <file name>\n");
		exit(1);
	}

	if ((fp=fopen(argv[1],"r")))
	{
		parseInput(argv[2]);
		//traverse root directory for file name
		//if no file name, print "File Not Found" and return
		//else, write file to Linux directory
		getFirstSector();
		writeFile(fp, argv[1], argv[2]);
	}
	else
	{
		printf("Fail to open the image file.\n");
	}
    			
	return 0;
}



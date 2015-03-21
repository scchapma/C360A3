#include<stdio.h>
#include<stdlib.h>

#define BYTES_PER_SECTOR 512


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

void testAttributes(FILE *fp, int cur, int *fileFlag, int *directoryFlag, char *fileName)
{
	//reset flags to off
	*fileFlag = 0;
	*directoryFlag = 0;

	int attribute_offset = 11;
	char tmp;

	fseek(fp, cur + attribute_offset, SEEK_SET);
	fread(&tmp,1,1,fp);

	//test for directory
	if(tmp == 0x0F)
	{
		return;
	}
	else if(tmp & 0x10)
	{
		*directoryFlag = 1;
		printf("Directory found.\n");
	}
	//test for label
	else if (tmp & 0x08)
	{
		fseek(fp, cur, SEEK_SET);
		fread(fileName,11,8,fp);
		printf("File Name found.\n");
	}
	//test for file - TODO:  Logic correct?  Any other possibilities?
	else 
	{
		*fileFlag = 1;
		printf("File found.\n");
	}
}

void getFileSize(FILE *fp, int cur, long *fileSize)
{
	int file_size_offset = 28;
	int tmp1, tmp2, tmp3, tmp4;

	fseek(fp, cur + file_size_offset, SEEK_SET);
	fread(&tmp1,1,1,fp);
	fread(&tmp2,1,1,fp);
	fread(&tmp3,1,1,fp);
	fread(&tmp4,1,1,fp);

	*fileSize = tmp1 + (tmp2 << 8) + (tmp3 << 16) + (tmp4 << 24);
}

void getFileCreationDate(FILE *fp, int cur, int *fileDate)
{
	int file_date_offset = 16;
	int tmp1, tmp2;

	fseek(fp, cur + file_date_offset, SEEK_SET);
	fread(&tmp1,1,1,fp);
	fread(&tmp2,1,1,fp);

	*fileDate = tmp1 + (tmp2 << 8);

	//break date down into year, month, and day
	//year is bits 15 to 9 - use mask 0xFE00
	//month is bits 8 to 5 - use mask 0x01E0
	//day is bits 4 to 0 - use mask 0x001F
}

void getFileCreationTime(FILE *fp, int cur, int *fileTime)
{
	int file_time_offset = 14;
	int tmp1, tmp2;

	fseek(fp, cur + file_time_offset, SEEK_SET);
	fread(&tmp1,1,1,fp);
	fread(&tmp2,1,1,fp);

	*fileTime = tmp1 + (tmp2 << 8);

	//break time down into hours, minutes and seconds
	//hour is bits 15 to 11 - use mask 0xF800
	//minute is bits 10 to 5 - use mask 0x07E0
	//second is bits 4 to 0 - use mask 0x001F
}

// loop through the root directory
// Each entry has 32 bytes in root directory
//void getNumberFiles(FILE *fp, int* number_files)
void parseDirectory(FILE *fp, int *fileFlag, int *directoryFlag, long *fileSize, char *fileName, int *fileDate, int *fileTime)
{
	int base = 9728;  // the first byte of the root directory

	int cur = base;   // point to the first byte of the current entry
	int offset = 32;  // Each entry has 32 bytes in root directory

	int attribute_offset = 11;

	fseek(fp, base, SEEK_SET);
	//char tmp;
	//char tmp2;
	int tmp;
	char tmp2;
	fread(&tmp,1,1,fp);

	//traverse each item in root directory
	while(tmp != 0x00)  
	{
		// Search for files
		// 0xE5 indicates that the directory entry is free (i.e., currently unused) 
		if (tmp != 0xE5)
		{
			//test for regular file or directory and set flag
			//get file name
			testAttributes(fp, cur, fileFlag, directoryFlag, fileName);

			//get file size	
			getFileSize(fp, cur, fileSize);
			printf("FileSize: %ld bytes.\n", *fileSize);

			//get file creation date
			getFileCreationDate(fp, cur, fileDate);
			printf("FileDate: %d\n", *fileDate);

			//get file creation time
			getFileCreationTime(fp, cur, fileTime);
			printf("FileTime: %d\n", *fileTime);

			//print formatted directory listing

		}
		
		// Go to next entry in Root Directory
		cur = cur + offset;
		fseek(fp, cur, SEEK_SET);
		fread(&tmp,1,1,fp);
	}
	//*number_files = counter;
	//printf("Number of files: %d\n", *number_files);
}

int main()
{
	FILE *fp;
	int *fileFlag = malloc(sizeof(int));
	int *directoryFlag = malloc(sizeof(int));
	long *fileSize = malloc(sizeof(long));
	char *fileName = malloc(sizeof(char)*8*8);
	int *fileDate = malloc(sizeof(int));
	int *fileTime = malloc(sizeof(int));

	int size;
	
	if ((fp=fopen("disk2.IMA","r")))
	{
		//printf("Successfully open the image file.\n");
		parseDirectory(fp, fileFlag, directoryFlag, fileSize, fileName, fileDate, fileTime);
	}
	else
	{
		printf("Fail to open the image file.\n");
	}

	free(fileFlag);
	free(directoryFlag);
	free(fileSize);
	free(fileName);
	free(fileDate);
	free(fileTime);	
	fclose(fp);
	
	return 0;
}

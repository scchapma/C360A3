#include<stdio.h>
#include<stdlib.h>
#include<time.h>

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
		//printf("No file or directory found.\n");
	}
	//smooth out logic
	else if(tmp & 0x10)
	{
		*directoryFlag = 1;
		//printf("Directory found.\n");
		fseek(fp, cur, SEEK_SET);
		fread(fileName,8,8,fp);	
		printf("File name: %s\n", fileName);
	}
	//test for label
	else if (tmp & 0x08)
	{
		fseek(fp, cur, SEEK_SET);
		//fread(fileName,11,8,fp
		fread(fileName,8,8,fp);	
		printf("File name: %s\n", fileName);
		//printf("File Name found.\n");
	}
	//test for file - TODO:  Logic correct?  Any other possibilities?
	else 
	{
		*fileFlag = 1;
		//printf("File found.\n");
		fseek(fp, cur, SEEK_SET);
		fread(fileName,8,8,fp);	
		printf("File name: %s\n", fileName);
	}
}

void getFileSize(FILE *fp, int cur, long *fileSize)
{

	int file_size_offset = 28;
	int *tmp1 = malloc(sizeof(int));
	int *tmp2 = malloc(sizeof(int));
	int *tmp3 = malloc(sizeof(int));
	int *tmp4 = malloc(sizeof(int));

	long retVal;

	fseek(fp, cur + file_size_offset, SEEK_SET);
	fread(tmp1,1,1,fp);
	fread(tmp2,1,1,fp);
	fread(tmp3,1,1,fp);
	fread(tmp4,1,1,fp);

	*fileSize = *tmp1+((*tmp2)<<8) + ((*tmp3) << 16) + ((*tmp4) << 24);

	free(tmp1);
	free(tmp2);
	free(tmp3);
	free(tmp4);
}

void getFileCreationDate(FILE *fp, int cur, int *fileDate, int *year, int *month, int *day)
{
	
	int file_date_offset = 16;
	
	int *tmp1 = malloc(sizeof(int));
	int *tmp2 = malloc(sizeof(int));
	int retVal;

	fseek(fp, cur + file_date_offset, SEEK_SET);
	fread(tmp1,1,1,fp);
	fread(tmp2,1,1,fp);

	*fileDate = *tmp1 + ((*tmp2) << 8);

	free(tmp1);
	free(tmp2);

	//break date down into year, month, and day
	//year is bits 15 to 9 - use mask 0xFE00
	//int year = (*fileDate & 0xFE00) >> 9;
	*year = (*fileDate & 0xFE00) >> 9;
	//month is bits 8 to 5 - use mask 0x01E0
	//int month = (*fileDate & 0x01E0) >> 5;
	*month = (*fileDate & 0x01E0) >> 5;
	//day is bits 4 to 0 - use mask 0x001F
	//int day = (*fileDate & 0x001F);
	*day = (*fileDate & 0x001F);

	//printf("Date: %d - %d - %d\n", *month, *day, (*year + 1980));
}

void getFileCreationTime(FILE *fp, int cur, int *fileTime, int *hour, int *minute, int *second)
{

	int file_time_offset = 14;
	
	int *tmp1 = malloc(sizeof(int));
	int *tmp2 = malloc(sizeof(int));
	int retVal;

	fseek(fp, cur + file_time_offset, SEEK_SET);
	fread(tmp1,1,1,fp);
	fread(tmp2,1,1,fp);

	*fileTime = *tmp1 + ((*tmp2) << 8);

	free(tmp1);
	free(tmp2);

	//break time down into hours, minutes and seconds
	//hour is bits 15 to 11 - use mask 0xF800
	//int hour = (*fileTime & 0xF800) >> 11;
	*hour = (*fileTime & 0xF800) >> 11;
	//minute is bits 10 to 5 - use mask 0x07E0
	//int minute = (*fileTime & 0x07E0)>> 5;
	*minute = (*fileTime & 0x07E0)>> 5;
	//second is bits 4 to 0 - use mask 0x001F
	//int second = (*fileTime & 0x001F);
	*second = (*fileTime & 0x001F);

	//printf("Time: %4d - %2d - %2d\n", *hour, *minute, *second);
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

	//create additional pointers for time and date
	int *year = malloc(sizeof(int));
	int *month = malloc(sizeof(int));
	int *day = malloc(sizeof(int));
	int *hour = malloc(sizeof(int));
	int *minute = malloc(sizeof(int));
	int *second = malloc(sizeof(int));

	//add time struct
	struct tm str_time;
	time_t time_of_day;

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
			//printf("FileSize: %ld bytes.\n", *fileSize);

			//get file creation date
			getFileCreationDate(fp, cur, fileDate, year, month, day);
			//printf("FileDate: %d\n", *fileDate);

			//get file creation time
			getFileCreationTime(fp, cur, fileTime, hour, minute, second);
			//printf("FileTime: %d\n", *fileTime);

			//populate time struct
			str_time.tm_year = *year + 80;
			str_time.tm_mon = *month - 1;
			str_time.tm_mday = *day;
			str_time.tm_hour = *hour;
			str_time.tm_min = *minute;
			str_time.tm_sec = *second;

			time_of_day = mktime(&str_time);
			printf(ctime(&time_of_day));

			//print formatted directory listing
			if(*directoryFlag || *fileFlag){
				if (*directoryFlag) printf("D ");
				else if (*fileFlag) printf("F ");

				printf("      %ld   ", *fileSize);
				printf("      %s    ", fileName);
				printf("%4d-%2d-%2d  %2d:%2d\n", *year, *month, *day, *hour, *minute);
			}
		}
		
		// Go to next entry in Root Directory
		cur = cur + offset;
		fseek(fp, cur, SEEK_SET);
		fread(&tmp,1,1,fp);
	}
	
	free(year);
	free(month);
	free(day);
	free(hour);
	free(minute);
	free(second);
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

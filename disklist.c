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

void testAttributes(FILE *fp, int cur, unsigned char *fileFlag, unsigned char *directoryFlag, char *fileName, char *fileExtension)
{
	//reset flags to off
	*fileFlag = 0;
	*directoryFlag = 0;

	int attribute_offset = 11;
	unsigned char tmp;

	fseek(fp, cur + attribute_offset, SEEK_SET);
	fread(&tmp,1,1,fp);

	//test for directory
	if(tmp == 0x0F)
	{
		return;
	}
	else
	{
		fseek(fp, cur, SEEK_SET);	
		fread(fileName, 1, 8, fp);	
		fread(fileExtension, 1, 3, fp);

		if(tmp & 0x10) *directoryFlag = 1;
		else if (tmp & 0x08) return;
		else *fileFlag = 1;
	}
}

void getFileSize(FILE *fp, int cur, unsigned int *fileSize)
{
	unsigned int file_size_offset = 28;
	unsigned char tmp1; 
	unsigned char tmp2; 
	unsigned char tmp3; 
	unsigned char tmp4; 

	fseek(fp, cur + file_size_offset, SEEK_SET);
	fread(&tmp1,1,1,fp);
	fread(&tmp2,1,1,fp);
	fread(&tmp3,1,1,fp);
	fread(&tmp4,1,1,fp);

	*fileSize = tmp1+((tmp2)<<8) + ((tmp3) << 16) + ((tmp4) << 24);
}

void getFileCreationDate(FILE *fp, int cur, unsigned int *fileDate, unsigned char *year, unsigned char *month, unsigned char *day)
{
	unsigned int file_date_offset = 16;	
	unsigned char tmp1; 
	unsigned char tmp2;

	fseek(fp, cur + file_date_offset, SEEK_SET);
	fread(&tmp1,1,1,fp);
	fread(&tmp2,1,1,fp); 

	*fileDate = tmp1 + ((tmp2) << 8);

	//break date down into year, month, and day
	//year is bits 15 to 9 - use mask 0xFE00
	*year = (*fileDate & 0xFE00) >> 9;	
	//month is bits 8 to 5 - use mask 0x01E0
	*month = (*fileDate & 0x01E0) >> 5;
	//day is bits 4 to 0 - use mask 0x001F
	*day = (*fileDate & 0x001F);
}

void getFileCreationTime(FILE *fp, int cur, unsigned int *fileTime, unsigned char *hour, unsigned char *minute, unsigned char *second)
{
	unsigned int file_time_offset = 14;	
	unsigned char tmp1;
	unsigned char tmp2;

	fseek(fp, cur + file_time_offset, SEEK_SET);
	fread(&tmp1,1,1,fp);
	fread(&tmp2,1,1,fp);

	*fileTime = tmp1 + ((tmp2) << 8);

	//break time down into hours, minutes and seconds
	//hour is bits 15 to 11 - use mask 0xF800
	*hour = (*fileTime & 0xF800) >> 11;
	//minute is bits 10 to 5 - use mask 0x07E0
	*minute = (*fileTime & 0x07E0)>> 5;
	//second is bits 4 to 0 - use mask 0x001F
	*second = (*fileTime & 0x001F);
}

void printReport(struct tm str_time, char *buffer, unsigned char *directoryFlag, unsigned char *fileFlag, unsigned int *fileSize, char *fileName, char *fileExtension)
{
	if(*directoryFlag || *fileFlag){
		if (*directoryFlag) printf("D ");
		else if (*fileFlag) printf("F ");

		printf("%10d", *fileSize);
		char file[50];
		fileName = strtok(fileName, " ");
		strcpy(file, fileName);
		strcat(file, ".");
		strcat(file, fileExtension);
		printf("%20s", file);
		strftime (buffer, SIZE, " %F %H:%M\n", &str_time);
		fputs(buffer, stdout);
	}
}

// loop through the root directory
// Each entry has 32 bytes in root directory
void parseDirectory(FILE *fp, unsigned char *fileFlag, unsigned char *directoryFlag, unsigned int *fileSize, char *fileName, char *fileExtension, unsigned int *fileDate, unsigned int *fileTime)
{
	unsigned int base = 9728;  // the first byte of the root directory
	unsigned int cur = base;   // point to the first byte of the current entry
	unsigned int offset = 32;  // Each entry has 32 bytes in root directory

	//int *tmp1 = (int *) emalloc(sizeof(int));
	unsigned char tmp1; 

	fseek(fp, base, SEEK_SET);
	fread(&tmp1,1,1,fp);
	
	//create additional pointers for time and date	
	unsigned char *year = (unsigned char *) emalloc(sizeof(unsigned char));
	unsigned char *month = (unsigned char *) emalloc(sizeof(unsigned char));
	unsigned char *day = (unsigned char *) emalloc(sizeof(unsigned char));

	unsigned char *hour = (unsigned char *) emalloc(sizeof(unsigned char));
	unsigned char *minute = (unsigned char *) emalloc(sizeof(unsigned char));
	unsigned char *second = (unsigned char *) emalloc(sizeof(unsigned char));

	//add time struct
	struct tm str_time;
	char buffer[SIZE];

	//traverse each item in root directory
	while(tmp1 != 0x00)  
	{
		// Search for files
		// 0xE5 indicates that the directory entry is free (i.e., currently unused) 
		if (tmp1 != 0xE5)
		{
			//test for regular file or directory and set flag
			//get file name
			testAttributes(fp, cur, fileFlag, directoryFlag, fileName, fileExtension);			
			getFileSize(fp, cur, fileSize);
			getFileCreationDate(fp, cur, fileDate, year, month, day);
			getFileCreationTime(fp, cur, fileTime, hour, minute, second);

			//populate time struct
			str_time.tm_year = *year + 80;
			str_time.tm_mon = *month - 1;
			str_time.tm_mday = *day;
			str_time.tm_hour = *hour;
			str_time.tm_min = *minute;
			str_time.tm_sec = *second;

			printReport(str_time, buffer, directoryFlag, fileFlag, fileSize, fileName, fileExtension);
		}
		
		// Go to next entry in Root Directory
		cur = cur + offset;
		fseek(fp, cur, SEEK_SET);
		fread(&tmp1,1,1,fp);
	}
	
	free(year);
	free(month);
	free(day);
	free(hour);
	free(minute);
	free(second);
	//free(tmp1);
}

int main()
{
	FILE *fp;
	unsigned char *fileFlag = (unsigned char *) emalloc(sizeof(unsigned char));
	unsigned char *directoryFlag = (unsigned char *) emalloc(sizeof(unsigned char));
	unsigned int *fileSize = (unsigned int *) emalloc(sizeof(unsigned int));
	char *fileName = (char *) emalloc(sizeof(char)*8*8);
	char *fileExtension = (char *) emalloc(sizeof(char)*3*8);
	unsigned int *fileDate = (unsigned int *) emalloc(sizeof(unsigned int));
	unsigned int *fileTime = (unsigned int *) emalloc(sizeof(unsigned int));
	
	if ((fp=fopen("disk2.IMA","r")))
	{
		//printf("Successfully open the image file.\n");
		parseDirectory(fp, fileFlag, directoryFlag, fileSize, fileName, fileExtension, fileDate, fileTime);
	}
	else
	{
		printf("Fail to open the image file.\n");
	}

	free(fileFlag);
	free(directoryFlag);
	free(fileSize);
	free(fileName);
	free(fileExtension);
	free(fileDate);
	free(fileTime);	
	
	fclose(fp);
	
	return 0;
}

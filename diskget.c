#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<string.h>

#define BYTES_PER_SECTOR 512
#define SIZE 256

int main()
{
	FILE *fp;
	FILE *fp2;

	char buffer[512];

	char *tmp1 = malloc(sizeof(char));
	//char *tmp2 = malloc(sizeof(char));
	//char *tmp3 = malloc(sizeof(char));
	//char *tmp4 = malloc(sizeof(char));

	int base = 16896;
	int i = 0;
	int j = 0;
	
	if ((fp=fopen("disk2.IMA","r")))
	{
		fp2 = fopen("file4.pdf" , "w");
		
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
		printf("Fail to open the image file.\n");
	}
	
	return 0;
}



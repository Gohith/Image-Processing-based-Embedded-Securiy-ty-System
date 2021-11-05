#include "stdio.h"
#include "string.h"
#include "stdlib.h"

char data[] = {"4A8C8CE6BB9F8391CB96F86497680EFA"};
char s1[10], s2[10][10], s3[10];

int main()
{
	int i, j;
	//int val = atoi("8C");
	//val = (val *10);
	for(i=0, j=0; i<32; i++)
	{
		s1[j] = data[i];
		j++; i++;
		s1[j] = data[i];
		j++; 
		s1[j] = '\0';
			
		int ret = strtol(s1, NULL, 16);
		j--; j--;
		printf("val = %d\n", ret);
	}
	 	
	return 0;
}

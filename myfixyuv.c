#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define cif_L  152064 /* (352 * 288 * 1.5) yuv is in 420 format*/
#define qcif_L 	38016 /* (176 * 144 * 1.5) yuv is in 420 format*/

int main(int cn, char *cl[])
{
	int i, j, choice;
	char buf1[cif_L], buf2[qcif_L], last_buf1[cif_L], last_buf2[qcif_L];
	FILE *f=0, *fi=0, *fo=0;
	int nF=0;
	char tmp1[10],tmp2[10],tmp3[10],tmp4[10],tmp5[10],tmp6[10],tmp7[10],tmp8[10];
	int  tmp9, tmp10;

	if (cn != 6) {
    		puts("myfixyuv <tracefile> <cif/qcif> <noframe> <err.yuv> <fix.yuv>");
    		puts("filtered trace file");
    		puts("noframe\t no of frames");
    		puts("err.yuv\tdecoded video");
    		puts("fix.yuv\tfixed video");
    		puts("myfixyuv.exe filteredtrace.txt cif 300 err.yuv fix.yuv");
    		return 0;
   }
  
  	if(strcmp(cl[2],"cif")==0)
  		choice=0;
  	else 
  		choice=1;
  
  	nF=atoi(cl[3]);	
        printf("choice=%d nF=%d\n", choice, nF);
  	int* ptr=malloc(sizeof(int)*(nF+1));
  	for(i=0;i<=nF;i++)
  	   ptr[i]=0;
  	
  	if ((f = fopen(cl[1], "r")) == 0) {
    		fprintf(stderr, "error opening %s\n", cl[1]);
    		return 0;
  	}

  	if ((fi = fopen(cl[4], "rb")) == 0) {
    		fprintf(stderr, "error opening %s\n", cl[4]);
    		return 0;
  	}
  	
  	if ((fo = fopen(cl[5], "wb")) == 0) {
    		fprintf(stderr, "error opening %s\n", cl[5]);
    		return 0;
  	}
   
    while(!feof(f)){
      fscanf(f, "%s%s%s%s%s%s%s%s%d%d", tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, &tmp9, &tmp10);
      //printf("%s %s %s %s %s %s %s %s %d %d\n", tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7, tmp8, tmp9, tmp10);
      if(strcmp(tmp6,"SliceData")==0)
        ptr[tmp9]=1;
    }

    if(ptr[0]==1)
      ptr[1]=1;
  
    //for(i=1; i<=nF; i++) {
    //   if(ptr[i]==1)
    //     printf("%d\n",i);
    //}    
	
    for(i=1; i<=nF; i++) 
    {
      if(ptr[i]==1)
      { 
        if(choice==0) 
        {
          if(!feof(fi))
          {
              printf("copy ori cif frame %d to fixed video file\n", i);
	      fread(buf1, cif_L, 1, fi);
	      fwrite(buf1, cif_L, 1, fo);
              memcpy(last_buf1, buf1, cif_L);
	  }
	} 
        else 
        {
	  if(!feof(fi))
          {
	      fread(buf2, qcif_L, 1, fi);
	      fwrite(buf2, qcif_L, 1, fo);
              memcpy(last_buf2, buf2, qcif_L);
          }
	}
      }  
      else 
      {
          if(choice==0) {
                printf("copy prvious cif frame to fixed video file for error frame %d\n", i); 
		fwrite(last_buf1, cif_L, 1, fo);
          } else { 
		fwrite(last_buf2, qcif_L, 1, fo);	
          }
      }  
    }
    
    free(ptr);
    fclose(f);
    fclose(fi);
    fclose(fo);
  	
    return 0;	
}


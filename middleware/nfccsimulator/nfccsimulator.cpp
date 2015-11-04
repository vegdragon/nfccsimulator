#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>
#include <string.h>
// #include <iostream>
#define DEVICE_NAME "/dev/pn54x"  

typedef struct nci_data
{
    long             timestamp;
    long             delay;
    char             direction;    // 'X': HOST->NFCC    'R':NFCC->HOST
    unsigned char    data[300];
    int              len;
} nci_data_t;

using namespace std;

void strToHex (const char * str, nci_data_t & nciData)
{
  const char *p  = str;

  int i = 0;

  printf("str=%s\n", str);

  // line end with 0D0A
  for(i=0;*p!=0xd && *p!=0xa;i++) 
  {
    unsigned char byte;

    byte = (*p-'0') * 0x10 + (*(p+1)-'0');
    nciData.data[i] = byte;
    p += 2;
    printf("data[%d] = 0x%02x, *(p)=0x%x\n", i, byte, *(p));
  }

  nciData.len = i;
}

#if __USE_STL__
long parseTimestamp(string & line)
{
  size_t posStartOfTime=0, posEndOfTime=0;
  const char * strTime = NULL;
  long result = 0;
  long h, m, s, ms;

  posStartOfTime = line.find(" ") + 1;
  posEndOfTime = line.find(" D/NxpNci");
  strTime = line.substr(posStartOfTime, posEndOfTime).c_str();
  h = (strTime[0] - '0') * 10 + strTime[1];
  m = (strTime[3] - '0') * 10 + strTime[4];
  s = (strTime[6] - '0') * 10 + strTime[7];
  ms = (strTime[9] - '0') * 100 + (strTime[10] - '0') * 10 + strTime[11];
  result = h * 60*60 * 1000 + m * 60*60 * 1000 + s * 1000 + ms;

  printf("timestamp: %ld\n",result);
  return result;
}


int readNciDataFromFile(const char * fileName, int fd)
{
    string line;
    ifstream infile(fileName);
    string str ("> ");
    size_t found;
        long timestamp = 0;
    while (getline(infile, line))
    {
        nci_data_t nciData;
        nciData.timestamp = parseTimestamp(line);
        found = line.find(str);
        if (found!=string::npos)
        {
            
            strToHex (line.substr (found+2).c_str(), nciData);
            printf("Send ioctl cmd 1...\n");
                ioctl(fd, 1, &nciData);
            cout << line.substr (found+2).c_str() << endl;
        }
    }

    infile.close();
    return 0;
}
#else /* __USE_STL__ */
long parseTimestamp(char * line)
{
  size_t posStartOfTime=0, posEndOfTime=0;
  char * strTime = NULL;
  long result = 0;
  long h, m, s, ms;

  strTime = strstr(line, " ");
  if (strTime != NULL) 
    strTime++;
  else
  {
    result = -1;
    goto ret;
  }

  h = (strTime[0] - '0') * 10 + strTime[1];
  m = (strTime[3] - '0') * 10 + strTime[4];
  s = (strTime[6] - '0') * 10 + strTime[7];
  ms = (strTime[9] - '0') * 100 + (strTime[10] - '0') * 10 + strTime[11];
  result = h * 60*60 * 1000 + m * 60*60 * 1000 + s * 1000 + ms;

  printf("timestamp: %ld\n",result);
  
ret:
  return result;
}

int readNciDataFromFile(const char * fileName, int fd)
{
   FILE * fp;
   char * line = NULL;
   size_t len = 0;
   ssize_t read;
   char * strFound = NULL;
   long prevTimestamp = 0;

   fp = fopen(fileName, "r");
   if (fp == NULL)
       exit(EXIT_FAILURE);

   while ((read = getline(&line, &len, fp)) != -1) {
       printf("Retrieved line of length %zu :\n", read);
       printf("%s", line);
       nci_data_t nciData;
       nciData.timestamp = parseTimestamp(line);
       nciData.delay = nciData.timestamp - prevTimestamp;
       prevTimestamp = nciData.timestamp;
       
       strFound = strchr (line, '>');
       if (strFound != NULL)
       {
         strToHex (strFound+2, nciData);
         nciData.direction = *(strFound - 21);
         printf("strFound=%s, direction=%c\n", strFound, nciData.direction);
         printf("Send ioctl cmd 1...\n");
         ioctl(fd, 1, &nciData);
         printf("%s\n", strFound+2);
       }
   }

   fclose(fp);
   if (line)
       free(line);

   return 0;
}
#endif /* __USE_STL__ */

int main(int argc, char** argv)  
{  
    int fd = -1;  
    int val = 0;  
    //nci_data_t data1, data2;
		// data1.timestamp = 0;
		//strcpy(data1.data, "20000101");
		//strcpy(data2.data, "400003001001");

    fd = open(DEVICE_NAME, O_RDWR);  
    if(fd == -1) {  
        printf("Failed to open device %s.\n", DEVICE_NAME);  
        return -1;  
    }  
      
    printf("Read original value:\n");  
    read(fd, &val, sizeof(val));  
    printf("%d.\n\n", val);  
    val = 5;  
    printf("Write value %d to %s.\n\n", val, DEVICE_NAME);  
        write(fd, &val, sizeof(val));  
      
    printf("Read the value again:\n");  
        read(fd, &val, sizeof(val));  
        printf("%d.\n\n", val); 

    printf("Send ioctl cmd 0...\n");
    ioctl(fd, 0, 1);

		readNciDataFromFile("/etc/nfc_on_off_filtered.log", fd);        

    printf("Send ioctl cmd 2...\n");
    ioctl(fd, 2, 1);
    
printf("closing fd 0...\n");
    close(fd);  

    return 0;  
}  


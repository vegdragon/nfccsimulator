#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <vector>
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

  // printf("str=%s\n", str);

  // line end with 0D0A
  for(i=0;*p!=0xd && *p!=0xa;i++) 
  {
    unsigned char byte;

    byte = (*p-'0') * 0x10 + (*(p+1)-'0');
    nciData.data[i] = byte;
    p += 2;
  // printf("data[%d] = %02x, *(p)=0x%x\n", i, byte, *(p));
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

  printf("%s: enter.\n", __FUNCTION__);

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
  printf("%s: exit.\n", __FUNCTION__);
  return result;
}

int readNciDataFromFile(const char * fileName, 
                        int fd, 
                        std::vector<nci_data_t> &sendingData,
                        std::vector<nci_data_t> &receivingData
                        )
{
  FILE * fp;
  char * line = NULL;
  size_t len = 0;
  ssize_t ret;
  char * strFound = NULL;
  long prevTimestamp = 0;
  nci_data_t * pSendingData = NULL;
  unsigned char    dataRead[300];

  printf("%s: enter.\n", __FUNCTION__);

  fp = fopen(fileName, "r");
  if (fp == NULL)
    goto exit;

  while ((ret = getline(&line, &len, fp)) != -1) {
    printf("Retrieved line of length %zu :\n", read);
    printf("%s", line);
    nci_data_t nciData;
    nciData.timestamp = parseTimestamp(line);
    nciData.delay = prevTimestamp==0 ? 0:nciData.timestamp - prevTimestamp;
    prevTimestamp = nciData.timestamp;

    strFound = strchr (line, '>'); 
    if (strFound != NULL)
    {
      strToHex (strFound+2, nciData);
      nciData.direction = *(strFound - 21);
      printf("strFound=%s, direction=%c\n", strFound, nciData.direction);
      printf("Send ioctl cmd 1...\n");
      ioctl(fd, 1, &nciData);
      printf("%s, \tdirection=%c\n", strFound+2, nciData.direction);
      if ('X' == nciData.direction)
      {
        sendingData.push_back(nciData);
      }
      else if ('R' == nciData.direction)
      {
        receivingData.push_back(nciData);
      }
      else
      {
        printf("Warning: Incorrect data from file!");
      }
    }
    else
    {
      printf("Warning: End of file. Exiting...");
      break;
    }
  }

  fclose(fp);
  if (line)
    free(line);

exit:
  printf("%s: exit.\n", __FUNCTION__);

  return 0;
}
#endif /* __USE_STL__ */


int startCommunication(int fd, 
                        std::vector<nci_data_t> &sendingData,
                        std::vector<nci_data_t> &receivingData)
{
  nci_data_t * pSendingData = NULL;
  nci_data_t * pReceivingData = NULL;
  unsigned char    dataRead[300];

  printf("%s: enter.\n", __FUNCTION__);

  vector<nci_data_t>::iterator v = sendingData.begin();
  while( v != sendingData.end()) {
      // pSendingData = v;
      printf("%s: (*v).delay = %d. (*v).len = %d\n", __FUNCTION__, (*v).delay, (*v).len);
      usleep((*v).delay * 1000);
      write(fd, (*v).data, (*v).len);

      printf("write: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        (*v).data[0],(*v).data[1],(*v).data[2],(*v).data[3],(*v).data[4],
        (*v).data[5],(*v).data[6],(*v).data[7],(*v).data[8],(*v).data[9]);
      read(fd, dataRead, sizeof(dataRead));
      printf("read: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        dataRead[0],dataRead[1],dataRead[2],dataRead[3],dataRead[4],
        dataRead[5],dataRead[6],dataRead[7],dataRead[8],dataRead[9]);

      v++;
   }

   printf("%s: exit.\n", __FUNCTION__);

   return 0;
}

int main(int argc, char** argv)  
{  
  int fd = -1;  
  int val = 0;  
  std::vector<nci_data_t> sendingData, receivingData;

  fd = open(DEVICE_NAME, O_RDWR);  
  if(fd == -1) {  
    printf("Failed to open device %s.\n", DEVICE_NAME);  
    return -1;  
  }  

  readNciDataFromFile("/etc/nfc_on_off_filtered.log", fd, sendingData, receivingData); 
  startCommunication(fd, sendingData, receivingData);


  printf("closing fd 0...\n");
  close(fd);  

  return 0;  
}  


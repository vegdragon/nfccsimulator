#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include "NciLogFileProcessor.h"

using namespace std;

void NciLogFileProcessor::printTimestamp()
{
    struct timeval detail_time;
    gettimeofday(&detail_time,NULL);
    printf("[%d.%d] ",
    detail_time.tv_sec,  /* seconds */
    detail_time.tv_usec); /* microseconds */
}

void NciLogFileProcessor::strToHex (const char * str, nci_data_t & nciData)
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


long NciLogFileProcessor::parseTimestamp(char * line)
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

int NciLogFileProcessor::readNciDataFromFile(const char * fileName, 
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
  int i = 0;

  printf("%s: enter.\n", __FUNCTION__);

  fp = fopen(fileName, "r");
  if (fp == NULL)
    goto exit;

  while ((ret = getline(&line, &len, fp)) != -1 && i<10) {
    i++;
    // printf("Retrieved line of length %zu :\n", read);
    printf("%s", line);
    nci_data_t nciData;
    nciData.timestamp = parseTimestamp(line);
    nciData.delay = prevTimestamp==0 ? 0:nciData.timestamp - prevTimestamp;
    prevTimestamp = nciData.timestamp;
    printf("%s: delay=%d, prevTimestamp=%d\n", __FUNCTION__, nciData.delay, prevTimestamp);

    strFound = strchr (line, '>'); 
    if (strFound != NULL)
    {
      strToHex (strFound+2, nciData);
      nciData.direction = *(strFound - 21);
      // printf("strFound=%s, direction=%c\n", strFound, nciData.direction);
      // printf("Send ioctl cmd 1...\n");
      ioctl(fd, 1, &nciData);
      printf("%s: %s, \tdirection=%c\n", __FUNCTION__, strFound+2, nciData.direction);
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


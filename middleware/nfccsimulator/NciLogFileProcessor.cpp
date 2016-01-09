#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include "NciLogFileProcessor.h"


#define NORMAL_MODE_HEADER_LEN      3

using namespace std;

void NciLogFileProcessor::printTimestamp()
{
    struct timeval detail_time;
    gettimeofday(&detail_time,NULL);
    printf("[%ld.%ld] ",
        detail_time.tv_sec,  /* seconds */
        detail_time.tv_usec); /* microseconds */
}

void NciLogFileProcessor::strToHex (const char * str, nci_data_t & nciData)
{
  const char *p  = str;
  static const char numMap[23] = { 0,1,2,3,4,5,6,7,8,9,'x','x','x','x','x','x','x',0xa,0xb,0xc,0xd,0xe,0xf };

  int i = 0;

  // printf("str=%s\n", str);

  // line end with 0D0A
  for(i=0;*p!=0xd && *p!=0xa;i++) 
  {
    unsigned char byte;
    char bit1, bit2;
    bit1 = *p - '0'; bit2 = *(p+1) - '0';

    // byte = (*p-'0') * 0x10 + (*(p+1)-'0');
    if ((bit1<23) && (bit2<23))
    {
        byte = numMap[(*p-'0')] * 0x10 + numMap[*(p+1)-'0'];
        nciData.data[i] = byte;
        p += 2;
        // printf("data[%d] = %02x, *(p)=0x%x\n", i, byte, *(p));
    }
    else
    {
        printf ("%s(%d): ERROR Invalid data!", __FUNCTION__, __LINE__);
        break;
    }
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
                        std::vector<nci_data_t> &rxData
                        )
{
  FILE * fp;
  char * line = NULL;
  size_t len = 0;
  int ret = 0;
  char * strFound = NULL;
  long prevTimestamp = 0;
  nci_data_t * pSendingData = NULL;
  unsigned char    dataRead[300];
  int idx = 0;

  printf("%s: enter.\n", __FUNCTION__);

  fp = fopen(fileName, "r");
  if (fp == NULL)
  {
    printf("%s: fopen failed!\n", __FUNCTION__);
    ret = -1;
    goto exit;
  }

  for (idx=0;(ret = getline(&line, &len, fp)) != -1;idx++) {
    // printf("Retrieved line of length %zu :\n", read);
    printf("%s", line);
    nci_data_t nciHeader;
    nci_data_t nciData;
    
    printf("%s: delay=%ld, prevTimestamp=%ld\n", __FUNCTION__, nciHeader.delay, prevTimestamp);

    strFound = strchr (line, '>'); 
    if (strFound != NULL)
    {
      strToHex (strFound+2, nciData);
      nciData.index = idx;
      nciData.direction = *(strFound - 21);
      nciData.timestamp = parseTimestamp(line);
      nciData.delay = prevTimestamp==0 ? 0:nciData.timestamp - prevTimestamp;
      prevTimestamp = nciData.timestamp;

      if ('R' == nciData.direction)
      {
        nciHeader.len = NORMAL_MODE_HEADER_LEN;
        nciHeader.index = idx;
        nciHeader.direction  = nciData.direction;
        nciHeader.timestamp = nciData.timestamp;
        nciHeader.delay = prevTimestamp==0 ? 0:nciData.timestamp - prevTimestamp;

        nciHeader.data[0] = nciData.data[0]; 
        nciHeader.data[1] = nciData.data[1];
        nciHeader.data[2] = nciData.data[2];

        ioctl(fd, 1, &nciHeader);
        printf("%s: len(%d), \tdirection(%c)\n", __FUNCTION__, nciData.len, nciHeader.direction);
        rxData.push_back(nciHeader);

        nciData.len -= NORMAL_MODE_HEADER_LEN;
        memcpy (nciData.data, nciData.data+NORMAL_MODE_HEADER_LEN, nciData.len);
        nciData.delay = 0;
        ioctl(fd, 1, &nciData);
        printf("%s: %s, \tdirection=%c\n", __FUNCTION__, strFound+2, nciData.direction);
        rxData.push_back(nciData);
      }
      else if ('X' == nciData.direction)
      {
        

        ioctl(fd, 1, &nciData);
        printf("%s: %s, \tdirection=%c\n", __FUNCTION__, strFound+2, nciData.direction);
        rxData.push_back(nciData);
      }
      else
      {
        ret = -1;
        goto exit;
      }

      
      // printf("strFound=%s, direction=%c\n", strFound, nciData.direction);
      // printf("Send ioctl cmd 1...\n");
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

  return ret;
}


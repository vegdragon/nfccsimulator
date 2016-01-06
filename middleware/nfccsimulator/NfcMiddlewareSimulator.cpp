#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include "NciLogFileProcessor.h"

#define DEVICE_NAME "/dev/pn54x"

#define CMD_NCI_FIFO_INIT       0
#define CMD_NCI_FIFO_PUSH       1
#define CMD_NCI_FIFO_RELEASE    2
#define CMD_NCI_ENGINE_START    3
#define CMD_NCI_ENGINE_STOP     4
#define CMD_NCI_FIFO_GETALL     5


using namespace std;

int startCommunication(int fd, std::vector<nci_data_t> &rxData)
{
  nci_data_t * pSendingData = NULL;
  nci_data_t * pReceivingData = NULL;
  unsigned char    dataRead[300];
  int ret = 0;

  printf("%s: enter.\n", __FUNCTION__);

  vector<nci_data_t>::iterator vData = rxData.begin();
  while( vData != rxData.end()) {
      // pSendingData = v;
      printf("%s: (*vData).delay = %ld. (*vData).len = %d\n", __FUNCTION__, (*vData).delay, (*vData).len);
      if ((*vData).delay>0) usleep((*vData).delay * 1000);

      NciLogFileProcessor::printTimestamp();

      printf("[%d][%ld][%c]: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                  (*vData).index, (*vData).timestamp, (*vData).direction,
                  (*vData).data[0],(*vData).data[1],(*vData).data[2],(*vData).data[3],(*vData).data[4],
                  (*vData).data[5],(*vData).data[6],(*vData).data[7],(*vData).data[8],(*vData).data[9]);

      if ('X' == (*vData).direction)
      {
          write(fd, (*vData).data, (*vData).len); 
      }
      else if ('R' == (*vData).direction)
      {
          read(fd, dataRead, sizeof(dataRead));
          printf("read[%d][%ld][%c]: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
            (*vData).index,(*vData).timestamp,(*vData).direction,
            dataRead[0],dataRead[1],dataRead[2],dataRead[3],dataRead[4],
            dataRead[5],dataRead[6],dataRead[7],dataRead[8],dataRead[9]);
      }
      else
      {
          printf("%s: Invalid Data!!!.\n", __FUNCTION__);
          ret = -1;
          break;
      }

      vData++;
   }
  

   printf("%s: exit.\n", __FUNCTION__);

   return ret;
}

int main(int argc, char** argv)  
{  
  int fd = -1;
  int val = 0;
  std::vector<nci_data_t> rxData;
  NciLogFileProcessor nlfp;
  int i;

  if (argc < 2) return 0;

  fd = open(DEVICE_NAME, O_RDWR);  
  if(fd == -1) 
  {
    printf("Failed to open device %s.\n", DEVICE_NAME);  
    return -1;  
  }


  switch (argv[1][0])
  {
  case 'h':
    printf ("help: \n\tr - read log;\n\ts - start nci engine;\n\to - stop engine;\n\tc - start communication;\n\tx- release engine\n\n");
    break;
  case 'r':
    nlfp.readNciDataFromFile("/etc/nfc_on_off_filtered.log", fd, rxData); 
    ioctl(fd, CMD_NCI_ENGINE_START, NULL);
    break;
  case 'e':
    nlfp.readNciDataFromFile("/etc/nfc_on_off_filtered.log", fd, rxData); 
    ioctl(fd, CMD_NCI_FIFO_GETALL, NULL);
    break;
  case 's':
    ioctl(fd, CMD_NCI_ENGINE_START, NULL);
    break;
  case 'o':
    ioctl(fd, CMD_NCI_ENGINE_STOP, NULL);
    break;
  case 'c':
    nlfp.readNciDataFromFile("/etc/nfc_on_off_filtered.log", fd, rxData); 
    ioctl(fd, CMD_NCI_ENGINE_START, NULL);
    usleep (20000);
    startCommunication(fd, rxData);
    break;
  case 'x':
    ioctl(fd, CMD_NCI_FIFO_RELEASE, NULL);
    break;
  default:
    break;
  };

  ioctl(fd, CMD_NCI_ENGINE_STOP, NULL);
  
  printf("closing fd 0...\n");
  close(fd);

  return 0;  
}  


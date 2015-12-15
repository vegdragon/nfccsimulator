#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sys/time.h>
#include "NciLogFileProcessor.h"

#define DEVICE_NAME "/dev/pn54x"

using namespace std;

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

      NciLogFileProcessor::printTimestamp();
      printf("write: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
        (*v).data[0],(*v).data[1],(*v).data[2],(*v).data[3],(*v).data[4],
        (*v).data[5],(*v).data[6],(*v).data[7],(*v).data[8],(*v).data[9]);

      write(fd, (*v).data, (*v).len);
      
      read(fd, dataRead, sizeof(dataRead));
      NciLogFileProcessor::printTimestamp();
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
  NciLogFileProcessor nlfp;

  fd = open(DEVICE_NAME, O_RDWR);  
  if(fd == -1) 
  {
    printf("Failed to open device %s.\n", DEVICE_NAME);  
    return -1;  
  }


  nlfp.readNciDataFromFile("/etc/nfc_on_off_filtered.log", fd, sendingData, receivingData);

  ioctl(fd, 3, NULL);
  usleep(50000);
  startCommunication(fd, sendingData, receivingData);
  ioctl(fd, 4, NULL);


  printf("closing fd 0...\n");
  close(fd);  

  return 0;  
}  


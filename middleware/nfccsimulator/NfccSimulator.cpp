#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sys/time.h>
#define DEVICE_NAME "/dev/pn54x"


int NfccSimulator::init(int fd)
{
  int ret = 0;

  if (fd <= 0)
  {
    printf ("%s: invlaide fd(%d)!", __FUNCTION__, fd);
    ret = -1;
    goto exit;
  }
  _fdOfNfcDriver = fd;

exit:
  return ret;
}

void NfccSimulator::bootup()
{
  ioctl();
}

void NfccSimulator::shutdown()
{

}

int readNciDataFromFile(const char * fileName, 
                        int fd, 
                        std::vector<nci_data_t> &sendingData,
                        std::vector<nci_data_t> &receivingData
                        )
{
  
      nciData.direction = *(strFound - 21);
      // printf("strFound=%s, direction=%c\n", strFound, nciData.direction);
      // printf("Send ioctl cmd 1...\n");
      ioctl(fd, 1, &nciData);
  

  return 0;
}
#endif /* __USE_STL__ */





#ifndef __NCILOGFILEPROCESSOR__
#define __NCILOGFILEPROCESSOR__

#include <sys/time.h>
#include <vector>

typedef struct nci_data
{
  long             timestamp;
  long             delay;
  char             direction;    // 'X': HOST->NFCC    'R':NFCC->HOST
  unsigned char    data[300];
  int              len;
} nci_data_t;

class NciLogFileProcessor
{
public:
  int readNciDataFromFile(const char * fileName, 
                        int fd, 
                        std::vector<nci_data_t> &rxData
                        );
  
  static void printTimestamp ();

private:
  void strToHex (const char * str, nci_data_t & nciData);
  long parseTimestamp (char * line);

};

#endif /* __NCILOGFILEPROCESSOR__ */
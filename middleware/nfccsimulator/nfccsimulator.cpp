#include <stdio.h>  
#include <stdlib.h>  
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

#define DEVICE_NAME "/dev/pn54x"  

typedef struct nci_data
{
    long		timestamp;
    unsigned char	data[300];
} nci_data_t;

using namespace std;

void strToHex (const char * str, unsigned char * data)
{
  const char *p1  = str;
  const char *p2  = p1;
  int i = 0;

  printf("str=%s\n", str);

  for(i=0;*p1 != NULL;i++) 
  {
    unsigned char byte;

    byte = (*p1-'0') * 0x10 + (*(p1+1)-'0');
    data[i] = byte;
    printf("data[%d] = 0x%x\n", i, byte);
    
    p1 += 2;
  }
}

int readNciDataFromFile(const char * fileName, int fd)
{
	string line;
	ifstream infile(fileName);
	string str ("> ");
	size_t found;
	while (getline(infile, line))
	{
		found = line.find(str);
		if (found!=string::npos)
		{
			nci_data_t nciData;
			strToHex (line.substr (found+2).c_str(), nciData.data);
			printf("Send ioctl cmd 1...\n");
		        ioctl(fd, 1, &nciData);
			cout << line.substr (found+2).c_str() << endl;
		}
	}

	infile.close();
	return 0;
}

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


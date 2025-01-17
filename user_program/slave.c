#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>

#define PAGE_SIZE 4096
#define BUF_SIZE 512
int main (int argc, char* argv[])
{
	char buf[BUF_SIZE];
	int i, dev_fd, file_fd;// the fd for the device and the fd for the input file
	size_t ret, file_size = 0, data_size = -1;
	char file_name[50];
	char method[20];
	char ip[20];
	struct timeval start;
	struct timeval end;
	double trans_time; //calulate the time between the device is opened and it is closed
	char *kernel_address, *file_address;


	strcpy(file_name, argv[1]);
	strcpy(method, argv[2]);
	strcpy(ip, argv[3]);

	if( (dev_fd = open("/dev/slave_device", O_RDWR)) < 0)//should be O_RDWR for PROT_WRITE when mmap()
	{
		perror("failed to open /dev/slave_device\n");
		return 1;
	}
	gettimeofday(&start ,NULL);
	if( (file_fd = open (file_name, O_RDWR | O_CREAT | O_TRUNC)) < 0)
	{
		perror("failed to open input file\n");
		return 1;
	}

	if(ioctl(dev_fd, 0x12345677, ip) == -1)	//0x12345677 : connect to master in the device
	{
		perror("ioclt create slave socket error\n");
		return 1;
	}

    write(1, "ioctl success\n", 14);

	switch(method[0])
	{
		case 'f'://fcntl : read()/write()
			do
			{
				ret = read(dev_fd, buf, sizeof(buf)); // read from the the device
				write(file_fd, buf, ret); //write to the input file
				file_size += ret;
			}while(ret > 0);
			break;
		case 'm': ;//mmap : memory mapped I/O
			char *data = malloc(50000*BUF_SIZE*sizeof(char));
			char *tmp = data;
			do
			{
				
				ret = read(dev_fd, buf, sizeof(buf));
				//printf("ret: %d\n", ret);
				memcpy(tmp, buf, ret);
				tmp += ret;
				
			}while(ret > 0);
			data_size = strlen(data);
			file_size = data_size;
			ftruncate(file_fd, data_size);
	
			char *addr;
    			addr = mmap(NULL, data_size, PROT_WRITE|PROT_READ, MAP_SHARED, file_fd, 0);
			memcpy(addr, data, data_size);
			*(addr + data_size) = '\0';
			free(data);
			
			ioctl(dev_fd, 0x12345680, addr); // after written all data to mapped region, call ioctl() to get the information of page descriptor ( 0x12345680 means default operations in slave_ioctl() in slave_device.c )
			
			munmap(addr, data_size);
			break;
	}



	if(ioctl(dev_fd, 0x12345679) == -1)// end receiving data, close the connection
	{
		perror("ioclt client exits error\n");
		return 1;
	}
	gettimeofday(&end, NULL);
	trans_time = (end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)*0.0001;
	printf("Transmission time: %lf ms, File size: %d bytes\n", trans_time, file_size );


	close(file_fd);
	close(dev_fd);
	return 0;
}



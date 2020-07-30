// SPDX-License-Identifier: GPL-2.0
//
// * Hidraw Userspace Example
// *
 //* Copyright (c) 2010 Alan Ott <alan@signal11.us>
// * Copyright (c) 2010 Signal 11 Software
// *
// * The code may be used by anyone for any purpose,
// * and can serve as a starting point for developing
// * applications using hidraw.
 

// Linux 
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

//
// * Ugly hack to work around failing compilation on systems that don't
// * yet populate new version of hidraw.h to userspace.
 
#ifndef HIDIOCSFEATURE
#warning Please have your distro update the userspace kernel headers
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif

// Unix 
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

// C 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define LOGITECHID 0x046d
#define POWERSHELLID 0xffffcae2

const char *bus_str(int bus);


typedef struct __attribute__((packed)) 
appleJs_t
{
char align;//?
char up;
char right;
char down;
char left;
char ka;
char kb;
char kx;
char ky;
char kl;
char kr;
char ks;//not used?
}appleJs_t;


typedef struct 
hidRaw_t{
	char buf[256];
	struct hidraw_devinfo info;
	int fd;
	appleJs_t jsData;

}hidRaw_t;


void getRawName(int fd,char* buf)
{
	int res;
    // Get Raw Name
	res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
	if (res < 0)
        printf("ERROR: HIDIOCGRAWNAME");

}

void getRawInfo(int fd, struct hidraw_devinfo* info)
{
int res;
	// Get Raw Info 
	res = ioctl(fd, HIDIOCGRAWINFO, info);
	if (res < 0) {
        printf("ERROR: HIDIOCGRAWINFO");
    } }

hidRaw_t* findLogitechRawDevice()
{

	int found=0;
	hidRaw_t* hid=(hidRaw_t*)malloc(sizeof(hidRaw_t));
	
	char *device =(char*)malloc(256);
	int deviceCount=0;
	//int fd;
	sprintf(device,"/dev/hidraw%d",deviceCount);
	 //Open device

	hid->fd = open(device, O_RDWR);

	//while exists devices
	while(hid->fd>=0 && !found)
	{
	 //get parameters
	 getRawName(hid->fd,hid->buf);
     printf("Nombre: %s\n", hid->buf);

	 getRawInfo(hid->fd,&hid->info);
     printf("\tvendor: 0x%x\n", hid->info.vendor);
     printf("\tproduct: 0x%x\n", hid->info.product);

	 if( hid->info.vendor==LOGITECHID && hid->info.product==POWERSHELLID)
	 {	found=1;
	 	printf("found!\n");
	 }else{
		 //close device
		 close(hid->fd);
		 //next
		 deviceCount++;
		 sprintf(device,"/dev/hidraw%d",deviceCount);
		 //Open device
		 hid->fd = open(device, O_RDWR);
		}

	} 
	//done
	free (device);
	if(!found){
	
	free(hid);
	return NULL;
	}
	return hid;

}


void closeHidRaw(hidRaw_t* hid)
{
	if(hid!=NULL){
		close(hid->fd);
		free(hid);
	}	

}



int main(int argc, char **argv)
{
	hidRaw_t* hid=findLogitechRawDevice();
	if(hid!=NULL)
	{
	while(1){
	read(hid->fd,&hid->jsData,sizeof(appleJs_t));
	printf(
		"ALIGN? %d\n"
		"UP: %d \n"
		"DOWN: %d \n"
		"LEFT: %d \n"
		"RIGHT: %d \n"
		"X: %d \n"
		"Y: %d \n"
		"A: %d \n"
		"B: %d \n"
		"L: %d \n"
		"R: %d \n"
		"II: %d \n",
        hid->jsData.align,
		hid->jsData.up,
		hid->jsData.down,
		hid->jsData.left,
		hid->jsData.right,
		hid->jsData.ky,
		hid->jsData.kx,
		hid->jsData.ka,
		hid->jsData.kb,
		hid->jsData.kl,
		hid->jsData.kr,
		hid->jsData.ks);

	}}
	
	return 0;
}

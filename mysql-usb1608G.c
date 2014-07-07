#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <asm/types.h>

#include "pmd.h"
#include "usb-1608G.h"

//mysql libraries
#include <my_global.h>
#include <mysql.h>

#define MAX_COUNT     (0xffff)

/* Test Program */
int toContinue()
{
  int answer;
  answer = 0; //answer = getchar();
  printf("Continue [yY]? ");
  while((answer = getchar()) == '\0' ||
    answer == '\n');
  return ( answer == 'y' || answer == 'Y');
}

//mysql crap out function
void finish_with_error(MYSQL *con)
{
        fprintf(stderr, "%s\n", mysql_error(con));
        mysql_close(con);
        exit(1);
}

char *getPassword(char *password){
	FILE *pFile;
	
	//password is stored in a file called "password"
	pFile=fopen("password","r");
	fscanf(pFile,"%s",password);
	fclose(pFile);

	return password;
}


int main (int argc, char **argv)
{
	//database password
	char password[16];

        //build MySQL connection
        MYSQL *con = mysql_init(NULL);

        if (con == NULL){
                fprintf(stderr, "%s\n", mysql_error(con));
                exit(1);
        }

        if (mysql_real_connect(con, "hfgx620.tunl.daq", "uva_remote", getPassword(password),"slowcontrols", 0, NULL, 0) == NULL){
                finish_with_error(con);
        }


  usb_dev_handle *udev = NULL;

  float table_AIN[NGAINS_1608G][2];
  ScanList list[NCHAN_1608G];  // scan list used to configure the A/D channels.

  int i;
  int flag;

  __u16 value;

  __u8 mode, gain, channel;

  udev = NULL;
  if ((udev = usb_device_find_USB_MCC(USB1608G_PID))) {
    printf("Acquiring data.  Press 'x' then enter to stop.\n");
  } else {
    printf("Failure, did not find a USB 1608G!\n");
    return 0;
  }
  // some initialization
  usbInit_1608G(udev);
  usbBuildGainTable_USB1608G(udev, table_AIN);

  //set gain
  gain = BP_10V;
  
  //set non-blocking getchar() for infinite while loop
  flag = fcntl(fileno(stdin), F_GETFL);
  fcntl(0, F_SETFL, flag | O_NONBLOCK);

  //set to 1 to turn channel monitoring on
  int channel_activated[8]={0};
 
  //activate channels
  channel_activated[0]=1;
  channel_activated[1]=1;
  channel_activated[2]=1;
  channel_activated[3]=1;
  channel_activated[4]=1;
  channel_activated[5]=1;
  channel_activated[6]=1;
  channel_activated[7]=1;

  /*raw_meas is what the ADC sees in volts; calc_meas will multiple the voltage
 * by necessary constants to get measurement value in Kelvin, liters/second,
 * inches, etc*/
  float raw_meas, calc_meas;

  /*will hold the final query*/
  char buffer[1000];

  do {
	//base query
	char query[] ="INSERT INTO usb1608g (device, raw_reading, measurement_reading) VALUES";

	for(i=0;i<8;++i){
		//skip this channel if not activated
		if(channel_activated[i]==0) continue;

		channel=i;
		mode = (LAST_CHANNEL | DIFFERENTIAL);
		list[0].range = gain;
	        list[0].mode = mode;
		list[0].channel = channel;
		usbAInConfig_USB1608G(udev, list);
	
		value = usbAIn_USB1608G(udev, channel);
		value = rint(value*table_AIN[gain][0] + table_AIN[gain][1]);

		//get values for database
		raw_meas=volts_USB1608G(udev,gain,value);
		calc_meas=raw_meas;

		//printf("Channel %d  Mode = %#x  Gain = %d Sample[%d] = %#x Volts = %lf\n",list[0].channel, list[0].mode, list[0].range, i, value, raw_meas);

		//complete the query with values
		snprintf(buffer, sizeof buffer, "%s('d%d','%1f','%1f')",query,i, raw_meas,calc_meas);

		//add to db
		if (mysql_query(con,buffer)) {
			finish_with_error(con);
		}

	}

	//usleep(500000);	  
	sleep(1);	  

  }while(!isalpha(getchar()));

return 0;
}


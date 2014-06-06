/*

*/

#ifndef PowerUtils_H
#define PowerUtils_H


#include "Arduino.h"

#define SMOOTH_BUFF_SHIFT 4 //must be less than 8
#define SMOOTH_BUFF_SIZE (1 << SMOOTH_BUFF_SHIFT)

class PowerUtils {

  public:

    unsigned char voltPin;
    unsigned char currPin;
    unsigned char bkltPin;

    unsigned char iVCC;
    unsigned char iVin;
    unsigned char iCurr;
    unsigned char iFPS;

    unsigned int buffVCC[SMOOTH_BUFF_SIZE];
    unsigned int buffVin[SMOOTH_BUFF_SIZE];
    unsigned int buffCurr[SMOOTH_BUFF_SIZE];
    unsigned int buffFPS[SMOOTH_BUFF_SIZE];

    unsigned int intRef; //internal 1.1 reference in mV, (usually 1100)
    unsigned int RH; //voltage sensing high end divivider in Ohm, (usually 10000)
    unsigned int RL; //voltage sensing low end divivider in Ohm, (usually 1000)
    unsigned int RSense; //current sensing resistor in mOhm, (usually 100)
    unsigned int RIout; //gain resistor for current sensing in Ohm, (usually 1000)

    PowerUtils(unsigned char vPin = 7, unsigned char cPin = 0, unsigned char lPin = 9); //the constructor
    void begin(unsigned int ir = 1100, unsigned int h = 10000, unsigned int l = 2200, unsigned int s = 200, unsigned int i = 1000);
    unsigned int getVCC(unsigned char filtered = 1); //returns voltage on VCC rail in mV
    unsigned int getVin(unsigned char filtered = 1); //returns voltage on input before converter in mV
    unsigned int getCurrent(unsigned char filtered = 1); //returns current drawn by the system in mA
    unsigned int getFPS(unsigned char filtered = 1);
    unsigned int getFreeRAM(); //returns number of the largest free block of RAM in bytes
    unsigned long getPower(); //returns power drawn by the system in mW
    unsigned long getPower(unsigned int u, unsigned int i); //returns power in mW, takes Voltage in mV and Current in mA

    void setBacklight(unsigned char level);

    void setIntRef(unsigned int r);
    void setRH(unsigned int r);
    void setRL(unsigned int r);
    void setRSense(unsigned int r);
    void setRIout(unsigned int r);

    void printVin(Print &p);
    void printVCC(Print &p);
    void printCurrent(Print &p);
    void print1kdecimal(Print &p, unsigned int val, unsigned char ndd = 3, unsigned char nip = 2); //prints integer divided by 1000 as float, ndd = number of decimal places, nip = number of integer places (will be padded by spaces)

    void update(); //call in the loop

  private:

}; //end of class PowerUtils



#endif

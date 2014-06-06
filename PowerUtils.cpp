/*

*/


#include "PowerUtils.h"

extern uint8_t analog_reference;

#if !defined(INTERNAL1V1)
  #define INTERNAL1V1 INTERNAL
#endif

PowerUtils::PowerUtils(unsigned char vPin, unsigned char cPin, unsigned char lPin) :
  voltPin(vPin),
  currPin(cPin),
  bkltPin(lPin),
  iVCC(0),
  iVin(0),
  iCurr(0),
  iFPS(0),
  intRef(1100),
  RH(10000),
  RL(2200),
  RSense(200),
  RIout(1000)
{
  pinMode(this->bkltPin, OUTPUT);

}

void PowerUtils::begin(unsigned int ir, unsigned int h, unsigned int l, unsigned int s, unsigned int i) {
  this->intRef = ir;
  this->RH = h;
  this->RL = l;
  this->RSense = s;
  this->RIout = i;

  //init the smoothing buffer
	unsigned int vcc = this->getVCC(0);
	unsigned int vin = this->getVin(0);
	unsigned int curr = this->getCurrent();
	unsigned int time = millis();
	for (int i = 0; i < SMOOTH_BUFF_SIZE; i++) {
		this->buffVCC[i] = vcc;
		this->buffVin[i] = vin;
		this->buffCurr[i] = curr;
		this->buffFPS[i] = time;
	}
}

unsigned int PowerUtils::getVCC(unsigned char filtered) {
//reads internal 1V1 reference against VCC
  #if defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0); // For ATtiny84
  #elif defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2); // For ATtiny85
  #elif defined(__AVR_ATmega1284P__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  // For ATmega1284
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);  // For ATmega328
  #endif
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  uint8_t low = ADCL;
  unsigned int val = (ADCH << 8) | low;
	val =  ((uint32_t)1024 * this->intRef) / val;
	if (++this->iVCC >= SMOOTH_BUFF_SIZE) this->iVCC = 0;
	this->buffVCC[this->iVCC] = val;
	if (!filtered) return val;

	//smoothing
	long sum = 0;
	for (int i = 0; i < SMOOTH_BUFF_SIZE; i++) {
		sum += this->buffVCC[i];
	}
	return sum >> SMOOTH_BUFF_SHIFT;
}

unsigned int PowerUtils::getVin(unsigned char filtered) {
	unsigned int val = 0;
	if (analog_reference != INTERNAL1V1) {
    analogReference(INTERNAL1V1);
    val = analogRead(this->voltPin);
    delay(15);
    val = analogRead(this->voltPin); //discard first reading
	};
  val = analogRead(this->voltPin);
  // Vout = 1100 * val / 1024
  // I = Vbatt / (RH + RL)
  // Vbatt = Vout * (L + H) / L
  //Serial.println(val);
  unsigned int volt = (uint32_t(this->intRef) * uint32_t(val) / 2) * (uint32_t(this->RH + this->RL) / 2) / (uint32_t(1024/4) * uint32_t(this->RL)); // note: watch for overflow of 32 bit int
	if (++this->iVin >= SMOOTH_BUFF_SIZE) this->iVin = 0;
	this->buffVin[this->iVin] = volt;
	if (!filtered) return volt;

	//smoothing
	long sum = 0;
	for (int i = 0; i < SMOOTH_BUFF_SIZE; i++) {
		sum += this->buffVin[i];
	}
	return sum >> SMOOTH_BUFF_SHIFT;
}


unsigned int PowerUtils::getCurrent(unsigned char filtered) {
	unsigned int val = 0;
	if (analog_reference != INTERNAL1V1) {
    analogReference(INTERNAL1V1);
    val = analogRead(this->currPin);
    delay(15);
    val = analogRead(this->currPin); //discard first reading
  }
  val = analogRead(this->currPin);
  delay(15);
  val = analogRead(this->currPin);
  //return val;
  //  Vout = 1100 * val / 1024
  //  Rs = Vs / I -> I = Vs/Rs
  //  Vout = Vs * RIout / 100 -> Vs = 100 * Vout / RIout
  //  I = 100 * 1100 * val / (1024 * RIout * Rs)
  //
//  unsigned int amp = (this->intRef * 100 * 1000 * val / (1024 * this->RIout * this->RSense));
  unsigned int amp = uint32_t(this->intRef) * uint32_t(98) * uint32_t(val) / (uint32_t(this->RIout) * uint32_t(this->RSense));
	//if (amp < 5) amp = 0;
	if (++this->iCurr >= SMOOTH_BUFF_SIZE) this->iCurr = 0;
	this->buffCurr[this->iCurr] = amp;
	if (!filtered) return amp;

	//smoothing
	long sum = 0;
	for (int i = 0; i < SMOOTH_BUFF_SIZE; i++) {
		sum += this->buffCurr[i];
	}
	return sum >> SMOOTH_BUFF_SHIFT;
}

unsigned int PowerUtils::getFPS(unsigned char filtered) {
	unsigned int time = millis();
	if (++this->iFPS >= SMOOTH_BUFF_SIZE) this->iFPS = 0;
	this->buffFPS[this->iFPS] = time;
	unsigned int tdx = 0;
	if (this->iFPS == SMOOTH_BUFF_SIZE - 1) {
		tdx = time - this->buffFPS[0];
	} else {
		tdx = time - this->buffFPS[this->iFPS + 1];
	}
	return 1000 * SMOOTH_BUFF_SIZE / tdx;
}

unsigned int PowerUtils::getFreeRAM() {
  extern int  __bss_end;
  extern int  *__brkval;
  int free_memory;
  if((int)__brkval == 0) {
    free_memory = ((int)&free_memory) - ((int)&__bss_end);
  }
  else {
    free_memory = ((int)&free_memory) - ((int)__brkval);
  }
  return free_memory;
}

void PowerUtils::setIntRef(unsigned int r) {
  this->intRef = r;
}

void PowerUtils::setRSense(unsigned int r) {
  this->RSense = r;
}

void PowerUtils::setRIout(unsigned int r) {
  this->RIout = r;
}

void PowerUtils::setRH(unsigned int r) {
  this->RH = r;
}

void PowerUtils::setRL(unsigned int r) {
  this->RL = r;
}

void PowerUtils::setBacklight(unsigned char level) {
  analogWrite(this->bkltPin, level);
}

void PowerUtils::printVin(Print &p) {
  unsigned int volt = this->getVin();
  if (volt < 1000) p.print(" ");
  if (volt < 100) p.print(" ");
  if (volt < 10) p.print(" ");
  p.print(volt);
  p.print("mV");
}

void PowerUtils::printVCC(Print &p) {
  unsigned int volt = this->getVCC();
  if (volt < 1000) p.print(" ");
  if (volt < 100) p.print(" ");
  if (volt < 10) p.print(" ");
  p.print(volt);
  p.print("mV");
}

void PowerUtils::printCurrent(Print &p) {
  unsigned int curr = this->getCurrent();
  if (curr < 1000) p.print(" ");
  if (curr < 100) p.print(" ");
  if (curr < 10) p.print(" ");
  p.print(curr);
  p.print("mA");
}

void PowerUtils::print1kdecimal(Print &p, unsigned int val, unsigned char ndd, unsigned char nip) {
  //print integral part first
  if (nip > 1 && val < 10000) p.print(' ');
  unsigned int cc = val/1000;
  p.print(cc);
  if (ndd > 0) {
    p.print('.');
    cc = (val - 1000 * cc) / 100;
    p.print(cc);
  }
  if (ndd > 1) {
    cc = (val - 100 * (val/100)) / 10;
    p.print(cc);
  }
  if (ndd > 2) {
    cc = val - 10 * (val/10);
    p.print(cc);
  }
}

void PowerUtils::update() {

}

unsigned long PowerUtils::getPower() {
  return this->getPower(this->getVCC(), this->getCurrent());
}

unsigned long PowerUtils::getPower(unsigned int u, unsigned int i) {
  return uint32_t(u) * uint32_t(i) / 1000;
}



// libary includes
#include <xCore.h>
#include <xSN01.h>
#include <xSW01.h>
#include <xPB04.h>

// xchip instances
xPB04 PB04;
xSN01 SN01;
xSW01 SW01;

// TTN KEYS
char *DEV_ADDR = "000081064825742E";
char *APPSKEY = "70B3D57ED001A218";
char *NWKSKEY = "3B9093E93C546818065011CDD0478F19";

char debug = '1';

#define CR02_I2C_ADDRESS 0x08 // slave address of CR02

#define BUFLEN 18 // buffer size to send over i2c to CC03
char data[BUFLEN];

int32_t latitude;
int32_t longitude;
int16_t temperature;
uint16_t pressure;
uint16_t humidity;
uint16_t voltage;
uint16_t current;
float hdop;
void setup() {
  delay(4000);
  Wire.begin(); // begin i2c
  SW01.begin(); // configure and start SW01
  SN01.begin(); // configure and start SN02
  PB04.begin(); // configure and start PB04
  delay(2000);

  // send ttn keys over to CR02
  send_keys(APPSKEY, DEV_ADDR, NWKSKEY, debug);
  delay(50);
}

void loop() {
  // poll sensors
  PB04.poll();
  SW01.poll();
  SN01.poll();

  // read sensor values
  temperature = SW01.getTempC() * 100;
  humidity = SW01.getHumidity() * 100;
  pressure = SW01.getPressure() / 100;
  voltage = PB04.getVoltage() * 100;
  current = PB04.getCurrent();
  hdop = SN01.getHDOP();

  data[0] = highByte(temperature);
  data[1] = lowByte(temperature);
  data[2] = highByte(humidity);
  data[3] = lowByte(humidity);
  data[4] = highByte(pressure);
  data[5] = lowByte(pressure);

  if (hdop > 6.0) { // check hdop to ensure relaibale data, else provide default coordinates
    latitude = SN01.getLatitude() * 10000000;
    longitude = SN01.getLongitude() * 10000000;
  } else {
    latitude = -340851700;
    longitude = 188219427;
  }
  data[6] = (latitude >> 24) & 0xFF;
  data[7] = (latitude >> 16) & 0xFF;
  data[8] = (latitude >> 8) & 0xFF;
  data[9] = latitude & 0xFF;
  data[10] = (longitude >> 24) & 0xFF;
  data[11] = (longitude >> 16) & 0xFF;
  data[12] = (longitude >> 8) & 0xFF;
  data[13] = longitude & 0xFF;
  data[14] = highByte(voltage);
  data[15] = lowByte(voltage);
  data[16] = highByte(current);
  data[17] = lowByte(current);

  // send the data over i2c
  send_data(data);
  delay(4000);
}

void send_keys(char *APPSKEY, char *DEV_ADDR, char *NWKSKEY, char debug)
{
  char* keys[3] = {APPSKEY, DEV_ADDR, NWKSKEY};
  Wire.beginTransmission(CR02_I2C_ADDRESS);
  Wire.write(debug);
  Wire.endTransmission();
  delay(100);
  for (int i = 0; i < 3; i++) {
    Wire.beginTransmission(CR02_I2C_ADDRESS);
    Wire.write('a' + i);
    Wire.endTransmission();
    delay(100);
    Wire.beginTransmission(CR02_I2C_ADDRESS);
    Wire.write(keys[i]);
    Wire.endTransmission();
    delay(100);
  }
}

void send_data(char*buf)
{
  Wire.beginTransmission(CR02_I2C_ADDRESS);
  Wire.write('0');
  Wire.write(buf, BUFLEN);
  Wire.endTransmission();
}

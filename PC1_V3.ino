#include <SPI.h>
#include <RH_RF69.h>

#define RFM69_INT     3  // 
#define RFM69_CS      4  //
#define RFM69_RST     2  // "A"
#define LED           13
#define RF69_FREQ     915.0
RH_RF69 rf69(RFM69_CS, RFM69_INT);

byte databit;
int i = 0;
uint8_t radiopacket[55];

void setup() {
  Serial.begin(9600);
  
  pinMode(LED, OUTPUT);     
  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, LOW);
 
  Serial.println("RFM69 Test!");
  Serial.println();
 
  // manual reset
  digitalWrite(RFM69_RST, HIGH);
  delay(10);
  digitalWrite(RFM69_RST, LOW);
  delay(10);

  if (!rf69.init()) {
    Serial.println("RFM69 radio init failed");
    while (1);
  }
  Serial.println("RFM69 radio init OK!");

  if (!rf69.setFrequency(RF69_FREQ)) {
    Serial.println("setFrequency failed");
  }

  rf69.setTxPower(20, true); 

  uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  rf69.setEncryptionKey(key);
}

void loop() {

  delay(1000);

  int counter = 0;
  if (Serial.available() > 0)
  {
    while (i < 4){
    databit = Serial.read();
    radiopacket[counter] = databit;
    counter++;
    i++;
    }
  } 
  Serial.print("Sending ");

  rf69.send((uint8_t *)radiopacket, strlen(radiopacket));
  rf69.waitPacketSent();

  Serial.print("Sent ");

  uint8_t buf[RH_RF69_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);
  
  if (rf69.recv(buf, &len)) {
    Serial.print("Got a reply: ");
    Serial.println((char*)buf);
  } else {
    Serial.println("No reply, is another RFM69 listening?");
  }
}

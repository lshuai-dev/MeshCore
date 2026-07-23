#include "chsc6x.h"
#include "Arduino.h"

#define TOUCH_SDA_PIN      (17)
#define TOUCH_SCL_PIN      (18)
#define TOUCH_INT_PIN      (-1)
#define TOUCH_RST_PIN      (21)

uint16_t  touchX,touchY;
chsc6x touch(&Wire1,TOUCH_SDA_PIN,TOUCH_SCL_PIN,TOUCH_INT_PIN,TOUCH_RST_PIN);
void setup() {
    Serial.begin(115200);
    pinMode(40,OUTPUT);
    digitalWrite(40,LOW);
    delay(100);
    touch.chsc6x_init();
}

void loop() {
    if(touch.chsc6x_read_touch_info(&touchX,&touchY)==0)
    {
        Serial.printf("Touch X: %d, Y: %d\r\n", touchX, touchY);
    }
    delay(100);
}

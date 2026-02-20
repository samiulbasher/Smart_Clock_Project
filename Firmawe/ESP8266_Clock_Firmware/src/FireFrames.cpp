#include <Arduino.h>
#include "FireFrames.h"


// // ISR for display refresh
// void display_updater() {
//     display.display(70);
//   }
  
//   void setup() {
//     display.begin(16);
//     display.clearDisplay();
    
//     display_ticker.attach(0.002, display_updater);
  
  
//     display.setBrightness(20);
//     yield(); 
//   }
  
//   void drawFrame(uint16_t *frame) {
//     display.clearDisplay();
//     int imageHeight = 32;
//     int imageWidth = 64;  
//     int counter = 0;
//     for (int yy = 0; yy < imageHeight; yy++) {
//       for (int xx = 0; xx < imageWidth; xx++) {
//         display.drawPixel(xx, yy, frame[counter]);
//         counter++;
//       }
//     }
//     delay(100);
//   }
  
//   void loop() {
//     drawFrame(fire_frame_1);
//     drawFrame(fire_frame_2);
//     drawFrame(fire_frame_3);
//     drawFrame(fire_frame_4);
//     drawFrame(fire_frame_5);
//     drawFrame(fire_frame_6);
//     drawFrame(fire_frame_7);
//     drawFrame(fire_frame_8);
//     drawFrame(fire_frame_9);
//     //drawFrame(fire_frame_10);
//     //drawFrame(fire_frame_11);
//     yield();
    
//     //delay(2000);
// }
  



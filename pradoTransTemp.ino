#include <BluetoothSerial.h>
#include "ELMduino.h"
#include <Wire.h>
#include <U8g2lib.h>
#include <cstring>

BluetoothSerial SerialBT;



// Define TCA9548A I2C multiplexer address
#define TCA9548A_ADDR 0x70

// Define I2C pins for ESP32
#define I2C_SCL 22
#define I2C_SDA 21

#define BUTTON_PIN 32
// Create U8g2 display instances

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2DisplayNormal(U8G2_R0, U8X8_PIN_NONE);
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2DisplayMirrored(U8G2_MIRROR, U8X8_PIN_NONE);

#define ELM_PORT SerialBT
#define DEBUG_PORT Serial
ELM327 myELM327;

bool isMirrored = false;

int transPanOilTemp = 0;
int torqueConverterTemp = 0;
int curGear = 1;
int TCLockup = 0;
int coolant = 0;

void setup()
{
   pinMode(BUTTON_PIN, INPUT);
  DEBUG_PORT.begin(115200);
  SerialBT.setPin("1234", 4);
  ELM_PORT.begin("Esp32OBD", true);
  uint8_t remoteAddress[6] = { 0x33, 0x33, 0x1A, 0x04, 0x00, 0x00 };

  Wire.begin(I2C_SDA, I2C_SCL);
  // Initialize displays
  
  for (int i = 1; i < 7; i++) {
    selectTCAChannel(i);
    u8g2DisplayNormal.begin();
    u8g2DisplayMirrored.begin();

    // Construct the message string dynamically
    String message = String(i) + ": Connecting to ELM327...";
    
    displayMessage(u8g2Display1(), message.c_str());
  }

  if (!ELM_PORT.connect(remoteAddress))
  {
    if (!ELM_PORT.connect("OBDII"))
    {
      DEBUG_PORT.println("Couldn't connect to OBD scanner - Phase 1");
      while (1)
          ;
    }
  }

  if (!myELM327.begin(ELM_PORT))
  {
      Serial.println("Couldn't connect to OBD scanner - Phase 2");
      while (1)
          ;
  }

  for (int i = 1; i < 7; i++) {
    selectTCAChannel(i);
    displayMessage(u8g2Display1(), "Connected!");
  }

  delay(1000);
}

void loop()
{
  int buttonState = digitalRead(BUTTON_PIN);
  
  // Check if the button is pressed or not, Select Heads up display mode accordingly
  if (buttonState == HIGH) 
  {
    isMirrored = !isMirrored;
  }

  loadTransTemps();
  loadCurGearTCLockup();
  loadEngineTempCoolant();

  if(transPanOilTemp > -1)
  {
    selectTCAChannel(2);
    drawTempGauge(u8g2Display1(), transPanOilTemp, "TRANS:    PAN");

    selectTCAChannel(3);
    drawTempGauge(u8g2Display1(), torqueConverterTemp, "TRANS:    TC");
  }
  
  selectTCAChannel(1);
  displayVoltage();

  selectTCAChannel(4);
  drawTempGauge(u8g2Display1(), coolant, "COOLANT");

  selectTCAChannel(5);
  displayGear(curGear);

  selectTCAChannel(6);
  displayTCL(TCLockup);
  // Wait 0.1 seconds before repeating the loop
  delay(20);

}


U8G2_SH1106_128X64_NONAME_F_HW_I2C& u8g2Display1() {
    if (isMirrored) {
        u8g2DisplayMirrored.setFlipMode(1); // Enable mirrored mode
        return u8g2DisplayMirrored;
    } else {
        u8g2DisplayNormal.setFlipMode(0);
        return u8g2DisplayNormal;
    }
}


void selectTCAChannel(uint8_t channel) {
  if (channel > 7) return; // TCA9548A has 8 channels (0-7)
  Wire.beginTransmission(TCA9548A_ADDR);
  Wire.write(1 << channel); // Select channel
  Wire.endTransmission();
}

// Function to display a message on a given display
void displayMessage(U8G2_SH1106_128X64_NONAME_F_HW_I2C &display, const char* message) {
  const int maxWidth = 20; // Maximum characters per line
  int lineHeight = 12;     // Line height in pixels (adjust based on font size)
  int y = 12;              // Starting y-coordinate for the first line
  char buffer[21];         // Buffer to hold a single line (20 chars + null terminator)
  int len = strlen(message);

  display.clearBuffer();            // Clear display buffer
  display.setFont(u8g2_font_ncenB08_tr); // Set font

  for (int i = 0; i < len;) 
  {
    int charsToCopy = 0;

    // Find the maximum length for the current line
    for (int j = i; j < len && charsToCopy < maxWidth; j++) {
        if (message[j] == ' ' || j == len - 1) { // Split at spaces or end of string
            if (j - i < maxWidth) {
                charsToCopy = j - i + 1; // Include space
            } else {
                break;
            }
        }
    }

    // If no space was found within maxWidth, break mid-word (unlikely but safe)
    if (charsToCopy == 0) {
        charsToCopy = maxWidth;
    }

    // Copy characters to the buffer
    strncpy(buffer, message + i, charsToCopy);
    buffer[charsToCopy] = '\0'; // Null-terminate the buffer

    // Draw the line on the display
    display.drawStr(0, y, buffer);

    // Move to the next line
    y += lineHeight;

    // Skip the characters we just displayed
    i += charsToCopy;

    // Skip any extra spaces to prevent blank lines
    while (message[i] == ' ') {
        i++;
    }

    // Stop if we exceed the display height
    if (y > 64) {
        break;
    }
  }

  display.sendBuffer(); // Send buffer to display
}








// Function to draw a moving half-circle gauge
void drawTempGauge(U8G2_SH1106_128X64_NONAME_F_HW_I2C &display, int value, const char* name) {
  display.clearBuffer(); // Clear display buffer

  // Define the center and radius of the half-circle gauge
  int centerX = 84; // Center X-coordinate of the gauge
  int centerY = 50; // Center Y-coordinate of the gauge (lower for half-circle)
  int radius = 40;  // Radius of the gauge

  // Draw the half-circle outline
  for (int angle = 0; angle <= 180; angle += 1) { // Draw from -90° to +90°
    int x = centerX + cos(radians(angle)) * radius;
    int y = centerY - sin(radians(angle)) * radius;

    if (angle <= 75) {
        // Make the line thicker for angles >= 75°
        for (int thickness = -4; thickness <= 1; thickness++) {
            int xThick = centerX + cos(radians(angle)) * (radius + thickness);
            int yThick = centerY - sin(radians(angle)) * (radius + thickness);
            display.drawPixel(xThick, yThick);
        }
    } else {
        display.drawPixel(x, y); // Regular line for other angles
    }
  }

  // Draw ticks around the half-circle
  for (int angle = 0; angle <= 180; angle += 15) { // Draw tick marks every 15°
    
    int x1 = centerX + cos(radians(angle)) * (radius - 4);
    int y1 = centerY - sin(radians(angle)) * (radius - 4);
    int x2 = centerX + cos(radians(angle)) * radius;
    int y2 = centerY - sin(radians(angle)) * radius;
    display.drawLine(x1, y1, x2, y2);
  }

  // Calculate needle position based on value
  int needleAngle = map(value, 0, 200, 180, 0); // Map value (0-100) to angle (-90° to 90°)
  int needleX = centerX + cos(radians(needleAngle)) * (radius - 8);
  int needleY = centerY - sin(radians(needleAngle)) * (radius - 8);

  // Draw the needle
  display.drawLine(centerX, centerY, needleX, needleY);


  

  

  // Display percentage value below the gauge
  char valueStr[10];
  snprintf(valueStr, sizeof(valueStr), "%d", value);
  display.setFont(u8g2_font_ncenB12_tr);
  display.drawStr(centerX - 80, centerY - 30, valueStr);

  display.setFont(u8g2_font_ncenB12_tr); // Same large font for "C"
  display.drawStr(centerX - 45, centerY - 30, "C"); // Directly use the string "C"

  // Display the "o"
  display.setFont(u8g2_font_ncenB08_tr); // Use a smaller font for "o"
  display.drawStr(centerX - 50, centerY - 40, "o"); // Directly use the string "o"

  // Display the label name
  char charStr[20];
  snprintf(charStr, sizeof(charStr), name);
  display.setFont(u8g2_font_ncenB08_tr); // Smaller font for the label
  display.drawStr(centerX - 80, centerY + 10, charStr); // Adjust position for label

  display.sendBuffer(); // Send buffer to display
}



void displayVoltage()
{
  const char* command = "AT RV";

  myELM327.sendCommand(command);

  String volts = "";

  unsigned long timeout = millis() + 1000; // Set a timeout for 2 seconds
  while (millis() < timeout) 
  {
    if (ELM_PORT.available()) 
    {
      char c = ELM_PORT.read();
      volts += c;

      if (c == '>') // End of response
      { 
        DEBUG_PORT.println();
        break;
      }
    }
  }
  const char* voltsChar = volts.c_str();
  if (strcmp(voltsChar, "?") != 0) {
    // This block will execute only if voltsChar does not equal "?"
    u8g2Display1().clearBuffer();
    u8g2Display1().setFont(u8g2_font_ncenB18_tr);
    u8g2Display1().drawStr(40, 40, voltsChar);
    u8g2Display1().sendBuffer();
  }
}


void displayGear(int gear)
{
  char valueStr[10];
  // Format the float with one decimal place (e.g., "12.3")
  u8g2Display1().clearBuffer();
  snprintf(valueStr, sizeof(valueStr), "%d", gear);
  u8g2Display1().setFont(u8g2_font_ncenB18_tr);
  u8g2Display1().drawStr(55, 30, valueStr);
  u8g2Display1().drawStr(30, 60, "Gear"); // Directly use the string "C"
  u8g2Display1().sendBuffer();
}

void displayTCL(int tclvalue)
{
  char valueStr[10];
  u8g2Display1().clearBuffer();
  u8g2Display1().setFont(u8g2_font_ncenB12_tr);
  u8g2Display1().drawStr(20, 30, "TC Lockup"); // Directly use the string "C"
  if(tclvalue)
  {
    u8g2Display1().drawStr(47, 60, "ON"); // Directly use the string "C"
  }
  else
  {
    u8g2Display1().drawStr(40, 60, "OFF"); // Directly use the string "C"
  }
  
  u8g2Display1().sendBuffer();
}



void loadTransTemps()
{
  const char* command = "2182";

  myELM327.sendCommand(command);

  String temp = "";

  unsigned long timeout = millis() + 2000; // Set a timeout for 2 seconds
  while (millis() < timeout) 
  {
    if (ELM_PORT.available()) 
    {
      char c = ELM_PORT.read();
      temp += c;

      if (c == '>') // End of response
      { 
        DEBUG_PORT.println();
        break;
      }
    }
  }
  String filtered = temp.substring(4); // Get substring starting from the 3rd character
  const char* tempChar = filtered.c_str();

  int A = strtol(tempChar, nullptr, 16) >> 24; // Extract first byte (A)
  int B = (strtol(tempChar, nullptr, 16) >> 16) & 0xFF; // Extract second byte (B)
  int C = (strtol(tempChar, nullptr, 16) >> 8) & 0xFF;  // Extract third byte (C)
  int D = strtol(tempChar, nullptr, 16) & 0xFF;
  
  float pantemperature = ((((A * 256) + B) * 0.35) - 3600) / 90.0;
  float tctemperature = ((((C * 256) + D) * 0.35) - 3600) / 90.0;

  transPanOilTemp = (int)pantemperature;
  torqueConverterTemp = (int)tctemperature;
}

void loadCurGearTCLockup()
{
  const char* command = "2185";

  myELM327.sendCommand(command);

  String temp = "";

  unsigned long timeout = millis() + 2000; // Set a timeout for 2 seconds
  while (millis() < timeout) 
  {
    if (ELM_PORT.available()) 
    {
      char c = ELM_PORT.read();
      temp += c;

      if (c == '>') // End of response
      { 
        DEBUG_PORT.println();
        break;
      }
    }
  }

  String filtered = temp.substring(4); // Get substring starting from the 3rd character
  const char* tempChar = filtered.c_str();

  long hexValue = strtol(tempChar, nullptr, 16);

  // Extract bytes
  int A = (hexValue >> 16) & 0xFF; // First byte (A)
  int B = (hexValue >> 8) & 0xFF;  // Second byte (B)
  int C = hexValue & 0xFF;         // Third byte (C)
  if(A > 0)
  {
    curGear = A;
    TCLockup = B;
  }
  
}

void loadEngineTempCoolant() 
{
  const char* command = "0105";
  
  // Send the OBD-II command
  myELM327.sendCommand(command);

  String response = ""; // To store the response
  unsigned long timeout = millis() + 2000; // 2-second timeout

  while (millis() < timeout) 
  {
    if (ELM_PORT.available()) 
    {
      char c = ELM_PORT.read();
      response += c;

      if (c == '>') // End of response
      {
        break;
      }
    }
  }

  // Debug output to verify the received response
  DEBUG_PORT.println("Raw Response: " + response);

  // Clean the response to extract the data byte
  String filtered = response.substring(4); // Get substring starting from the 3rd character
  const char* tempChar = filtered.c_str();
  int tempValue = strtol(tempChar, nullptr, 16);
  if (tempValue >= 40) 
  {
    // Convert hex string to an integer
    

    // Calculate the coolant temperature
    int coolantTemp = tempValue - 40;

    DEBUG_PORT.print("Coolant Temperature: ");
    DEBUG_PORT.println(coolantTemp);

    // Store the result in your variable
    coolant = coolantTemp; // Ensure 'coolant' is defined globally or passed by reference
  } 
  else 
  {
    DEBUG_PORT.println("Failed to parse response.");
  }
}


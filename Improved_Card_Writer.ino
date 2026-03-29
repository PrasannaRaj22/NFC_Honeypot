/*
 * IMPROVED DEMO CARD WRITER
 * Better error handling and authentication
 * 
 * WIRING:
 * MFRC522 → ESP32-S3
 * SS/SDA → GPIO 10, SCK → GPIO 12, MOSI → GPIO 11
 * MISO → GPIO 13, RST → GPIO 9, VCC → 3.3V, GND → GND
 */

#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 10
#define RST_PIN 9

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n================================================================");
  Serial.println("  IMPROVED DEMO CARD WRITER");
  Serial.println("================================================================\n");
  
  SPI.begin(12, 13, 11, SS_PIN);
  mfrc522.PCD_Init();
  delay(100);
  
  Serial.println("✅ MFRC522 Online\n");
  
  // Set default key
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
  
  Serial.println("Place your card on the reader...\n");
}

void loop() {
  if (!mfrc522.PICC_IsNewCardPresent()) return;
  if (!mfrc522.PICC_ReadCardSerial()) return;
  
  Serial.println("\n================================================================");
  Serial.println("CARD DETECTED");
  Serial.println("================================================================\n");
  
  // Check card type
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.print("Card Type: ");
  Serial.println(mfrc522.PICC_GetTypeName(piccType));
  
  if (piccType != MFRC522::PICC_TYPE_MIFARE_1K) {
    Serial.println("\n⚠️  WARNING: Not MIFARE Classic 1K");
    Serial.println("This writer is for MIFARE Classic 1K cards only.");
    Serial.println("\nIf you have:");
    Serial.println("  - NFC tags → Use the NFC tag writer");
    Serial.println("  - Metro card → Card is read-only, cannot write");
    Serial.println("  - Other card → May not be writable\n");
    
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(3000);
    return;
  }
  
  Serial.println("\nTesting card write capability...\n");
  
  // Test write on sector 1 first
  Serial.println("Testing Sector 1 (safe test area)...");
  
  bool canWrite = testWriteCapability();
  
  if (!canWrite) {
    Serial.println("\n================================================================");
    Serial.println("❌ CARD IS READ-ONLY OR PROTECTED");
    Serial.println("================================================================");
    Serial.println("\nThis card cannot be written to because:");
    Serial.println("  1. It's a secured card (metro card, access card)");
    Serial.println("  2. Write protection is enabled");
    Serial.println("  3. It's not a blank writable card");
    Serial.println("\n💡 SOLUTION:");
    Serial.println("  - Buy blank MIFARE Classic 1K cards (₹50-100)");
    Serial.println("  - Use NFC tags instead (if you have them)");
    Serial.println("  - Use this card AS-IS for demo (read-only)");
    Serial.println("================================================================\n");
    
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
    delay(3000);
    return;
  }
  
  Serial.println("✅ Card is writable! Proceeding...\n");
  
  // Write demo data
  writeDemoData();
  
  Serial.println("\nRemove card and place another...\n");
  
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  delay(3000);
}

bool testWriteCapability() {
  byte testData[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                       0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
  
  // Try to write to block 4 (first block of sector 1)
  byte trailerBlock = 7; // Sector 1 trailer
  
  // Authenticate
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A,
    trailerBlock,
    &key,
    &(mfrc522.uid)
  );
  
  if (status != MFRC522::STATUS_OK) {
    Serial.println("  ✗ Authentication failed");
    return false;
  }
  
  Serial.println("  ✓ Authentication OK");
  
  // Try write
  status = mfrc522.MIFARE_Write(4, testData, 16);
  
  if (status != MFRC522::STATUS_OK) {
    Serial.print("  ✗ Write failed: ");
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }
  
  Serial.println("  ✓ Write test OK");
  
  // Verify
  byte buffer[18];
  byte size = sizeof(buffer);
  status = mfrc522.MIFARE_Read(4, buffer, &size);
  
  if (status != MFRC522::STATUS_OK) {
    Serial.println("  ✗ Read verification failed");
    return false;
  }
  
  // Check if data matches
  for (byte i = 0; i < 16; i++) {
    if (buffer[i] != testData[i]) {
      Serial.println("  ✗ Data verification failed");
      return false;
    }
  }
  
  Serial.println("  ✓ Verification OK");
  return true;
}

void writeDemoData() {
  Serial.println("Writing demo data to card...\n");
  
  int successCount = 0;
  int failCount = 0;
  
  // Sector 1: Personal Info
  Serial.println("Sector 1: Personal Information");
  
  byte name[16] = {'R', 'A', 'J', 'E', 'N', 'D', 'R', 'A', 
                   'N', ' ', 'N', 0x00, 0x00, 0x00, 0x00, 0x00};
  if (writeBlockSafe(4, name, 7)) {
    Serial.println("  ✓ Name: RAJENDRAN N");
    successCount++;
  } else {
    Serial.println("  ✗ Name: FAILED");
    failCount++;
  }
  
  byte phone[16] = {'+', '9', '1', '-', '9', '8', '7', '6',
                    '5', '4', '3', '2', '1', '0', 0x00, 0x00};
  if (writeBlockSafe(5, phone, 7)) {
    Serial.println("  ✓ Phone: +91-9876543210");
    successCount++;
  } else {
    Serial.println("  ✗ Phone: FAILED");
    failCount++;
  }
  
  Serial.println("\n================================================================");
  Serial.println("WRITE SUMMARY:");
  Serial.println("================================================================");
  Serial.println("Successful: " + String(successCount));
  Serial.println("Failed: " + String(failCount));
  
  if (successCount > 0) {
    Serial.println("\n✅ CARD HAS DEMO DATA!");
    Serial.println("You can use this for demonstration.");
    Serial.println("The honeypot will show the data being stolen.");
  } else {
    Serial.println("\n❌ ALL WRITES FAILED");
    Serial.println("This card is fully protected.");
  }
  Serial.println("================================================================\n");
}

bool writeBlockSafe(byte blockAddr, byte* data, byte trailerBlock) {
  // Authenticate
  MFRC522::StatusCode status = mfrc522.PCD_Authenticate(
    MFRC522::PICC_CMD_MF_AUTH_KEY_A,
    trailerBlock,
    &key,
    &(mfrc522.uid)
  );
  
  if (status != MFRC522::STATUS_OK) {
    return false;
  }
  
  // Write
  status = mfrc522.MIFARE_Write(blockAddr, data, 16);
  
  if (status != MFRC522::STATUS_OK) {
    return false;
  }
  
  delay(50); // Give card time to write
  
  // Verify
  byte buffer[18];
  byte size = sizeof(buffer);
  status = mfrc522.MIFARE_Read(blockAddr, buffer, &size);
  
  if (status != MFRC522::STATUS_OK) {
    return false;
  }
  
  // Check data
  for (byte i = 0; i < 16; i++) {
    if (buffer[i] != data[i]) {
      return false;
    }
  }
  
  return true;
}

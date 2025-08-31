#include <Keyboard.h>
#include <Base64.h>
#include <AESLib.h>



// 定义UART引脚
#define fpSerial Serial1
#define DebugSerial Serial

//调试模式打开
//#define DEBUG_MODE

// 定义指令包格式
const uint8_t HEADER_HIGH = 0xEF;
const uint8_t HEADER_LOW = 0x01;

// #define DEFAULT_MODULE_ADDR
#define NEW_MODULE_ADDR

//Define module that has factory default addr
#ifdef DEFAULT_MODULE_ADDR
//command 8 will change  module addr from default addr to new addr
const uint32_t DEVICE_ADDRESS = 0xFFFFFFFF;
const uint32_t NEW_DEVICE_ADDRESS = 0x0106E9D3;
#endif

//Define module that has addr changed,set your own addr!
#ifdef NEW_MODULE_ADDR
//command 8 will change revert module addr to default addr
const uint32_t DEVICE_ADDRESS = 0x0106E9D3;
const uint32_t NEW_DEVICE_ADDRESS = 0xFFFFFFFF;
#endif


// 定义指令码
const uint8_t CMD_GET_IMAGE = 0x01;       // 获取图像
const uint8_t CMD_GEN_CHAR = 0x02;        // 生成特征
const uint8_t CMD_MATCH = 0x03;           // 精确比对指纹
const uint8_t CMD_SEARCH = 0x04;          // 搜索指纹
const uint8_t CMD_REG_MODEL = 0x05;       // 合并特征
const uint8_t CMD_STORE_CHAR = 0x06;      // 存储模板
const uint8_t CMD_CLEAR_LIB = 0x0D;       // 清空指纹库
const uint8_t CMD_READ_SYSPARA = 0x0F;    // 读模组基本参数
const uint8_t CMD_READ_CHIPSN = 0x34;     // 读模组序列号
const uint8_t CMD_SET_CHIPPWD = 0x12;     // 设置模组密码
const uint8_t CMD_VERIFY_CHIPPWD = 0x13;  // 验证模组密码
const uint8_t CMD_SET_CHIPADDR = 0x15;    // 设置模组地址
const uint8_t CMD_FACTORY_RESET = 0x3b;   // 恢复出厂设置


// 定义缓冲区ID
uint8_t BUFFER_ID = 0;

// 定义模板存储起始位置
uint16_t PAGE_ID = 0;

// 全局变量
bool isInit = false;

uint8_t module_sn[16];
uint8_t module_pwd[4] = { 'T', '5', '2', 'C' };
uint8_t new_module_addr[4] = { (NEW_DEVICE_ADDRESS >> 24) & 0xFF, (NEW_DEVICE_ADDRESS >> 16) & 0xFF, (NEW_DEVICE_ADDRESS >> 8) & 0xFF, NEW_DEVICE_ADDRESS & 0xFF };
AESLib aesLib;
char encrypted_base64[] = "AAECAwQFBgcICQoLDA0OD4NCHqJqj7j9TmrCwI//X7XRGtsRwZipl4mVmiuGx0Gj";


// 函数声明
void command_use(void);
int read_FP_info(void);
void sendCommand(uint8_t cmd, uint8_t param1 = 0, uint16_t param2 = 0);
void sendCommand1(uint8_t cmd, uint8_t param1, uint16_t param2, uint16_t param3);
bool verifyResponse();
void printResponse(uint8_t* response, uint8_t length);

/*****My declaration start*****/
bool get_module_sn(uint8_t module_sn[16]);
bool set_module_password(uint8_t module_pwd[4]);
bool verify_module_password(uint8_t module_pwd[4]);
bool set_module_address(uint8_t new_module_addr[4]);
bool module_factory_reset(void);
void decrypt_password_base64(const char* encrypted_base64, char* output_buffer, int output_buffer_len, const uint8_t* aes_key, int key_len);
/*****My declaration end*****/

void printHex(uint8_t* data, uint8_t len) {
  for (uint8_t i = 0; i < len; i++) {
    if (data[i] < 0x10) DebugSerial.print('0');
    DebugSerial.print(data[i], HEX);
    DebugSerial.print(' ');
  }
  DebugSerial.println();
}

void setup() {
  // Init the serial port for fingerprint module
  fpSerial.begin(57600);
  // Init the usb virtual keyboard
  Keyboard.begin();
#if defined(DEBUG_MODE)
  DebugSerial.begin(57600);
  DebugSerial.println("Passkey Inited...");
#endif
}

void loop() {
  decrypt_password_and_keyin();
  delay(500);
}

void command_use(void) {
  DebugSerial.println("------------------------------------------");
  DebugSerial.println("command input:");
  DebugSerial.println("\"1\" means \"register fingerprint.\"");
  DebugSerial.println("\"2\" means \"search fingerprint.\"");
  DebugSerial.println("\"3\" means \"clear fingerprint.\"");
  DebugSerial.println("\"4\" means \"display module serial number.\"");
  DebugSerial.println("\"5\" means \"search fingerprint and decrypt test password\"");
  DebugSerial.println("\"6\" means \"set module password(hardcoded)\"");
  DebugSerial.println("\"7\" means \"verify module password(hardcoded)\"");
  DebugSerial.println("\"8\" means \"set module address(hardcoded)\"");
  DebugSerial.println("\"9\" means \"factory reset the module\"");
  DebugSerial.println("------------------------------------------");
}

int run_command(void) {
  // 当串口有数据可读时
  if (DebugSerial.available() > 0) {
    // 读取直到换行符（自动处理回车+换行）
    // defautl timeout after 1s
    String command = DebugSerial.readStringUntil('\n');
    command.trim();  // 去除首尾空白字符（包括换行和回车）

    // 判断命令并执行对应操作
    if (command == "1") {
      //注册指纹
      DebugSerial.println("[INFO] registering...");
      if (register_FP()) {
        DebugSerial.println("FP registration process complete!");
      } else {
        DebugSerial.println("FP registration process fail! Please 'register' again!!!");
      }
    } else if (command == "2") {
      //搜索指纹匹配
      DebugSerial.println("[INFO] searching...");
      if (search_FP()) {
        DebugSerial.println("FP match succeed!");
      } else {
        DebugSerial.println("FP match fail! Please 'search' again!!!");
      }
    } else if (command == "3") {
      //清除模组存储的指纹
      DebugSerial.println("[INFO] clearing...");
      if (clear_all_fingerprint()) {
        DebugSerial.println("FP clear FP all lib succeed!");
      } else {
        DebugSerial.println("FP clear FP all lib fail! Please 'clear' again!!!");
      }
    } else if (command == "4") {
      //读取指纹模组的SN
      DebugSerial.println("[INFO] reading chip SN...");
      if (get_module_sn(module_sn)) {
        DebugSerial.println("Chip SN:");
        for (int i = 0; i < 16; i++) {
          if (module_sn[i] < 0x10) DebugSerial.print("0");
          DebugSerial.print(module_sn[i], HEX);
        }
        DebugSerial.print("\n");
      }
    } else if (command == "5") {
      decrypt_password_and_keyin();
      //读取指纹，输出密码
    } else if (command == "6") {
      if (set_module_password(module_pwd)) {
        DebugSerial.println("password set succeed!");
      } else {
        DebugSerial.println("password set failed!");
      }
    }
    //设置模组密码
    else if (command == "7") {
      if (verify_module_password(module_pwd)) {
        DebugSerial.println("password verify succeed!");
      } else {
        DebugSerial.println("password verify failed!");
      }
    }
    //验证模组密码
    else if (command == "8") {
      if (set_module_address(new_module_addr)) {
        DebugSerial.println("address set succeed!");
      } else {
        DebugSerial.println("address set failed!");
      }
      //修改模组地址
    } else if (command == "9") {
      if (module_factory_reset()) {
        DebugSerial.println("module reset succeed!");
      } else {
        DebugSerial.println("module reset failed!");
      }
      //修改模组地址
    } else {
      // 未知命令处理
      DebugSerial.print("[ERROR] unknown cmd: ");
      DebugSerial.println(command);
    }
  }
  return 0;
}

//读模组基本参数
int read_FP_info(void) {
  uint8_t response[32];
  uint8_t index = 0;
  uint32_t startTime = millis();

  DebugSerial.println("ZW101 FPM info:");
  send_cmd(CMD_READ_SYSPARA);

  // 等待响应包
  while (millis() - startTime < 2000) {
    if (fpSerial.available()) {
      response[index++] = fpSerial.read();
      if (index >= 28) break;
    }
  }

  // 打印响应包
  // printResponse(response, index);

  // 检查确认码
  if (index >= 28 && response[9] == 0x00) {
    uint16_t register_cnt = (uint16_t)(response[10] << 8) | response[11];
    uint16_t fp_temp_size = (uint16_t)(response[12] << 8) | response[13];
    uint16_t fp_lib_size = (uint16_t)(response[14] << 8) | response[15];
    uint16_t score_level = (uint16_t)(response[16] << 8) | response[17];
    uint32_t device_addr = (uint32_t)(response[18] << 24) | (response[19] << 16) | (response[20] << 8) | response[21];
    uint16_t data_pack_size = (uint16_t)(response[22] << 8) | response[23];
    if (0 == data_pack_size) {
      data_pack_size = 32;
    } else if (1 == data_pack_size) {
      data_pack_size = 64;
    } else if (2 == data_pack_size) {
      data_pack_size = 128;
    } else if (3 == data_pack_size) {
      data_pack_size = 256;
    }
    uint16_t baud_set = (uint16_t)(response[24] << 8) | response[25];

    DebugSerial.print("register cnt:");
    DebugSerial.println(register_cnt);
    DebugSerial.print("temp size:0x");
    DebugSerial.println(fp_temp_size, HEX);
    DebugSerial.print("lib size:");
    DebugSerial.println(fp_lib_size);
    DebugSerial.print("level:");
    DebugSerial.println(score_level);
    DebugSerial.print("devece address:0x");
    DebugSerial.println(device_addr, HEX);
    DebugSerial.print("data size:");
    DebugSerial.println(data_pack_size);
    DebugSerial.print("baud:");
    DebugSerial.println(baud_set * 9600);
    return 1;  // 成功
  } else {
    return 0;  // 失败
  }
}



//注册指纹
int register_FP(void) {
  BUFFER_ID = 1;
  while (BUFFER_ID <= 5) {
    // 步骤1：获取图像
    send_cmd(CMD_GET_IMAGE);
    // 等待指纹模组响应
    if (verifyResponse()) {
    } else {
      delay(1000);
      continue;
    }

    // 步骤2：生成特征
    send_cmd2(CMD_GEN_CHAR, BUFFER_ID);
    if (verifyResponse()) {
      BUFFER_ID++;
      DebugSerial.println("Press your finger...");
    } else {
      continue;
    }
  }

  // 步骤3：合并特征
  send_cmd(CMD_REG_MODEL);
  if (verifyResponse()) {

  } else {
    return 0;
  }

  // 步骤4：存储模板
  BUFFER_ID = 1;
  sendCommand(CMD_STORE_CHAR, BUFFER_ID, PAGE_ID);
  if (verifyResponse()) {
  } else {
    return 0;
  }
  PAGE_ID += 1;
  //不+1则再次录入后只会覆盖第一位指纹，而不会新增指纹模板
  return 1;
}

//搜索指纹
int search_FP(void) {
  int serch_cnt = 0;
  BUFFER_ID = 1;
  while (serch_cnt <= 5) {
    // 步骤1：获取图像
    send_cmd(CMD_GET_IMAGE);

    // 等待指纹模组响应
    if (verifyResponse()) {

    } else {
      delay(1000);
      continue;
    }

    // 步骤2：生成特征
    send_cmd2(CMD_GEN_CHAR, BUFFER_ID);
    if (verifyResponse()) {
      break;
    } else {
      serch_cnt++;
      delay(500);
      continue;
    }
  }

  // 步骤3：搜索指纹
  BUFFER_ID = 1;
  sendCommand1(CMD_SEARCH, BUFFER_ID, 1, 1);
  if (verifyResponse()) {
    return 1;
  }
  return 0;
}

//清空指纹库
int clear_all_fingerprint(void) {
  send_cmd(CMD_CLEAR_LIB);
  if (verifyResponse()) {
    return 1;
  }
  return 0;
}

// 发送指令包
void send_cmd(uint8_t cmd) {
  uint8_t packet[12];
  uint16_t length = 3;
  uint16_t checksum = 1 + length + cmd;

  // 构建指令包
  packet[0] = HEADER_HIGH;
  packet[1] = HEADER_LOW;
  packet[2] = (DEVICE_ADDRESS >> 24) & 0xFF;
  packet[3] = (DEVICE_ADDRESS >> 16) & 0xFF;
  packet[4] = (DEVICE_ADDRESS >> 8) & 0xFF;
  packet[5] = DEVICE_ADDRESS & 0xFF;
  packet[6] = 0x01;                  // 包标识：命令包
  packet[7] = (length >> 8) & 0xFF;  // 包长度高字节
  packet[8] = length & 0xFF;         // 包长度低字节
  packet[9] = cmd;

  packet[10] = (checksum >> 8) & 0xFF;
  packet[11] = checksum & 0xFF;

#if defined(DEBUG_MODE)
  DebugSerial.println("send_cmd:");
  printHex(packet, 12);
#endif

  // 发送指令包
  for (int i = 0; i < 12; i++) {
    fpSerial.write(packet[i]);
  }
}
// 发送指令包
void send_cmd2(uint8_t cmd, uint8_t param1) {
  uint8_t packet[13];
  uint16_t length = 4;
  uint16_t checksum = 1 + length + cmd + param1;

  // 构建指令包
  packet[0] = HEADER_HIGH;
  packet[1] = HEADER_LOW;
  packet[2] = (DEVICE_ADDRESS >> 24) & 0xFF;
  packet[3] = (DEVICE_ADDRESS >> 16) & 0xFF;
  packet[4] = (DEVICE_ADDRESS >> 8) & 0xFF;
  packet[5] = DEVICE_ADDRESS & 0xFF;
  packet[6] = 0x01;                  // 包标识：命令包
  packet[7] = (length >> 8) & 0xFF;  // 包长度高字节
  packet[8] = length & 0xFF;         // 包长度低字节
  packet[9] = cmd;
  packet[10] = param1;
  packet[11] = (checksum >> 8) & 0xFF;
  packet[12] = checksum & 0xFF;

#if defined(DEBUG_MODE)
  DebugSerial.println("send_cmd2:");
  printHex(packet, 13);
#endif

  // 发送指令包
  for (int i = 0; i < 13; i++) {
    fpSerial.write(packet[i]);
  }
}

// 发送指令包
void sendCommand(uint8_t cmd, uint8_t param1, uint16_t param2) {
  uint8_t packet[15];
  uint16_t length = 6;
  uint16_t checksum = 1 + length + cmd + param1 + (param2 >> 8) + (param2 & 0xFF);

  // 构建指令包
  packet[0] = HEADER_HIGH;
  packet[1] = HEADER_LOW;
  packet[2] = (DEVICE_ADDRESS >> 24) & 0xFF;
  packet[3] = (DEVICE_ADDRESS >> 16) & 0xFF;
  packet[4] = (DEVICE_ADDRESS >> 8) & 0xFF;
  packet[5] = DEVICE_ADDRESS & 0xFF;
  packet[6] = 0x01;                  // 包标识：命令包
  packet[7] = (length >> 8) & 0xFF;  // 包长度高字节
  packet[8] = length & 0xFF;         // 包长度低字节
  packet[9] = cmd;
  packet[10] = param1;
  packet[11] = (param2 >> 8) & 0xFF;
  packet[12] = param2 & 0xFF;
  packet[13] = (checksum >> 8) & 0xFF;
  packet[14] = checksum & 0xFF;

#if defined(DEBUG_MODE)
  DebugSerial.println("send:");
  printHex(packet, (2 + 4 + 3 + length));
#endif

  // 发送指令包
  for (int i = 0; i < (2 + 4 + 3 + length); i++) {
    fpSerial.write(packet[i]);
  }
}

// 发送指令包
void sendCommand1(uint8_t cmd, uint8_t param1, uint16_t param2, uint16_t param3) {
  uint8_t packet[17];
  uint16_t length = 8;
  uint16_t checksum = 1 + length + cmd + param1 + (param2 >> 8) + (param2 & 0xFF) + (param3 >> 8) + (param3 & 0xFF);

  // 构建指令包
  packet[0] = HEADER_HIGH;
  packet[1] = HEADER_LOW;
  packet[2] = (DEVICE_ADDRESS >> 24) & 0xFF;
  packet[3] = (DEVICE_ADDRESS >> 16) & 0xFF;
  packet[4] = (DEVICE_ADDRESS >> 8) & 0xFF;
  packet[5] = DEVICE_ADDRESS & 0xFF;
  packet[6] = 0x01;                  // 包标识：命令包
  packet[7] = (length >> 8) & 0xFF;  // 包长度高字节
  packet[8] = length & 0xFF;         // 包长度低字节
  packet[9] = cmd;
  packet[10] = param1;
  packet[11] = (param2 >> 8) & 0xFF;
  packet[12] = param2 & 0xFF;
  packet[13] = (param3 >> 8) & 0xFF;
  packet[14] = param3 & 0xFF;
  packet[15] = (checksum >> 8) & 0xFF;
  packet[16] = checksum & 0xFF;

#if defined(DEBUG_MODE)
  DebugSerial.println("send:");
  printHex(packet, (2 + 4 + 3 + length));
#endif

  // 发送指令包
  for (int i = 0; i < 2 + 4 + 3 + length; i++) {
    fpSerial.write(packet[i]);
  }
}



// 接收响应包
bool verifyResponse() {
  uint8_t response[50];
  uint8_t index = 0;
  uint32_t startTime = millis();

  // 等待响应包
  while (millis() - startTime < 500) {
    if (fpSerial.available()) {
      response[index++] = fpSerial.read();
    }
  }

  // 打印响应包
#if defined(DEBUG_MODE)
  printResponse(response, index);

#endif

  // 检查确认码
  if (index >= 12 && response[9] == 0x00) {
    return true;  // 成功
  } else {
    return false;  // 失败
  }
}

// 打印响应包
void printResponse(uint8_t* response, uint8_t length) {
  DebugSerial.print("Response:");
  for (int i = 0; i < length; i++) {
    if (response[i] < 0x10) DebugSerial.print('0');
    DebugSerial.print(response[i], HEX);
    DebugSerial.print(" ");
  }
  DebugSerial.println();
}


/*******custom command send to send single 4 bytes command******/
void sendCommand2(uint8_t cmd, uint8_t param1[4]) {
  uint8_t packet[16];
  uint16_t length = 7;
  uint16_t checksum = 1 + length + cmd + param1[0] + param1[1] + param1[2] + param1[3];

  // 构建指令包
  packet[0] = HEADER_HIGH;
  packet[1] = HEADER_LOW;
  packet[2] = (DEVICE_ADDRESS >> 24) & 0xFF;
  packet[3] = (DEVICE_ADDRESS >> 16) & 0xFF;
  packet[4] = (DEVICE_ADDRESS >> 8) & 0xFF;
  packet[5] = DEVICE_ADDRESS & 0xFF;
  packet[6] = 0x01;                  // 包标识：命令包
  packet[7] = (length >> 8) & 0xFF;  // 包长度高字节
  packet[8] = length & 0xFF;         // 包长度低字节
  packet[9] = cmd;
  packet[10] = param1[0];
  packet[11] = param1[1];
  packet[12] = param1[2];
  packet[13] = param1[3];
  packet[14] = (checksum >> 8) & 0xFF;
  packet[15] = checksum & 0xFF;

#if defined(DEBUG_MODE)
  DebugSerial.println("sendCommand2:");
  printHex(packet, 16);
#endif

  // 发送指令包
  for (int i = 0; i < 16; i++) {
    fpSerial.write(packet[i]);
  }
}
/*******custom command send to send single 4 bytes command******/


/*****get serial number for module****/
bool get_module_sn(uint8_t module_sn[16]) {
  uint8_t response[50];
  uint8_t index = 0;
  uint32_t startTime = millis();
  send_cmd2(CMD_READ_CHIPSN, 0);
  // should receive 44 bytes response
  while (millis() - startTime < 2000) {
    if (fpSerial.available()) {
      response[index++] = fpSerial.read();
      if (index >= 44) break;
    }
  }
  if (index < 40) return false;
  for (int i = 0; i < 16; i++) {
    module_sn[i] = response[10 + i];  // 从第11个字节开始（下标10）
  }
  return true;
}
/*****get serial number for module****/


/*****set module password****/
/*******没什么用，密码掉电就丢失，别用这个功能*******/
bool set_module_password(uint8_t module_pwd[4]) {
  sendCommand2(CMD_SET_CHIPPWD, module_pwd);
  return verifyResponse();
}
/*****set module password****/



/*****verify module password****/
bool verify_module_password(uint8_t module_pwd[4]) {
  sendCommand2(CMD_VERIFY_CHIPPWD, module_pwd);
  return verifyResponse();
}
/*****verify module password****/



/*****set module address****/
/******一定要记住修改后的addr，否则连factory reset都无法进行*******/
bool set_module_address(uint8_t new_module_addr[4]) {
  sendCommand2(CMD_SET_CHIPADDR, new_module_addr);
  return verifyResponse();
}
/*****set module address****/

/*****module  factory reset****/
/*******factory reset并不修改addr，而且addr不对时也无法factory reset********/
bool module_factory_reset(void) {
  send_cmd(CMD_FACTORY_RESET);
  return verifyResponse();
}
/*****module factory reset****/

/*****decrypt password with aes****/
/********ChatGPT写的，我也不懂*********/
void decrypt_password_base64(const char* encrypted_base64, char* output_buffer, int output_buffer_len, const uint8_t* aes_key, int key_len) {
  int base64_len = strlen(encrypted_base64);
  char base64_buf[base64_len + 1];
  strcpy(base64_buf, encrypted_base64);

  int decoded_len = Base64.decodedLength(base64_buf, base64_len);
  byte decoded[decoded_len];
  Base64.decode((char*)decoded, base64_buf, base64_len);

  if (decoded_len <= 16) {  // IV + ciphertext 长度不合理
    output_buffer[0] = '\0';
    return;
  }

  byte iv[16];
  memcpy(iv, decoded, 16);  // 前16字节是IV

  byte* ciphertext = decoded + 16;  // 密文指针
  int ciphertext_len = decoded_len - 16;

  int decrypted_len = aesLib.decrypt(ciphertext, ciphertext_len, (byte*)output_buffer, aes_key, key_len, iv);

  if (decrypted_len <= 0) {
    output_buffer[0] = '\0';
    return;
  }

  // 去除PKCS7填充
  byte pad_len = output_buffer[decrypted_len - 1];
  if (pad_len > 0 && pad_len <= 16) {
    decrypted_len -= pad_len;
  }

  if (decrypted_len < output_buffer_len) {
    output_buffer[decrypted_len] = '\0';
  } else {
    output_buffer[output_buffer_len - 1] = '\0';
  }
}

/*****decrypt password with aes****/



/*****test decrypt password with module_sn****/
void decrypt_password_and_keyin() {
  char decrypted[64];
  if (search_FP()) {
    if (get_module_sn(module_sn)) {
      decrypt_password_base64(encrypted_base64, decrypted, sizeof(decrypted), module_sn, 16);
      Keyboard.print(decrypted);
      Keyboard.write(KEY_RETURN);
    }
  }
}
/*****test decrypt password with module_sn****/

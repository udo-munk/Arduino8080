//
// Intel 8080 CPU emulation on an Arduino Nano
//            derived from Z80PACK
// Copyright 2024, Udo Munk
//
// History:
// 19-MAY-2024 implemented configuration dialog
//

#define BS  0x08 // backspace
#define DEL 0x7f // delete

// read a config command line from the terminal
void get_cmdline(char *buf, int len)
{
  int i = 0;
  char c;

  while (i < len - 1) {
    while (!Serial.available())
      ;
    c = Serial.read();
    if ((c == BS) || (c == DEL)) {
      if (i >= 1) {
        Serial.write(BS);
        Serial.write(' ');
        Serial.write(BS);
        i--;
      }
    } else if (c != '\r') {
      buf[i] = c;
      Serial.write(c);
      i++;
    } else {
      break;
    }
  }
  buf[i] = '\0';
}

// configuration dialog for the machine
void config(void)
{
  char s[10];
  int go_flag = 0;

  while (!go_flag) {
    Serial.print(F("1 - port 255 value: 0x"));
    Serial.println(fp_value, HEX);
    Serial.println(F("2 - load file"));
    Serial.println(F("3 - run machine\n"));
    Serial.print(F("Command: "));

    get_cmdline(s, 2);
    Serial.println(F("\n"));

    switch (*s) {
    case '1':
again:
      Serial.print(F("Value: "));
      get_cmdline(s, 3);
      Serial.println(F("\n"));
      if ((*s >= 'a') && (*s <= 'f'))
        *s = toupper(*s);
      if ((*(s + 1) >= 'a') && (*(s + 1) <= 'f'))
        *(s + 1) = toupper(*(s + 1));
      if (!isxdigit(*s) || !isxdigit(*(s + 1))) {
        Serial.println(F("What?"));
        goto again;
      }
      fp_value = (*s <= '9') ? (*s - '0') : (*s - 'A' + 10);
      fp_value <<= 4;
      fp_value += (*(s + 1) <= '9') ? (*(s + 1) - '0') :
                  (*(s + 1) - 'A' + 10);
      break;

    case '2':
      Serial.print(F("Filename: "));
      get_cmdline(s, 9);
      Serial.println();
      load_file(s);
      break;

    case '3':
      go_flag = 1;
      break;

    default:
      break;
    }
  }
}

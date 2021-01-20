#include <Wire.h>
#include <AM2320-int.h>
#include <LiquidCrystal_I2C.h>
#include <GFDS18B20.h>
//На версии параметров плат 1.0.4+ Скетч использует 11 562 байт (70%) памяти устройства.
//Глобальные переменные используют 296 байт (57%) динамической памяти

//На версии параметров плат 1.0.3 Скетч использует 11 338 байт (69%) памяти устройства.
//Глобальные переменные используют 296 байт (57%) динамической памяти

//На версии параметров плат 1.0.2 Скетч использует 11 458 байт (69%) памяти устройства.
//Глобальные переменные используют 296 байт (57%) динамической памяти

//На версии параметров плат 1.0.1 Скетч использует 11 132 байт (67%) памяти устройства.
//Глобальные переменные используют 280 байт (54%) динамической памяти

//23.03.2020
//Скетч использует 12 500 байт (76%) памяти устройства. Всего доступно 16 384 байт.
//Глобальные переменные используют 338 байт (66%) динамической памяти

//24.03.2020
//Скетч использует 12 292 байт (75%) памяти устройства. Всего доступно 16 384 байт.
//Глобальные переменные используют 338 байт (66%) динамической памяти

//16.01.2021
//Скетч использует 13 090 байт (79%) памяти устройства. Всего доступно 16 384 байт.
//Глобальные переменные используют 342 байт (66%) динамической памяти

//18.01.2021
//Скетч использует 13 180 байт (80%) памяти устройства. Всего доступно 16 384 байт.
//Глобальные переменные используют 344 байт (67%) динамической памяти

// Код из библиотеки AM2321 Temperature & Humidity Sensor library for Arduino, сделанной Тимофеевым Е.Н.  (AM2320-master)
//Для сокращения потребления памяти влажность сделана целочисленной, так как всё равно десятые никого не интересуют. Температура тоже целочисленная удесятерённая, потому что write(FLOAT) не выводит после запятой при использовании библтотеки GFDS18B20.


//Uncomment for testing
//#define TEST


// Пины и адреса
#define VbatPin 6
#define FanPin 2
#define ExternPin 7
#define ButtonPin 5
#define OWPIN 8
#define CO2EnPin 11
#define ADC_CONST 41            //цена деления АЦП (1/1024) * 10000

// Пороговые уровни
#define FAN_SKIP 7             //Сколько циклов основной программы пропустить от запуска до остановки вентилятора. Макс 255 По умолчанию 7 (4 секунды)
#define ADC_BAT100 990         //Показание АЦП при 100% заряда аккумулятора (4,2В-1010, 4,05В-990)
#define ADC_BAT0 810            //Показание АЦП при разряженной батарее (3,2В + 0,1В падение на защите, итого реально 3,3В - 780, 3,5В - 828)

#define NO_BACKLIGHT_LEVEL 15   //Уровень заряда для отключения подсветки
#define SLEEP_LEVEL 5        //Уровень заряда для ухода в сон на 5 минут
#define NO_FAN_LEVEL 20           //Уровень заряда для отключения вентилятора и более продолжительного сна

#define CO2_WARN_LEVEL 800
#define CO2_ALARM_LEVEL 1500

byte co2_flag = 0;              // Показатель концентрации CO2: до 800ppm = 0, от 800 до 1200 = 1, от 1200 = 2. Управляет миганием подсветки Оптимизировать до 2 бит
volatile boolean key_pressed = false;    // "Key pressed" flag. "Volatile" modifier makes it changeable in interrupt service routines.
byte backlight_counter = 15;     //счётчик циклов принудительной подсветки, при сбросе на 0 подсветка отключается
byte battery_level = NO_FAN_LEVEL;       //Заряд аккумулятора, по умолчанию 30%
byte fan_counter = 0;            //счётчик на вентилятор
byte cycle_counter = 1;            //счётчик циклов
byte period = 2;                  //периодичность пробуждения
byte fan_period = 10;             //период включения вентилятора
byte extern_period = 255;        //период включения выхода типа открытый сток
unsigned int Vbat = 0;            //напряжение батареи*10000

byte batt_symbol[6][8] =   // Батарейка разной степени заряженности
{{
    B01110,
    B11111,
    B10001,
    B10001,
    B10001,
    B10001,
    B10001,
    B11111
  }, {
    B01110,
    B11111,
    B10001,
    B10001,
    B10001,
    B10001,
    B11111,
    B11111
  }, {
    B01110,
    B11111,
    B10001,
    B10001,
    B10001,
    B11111,
    B11111,
    B11111
  }, {
    B01110,
    B11111,
    B10001,
    B10001,
    B11111,
    B11111,
    B11111,
    B11111
  }, {
    B01110,
    B11111,
    B10001,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111
  },
  {
    B01110,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111,
    B11111
  }
};

byte two[8] =  // Подстрочная цифра 2 для CO2
{
  B00000,
  B00000,
  B00000,
  B01100,
  B10010,
  B00100,
  B01000,
  B11110
};


DS18B20 ds(OWPIN);
LiquidCrystal_I2C lcd(0x3F, 16, 2); // set the LCD address to 0x3f for a 16 chars and 2 line display
AM2320 th; //объявление переменной типа "датчик AM2320"

void setup()
{
  pinMode(ButtonPin, INPUT_PULLUP);
  attachInterrupt(ButtonPin, ButtonISR, FALLING);
  pinMode(FanPin, OUTPUT);
  digitalWrite(FanPin, LOW);
  pinMode(ExternPin, OUTPUT);
  digitalWrite(ExternPin, LOW);
  pinMode(CO2EnPin, OUTPUT);
  digitalWrite(CO2EnPin, LOW);

  analogReference(INTERNAL1V5);
  unsigned int ADCbat = analogRead(VbatPin);
  Vbat = ADCbat * ADC_CONST;
  battery_level = constrain(map(ADCbat, ADC_BAT0, ADC_BAT100, 0, 100), 0, 100);

  if (battery_level > SLEEP_LEVEL) {
    digitalWrite(CO2EnPin, HIGH);
  }

  Serial.begin(9600);
  Wire.setModule(0);
  Wire.begin();
  lcd.init();
  lcd.noBacklight();
  lcd.home();
  lcd.print("Vbat="); printDec(Vbat / 1000);
  lcd.setCursor(0, 1);
  for (byte i = 0; i < 16; i++) {
    lcd.write(255);

#ifdef TEST
    sleep (300);                    //fast boot for testing
#else
    sleepSeconds(1);
#endif

  }
  lcd.createChar(1, two);
  for (byte i = 2; i <= 7; i++) {
    lcd.createChar(i, batt_symbol[i - 2]);
  }
  lcd_draw_template();

}


void loop()
{
#ifdef TEST
  lcd.home();
  lcd.print(cycle_counter);
#endif

  if (key_pressed) {                                                  //Обработка нажатия кнопки
    if (battery_level) lcd.backlight();
    lcd.clear();
    lcd.home();
    unsigned int ADCbat = analogRead(VbatPin);
    Vbat = ADCbat * ADC_CONST;
    lcd.print("Bat level "); lcd.print(battery_level); lcd.setCursor(0, 1); lcd.print("ADC="); lcd.print(ADCbat); lcd.print(" V="); printDec(Vbat / 1000);
    while (!digitalRead(ButtonPin)) {                                                   //Detect button hold
      sleep(100);
    }
    key_pressed = false;
    backlight_counter = 15;
    fan_counter = FAN_SKIP;
    cycle_counter = 0;
    lcd_draw_template();
  };

  if (!(cycle_counter % period)) {                                                              //если остаток от деления = 0, то программа выполняется, иначе спим секунду
    unsigned int ADCbat = analogRead(VbatPin);
    battery_level = (battery_level * 3 + constrain(map(ADCbat, ADC_BAT0, ADC_BAT100, 0, 100), 0, 100)) / 4;
    if (battery_level < SLEEP_LEVEL) {   //10% Когда аккумулятор разряжен, просыпаться раз в ~3 минуты
      period = 254;
      fan_period = 255;                  //и полностью отключить вентилятор
    }
    else {

      if (battery_level < NO_FAN_LEVEL) {   //30% Когда не жалко энергии, покрутить вентилятором раз в 38 секунд. Если жалко, крутим раз в 3 минуты и просыпаемся раз в 30 секунд
        period = 42;
        fan_period = 254;
      }
      else {
        period = 6;
        fan_period = 50;
      }
    }
    /*if (((co2_flag == 1) || backlight_counter) && battery_level >= NO_BACKLIGHT_LEVEL) {
      lcd.backlight();
      digitalWrite(ExternPin, HIGH);
      }
      else {
      lcd.noBacklight();
      digitalWrite(ExternPin, LOW);
      }*/
    switch (co2_flag) {
      case 1:
        extern_period = 11;
        break;
      case 2:
        extern_period = 2;
        break;
      default:
        extern_period = 255;
        break;
    }

    co2_flag = 0;

    //Температура, влажность
    if (th.Read() == 0) {
      lcd.setCursor(2, 1);
      //lcd.print(th.t / 10);
      //lcd.print(".");
      //lcd.print(abs(th.t % 10));
      printDec(th.t);
      lcd.write(223); lcd.print("C ");
      lcd.setCursor(14, 1);
      lcd.print(" ");
      lcd.setCursor(13, 1);
      lcd.print(th.h);
    }

    //Углекислый газ
    lcd.setCursor(4, 0);
    if (battery_level > SLEEP_LEVEL) {
      digitalWrite(CO2EnPin, HIGH);
      byte cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
      Serial.write(cmd, 9);
      char response[9];
      Serial.readBytes(response, 9);

      // CRC check
      byte i;
      byte crc = 0;
      for (i = 1; i < 8; i++) crc += response[i];
      crc = 255 - crc;
      crc++;
      // End of CRC check

      //вывод значения CO2
      if ( !((byte)response[0] == 0xFF && (byte)response[1] == 0x86 && (byte)response[8] == crc)) {
        lcd.print("offline");
      } else {
        int ppm = (256 * (byte)response[2]) + (byte)response[3];

        if (ppm < 300 || ppm > 5000) ppm = 0;
        if (ppm > CO2_WARN_LEVEL && ppm <= CO2_ALARM_LEVEL) co2_flag = 1;
        if (ppm > CO2_ALARM_LEVEL && ppm <= 5000) co2_flag = 2;

        lcd.print("       ");
        lcd.setCursor(4, 0);
        lcd.print(ppm);
        lcd.print("ppm");
      }
    } else {
      digitalWrite(CO2EnPin, LOW);
      lcd.print("LOW BAT");
    }


    //Температура внешнего датчика DS18B20
    lcd.setCursor(11, 0);
    if (!ds.reset()) {
      int32_t DStemp = ReadTemp();
      printDec(DStemp);
      tempCMD();
    } else {
      lcd.print("    ");
    }

    draw_battery(battery_level);
  }                 /////////Конец рабочего пробуждения
  
  if (backlight_counter) {
    //if (battery_level) lcd.backlight();
    lcd.setBacklight(battery_level);
    backlight_counter--;
  }
  else {
    lcd.noBacklight();
  }
 
  if ((!(cycle_counter % extern_period)) && battery_level) {
    digitalWrite(ExternPin, HIGH);
  }
  if (!(cycle_counter % fan_period)) {
    fan_counter = FAN_SKIP;
  }
  if (fan_counter && battery_level) {
    fan_counter--;
    digitalWrite(FanPin, HIGH);
  } else {
    digitalWrite(FanPin, LOW);
  }
  if (cycle_counter++ == 254) cycle_counter = 1;

  sleep(500);
  digitalWrite(ExternPin, LOW);
  if (!battery_level) suspend();                                // Если аккумулятор разряжен до 0, войти в спячку и просыпаться только по кнопке без вентилятора и подсветки
}

////////////Functions

void printDec(int32_t ui)
{
  lcd.print(ui / 10, DEC);
  lcd.print(".");
  lcd.print(abs(ui % 10), DEC);
}

void lcd_draw_template() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CO\1:");
  lcd.setCursor(0, 1);
  lcd.print("T:");
  lcd.setCursor(10, 1);
  lcd.print("RH:  %");
}

void draw_battery(byte percent)           //Вывод символа батарейки
{
  //lcd.createChar(2, batt_symbol[round(percent/20)]);
  lcd.setCursor(15, 0);
  //lcd.print("\2");
  lcd.write(round(percent / 20.0) + 2);

}

void ButtonISR() {                       //Обработчик прерывания кнопки
  key_pressed = true;
  wakeup();
}

void tempCMD(void) {     //Send a global temperature convert command
  ds.reset();
  ds.write_byte(0xcc);  // was ds.select(work); so request all OW's to do next command
  ds.write_byte(0x44);  // start conversion, with parasite power on at the end
  //delay(1000);        // При использовании не забывать дать время 750мс на преобразование температуры
}

int32_t ReadTemp(void)
{
  uint16_t temp;
  ds.reset();
  ds.write_byte(0xcc); // skip ROM command
  ds.write_byte(0xbe); // read scratchpad command
  temp = ds.ReadDS1820();
  int16_t stemp = (int16_t)temp;
  return ((int32_t)stemp * 6250 / 10000);
}

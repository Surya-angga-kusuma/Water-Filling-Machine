#include <Keypad.h>
#include <LiquidCrystal_I2C.h>

const int ROW_NUM = 4; // empat baris
const int COLUMN_NUM = 3; // tiga kolom

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3' },
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

LiquidCrystal_I2C lcd(0x27, 20, 4); // baris 8
byte pin_rows[ROW_NUM] = {9, 8, 7, 6}; // terhubung ke pin baris keypad
byte pin_column[COLUMN_NUM] = {5, 4, 3}; // terhubung ke pin kolom keypad
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

int sensorInterrupt = 0;  // interrupt 0
int sensorPin       = 2; //Pin Digital 2
int solenoidValve = 5; // Pin Digital 5
unsigned int SetPoint = 400; //400 mililiter
String code = "";
/* Sensor aliran hall-effect mengeluarkan pulsa per detik per liter/menit aliran. */
float calibrationFactor = 90; // Anda dapat mengubahnya sesuai dengan lembar data Anda

volatile byte pulseCount = 0;

float flowRate = 0.0;
unsigned int flowMilliLitres = 0;
unsigned long totalMilliLitres = 0, volume = 0;

unsigned long oldTime;
const int relais_moteur = 13; // relay terhubung ke pin 3 papan Adruino

void setup() {
  totalMilliLitres = 0;
  pinMode(relais_moteur, OUTPUT);
  lcd.init(); // inisialisasi tampilan
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(5, 1);
  lcd.print("PROACTIVE");
  lcd.setCursor(5, 2);
  lcd.print("ROBOTIKAA");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Proactive Education");
  lcd.setCursor(2, 1);
  lcd.print("Masukkan Volume:");
  lcd.setCursor(0, 3);
  lcd.print("Tekan * Untuk Mulai");

  // Inisialisasi koneksi serial untuk melaporkan nilai ke host
  Serial.begin(9600);
  pinMode(solenoidValve , OUTPUT);
  digitalWrite(solenoidValve, HIGH);
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);
  /* Sensor hall-effect terhubung ke pin 2 yang menggunakan interrupt 0. Dikonfigurasi untuk dipicu pada perubahan keadaan FALLING (transisi dari tinggi ke rendah)*/
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING); // Anda dapat menggunakan Rising atau Falling
}

void loop() {
  char key = keypad.getKey();

  if (key) // Tombol pada keyboard ditekan
  {
    code += key;
    lcd.setCursor(0, 2); // berdiri di baris kedua
    lcd.print("Volume : ");
    lcd.print(code);  // menampilkan nilai volume
    lcd.print("ml");
    //    lcd.setCursor(0, 3);
    //    lcd.print("Tekan # Untuk Mulai");
    delay(100);
  }


  if (key == '*') { // jika Anda menekan tombol 'D'
    if (code.toInt() <= 1500)
    {
      volume = code.toInt();
      lcd.clear();
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print("Proactive Education");
      lcd.setCursor(0, 1);
      lcd.print("Sedang Mengisi.......");
      lcd.setCursor(0, 2);
      lcd.print("debit : 0 ml/detik");
      lcd.setCursor(0, 3);
      lcd.print("volume: 0 ml");
      totalMilliLitres = 0; // Reset totalMilliLitres untuk memulai pengisian baru
      digitalWrite(relais_moteur, HIGH); // Mulai pompa air

    }
    else
    {
      lcd.clear();
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print("Proactive Education");
      lcd.setCursor(2, 1);
      lcd.print("Masukkan Volume:");
    }
    code = "";
  }

  if (totalMilliLitres < volume)
  {
    digitalWrite(relais_moteur, HIGH); // Mulai pompa air

    if ((millis() - oldTime) > 1000)    // Hanya memproses hitungan satu kali per detik
    {
      // Menonaktifkan interrupt saat menghitung laju aliran dan mengirimkan nilai ke host
      detachInterrupt(sensorInterrupt);

      // Karena loop ini mungkin tidak selesai dalam interval 1 detik yang tepat, kami menghitung jumlah milidetik yang telah berlalu sejak eksekusi terakhir dan menggunakannya untuk menyesuaikan output. Kami juga menerapkan calibrationFactor untuk mengukur output berdasarkan jumlah pulsa per detik per satuan ukur (liter/menit dalam kasus ini) yang berasal dari sensor.
      flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;

      // Catat waktu eksekusi proses ini. Perhatikan bahwa karena kami menonaktifkan interrupt, fungsi millis() sebenarnya tidak akan bertambah tepat pada titik ini, tetapi itu masih akan mengembalikan nilai yang diatur tepat sebelum interrupt dimatikan.
      oldTime = millis();

      // Bagi laju aliran dalam liter/menit dengan 60 untuk menentukan berapa liter yang telah melewati sensor dalam interval 1 detik ini, kemudian kali 1000 untuk mengonversi ke mililiter.
      flowMilliLitres = (flowRate / 60) * 1000;

      // Tambahkan mililiter yang lewat dalam detik ini ke total kumulatif
      totalMilliLitres += flowMilliLitres;

      unsigned int frac;
      lcd.clear();
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print("Proactive Education");
      lcd.setCursor(0, 1);
      lcd.print("Sedang Mengisi.......");
      lcd.setCursor(0, 2);
      lcd.print("debit :");
      lcd.print(flowMilliLitres);  // Tampilkan laju aliran pada layar lcd
      lcd.print(" ml/detik");
      lcd.setCursor(0, 3);
      lcd.print("volume:");
      lcd.print(totalMilliLitres);  // Tampilkan jumlah yang diisi
      lcd.print(" ml");

      if (totalMilliLitres > 5)
      {
        SetSolinoidValve();
      }

      // Reset hitungan pulsa agar kita dapat mulai menghitung lagi
      pulseCount = 0;

      // Aktifkan interrupt lagi sekarang setelah kita selesai mengirim output
      attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
    }
    if (totalMilliLitres >= volume)
    {
      digitalWrite(relais_moteur, LOW);
      lcd.clear();
      lcd.backlight();
      lcd.setCursor(0, 0);
      lcd.print("Proactive Education");
      lcd.setCursor(0, 1);
      lcd.print(" Pengisian Selesai!");
      delay(2000);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Proactive Education");
      lcd.setCursor(2, 1);
      lcd.print("Masukkan Volume:");
      lcd.setCursor(0, 3);
      lcd.print("Tekan * Untuk Mulai");
    }
  }

  else
  {
    digitalWrite(relais_moteur, LOW); // Berhenti pompa air
    volume = 0;
  }


}

// Rutin Pelayanan Insterrupt
void pulseCounter() {
  // Tambahkan hitungan pulsa
  pulseCount++;
}

void SetSolinoidValve()
{
  digitalWrite(solenoidValve, LOW);
}

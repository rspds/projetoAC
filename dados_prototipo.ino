#define TEMP1 4
#define CORRENTE_PORTA 1
#define TEMPO_PROCESSAMENTO 10000

#include <OneWire.h>
#include <EmonLib.h>
#include <SD.h>
#include <EEPROM.h>

OneWire  dsTemp1(TEMP1);
EnergyMonitor emon1;
File arq_corrente;
File arq_temepraura;

void contador();
void inicializacao_SD();
void leitura();
float lerTemp1();
float conversao(int16_t raw, byte data[], byte type_s);
byte chip(byte addr);

double Irms;
int rede = 220.0;
float temp;
bool chave = false;

void setup()
{
	EEPROM.write(0,0);
	
	SD.begin(10);

	//inicializacao_SD();
	
	Serial.begin(9600);

	emon1.current(CORRENTE_PORTA, 27);
}
/*
falta:
led piscando quando estiver em funcionamento
leds piscando diferente a apartir do tipo de funcionamento dele
botao para iniciar

*/
void loop()
{
	leitura();
	
	if(!digitalRead(but))   t_but = 0x01;
	if(digitalRead(but) && t_but) {
		t_but = 0x00;
		chave = !chave;
		if(chave)
			inicializacao_SD();
	}	

	if(chave)
	{
		arq_corrente.println(Irms);
		arq_temperatura.println(temp);
	}

	while(millis()-time < TEMPO_PROCESSAMENTO)
		delay(1);

	time = millis();

}

void contador()
{
	byte x = EEPROM.read(0);
	if(x==255)
		x=0;
	EEPROM.write(0, x+1);
}

void inicializacao_SD()
{	
	byte x = contador();

	String titulo1 = "Corrente";
	titulo1 += x;
	titulo1 += ".txt"
	String titulo2 = "Temperatura";
	titulo2 += x;
	titulo2 += ".txt"

	arq_corrente = SD.open(titulo1, FILE_WRITE);
	arq_temperatura = SD.open(titulo2, FILE_WRITE);

	if(arq_corrente) {
		arq_corrente.println("Teste de arquivos TXT em SD no Arduino");
		Serial.println("OK.");
	}
	else 
		Serial.println("Erro ao abrir ou criar o arquivo texto.txt.");


	if(arq_temperatura) {
		arq_temperatura.println("Teste de arquivos TXT em SD no Arduino");
		Serial.println("OK.");
	}
	else 
		Serial.println("Erro ao abrir ou criar o arquivo texto.txt.");
}

void leitura()
{
	double Irms = emon1.calcIrms(1480);
	Serial.print("Corrente : ");
	Serial.print(Irms); // Irms
	Serial.print("Potencia : ");
	Serial.println(Irms*rede);
	temp = lerTemp1();
	Serial.print("Temperatura: ");
	Serial.println(temp);
}

float lerTemp1()
{
	byte i;
	byte type_s;
	byte data[12];
	byte addr[8];
	float celsius;
	bool flag = true;

	do
	{
		dsTemp1.reset_search();

		if (!dsTemp1.search(addr))
			return -1;

		if (OneWire::crc8(addr, 7) != addr[7])
			continue;

		type_s = chip(addr[0]);

		if (type_s == -1)
			return -1;

		dsTemp1.reset();
		dsTemp1.select(addr);
		dsTemp1.write(0x44);        // start conversion, use ds.write(0x44,1) with parasite power on at the end

		delay(1000);

		if (dsTemp1.reset() == false)
			return -1;
		dsTemp1.select(addr);
		dsTemp1.write(0xBE);         // Read Scratchpad

		for ( i = 0; i < 9; i++)    // we need 9 bytes
			data[i] = dsTemp1.read();
		if (OneWire::crc8(data, 8) != data[8])
			continue;

		flag = false;

	} while (flag);

	int16_t raw = (data[1] << 8) | data[0];

	celsius = conversao(raw, data, type_s);

	return celsius;
}

float conversao(int16_t raw, byte data[], byte type_s)
{
	if (type_s) {
		raw = raw << 3; // 9 bit resolution default
		if (data[7] == 0x10) {
			raw = (raw & 0xFFF0) + 12 - data[6];
		}
	} else {
		byte cfg = (data[4] & 0x60);
		if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
		else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
	}
	return (float)raw / 16.0;
}

byte chip(byte addr)
{
	byte type_s;

	switch (addr) {
		case 0x10:
			type_s = 1;
			break;
		case 0x28:
			type_s = 0;
			break;
		case 0x22:
			type_s = 0;
			break;
		default:
			return -1;
	}
	return type_s;
}

